/*
 * libjingle
 * Copyright 2009 Google Inc.
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

#undef HAVE_CONFIG_H

#include "talk/session/media/srtpfilter.h"

#include <string.h>

#include <algorithm>

#include "talk/base/base64.h"
#include "talk/base/logging.h"
#include "talk/base/stringencode.h"
#include "talk/base/timeutils.h"
#include "talk/media/base/rtputils.h"

// Enable this line to turn on SRTP debugging
// #define SRTP_DEBUG

#ifdef HAVE_SRTP
#ifdef SRTP_RELATIVE_PATH
#include "srtp.h"  // NOLINT
extern "C" srtp_stream_t srtp_get_stream(srtp_t srtp, uint32_t ssrc);
#include "srtp_priv.h"  // NOLINT
#else
#include "third_party/libsrtp/include/srtp.h"
extern "C" srtp_stream_t srtp_get_stream(srtp_t srtp, uint32_t ssrc);
#include "third_party/libsrtp/include/srtp_priv.h"
#endif  // SRTP_RELATIVE_PATH
#ifdef  ENABLE_EXTERNAL_AUTH
#include "talk/session/media/externalhmac.h"
#endif  // ENABLE_EXTERNAL_AUTH
#ifdef _DEBUG
extern "C" debug_module_t mod_srtp;
extern "C" debug_module_t mod_auth;
extern "C" debug_module_t mod_cipher;
extern "C" debug_module_t mod_stat;
extern "C" debug_module_t mod_alloc;
extern "C" debug_module_t mod_aes_icm;
extern "C" debug_module_t mod_aes_hmac;
#endif
#else
// SrtpFilter needs that constant.
#define SRTP_MASTER_KEY_LEN 30
#endif  // HAVE_SRTP

namespace cricket {

const char CS_AES_CM_128_HMAC_SHA1_80[] = "AES_CM_128_HMAC_SHA1_80";
const char CS_AES_CM_128_HMAC_SHA1_32[] = "AES_CM_128_HMAC_SHA1_32";
const int SRTP_MASTER_KEY_BASE64_LEN = SRTP_MASTER_KEY_LEN * 4 / 3;
const int SRTP_MASTER_KEY_KEY_LEN = 16;
const int SRTP_MASTER_KEY_SALT_LEN = 14;

#ifndef HAVE_SRTP

// This helper function is used on systems that don't (yet) have SRTP,
// to log that the functions that require it won't do anything.
namespace {
bool SrtpNotAvailable(const char *func) {
  LOG(LS_ERROR) << func << ": SRTP is not available on your system.";
  return false;
}
}  // anonymous namespace

#endif  // !HAVE_SRTP

void EnableSrtpDebugging() {
#ifdef HAVE_SRTP
#ifdef _DEBUG
  debug_on(mod_srtp);
  debug_on(mod_auth);
  debug_on(mod_cipher);
  debug_on(mod_stat);
  debug_on(mod_alloc);
  debug_on(mod_aes_icm);
  // debug_on(mod_aes_cbc);
  // debug_on(mod_hmac);
#endif
#endif  // HAVE_SRTP
}

// NOTE: This is called from ChannelManager D'tor.
void ShutdownSrtp() {
#ifdef HAVE_SRTP
  // If srtp_dealloc is not executed then this will clear all existing sessions.
  // This should be called when application is shutting down.
  SrtpSession::Terminate();
#endif
}

SrtpFilter::SrtpFilter()
    : state_(ST_INIT),
      signal_silent_time_in_ms_(0) {
}

SrtpFilter::~SrtpFilter() {
}

bool SrtpFilter::IsActive() const {
  return state_ >= ST_ACTIVE;
}

bool SrtpFilter::SetOffer(const std::vector<CryptoParams>& offer_params,
                          ContentSource source) {
  if (!ExpectOffer(source)) {
     LOG(LS_ERROR) << "Wrong state to update SRTP offer";
     return false;
  }
  return StoreParams(offer_params, source);
}

bool SrtpFilter::SetAnswer(const std::vector<CryptoParams>& answer_params,
                           ContentSource source) {
  return DoSetAnswer(answer_params, source, true);
}

bool SrtpFilter::SetProvisionalAnswer(
    const std::vector<CryptoParams>& answer_params,
    ContentSource source) {
  return DoSetAnswer(answer_params, source, false);
}

bool SrtpFilter::SetRtpParams(const std::string& send_cs,
                              const uint8* send_key, int send_key_len,
                              const std::string& recv_cs,
                              const uint8* recv_key, int recv_key_len) {
  if (state_ == ST_ACTIVE) {
    LOG(LS_ERROR) << "Tried to set SRTP Params when filter already active";
    return false;
  }
  CreateSrtpSessions();
  if (!send_session_->SetSend(send_cs, send_key, send_key_len))
    return false;

  if (!recv_session_->SetRecv(recv_cs, recv_key, recv_key_len))
    return false;

  state_ = ST_ACTIVE;

  LOG(LS_INFO) << "SRTP activated with negotiated parameters:"
               << " send cipher_suite " << send_cs
               << " recv cipher_suite " << recv_cs;
  return true;
}

// This function is provided separately because DTLS-SRTP behaves
// differently in RTP/RTCP mux and non-mux modes.
//
// - In the non-muxed case, RTP and RTCP are keyed with different
//   keys (from different DTLS handshakes), and so we need a new
//   SrtpSession.
// - In the muxed case, they are keyed with the same keys, so
//   this function is not needed
bool SrtpFilter::SetRtcpParams(const std::string& send_cs,
                               const uint8* send_key, int send_key_len,
                               const std::string& recv_cs,
                               const uint8* recv_key, int recv_key_len) {
  // This can only be called once, but can be safely called after
  // SetRtpParams
  if (send_rtcp_session_ || recv_rtcp_session_) {
    LOG(LS_ERROR) << "Tried to set SRTCP Params when filter already active";
    return false;
  }

  send_rtcp_session_.reset(new SrtpSession());
  SignalSrtpError.repeat(send_rtcp_session_->SignalSrtpError);
  send_rtcp_session_->set_signal_silent_time(signal_silent_time_in_ms_);
  if (!send_rtcp_session_->SetRecv(send_cs, send_key, send_key_len))
    return false;

  recv_rtcp_session_.reset(new SrtpSession());
  SignalSrtpError.repeat(recv_rtcp_session_->SignalSrtpError);
  recv_rtcp_session_->set_signal_silent_time(signal_silent_time_in_ms_);
  if (!recv_rtcp_session_->SetRecv(recv_cs, recv_key, recv_key_len))
    return false;

  LOG(LS_INFO) << "SRTCP activated with negotiated parameters:"
               << " send cipher_suite " << send_cs
               << " recv cipher_suite " << recv_cs;

  return true;
}

bool SrtpFilter::ProtectRtp(void* p, int in_len, int max_len, int* out_len) {
  if (!IsActive()) {
    LOG(LS_WARNING) << "Failed to ProtectRtp: SRTP not active";
    return false;
  }
  return send_session_->ProtectRtp(p, in_len, max_len, out_len);
}

bool SrtpFilter::ProtectRtp(void* p, int in_len, int max_len, int* out_len,
                            int64* index) {
  if (!IsActive()) {
    LOG(LS_WARNING) << "Failed to ProtectRtp: SRTP not active";
    return false;
  }

  return send_session_->ProtectRtp(p, in_len, max_len, out_len, index);
}

bool SrtpFilter::ProtectRtcp(void* p, int in_len, int max_len, int* out_len) {
  if (!IsActive()) {
    LOG(LS_WARNING) << "Failed to ProtectRtcp: SRTP not active";
    return false;
  }
  if (send_rtcp_session_) {
    return send_rtcp_session_->ProtectRtcp(p, in_len, max_len, out_len);
  } else {
    return send_session_->ProtectRtcp(p, in_len, max_len, out_len);
  }
}

bool SrtpFilter::UnprotectRtp(void* p, int in_len, int* out_len) {
  if (!IsActive()) {
    LOG(LS_WARNING) << "Failed to UnprotectRtp: SRTP not active";
    return false;
  }
  return recv_session_->UnprotectRtp(p, in_len, out_len);
}

bool SrtpFilter::UnprotectRtcp(void* p, int in_len, int* out_len) {
  if (!IsActive()) {
    LOG(LS_WARNING) << "Failed to UnprotectRtcp: SRTP not active";
    return false;
  }
  if (recv_rtcp_session_) {
    return recv_rtcp_session_->UnprotectRtcp(p, in_len, out_len);
  } else {
    return recv_session_->UnprotectRtcp(p, in_len, out_len);
  }
}

bool SrtpFilter::GetRtpAuthParams(uint8** key, int* key_len, int* tag_len) {
  if (!IsActive()) {
    LOG(LS_WARNING) << "Failed to GetRtpAuthParams: SRTP not active";
    return false;
  }

  return send_session_->GetRtpAuthParams(key, key_len, tag_len);
}

void SrtpFilter::set_signal_silent_time(uint32 signal_silent_time_in_ms) {
  signal_silent_time_in_ms_ = signal_silent_time_in_ms;
  if (state_ == ST_ACTIVE) {
    send_session_->set_signal_silent_time(signal_silent_time_in_ms);
    recv_session_->set_signal_silent_time(signal_silent_time_in_ms);
    if (send_rtcp_session_)
      send_rtcp_session_->set_signal_silent_time(signal_silent_time_in_ms);
    if (recv_rtcp_session_)
      recv_rtcp_session_->set_signal_silent_time(signal_silent_time_in_ms);
  }
}

bool SrtpFilter::ExpectOffer(ContentSource source) {
  return ((state_ == ST_INIT) ||
          (state_ == ST_ACTIVE) ||
          (state_  == ST_SENTOFFER && source == CS_LOCAL) ||
          (state_  == ST_SENTUPDATEDOFFER && source == CS_LOCAL) ||
          (state_ == ST_RECEIVEDOFFER && source == CS_REMOTE) ||
          (state_ == ST_RECEIVEDUPDATEDOFFER && source == CS_REMOTE));
}

bool SrtpFilter::StoreParams(const std::vector<CryptoParams>& params,
                             ContentSource source) {
  offer_params_ = params;
  if (state_ == ST_INIT) {
    state_ = (source == CS_LOCAL) ? ST_SENTOFFER : ST_RECEIVEDOFFER;
  } else {  // state >= ST_ACTIVE
    state_ =
        (source == CS_LOCAL) ? ST_SENTUPDATEDOFFER : ST_RECEIVEDUPDATEDOFFER;
  }
  return true;
}

bool SrtpFilter::ExpectAnswer(ContentSource source) {
  return ((state_ == ST_SENTOFFER && source == CS_REMOTE) ||
          (state_ == ST_RECEIVEDOFFER && source == CS_LOCAL) ||
          (state_ == ST_SENTUPDATEDOFFER && source == CS_REMOTE) ||
          (state_ == ST_RECEIVEDUPDATEDOFFER && source == CS_LOCAL) ||
          (state_ == ST_SENTPRANSWER_NO_CRYPTO && source == CS_LOCAL) ||
          (state_ == ST_SENTPRANSWER && source == CS_LOCAL) ||
          (state_ == ST_RECEIVEDPRANSWER_NO_CRYPTO && source == CS_REMOTE) ||
          (state_ == ST_RECEIVEDPRANSWER && source == CS_REMOTE));
}

bool SrtpFilter::DoSetAnswer(const std::vector<CryptoParams>& answer_params,
                             ContentSource source,
                             bool final) {
  if (!ExpectAnswer(source)) {
    LOG(LS_ERROR) << "Invalid state for SRTP answer";
    return false;
  }

  // If the answer doesn't requests crypto complete the negotiation of an
  // unencrypted session.
  // Otherwise, finalize the parameters and apply them.
  if (answer_params.empty()) {
    if (final) {
      return ResetParams();
    } else {
      // Need to wait for the final answer to decide if
      // we should go to Active state.
      state_ = (source == CS_LOCAL) ? ST_SENTPRANSWER_NO_CRYPTO :
                                      ST_RECEIVEDPRANSWER_NO_CRYPTO;
      return true;
    }
  }
  CryptoParams selected_params;
  if (!NegotiateParams(answer_params, &selected_params))
    return false;
  const CryptoParams& send_params =
      (source == CS_REMOTE) ? selected_params : answer_params[0];
  const CryptoParams& recv_params =
      (source == CS_REMOTE) ? answer_params[0] : selected_params;
  if (!ApplyParams(send_params, recv_params)) {
    return false;
  }

  if (final) {
    offer_params_.clear();
    state_ = ST_ACTIVE;
  } else {
    state_ =
        (source == CS_LOCAL) ? ST_SENTPRANSWER : ST_RECEIVEDPRANSWER;
  }
  return true;
}

void SrtpFilter::CreateSrtpSessions() {
  send_session_.reset(new SrtpSession());
  applied_send_params_ = CryptoParams();
  recv_session_.reset(new SrtpSession());
  applied_recv_params_ = CryptoParams();

  SignalSrtpError.repeat(send_session_->SignalSrtpError);
  SignalSrtpError.repeat(recv_session_->SignalSrtpError);

  send_session_->set_signal_silent_time(signal_silent_time_in_ms_);
  recv_session_->set_signal_silent_time(signal_silent_time_in_ms_);
}

bool SrtpFilter::NegotiateParams(const std::vector<CryptoParams>& answer_params,
                                 CryptoParams* selected_params) {
  // We're processing an accept. We should have exactly one set of params,
  // unless the offer didn't mention crypto, in which case we shouldn't be here.
  bool ret = (answer_params.size() == 1U && !offer_params_.empty());
  if (ret) {
    // We should find a match between the answer params and the offered params.
    std::vector<CryptoParams>::const_iterator it;
    for (it = offer_params_.begin(); it != offer_params_.end(); ++it) {
      if (answer_params[0].Matches(*it)) {
        break;
      }
    }

    if (it != offer_params_.end()) {
      *selected_params = *it;
    } else {
      ret = false;
    }
  }

  if (!ret) {
    LOG(LS_WARNING) << "Invalid parameters in SRTP answer";
  }
  return ret;
}

bool SrtpFilter::ApplyParams(const CryptoParams& send_params,
                             const CryptoParams& recv_params) {
  // TODO(jiayl): Split this method to apply send and receive CryptoParams
  // independently, so that we can skip one method when either send or receive
  // CryptoParams is unchanged.
  if (applied_send_params_.cipher_suite == send_params.cipher_suite &&
      applied_send_params_.key_params == send_params.key_params &&
      applied_recv_params_.cipher_suite == recv_params.cipher_suite &&
      applied_recv_params_.key_params == recv_params.key_params) {
    LOG(LS_INFO) << "Applying the same SRTP parameters again. No-op.";

    // We do not want to reset the ROC if the keys are the same. So just return.
    return true;
  }
  // TODO(juberti): Zero these buffers after use.
  bool ret;
  uint8 send_key[SRTP_MASTER_KEY_LEN], recv_key[SRTP_MASTER_KEY_LEN];
  ret = (ParseKeyParams(send_params.key_params, send_key, sizeof(send_key)) &&
         ParseKeyParams(recv_params.key_params, recv_key, sizeof(recv_key)));
  if (ret) {
    CreateSrtpSessions();
    ret = (send_session_->SetSend(send_params.cipher_suite,
                                  send_key, sizeof(send_key)) &&
           recv_session_->SetRecv(recv_params.cipher_suite,
                                  recv_key, sizeof(recv_key)));
  }
  if (ret) {
    LOG(LS_INFO) << "SRTP activated with negotiated parameters:"
                 << " send cipher_suite " << send_params.cipher_suite
                 << " recv cipher_suite " << recv_params.cipher_suite;
    applied_send_params_ = send_params;
    applied_recv_params_ = recv_params;
  } else {
    LOG(LS_WARNING) << "Failed to apply negotiated SRTP parameters";
  }
  return ret;
}

bool SrtpFilter::ResetParams() {
  offer_params_.clear();
  state_ = ST_INIT;
  LOG(LS_INFO) << "SRTP reset to init state";
  return true;
}

bool SrtpFilter::ParseKeyParams(const std::string& key_params,
                                uint8* key, int len) {
  // example key_params: "inline:YUJDZGVmZ2hpSktMbW9QUXJzVHVWd3l6MTIzNDU2"

  // Fail if key-method is wrong.
  if (key_params.find("inline:") != 0) {
    return false;
  }

  // Fail if base64 decode fails, or the key is the wrong size.
  std::string key_b64(key_params.substr(7)), key_str;
  if (!talk_base::Base64::Decode(key_b64, talk_base::Base64::DO_STRICT,
                                 &key_str, NULL) ||
      static_cast<int>(key_str.size()) != len) {
    return false;
  }

  memcpy(key, key_str.c_str(), len);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// SrtpSession

#ifdef HAVE_SRTP

bool SrtpSession::inited_ = false;

SrtpSession::SrtpSession()
    : session_(NULL),
      rtp_auth_tag_len_(0),
      rtcp_auth_tag_len_(0),
      srtp_stat_(new SrtpStat()),
      last_send_seq_num_(-1) {
  sessions()->push_back(this);
  SignalSrtpError.repeat(srtp_stat_->SignalSrtpError);
}

SrtpSession::~SrtpSession() {
  sessions()->erase(std::find(sessions()->begin(), sessions()->end(), this));
  if (session_) {
    srtp_dealloc(session_);
  }
}

bool SrtpSession::SetSend(const std::string& cs, const uint8* key, int len) {
  return SetKey(ssrc_any_outbound, cs, key, len);
}

bool SrtpSession::SetRecv(const std::string& cs, const uint8* key, int len) {
  return SetKey(ssrc_any_inbound, cs, key, len);
}

bool SrtpSession::ProtectRtp(void* p, int in_len, int max_len, int* out_len) {
  if (!session_) {
    LOG(LS_WARNING) << "Failed to protect SRTP packet: no SRTP Session";
    return false;
  }

  int need_len = in_len + rtp_auth_tag_len_;  // NOLINT
  if (max_len < need_len) {
    LOG(LS_WARNING) << "Failed to protect SRTP packet: The buffer length "
                    << max_len << " is less than the needed " << need_len;
    return false;
  }

  *out_len = in_len;
  int err = srtp_protect(session_, p, out_len);
  uint32 ssrc;
  if (GetRtpSsrc(p, in_len, &ssrc)) {
    srtp_stat_->AddProtectRtpResult(ssrc, err);
  }
  int seq_num;
  GetRtpSeqNum(p, in_len, &seq_num);
  if (err != err_status_ok) {
    LOG(LS_WARNING) << "Failed to protect SRTP packet, seqnum="
                    << seq_num << ", err=" << err << ", last seqnum="
                    << last_send_seq_num_;
    return false;
  }
  last_send_seq_num_ = seq_num;
  return true;
}

bool SrtpSession::ProtectRtp(void* p, int in_len, int max_len, int* out_len,
                             int64* index) {
  if (!ProtectRtp(p, in_len, max_len, out_len)) {
    return false;
  }
  return (index) ? GetSendStreamPacketIndex(p, in_len, index) : true;
}

bool SrtpSession::ProtectRtcp(void* p, int in_len, int max_len, int* out_len) {
  if (!session_) {
    LOG(LS_WARNING) << "Failed to protect SRTCP packet: no SRTP Session";
    return false;
  }

  int need_len = in_len + sizeof(uint32) + rtcp_auth_tag_len_;  // NOLINT
  if (max_len < need_len) {
    LOG(LS_WARNING) << "Failed to protect SRTCP packet: The buffer length "
                    << max_len << " is less than the needed " << need_len;
    return false;
  }

  *out_len = in_len;
  int err = srtp_protect_rtcp(session_, p, out_len);
  srtp_stat_->AddProtectRtcpResult(err);
  if (err != err_status_ok) {
    LOG(LS_WARNING) << "Failed to protect SRTCP packet, err=" << err;
    return false;
  }
  return true;
}

bool SrtpSession::UnprotectRtp(void* p, int in_len, int* out_len) {
  if (!session_) {
    LOG(LS_WARNING) << "Failed to unprotect SRTP packet: no SRTP Session";
    return false;
  }

  *out_len = in_len;
  int err = srtp_unprotect(session_, p, out_len);
  uint32 ssrc;
  if (GetRtpSsrc(p, in_len, &ssrc)) {
    srtp_stat_->AddUnprotectRtpResult(ssrc, err);
  }
  if (err != err_status_ok) {
    LOG(LS_WARNING) << "Failed to unprotect SRTP packet, err=" << err;
    return false;
  }
  return true;
}

bool SrtpSession::UnprotectRtcp(void* p, int in_len, int* out_len) {
  if (!session_) {
    LOG(LS_WARNING) << "Failed to unprotect SRTCP packet: no SRTP Session";
    return false;
  }

  *out_len = in_len;
  int err = srtp_unprotect_rtcp(session_, p, out_len);
  srtp_stat_->AddUnprotectRtcpResult(err);
  if (err != err_status_ok) {
    LOG(LS_WARNING) << "Failed to unprotect SRTCP packet, err=" << err;
    return false;
  }
  return true;
}

bool SrtpSession::GetRtpAuthParams(uint8** key, int* key_len,
                                   int* tag_len) {
#if defined(ENABLE_EXTERNAL_AUTH)
  external_hmac_ctx_t* external_hmac = NULL;
  // stream_template will be the reference context for other streams.
  // Let's use it for getting the keys.
  srtp_stream_ctx_t* srtp_context = session_->stream_template;
  if (srtp_context && srtp_context->rtp_auth) {
    external_hmac = reinterpret_cast<external_hmac_ctx_t*>(
        srtp_context->rtp_auth->state);
  }

  if (!external_hmac) {
    LOG(LS_ERROR) << "Failed to get auth keys from libsrtp!.";
    return false;
  }

  *key = external_hmac->key;
  *key_len = external_hmac->key_length;
  *tag_len = rtp_auth_tag_len_;
  return true;
#else
  return false;
#endif
}

bool SrtpSession::GetSendStreamPacketIndex(void* p, int in_len, int64* index) {
  srtp_hdr_t* hdr = reinterpret_cast<srtp_hdr_t*>(p);
  srtp_stream_ctx_t* stream = srtp_get_stream(session_, hdr->ssrc);
  if (stream == NULL)
    return false;

  // Shift packet index, put into network byte order
  *index = be64_to_cpu(rdbx_get_packet_index(&stream->rtp_rdbx) << 16);
  return true;
}

void SrtpSession::set_signal_silent_time(uint32 signal_silent_time_in_ms) {
  srtp_stat_->set_signal_silent_time(signal_silent_time_in_ms);
}

bool SrtpSession::SetKey(int type, const std::string& cs,
                         const uint8* key, int len) {
  if (session_) {
    LOG(LS_ERROR) << "Failed to create SRTP session: "
                  << "SRTP session already created";
    return false;
  }

  if (!Init()) {
    return false;
  }

  srtp_policy_t policy;
  memset(&policy, 0, sizeof(policy));

  if (cs == CS_AES_CM_128_HMAC_SHA1_80) {
    crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
    crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);
  } else if (cs == CS_AES_CM_128_HMAC_SHA1_32) {
    crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy.rtp);   // rtp is 32,
    crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);  // rtcp still 80
  } else {
    LOG(LS_WARNING) << "Failed to create SRTP session: unsupported"
                    << " cipher_suite " << cs.c_str();
    return false;
  }

  if (!key || len != SRTP_MASTER_KEY_LEN) {
    LOG(LS_WARNING) << "Failed to create SRTP session: invalid key";
    return false;
  }

  policy.ssrc.type = static_cast<ssrc_type_t>(type);
  policy.ssrc.value = 0;
  policy.key = const_cast<uint8*>(key);
  // TODO(astor) parse window size from WSH session-param
  policy.window_size = 1024;
  policy.allow_repeat_tx = 1;
  // If external authentication option is enabled, supply custom auth module
  // id EXTERNAL_HMAC_SHA1 in the policy structure.
  // We want to set this option only for rtp packets.
  // By default policy structure is initialized to HMAC_SHA1.
#if defined(ENABLE_EXTERNAL_AUTH)
  // Enable external HMAC authentication only for outgoing streams.
  if (type == ssrc_any_outbound) {
    policy.rtp.auth_type = EXTERNAL_HMAC_SHA1;
  }
#endif
  policy.next = NULL;

  int err = srtp_create(&session_, &policy);
  if (err != err_status_ok) {
    LOG(LS_ERROR) << "Failed to create SRTP session, err=" << err;
    return false;
  }


  rtp_auth_tag_len_ = policy.rtp.auth_tag_len;
  rtcp_auth_tag_len_ = policy.rtcp.auth_tag_len;
  return true;
}

bool SrtpSession::Init() {
  if (!inited_) {
    int err;
    err = srtp_init();
    if (err != err_status_ok) {
      LOG(LS_ERROR) << "Failed to init SRTP, err=" << err;
      return false;
    }

    err = srtp_install_event_handler(&SrtpSession::HandleEventThunk);
    if (err != err_status_ok) {
      LOG(LS_ERROR) << "Failed to install SRTP event handler, err=" << err;
      return false;
    }
#if defined(ENABLE_EXTERNAL_AUTH)
    err = external_crypto_init();
    if (err != err_status_ok) {
      LOG(LS_ERROR) << "Failed to initialize fake auth, err=" << err;
      return false;
    }
#endif
    inited_ = true;
  }

  return true;
}

void SrtpSession::Terminate() {
  if (inited_) {
    int err = srtp_shutdown();
    if (err) {
      LOG(LS_ERROR) << "srtp_shutdown failed. err=" << err;
      return;
    }
    inited_ = false;
  }
}

void SrtpSession::HandleEvent(const srtp_event_data_t* ev) {
  switch (ev->event) {
    case event_ssrc_collision:
      LOG(LS_INFO) << "SRTP event: SSRC collision";
      break;
    case event_key_soft_limit:
      LOG(LS_INFO) << "SRTP event: reached soft key usage limit";
      break;
    case event_key_hard_limit:
      LOG(LS_INFO) << "SRTP event: reached hard key usage limit";
      break;
    case event_packet_index_limit:
      LOG(LS_INFO) << "SRTP event: reached hard packet limit (2^48 packets)";
      break;
    default:
      LOG(LS_INFO) << "SRTP event: unknown " << ev->event;
      break;
  }
}

void SrtpSession::HandleEventThunk(srtp_event_data_t* ev) {
  for (std::list<SrtpSession*>::iterator it = sessions()->begin();
       it != sessions()->end(); ++it) {
    if ((*it)->session_ == ev->session) {
      (*it)->HandleEvent(ev);
      break;
    }
  }
}

std::list<SrtpSession*>* SrtpSession::sessions() {
  LIBJINGLE_DEFINE_STATIC_LOCAL(std::list<SrtpSession*>, sessions, ());
  return &sessions;
}

#else   // !HAVE_SRTP

// On some systems, SRTP is not (yet) available.

SrtpSession::SrtpSession() {
  LOG(WARNING) << "SRTP implementation is missing.";
}

SrtpSession::~SrtpSession() {
}

bool SrtpSession::SetSend(const std::string& cs, const uint8* key, int len) {
  return SrtpNotAvailable(__FUNCTION__);
}

bool SrtpSession::SetRecv(const std::string& cs, const uint8* key, int len) {
  return SrtpNotAvailable(__FUNCTION__);
}

bool SrtpSession::ProtectRtp(void* data, int in_len, int max_len,
                             int* out_len) {
  return SrtpNotAvailable(__FUNCTION__);
}

bool SrtpSession::ProtectRtcp(void* data, int in_len, int max_len,
                              int* out_len) {
  return SrtpNotAvailable(__FUNCTION__);
}

bool SrtpSession::UnprotectRtp(void* data, int in_len, int* out_len) {
  return SrtpNotAvailable(__FUNCTION__);
}

bool SrtpSession::UnprotectRtcp(void* data, int in_len, int* out_len) {
  return SrtpNotAvailable(__FUNCTION__);
}

void SrtpSession::set_signal_silent_time(uint32 signal_silent_time) {
  // Do nothing.
}

#endif  // HAVE_SRTP

///////////////////////////////////////////////////////////////////////////////
// SrtpStat

#ifdef HAVE_SRTP

SrtpStat::SrtpStat()
    : signal_silent_time_(1000) {
}

void SrtpStat::AddProtectRtpResult(uint32 ssrc, int result) {
  FailureKey key;
  key.ssrc = ssrc;
  key.mode = SrtpFilter::PROTECT;
  switch (result) {
    case err_status_ok:
      key.error = SrtpFilter::ERROR_NONE;
      break;
    case err_status_auth_fail:
      key.error = SrtpFilter::ERROR_AUTH;
      break;
    default:
      key.error = SrtpFilter::ERROR_FAIL;
  }
  HandleSrtpResult(key);
}

void SrtpStat::AddUnprotectRtpResult(uint32 ssrc, int result) {
  FailureKey key;
  key.ssrc = ssrc;
  key.mode = SrtpFilter::UNPROTECT;
  switch (result) {
    case err_status_ok:
      key.error = SrtpFilter::ERROR_NONE;
      break;
    case err_status_auth_fail:
      key.error = SrtpFilter::ERROR_AUTH;
      break;
    case err_status_replay_fail:
    case err_status_replay_old:
      key.error = SrtpFilter::ERROR_REPLAY;
      break;
    default:
      key.error = SrtpFilter::ERROR_FAIL;
  }
  HandleSrtpResult(key);
}

void SrtpStat::AddProtectRtcpResult(int result) {
  AddProtectRtpResult(0U, result);
}

void SrtpStat::AddUnprotectRtcpResult(int result) {
  AddUnprotectRtpResult(0U, result);
}

void SrtpStat::HandleSrtpResult(const SrtpStat::FailureKey& key) {
  // Handle some cases where error should be signalled right away. For other
  // errors, trigger error for the first time seeing it.  After that, silent
  // the same error for a certain amount of time (default 1 sec).
  if (key.error != SrtpFilter::ERROR_NONE) {
    // For errors, signal first time and wait for 1 sec.
    FailureStat* stat = &(failures_[key]);
    uint32 current_time = talk_base::Time();
    if (stat->last_signal_time == 0 ||
        talk_base::TimeDiff(current_time, stat->last_signal_time) >
        static_cast<int>(signal_silent_time_)) {
      SignalSrtpError(key.ssrc, key.mode, key.error);
      stat->last_signal_time = current_time;
    }
  }
}

#else   // !HAVE_SRTP

// On some systems, SRTP is not (yet) available.

SrtpStat::SrtpStat()
    : signal_silent_time_(1000) {
  LOG(WARNING) << "SRTP implementation is missing.";
}

void SrtpStat::AddProtectRtpResult(uint32 ssrc, int result) {
  SrtpNotAvailable(__FUNCTION__);
}

void SrtpStat::AddUnprotectRtpResult(uint32 ssrc, int result) {
  SrtpNotAvailable(__FUNCTION__);
}

void SrtpStat::AddProtectRtcpResult(int result) {
  SrtpNotAvailable(__FUNCTION__);
}

void SrtpStat::AddUnprotectRtcpResult(int result) {
  SrtpNotAvailable(__FUNCTION__);
}

void SrtpStat::HandleSrtpResult(const SrtpStat::FailureKey& key) {
  SrtpNotAvailable(__FUNCTION__);
}

#endif  // HAVE_SRTP

}  // namespace cricket
