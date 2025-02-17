/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QML Shaders plugin of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "shadereffectsource.h"
#include "shadereffectbuffer.h"
#include "shadereffect.h"
#include "glfunctions.h"

/*!
    \qmltype ShaderEffectSource
    \instantiates ShaderEffectSource
    \ingroup qml-shader-elements
    \brief The ShaderEffectSource object encapsulates the source content for the ShaderEffectItem.

    ShaderEffectSource is available in the \b{Qt.labs.shaders 1.0} module.
    \e {Elements in the Qt.labs module are not guaranteed to remain compatible
    in future versions.}

    This element provides preliminary support for OpenGL shaders in QML,
    and may be heavily changed or removed in later versions.

    Requirement for the ability to use of shaders is that the application is either using
    opengl graphicssystem or has set QGLWidget as the viewport to QDeclarativeView (recommended way).

    ShaderEffectSource object encapsulates the source content so that it can be utilized in ShaderEffectItem.
    Source content can be a live QML object tree, or a snapshot of QML object tree.

*/

ShaderEffectSource::ShaderEffectSource(QDeclarativeItem *parent)
    : QDeclarativeItem(parent)
    , m_sourceItem(0)
    , m_wrapMode(ClampToEdge)
    , m_sourceRect(0, 0, 0, 0)
    , m_textureSize(0, 0)
    , m_format(RGBA)
    , m_size(0, 0)
    , m_fbo(0)
    , m_multisampledFbo(0)
    , m_refs(0)
    , m_dirtyTexture(true)
    , m_dirtySceneGraph(true)
    , m_multisamplingSupported(false)
    , m_checkedForMultisamplingSupport(false)
    , m_live(true)
    , m_hideSource(false)
    , m_mirrored(false)
{
}

ShaderEffectSource::~ShaderEffectSource()
{
    if (m_refs && m_sourceItem)
        detachSourceItem();

    delete m_fbo;
    delete m_multisampledFbo;
}

/*!
    \qmlproperty Item ShaderEffectSource::sourceItem
    This property holds the Item which is used as the source for the shader effect.
    If the item has children, those are included as well.

    \note When source item content is passed to the ShaderEffectItem(s), it is always clipped to the boundingrect of the
    sourceItem regardless of its clipping property.
*/

void ShaderEffectSource::setSourceItem(QDeclarativeItem *item)
{
    if (item == m_sourceItem)
        return;

    if (m_sourceItem) {
        disconnect(m_sourceItem, SIGNAL(widthChanged()), this, SLOT(markSourceSizeDirty()));
        disconnect(m_sourceItem, SIGNAL(heightChanged()), this, SLOT(markSourceSizeDirty()));

        if (m_refs)
            detachSourceItem();
    }

    m_sourceItem = item;

    if (m_sourceItem) {

        // Must have some item as parent
        if (m_sourceItem->parentItem() == 0)
            m_sourceItem->setParentItem(this);

        if (m_refs)
            attachSourceItem();

        connect(m_sourceItem, SIGNAL(widthChanged()), this, SLOT(markSourceSizeDirty()));
        connect(m_sourceItem, SIGNAL(heightChanged()), this, SLOT(markSourceSizeDirty()));
    }

    updateSizeAndTexture();
    emit sourceItemChanged();
    emit repaintRequired();
}

/*!
    \qmlproperty QRectF ShaderEffectSource::sourceRect
    This property can be used to specify margins for the source content.

    If other value than Qt.rect(0,0,0,0) is assigned to this property, it is interpreted as
    specifying a relative source rectangle for the source content.

    For example, setting Qt.rect(-10.0, -10.0, 120.0, 120.0) for a source that has width and height
    of 100 pixels would produce 10 pixels margins to each side of the source.

    Margins are useful when the original content is wanted to be spread outside the original source area,
    like when creating a dropshadow with the shader or in other similar effects.

    The default value is Qt.rect(0,0,0,0).
*/

void ShaderEffectSource::setSourceRect(const QRectF &rect)
{
    if (rect == m_sourceRect)
        return;
    m_sourceRect = rect;
    updateSizeAndTexture();
    emit sourceRectChanged();
    emit repaintRequired();

    m_dirtyTexture = true;
    markSourceItemDirty();
}

/*!
    \qmlproperty QSize ShaderEffectSource::textureSize
    This property holds the size for the texture containing the source content.

    If value QSize(0,0) is assigned to this property, texture is resized
    according to the source size. Otherwise source content is scaled to
    the given size.

    The default value is QSize(0,0).
*/

void ShaderEffectSource::setTextureSize(const QSize &size)
{
    if (size == m_textureSize)
        return;

    m_textureSize = size;
    updateSizeAndTexture();
    emit textureSizeChanged();
    emit repaintRequired();

    m_dirtyTexture = true;
    markSourceItemDirty();
}

/*!
    \qmlproperty bool ShaderEffectSource::live
    This property holds the optimization flag to define whether the source item content is changing or
    static.

    If value true is assigned to this property, source item content is re-rendered into a
    texture for every frame. Setting the value to false improves the performance as it skips
    rendering the source item (and its chidleren) and instead immediately passes the previously
    rendered and cached texture to the shaders.

    The default value is true.
*/

void ShaderEffectSource::setLive(bool s)
{
    if (s == m_live)
        return;

    m_live = s;

    emit liveChanged();
    emit repaintRequired();
}

/*!
    \qmlproperty bool ShaderEffectSource::hideSource
    This property holds the flag to define whether the original source item is
    hidden when the effect item is drawn.

    The default value is false.
*/

void ShaderEffectSource::setHideSource(bool hide)
{
    if (hide == m_hideSource)
        return;

    m_hideSource = hide;

    emit hideSourceChanged();
    emit repaintRequired();
}

/*!
    \qmlproperty enumeration ShaderEffectSource::wrapMode

    This property defines the wrap parameter for the source after it has been mapped as a texture.

    \list
    \li ShaderEffectSource.ClampToEdge - Causes texturecoordinates to be clamped to the range [ 1/2*N , 1 - 1/2*N ], where N is the texture width.
    \li ShaderEffectSource.RepeatHorizontally - Causes the integer part of the horizontal texturecoordinate to be ignored; the GL uses only the fractional part, thereby creating a horizontal repeating pattern.
    \li ShaderEffectSource.RepeatVertically - Causes the integer part of the vertical texturecoordinate to be ignored; the GL uses only the fractional part, thereby creating a vertical repeating pattern.
    \li ShaderEffectSource.Repeat - Causes the integer part of both the horizontal and vertical texturecoordinates to be ignored; the GL uses only the fractional part, thereby creating a repeating pattern.
    \endlist

    The default value is ClampToEdge.

*/

void ShaderEffectSource::setWrapMode(WrapMode mode)
{
    if (mode == m_wrapMode)
        return;

    m_wrapMode = mode;
    emit wrapModeChanged();

    m_dirtyTexture = true;
    markSourceItemDirty();
}

/*!
    \qmlmethod ShaderEffectSource::grab()

    Repaints the source item content into the texture.

    This method is useful when ShaderEffectSource::live has been set to false and
    the changes in the source item content is desired to be made visible for the shaders.

*/

void ShaderEffectSource::grab()
{
    m_dirtyTexture = true;
    emit repaintRequired();
}

void ShaderEffectSource::bind()
{
    GLint filtering = smooth() ? GL_LINEAR : GL_NEAREST;
    GLuint hwrap = (m_wrapMode == Repeat || m_wrapMode == RepeatHorizontally) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    GLuint vwrap = (m_wrapMode == Repeat || m_wrapMode == RepeatVertically) ? GL_REPEAT : GL_CLAMP_TO_EDGE;

    QOpenGLContext *context = QOpenGLContext::currentContext();
    QOpenGLFunctions *f = context->functions();
    if (!context->isOpenGLES())
        f->glEnable(GL_TEXTURE_2D);

    if (m_fbo && m_fbo->isValid()) {
        f->glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
    } else {
        m_dirtyTexture = true;
        emit repaintRequired();
        markSourceItemDirty();
        f->glBindTexture(GL_TEXTURE_2D, 0);
    }

    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth() ? GL_LINEAR : GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, hwrap);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, vwrap);
}

void ShaderEffectSource::refFromEffectItem()
{
    if (m_refs++ == 0) {
        attachSourceItem();
        emit activeChanged();
    }
}

void ShaderEffectSource::derefFromEffectItem()
{
    if (--m_refs == 0) {
        detachSourceItem();
        emit activeChanged();
    }
    Q_ASSERT(m_refs >= 0);
}

void ShaderEffectSource::updateBackbuffer()
{
    if (!m_sourceItem || !QGLContext::currentContext())
        return;

    // Multisampling is not (for now) supported.
    QSize size = QSize(m_sourceItem->width(), m_sourceItem->height());
    if (!m_textureSize.isEmpty())
        size = m_textureSize;

    if (size.height() > 0 && size.width() > 0) {
        QGLFramebufferObjectFormat format;
        format.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
        format.setInternalTextureFormat(m_format);

        if (!m_fbo) {
            m_fbo =  new ShaderEffectBuffer(size, format);
        } else {
            if (!m_fbo->isValid() || m_fbo->size() != size || m_fbo->format().internalTextureFormat() != GLenum(m_format)) {
                delete m_fbo;
                m_fbo = 0;
                m_fbo =  new ShaderEffectBuffer(size, format);
            }
        }
    }

    // Note that real update for the source content happens in shadereffect.cpp
    m_dirtyTexture = false;
}

void ShaderEffectSource::markSceneGraphDirty()
{
    m_dirtySceneGraph = true;
    emit repaintRequired();
}

void ShaderEffectSource::markSourceSizeDirty()
{
    Q_ASSERT(m_sourceItem);
    if (m_textureSize.isEmpty())
        updateSizeAndTexture();
    if (m_refs)
        emit repaintRequired();
}

void ShaderEffectSource::markSourceItemDirty()
{
    m_dirtyTexture = true;
    if (m_sourceItem) {
        ShaderEffect* effect = qobject_cast<ShaderEffect*> (m_sourceItem->graphicsEffect());
        if (effect)
            effect->m_changed = true;
    }
}

void ShaderEffectSource::updateSizeAndTexture()
{
    if (m_sourceItem) {
        QSize size = m_textureSize;
        if (size.isEmpty())
            size = QSize(m_sourceItem->width(), m_sourceItem->height());
        if (size.width() < 1)
            size.setWidth(1);
        if (size.height() < 1)
            size.setHeight(1);
        if (m_fbo && (m_fbo->size() != size || !m_fbo->isValid())) {
            delete m_fbo;
            m_fbo = 0;
            delete m_multisampledFbo;
            m_fbo = m_multisampledFbo = 0;
        }
        if (m_size.width() != size.width()) {
            m_size.setWidth(size.width());
            emit widthChanged();
        }
        if (m_size.height() != size.height()) {
            m_size.setHeight(size.height());
            emit heightChanged();
        }
        m_dirtyTexture = true;
    } else {
        if (m_size.width() != 0) {
            m_size.setWidth(0);
            emit widthChanged();
        }
        if (m_size.height() != 0) {
            m_size.setHeight(0);
            emit heightChanged();
        }
    }
}

void ShaderEffectSource::attachSourceItem()
{
    if (!m_sourceItem)
        return;

    ShaderEffect *effect = qobject_cast<ShaderEffect*> (m_sourceItem->graphicsEffect());

    if (!effect) {
        effect = new ShaderEffect();
        m_sourceItem->setGraphicsEffect(effect);
    }

    if (effect)
        effect->addRenderTarget(this);

    m_sourceItem->update();
}

void ShaderEffectSource::detachSourceItem()
{
    if (!m_sourceItem)
        return;

    ShaderEffect* effect = qobject_cast<ShaderEffect*> (m_sourceItem->graphicsEffect());

    if (effect)
        effect->removeRenderTarget(this);

    delete m_fbo;
    m_fbo = 0;

    delete m_multisampledFbo;
    m_multisampledFbo = 0;

    m_dirtyTexture = true;
}
