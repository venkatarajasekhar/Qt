// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/browser_url_handler_impl.h"

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "cc/base/switches.h"
#include "content/browser/frame_host/debug_urls.h"
#include "content/browser/webui/web_ui_impl.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

namespace content {

// Handles rewriting view-source URLs for what we'll actually load.
static bool HandleViewSource(GURL* url, BrowserContext* browser_context) {
  if (url->SchemeIs(kViewSourceScheme)) {
    // Load the inner URL instead.
    *url = GURL(url->GetContent());

    // Bug 26129: limit view-source to view the content and not any
    // other kind of 'active' url scheme like 'javascript' or 'data'.
    static const char* const default_allowed_sub_schemes[] = {
        url::kHttpScheme,
        url::kHttpsScheme,
        url::kFtpScheme,
        kChromeDevToolsScheme,
        kChromeUIScheme,
        url::kFileScheme,
        url::kFileSystemScheme
    };

    // Merge all the schemes for which view-source is allowed by default, with
    // the WebUI schemes defined by the ContentBrowserClient.
    std::vector<std::string> all_allowed_sub_schemes;
    for (size_t i = 0; i < arraysize(default_allowed_sub_schemes); ++i)
      all_allowed_sub_schemes.push_back(default_allowed_sub_schemes[i]);
    GetContentClient()->browser()->GetAdditionalWebUISchemes(
        &all_allowed_sub_schemes);

    bool is_sub_scheme_allowed = false;
    for (size_t i = 0; i < all_allowed_sub_schemes.size(); ++i) {
      if (url->SchemeIs(all_allowed_sub_schemes[i].c_str())) {
        is_sub_scheme_allowed = true;
        break;
      }
    }

    if (!is_sub_scheme_allowed) {
      *url = GURL(url::kAboutBlankURL);
      return false;
    }

    return true;
  }
  return false;
}

// Turns a non view-source URL into the corresponding view-source URL.
static bool ReverseViewSource(GURL* url, BrowserContext* browser_context) {
  // No action necessary if the URL is already view-source:
  if (url->SchemeIs(kViewSourceScheme))
    return false;

  url::Replacements<char> repl;
  repl.SetScheme(kViewSourceScheme,
                 url::Component(0, strlen(kViewSourceScheme)));
  repl.SetPath(url->spec().c_str(), url::Component(0, url->spec().size()));
  *url = url->ReplaceComponents(repl);
  return true;
}

static bool DebugURLHandler(GURL* url, BrowserContext* browser_context) {
  // If running inside the Telemetry test harness, allow automated
  // navigations to access browser-side debug URLs. They must use the
  // chrome:// scheme, since the about: scheme won't be rewritten in
  // this code path.
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          cc::switches::kEnableGpuBenchmarking)) {
    if (HandleDebugURL(*url, PAGE_TRANSITION_FROM_ADDRESS_BAR)) {
      return true;
    }
  }

  // Circumvent processing URLs that the renderer process will handle.
  return IsRendererDebugURL(*url);
}

// static
BrowserURLHandler* BrowserURLHandler::GetInstance() {
  return BrowserURLHandlerImpl::GetInstance();
}

// static
BrowserURLHandler::URLHandler BrowserURLHandler::null_handler() {
  // Required for VS2010: http://connect.microsoft.com/VisualStudio/feedback/details/520043/error-converting-from-null-to-a-pointer-type-in-std-pair
  return NULL;
}

// static
BrowserURLHandlerImpl* BrowserURLHandlerImpl::GetInstance() {
  return Singleton<BrowserURLHandlerImpl>::get();
}

BrowserURLHandlerImpl::BrowserURLHandlerImpl() {
  AddHandlerPair(&DebugURLHandler, BrowserURLHandlerImpl::null_handler());

  GetContentClient()->browser()->BrowserURLHandlerCreated(this);

  // view-source:
  AddHandlerPair(&HandleViewSource, &ReverseViewSource);
}

BrowserURLHandlerImpl::~BrowserURLHandlerImpl() {
}

void BrowserURLHandlerImpl::AddHandlerPair(URLHandler handler,
                                           URLHandler reverse_handler) {
  url_handlers_.push_back(HandlerPair(handler, reverse_handler));
}

void BrowserURLHandlerImpl::RewriteURLIfNecessary(
    GURL* url,
    BrowserContext* browser_context,
    bool* reverse_on_redirect) {
  for (size_t i = 0; i < url_handlers_.size(); ++i) {
    URLHandler handler = *url_handlers_[i].first;
    if (handler && handler(url, browser_context)) {
      *reverse_on_redirect = (url_handlers_[i].second != NULL);
      return;
    }
  }
}

bool BrowserURLHandlerImpl::ReverseURLRewrite(
    GURL* url, const GURL& original, BrowserContext* browser_context) {
  for (size_t i = 0; i < url_handlers_.size(); ++i) {
    URLHandler reverse_rewriter = *url_handlers_[i].second;
    if (reverse_rewriter) {
      GURL test_url(original);
      URLHandler handler = *url_handlers_[i].first;
      if (!handler) {
        if (reverse_rewriter(url, browser_context))
          return true;
      } else if (handler(&test_url, browser_context)) {
        return reverse_rewriter(url, browser_context);
      }
    }
  }
  return false;
}

}  // namespace content
