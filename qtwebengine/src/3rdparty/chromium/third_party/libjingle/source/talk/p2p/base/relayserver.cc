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

#include "talk/p2p/base/relayserver.h"

#ifdef POSIX
#include <errno.h>
#endif  // POSIX

#include <algorithm>

#include "talk/base/asynctcpsocket.h"
#include "talk/base/helpers.h"
#include "talk/base/logging.h"
#include "talk/base/socketadapters.h"

namespace cricket {

// By default, we require a ping every 90 seconds.
const int MAX_LIFETIME = 15 * 60 * 1000;

// The number of bytes in each of the usernames we use.
const uint32 USERNAME_LENGTH = 16;

// Calls SendTo on the given socket and logs any bad results.
void Send(talk_base::AsyncPacketSocket* socket, const char* bytes, size_t size,
          const talk_base::SocketAddress& addr) {
  talk_base::PacketOptions options;
  int result = socket->SendTo(bytes, size, addr, options);
  if (result < static_cast<int>(size)) {
    LOG(LS_ERROR) << "SendTo wrote only " << result << " of " << size
                  << " bytes";
  } else if (result < 0) {
    LOG_ERR(LS_ERROR) << "SendTo";
  }
}

// Sends the given STUN message on the given socket.
void SendStun(const StunMessage& msg,
              talk_base::AsyncPacketSocket* socket,
              const talk_base::SocketAddress& addr) {
  talk_base::ByteBuffer buf;
  msg.Write(&buf);
  Send(socket, buf.Data(), buf.Length(), addr);
}

// Constructs a STUN error response and sends it on the given socket.
void SendStunError(const StunMessage& msg, talk_base::AsyncPacketSocket* socket,
                   const talk_base::SocketAddress& remote_addr, int error_code,
                   const char* error_desc, const std::string& magic_cookie) {
  RelayMessage err_msg;
  err_msg.SetType(GetStunErrorResponseType(msg.type()));
  err_msg.SetTransactionID(msg.transaction_id());

  StunByteStringAttribute* magic_cookie_attr =
      StunAttribute::CreateByteString(cricket::STUN_ATTR_MAGIC_COOKIE);
  if (magic_cookie.size() == 0) {
    magic_cookie_attr->CopyBytes(cricket::TURN_MAGIC_COOKIE_VALUE,
                                 sizeof(cricket::TURN_MAGIC_COOKIE_VALUE));
  } else {
    magic_cookie_attr->CopyBytes(magic_cookie.c_str(), magic_cookie.size());
  }
  err_msg.AddAttribute(magic_cookie_attr);

  StunErrorCodeAttribute* err_code = StunAttribute::CreateErrorCode();
  err_code->SetClass(error_code / 100);
  err_code->SetNumber(error_code % 100);
  err_code->SetReason(error_desc);
  err_msg.AddAttribute(err_code);

  SendStun(err_msg, socket, remote_addr);
}

RelayServer::RelayServer(talk_base::Thread* thread)
  : thread_(thread), log_bindings_(true) {
}

RelayServer::~RelayServer() {
  // Deleting the binding will cause it to be removed from the map.
  while (!bindings_.empty())
    delete bindings_.begin()->second;
  for (size_t i = 0; i < internal_sockets_.size(); ++i)
    delete internal_sockets_[i];
  for (size_t i = 0; i < external_sockets_.size(); ++i)
    delete external_sockets_[i];
  for (size_t i = 0; i < removed_sockets_.size(); ++i)
    delete removed_sockets_[i];
  while (!server_sockets_.empty()) {
    talk_base::AsyncSocket* socket = server_sockets_.begin()->first;
    server_sockets_.erase(server_sockets_.begin()->first);
    delete socket;
  }
}

void RelayServer::AddInternalSocket(talk_base::AsyncPacketSocket* socket) {
  ASSERT(internal_sockets_.end() ==
      std::find(internal_sockets_.begin(), internal_sockets_.end(), socket));
  internal_sockets_.push_back(socket);
  socket->SignalReadPacket.connect(this, &RelayServer::OnInternalPacket);
}

void RelayServer::RemoveInternalSocket(talk_base::AsyncPacketSocket* socket) {
  SocketList::iterator iter =
      std::find(internal_sockets_.begin(), internal_sockets_.end(), socket);
  ASSERT(iter != internal_sockets_.end());
  internal_sockets_.erase(iter);
  removed_sockets_.push_back(socket);
  socket->SignalReadPacket.disconnect(this);
}

void RelayServer::AddExternalSocket(talk_base::AsyncPacketSocket* socket) {
  ASSERT(external_sockets_.end() ==
      std::find(external_sockets_.begin(), external_sockets_.end(), socket));
  external_sockets_.push_back(socket);
  socket->SignalReadPacket.connect(this, &RelayServer::OnExternalPacket);
}

void RelayServer::RemoveExternalSocket(talk_base::AsyncPacketSocket* socket) {
  SocketList::iterator iter =
      std::find(external_sockets_.begin(), external_sockets_.end(), socket);
  ASSERT(iter != external_sockets_.end());
  external_sockets_.erase(iter);
  removed_sockets_.push_back(socket);
  socket->SignalReadPacket.disconnect(this);
}

void RelayServer::AddInternalServerSocket(talk_base::AsyncSocket* socket,
                                          cricket::ProtocolType proto) {
  ASSERT(server_sockets_.end() ==
         server_sockets_.find(socket));
  server_sockets_[socket] = proto;
  socket->SignalReadEvent.connect(this, &RelayServer::OnReadEvent);
}

void RelayServer::RemoveInternalServerSocket(
    talk_base::AsyncSocket* socket) {
  ServerSocketMap::iterator iter = server_sockets_.find(socket);
  ASSERT(iter != server_sockets_.end());
  server_sockets_.erase(iter);
  socket->SignalReadEvent.disconnect(this);
}

int RelayServer::GetConnectionCount() const {
  return static_cast<int>(connections_.size());
}

talk_base::SocketAddressPair RelayServer::GetConnection(int connection) const {
  int i = 0;
  for (ConnectionMap::const_iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    if (i == connection) {
      return it->second->addr_pair();
    }
    ++i;
  }
  return talk_base::SocketAddressPair();
}

bool RelayServer::HasConnection(const talk_base::SocketAddress& address) const {
  for (ConnectionMap::const_iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    if (it->second->addr_pair().destination() == address) {
      return true;
    }
  }
  return false;
}

void RelayServer::OnReadEvent(talk_base::AsyncSocket* socket) {
  ASSERT(server_sockets_.find(socket) != server_sockets_.end());
  AcceptConnection(socket);
}

void RelayServer::OnInternalPacket(
    talk_base::AsyncPacketSocket* socket, const char* bytes, size_t size,
    const talk_base::SocketAddress& remote_addr,
    const talk_base::PacketTime& packet_time) {

  // Get the address of the connection we just received on.
  talk_base::SocketAddressPair ap(remote_addr, socket->GetLocalAddress());
  ASSERT(!ap.destination().IsNil());

  // If this did not come from an existing connection, it should be a STUN
  // allocate request.
  ConnectionMap::iterator piter = connections_.find(ap);
  if (piter == connections_.end()) {
    HandleStunAllocate(bytes, size, ap, socket);
    return;
  }

  RelayServerConnection* int_conn = piter->second;

  // Handle STUN requests to the server itself.
  if (int_conn->binding()->HasMagicCookie(bytes, size)) {
    HandleStun(int_conn, bytes, size);
    return;
  }

  // Otherwise, this is a non-wrapped packet that we are to forward.  Make sure
  // that this connection has been locked.  (Otherwise, we would not know what
  // address to forward to.)
  if (!int_conn->locked()) {
    LOG(LS_WARNING) << "Dropping packet: connection not locked";
    return;
  }

  // Forward this to the destination address into the connection.
  RelayServerConnection* ext_conn = int_conn->binding()->GetExternalConnection(
      int_conn->default_destination());
  if (ext_conn && ext_conn->locked()) {
    // TODO: Check the HMAC.
    ext_conn->Send(bytes, size);
  } else {
    // This happens very often and is not an error.
    LOG(LS_INFO) << "Dropping packet: no external connection";
  }
}

void RelayServer::OnExternalPacket(
    talk_base::AsyncPacketSocket* socket, const char* bytes, size_t size,
    const talk_base::SocketAddress& remote_addr,
    const talk_base::PacketTime& packet_time) {

  // Get the address of the connection we just received on.
  talk_base::SocketAddressPair ap(remote_addr, socket->GetLocalAddress());
  ASSERT(!ap.destination().IsNil());

  // If this connection already exists, then forward the traffic.
  ConnectionMap::iterator piter = connections_.find(ap);
  if (piter != connections_.end()) {
    // TODO: Check the HMAC.
    RelayServerConnection* ext_conn = piter->second;
    RelayServerConnection* int_conn =
        ext_conn->binding()->GetInternalConnection(
            ext_conn->addr_pair().source());
    ASSERT(int_conn != NULL);
    int_conn->Send(bytes, size, ext_conn->addr_pair().source());
    ext_conn->Lock();  // allow outgoing packets
    return;
  }

  // The first packet should always be a STUN / TURN packet.  If it isn't, then
  // we should just ignore this packet.
  RelayMessage msg;
  talk_base::ByteBuffer buf(bytes, size);
  if (!msg.Read(&buf)) {
    LOG(LS_WARNING) << "Dropping packet: first packet not STUN";
    return;
  }

  // The initial packet should have a username (which identifies the binding).
  const StunByteStringAttribute* username_attr =
      msg.GetByteString(STUN_ATTR_USERNAME);
  if (!username_attr) {
    LOG(LS_WARNING) << "Dropping packet: no username";
    return;
  }

  uint32 length = talk_base::_min(static_cast<uint32>(username_attr->length()),
                                  USERNAME_LENGTH);
  std::string username(username_attr->bytes(), length);
  // TODO: Check the HMAC.

  // The binding should already be present.
  BindingMap::iterator biter = bindings_.find(username);
  if (biter == bindings_.end()) {
    LOG(LS_WARNING) << "Dropping packet: no binding with username";
    return;
  }

  // Add this authenticted connection to the binding.
  RelayServerConnection* ext_conn =
      new RelayServerConnection(biter->second, ap, socket);
  ext_conn->binding()->AddExternalConnection(ext_conn);
  AddConnection(ext_conn);

  // We always know where external packets should be forwarded, so we can lock
  // them from the beginning.
  ext_conn->Lock();

  // Send this message on the appropriate internal connection.
  RelayServerConnection* int_conn = ext_conn->binding()->GetInternalConnection(
      ext_conn->addr_pair().source());
  ASSERT(int_conn != NULL);
  int_conn->Send(bytes, size, ext_conn->addr_pair().source());
}

bool RelayServer::HandleStun(
    const char* bytes, size_t size, const talk_base::SocketAddress& remote_addr,
    talk_base::AsyncPacketSocket* socket, std::string* username,
    StunMessage* msg) {

  // Parse this into a stun message. Eat the message if this fails.
  talk_base::ByteBuffer buf(bytes, size);
  if (!msg->Read(&buf)) {
    return false;
  }

  // The initial packet should have a username (which identifies the binding).
  const StunByteStringAttribute* username_attr =
      msg->GetByteString(STUN_ATTR_USERNAME);
  if (!username_attr) {
    SendStunError(*msg, socket, remote_addr, 432, "Missing Username", "");
    return false;
  }

  // Record the username if requested.
  if (username)
    username->append(username_attr->bytes(), username_attr->length());

  // TODO: Check for unknown attributes (<= 0x7fff)

  return true;
}

void RelayServer::HandleStunAllocate(
    const char* bytes, size_t size, const talk_base::SocketAddressPair& ap,
    talk_base::AsyncPacketSocket* socket) {

  // Make sure this is a valid STUN request.
  RelayMessage request;
  std::string username;
  if (!HandleStun(bytes, size, ap.source(), socket, &username, &request))
    return;

  // Make sure this is a an allocate request.
  if (request.type() != STUN_ALLOCATE_REQUEST) {
    SendStunError(request,
                  socket,
                  ap.source(),
                  600,
                  "Operation Not Supported",
                  "");
    return;
  }

  // TODO: Check the HMAC.

  // Find or create the binding for this username.

  RelayServerBinding* binding;

  BindingMap::iterator biter = bindings_.find(username);
  if (biter != bindings_.end()) {
    binding = biter->second;
  } else {
    // NOTE: In the future, bindings will be created by the bot only.  This
    //       else-branch will then disappear.

    // Compute the appropriate lifetime for this binding.
    uint32 lifetime = MAX_LIFETIME;
    const StunUInt32Attribute* lifetime_attr =
        request.GetUInt32(STUN_ATTR_LIFETIME);
    if (lifetime_attr)
      lifetime = talk_base::_min(lifetime, lifetime_attr->value() * 1000);

    binding = new RelayServerBinding(this, username, "0", lifetime);
    binding->SignalTimeout.connect(this, &RelayServer::OnTimeout);
    bindings_[username] = binding;

    if (log_bindings_) {
      LOG(LS_INFO) << "Added new binding " << username << ", "
                   << bindings_.size() << " total";
    }
  }

  // Add this connection to the binding.  It starts out unlocked.
  RelayServerConnection* int_conn =
      new RelayServerConnection(binding, ap, socket);
  binding->AddInternalConnection(int_conn);
  AddConnection(int_conn);

  // Now that we have a connection, this other method takes over.
  HandleStunAllocate(int_conn, request);
}

void RelayServer::HandleStun(
    RelayServerConnection* int_conn, const char* bytes, size_t size) {

  // Make sure this is a valid STUN request.
  RelayMessage request;
  std::string username;
  if (!HandleStun(bytes, size, int_conn->addr_pair().source(),
                  int_conn->socket(), &username, &request))
    return;

  // Make sure the username is the one were were expecting.
  if (username != int_conn->binding()->username()) {
    int_conn->SendStunError(request, 430, "Stale Credentials");
    return;
  }

  // TODO: Check the HMAC.

  // Send this request to the appropriate handler.
  if (request.type() == STUN_SEND_REQUEST)
    HandleStunSend(int_conn, request);
  else if (request.type() == STUN_ALLOCATE_REQUEST)
    HandleStunAllocate(int_conn, request);
  else
    int_conn->SendStunError(request, 600, "Operation Not Supported");
}

void RelayServer::HandleStunAllocate(
    RelayServerConnection* int_conn, const StunMessage& request) {

  // Create a response message that includes an address with which external
  // clients can communicate.

  RelayMessage response;
  response.SetType(STUN_ALLOCATE_RESPONSE);
  response.SetTransactionID(request.transaction_id());

  StunByteStringAttribute* magic_cookie_attr =
      StunAttribute::CreateByteString(cricket::STUN_ATTR_MAGIC_COOKIE);
  magic_cookie_attr->CopyBytes(int_conn->binding()->magic_cookie().c_str(),
                               int_conn->binding()->magic_cookie().size());
  response.AddAttribute(magic_cookie_attr);

  size_t index = rand() % external_sockets_.size();
  talk_base::SocketAddress ext_addr =
      external_sockets_[index]->GetLocalAddress();

  StunAddressAttribute* addr_attr =
      StunAttribute::CreateAddress(STUN_ATTR_MAPPED_ADDRESS);
  addr_attr->SetIP(ext_addr.ipaddr());
  addr_attr->SetPort(ext_addr.port());
  response.AddAttribute(addr_attr);

  StunUInt32Attribute* res_lifetime_attr =
      StunAttribute::CreateUInt32(STUN_ATTR_LIFETIME);
  res_lifetime_attr->SetValue(int_conn->binding()->lifetime() / 1000);
  response.AddAttribute(res_lifetime_attr);

  // TODO: Support transport-prefs (preallocate RTCP port).
  // TODO: Support bandwidth restrictions.
  // TODO: Add message integrity check.

  // Send a response to the caller.
  int_conn->SendStun(response);
}

void RelayServer::HandleStunSend(
    RelayServerConnection* int_conn, const StunMessage& request) {

  const StunAddressAttribute* addr_attr =
      request.GetAddress(STUN_ATTR_DESTINATION_ADDRESS);
  if (!addr_attr) {
    int_conn->SendStunError(request, 400, "Bad Request");
    return;
  }

  const StunByteStringAttribute* data_attr =
      request.GetByteString(STUN_ATTR_DATA);
  if (!data_attr) {
    int_conn->SendStunError(request, 400, "Bad Request");
    return;
  }

  talk_base::SocketAddress ext_addr(addr_attr->ipaddr(), addr_attr->port());
  RelayServerConnection* ext_conn =
      int_conn->binding()->GetExternalConnection(ext_addr);
  if (!ext_conn) {
    // Create a new connection to establish the relationship with this binding.
    ASSERT(external_sockets_.size() == 1);
    talk_base::AsyncPacketSocket* socket = external_sockets_[0];
    talk_base::SocketAddressPair ap(ext_addr, socket->GetLocalAddress());
    ext_conn = new RelayServerConnection(int_conn->binding(), ap, socket);
    ext_conn->binding()->AddExternalConnection(ext_conn);
    AddConnection(ext_conn);
  }

  // If this connection has pinged us, then allow outgoing traffic.
  if (ext_conn->locked())
    ext_conn->Send(data_attr->bytes(), data_attr->length());

  const StunUInt32Attribute* options_attr =
      request.GetUInt32(STUN_ATTR_OPTIONS);
  if (options_attr && (options_attr->value() & 0x01)) {
    int_conn->set_default_destination(ext_addr);
    int_conn->Lock();

    RelayMessage response;
    response.SetType(STUN_SEND_RESPONSE);
    response.SetTransactionID(request.transaction_id());

    StunByteStringAttribute* magic_cookie_attr =
        StunAttribute::CreateByteString(cricket::STUN_ATTR_MAGIC_COOKIE);
    magic_cookie_attr->CopyBytes(int_conn->binding()->magic_cookie().c_str(),
                                 int_conn->binding()->magic_cookie().size());
    response.AddAttribute(magic_cookie_attr);

    StunUInt32Attribute* options2_attr =
      StunAttribute::CreateUInt32(cricket::STUN_ATTR_OPTIONS);
    options2_attr->SetValue(0x01);
    response.AddAttribute(options2_attr);

    int_conn->SendStun(response);
  }
}

void RelayServer::AddConnection(RelayServerConnection* conn) {
  ASSERT(connections_.find(conn->addr_pair()) == connections_.end());
  connections_[conn->addr_pair()] = conn;
}

void RelayServer::RemoveConnection(RelayServerConnection* conn) {
  ConnectionMap::iterator iter = connections_.find(conn->addr_pair());
  ASSERT(iter != connections_.end());
  connections_.erase(iter);
}

void RelayServer::RemoveBinding(RelayServerBinding* binding) {
  BindingMap::iterator iter = bindings_.find(binding->username());
  ASSERT(iter != bindings_.end());
  bindings_.erase(iter);

  if (log_bindings_) {
    LOG(LS_INFO) << "Removed binding " << binding->username() << ", "
                 << bindings_.size() << " remaining";
  }
}

void RelayServer::OnMessage(talk_base::Message *pmsg) {
#if ENABLE_DEBUG
  static const uint32 kMessageAcceptConnection = 1;
  ASSERT(pmsg->message_id == kMessageAcceptConnection);
#endif
  talk_base::MessageData* data = pmsg->pdata;
  talk_base::AsyncSocket* socket =
      static_cast <talk_base::TypedMessageData<talk_base::AsyncSocket*>*>
      (data)->data();
  AcceptConnection(socket);
  delete data;
}

void RelayServer::OnTimeout(RelayServerBinding* binding) {
  // This call will result in all of the necessary clean-up. We can't call
  // delete here, because you can't delete an object that is signaling you.
  thread_->Dispose(binding);
}

void RelayServer::AcceptConnection(talk_base::AsyncSocket* server_socket) {
  // Check if someone is trying to connect to us.
  talk_base::SocketAddress accept_addr;
  talk_base::AsyncSocket* accepted_socket =
      server_socket->Accept(&accept_addr);
  if (accepted_socket != NULL) {
    // We had someone trying to connect, now check which protocol to
    // use and create a packet socket.
    ASSERT(server_sockets_[server_socket] == cricket::PROTO_TCP ||
           server_sockets_[server_socket] == cricket::PROTO_SSLTCP);
    if (server_sockets_[server_socket] == cricket::PROTO_SSLTCP) {
      accepted_socket = new talk_base::AsyncSSLServerSocket(accepted_socket);
    }
    talk_base::AsyncTCPSocket* tcp_socket =
        new talk_base::AsyncTCPSocket(accepted_socket, false);

    // Finally add the socket so it can start communicating with the client.
    AddInternalSocket(tcp_socket);
  }
}

RelayServerConnection::RelayServerConnection(
    RelayServerBinding* binding, const talk_base::SocketAddressPair& addrs,
    talk_base::AsyncPacketSocket* socket)
  : binding_(binding), addr_pair_(addrs), socket_(socket), locked_(false) {
  // The creation of a new connection constitutes a use of the binding.
  binding_->NoteUsed();
}

RelayServerConnection::~RelayServerConnection() {
  // Remove this connection from the server's map (if it exists there).
  binding_->server()->RemoveConnection(this);
}

void RelayServerConnection::Send(const char* data, size_t size) {
  // Note that the binding has been used again.
  binding_->NoteUsed();

  cricket::Send(socket_, data, size, addr_pair_.source());
}

void RelayServerConnection::Send(
    const char* data, size_t size, const talk_base::SocketAddress& from_addr) {
  // If the from address is known to the client, we don't need to send it.
  if (locked() && (from_addr == default_dest_)) {
    Send(data, size);
    return;
  }

  // Wrap the given data in a data-indication packet.

  RelayMessage msg;
  msg.SetType(STUN_DATA_INDICATION);

  StunByteStringAttribute* magic_cookie_attr =
      StunAttribute::CreateByteString(cricket::STUN_ATTR_MAGIC_COOKIE);
  magic_cookie_attr->CopyBytes(binding_->magic_cookie().c_str(),
                               binding_->magic_cookie().size());
  msg.AddAttribute(magic_cookie_attr);

  StunAddressAttribute* addr_attr =
      StunAttribute::CreateAddress(STUN_ATTR_SOURCE_ADDRESS2);
  addr_attr->SetIP(from_addr.ipaddr());
  addr_attr->SetPort(from_addr.port());
  msg.AddAttribute(addr_attr);

  StunByteStringAttribute* data_attr =
      StunAttribute::CreateByteString(STUN_ATTR_DATA);
  ASSERT(size <= 65536);
  data_attr->CopyBytes(data, uint16(size));
  msg.AddAttribute(data_attr);

  SendStun(msg);
}

void RelayServerConnection::SendStun(const StunMessage& msg) {
  // Note that the binding has been used again.
  binding_->NoteUsed();

  cricket::SendStun(msg, socket_, addr_pair_.source());
}

void RelayServerConnection::SendStunError(
      const StunMessage& request, int error_code, const char* error_desc) {
  // An error does not indicate use.  If no legitimate use off the binding
  // occurs, we want it to be cleaned up even if errors are still occuring.

  cricket::SendStunError(
      request, socket_, addr_pair_.source(), error_code, error_desc,
      binding_->magic_cookie());
}

void RelayServerConnection::Lock() {
  locked_ = true;
}

void RelayServerConnection::Unlock() {
  locked_ = false;
}

// IDs used for posted messages:
const uint32 MSG_LIFETIME_TIMER = 1;

RelayServerBinding::RelayServerBinding(
    RelayServer* server, const std::string& username,
    const std::string& password, uint32 lifetime)
  : server_(server), username_(username), password_(password),
    lifetime_(lifetime) {
  // For now, every connection uses the standard magic cookie value.
  magic_cookie_.append(
      reinterpret_cast<const char*>(TURN_MAGIC_COOKIE_VALUE),
      sizeof(TURN_MAGIC_COOKIE_VALUE));

  // Initialize the last-used time to now.
  NoteUsed();

  // Set the first timeout check.
  server_->thread()->PostDelayed(lifetime_, this, MSG_LIFETIME_TIMER);
}

RelayServerBinding::~RelayServerBinding() {
  // Clear the outstanding timeout check.
  server_->thread()->Clear(this);

  // Clean up all of the connections.
  for (size_t i = 0; i < internal_connections_.size(); ++i)
    delete internal_connections_[i];
  for (size_t i = 0; i < external_connections_.size(); ++i)
    delete external_connections_[i];

  // Remove this binding from the server's map.
  server_->RemoveBinding(this);
}

void RelayServerBinding::AddInternalConnection(RelayServerConnection* conn) {
  internal_connections_.push_back(conn);
}

void RelayServerBinding::AddExternalConnection(RelayServerConnection* conn) {
  external_connections_.push_back(conn);
}

void RelayServerBinding::NoteUsed() {
  last_used_ = talk_base::Time();
}

bool RelayServerBinding::HasMagicCookie(const char* bytes, size_t size) const {
  if (size < 24 + magic_cookie_.size()) {
    return false;
  } else {
    return memcmp(bytes + 24, magic_cookie_.c_str(), magic_cookie_.size()) == 0;
  }
}

RelayServerConnection* RelayServerBinding::GetInternalConnection(
    const talk_base::SocketAddress& ext_addr) {

  // Look for an internal connection that is locked to this address.
  for (size_t i = 0; i < internal_connections_.size(); ++i) {
    if (internal_connections_[i]->locked() &&
        (ext_addr == internal_connections_[i]->default_destination()))
      return internal_connections_[i];
  }

  // If one was not found, we send to the first connection.
  ASSERT(internal_connections_.size() > 0);
  return internal_connections_[0];
}

RelayServerConnection* RelayServerBinding::GetExternalConnection(
    const talk_base::SocketAddress& ext_addr) {
  for (size_t i = 0; i < external_connections_.size(); ++i) {
    if (ext_addr == external_connections_[i]->addr_pair().source())
      return external_connections_[i];
  }
  return 0;
}

void RelayServerBinding::OnMessage(talk_base::Message *pmsg) {
  if (pmsg->message_id == MSG_LIFETIME_TIMER) {
    ASSERT(!pmsg->pdata);

    // If the lifetime timeout has been exceeded, then send a signal.
    // Otherwise, just keep waiting.
    if (talk_base::Time() >= last_used_ + lifetime_) {
      LOG(LS_INFO) << "Expiring binding " << username_;
      SignalTimeout(this);
    } else {
      server_->thread()->PostDelayed(lifetime_, this, MSG_LIFETIME_TIMER);
    }

  } else {
    ASSERT(false);
  }
}

}  // namespace cricket
