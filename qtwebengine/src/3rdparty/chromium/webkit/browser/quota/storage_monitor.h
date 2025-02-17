// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_BROWSER_QUOTA_STORAGE_MONITOR_H_
#define WEBKIT_BROWSER_QUOTA_STORAGE_MONITOR_H_

#include <map>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "webkit/browser/quota/storage_observer.h"

namespace content {
class StorageMonitorTestBase;
}

namespace quota {

class QuotaManager;

// This class dispatches storage events to observers of a common
// StorageObserver::Filter.
class WEBKIT_STORAGE_BROWSER_EXPORT_PRIVATE StorageObserverList {
 public:
  StorageObserverList();
  virtual ~StorageObserverList();

  // Adds/removes an observer.
  void AddObserver(StorageObserver* observer,
                   const StorageObserver::MonitorParams& params);
  void RemoveObserver(StorageObserver* observer);

  // Returns the number of observers.
  int ObserverCount() const;

  // Forwards a storage change to observers. The event may be dispatched
  // immediately to an observer or after a delay, depending on the desired event
  // rate of the observer.
  void OnStorageChange(const StorageObserver::Event& event);

  // Dispatch an event to observers that require it.
  void MaybeDispatchEvent(const StorageObserver::Event& event);

  // Ensure the specified observer receives the next dispatched event.
  void ScheduleUpdateForObserver(StorageObserver* observer);

 private:
  struct WEBKIT_STORAGE_BROWSER_EXPORT_PRIVATE ObserverState {
    GURL origin;
    base::TimeTicks last_notification_time;
    base::TimeDelta rate;
    bool requires_update;

    ObserverState();
  };
  typedef std::map<StorageObserver*, ObserverState> StorageObserverStateMap;

  void DispatchPendingEvent();

  StorageObserverStateMap observers_;
  base::OneShotTimer<StorageObserverList> notification_timer_;
  StorageObserver::Event pending_event_;

  friend class content::StorageMonitorTestBase;

  DISALLOW_COPY_AND_ASSIGN(StorageObserverList);
};


// Manages the storage observers of a common host. Caches the usage and quota of
// the host to avoid accumulating for every change.
class WEBKIT_STORAGE_BROWSER_EXPORT_PRIVATE HostStorageObservers {
 public:
  explicit HostStorageObservers(QuotaManager* quota_manager);
  virtual ~HostStorageObservers();

  bool is_initialized() const { return initialized_; }

  // Adds/removes an observer.
  void AddObserver(
      StorageObserver* observer,
      const StorageObserver::MonitorParams& params);
  void RemoveObserver(StorageObserver* observer);
  bool ContainsObservers() const;

  // Handles a usage change.
  void NotifyUsageChange(const StorageObserver::Filter& filter, int64 delta);

 private:
  void StartInitialization(const StorageObserver::Filter& filter);
  void GotHostUsageAndQuota(const StorageObserver::Filter& filter,
                            QuotaStatusCode status,
                            int64 usage,
                            int64 quota);
  void DispatchEvent(const StorageObserver::Filter& filter, bool is_update);

  QuotaManager* quota_manager_;
  StorageObserverList observers_;

  // Flags used during initialization of the cached properties.
  bool initialized_;
  bool initializing_;
  bool event_occurred_before_init_;
  int64 usage_deltas_during_init_;

  // Cached accumulated usage and quota for the host.
  int64 cached_usage_;
  int64 cached_quota_;

  base::WeakPtrFactory<HostStorageObservers> weak_factory_;

  friend class content::StorageMonitorTestBase;

  DISALLOW_COPY_AND_ASSIGN(HostStorageObservers);
};


// Manages the observers of a common storage type.
class WEBKIT_STORAGE_BROWSER_EXPORT_PRIVATE StorageTypeObservers {
 public:
  explicit StorageTypeObservers(QuotaManager* quota_manager);
  virtual ~StorageTypeObservers();

  // Adds and removes an observer.
  void AddObserver(StorageObserver* observer,
                   const StorageObserver::MonitorParams& params);
  void RemoveObserver(StorageObserver* observer);
  void RemoveObserverForFilter(StorageObserver* observer,
                               const StorageObserver::Filter& filter);

  // Returns the observers of a specific host.
  const HostStorageObservers* GetHostObservers(const std::string& host) const;

  // Handles a usage change.
  void NotifyUsageChange(const StorageObserver::Filter& filter, int64 delta);

 private:
  typedef std::map<std::string, HostStorageObservers*> HostObserversMap;

  QuotaManager* quota_manager_;
  HostObserversMap host_observers_map_;

  DISALLOW_COPY_AND_ASSIGN(StorageTypeObservers);
};


// Storage monitor manages observers and dispatches storage events to them.
class WEBKIT_STORAGE_BROWSER_EXPORT_PRIVATE StorageMonitor {
 public:
  explicit StorageMonitor(QuotaManager* quota_manager);
  virtual ~StorageMonitor();

  // Adds and removes an observer.
  void AddObserver(StorageObserver* observer,
                   const StorageObserver::MonitorParams& params);
  void RemoveObserver(StorageObserver* observer);
  void RemoveObserverForFilter(StorageObserver* observer,
                               const StorageObserver::Filter& filter);

  // Returns the observers of a specific storage type.
  const StorageTypeObservers* GetStorageTypeObservers(
      StorageType storage_type) const;

  // Handles a usage change.
  void NotifyUsageChange(const StorageObserver::Filter& filter, int64 delta);

 private:
  typedef std::map<StorageType, StorageTypeObservers*> StorageTypeObserversMap;

  QuotaManager* quota_manager_;
  StorageTypeObserversMap storage_type_observers_map_;

  DISALLOW_COPY_AND_ASSIGN(StorageMonitor);
};

}  // namespace quota

#endif  // WEBKIT_BROWSER_QUOTA_STORAGE_MONITOR_H_
