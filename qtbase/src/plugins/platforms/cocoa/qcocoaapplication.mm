/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/****************************************************************************
**
** Copyright (c) 2007-2008, Apple, Inc.
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright notice,
**     this list of conditions and the following disclaimer.
**
**   * Redistributions in binary form must reproduce the above copyright notice,
**     this list of conditions and the following disclaimer in the documentation
**     and/or other materials provided with the distribution.
**
**   * Neither the name of Apple, Inc. nor the names of its contributors
**     may be used to endorse or promote products derived from this software
**     without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#include <qcocoaapplication.h>

#include <qcocoaintrospection.h>
#include <qcocoaapplicationdelegate.h>
#include <qcocoahelpers.h>
#include <qguiapplication.h>
#include <qdebug.h>

QT_USE_NAMESPACE

@implementation NSApplication (QT_MANGLE_NAMESPACE(QApplicationIntegration))

- (void)QT_MANGLE_NAMESPACE(qt_setDockMenu):(NSMenu *)newMenu
{
    [[QCocoaApplicationDelegate sharedDelegate] setDockMenu:newMenu];
}

- (QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *)QT_MANGLE_NAMESPACE(qt_qcocoamenuLoader)
{
    return [[QCocoaApplicationDelegate sharedDelegate] menuLoader];
}

- (int)QT_MANGLE_NAMESPACE(qt_validModesForFontPanel):(NSFontPanel *)fontPanel
{
    Q_UNUSED(fontPanel);
    // only display those things that QFont can handle
    return NSFontPanelFaceModeMask
            | NSFontPanelSizeModeMask
            | NSFontPanelCollectionModeMask
            | NSFontPanelUnderlineEffectModeMask
            | NSFontPanelStrikethroughEffectModeMask;
}

- (void)qt_sendPostedMessage:(NSEvent *)event
{
    // WARNING: data1 and data2 is truncated to from 64-bit to 32-bit on OS 10.5!
    // That is why we need to split the address in two parts:
    quint64 lower = [event data1];
    quint64 upper = [event data2];
    QCocoaPostMessageArgs *args = reinterpret_cast<QCocoaPostMessageArgs *>(lower | (upper << 32));
    // Special case for convenience: if the argument is an NSNumber, we unbox it directly.
    // Use NSValue instead if this behaviour is unwanted.
    id a1 = ([args->arg1 isKindOfClass:[NSNumber class]]) ? (id)[args->arg1 longValue] : args->arg1;
    id a2 = ([args->arg2 isKindOfClass:[NSNumber class]]) ? (id)[args->arg2 longValue] : args->arg2;
    switch (args->argCount) {
    case 0:
        [args->target performSelector:args->selector];
        break;
    case 1:
        [args->target performSelector:args->selector withObject:a1];
        break;
    case 3:
        [args->target performSelector:args->selector withObject:a1 withObject:a2];
        break;
    }

    delete args;
}

static const QByteArray q_macLocalEventType = QByteArrayLiteral("mac_generic_NSEvent");

- (BOOL)qt_filterEvent:(NSEvent *)event
{
    if (qApp && qApp->eventDispatcher()->
            filterNativeEvent(q_macLocalEventType, static_cast<void*>(event), 0))
        return true;

    if ([event type] == NSApplicationDefined) {
        switch ([event subtype]) {
            case QtCocoaEventSubTypePostMessage:
                [NSApp qt_sendPostedMessage:event];
                return true;
            default:
                break;
        }
    }

    return false;
}

@end

@implementation QNSApplication

- (void)qt_sendEvent_original:(NSEvent *)event
{
    Q_UNUSED(event);
    // This method will only be used as a signature
    // template for the method we add into NSApplication
    // containing the original [NSApplication sendEvent:] implementation
}

- (void)qt_sendEvent_replacement:(NSEvent *)event
{
    // This method (or its implementation to be precise) will
    // be called instead of sendEvent if redirection occurs.
    // 'self' will then be an instance of NSApplication
    // (and not QNSApplication)
    if (![NSApp qt_filterEvent:event])
        [self qt_sendEvent_original:event];
}

- (void)sendEvent:(NSEvent *)event
{
    // This method will be called if
    // no redirection occurs
    if (![NSApp qt_filterEvent:event])
        [super sendEvent:event];
}

@end

QT_BEGIN_NAMESPACE

void qt_redirectNSApplicationSendEvent()
{
    if (QCoreApplication::testAttribute(Qt::AA_MacPluginApplication))
        // In a plugin we cannot chain sendEvent hooks: a second plugin could
        // store the implementation of the first, which during the program flow
        // can be unloaded.
        return;

    if ([NSApp isMemberOfClass:[QNSApplication class]]) {
        // No need to change implementation since Qt
        // already controls a subclass of NSApplication
        return;
    }

    // Change the implementation of [NSApplication sendEvent] to the
    // implementation of qt_sendEvent_replacement found in QNSApplication.
    // And keep the old implementation that gets overwritten inside a new
    // method 'qt_sendEvent_original' that we add to NSApplication
    qt_cocoa_change_implementation(
            [NSApplication class],
            @selector(sendEvent:),
            [QNSApplication class],
            @selector(qt_sendEvent_replacement:),
            @selector(qt_sendEvent_original:));
 }

void qt_resetNSApplicationSendEvent()
{
    if (QCoreApplication::testAttribute(Qt::AA_MacPluginApplication))
        return;


    qt_cocoa_change_back_implementation([NSApplication class],
                                         @selector(sendEvent:),
                                         @selector(QT_MANGLE_NAMESPACE(qt_sendEvent_original):));
}

QT_END_NAMESPACE
