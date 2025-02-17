/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
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

#import <WebCore/DOMObject.h>

#if WEBKIT_VERSION_MAX_ALLOWED >= WEBKIT_VERSION_LATEST

@class DOMNode;
@class DOMTestObj;
@class NSString;

enum {
#if ENABLE(Condition22) || ENABLE(Condition23)
    DOM_IMPLEMENTSCONSTANT1 = 1,
#endif
#if ENABLE(Condition22) || ENABLE(Condition23)
    DOM_IMPLEMENTSCONSTANT2 = 2,
#endif
#if ENABLE(Condition11) || ENABLE(Condition12)
    DOM_SUPPLEMENTALCONSTANT1 = 1,
#endif
#if ENABLE(Condition11) || ENABLE(Condition12)
    DOM_SUPPLEMENTALCONSTANT2 = 2
#endif

};

@interface DOMTestInterface : DOMObject
@property(readonly, copy) NSString *implementsStr1;
@property(copy) NSString *implementsStr2;
@property(copy) NSString *implementsStr3;
@property(retain) DOMNode *implementsNode;
@property(readonly, copy) NSString *supplementalStr1;
@property(copy) NSString *supplementalStr2;
@property(copy) NSString *supplementalStr3;
@property(retain) DOMNode *supplementalNode;

#if ENABLE(Condition22) || ENABLE(Condition23)
- (void)implementsMethod1;
#endif
#if ENABLE(Condition22) || ENABLE(Condition23)
- (DOMTestObj *)implementsMethod2:(NSString *)strArg objArg:(DOMTestObj *)objArg;
#endif
#if ENABLE(Condition22) || ENABLE(Condition23)
- (void)implementsMethod3;
#endif
#if ENABLE(Condition22) || ENABLE(Condition23)
- (void)implementsMethod4;
#endif
#if ENABLE(Condition11) || ENABLE(Condition12)
- (void)supplementalMethod1;
#endif
#if ENABLE(Condition11) || ENABLE(Condition12)
- (DOMTestObj *)supplementalMethod2:(NSString *)strArg objArg:(DOMTestObj *)objArg;
#endif
#if ENABLE(Condition11) || ENABLE(Condition12)
- (void)supplementalMethod3;
#endif
#if ENABLE(Condition11) || ENABLE(Condition12)
- (void)supplementalMethod4;
#endif
@end

#endif
