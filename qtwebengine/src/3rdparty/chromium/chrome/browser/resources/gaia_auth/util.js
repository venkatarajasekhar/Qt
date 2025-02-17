// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Alias for document.getElementById.
 * @param {string} id The ID of the element to find.
 * @return {HTMLElement} The found element or null if not found.
 */
function $(id) {
  return document.getElementById(id);
}

/**
 * Extract query params from given search string of an URL.
 * @param {string} search The search portion of an URL to extract params.
 * @return {Object} The key value pairs of the extracted params.
 */
function getUrlSearchParams(search) {
  var params = {};

  if (search) {
    // Strips leading '?'
    search = search.substring(1);
    var pairs = search.split('&');
    for (var i = 0; i < pairs.length; ++i) {
      var pair = pairs[i].split('=');
      if (pair.length == 2) {
        params[pair[0]] = decodeURIComponent(pair[1]);
      } else {
        params[pair] = true;
      }
    }
  }

  return params;
}

/**
 * Creates a new URL which is the old URL with a GET param of key=value.
 * Copied from ui/webui/resources/js/util.js.
 * @param {string} url The base URL. There is not sanity checking on the URL so
 *     it must be passed in a proper format.
 * @param {string} key The key of the param.
 * @param {string} value The value of the param.
 * @return {string} The new URL.
 */
function appendParam(url, key, value) {
  var param = encodeURIComponent(key) + '=' + encodeURIComponent(value);

  if (url.indexOf('?') == -1)
    return url + '?' + param;
  return url + '&' + param;
}

/**
 * Creates a new URL by striping all query parameters.
 * @param {string} url The original URL.
 * @return {string} The new URL with all query parameters stripped.
 */
function stripParams(url) {
  return url.substring(0, url.indexOf('?')) || url;
}

/**
 * Extract domain name from an URL.
 * @param {string} url An URL string.
 * @return {string} The host name of the URL.
 */
function extractDomain(url) {
  var a = document.createElement('a');
  a.href = url;
  return a.hostname;
}
