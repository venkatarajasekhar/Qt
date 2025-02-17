// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/power_monitor_broadcast_source.h"

#include "base/message_loop/message_loop.h"
#include "content/common/power_monitor_messages.h"
#include "ipc/message_filter.h"

namespace content {

class PowerMessageFilter : public IPC::MessageFilter {
 public:
  PowerMessageFilter(
      PowerMonitorBroadcastSource* source,
      scoped_refptr<base::MessageLoopProxy> message_loop)
      : source_(source),
        message_loop_(message_loop) {
  }

  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE {
    bool handled = true;
    IPC_BEGIN_MESSAGE_MAP(PowerMessageFilter, message)
      IPC_MESSAGE_HANDLER(PowerMonitorMsg_PowerStateChange, OnPowerStateChange)
      IPC_MESSAGE_HANDLER(PowerMonitorMsg_Suspend, OnSuspend)
      IPC_MESSAGE_HANDLER(PowerMonitorMsg_Resume, OnResume)
      IPC_MESSAGE_UNHANDLED(handled = false)
    IPC_END_MESSAGE_MAP()
    return handled;
  }

  void RemoveSource() {
    source_ = NULL;
  }

 private:
  friend class base::RefCounted<PowerMessageFilter>;

  virtual ~PowerMessageFilter() {};

  void OnPowerStateChange(bool on_battery_power) {
    message_loop_->PostTask(FROM_HERE,
        base::Bind(&PowerMessageFilter::NotifySourcePowerStateChange, this,
            on_battery_power));
  }
  void OnSuspend() {
    message_loop_->PostTask(FROM_HERE,
        base::Bind(&PowerMessageFilter::NotifySourceSuspend, this));
  }
  void OnResume() {
    message_loop_->PostTask(FROM_HERE,
        base::Bind(&PowerMessageFilter::NotifySourceResume, this));
  }

  void NotifySourcePowerStateChange(bool on_battery_power) {
    if (source_)
      source_->OnPowerStateChange(on_battery_power);
  }
  void NotifySourceSuspend() {
    if (source_)
      source_->OnSuspend();
  }
  void NotifySourceResume() {
     if (source_)
      source_->OnResume();
  }

  // source_ should only be accessed on the thread associated with
  // message_loop_.
  PowerMonitorBroadcastSource* source_;
  scoped_refptr<base::MessageLoopProxy> message_loop_;

  DISALLOW_COPY_AND_ASSIGN(PowerMessageFilter);
};

PowerMonitorBroadcastSource::PowerMonitorBroadcastSource()
    : last_reported_battery_power_state_(false) {
  message_filter_ = new PowerMessageFilter(this,
      base::MessageLoopProxy::current());
}

PowerMonitorBroadcastSource::~PowerMonitorBroadcastSource() {
  message_filter_->RemoveSource();
}

IPC::MessageFilter* PowerMonitorBroadcastSource::GetMessageFilter() {
  return message_filter_.get();
}

bool PowerMonitorBroadcastSource::IsOnBatteryPowerImpl() {
  return last_reported_battery_power_state_;
}

void PowerMonitorBroadcastSource::OnPowerStateChange(bool on_battery_power) {
  last_reported_battery_power_state_ = on_battery_power;
  ProcessPowerEvent(PowerMonitorSource::POWER_STATE_EVENT);
}

void PowerMonitorBroadcastSource::OnSuspend() {
  ProcessPowerEvent(PowerMonitorSource::SUSPEND_EVENT);
}

void PowerMonitorBroadcastSource::OnResume() {
  ProcessPowerEvent(PowerMonitorSource::RESUME_EVENT);
}

}  // namespace content
