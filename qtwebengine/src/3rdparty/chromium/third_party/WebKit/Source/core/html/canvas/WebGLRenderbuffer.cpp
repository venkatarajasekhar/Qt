/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "core/html/canvas/WebGLRenderbuffer.h"

#include "core/html/canvas/WebGLRenderingContextBase.h"

namespace WebCore {

PassRefPtr<WebGLRenderbuffer> WebGLRenderbuffer::create(WebGLRenderingContextBase* ctx)
{
    return adoptRef(new WebGLRenderbuffer(ctx));
}

WebGLRenderbuffer::~WebGLRenderbuffer()
{
    deleteObject(0);
}

WebGLRenderbuffer::WebGLRenderbuffer(WebGLRenderingContextBase* ctx)
    : WebGLSharedObject(ctx)
    , m_internalFormat(GL_RGBA4)
    , m_width(0)
    , m_height(0)
    , m_hasEverBeenBound(false)
{
    ScriptWrappable::init(this);
    setObject(ctx->webContext()->createRenderbuffer());
}

void WebGLRenderbuffer::deleteObjectImpl(blink::WebGraphicsContext3D* context3d, Platform3DObject object)
{
    context3d->deleteRenderbuffer(object);
    deleteEmulatedStencilBuffer(context3d);
}

void WebGLRenderbuffer::deleteEmulatedStencilBuffer(blink::WebGraphicsContext3D* context3d)
{
    if (!m_emulatedStencilBuffer)
        return;
    m_emulatedStencilBuffer->deleteObject(context3d);
    m_emulatedStencilBuffer.clear();
}

}
