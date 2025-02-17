// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/ssl/ssl_host_state.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Certificates for test data. They're obtained with:
//
// $ openssl s_client -connect [host]:443 -showcerts
// $ openssl x509 -inform PEM -outform DER > /tmp/host.der
// $ xxd -i /tmp/host.der

// Google's cert.

unsigned char google_der[] = {
  0x30, 0x82, 0x03, 0x21, 0x30, 0x82, 0x02, 0x8a, 0xa0, 0x03, 0x02, 0x01,
  0x02, 0x02, 0x10, 0x3c, 0x8d, 0x3a, 0x64, 0xee, 0x18, 0xdd, 0x1b, 0x73,
  0x0b, 0xa1, 0x92, 0xee, 0xf8, 0x98, 0x1b, 0x30, 0x0d, 0x06, 0x09, 0x2a,
  0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00, 0x30, 0x4c,
  0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x5a,
  0x41, 0x31, 0x25, 0x30, 0x23, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x1c,
  0x54, 0x68, 0x61, 0x77, 0x74, 0x65, 0x20, 0x43, 0x6f, 0x6e, 0x73, 0x75,
  0x6c, 0x74, 0x69, 0x6e, 0x67, 0x20, 0x28, 0x50, 0x74, 0x79, 0x29, 0x20,
  0x4c, 0x74, 0x64, 0x2e, 0x31, 0x16, 0x30, 0x14, 0x06, 0x03, 0x55, 0x04,
  0x03, 0x13, 0x0d, 0x54, 0x68, 0x61, 0x77, 0x74, 0x65, 0x20, 0x53, 0x47,
  0x43, 0x20, 0x43, 0x41, 0x30, 0x1e, 0x17, 0x0d, 0x30, 0x38, 0x30, 0x35,
  0x30, 0x32, 0x31, 0x37, 0x30, 0x32, 0x35, 0x35, 0x5a, 0x17, 0x0d, 0x30,
  0x39, 0x30, 0x35, 0x30, 0x32, 0x31, 0x37, 0x30, 0x32, 0x35, 0x35, 0x5a,
  0x30, 0x68, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13,
  0x02, 0x55, 0x53, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x08,
  0x13, 0x0a, 0x43, 0x61, 0x6c, 0x69, 0x66, 0x6f, 0x72, 0x6e, 0x69, 0x61,
  0x31, 0x16, 0x30, 0x14, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13, 0x0d, 0x4d,
  0x6f, 0x75, 0x6e, 0x74, 0x61, 0x69, 0x6e, 0x20, 0x56, 0x69, 0x65, 0x77,
  0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x0a, 0x47,
  0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x20, 0x49, 0x6e, 0x63, 0x31, 0x17, 0x30,
  0x15, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0e, 0x77, 0x77, 0x77, 0x2e,
  0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x81,
  0x9f, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01,
  0x01, 0x01, 0x05, 0x00, 0x03, 0x81, 0x8d, 0x00, 0x30, 0x81, 0x89, 0x02,
  0x81, 0x81, 0x00, 0x9b, 0x19, 0xed, 0x5d, 0xa5, 0x56, 0xaf, 0x49, 0x66,
  0xdb, 0x79, 0xfd, 0xc2, 0x1c, 0x78, 0x4e, 0x4f, 0x11, 0xa5, 0x8a, 0xac,
  0xe2, 0x94, 0xee, 0xe3, 0xe2, 0x4b, 0xc0, 0x03, 0x25, 0xa7, 0x99, 0xcc,
  0x65, 0xe1, 0xec, 0x94, 0xae, 0xae, 0xf0, 0xa7, 0x99, 0xbc, 0x10, 0xd7,
  0xed, 0x87, 0x30, 0x47, 0xcd, 0x50, 0xf9, 0xaf, 0xd3, 0xd3, 0xf4, 0x0b,
  0x8d, 0x47, 0x8a, 0x2e, 0xe2, 0xce, 0x53, 0x9b, 0x91, 0x99, 0x7f, 0x1e,
  0x5c, 0xf9, 0x1b, 0xd6, 0xe9, 0x93, 0x67, 0xe3, 0x4a, 0xf8, 0xcf, 0xc4,
  0x8c, 0x0c, 0x68, 0xd1, 0x97, 0x54, 0x47, 0x0e, 0x0a, 0x24, 0x30, 0xa7,
  0x82, 0x94, 0xae, 0xde, 0xae, 0x3f, 0xbf, 0xba, 0x14, 0xc6, 0xf8, 0xb2,
  0x90, 0x8e, 0x36, 0xad, 0xe1, 0xd0, 0xbe, 0x16, 0x9a, 0xb3, 0x5e, 0x72,
  0x38, 0x49, 0xda, 0x74, 0xa1, 0x3f, 0xff, 0xd2, 0x87, 0x81, 0xed, 0x02,
  0x03, 0x01, 0x00, 0x01, 0xa3, 0x81, 0xe7, 0x30, 0x81, 0xe4, 0x30, 0x28,
  0x06, 0x03, 0x55, 0x1d, 0x25, 0x04, 0x21, 0x30, 0x1f, 0x06, 0x08, 0x2b,
  0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01, 0x06, 0x08, 0x2b, 0x06, 0x01,
  0x05, 0x05, 0x07, 0x03, 0x02, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x86,
  0xf8, 0x42, 0x04, 0x01, 0x30, 0x36, 0x06, 0x03, 0x55, 0x1d, 0x1f, 0x04,
  0x2f, 0x30, 0x2d, 0x30, 0x2b, 0xa0, 0x29, 0xa0, 0x27, 0x86, 0x25, 0x68,
  0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x63, 0x72, 0x6c, 0x2e, 0x74, 0x68,
  0x61, 0x77, 0x74, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x54, 0x68, 0x61,
  0x77, 0x74, 0x65, 0x53, 0x47, 0x43, 0x43, 0x41, 0x2e, 0x63, 0x72, 0x6c,
  0x30, 0x72, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01,
  0x04, 0x66, 0x30, 0x64, 0x30, 0x22, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05,
  0x05, 0x07, 0x30, 0x01, 0x86, 0x16, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
  0x2f, 0x6f, 0x63, 0x73, 0x70, 0x2e, 0x74, 0x68, 0x61, 0x77, 0x74, 0x65,
  0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x3e, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05,
  0x05, 0x07, 0x30, 0x02, 0x86, 0x32, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
  0x2f, 0x77, 0x77, 0x77, 0x2e, 0x74, 0x68, 0x61, 0x77, 0x74, 0x65, 0x2e,
  0x63, 0x6f, 0x6d, 0x2f, 0x72, 0x65, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x6f,
  0x72, 0x79, 0x2f, 0x54, 0x68, 0x61, 0x77, 0x74, 0x65, 0x5f, 0x53, 0x47,
  0x43, 0x5f, 0x43, 0x41, 0x2e, 0x63, 0x72, 0x74, 0x30, 0x0c, 0x06, 0x03,
  0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x02, 0x30, 0x00, 0x30, 0x0d,
  0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05,
  0x00, 0x03, 0x81, 0x81, 0x00, 0x31, 0x0a, 0x6c, 0xa2, 0x9e, 0xe9, 0x54,
  0x19, 0x16, 0x68, 0x99, 0x91, 0xd6, 0x43, 0xcb, 0x6b, 0xb4, 0xcc, 0x6c,
  0xcc, 0xb0, 0xfb, 0xf1, 0xee, 0x81, 0xbf, 0x00, 0x2b, 0x6f, 0x50, 0x12,
  0xc6, 0xaf, 0x02, 0x2a, 0x36, 0xc1, 0x28, 0xde, 0xc5, 0x4c, 0x56, 0x20,
  0x6d, 0xf5, 0x3d, 0x42, 0xb9, 0x18, 0x81, 0x20, 0xb2, 0xdd, 0x57, 0x5d,
  0xeb, 0xbe, 0x32, 0x84, 0x50, 0x45, 0x51, 0x6e, 0xcd, 0xe4, 0x2e, 0x2a,
  0x38, 0x88, 0x9f, 0x52, 0xed, 0x28, 0xff, 0xfc, 0x8d, 0x57, 0xb5, 0xad,
  0x64, 0xae, 0x4d, 0x0e, 0x0e, 0xd9, 0x3d, 0xac, 0xb8, 0xfe, 0x66, 0x4c,
  0x15, 0x8f, 0x44, 0x52, 0xfa, 0x7c, 0x3c, 0x04, 0xed, 0x7f, 0x37, 0x61,
  0x04, 0xfe, 0xd5, 0xe9, 0xb9, 0xb0, 0x9e, 0xfe, 0xa5, 0x11, 0x69, 0xc9,
  0x63, 0xd6, 0x46, 0x81, 0x6f, 0x00, 0xd8, 0x72, 0x2f, 0x82, 0x37, 0x44,
  0xc1
};

}  // namespace

namespace content {

class SSLHostStateTest : public testing::Test {
};

TEST_F(SSLHostStateTest, DidHostRunInsecureContent) {
  SSLHostState state;

  EXPECT_FALSE(state.DidHostRunInsecureContent("www.google.com", 42));
  EXPECT_FALSE(state.DidHostRunInsecureContent("www.google.com", 191));
  EXPECT_FALSE(state.DidHostRunInsecureContent("example.com", 42));

  state.HostRanInsecureContent("www.google.com", 42);

  EXPECT_TRUE(state.DidHostRunInsecureContent("www.google.com", 42));
  EXPECT_FALSE(state.DidHostRunInsecureContent("www.google.com", 191));
  EXPECT_FALSE(state.DidHostRunInsecureContent("example.com", 42));

  state.HostRanInsecureContent("example.com", 42);

  EXPECT_TRUE(state.DidHostRunInsecureContent("www.google.com", 42));
  EXPECT_FALSE(state.DidHostRunInsecureContent("www.google.com", 191));
  EXPECT_TRUE(state.DidHostRunInsecureContent("example.com", 42));
}

TEST_F(SSLHostStateTest, QueryPolicy) {
  scoped_refptr<net::X509Certificate> google_cert(
      net::X509Certificate::CreateFromBytes(
          reinterpret_cast<const char*>(google_der), sizeof(google_der)));

  SSLHostState state;

  EXPECT_EQ(net::CertPolicy::UNKNOWN,
            state.QueryPolicy(google_cert.get(),
                              "www.google.com",
                              net::CERT_STATUS_DATE_INVALID));
  EXPECT_EQ(net::CertPolicy::UNKNOWN,
            state.QueryPolicy(google_cert.get(),
                              "google.com",
                              net::CERT_STATUS_DATE_INVALID));
  EXPECT_EQ(net::CertPolicy::UNKNOWN,
            state.QueryPolicy(google_cert.get(),
                              "example.com",
                              net::CERT_STATUS_DATE_INVALID));

  state.AllowCertForHost(google_cert.get(),
                         "www.google.com",
                         net::CERT_STATUS_DATE_INVALID);

  EXPECT_EQ(net::CertPolicy::ALLOWED,
           state.QueryPolicy(google_cert.get(),
                             "www.google.com",
                             net::CERT_STATUS_DATE_INVALID));
  EXPECT_EQ(net::CertPolicy::UNKNOWN,
           state.QueryPolicy(google_cert.get(),
                             "google.com",
                             net::CERT_STATUS_DATE_INVALID));
  EXPECT_EQ(net::CertPolicy::UNKNOWN,
           state.QueryPolicy(google_cert.get(),
                             "example.com",
                             net::CERT_STATUS_DATE_INVALID));

  state.AllowCertForHost(google_cert.get(),
                         "example.com",
                         net::CERT_STATUS_DATE_INVALID);

  EXPECT_EQ(net::CertPolicy::ALLOWED,
          state.QueryPolicy(google_cert.get(),
                            "www.google.com",
                            net::CERT_STATUS_DATE_INVALID));
  EXPECT_EQ(net::CertPolicy::UNKNOWN,
          state.QueryPolicy(google_cert.get(),
                            "google.com",
                            net::CERT_STATUS_DATE_INVALID));
  EXPECT_EQ(net::CertPolicy::ALLOWED,
          state.QueryPolicy(google_cert.get(),
                            "example.com",
                            net::CERT_STATUS_DATE_INVALID));

  state.DenyCertForHost(google_cert.get(),
                        "example.com",
                        net::CERT_STATUS_DATE_INVALID);

  EXPECT_EQ(net::CertPolicy::ALLOWED,
          state.QueryPolicy(google_cert.get(),
                            "www.google.com",
                            net::CERT_STATUS_DATE_INVALID));
  EXPECT_EQ(net::CertPolicy::UNKNOWN,
          state.QueryPolicy(google_cert.get(),
                            "google.com",
                            net::CERT_STATUS_DATE_INVALID));
  EXPECT_EQ(net::CertPolicy::DENIED,
          state.QueryPolicy(google_cert.get(),
                            "example.com",
                            net::CERT_STATUS_DATE_INVALID));

  state.Clear();

  EXPECT_EQ(net::CertPolicy::UNKNOWN,
            state.QueryPolicy(google_cert.get(),
                              "www.google.com",
                              net::CERT_STATUS_DATE_INVALID));
  EXPECT_EQ(net::CertPolicy::UNKNOWN,
            state.QueryPolicy(google_cert.get(),
                              "google.com",
                              net::CERT_STATUS_DATE_INVALID));
  EXPECT_EQ(net::CertPolicy::UNKNOWN,
            state.QueryPolicy(google_cert.get(),
                              "example.com",
                              net::CERT_STATUS_DATE_INVALID));
}

}  // namespace content
