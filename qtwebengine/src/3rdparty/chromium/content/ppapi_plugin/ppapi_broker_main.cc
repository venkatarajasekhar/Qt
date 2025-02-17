// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"
#include "content/child/child_process.h"
#include "content/common/content_constants_internal.h"
#include "content/ppapi_plugin/ppapi_thread.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"

namespace content {

// Main function for starting the PPAPI broker process.
int PpapiBrokerMain(const MainFunctionParams& parameters) {
  const CommandLine& command_line = parameters.command_line;
  if (command_line.HasSwitch(switches::kPpapiStartupDialog)) {
    ChildProcess::WaitForDebugger("PpapiBroker");
  }

  base::MessageLoop main_message_loop;
  base::PlatformThread::SetName("CrPPAPIBrokerMain");
  base::debug::TraceLog::GetInstance()->SetProcessName("PPAPI Broker Process");
  base::debug::TraceLog::GetInstance()->SetProcessSortIndex(
      kTraceEventPpapiBrokerProcessSortIndex);

  ChildProcess ppapi_broker_process;
  ppapi_broker_process.set_main_thread(
      new PpapiThread(parameters.command_line, true));  // Broker.

  main_message_loop.Run();
  DVLOG(1) << "PpapiBrokerMain exiting";
  return 0;
}

}  // namespace content
