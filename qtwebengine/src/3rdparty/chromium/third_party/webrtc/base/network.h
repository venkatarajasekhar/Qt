/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_BASE_NETWORK_H_
#define WEBRTC_BASE_NETWORK_H_

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/ipaddress.h"
#include "webrtc/base/messagehandler.h"
#include "webrtc/base/sigslot.h"

#if defined(WEBRTC_POSIX)
struct ifaddrs;
#endif  // defined(WEBRTC_POSIX)

namespace rtc {

class Network;
class Thread;

enum AdapterType {
  // This enum resembles the one in Chromium net::ConnectionType.
  ADAPTER_TYPE_UNKNOWN = 0,
  ADAPTER_TYPE_ETHERNET = 1,
  ADAPTER_TYPE_WIFI = 2,
  ADAPTER_TYPE_CELLULAR = 3,
  ADAPTER_TYPE_VPN = 4
};

// Makes a string key for this network. Used in the network manager's maps.
// Network objects are keyed on interface name, network prefix and the
// length of that prefix.
std::string MakeNetworkKey(const std::string& name, const IPAddress& prefix,
                           int prefix_length);

// Generic network manager interface. It provides list of local
// networks.
class NetworkManager {
 public:
  typedef std::vector<Network*> NetworkList;

  NetworkManager();
  virtual ~NetworkManager();

  // Called when network list is updated.
  sigslot::signal0<> SignalNetworksChanged;

  // Indicates a failure when getting list of network interfaces.
  sigslot::signal0<> SignalError;

  // Start/Stop monitoring of network interfaces
  // list. SignalNetworksChanged or SignalError is emitted immidiately
  // after StartUpdating() is called. After that SignalNetworksChanged
  // is emitted wheneven list of networks changes.
  virtual void StartUpdating() = 0;
  virtual void StopUpdating() = 0;

  // Returns the current list of networks available on this machine.
  // UpdateNetworks() must be called before this method is called.
  // It makes sure that repeated calls return the same object for a
  // given network, so that quality is tracked appropriately. Does not
  // include ignored networks.
  virtual void GetNetworks(NetworkList* networks) const = 0;

  // Dumps a list of networks available to LS_INFO.
  virtual void DumpNetworks(bool include_ignored) {}
};

// Base class for NetworkManager implementations.
class NetworkManagerBase : public NetworkManager {
 public:
  NetworkManagerBase();
  virtual ~NetworkManagerBase();

  virtual void GetNetworks(std::vector<Network*>* networks) const;
  bool ipv6_enabled() const { return ipv6_enabled_; }
  void set_ipv6_enabled(bool enabled) { ipv6_enabled_ = enabled; }

 protected:
  typedef std::map<std::string, Network*> NetworkMap;
  // Updates |networks_| with the networks listed in |list|. If
  // |network_map_| already has a Network object for a network listed
  // in the |list| then it is reused. Accept ownership of the Network
  // objects in the |list|. |changed| will be set to true if there is
  // any change in the network list.
  void MergeNetworkList(const NetworkList& list, bool* changed);

 private:
  friend class NetworkTest;
  void DoUpdateNetworks();

  NetworkList networks_;
  NetworkMap networks_map_;
  bool ipv6_enabled_;
};

// Basic implementation of the NetworkManager interface that gets list
// of networks using OS APIs.
class BasicNetworkManager : public NetworkManagerBase,
                            public MessageHandler {
 public:
  BasicNetworkManager();
  virtual ~BasicNetworkManager();

  virtual void StartUpdating();
  virtual void StopUpdating();

  // Logs the available networks.
  virtual void DumpNetworks(bool include_ignored);

  // MessageHandler interface.
  virtual void OnMessage(Message* msg);
  bool started() { return start_count_ > 0; }

  // Sets the network ignore list, which is empty by default. Any network on
  // the ignore list will be filtered from network enumeration results.
  void set_network_ignore_list(const std::vector<std::string>& list) {
    network_ignore_list_ = list;
  }
#if defined(WEBRTC_LINUX)
  // Sets the flag for ignoring non-default routes.
  void set_ignore_non_default_routes(bool value) {
    ignore_non_default_routes_ = true;
  }
#endif

 protected:
#if defined(WEBRTC_POSIX)
  // Separated from CreateNetworks for tests.
  void ConvertIfAddrs(ifaddrs* interfaces,
                      bool include_ignored,
                      NetworkList* networks) const;
#endif  // defined(WEBRTC_POSIX)

  // Creates a network object for each network available on the machine.
  bool CreateNetworks(bool include_ignored, NetworkList* networks) const;

  // Determines if a network should be ignored.
  bool IsIgnoredNetwork(const Network& network) const;

 private:
  friend class NetworkTest;

  void DoUpdateNetworks();

  Thread* thread_;
  bool sent_first_update_;
  int start_count_;
  std::vector<std::string> network_ignore_list_;
  bool ignore_non_default_routes_;
};

// Represents a Unix-type network interface, with a name and single address.
class Network {
 public:
  Network(const std::string& name, const std::string& description,
          const IPAddress& prefix, int prefix_length);

  Network(const std::string& name, const std::string& description,
          const IPAddress& prefix, int prefix_length, AdapterType type);

  // Returns the name of the interface this network is associated wtih.
  const std::string& name() const { return name_; }

  // Returns the OS-assigned name for this network. This is useful for
  // debugging but should not be sent over the wire (for privacy reasons).
  const std::string& description() const { return description_; }

  // Returns the prefix for this network.
  const IPAddress& prefix() const { return prefix_; }
  // Returns the length, in bits, of this network's prefix.
  int prefix_length() const { return prefix_length_; }

  // |key_| has unique value per network interface. Used in sorting network
  // interfaces. Key is derived from interface name and it's prefix.
  std::string key() const { return key_; }

  // Returns the Network's current idea of the 'best' IP it has.
  // 'Best' currently means the first one added.
  // TODO: We should be preferring temporary addresses.
  // Returns an unset IP if this network has no active addresses.
  IPAddress ip() const {
    if (ips_.size() == 0) {
      return IPAddress();
    }
    return ips_.at(0);
  }
  // Adds an active IP address to this network. Does not check for duplicates.
  void AddIP(const IPAddress& ip) { ips_.push_back(ip); }

  // Sets the network's IP address list. Returns true if new IP addresses were
  // detected. Passing true to already_changed skips this check.
  bool SetIPs(const std::vector<IPAddress>& ips, bool already_changed);
  // Get the list of IP Addresses associated with this network.
  const std::vector<IPAddress>& GetIPs() { return ips_;}
  // Clear the network's list of addresses.
  void ClearIPs() { ips_.clear(); }

  // Returns the scope-id of the network's address.
  // Should only be relevant for link-local IPv6 addresses.
  int scope_id() const { return scope_id_; }
  void set_scope_id(int id) { scope_id_ = id; }

  // Indicates whether this network should be ignored, perhaps because
  // the IP is 0, or the interface is one we know is invalid.
  bool ignored() const { return ignored_; }
  void set_ignored(bool ignored) { ignored_ = ignored; }

  AdapterType type() const { return type_; }
  int preference() const { return preference_; }
  void set_preference(int preference) { preference_ = preference; }

  // Debugging description of this network
  std::string ToString() const;

 private:
  std::string name_;
  std::string description_;
  IPAddress prefix_;
  int prefix_length_;
  std::string key_;
  std::vector<IPAddress> ips_;
  int scope_id_;
  bool ignored_;
  AdapterType type_;
  int preference_;

  friend class NetworkManager;
};

}  // namespace rtc

#endif  // WEBRTC_BASE_NETWORK_H_
