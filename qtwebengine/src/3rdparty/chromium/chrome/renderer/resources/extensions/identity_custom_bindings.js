// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the Identity API.

var binding = require('binding').Binding.create('identity');

binding.registerCustomHook(function(binding, id, contextType) {
  var apiFunctions = binding.apiFunctions;
  var identity = binding.compiledApi;

  apiFunctions.setHandleRequest('getRedirectURL', function(path) {
    if (path === null || path === undefined)
      path = '/';
    else
      path = String(path);
    if (path[0] != '/')
      path = '/' + path;
    return 'https://' + id + '.chromiumapp.org' + path;
  });
});

exports.binding = binding.generate();
