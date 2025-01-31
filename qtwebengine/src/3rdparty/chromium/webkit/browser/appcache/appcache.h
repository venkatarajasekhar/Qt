// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_BROWSER_APPCACHE_APPCACHE_H_
#define WEBKIT_BROWSER_APPCACHE_APPCACHE_H_

#include <map>
#include <set>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "url/gurl.h"
#include "webkit/browser/appcache/appcache_database.h"
#include "webkit/browser/appcache/appcache_entry.h"
#include "webkit/browser/appcache/manifest_parser.h"
#include "webkit/browser/webkit_storage_browser_export.h"

namespace net {
class IOBuffer;
}

namespace content {
FORWARD_DECLARE_TEST(AppCacheTest, InitializeWithManifest);
FORWARD_DECLARE_TEST(AppCacheTest, ToFromDatabaseRecords);
class AppCacheTest;
class AppCacheStorageImplTest;
class AppCacheUpdateJobTest;
}

namespace appcache {

class AppCacheExecutableHandler;
class AppCacheGroup;
class AppCacheHost;
class AppCacheStorage;

// Set of cached resources for an application. A cache exists as long as a
// host is associated with it, the cache is in an appcache group or the
// cache is being created during an appcache upate.
class WEBKIT_STORAGE_BROWSER_EXPORT AppCache
    : public base::RefCounted<AppCache> {
 public:
  typedef std::map<GURL, AppCacheEntry> EntryMap;
  typedef std::set<AppCacheHost*> AppCacheHosts;

  AppCache(AppCacheStorage* storage, int64 cache_id);

  int64 cache_id() const { return cache_id_; }

  AppCacheGroup* owning_group() const { return owning_group_.get(); }

  bool is_complete() const { return is_complete_; }
  void set_complete(bool value) { is_complete_ = value; }

  // Adds a new entry. Entry must not already be in cache.
  void AddEntry(const GURL& url, const AppCacheEntry& entry);

  // Adds a new entry or modifies an existing entry by merging the types
  // of the new entry with the existing entry. Returns true if a new entry
  // is added, false if the flags are merged into an existing entry.
  bool AddOrModifyEntry(const GURL& url, const AppCacheEntry& entry);

  // Removes an entry from the EntryMap, the URL must be in the set.
  void RemoveEntry(const GURL& url);

  // Do not store or delete the returned ptr, they're owned by 'this'.
  AppCacheEntry* GetEntry(const GURL& url);
  const AppCacheEntry* GetEntryWithResponseId(int64 response_id) {
    return GetEntryAndUrlWithResponseId(response_id, NULL);
  }
  const AppCacheEntry* GetEntryAndUrlWithResponseId(
      int64 response_id, GURL* optional_url);
  const EntryMap& entries() const { return entries_; }

  // The AppCache owns the collection of executable handlers that have
  // been started for this instance. The getter looks up an existing
  // handler returning null if not found, the GetOrCreate method will
  // cons one up if not found.
  // Do not store the returned ptrs, they're owned by 'this'.
  AppCacheExecutableHandler* GetExecutableHandler(int64 response_id);
  AppCacheExecutableHandler* GetOrCreateExecutableHandler(
      int64 response_id, net::IOBuffer* handler_source);

  // Returns the URL of the resource used as entry for 'namespace_url'.
  GURL GetFallbackEntryUrl(const GURL& namespace_url) const {
    return GetNamespaceEntryUrl(fallback_namespaces_, namespace_url);
  }
  GURL GetInterceptEntryUrl(const GURL& namespace_url) const {
    return GetNamespaceEntryUrl(intercept_namespaces_, namespace_url);
  }

  AppCacheHosts& associated_hosts() { return associated_hosts_; }

  bool IsNewerThan(AppCache* cache) const {
    // TODO(michaeln): revisit, the system clock can be set
    // back in time which would confuse this logic.
    if (update_time_ > cache->update_time_)
      return true;

    // Tie breaker. Newer caches have a larger cache ID.
    if (update_time_ == cache->update_time_)
      return cache_id_ > cache->cache_id_;

    return false;
  }

  base::Time update_time() const { return update_time_; }

  int64 cache_size() const { return cache_size_; }

  void set_update_time(base::Time ticks) { update_time_ = ticks; }

  // Initializes the cache with information in the manifest.
  // Do not use the manifest after this call.
  void InitializeWithManifest(Manifest* manifest);

  // Initializes the cache with the information in the database records.
  void InitializeWithDatabaseRecords(
      const AppCacheDatabase::CacheRecord& cache_record,
      const std::vector<AppCacheDatabase::EntryRecord>& entries,
      const std::vector<AppCacheDatabase::NamespaceRecord>& intercepts,
      const std::vector<AppCacheDatabase::NamespaceRecord>& fallbacks,
      const std::vector<AppCacheDatabase::OnlineWhiteListRecord>& whitelists);

  // Returns the database records to be stored in the AppCacheDatabase
  // to represent this cache.
  void ToDatabaseRecords(
      const AppCacheGroup* group,
      AppCacheDatabase::CacheRecord* cache_record,
      std::vector<AppCacheDatabase::EntryRecord>* entries,
      std::vector<AppCacheDatabase::NamespaceRecord>* intercepts,
      std::vector<AppCacheDatabase::NamespaceRecord>* fallbacks,
      std::vector<AppCacheDatabase::OnlineWhiteListRecord>* whitelists);

  bool FindResponseForRequest(const GURL& url,
      AppCacheEntry* found_entry, GURL* found_intercept_namespace,
      AppCacheEntry* found_fallback_entry, GURL* found_fallback_namespace,
      bool* found_network_namespace);

  // Populates the 'infos' vector with an element per entry in the appcache.
  void ToResourceInfoVector(AppCacheResourceInfoVector* infos) const;

  static const Namespace* FindNamespace(
      const NamespaceVector& namespaces,
      const GURL& url);

 private:
  friend class AppCacheGroup;
  friend class AppCacheHost;
  friend class content::AppCacheTest;
  friend class content::AppCacheStorageImplTest;
  friend class content::AppCacheUpdateJobTest;
  friend class base::RefCounted<AppCache>;

  ~AppCache();

  // Use AppCacheGroup::Add/RemoveCache() to manipulate owning group.
  void set_owning_group(AppCacheGroup* group) { owning_group_ = group; }

  // FindResponseForRequest helpers
  const Namespace* FindInterceptNamespace(const GURL& url) {
    return FindNamespace(intercept_namespaces_, url);
  }
  const Namespace* FindFallbackNamespace(const GURL& url) {
    return FindNamespace(fallback_namespaces_, url);
  }
  bool IsInNetworkNamespace(const GURL& url) {
    return FindNamespace(online_whitelist_namespaces_, url) != NULL;
  }

  GURL GetNamespaceEntryUrl(const NamespaceVector& namespaces,
                            const GURL& namespace_url) const;

  // Use AppCacheHost::Associate*Cache() to manipulate host association.
  void AssociateHost(AppCacheHost* host) {
    associated_hosts_.insert(host);
  }
  void UnassociateHost(AppCacheHost* host);

  const int64 cache_id_;
  scoped_refptr<AppCacheGroup> owning_group_;
  AppCacheHosts associated_hosts_;

  EntryMap entries_;    // contains entries of all types

  NamespaceVector intercept_namespaces_;
  NamespaceVector fallback_namespaces_;
  NamespaceVector online_whitelist_namespaces_;
  bool online_whitelist_all_;

  bool is_complete_;

  // when this cache was last updated
  base::Time update_time_;

  int64 cache_size_;

  typedef std::map<int64, AppCacheExecutableHandler*> HandlerMap;
  HandlerMap executable_handlers_;

  // to notify storage when cache is deleted
  AppCacheStorage* storage_;

  FRIEND_TEST_ALL_PREFIXES(content::AppCacheTest, InitializeWithManifest);
  FRIEND_TEST_ALL_PREFIXES(content::AppCacheTest, ToFromDatabaseRecords);
  DISALLOW_COPY_AND_ASSIGN(AppCache);
};

}  // namespace appcache

#endif  // WEBKIT_BROWSER_APPCACHE_APPCACHE_H_
