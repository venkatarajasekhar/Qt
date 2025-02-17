// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BROWSER_URL_HANDLER_IMPL_H_
#define CONTENT_BROWSER_BROWSER_URL_HANDLER_IMPL_H_

#include <vector>
#include <utility>

#include "base/gtest_prod_util.h"
#include "base/memory/singleton.h"
#include "content/public/browser/browser_url_handler.h"

class GURL;

namespace content {
class BrowserContext;

class CONTENT_EXPORT BrowserURLHandlerImpl : public BrowserURLHandler {
 public:
  // Returns the singleton instance.
  static BrowserURLHandlerImpl* GetInstance();

  // BrowserURLHandler implementation:
  virtual void RewriteURLIfNecessary(GURL* url,
                                     BrowserContext* browser_context,
                                     bool* reverse_on_redirect) OVERRIDE;
  // Add the specified handler pair to the list of URL handlers.
  virtual void AddHandlerPair(URLHandler handler,
                              URLHandler reverse_handler) OVERRIDE;

  // Reverses the rewriting that was done for |original| using the new |url|.
  bool ReverseURLRewrite(GURL* url, const GURL& original,
                         BrowserContext* browser_context);

 private:
  // This object is a singleton:
  BrowserURLHandlerImpl();
  virtual ~BrowserURLHandlerImpl();
  friend struct DefaultSingletonTraits<BrowserURLHandlerImpl>;

  // The list of known URLHandlers, optionally with reverse-rewriters.
  typedef std::pair<URLHandler, URLHandler> HandlerPair;
  std::vector<HandlerPair> url_handlers_;

  FRIEND_TEST_ALL_PREFIXES(BrowserURLHandlerImplTest, BasicRewriteAndReverse);
  FRIEND_TEST_ALL_PREFIXES(BrowserURLHandlerImplTest, NullHandlerReverse);

  DISALLOW_COPY_AND_ASSIGN(BrowserURLHandlerImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BROWSER_URL_HANDLER_IMPL_H_
