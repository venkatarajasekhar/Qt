// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef InternalsNavigatorContentUtils_h
#define InternalsNavigatorContentUtils_h

namespace WebCore {

class Document;
class NavigatorContentUtilsClientMock;
class Internals;

class InternalsNavigatorContentUtils {
public:
    static void setNavigatorContentUtilsClientMock(Internals&, Document*);
};

} // namespace WebCore

#endif // InternalsNavigatorContentUtils_h
