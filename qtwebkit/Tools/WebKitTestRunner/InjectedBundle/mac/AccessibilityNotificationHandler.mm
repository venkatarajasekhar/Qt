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
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

#import "config.h"
#import "InjectedBundle.h"
#import "InjectedBundlePage.h"
#import "JSWrapper.h"
#import "AccessibilityNotificationHandler.h"
#import "AccessibilityUIElement.h"

#import <JavaScriptCore/JSRetainPtr.h>
#import <JavaScriptCore/JSStringRef.h>
#import <JavaScriptCore/JSStringRefCF.h>
#import <WebKit2/WKBundleFrame.h>
#import <wtf/RetainPtr.h>

@interface NSObject (WebAccessibilityObjectWrapperAdditions)
+ (void)accessibilitySetShouldRepostNotifications:(BOOL)repost;
@end

@interface NSString (JSStringRefAdditions)
- (JSStringRef)createJSStringRef;
@end

@implementation NSString (JSStringRefAdditions)

- (JSStringRef)createJSStringRef
{
    return JSStringCreateWithCFString((CFStringRef)self);
}

@end

@implementation AccessibilityNotificationHandler

- (id)init
{
    if (!(self = [super init]))
        return nil;

    return self;
}

- (void)setPlatformElement:(id)platformElement
{
    m_platformElement = platformElement;
}

- (void)dealloc
{
    [self stopObserving];

    WKBundleFrameRef mainFrame = WKBundlePageGetMainFrame(WTR::InjectedBundle::shared().page()->page());
    JSContextRef context = WKBundleFrameGetJavaScriptContext(mainFrame);

    JSValueUnprotect(context, m_notificationFunctionCallback);
    m_notificationFunctionCallback = 0;

    [super dealloc];
}

- (void)setCallback:(JSValueRef)callback
{
    if (!callback)
        return;

    WKBundleFrameRef mainFrame = WKBundlePageGetMainFrame(WTR::InjectedBundle::shared().page()->page());
    JSContextRef context = WKBundleFrameGetJavaScriptContext(mainFrame);

    if (m_notificationFunctionCallback)
        JSValueUnprotect(context, m_notificationFunctionCallback);

    m_notificationFunctionCallback = callback;
    JSValueProtect(context, m_notificationFunctionCallback);
}

- (void)startObserving
{
    // Once we start requesting notifications, it's on for the duration of the program.
    // This is to avoid any race conditions between tests turning this flag on and off. Instead
    // AccessibilityNotificationHandler can ignore events it doesn't care about.
    id webAccessibilityObjectWrapperClass = NSClassFromString(@"WebAccessibilityObjectWrapper");
    ASSERT(webAccessibilityObjectWrapperClass);
    [webAccessibilityObjectWrapperClass accessibilitySetShouldRepostNotifications:YES];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_notificationReceived:) name:@"AXDRTNotification" object:nil];
}

- (void)stopObserving
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)_notificationReceived:(NSNotification *)notification
{
    NSString *notificationName = [[notification userInfo] objectForKey:@"notificationName"];
    if (!notificationName)
        return;
    if (m_platformElement && m_platformElement != [notification object])
        return;

    WKBundleFrameRef mainFrame = WKBundlePageGetMainFrame(WTR::InjectedBundle::shared().page()->page());
    JSContextRef context = WKBundleFrameGetJavaScriptContext(mainFrame);

    JSRetainPtr<JSStringRef> jsNotification(Adopt, [notificationName createJSStringRef]);
    JSValueRef notificationNameArgument = JSValueMakeString(context, jsNotification.get());
    if (m_platformElement) {
        // Listener for one element just gets one argument, the notification name.
        JSObjectCallAsFunction(context, const_cast<JSObjectRef>(m_notificationFunctionCallback), 0, 1, &notificationNameArgument, 0);
    } else {
        // A global listener gets the element and the notification name as arguments.
        JSValueRef arguments[2];
        arguments[0] = toJS(context, WTF::getPtr(WTR::AccessibilityUIElement::create([notification object])));
        arguments[1] = notificationNameArgument;
        JSObjectCallAsFunction(context, const_cast<JSObjectRef>(m_notificationFunctionCallback), 0, 2, arguments, 0);
    }
}

@end

