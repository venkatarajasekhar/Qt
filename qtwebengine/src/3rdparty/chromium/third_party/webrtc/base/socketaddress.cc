/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/base/socketaddress.h"

#if defined(WEBRTC_POSIX)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#if defined(OPENBSD)
#include <netinet/in_systm.h>
#endif
#if !defined(__native_client__)
#include <netinet/ip.h>
#endif
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <sstream>

#include "webrtc/base/byteorder.h"
#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/nethelpers.h"

#if defined(WEBRTC_WIN)
#include "webrtc/base/win32.h"
#endif

namespace rtc {

SocketAddress::SocketAddress() {
  Clear();
}

SocketAddress::SocketAddress(const std::string& hostname, int port) {
  SetIP(hostname);
  SetPort(port);
}

SocketAddress::SocketAddress(uint32 ip_as_host_order_integer, int port) {
  SetIP(IPAddress(ip_as_host_order_integer));
  SetPort(port);
}

SocketAddress::SocketAddress(const IPAddress& ip, int port) {
  SetIP(ip);
  SetPort(port);
}

SocketAddress::SocketAddress(const SocketAddress& addr) {
  this->operator=(addr);
}

void SocketAddress::Clear() {
  hostname_.clear();
  literal_ = false;
  ip_ = IPAddress();
  port_ = 0;
  scope_id_ = 0;
}

bool SocketAddress::IsNil() const {
  return hostname_.empty() && IPIsUnspec(ip_) && 0 == port_;
}

bool SocketAddress::IsComplete() const {
  return (!IPIsAny(ip_)) && (0 != port_);
}

SocketAddress& SocketAddress::operator=(const SocketAddress& addr) {
  hostname_ = addr.hostname_;
  ip_ = addr.ip_;
  port_ = addr.port_;
  literal_ = addr.literal_;
  scope_id_ = addr.scope_id_;
  return *this;
}

void SocketAddress::SetIP(uint32 ip_as_host_order_integer) {
  hostname_.clear();
  literal_ = false;
  ip_ = IPAddress(ip_as_host_order_integer);
  scope_id_ = 0;
}

void SocketAddress::SetIP(const IPAddress& ip) {
  hostname_.clear();
  literal_ = false;
  ip_ = ip;
  scope_id_ = 0;
}

void SocketAddress::SetIP(const std::string& hostname) {
  hostname_ = hostname;
  literal_ = IPFromString(hostname, &ip_);
  if (!literal_) {
    ip_ = IPAddress();
  }
  scope_id_ = 0;
}

void SocketAddress::SetResolvedIP(uint32 ip_as_host_order_integer) {
  ip_ = IPAddress(ip_as_host_order_integer);
  scope_id_ = 0;
}

void SocketAddress::SetResolvedIP(const IPAddress& ip) {
  ip_ = ip;
  scope_id_ = 0;
}

void SocketAddress::SetPort(int port) {
  ASSERT((0 <= port) && (port < 65536));
  port_ = port;
}

uint32 SocketAddress::ip() const {
  return ip_.v4AddressAsHostOrderInteger();
}

const IPAddress& SocketAddress::ipaddr() const {
  return ip_;
}

uint16 SocketAddress::port() const {
  return port_;
}

std::string SocketAddress::HostAsURIString() const {
  // If the hostname was a literal IP string, it may need to have square
  // brackets added (for SocketAddress::ToString()).
  if (!literal_ && !hostname_.empty())
    return hostname_;
  if (ip_.family() == AF_INET6) {
    return "[" + ip_.ToString() + "]";
  } else {
    return ip_.ToString();
  }
}

std::string SocketAddress::HostAsSensitiveURIString() const {
  // If the hostname was a literal IP string, it may need to have square
  // brackets added (for SocketAddress::ToString()).
  if (!literal_ && !hostname_.empty())
    return hostname_;
  if (ip_.family() == AF_INET6) {
    return "[" + ip_.ToSensitiveString() + "]";
  } else {
    return ip_.ToSensitiveString();
  }
}

std::string SocketAddress::PortAsString() const {
  std::ostringstream ost;
  ost << port_;
  return ost.str();
}

std::string SocketAddress::ToString() const {
  std::ostringstream ost;
  ost << *this;
  return ost.str();
}

std::string SocketAddress::ToSensitiveString() const {
  std::ostringstream ost;
  ost << HostAsSensitiveURIString() << ":" << port();
  return ost.str();
}

bool SocketAddress::FromString(const std::string& str) {
  if (str.at(0) == '[') {
    std::string::size_type closebracket = str.rfind(']');
    if (closebracket != std::string::npos) {
      std::string::size_type colon = str.find(':', closebracket);
      if (colon != std::string::npos && colon > closebracket) {
        SetPort(strtoul(str.substr(colon + 1).c_str(), NULL, 10));
        SetIP(str.substr(1, closebracket - 1));
      } else {
        return false;
      }
    }
  } else {
    std::string::size_type pos = str.find(':');
    if (std::string::npos == pos)
      return false;
    SetPort(strtoul(str.substr(pos + 1).c_str(), NULL, 10));
    SetIP(str.substr(0, pos));
  }
  return true;
}

std::ostream& operator<<(std::ostream& os, const SocketAddress& addr) {
  os << addr.HostAsURIString() << ":" << addr.port();
  return os;
}

bool SocketAddress::IsAnyIP() const {
  return IPIsAny(ip_);
}

bool SocketAddress::IsLoopbackIP() const {
  return IPIsLoopback(ip_) || (IPIsAny(ip_) &&
                               0 == strcmp(hostname_.c_str(), "localhost"));
}

bool SocketAddress::IsPrivateIP() const {
  return IPIsPrivate(ip_);
}

bool SocketAddress::IsUnresolvedIP() const {
  return IPIsUnspec(ip_) && !literal_ && !hostname_.empty();
}

bool SocketAddress::operator==(const SocketAddress& addr) const {
  return EqualIPs(addr) && EqualPorts(addr);
}

bool SocketAddress::operator<(const SocketAddress& addr) const {
  if (ip_ < addr.ip_)
    return true;
  else if (addr.ip_ < ip_)
    return false;

  // We only check hostnames if both IPs are zero.  This matches EqualIPs()
  if (addr.IsAnyIP()) {
    if (hostname_ < addr.hostname_)
      return true;
    else if (addr.hostname_ < hostname_)
      return false;
  }

  return port_ < addr.port_;
}

bool SocketAddress::EqualIPs(const SocketAddress& addr) const {
  return (ip_ == addr.ip_) &&
      ((!IPIsAny(ip_)) || (hostname_ == addr.hostname_));
}

bool SocketAddress::EqualPorts(const SocketAddress& addr) const {
  return (port_ == addr.port_);
}

size_t SocketAddress::Hash() const {
  size_t h = 0;
  h ^= HashIP(ip_);
  h ^= port_ | (port_ << 16);
  return h;
}

void SocketAddress::ToSockAddr(sockaddr_in* saddr) const {
  memset(saddr, 0, sizeof(*saddr));
  if (ip_.family() != AF_INET) {
    saddr->sin_family = AF_UNSPEC;
    return;
  }
  saddr->sin_family = AF_INET;
  saddr->sin_port = HostToNetwork16(port_);
  if (IPIsAny(ip_)) {
    saddr->sin_addr.s_addr = INADDR_ANY;
  } else {
    saddr->sin_addr = ip_.ipv4_address();
  }
}

bool SocketAddress::FromSockAddr(const sockaddr_in& saddr) {
  if (saddr.sin_family != AF_INET)
    return false;
  SetIP(NetworkToHost32(saddr.sin_addr.s_addr));
  SetPort(NetworkToHost16(saddr.sin_port));
  literal_ = false;
  return true;
}

static size_t ToSockAddrStorageHelper(sockaddr_storage* addr,
                                      IPAddress ip, int port, int scope_id) {
  memset(addr, 0, sizeof(sockaddr_storage));
  addr->ss_family = ip.family();
  if (addr->ss_family == AF_INET6) {
    sockaddr_in6* saddr = reinterpret_cast<sockaddr_in6*>(addr);
    saddr->sin6_addr = ip.ipv6_address();
    saddr->sin6_port = HostToNetwork16(port);
    saddr->sin6_scope_id = scope_id;
    return sizeof(sockaddr_in6);
  } else if (addr->ss_family == AF_INET) {
    sockaddr_in* saddr = reinterpret_cast<sockaddr_in*>(addr);
    saddr->sin_addr = ip.ipv4_address();
    saddr->sin_port = HostToNetwork16(port);
    return sizeof(sockaddr_in);
  }
  return 0;
}

size_t SocketAddress::ToDualStackSockAddrStorage(sockaddr_storage *addr) const {
  return ToSockAddrStorageHelper(addr, ip_.AsIPv6Address(), port_, scope_id_);
}

size_t SocketAddress::ToSockAddrStorage(sockaddr_storage* addr) const {
  return ToSockAddrStorageHelper(addr, ip_, port_, scope_id_);
}

std::string SocketAddress::IPToString(uint32 ip_as_host_order_integer) {
  return IPAddress(ip_as_host_order_integer).ToString();
}

std::string IPToSensitiveString(uint32 ip_as_host_order_integer) {
  return IPAddress(ip_as_host_order_integer).ToSensitiveString();
}

bool SocketAddress::StringToIP(const std::string& hostname, uint32* ip) {
  in_addr addr;
  if (rtc::inet_pton(AF_INET, hostname.c_str(), &addr) == 0)
    return false;
  *ip = NetworkToHost32(addr.s_addr);
  return true;
}

bool SocketAddress::StringToIP(const std::string& hostname, IPAddress* ip) {
  in_addr addr4;
  if (rtc::inet_pton(AF_INET, hostname.c_str(), &addr4) > 0) {
    if (ip) {
      *ip = IPAddress(addr4);
    }
    return true;
  }

  in6_addr addr6;
  if (rtc::inet_pton(AF_INET6, hostname.c_str(), &addr6) > 0) {
    if (ip) {
      *ip = IPAddress(addr6);
    }
    return true;
  }
  return false;
}

uint32 SocketAddress::StringToIP(const std::string& hostname) {
  uint32 ip = 0;
  StringToIP(hostname, &ip);
  return ip;
}

bool SocketAddressFromSockAddrStorage(const sockaddr_storage& addr,
                                      SocketAddress* out) {
  if (!out) {
    return false;
  }
  if (addr.ss_family == AF_INET) {
    const sockaddr_in* saddr = reinterpret_cast<const sockaddr_in*>(&addr);
    *out = SocketAddress(IPAddress(saddr->sin_addr),
                         NetworkToHost16(saddr->sin_port));
    return true;
  } else if (addr.ss_family == AF_INET6) {
    const sockaddr_in6* saddr = reinterpret_cast<const sockaddr_in6*>(&addr);
    *out = SocketAddress(IPAddress(saddr->sin6_addr),
                         NetworkToHost16(saddr->sin6_port));
    out->SetScopeID(saddr->sin6_scope_id);
    return true;
  }
  return false;
}

SocketAddress EmptySocketAddressWithFamily(int family) {
  if (family == AF_INET) {
    return SocketAddress(IPAddress(INADDR_ANY), 0);
  } else if (family == AF_INET6) {
    return SocketAddress(IPAddress(in6addr_any), 0);
  }
  return SocketAddress();
}

}  // namespace rtc
