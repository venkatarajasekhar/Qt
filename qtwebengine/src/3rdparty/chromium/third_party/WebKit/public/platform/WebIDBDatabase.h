/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebIDBDatabase_h
#define WebIDBDatabase_h

#include "WebBlobInfo.h"
#include "WebCommon.h"
#include "WebIDBCursor.h"
#include "WebIDBMetadata.h"
#include "public/platform/WebIDBTypes.h"

namespace blink {

class WebData;
class WebIDBCallbacks;
class WebIDBDatabaseCallbacks;
class WebIDBDatabaseError;
class WebIDBKey;
class WebIDBKeyPath;
class WebIDBKeyRange;

// See comment in WebIDBFactory for a high level overview of these classes.
class WebIDBDatabase {
public:
    virtual ~WebIDBDatabase() { }

    // FIXME: Remove once Chromium is updated to use the new enums.
    typedef WebIDBTransactionMode TransactionMode;
    static const WebIDBTransactionMode TransactionReadOnly = WebIDBTransactionModeReadOnly;
    static const WebIDBTransactionMode TransactionReadWrite = WebIDBTransactionModeReadWrite;
    static const WebIDBTransactionMode TransactionVersionChange = WebIDBTransactionModeVersionChange;

    virtual void createObjectStore(long long transactionId, long long objectStoreId, const WebString& name, const WebIDBKeyPath&, bool autoIncrement) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void deleteObjectStore(long long transactionId, long long objectStoreId) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void createTransaction(long long id, WebIDBDatabaseCallbacks*, const WebVector<long long>& scope, blink::WebIDBTransactionMode) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void close() { BLINK_ASSERT_NOT_REACHED(); }
    virtual void forceClose() { BLINK_ASSERT_NOT_REACHED(); }

    virtual void abort(long long transactionId) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void commit(long long transactionId) { BLINK_ASSERT_NOT_REACHED(); }

    virtual void createIndex(long long transactionId, long long objectStoreId, long long indexId, const WebString& name, const WebIDBKeyPath&, bool unique, bool multiEntry) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void deleteIndex(long long transactionId, long long objectStoreId, long long indexId) { BLINK_ASSERT_NOT_REACHED(); }

    // FIXME: Remove once Chromium is updated to use the new enums.
    typedef WebIDBTaskType TaskType;
    static const WebIDBTaskType NormalTask = WebIDBTaskTypeNormal;
    static const WebIDBTaskType PreemptiveTask = WebIDBTaskTypePreemptive;
    static const WebIDBTaskType TaskTypeLast = WebIDBTaskTypeLast;

    // FIXME: Remove once Chromium is updated to use the new enums.
    typedef WebIDBPutMode PutMode;
    static const WebIDBPutMode AddOrUpdate = WebIDBPutModeAddOrUpdate;
    static const WebIDBPutMode AddOnly = WebIDBPutModeAddOnly;
    static const WebIDBPutMode CursorUpdate = WebIDBPutModeCursorUpdate;
    static const WebIDBPutMode PutModeLast = WebIDBPutModeLast;

    static const long long minimumIndexId = 30;

    typedef WebVector<WebIDBKey> WebIndexKeys;

    virtual void get(long long transactionId, long long objectStoreId, long long indexId, const WebIDBKeyRange&, bool keyOnly, WebIDBCallbacks*) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void put(long long transactionId, long long objectStoreId, const WebData& value, const WebVector<WebBlobInfo>&, const WebIDBKey&, blink::WebIDBPutMode, WebIDBCallbacks*, const WebVector<long long>& indexIds, const WebVector<WebIndexKeys>&) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void setIndexKeys(long long transactionId, long long objectStoreId, const WebIDBKey&, const WebVector<long long>& indexIds, const WebVector<WebIndexKeys>&) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void setIndexesReady(long long transactionId, long long objectStoreId, const WebVector<long long>& indexIds) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void openCursor(long long transactionId, long long objectStoreId, long long indexId, const WebIDBKeyRange&, blink::WebIDBCursorDirection, bool keyOnly, blink::WebIDBTaskType, WebIDBCallbacks*) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void count(long long transactionId, long long objectStoreId, long long indexId, const WebIDBKeyRange&, WebIDBCallbacks*) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void deleteRange(long long transactionId, long long objectStoreId, const WebIDBKeyRange&, WebIDBCallbacks*) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void clear(long long transactionId, long long objectStoreId, WebIDBCallbacks*) { BLINK_ASSERT_NOT_REACHED(); }
    virtual void ackReceivedBlobs(const WebVector<WebString>& uuids) { BLINK_ASSERT_NOT_REACHED(); }

protected:
    WebIDBDatabase() { }
};

} // namespace blink

#endif // WebIDBDatabase_h
