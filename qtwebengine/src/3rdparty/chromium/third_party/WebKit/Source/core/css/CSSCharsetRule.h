/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2008, 2012 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef CSSCharsetRule_h
#define CSSCharsetRule_h

#include "core/css/CSSRule.h"

namespace WebCore {

class ExceptionState;

class CSSCharsetRule FINAL : public CSSRule {
public:
    static PassRefPtrWillBeRawPtr<CSSCharsetRule> create(CSSStyleSheet* parent, const String& encoding)
    {
        return adoptRefWillBeNoop(new CSSCharsetRule(parent, encoding));
    }

    virtual ~CSSCharsetRule() { }

    virtual CSSRule::Type type() const OVERRIDE { return CHARSET_RULE; }
    virtual String cssText() const OVERRIDE;
    virtual void reattach(StyleRuleBase* rule) OVERRIDE { ASSERT_UNUSED(rule, !rule); }

    const String& encoding() const { return m_encoding; }
    void setEncoding(const String& encoding) { m_encoding = encoding; }

    virtual void trace(Visitor* visitor) OVERRIDE { CSSRule::trace(visitor); }

private:
    CSSCharsetRule(CSSStyleSheet* parent, const String& encoding);

    String m_encoding;
};

DEFINE_CSS_RULE_TYPE_CASTS(CSSCharsetRule, CHARSET_RULE);

} // namespace WebCore

#endif // CSSCharsetRule_h
