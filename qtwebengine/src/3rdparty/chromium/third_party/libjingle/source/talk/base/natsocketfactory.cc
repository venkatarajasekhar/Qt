/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "talk/base/natsocketfactory.h"

#include "talk/base/logging.h"
#include "talk/base/natserver.h"
#include "talk/base/virtualsocketserver.h"

namespace talk_base {

// Packs the given socketaddress into the buffer in buf, in the quasi-STUN
// format that the natserver uses.
// Returns 0 if an invalid address is passed.
size_t PackAddressForNAT(char* buf, size_t buf_size,
                         const SocketAddress& remote_addr) {
  const IPAddress& ip = remote_addr.ipaddr();
  int family = ip.family();
  buf[0] = 0;
  buf[1] = family;
  // Writes the port.
  *(reinterpret_cast<uint16*>(&buf[2])) = HostToNetwork16(remote_addr.port());
  if (family == AF_INET) {
    ASSERT(buf_size >= kNATEncodedIPv4AddressSize);
    in_addr v4addr = ip.ipv4_address();
    memcpy(&buf[4], &v4addr, kNATEncodedIPv4AddressSize - 4);
    return kNATEncodedIPv4AddressSize;
  } else if (family == AF_INET6) {
    ASSERT(buf_size >= kNATEncodedIPv6AddressSize);
    in6_addr v6addr = ip.ipv6_address();
    memcpy(&buf[4], &v6addr, kNATEncodedIPv6AddressSize - 4);
    return kNATEncodedIPv6AddressSize;
  }
  return 0U;
}

// Decodes the remote address from a packet that has been encoded with the nat's
// quasi-STUN format. Returns the length of the address (i.e., the offset into
// data where the original packet starts).
size_t UnpackAddressFromNAT(const char* buf, size_t buf_size,
                            SocketAddress* remote_addr) {
  ASSERT(buf_size >= 8);
  ASSERT(buf[0] == 0);
  int family = buf[1];
  uint16 port = NetworkToHost16(*(reinterpret_cast<const uint16*>(&buf[2])));
  if (family == AF_INET) {
    const in_addr* v4addr = reinterpret_cast<const in_addr*>(&buf[4]);
    *remote_addr = SocketAddress(IPAddress(*v4addr), port);
    return kNATEncodedIPv4AddressSize;
  } else if (family == AF_INET6) {
    ASSERT(buf_size >= 20);
    const in6_addr* v6addr = reinterpret_cast<const in6_addr*>(&buf[4]);
    *remote_addr = SocketAddress(IPAddress(*v6addr), port);
    return kNATEncodedIPv6AddressSize;
  }
  return 0U;
}


// NATSocket
class NATSocket : public AsyncSocket, public sigslot::has_slots<> {
 public:
  explicit NATSocket(NATInternalSocketFactory* sf, int family, int type)
      : sf_(sf), family_(family), type_(type), connected_(false),
        socket_(NULL), buf_(NULL), size_(0) {
  }

  virtual ~NATSocket() {
    delete socket_;
    delete[] buf_;
  }

  virtual SocketAddress GetLocalAddress() const {
    return (socket_) ? socket_->GetLocalAddress() : SocketAddress();
  }

  virtual SocketAddress GetRemoteAddress() const {
    return remote_addr_;  // will be NIL if not connected
  }

  virtual int Bind(const SocketAddress& addr) {
    if (socket_) {  // already bound, bubble up error
      return -1;
    }

    int result;
    socket_ = sf_->CreateInternalSocket(family_, type_, addr, &server_addr_);
    result = (socket_) ? socket_->Bind(addr) : -1;
    if (result >= 0) {
      socket_->SignalConnectEvent.connect(this, &NATSocket::OnConnectEvent);
      socket_->SignalReadEvent.connect(this, &NATSocket::OnReadEvent);
      socket_->SignalWriteEvent.connect(this, &NATSocket::OnWriteEvent);
      socket_->SignalCloseEvent.connect(this, &NATSocket::OnCloseEvent);
    } else {
      server_addr_.Clear();
      delete socket_;
      socket_ = NULL;
    }

    return result;
  }

  virtual int Connect(const SocketAddress& addr) {
    if (!socket_) {  // socket must be bound, for now
      return -1;
    }

    int result = 0;
    if (type_ == SOCK_STREAM) {
      result = socket_->Connect(server_addr_.IsNil() ? addr : server_addr_);
    } else {
      connected_ = true;
    }

    if (result >= 0) {
      remote_addr_ = addr;
    }

    return result;
  }

  virtual int Send(const void* data, size_t size) {
    ASSERT(connected_);
    return SendTo(data, size, remote_addr_);
  }

  virtual int SendTo(const void* data, size_t size, const SocketAddress& addr) {
    ASSERT(!connected_ || addr == remote_addr_);
    if (server_addr_.IsNil() || type_ == SOCK_STREAM) {
      return socket_->SendTo(data, size, addr);
    }
    // This array will be too large for IPv4 packets, but only by 12 bytes.
    scoped_ptr<char[]> buf(new char[size + kNATEncodedIPv6AddressSize]);
    size_t addrlength = PackAddressForNAT(buf.get(),
                                          size + kNATEncodedIPv6AddressSize,
                                          addr);
    size_t encoded_size = size + addrlength;
    memcpy(buf.get() + addrlength, data, size);
    int result = socket_->SendTo(buf.get(), encoded_size, server_addr_);
    if (result >= 0) {
      ASSERT(result == static_cast<int>(encoded_size));
      result = result - static_cast<int>(addrlength);
    }
    return result;
  }

  virtual int Recv(void* data, size_t size) {
    SocketAddress addr;
    return RecvFrom(data, size, &addr);
  }

  virtual int RecvFrom(void* data, size_t size, SocketAddress *out_addr) {
    if (server_addr_.IsNil() || type_ == SOCK_STREAM) {
      return socket_->RecvFrom(data, size, out_addr);
    }
    // Make sure we have enough room to read the requested amount plus the
    // largest possible header address.
    SocketAddress remote_addr;
    Grow(size + kNATEncodedIPv6AddressSize);

    // Read the packet from the socket.
    int result = socket_->RecvFrom(buf_, size_, &remote_addr);
    if (result >= 0) {
      ASSERT(remote_addr == server_addr_);

      // TODO: we need better framing so we know how many bytes we can
      // return before we need to read the next address. For UDP, this will be
      // fine as long as the reader always reads everything in the packet.
      ASSERT((size_t)result < size_);

      // Decode the wire packet into the actual results.
      SocketAddress real_remote_addr;
      size_t addrlength =
          UnpackAddressFromNAT(buf_, result, &real_remote_addr);
      memcpy(data, buf_ + addrlength, result - addrlength);

      // Make sure this packet should be delivered before returning it.
      if (!connected_ || (real_remote_addr == remote_addr_)) {
        if (out_addr)
          *out_addr = real_remote_addr;
        result = result - static_cast<int>(addrlength);
      } else {
        LOG(LS_ERROR) << "Dropping packet from unknown remote address: "
                      << real_remote_addr.ToString();
        result = 0;  // Tell the caller we didn't read anything
      }
    }

    return result;
  }

  virtual int Close() {
    int result = 0;
    if (socket_) {
      result = socket_->Close();
      if (result >= 0) {
        connected_ = false;
        remote_addr_ = SocketAddress();
        delete socket_;
        socket_ = NULL;
      }
    }
    return result;
  }

  virtual int Listen(int backlog) {
    return socket_->Listen(backlog);
  }
  virtual AsyncSocket* Accept(SocketAddress *paddr) {
    return socket_->Accept(paddr);
  }
  virtual int GetError() const {
    return socket_->GetError();
  }
  virtual void SetError(int error) {
    socket_->SetError(error);
  }
  virtual ConnState GetState() const {
    return connected_ ? CS_CONNECTED : CS_CLOSED;
  }
  virtual int EstimateMTU(uint16* mtu) {
    return socket_->EstimateMTU(mtu);
  }
  virtual int GetOption(Option opt, int* value) {
    return socket_->GetOption(opt, value);
  }
  virtual int SetOption(Option opt, int value) {
    return socket_->SetOption(opt, value);
  }

  void OnConnectEvent(AsyncSocket* socket) {
    // If we're NATed, we need to send a request with the real addr to use.
    ASSERT(socket == socket_);
    if (server_addr_.IsNil()) {
      connected_ = true;
      SignalConnectEvent(this);
    } else {
      SendConnectRequest();
    }
  }
  void OnReadEvent(AsyncSocket* socket) {
    // If we're NATed, we need to process the connect reply.
    ASSERT(socket == socket_);
    if (type_ == SOCK_STREAM && !server_addr_.IsNil() && !connected_) {
      HandleConnectReply();
    } else {
      SignalReadEvent(this);
    }
  }
  void OnWriteEvent(AsyncSocket* socket) {
    ASSERT(socket == socket_);
    SignalWriteEvent(this);
  }
  void OnCloseEvent(AsyncSocket* socket, int error) {
    ASSERT(socket == socket_);
    SignalCloseEvent(this, error);
  }

 private:
  // Makes sure the buffer is at least the given size.
  void Grow(size_t new_size) {
    if (size_ < new_size) {
      delete[] buf_;
      size_ = new_size;
      buf_ = new char[size_];
    }
  }

  // Sends the destination address to the server to tell it to connect.
  void SendConnectRequest() {
    char buf[256];
    size_t length = PackAddressForNAT(buf, ARRAY_SIZE(buf), remote_addr_);
    socket_->Send(buf, length);
  }

  // Handles the byte sent back from the server and fires the appropriate event.
  void HandleConnectReply() {
    char code;
    socket_->Recv(&code, sizeof(code));
    if (code == 0) {
      SignalConnectEvent(this);
    } else {
      Close();
      SignalCloseEvent(this, code);
    }
  }

  NATInternalSocketFactory* sf_;
  int family_;
  int type_;
  bool connected_;
  SocketAddress remote_addr_;
  SocketAddress server_addr_;  // address of the NAT server
  AsyncSocket* socket_;
  char* buf_;
  size_t size_;
};

// NATSocketFactory
NATSocketFactory::NATSocketFactory(SocketFactory* factory,
                                   const SocketAddress& nat_addr)
    : factory_(factory), nat_addr_(nat_addr) {
}

Socket* NATSocketFactory::CreateSocket(int type) {
  return CreateSocket(AF_INET, type);
}

Socket* NATSocketFactory::CreateSocket(int family, int type) {
  return new NATSocket(this, family, type);
}

AsyncSocket* NATSocketFactory::CreateAsyncSocket(int type) {
  return CreateAsyncSocket(AF_INET, type);
}

AsyncSocket* NATSocketFactory::CreateAsyncSocket(int family, int type) {
  return new NATSocket(this, family, type);
}

AsyncSocket* NATSocketFactory::CreateInternalSocket(int family, int type,
    const SocketAddress& local_addr, SocketAddress* nat_addr) {
  *nat_addr = nat_addr_;
  return factory_->CreateAsyncSocket(family, type);
}

// NATSocketServer
NATSocketServer::NATSocketServer(SocketServer* server)
    : server_(server), msg_queue_(NULL) {
}

NATSocketServer::Translator* NATSocketServer::GetTranslator(
    const SocketAddress& ext_ip) {
  return nats_.Get(ext_ip);
}

NATSocketServer::Translator* NATSocketServer::AddTranslator(
    const SocketAddress& ext_ip, const SocketAddress& int_ip, NATType type) {
  // Fail if a translator already exists with this extternal address.
  if (nats_.Get(ext_ip))
    return NULL;

  return nats_.Add(ext_ip, new Translator(this, type, int_ip, server_, ext_ip));
}

void NATSocketServer::RemoveTranslator(
    const SocketAddress& ext_ip) {
  nats_.Remove(ext_ip);
}

Socket* NATSocketServer::CreateSocket(int type) {
  return CreateSocket(AF_INET, type);
}

Socket* NATSocketServer::CreateSocket(int family, int type) {
  return new NATSocket(this, family, type);
}

AsyncSocket* NATSocketServer::CreateAsyncSocket(int type) {
  return CreateAsyncSocket(AF_INET, type);
}

AsyncSocket* NATSocketServer::CreateAsyncSocket(int family, int type) {
  return new NATSocket(this, family, type);
}

AsyncSocket* NATSocketServer::CreateInternalSocket(int family, int type,
    const SocketAddress& local_addr, SocketAddress* nat_addr) {
  AsyncSocket* socket = NULL;
  Translator* nat = nats_.FindClient(local_addr);
  if (nat) {
    socket = nat->internal_factory()->CreateAsyncSocket(family, type);
    *nat_addr = (type == SOCK_STREAM) ?
        nat->internal_tcp_address() : nat->internal_address();
  } else {
    socket = server_->CreateAsyncSocket(family, type);
  }
  return socket;
}

// NATSocketServer::Translator
NATSocketServer::Translator::Translator(
    NATSocketServer* server, NATType type, const SocketAddress& int_ip,
    SocketFactory* ext_factory, const SocketAddress& ext_ip)
    : server_(server) {
  // Create a new private network, and a NATServer running on the private
  // network that bridges to the external network. Also tell the private
  // network to use the same message queue as us.
  VirtualSocketServer* internal_server = new VirtualSocketServer(server_);
  internal_server->SetMessageQueue(server_->queue());
  internal_factory_.reset(internal_server);
  nat_server_.reset(new NATServer(type, internal_server, int_ip,
                                  ext_factory, ext_ip));
}


NATSocketServer::Translator* NATSocketServer::Translator::GetTranslator(
    const SocketAddress& ext_ip) {
  return nats_.Get(ext_ip);
}

NATSocketServer::Translator* NATSocketServer::Translator::AddTranslator(
    const SocketAddress& ext_ip, const SocketAddress& int_ip, NATType type) {
  // Fail if a translator already exists with this extternal address.
  if (nats_.Get(ext_ip))
    return NULL;

  AddClient(ext_ip);
  return nats_.Add(ext_ip,
                   new Translator(server_, type, int_ip, server_, ext_ip));
}
void NATSocketServer::Translator::RemoveTranslator(
    const SocketAddress& ext_ip) {
  nats_.Remove(ext_ip);
  RemoveClient(ext_ip);
}

bool NATSocketServer::Translator::AddClient(
    const SocketAddress& int_ip) {
  // Fail if a client already exists with this internal address.
  if (clients_.find(int_ip) != clients_.end())
    return false;

  clients_.insert(int_ip);
  return true;
}

void NATSocketServer::Translator::RemoveClient(
    const SocketAddress& int_ip) {
  std::set<SocketAddress>::iterator it = clients_.find(int_ip);
  if (it != clients_.end()) {
    clients_.erase(it);
  }
}

NATSocketServer::Translator* NATSocketServer::Translator::FindClient(
    const SocketAddress& int_ip) {
  // See if we have the requested IP, or any of our children do.
  return (clients_.find(int_ip) != clients_.end()) ?
      this : nats_.FindClient(int_ip);
}

// NATSocketServer::TranslatorMap
NATSocketServer::TranslatorMap::~TranslatorMap() {
  for (TranslatorMap::iterator it = begin(); it != end(); ++it) {
    delete it->second;
  }
}

NATSocketServer::Translator* NATSocketServer::TranslatorMap::Get(
    const SocketAddress& ext_ip) {
  TranslatorMap::iterator it = find(ext_ip);
  return (it != end()) ? it->second : NULL;
}

NATSocketServer::Translator* NATSocketServer::TranslatorMap::Add(
    const SocketAddress& ext_ip, Translator* nat) {
  (*this)[ext_ip] = nat;
  return nat;
}

void NATSocketServer::TranslatorMap::Remove(
    const SocketAddress& ext_ip) {
  TranslatorMap::iterator it = find(ext_ip);
  if (it != end()) {
    delete it->second;
    erase(it);
  }
}

NATSocketServer::Translator* NATSocketServer::TranslatorMap::FindClient(
    const SocketAddress& int_ip) {
  Translator* nat = NULL;
  for (TranslatorMap::iterator it = begin(); it != end() && !nat; ++it) {
    nat = it->second->FindClient(int_ip);
  }
  return nat;
}

}  // namespace talk_base
