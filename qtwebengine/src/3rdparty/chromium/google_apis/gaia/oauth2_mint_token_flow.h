// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GAIA_OAUTH2_MINT_TOKEN_FLOW_H_
#define GOOGLE_APIS_GAIA_OAUTH2_MINT_TOKEN_FLOW_H_

#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "google_apis/gaia/oauth2_api_call_flow.h"
#include "url/gurl.h"

class GoogleServiceAuthError;
class OAuth2MintTokenFlowTest;

namespace base {
class DictionaryValue;
}

namespace content {
class URLFetcher;
}

namespace net {
class URLRequestContextGetter;
}

// IssueAdvice: messages to show to the user to get a user's approval.
// The structure is as follows:
// * Description 1
//   - Detail 1.1
//   - Details 1.2
// * Description 2
//   - Detail 2.1
//   - Detail 2.2
//   - Detail 2.3
// * Description 3
//   - Detail 3.1
struct IssueAdviceInfoEntry {
 public:
  IssueAdviceInfoEntry();
  ~IssueAdviceInfoEntry();

  base::string16 description;
  std::vector<base::string16> details;

  bool operator==(const IssueAdviceInfoEntry& rhs) const;
};

typedef std::vector<IssueAdviceInfoEntry> IssueAdviceInfo;

// This class implements the OAuth2 flow to Google to mint an OAuth2
// token for the given client and the given set of scopes from the
// OAuthLogin scoped "master" OAuth2 token for the user logged in to
// Chrome.
class OAuth2MintTokenFlow : public OAuth2ApiCallFlow {
 public:
  // There are four differnt modes when minting a token to grant
  // access to third-party app for a user.
  enum Mode {
    // Get the messages to display to the user without minting a token.
    MODE_ISSUE_ADVICE,
    // Record a grant but do not get a token back.
    MODE_RECORD_GRANT,
    // Mint a token for an existing grant.
    MODE_MINT_TOKEN_NO_FORCE,
    // Mint a token forcefully even if there is no existing grant.
    MODE_MINT_TOKEN_FORCE,
  };

  // Parameters needed to mint a token.
  struct Parameters {
   public:
    Parameters();
    Parameters(const std::string& at,
               const std::string& eid,
               const std::string& cid,
               const std::vector<std::string>& scopes_arg,
               Mode mode_arg);
    ~Parameters();

    std::string access_token;
    std::string extension_id;
    std::string client_id;
    std::vector<std::string> scopes;
    Mode mode;
  };

  class Delegate {
   public:
    virtual void OnMintTokenSuccess(const std::string& access_token,
                                    int time_to_live) {}
    virtual void OnIssueAdviceSuccess(const IssueAdviceInfo& issue_advice)  {}
    virtual void OnMintTokenFailure(const GoogleServiceAuthError& error) {}

   protected:
    virtual ~Delegate() {}
  };

  OAuth2MintTokenFlow(net::URLRequestContextGetter* context,
                      Delegate* delegate,
                      const Parameters& parameters);
  virtual ~OAuth2MintTokenFlow();

 protected:
  // Implementation of template methods in OAuth2ApiCallFlow.
  virtual GURL CreateApiCallUrl() OVERRIDE;
  virtual std::string CreateApiCallBody() OVERRIDE;

  virtual void ProcessApiCallSuccess(
      const net::URLFetcher* source) OVERRIDE;
  virtual void ProcessApiCallFailure(
      const net::URLFetcher* source) OVERRIDE;
  virtual void ProcessNewAccessToken(const std::string& access_token) OVERRIDE;
  virtual void ProcessMintAccessTokenFailure(
      const GoogleServiceAuthError& error) OVERRIDE;

 private:
  friend class OAuth2MintTokenFlowTest;
  FRIEND_TEST_ALL_PREFIXES(OAuth2MintTokenFlowTest, CreateApiCallBody);
  FRIEND_TEST_ALL_PREFIXES(OAuth2MintTokenFlowTest, ParseIssueAdviceResponse);
  FRIEND_TEST_ALL_PREFIXES(OAuth2MintTokenFlowTest, ParseMintTokenResponse);
  FRIEND_TEST_ALL_PREFIXES(OAuth2MintTokenFlowTest, ProcessApiCallSuccess);
  FRIEND_TEST_ALL_PREFIXES(OAuth2MintTokenFlowTest, ProcessApiCallFailure);
  FRIEND_TEST_ALL_PREFIXES(OAuth2MintTokenFlowTest,
      ProcessMintAccessTokenFailure);

  void ReportSuccess(const std::string& access_token, int time_to_live);
  void ReportIssueAdviceSuccess(const IssueAdviceInfo& issue_advice);
  void ReportFailure(const GoogleServiceAuthError& error);

  static bool ParseIssueAdviceResponse(
      const base::DictionaryValue* dict, IssueAdviceInfo* issue_advice);
  static bool ParseMintTokenResponse(
      const base::DictionaryValue* dict, std::string* access_token,
      int* time_to_live);

  Delegate* delegate_;
  Parameters parameters_;
  base::WeakPtrFactory<OAuth2MintTokenFlow> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(OAuth2MintTokenFlow);
};

#endif  // GOOGLE_APIS_GAIA_OAUTH2_MINT_TOKEN_FLOW_H_
