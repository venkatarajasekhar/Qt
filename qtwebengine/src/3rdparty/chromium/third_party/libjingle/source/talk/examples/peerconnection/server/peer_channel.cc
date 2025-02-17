/*
 * libjingle
 * Copyright 2011, Google Inc.
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

#include "talk/examples/peerconnection/server/peer_channel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include "talk/examples/peerconnection/server/data_socket.h"
#include "talk/examples/peerconnection/server/utils.h"

// Set to the peer id of the originator when messages are being
// exchanged between peers, but set to the id of the receiving peer
// itself when notifications are sent from the server about the state
// of other peers.
//
// WORKAROUND: Since support for CORS varies greatly from one browser to the
// next, we don't use a custom name for our peer-id header (originally it was
// "X-Peer-Id: ").  Instead, we use a "simple header", "Pragma" which should
// always be exposed to CORS requests.  There is a special CORS header devoted
// to exposing proprietary headers (Access-Control-Expose-Headers), however
// at this point it is not working correctly in some popular browsers.
static const char kPeerIdHeader[] = "Pragma: ";

static const char* kRequestPaths[] = {
  "/wait", "/sign_out", "/message",
};

enum RequestPathIndex {
  kWait,
  kSignOut,
  kMessage,
};

//
// ChannelMember
//

int ChannelMember::s_member_id_ = 0;

ChannelMember::ChannelMember(DataSocket* socket)
  : waiting_socket_(NULL), id_(++s_member_id_),
    connected_(true), timestamp_(time(NULL)) {
  assert(socket);
  assert(socket->method() == DataSocket::GET);
  assert(socket->PathEquals("/sign_in"));
  name_ = socket->request_arguments();  // TODO: urldecode
  if (!name_.length())
    name_ = "peer_" + int2str(id_);
  std::replace(name_.begin(), name_.end(), ',', '_');
}

ChannelMember::~ChannelMember() {
}

bool ChannelMember::is_wait_request(DataSocket* ds) const {
  return ds && ds->PathEquals(kRequestPaths[kWait]);
}

bool ChannelMember::TimedOut() {
  return waiting_socket_ == NULL && (time(NULL) - timestamp_) > 30;
}

std::string ChannelMember::GetPeerIdHeader() const {
  std::string ret(kPeerIdHeader + int2str(id_) + "\r\n");
  return ret;
}

bool ChannelMember::NotifyOfOtherMember(const ChannelMember& other) {
  assert(&other != this);
  QueueResponse("200 OK", "text/plain", GetPeerIdHeader(),
                other.GetEntry());
  return true;
}

// Returns a string in the form "name,id\n".
std::string ChannelMember::GetEntry() const {
  char entry[1024] = {0};
  sprintf(entry, "%s,%i,%i\n", name_.c_str(), id_, connected_);  // NOLINT
  return entry;
}

void ChannelMember::ForwardRequestToPeer(DataSocket* ds, ChannelMember* peer) {
  assert(peer);
  assert(ds);

  std::string extra_headers(GetPeerIdHeader());

  if (peer == this) {
    ds->Send("200 OK", true, ds->content_type(), extra_headers,
             ds->data());
  } else {
    printf("Client %s sending to %s\n",
        name_.c_str(), peer->name().c_str());
    peer->QueueResponse("200 OK", ds->content_type(), extra_headers,
                        ds->data());
    ds->Send("200 OK", true, "text/plain", "", "");
  }
}

void ChannelMember::OnClosing(DataSocket* ds) {
  if (ds == waiting_socket_) {
    waiting_socket_ = NULL;
    timestamp_ = time(NULL);
  }
}

void ChannelMember::QueueResponse(const std::string& status,
                                  const std::string& content_type,
                                  const std::string& extra_headers,
                                  const std::string& data) {
  if (waiting_socket_) {
    assert(queue_.size() == 0);
    assert(waiting_socket_->method() == DataSocket::GET);
    bool ok = waiting_socket_->Send(status, true, content_type, extra_headers,
                                    data);
    if (!ok) {
      printf("Failed to deliver data to waiting socket\n");
    }
    waiting_socket_ = NULL;
    timestamp_ = time(NULL);
  } else {
    QueuedResponse qr;
    qr.status = status;
    qr.content_type = content_type;
    qr.extra_headers = extra_headers;
    qr.data = data;
    queue_.push(qr);
  }
}

void ChannelMember::SetWaitingSocket(DataSocket* ds) {
  assert(ds->method() == DataSocket::GET);
  if (ds && !queue_.empty()) {
    assert(waiting_socket_ == NULL);
    const QueuedResponse& response = queue_.front();
    ds->Send(response.status, true, response.content_type,
             response.extra_headers, response.data);
    queue_.pop();
  } else {
    waiting_socket_ = ds;
  }
}


//
// PeerChannel
//

// static
bool PeerChannel::IsPeerConnection(const DataSocket* ds) {
  assert(ds);
  return (ds->method() == DataSocket::POST && ds->content_length() > 0) ||
         (ds->method() == DataSocket::GET && ds->PathEquals("/sign_in"));
}

ChannelMember* PeerChannel::Lookup(DataSocket* ds) const {
  assert(ds);

  if (ds->method() != DataSocket::GET && ds->method() != DataSocket::POST)
    return NULL;

  size_t i = 0;
  for (; i < ARRAYSIZE(kRequestPaths); ++i) {
    if (ds->PathEquals(kRequestPaths[i]))
      break;
  }

  if (i == ARRAYSIZE(kRequestPaths))
    return NULL;

  std::string args(ds->request_arguments());
  static const char kPeerId[] = "peer_id=";
  size_t found = args.find(kPeerId);
  if (found == std::string::npos)
    return NULL;

  int id = atoi(&args[found + ARRAYSIZE(kPeerId) - 1]);
  Members::const_iterator iter = members_.begin();
  for (; iter != members_.end(); ++iter) {
    if (id == (*iter)->id()) {
      if (i == kWait)
        (*iter)->SetWaitingSocket(ds);
      if (i == kSignOut)
        (*iter)->set_disconnected();
      return *iter;
    }
  }

  return NULL;
}

ChannelMember* PeerChannel::IsTargetedRequest(const DataSocket* ds) const {
  assert(ds);
  // Regardless of GET or POST, we look for the peer_id parameter
  // only in the request_path.
  const std::string& path = ds->request_path();
  size_t args = path.find('?');
  if (args == std::string::npos)
    return NULL;
  size_t found;
  const char kTargetPeerIdParam[] = "to=";
  do {
    found = path.find(kTargetPeerIdParam, args);
    if (found == std::string::npos)
      return NULL;
    if (found == (args + 1) || path[found - 1] == '&') {
      found += ARRAYSIZE(kTargetPeerIdParam) - 1;
      break;
    }
    args = found + ARRAYSIZE(kTargetPeerIdParam) - 1;
  } while (true);
  int id = atoi(&path[found]);
  Members::const_iterator i = members_.begin();
  for (; i != members_.end(); ++i) {
    if ((*i)->id() == id) {
      return *i;
    }
  }
  return NULL;
}

bool PeerChannel::AddMember(DataSocket* ds) {
  assert(IsPeerConnection(ds));
  ChannelMember* new_guy = new ChannelMember(ds);
  Members failures;
  BroadcastChangedState(*new_guy, &failures);
  HandleDeliveryFailures(&failures);
  members_.push_back(new_guy);

  printf("New member added (total=%s): %s\n",
      size_t2str(members_.size()).c_str(), new_guy->name().c_str());

  // Let the newly connected peer know about other members of the channel.
  std::string content_type;
  std::string response = BuildResponseForNewMember(*new_guy, &content_type);
  ds->Send("200 Added", true, content_type, new_guy->GetPeerIdHeader(),
           response);
  return true;
}

void PeerChannel::CloseAll() {
  Members::const_iterator i = members_.begin();
  for (; i != members_.end(); ++i) {
    (*i)->QueueResponse("200 OK", "text/plain", "", "Server shutting down");
  }
  DeleteAll();
}

void PeerChannel::OnClosing(DataSocket* ds) {
  for (Members::iterator i = members_.begin(); i != members_.end(); ++i) {
    ChannelMember* m = (*i);
    m->OnClosing(ds);
    if (!m->connected()) {
      i = members_.erase(i);
      Members failures;
      BroadcastChangedState(*m, &failures);
      HandleDeliveryFailures(&failures);
      delete m;
      if (i == members_.end())
        break;
    }
  }
  printf("Total connected: %s\n", size_t2str(members_.size()).c_str());
}

void PeerChannel::CheckForTimeout() {
  for (Members::iterator i = members_.begin(); i != members_.end(); ++i) {
    ChannelMember* m = (*i);
    if (m->TimedOut()) {
      printf("Timeout: %s\n", m->name().c_str());
      m->set_disconnected();
      i = members_.erase(i);
      Members failures;
      BroadcastChangedState(*m, &failures);
      HandleDeliveryFailures(&failures);
      delete m;
      if (i == members_.end())
        break;
    }
  }
}

void PeerChannel::DeleteAll() {
  for (Members::iterator i = members_.begin(); i != members_.end(); ++i)
    delete (*i);
  members_.clear();
}

void PeerChannel::BroadcastChangedState(const ChannelMember& member,
                                        Members* delivery_failures) {
  // This function should be called prior to DataSocket::Close().
  assert(delivery_failures);

  if (!member.connected()) {
    printf("Member disconnected: %s\n", member.name().c_str());
  }

  Members::iterator i = members_.begin();
  for (; i != members_.end(); ++i) {
    if (&member != (*i)) {
      if (!(*i)->NotifyOfOtherMember(member)) {
        (*i)->set_disconnected();
        delivery_failures->push_back(*i);
        i = members_.erase(i);
        if (i == members_.end())
          break;
      }
    }
  }
}

void PeerChannel::HandleDeliveryFailures(Members* failures) {
  assert(failures);

  while (!failures->empty()) {
    Members::iterator i = failures->begin();
    ChannelMember* member = *i;
    assert(!member->connected());
    failures->erase(i);
    BroadcastChangedState(*member, failures);
    delete member;
  }
}

// Builds a simple list of "name,id\n" entries for each member.
std::string PeerChannel::BuildResponseForNewMember(const ChannelMember& member,
                                                   std::string* content_type) {
  assert(content_type);

  *content_type = "text/plain";
  // The peer itself will always be the first entry.
  std::string response(member.GetEntry());
  for (Members::iterator i = members_.begin(); i != members_.end(); ++i) {
    if (member.id() != (*i)->id()) {
      assert((*i)->connected());
      response += (*i)->GetEntry();
    }
  }

  return response;
}
