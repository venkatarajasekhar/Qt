/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "public/platform/WebIDBKey.h"

#include "modules/indexeddb/IDBKey.h"

using namespace WebCore;

namespace blink {

WebIDBKey WebIDBKey::createArray(const WebVector<WebIDBKey>& array)
{
    WebIDBKey key;
    key.assignArray(array);
    return key;
}

WebIDBKey WebIDBKey::createBinary(const WebData& binary)
{
    WebIDBKey key;
    key.assignBinary(binary);
    return key;
}

WebIDBKey WebIDBKey::createString(const WebString& string)
{
    WebIDBKey key;
    key.assignString(string);
    return key;
}

WebIDBKey WebIDBKey::createDate(double date)
{
    WebIDBKey key;
    key.assignDate(date);
    return key;
}

WebIDBKey WebIDBKey::createNumber(double number)
{
    WebIDBKey key;
    key.assignNumber(number);
    return key;
}

WebIDBKey WebIDBKey::createInvalid()
{
    WebIDBKey key;
    key.assignInvalid();
    return key;
}

WebIDBKey WebIDBKey::createNull()
{
    WebIDBKey key;
    key.assignNull();
    return key;
}

void WebIDBKey::assign(const WebIDBKey& value)
{
    m_private = value.m_private;
}

static IDBKey* convertFromWebIDBKeyArray(const WebVector<WebIDBKey>& array)
{
    IDBKey::KeyArray keys;
    keys.reserveCapacity(array.size());
    for (size_t i = 0; i < array.size(); ++i) {
        switch (array[i].keyType()) {
        case WebIDBKeyTypeArray:
            keys.append(convertFromWebIDBKeyArray(array[i].array()));
            break;
        case WebIDBKeyTypeBinary:
            keys.append(IDBKey::createBinary(array[i].binary()));
            break;
        case WebIDBKeyTypeString:
            keys.append(IDBKey::createString(array[i].string()));
            break;
        case WebIDBKeyTypeDate:
            keys.append(IDBKey::createDate(array[i].date()));
            break;
        case WebIDBKeyTypeNumber:
            keys.append(IDBKey::createNumber(array[i].number()));
            break;
        case WebIDBKeyTypeInvalid:
            keys.append(IDBKey::createInvalid());
            break;
        case WebIDBKeyTypeNull:
        case WebIDBKeyTypeMin:
            ASSERT_NOT_REACHED();
            break;
        }
    }
    return IDBKey::createArray(keys);
}

static void convertToWebIDBKeyArray(const IDBKey::KeyArray& array, WebVector<WebIDBKey>& result)
{
    WebVector<WebIDBKey> keys(array.size());
    WebVector<WebIDBKey> subkeys;
    for (size_t i = 0; i < array.size(); ++i) {
        IDBKey* key = array[i];
        switch (key->type()) {
        case IDBKey::ArrayType:
            convertToWebIDBKeyArray(key->array(), subkeys);
            keys[i] = WebIDBKey::createArray(subkeys);
            break;
        case IDBKey::BinaryType:
            keys[i] = WebIDBKey::createBinary(key->binary());
            break;
        case IDBKey::StringType:
            keys[i] = WebIDBKey::createString(key->string());
            break;
        case IDBKey::DateType:
            keys[i] = WebIDBKey::createDate(key->date());
            break;
        case IDBKey::NumberType:
            keys[i] = WebIDBKey::createNumber(key->number());
            break;
        case IDBKey::InvalidType:
            keys[i] = WebIDBKey::createInvalid();
            break;
        case IDBKey::MinType:
            ASSERT_NOT_REACHED();
            break;
        }
    }
    result.swap(keys);
}

void WebIDBKey::assignArray(const WebVector<WebIDBKey>& array)
{
    m_private = convertFromWebIDBKeyArray(array);
}

void WebIDBKey::assignBinary(const WebData& binary)
{
    m_private = IDBKey::createBinary(binary);
}

void WebIDBKey::assignString(const WebString& string)
{
    m_private = IDBKey::createString(string);
}

void WebIDBKey::assignDate(double date)
{
    m_private = IDBKey::createDate(date);
}

void WebIDBKey::assignNumber(double number)
{
    m_private = IDBKey::createNumber(number);
}

void WebIDBKey::assignInvalid()
{
    m_private = IDBKey::createInvalid();
}

void WebIDBKey::assignNull()
{
    m_private.reset();
}

void WebIDBKey::reset()
{
    m_private.reset();
}

WebIDBKeyType WebIDBKey::keyType() const
{
    if (!m_private.get())
        return WebIDBKeyTypeNull;
    return static_cast<WebIDBKeyType>(m_private->type());
}

bool WebIDBKey::isValid() const
{
    if (!m_private.get())
        return false;
    return m_private->isValid();
}

WebVector<WebIDBKey> WebIDBKey::array() const
{
    WebVector<WebIDBKey> keys;
    convertToWebIDBKeyArray(m_private->array(), keys);
    return keys;
}

WebData WebIDBKey::binary() const
{
    return m_private->binary();
}

WebString WebIDBKey::string() const
{
    return m_private->string();
}

double WebIDBKey::date() const
{
    return m_private->date();
}

double WebIDBKey::number() const
{
    return m_private->number();
}

WebIDBKey::WebIDBKey(IDBKey* value)
    : m_private(value)
{
}

WebIDBKey& WebIDBKey::operator=(IDBKey* value)
{
    m_private = value;
    return *this;
}

WebIDBKey::operator IDBKey*() const
{
    return m_private.get();
}

} // namespace blink
