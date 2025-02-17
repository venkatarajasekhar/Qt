/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006, 2008, 2009, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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

#include "config.h"
#include "core/html/HTMLAppletElement.h"

#include "core/HTMLNames.h"
#include "core/dom/shadow/ShadowRoot.h"
#include "core/html/HTMLParamElement.h"
#include "core/loader/FrameLoader.h"
#include "core/loader/FrameLoaderClient.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/Settings.h"
#include "core/frame/csp/ContentSecurityPolicy.h"
#include "core/rendering/RenderApplet.h"
#include "platform/Widget.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace WebCore {

using namespace HTMLNames;

HTMLAppletElement::HTMLAppletElement(Document& document, bool createdByParser)
    : HTMLPlugInElement(appletTag, document, createdByParser, ShouldNotPreferPlugInsForImages)
{
    ScriptWrappable::init(this);

    m_serviceType = "application/x-java-applet";
}

PassRefPtrWillBeRawPtr<HTMLAppletElement> HTMLAppletElement::create(Document& document, bool createdByParser)
{
    RefPtrWillBeRawPtr<HTMLAppletElement> element = adoptRefWillBeNoop(new HTMLAppletElement(document, createdByParser));
    element->ensureUserAgentShadowRoot();
    return element.release();
}

void HTMLAppletElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == altAttr
        || name == archiveAttr
        || name == codeAttr
        || name == codebaseAttr
        || name == mayscriptAttr
        || name == objectAttr) {
        // Do nothing.
        return;
    }

    HTMLPlugInElement::parseAttribute(name, value);
}

bool HTMLAppletElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == codebaseAttr || attribute.name() == objectAttr
        || HTMLPlugInElement::isURLAttribute(attribute);
}

bool HTMLAppletElement::hasLegalLinkAttribute(const QualifiedName& name) const
{
    return name == codebaseAttr || HTMLPlugInElement::hasLegalLinkAttribute(name);
}

bool HTMLAppletElement::rendererIsNeeded(const RenderStyle& style)
{
    if (!fastHasAttribute(codeAttr) && !hasAuthorShadowRoot())
        return false;
    return HTMLPlugInElement::rendererIsNeeded(style);
}

RenderObject* HTMLAppletElement::createRenderer(RenderStyle* style)
{
    if (!canEmbedJava() || hasAuthorShadowRoot())
        return RenderObject::createObject(this, style);

    return new RenderApplet(this);
}

RenderWidget* HTMLAppletElement::renderWidgetForJSBindings() const
{
    if (!canEmbedJava())
        return 0;
    return HTMLPlugInElement::renderWidgetForJSBindings();
}

RenderWidget* HTMLAppletElement::existingRenderWidget() const
{
    return renderPart();
}

void HTMLAppletElement::updateWidgetInternal()
{
    ASSERT(!m_isDelayingLoadEvent);
    setNeedsWidgetUpdate(false);
    // FIXME: This should ASSERT isFinishedParsingChildren() instead.
    if (!isFinishedParsingChildren())
        return;

    RenderEmbeddedObject* renderer = renderEmbeddedObject();

    LocalFrame* frame = document().frame();
    ASSERT(frame);

    Vector<String> paramNames;
    Vector<String> paramValues;

    const AtomicString& codeBase = getAttribute(codebaseAttr);
    if (!codeBase.isNull()) {
        KURL codeBaseURL = document().completeURL(codeBase);
        paramNames.append("codeBase");
        paramValues.append(codeBase.string());
    }

    const AtomicString& archive = getAttribute(archiveAttr);
    if (!archive.isNull()) {
        paramNames.append("archive");
        paramValues.append(archive.string());
    }

    const AtomicString& code = getAttribute(codeAttr);
    paramNames.append("code");
    paramValues.append(code.string());

    // If the 'codebase' attribute is set, it serves as a relative root for the file that the Java
    // plugin will load. If the 'code' attribute is set, and the 'archive' is not set, then we need
    // to check the url generated by resolving 'code' against 'codebase'. If the 'archive'
    // attribute is set, then 'code' points to a class inside the archive, so we need to check the
    // url generated by resolving 'archive' against 'codebase'.
    KURL urlToCheck;
    KURL rootURL = codeBase.isNull() ? document().url() : document().completeURL(codeBase);
    if (!archive.isNull())
        urlToCheck = KURL(rootURL, archive);
    else if (!code.isNull())
        urlToCheck = KURL(rootURL, code);
    if (!canEmbedURL(urlToCheck))
        return;

    const AtomicString& name = document().isHTMLDocument() ? getNameAttribute() : getIdAttribute();
    if (!name.isNull()) {
        paramNames.append("name");
        paramValues.append(name.string());
    }

    paramNames.append("baseURL");
    KURL baseURL = document().baseURL();
    paramValues.append(baseURL.string());

    const AtomicString& mayScript = getAttribute(mayscriptAttr);
    if (!mayScript.isNull()) {
        paramNames.append("mayScript");
        paramValues.append(mayScript.string());
    }

    for (HTMLParamElement* param = Traversal<HTMLParamElement>::firstChild(*this); param; param = Traversal<HTMLParamElement>::nextSibling(*param)) {
        if (param->name().isEmpty())
            continue;

        paramNames.append(param->name());
        paramValues.append(param->value());
    }

    RefPtr<Widget> widget;
    if (frame->loader().allowPlugins(AboutToInstantiatePlugin))
        widget = frame->loader().client()->createJavaAppletWidget(this, baseURL, paramNames, paramValues);

    if (!widget) {
        if (!renderer->showsUnavailablePluginIndicator())
            renderer->setPluginUnavailabilityReason(RenderEmbeddedObject::PluginMissing);
        return;
    }
    document().setContainsPlugins();
    setWidget(widget);
}

bool HTMLAppletElement::canEmbedJava() const
{
    if (document().isSandboxed(SandboxPlugins))
        return false;

    Settings* settings = document().settings();
    if (!settings)
        return false;

    if (!settings->javaEnabled())
        return false;

    return true;
}

bool HTMLAppletElement::canEmbedURL(const KURL& url) const
{
    DEFINE_STATIC_LOCAL(String, appletMimeType, ("application/x-java-applet"));

    if (!document().securityOrigin()->canDisplay(url)) {
        FrameLoader::reportLocalLoadFailed(document().frame(), url.string());
        return false;
    }

    if (!document().contentSecurityPolicy()->allowObjectFromSource(url)
        || !document().contentSecurityPolicy()->allowPluginType(appletMimeType, appletMimeType, url)) {
        renderEmbeddedObject()->setPluginUnavailabilityReason(RenderEmbeddedObject::PluginBlockedByContentSecurityPolicy);
        return false;
    }
    return true;
}

}
