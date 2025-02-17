/*
 * libjingle
 * Copyright 2011, Google Inc.
 * Portions Copyright 2011, RTFM, Inc.
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


#include <algorithm>
#include <set>
#include <string>

#include "talk/base/gunit.h"
#include "talk/base/helpers.h"
#include "talk/base/scoped_ptr.h"
#include "talk/base/ssladapter.h"
#include "talk/base/sslconfig.h"
#include "talk/base/sslidentity.h"
#include "talk/base/sslstreamadapter.h"
#include "talk/base/stream.h"

static const int kBlockSize = 4096;
static const char kAES_CM_HMAC_SHA1_80[] = "AES_CM_128_HMAC_SHA1_80";
static const char kAES_CM_HMAC_SHA1_32[] = "AES_CM_128_HMAC_SHA1_32";
static const char kExporterLabel[] = "label";
static const unsigned char kExporterContext[] = "context";
static int kExporterContextLen = sizeof(kExporterContext);

static const char kRSA_PRIVATE_KEY_PEM[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIICdwIBADANBgkqhkiG9w0BAQEFAASCAmEwggJdAgEAAoGBAMYRkbhmI7kVA/rM\n"
    "czsZ+6JDhDvnkF+vn6yCAGuRPV03zuRqZtDy4N4to7PZu9PjqrRl7nDMXrG3YG9y\n"
    "rlIAZ72KjcKKFAJxQyAKLCIdawKRyp8RdK3LEySWEZb0AV58IadqPZDTNHHRX8dz\n"
    "5aTSMsbbkZ+C/OzTnbiMqLL/vg6jAgMBAAECgYAvgOs4FJcgvp+TuREx7YtiYVsH\n"
    "mwQPTum2z/8VzWGwR8BBHBvIpVe1MbD/Y4seyI2aco/7UaisatSgJhsU46/9Y4fq\n"
    "2TwXH9QANf4at4d9n/R6rzwpAJOpgwZgKvdQjkfrKTtgLV+/dawvpxUYkRH4JZM1\n"
    "CVGukMfKNrSVH4Ap4QJBAOJmGV1ASPnB4r4nc99at7JuIJmd7fmuVUwUgYi4XgaR\n"
    "WhScBsgYwZ/JoywdyZJgnbcrTDuVcWG56B3vXbhdpMsCQQDf9zeJrjnPZ3Cqm79y\n"
    "kdqANep0uwZciiNiWxsQrCHztywOvbFhdp8iYVFG9EK8DMY41Y5TxUwsHD+67zao\n"
    "ZNqJAkEA1suLUP/GvL8IwuRneQd2tWDqqRQ/Td3qq03hP7e77XtF/buya3Ghclo5\n"
    "54czUR89QyVfJEC6278nzA7n2h1uVQJAcG6mztNL6ja/dKZjYZye2CY44QjSlLo0\n"
    "MTgTSjdfg/28fFn2Jjtqf9Pi/X+50LWI/RcYMC2no606wRk9kyOuIQJBAK6VSAim\n"
    "1pOEjsYQn0X5KEIrz1G3bfCbB848Ime3U2/FWlCHMr6ch8kCZ5d1WUeJD3LbwMNG\n"
    "UCXiYxSsu20QNVw=\n"
    "-----END RSA PRIVATE KEY-----\n";

static const char kCERT_PEM[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBmTCCAQKgAwIBAgIEbzBSAjANBgkqhkiG9w0BAQsFADARMQ8wDQYDVQQDEwZX\n"
    "ZWJSVEMwHhcNMTQwMTAyMTgyNDQ3WhcNMTQwMjAxMTgyNDQ3WjARMQ8wDQYDVQQD\n"
    "EwZXZWJSVEMwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAMYRkbhmI7kVA/rM\n"
    "czsZ+6JDhDvnkF+vn6yCAGuRPV03zuRqZtDy4N4to7PZu9PjqrRl7nDMXrG3YG9y\n"
    "rlIAZ72KjcKKFAJxQyAKLCIdawKRyp8RdK3LEySWEZb0AV58IadqPZDTNHHRX8dz\n"
    "5aTSMsbbkZ+C/OzTnbiMqLL/vg6jAgMBAAEwDQYJKoZIhvcNAQELBQADgYEAUflI\n"
    "VUe5Krqf5RVa5C3u/UTAOAUJBiDS3VANTCLBxjuMsvqOG0WvaYWP3HYPgrz0jXK2\n"
    "LJE/mGw3MyFHEqi81jh95J+ypl6xKW6Rm8jKLR87gUvCaVYn/Z4/P3AqcQTB7wOv\n"
    "UD0A8qfhfDM+LK6rPAnCsVN0NRDY3jvd6rzix9M=\n"
    "-----END CERTIFICATE-----\n";

#define MAYBE_SKIP_TEST(feature)                    \
  if (!(talk_base::SSLStreamAdapter::feature())) {  \
    LOG(LS_INFO) << "Feature disabled... skipping"; \
    return;                                         \
  }

class SSLStreamAdapterTestBase;

class SSLDummyStream : public talk_base::StreamInterface,
                       public sigslot::has_slots<> {
 public:
  explicit SSLDummyStream(SSLStreamAdapterTestBase *test,
                          const std::string &side,
                          talk_base::FifoBuffer *in,
                          talk_base::FifoBuffer *out) :
      test_(test),
      side_(side),
      in_(in),
      out_(out),
      first_packet_(true) {
    in_->SignalEvent.connect(this, &SSLDummyStream::OnEventIn);
    out_->SignalEvent.connect(this, &SSLDummyStream::OnEventOut);
  }

  virtual talk_base::StreamState GetState() const { return talk_base::SS_OPEN; }

  virtual talk_base::StreamResult Read(void* buffer, size_t buffer_len,
                                       size_t* read, int* error) {
    talk_base::StreamResult r;

    r = in_->Read(buffer, buffer_len, read, error);
    if (r == talk_base::SR_BLOCK)
      return talk_base::SR_BLOCK;
    if (r == talk_base::SR_EOS)
      return talk_base::SR_EOS;

    if (r != talk_base::SR_SUCCESS) {
      ADD_FAILURE();
      return talk_base::SR_ERROR;
    }

    return talk_base::SR_SUCCESS;
  }

  // Catch readability events on in and pass them up.
  virtual void OnEventIn(talk_base::StreamInterface *stream, int sig,
                         int err) {
    int mask = (talk_base::SE_READ | talk_base::SE_CLOSE);

    if (sig & mask) {
      LOG(LS_INFO) << "SSLDummyStream::OnEvent side=" << side_ <<  " sig="
        << sig << " forwarding upward";
      PostEvent(sig & mask, 0);
    }
  }

  // Catch writeability events on out and pass them up.
  virtual void OnEventOut(talk_base::StreamInterface *stream, int sig,
                          int err) {
    if (sig & talk_base::SE_WRITE) {
      LOG(LS_INFO) << "SSLDummyStream::OnEvent side=" << side_ <<  " sig="
        << sig << " forwarding upward";

      PostEvent(sig & talk_base::SE_WRITE, 0);
    }
  }

  // Write to the outgoing FifoBuffer
  talk_base::StreamResult WriteData(const void* data, size_t data_len,
                                    size_t* written, int* error) {
    return out_->Write(data, data_len, written, error);
  }

  // Defined later
  virtual talk_base::StreamResult Write(const void* data, size_t data_len,
                                        size_t* written, int* error);

  virtual void Close() {
    LOG(LS_INFO) << "Closing outbound stream";
    out_->Close();
  }

 private:
  SSLStreamAdapterTestBase *test_;
  const std::string side_;
  talk_base::FifoBuffer *in_;
  talk_base::FifoBuffer *out_;
  bool first_packet_;
};

static const int kFifoBufferSize = 4096;

class SSLStreamAdapterTestBase : public testing::Test,
                                 public sigslot::has_slots<> {
 public:
  SSLStreamAdapterTestBase(const std::string& client_cert_pem,
                           const std::string& client_private_key_pem,
                           bool dtls) :
      client_buffer_(kFifoBufferSize), server_buffer_(kFifoBufferSize),
      client_stream_(
          new SSLDummyStream(this, "c2s", &client_buffer_, &server_buffer_)),
      server_stream_(
          new SSLDummyStream(this, "s2c", &server_buffer_, &client_buffer_)),
      client_ssl_(talk_base::SSLStreamAdapter::Create(client_stream_)),
      server_ssl_(talk_base::SSLStreamAdapter::Create(server_stream_)),
      client_identity_(NULL), server_identity_(NULL),
      delay_(0), mtu_(1460), loss_(0), lose_first_packet_(false),
      damage_(false), dtls_(dtls),
      handshake_wait_(5000), identities_set_(false) {
    // Set use of the test RNG to get predictable loss patterns.
    talk_base::SetRandomTestMode(true);

    // Set up the slots
    client_ssl_->SignalEvent.connect(this, &SSLStreamAdapterTestBase::OnEvent);
    server_ssl_->SignalEvent.connect(this, &SSLStreamAdapterTestBase::OnEvent);

    if (!client_cert_pem.empty() && !client_private_key_pem.empty()) {
      client_identity_ = talk_base::SSLIdentity::FromPEMStrings(
          client_private_key_pem, client_cert_pem);
    } else {
      client_identity_ = talk_base::SSLIdentity::Generate("client");
    }
    server_identity_ = talk_base::SSLIdentity::Generate("server");

    client_ssl_->SetIdentity(client_identity_);
    server_ssl_->SetIdentity(server_identity_);
  }

  ~SSLStreamAdapterTestBase() {
    // Put it back for the next test.
    talk_base::SetRandomTestMode(false);
  }

  static void SetUpTestCase() {
    talk_base::InitializeSSL();
  }

  static void TearDownTestCase() {
    talk_base::CleanupSSL();
  }

  // Recreate the client/server identities with the specified validity period.
  // |not_before| and |not_after| are offsets from the current time in number
  // of seconds.
  void ResetIdentitiesWithValidity(int not_before, int not_after) {
    client_stream_ =
        new SSLDummyStream(this, "c2s", &client_buffer_, &server_buffer_);
    server_stream_ =
        new SSLDummyStream(this, "s2c", &server_buffer_, &client_buffer_);

    client_ssl_.reset(talk_base::SSLStreamAdapter::Create(client_stream_));
    server_ssl_.reset(talk_base::SSLStreamAdapter::Create(server_stream_));

    client_ssl_->SignalEvent.connect(this, &SSLStreamAdapterTestBase::OnEvent);
    server_ssl_->SignalEvent.connect(this, &SSLStreamAdapterTestBase::OnEvent);

    talk_base::SSLIdentityParams client_params;
    client_params.common_name = "client";
    client_params.not_before = not_before;
    client_params.not_after = not_after;
    client_identity_ = talk_base::SSLIdentity::GenerateForTest(client_params);

    talk_base::SSLIdentityParams server_params;
    server_params.common_name = "server";
    server_params.not_before = not_before;
    server_params.not_after = not_after;
    server_identity_ = talk_base::SSLIdentity::GenerateForTest(server_params);

    client_ssl_->SetIdentity(client_identity_);
    server_ssl_->SetIdentity(server_identity_);
  }

  virtual void OnEvent(talk_base::StreamInterface *stream, int sig, int err) {
    LOG(LS_INFO) << "SSLStreamAdapterTestBase::OnEvent sig=" << sig;

    if (sig & talk_base::SE_READ) {
      ReadData(stream);
    }

    if ((stream == client_ssl_.get()) && (sig & talk_base::SE_WRITE)) {
      WriteData();
    }
  }

  void SetPeerIdentitiesByDigest(bool correct) {
    unsigned char digest[20];
    size_t digest_len;
    bool rv;

    LOG(LS_INFO) << "Setting peer identities by digest";

    rv = server_identity_->certificate().ComputeDigest(talk_base::DIGEST_SHA_1,
                                                       digest, 20,
                                                       &digest_len);
    ASSERT_TRUE(rv);
    if (!correct) {
      LOG(LS_INFO) << "Setting bogus digest for server cert";
      digest[0]++;
    }
    rv = client_ssl_->SetPeerCertificateDigest(talk_base::DIGEST_SHA_1, digest,
                                               digest_len);
    ASSERT_TRUE(rv);


    rv = client_identity_->certificate().ComputeDigest(talk_base::DIGEST_SHA_1,
                                                       digest, 20, &digest_len);
    ASSERT_TRUE(rv);
    if (!correct) {
      LOG(LS_INFO) << "Setting bogus digest for client cert";
      digest[0]++;
    }
    rv = server_ssl_->SetPeerCertificateDigest(talk_base::DIGEST_SHA_1, digest,
                                               digest_len);
    ASSERT_TRUE(rv);

    identities_set_ = true;
  }

  void TestHandshake(bool expect_success = true) {
    server_ssl_->SetMode(dtls_ ? talk_base::SSL_MODE_DTLS :
                         talk_base::SSL_MODE_TLS);
    client_ssl_->SetMode(dtls_ ? talk_base::SSL_MODE_DTLS :
                         talk_base::SSL_MODE_TLS);

    if (!dtls_) {
      // Make sure we simulate a reliable network for TLS.
      // This is just a check to make sure that people don't write wrong
      // tests.
      ASSERT((mtu_ == 1460) && (loss_ == 0) && (lose_first_packet_ == 0));
    }

    if (!identities_set_)
      SetPeerIdentitiesByDigest(true);

    // Start the handshake
    int rv;

    server_ssl_->SetServerRole();
    rv = server_ssl_->StartSSLWithPeer();
    ASSERT_EQ(0, rv);

    rv = client_ssl_->StartSSLWithPeer();
    ASSERT_EQ(0, rv);

    // Now run the handshake
    if (expect_success) {
      EXPECT_TRUE_WAIT((client_ssl_->GetState() == talk_base::SS_OPEN)
                       && (server_ssl_->GetState() == talk_base::SS_OPEN),
                       handshake_wait_);
    } else {
      EXPECT_TRUE_WAIT(client_ssl_->GetState() == talk_base::SS_CLOSED,
                       handshake_wait_);
    }
  }

  talk_base::StreamResult DataWritten(SSLDummyStream *from, const void *data,
                                      size_t data_len, size_t *written,
                                      int *error) {
    // Randomly drop loss_ percent of packets
    if (talk_base::CreateRandomId() % 100 < static_cast<uint32>(loss_)) {
      LOG(LS_INFO) << "Randomly dropping packet, size=" << data_len;
      *written = data_len;
      return talk_base::SR_SUCCESS;
    }
    if (dtls_ && (data_len > mtu_)) {
      LOG(LS_INFO) << "Dropping packet > mtu, size=" << data_len;
      *written = data_len;
      return talk_base::SR_SUCCESS;
    }

    // Optionally damage application data (type 23). Note that we don't damage
    // handshake packets and we damage the last byte to keep the header
    // intact but break the MAC.
    if (damage_ && (*static_cast<const unsigned char *>(data) == 23)) {
      std::vector<char> buf(data_len);

      LOG(LS_INFO) << "Damaging packet";

      memcpy(&buf[0], data, data_len);
      buf[data_len - 1]++;

      return from->WriteData(&buf[0], data_len, written, error);
    }

    return from->WriteData(data, data_len, written, error);
  }

  void SetDelay(int delay) {
    delay_ = delay;
  }
  int GetDelay() { return delay_; }

  void SetLoseFirstPacket(bool lose) {
    lose_first_packet_ = lose;
  }
  bool GetLoseFirstPacket() { return lose_first_packet_; }

  void SetLoss(int percent) {
    loss_ = percent;
  }

  void SetDamage() {
    damage_ = true;
  }

  void SetMtu(size_t mtu) {
    mtu_ = mtu;
  }

  void SetHandshakeWait(int wait) {
    handshake_wait_ = wait;
  }

  void SetDtlsSrtpCiphers(const std::vector<std::string> &ciphers,
    bool client) {
    if (client)
      client_ssl_->SetDtlsSrtpCiphers(ciphers);
    else
      server_ssl_->SetDtlsSrtpCiphers(ciphers);
  }

  bool GetDtlsSrtpCipher(bool client, std::string *retval) {
    if (client)
      return client_ssl_->GetDtlsSrtpCipher(retval);
    else
      return server_ssl_->GetDtlsSrtpCipher(retval);
  }

  bool GetPeerCertificate(bool client, talk_base::SSLCertificate** cert) {
    if (client)
      return client_ssl_->GetPeerCertificate(cert);
    else
      return server_ssl_->GetPeerCertificate(cert);
  }

  bool ExportKeyingMaterial(const char *label,
                            const unsigned char *context,
                            size_t context_len,
                            bool use_context,
                            bool client,
                            unsigned char *result,
                            size_t result_len) {
    if (client)
      return client_ssl_->ExportKeyingMaterial(label,
                                               context, context_len,
                                               use_context,
                                               result, result_len);
    else
      return server_ssl_->ExportKeyingMaterial(label,
                                               context, context_len,
                                               use_context,
                                               result, result_len);
  }

  // To be implemented by subclasses.
  virtual void WriteData() = 0;
  virtual void ReadData(talk_base::StreamInterface *stream) = 0;
  virtual void TestTransfer(int size) = 0;

 protected:
  talk_base::FifoBuffer client_buffer_;
  talk_base::FifoBuffer server_buffer_;
  SSLDummyStream *client_stream_;  // freed by client_ssl_ destructor
  SSLDummyStream *server_stream_;  // freed by server_ssl_ destructor
  talk_base::scoped_ptr<talk_base::SSLStreamAdapter> client_ssl_;
  talk_base::scoped_ptr<talk_base::SSLStreamAdapter> server_ssl_;
  talk_base::SSLIdentity *client_identity_;  // freed by client_ssl_ destructor
  talk_base::SSLIdentity *server_identity_;  // freed by server_ssl_ destructor
  int delay_;
  size_t mtu_;
  int loss_;
  bool lose_first_packet_;
  bool damage_;
  bool dtls_;
  int handshake_wait_;
  bool identities_set_;
};

class SSLStreamAdapterTestTLS : public SSLStreamAdapterTestBase {
 public:
  SSLStreamAdapterTestTLS() :
      SSLStreamAdapterTestBase("", "", false) {
  };

  // Test data transfer for TLS
  virtual void TestTransfer(int size) {
    LOG(LS_INFO) << "Starting transfer test with " << size << " bytes";
    // Create some dummy data to send.
    size_t received;

    send_stream_.ReserveSize(size);
    for (int i = 0; i < size; ++i) {
      char ch = static_cast<char>(i);
      send_stream_.Write(&ch, 1, NULL, NULL);
    }
    send_stream_.Rewind();

    // Prepare the receive stream.
    recv_stream_.ReserveSize(size);

    // Start sending
    WriteData();

    // Wait for the client to close
    EXPECT_TRUE_WAIT(server_ssl_->GetState() == talk_base::SS_CLOSED, 10000);

    // Now check the data
    recv_stream_.GetSize(&received);

    EXPECT_EQ(static_cast<size_t>(size), received);
    EXPECT_EQ(0, memcmp(send_stream_.GetBuffer(),
                        recv_stream_.GetBuffer(), size));
  }

  void WriteData() {
    size_t position, tosend, size;
    talk_base::StreamResult rv;
    size_t sent;
    char block[kBlockSize];

    send_stream_.GetSize(&size);
    if (!size)
      return;

    for (;;) {
      send_stream_.GetPosition(&position);
      if (send_stream_.Read(block, sizeof(block), &tosend, NULL) !=
          talk_base::SR_EOS) {
        rv = client_ssl_->Write(block, tosend, &sent, 0);

        if (rv == talk_base::SR_SUCCESS) {
          send_stream_.SetPosition(position + sent);
          LOG(LS_VERBOSE) << "Sent: " << position + sent;
        } else if (rv == talk_base::SR_BLOCK) {
          LOG(LS_VERBOSE) << "Blocked...";
          send_stream_.SetPosition(position);
          break;
        } else {
          ADD_FAILURE();
          break;
        }
      } else {
        // Now close
        LOG(LS_INFO) << "Wrote " << position << " bytes. Closing";
        client_ssl_->Close();
        break;
      }
    }
  };

  virtual void ReadData(talk_base::StreamInterface *stream) {
    char buffer[1600];
    size_t bread;
    int err2;
    talk_base::StreamResult r;

    for (;;) {
      r = stream->Read(buffer, sizeof(buffer), &bread, &err2);

      if (r == talk_base::SR_ERROR || r == talk_base::SR_EOS) {
        // Unfortunately, errors are the way that the stream adapter
        // signals close in OpenSSL
        stream->Close();
        return;
      }

      if (r == talk_base::SR_BLOCK)
        break;

      ASSERT_EQ(talk_base::SR_SUCCESS, r);
      LOG(LS_INFO) << "Read " << bread;

      recv_stream_.Write(buffer, bread, NULL, NULL);
    }
  }

 private:
  talk_base::MemoryStream send_stream_;
  talk_base::MemoryStream recv_stream_;
};

class SSLStreamAdapterTestDTLS : public SSLStreamAdapterTestBase {
 public:
  SSLStreamAdapterTestDTLS() :
      SSLStreamAdapterTestBase("", "", true),
      packet_size_(1000), count_(0), sent_(0) {
  }

  SSLStreamAdapterTestDTLS(const std::string& cert_pem,
                           const std::string& private_key_pem) :
      SSLStreamAdapterTestBase(cert_pem, private_key_pem, true),
      packet_size_(1000), count_(0), sent_(0) {
  }

  virtual void WriteData() {
    unsigned char *packet = new unsigned char[1600];

    do {
      memset(packet, sent_ & 0xff, packet_size_);
      *(reinterpret_cast<uint32_t *>(packet)) = sent_;

      size_t sent;
      int rv = client_ssl_->Write(packet, packet_size_, &sent, 0);
      if (rv == talk_base::SR_SUCCESS) {
        LOG(LS_VERBOSE) << "Sent: " << sent_;
        sent_++;
      } else if (rv == talk_base::SR_BLOCK) {
        LOG(LS_VERBOSE) << "Blocked...";
        break;
      } else {
        ADD_FAILURE();
        break;
      }
    } while (sent_ < count_);

    delete [] packet;
  }

  virtual void ReadData(talk_base::StreamInterface *stream) {
    unsigned char buffer[2000];
    size_t bread;
    int err2;
    talk_base::StreamResult r;

    for (;;) {
      r = stream->Read(buffer, 2000, &bread, &err2);

      if (r == talk_base::SR_ERROR) {
        // Unfortunately, errors are the way that the stream adapter
        // signals close right now
        stream->Close();
        return;
      }

      if (r == talk_base::SR_BLOCK)
        break;

      ASSERT_EQ(talk_base::SR_SUCCESS, r);
      LOG(LS_INFO) << "Read " << bread;

      // Now parse the datagram
      ASSERT_EQ(packet_size_, bread);
      unsigned char* ptr_to_buffer = buffer;
      uint32_t packet_num = *(reinterpret_cast<uint32_t *>(ptr_to_buffer));

      for (size_t i = 4; i < packet_size_; i++) {
        ASSERT_EQ((packet_num & 0xff), buffer[i]);
      }
      received_.insert(packet_num);
    }
  }

  virtual void TestTransfer(int count) {
    count_ = count;

    WriteData();

    EXPECT_TRUE_WAIT(sent_ == count_, 10000);
    LOG(LS_INFO) << "sent_ == " << sent_;

    if (damage_) {
      WAIT(false, 2000);
      EXPECT_EQ(0U, received_.size());
    } else if (loss_ == 0) {
        EXPECT_EQ_WAIT(static_cast<size_t>(sent_), received_.size(), 1000);
    } else {
      LOG(LS_INFO) << "Sent " << sent_ << " packets; received " <<
          received_.size();
    }
  };

 private:
  size_t packet_size_;
  int count_;
  int sent_;
  std::set<int> received_;
};


talk_base::StreamResult SSLDummyStream::Write(const void* data, size_t data_len,
                                              size_t* written, int* error) {
  *written = data_len;

  LOG(LS_INFO) << "Writing to loopback " << data_len;

  if (first_packet_) {
    first_packet_ = false;
    if (test_->GetLoseFirstPacket()) {
      LOG(LS_INFO) << "Losing initial packet of length " << data_len;
      return talk_base::SR_SUCCESS;
    }
  }

  return test_->DataWritten(this, data, data_len, written, error);

  return talk_base::SR_SUCCESS;
};

class SSLStreamAdapterTestDTLSFromPEMStrings : public SSLStreamAdapterTestDTLS {
 public:
  SSLStreamAdapterTestDTLSFromPEMStrings() :
      SSLStreamAdapterTestDTLS(kCERT_PEM, kRSA_PRIVATE_KEY_PEM) {
  }
};

// Basic tests: TLS

// Test that we cannot read/write if we have not yet handshaked.
// This test only applies to NSS because OpenSSL has passthrough
// semantics for I/O before the handshake is started.
#if SSL_USE_NSS
TEST_F(SSLStreamAdapterTestTLS, TestNoReadWriteBeforeConnect) {
  talk_base::StreamResult rv;
  char block[kBlockSize];
  size_t dummy;

  rv = client_ssl_->Write(block, sizeof(block), &dummy, NULL);
  ASSERT_EQ(talk_base::SR_BLOCK, rv);

  rv = client_ssl_->Read(block, sizeof(block), &dummy, NULL);
  ASSERT_EQ(talk_base::SR_BLOCK, rv);
}
#endif


// Test that we can make a handshake work
TEST_F(SSLStreamAdapterTestTLS, TestTLSConnect) {
  TestHandshake();
};

// Test transfer -- trivial
TEST_F(SSLStreamAdapterTestTLS, TestTLSTransfer) {
  TestHandshake();
  TestTransfer(100000);
};

// Test read-write after close.
TEST_F(SSLStreamAdapterTestTLS, ReadWriteAfterClose) {
  TestHandshake();
  TestTransfer(100000);
  client_ssl_->Close();

  talk_base::StreamResult rv;
  char block[kBlockSize];
  size_t dummy;

  // It's an error to write after closed.
  rv = client_ssl_->Write(block, sizeof(block), &dummy, NULL);
  ASSERT_EQ(talk_base::SR_ERROR, rv);

  // But after closed read gives you EOS.
  rv = client_ssl_->Read(block, sizeof(block), &dummy, NULL);
  ASSERT_EQ(talk_base::SR_EOS, rv);
};

// Test a handshake with a bogus peer digest
TEST_F(SSLStreamAdapterTestTLS, TestTLSBogusDigest) {
  SetPeerIdentitiesByDigest(false);
  TestHandshake(false);
};

// Test moving a bunch of data

// Basic tests: DTLS
// Test that we can make a handshake work
TEST_F(SSLStreamAdapterTestDTLS, TestDTLSConnect) {
  MAYBE_SKIP_TEST(HaveDtls);
  TestHandshake();
};

// Test that we can make a handshake work if the first packet in
// each direction is lost. This gives us predictable loss
// rather than having to tune random
TEST_F(SSLStreamAdapterTestDTLS, TestDTLSConnectWithLostFirstPacket) {
  MAYBE_SKIP_TEST(HaveDtls);
  SetLoseFirstPacket(true);
  TestHandshake();
};

// Test a handshake with loss and delay
TEST_F(SSLStreamAdapterTestDTLS,
       TestDTLSConnectWithLostFirstPacketDelay2s) {
  MAYBE_SKIP_TEST(HaveDtls);
  SetLoseFirstPacket(true);
  SetDelay(2000);
  SetHandshakeWait(20000);
  TestHandshake();
};

// Test a handshake with small MTU
TEST_F(SSLStreamAdapterTestDTLS, TestDTLSConnectWithSmallMtu) {
  MAYBE_SKIP_TEST(HaveDtls);
  SetMtu(700);
  SetHandshakeWait(20000);
  TestHandshake();
};

// Test transfer -- trivial
TEST_F(SSLStreamAdapterTestDTLS, TestDTLSTransfer) {
  MAYBE_SKIP_TEST(HaveDtls);
  TestHandshake();
  TestTransfer(100);
};

TEST_F(SSLStreamAdapterTestDTLS, TestDTLSTransferWithLoss) {
  MAYBE_SKIP_TEST(HaveDtls);
  TestHandshake();
  SetLoss(10);
  TestTransfer(100);
};

TEST_F(SSLStreamAdapterTestDTLS, TestDTLSTransferWithDamage) {
  MAYBE_SKIP_TEST(HaveDtls);
  SetDamage();  // Must be called first because first packet
                // write happens at end of handshake.
  TestHandshake();
  TestTransfer(100);
};

// Test DTLS-SRTP with all high ciphers
TEST_F(SSLStreamAdapterTestDTLS, TestDTLSSrtpHigh) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  std::vector<std::string> high;
  high.push_back(kAES_CM_HMAC_SHA1_80);
  SetDtlsSrtpCiphers(high, true);
  SetDtlsSrtpCiphers(high, false);
  TestHandshake();

  std::string client_cipher;
  ASSERT_TRUE(GetDtlsSrtpCipher(true, &client_cipher));
  std::string server_cipher;
  ASSERT_TRUE(GetDtlsSrtpCipher(false, &server_cipher));

  ASSERT_EQ(client_cipher, server_cipher);
  ASSERT_EQ(client_cipher, kAES_CM_HMAC_SHA1_80);
};

// Test DTLS-SRTP with all low ciphers
TEST_F(SSLStreamAdapterTestDTLS, TestDTLSSrtpLow) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  std::vector<std::string> low;
  low.push_back(kAES_CM_HMAC_SHA1_32);
  SetDtlsSrtpCiphers(low, true);
  SetDtlsSrtpCiphers(low, false);
  TestHandshake();

  std::string client_cipher;
  ASSERT_TRUE(GetDtlsSrtpCipher(true, &client_cipher));
  std::string server_cipher;
  ASSERT_TRUE(GetDtlsSrtpCipher(false, &server_cipher));

  ASSERT_EQ(client_cipher, server_cipher);
  ASSERT_EQ(client_cipher, kAES_CM_HMAC_SHA1_32);
};


// Test DTLS-SRTP with a mismatch -- should not converge
TEST_F(SSLStreamAdapterTestDTLS, TestDTLSSrtpHighLow) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  std::vector<std::string> high;
  high.push_back(kAES_CM_HMAC_SHA1_80);
  std::vector<std::string> low;
  low.push_back(kAES_CM_HMAC_SHA1_32);
  SetDtlsSrtpCiphers(high, true);
  SetDtlsSrtpCiphers(low, false);
  TestHandshake();

  std::string client_cipher;
  ASSERT_FALSE(GetDtlsSrtpCipher(true, &client_cipher));
  std::string server_cipher;
  ASSERT_FALSE(GetDtlsSrtpCipher(false, &server_cipher));
};

// Test DTLS-SRTP with each side being mixed -- should select high
TEST_F(SSLStreamAdapterTestDTLS, TestDTLSSrtpMixed) {
  MAYBE_SKIP_TEST(HaveDtlsSrtp);
  std::vector<std::string> mixed;
  mixed.push_back(kAES_CM_HMAC_SHA1_80);
  mixed.push_back(kAES_CM_HMAC_SHA1_32);
  SetDtlsSrtpCiphers(mixed, true);
  SetDtlsSrtpCiphers(mixed, false);
  TestHandshake();

  std::string client_cipher;
  ASSERT_TRUE(GetDtlsSrtpCipher(true, &client_cipher));
  std::string server_cipher;
  ASSERT_TRUE(GetDtlsSrtpCipher(false, &server_cipher));

  ASSERT_EQ(client_cipher, server_cipher);
  ASSERT_EQ(client_cipher, kAES_CM_HMAC_SHA1_80);
};

// Test an exporter
TEST_F(SSLStreamAdapterTestDTLS, TestDTLSExporter) {
  MAYBE_SKIP_TEST(HaveExporter);
  TestHandshake();
  unsigned char client_out[20];
  unsigned char server_out[20];

  bool result;
  result = ExportKeyingMaterial(kExporterLabel,
                                kExporterContext, kExporterContextLen,
                                true, true,
                                client_out, sizeof(client_out));
  ASSERT_TRUE(result);

  result = ExportKeyingMaterial(kExporterLabel,
                                kExporterContext, kExporterContextLen,
                                true, false,
                                server_out, sizeof(server_out));
  ASSERT_TRUE(result);

  ASSERT_TRUE(!memcmp(client_out, server_out, sizeof(client_out)));
}

// Test not yet valid certificates are not rejected.
TEST_F(SSLStreamAdapterTestDTLS, TestCertNotYetValid) {
  MAYBE_SKIP_TEST(HaveDtls);
  long one_day = 60 * 60 * 24;
  // Make the certificates not valid until one day later.
  ResetIdentitiesWithValidity(one_day, one_day);
  TestHandshake();
}

// Test expired certificates are not rejected.
TEST_F(SSLStreamAdapterTestDTLS, TestCertExpired) {
  MAYBE_SKIP_TEST(HaveDtls);
  long one_day = 60 * 60 * 24;
  // Make the certificates already expired.
  ResetIdentitiesWithValidity(-one_day, -one_day);
  TestHandshake();
}

// Test data transfer using certs created from strings.
TEST_F(SSLStreamAdapterTestDTLSFromPEMStrings, TestTransfer) {
  MAYBE_SKIP_TEST(HaveDtls);
  TestHandshake();
  TestTransfer(100);
}

// Test getting the remote certificate.
TEST_F(SSLStreamAdapterTestDTLSFromPEMStrings, TestDTLSGetPeerCertificate) {
  MAYBE_SKIP_TEST(HaveDtls);

  // Peer certificates haven't been received yet.
  talk_base::scoped_ptr<talk_base::SSLCertificate> client_peer_cert;
  ASSERT_FALSE(GetPeerCertificate(true, client_peer_cert.accept()));
  ASSERT_FALSE(client_peer_cert != NULL);

  talk_base::scoped_ptr<talk_base::SSLCertificate> server_peer_cert;
  ASSERT_FALSE(GetPeerCertificate(false, server_peer_cert.accept()));
  ASSERT_FALSE(server_peer_cert != NULL);

  TestHandshake();

  // The client should have a peer certificate after the handshake.
  ASSERT_TRUE(GetPeerCertificate(true, client_peer_cert.accept()));
  ASSERT_TRUE(client_peer_cert != NULL);

  // It's not kCERT_PEM.
  std::string client_peer_string = client_peer_cert->ToPEMString();
  ASSERT_NE(kCERT_PEM, client_peer_string);

  // It must not have a chain, because the test certs are self-signed.
  talk_base::SSLCertChain* client_peer_chain;
  ASSERT_FALSE(client_peer_cert->GetChain(&client_peer_chain));

  // The server should have a peer certificate after the handshake.
  ASSERT_TRUE(GetPeerCertificate(false, server_peer_cert.accept()));
  ASSERT_TRUE(server_peer_cert != NULL);

  // It's kCERT_PEM
  ASSERT_EQ(kCERT_PEM, server_peer_cert->ToPEMString());

  // It must not have a chain, because the test certs are self-signed.
  talk_base::SSLCertChain* server_peer_chain;
  ASSERT_FALSE(server_peer_cert->GetChain(&server_peer_chain));
}
