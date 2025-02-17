// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_NAVIGATION_ENTRY_H_
#define CONTENT_PUBLIC_BROWSER_NAVIGATION_ENTRY_H_

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/public/common/page_transition_types.h"
#include "content/public/common/page_type.h"
#include "content/public/common/referrer.h"

class GURL;

namespace content {

class PageState;
struct FaviconStatus;
struct SSLStatus;

// A NavigationEntry is a data structure that captures all the information
// required to recreate a browsing state. This includes some opaque binary
// state as provided by the WebContents as well as some clear text title and
// URL which is used for our user interface.
class NavigationEntry {
 public:
  virtual ~NavigationEntry() {}

  CONTENT_EXPORT static NavigationEntry* Create();
  CONTENT_EXPORT static NavigationEntry* Create(const NavigationEntry& copy);

  // Page-related stuff --------------------------------------------------------

  // A unique ID is preserved across commits and redirects, which means that
  // sometimes a NavigationEntry's unique ID needs to be set (e.g. when
  // creating a committed entry to correspond to a to-be-deleted pending entry,
  // the pending entry's ID must be copied).
  virtual int GetUniqueID() const = 0;

  // The page type tells us if this entry is for an interstitial or error page.
  virtual content::PageType GetPageType() const = 0;

  // The actual URL of the page. For some about pages, this may be a scary
  // data: URL or something like that. Use GetVirtualURL() below for showing to
  // the user.
  virtual void SetURL(const GURL& url) = 0;
  virtual const GURL& GetURL() const = 0;

  // Used for specifying a base URL for pages loaded via data URLs.
  virtual void SetBaseURLForDataURL(const GURL& url) = 0;
  virtual const GURL& GetBaseURLForDataURL() const = 0;

  // The referring URL. Can be empty.
  virtual void SetReferrer(const content::Referrer& referrer) = 0;
  virtual const content::Referrer& GetReferrer() const = 0;

  // The virtual URL, when nonempty, will override the actual URL of the page
  // when we display it to the user. This allows us to have nice and friendly
  // URLs that the user sees for things like about: URLs, but actually feed
  // the renderer a data URL that results in the content loading.
  //
  // GetVirtualURL() will return the URL to display to the user in all cases, so
  // if there is no overridden display URL, it will return the actual one.
  virtual void SetVirtualURL(const GURL& url) = 0;
  virtual const GURL& GetVirtualURL() const = 0;

  // The title as set by the page. This will be empty if there is no title set.
  // The caller is responsible for detecting when there is no title and
  // displaying the appropriate "Untitled" label if this is being displayed to
  // the user.
  virtual void SetTitle(const base::string16& title) = 0;
  virtual const base::string16& GetTitle() const = 0;

  // Page state is an opaque blob created by Blink that represents the state of
  // the page. This includes form entries and scroll position for each frame.
  // We store it so that we can supply it back to Blink to restore form state
  // properly when the user goes back and forward.
  //
  // NOTE: This state is saved to disk and used to restore previous states.  If
  // the format is modified in the future, we should still be able to deal with
  // older versions.
  virtual void SetPageState(const PageState& state) = 0;
  virtual const PageState& GetPageState() const = 0;

  // Describes the current page that the tab represents. This is the ID that the
  // renderer generated for the page and is how we can tell new versus
  // renavigations.
  virtual void SetPageID(int page_id) = 0;
  virtual int32 GetPageID() const = 0;

  // Page-related helpers ------------------------------------------------------

  // Returns the title to be displayed on the tab. This could be the title of
  // the page if it is available or the URL. |languages| is the list of
  // accpeted languages (e.g., prefs::kAcceptLanguages) or empty if proper
  // URL formatting isn't needed (e.g., unit tests).
  virtual const base::string16& GetTitleForDisplay(
      const std::string& languages) const = 0;

  // Returns true if the current tab is in view source mode. This will be false
  // if there is no navigation.
  virtual bool IsViewSourceMode() const = 0;

  // Tracking stuff ------------------------------------------------------------

  // The transition type indicates what the user did to move to this page from
  // the previous page.
  virtual void SetTransitionType(content::PageTransition transition_type) = 0;
  virtual content::PageTransition GetTransitionType() const = 0;

  // The user typed URL was the URL that the user initiated the navigation
  // with, regardless of any redirects. This is used to generate keywords, for
  // example, based on "what the user thinks the site is called" rather than
  // what it's actually called. For example, if the user types "foo.com", that
  // may redirect somewhere arbitrary like "bar.com/foo", and we want to use
  // the name that the user things of the site as having.
  //
  // This URL will be is_empty() if the URL was navigated to some other way.
  // Callers should fall back on using the regular or display URL in this case.
  virtual const GURL& GetUserTypedURL() const = 0;

  // Post data is form data that was posted to get to this page. The data will
  // have to be reposted to reload the page properly. This flag indicates
  // whether the page had post data.
  //
  // The actual post data is stored either in
  // 1) browser_initiated_post_data when a new post data request is started.
  // 2) content_state when a post request has started and is extracted by
  //    WebKit to actually make the request.
  virtual void SetHasPostData(bool has_post_data) = 0;
  virtual bool GetHasPostData() const = 0;

  // The Post identifier associated with the page.
  virtual void SetPostID(int64 post_id) = 0;
  virtual int64 GetPostID() const = 0;

  // Holds the raw post data of a browser initiated post request.
  // For efficiency, this should be cleared when content_state is populated
  // since the data is duplicated.
  // Note, this field:
  // 1) is not persisted in session restore.
  // 2) is shallow copied with the static copy Create method above.
  // 3) may be NULL so check before use.
  virtual void SetBrowserInitiatedPostData(
      const base::RefCountedMemory* data) = 0;
  virtual const base::RefCountedMemory* GetBrowserInitiatedPostData() const = 0;

  // The favicon data and tracking information. See content::FaviconStatus.
  virtual const FaviconStatus& GetFavicon() const = 0;
  virtual FaviconStatus& GetFavicon() = 0;

  // All the SSL flags and state. See content::SSLStatus.
  virtual const SSLStatus& GetSSL() const = 0;
  virtual SSLStatus& GetSSL() = 0;

  // Store the URL that caused this NavigationEntry to be created.
  virtual void SetOriginalRequestURL(const GURL& original_url) = 0;
  virtual const GURL& GetOriginalRequestURL() const = 0;

  // Store whether or not we're overriding the user agent.
  virtual void SetIsOverridingUserAgent(bool override) = 0;
  virtual bool GetIsOverridingUserAgent() const = 0;

  // The time at which the last known local navigation has
  // completed. (A navigation can be completed more than once if the
  // page is reloaded.)
  //
  // If GetTimestamp() returns a null time, that means that either:
  //
  //   - this navigation hasn't completed yet;
  //   - this navigation was restored and for some reason the
  //     timestamp wasn't available;
  //   - or this navigation was copied from a foreign session.
  virtual void SetTimestamp(base::Time timestamp) = 0;
  virtual base::Time GetTimestamp() const = 0;

  // Used to specify if this entry should be able to access local file://
  // resources.
  virtual void SetCanLoadLocalResources(bool allow) = 0;
  virtual bool GetCanLoadLocalResources() const = 0;

  // Used to specify which frame to navigate. If empty, the main frame is
  // navigated. This is currently not persisted in session restore, because it
  // is currently only used in tests.
  virtual void SetFrameToNavigate(const std::string& frame_name) = 0;
  virtual const std::string& GetFrameToNavigate() const = 0;

  // Set extra data on this NavigationEntry according to the specified |key|.
  // This data is not persisted by default.
  virtual void SetExtraData(const std::string& key,
                            const base::string16& data) = 0;
  // If present, fills the |data| present at the specified |key|.
  virtual bool GetExtraData(const std::string& key,
                            base::string16* data) const = 0;
  // Removes the data at the specified |key|.
  virtual void ClearExtraData(const std::string& key) = 0;

  // The status code of the last known successful navigation.  If
  // GetHttpStatusCode() returns 0 that means that either:
  //
  //   - this navigation hasn't completed yet;
  //   - a response wasn't received;
  //   - or this navigation was restored and for some reason the
  //     status code wasn't available.
  virtual void SetHttpStatusCode(int http_status_code) = 0;
  virtual int GetHttpStatusCode() const = 0;

  // The redirect chain traversed during this navigation, from the initial
  // redirecting URL to the final non-redirecting current URL.
  virtual void SetRedirectChain(const std::vector<GURL>& redirects) = 0;
  virtual const std::vector<GURL>& GetRedirectChain() const = 0;

  // True if this entry is restored and hasn't been loaded.
  virtual bool IsRestored() const = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_NAVIGATION_ENTRY_H_
