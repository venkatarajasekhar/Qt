// This file was GENERATED by command:
//     pump.py sigslottester.h.pump
// DO NOT EDIT BY HAND!!!

/*
 * libjingle
 * Copyright 2014 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TALK_BASE_SIGSLOTTESTER_H_
#define TALK_BASE_SIGSLOTTESTER_H_

// To generate sigslottester.h from sigslottester.h.pump, execute:
// /home/build/google3/third_party/gtest/scripts/pump.py sigslottester.h.pump


// SigslotTester(s) are utility classes to check if signals owned by an
// object are being invoked at the right time and with the right arguments.
// They are meant to be used in tests. Tests must provide "capture" pointers
// (i.e. address of variables) where the arguments from the signal callback
// can be stored.
//
// Example:
//   /* Some signal */
//   sigslot::signal1<const std::string&> foo;
//
//   /* We want to monitor foo in some test. Note how signal argument is
//      const std::string&, but capture-type is std::string. Capture type
//      must be type that can be assigned to. */
//   std::string capture;
//   SigslotTester1<const std::string&, std::string> slot(&foo, &capture);
//   foo.emit("hello");
//   EXPECT_EQ(1, slot.callback_count());
//   EXPECT_EQ("hello", capture);
//   /* See unit-tests for more examples */

#include "talk/base/constructormagic.h"
#include "talk/base/sigslot.h"

namespace talk_base {

// For all the templates below:
// - A1-A5 is the type of the argument i in the callback. Signals may and often
//   do use const-references here for efficiency.
// - C1-C5 is the type of the variable to capture argument i. These should be
//   non-const value types suitable for use as lvalues.

template <class A1, class C1>
class SigslotTester1 : public sigslot::has_slots<> {
 public:
  SigslotTester1(sigslot::signal1<A1>* signal,
                C1* capture1)
      : callback_count_(0),
      capture1_(capture1) {
    signal->connect(this, &SigslotTester1::OnSignalCallback);
  }

  int callback_count() const { return callback_count_; }

 private:
  void OnSignalCallback(A1 arg1) {
    callback_count_++;
    *capture1_ = arg1;
  }

  int callback_count_;
  C1* capture1_;

  DISALLOW_COPY_AND_ASSIGN(SigslotTester1);
};

template <class A1, class A2, class C1, class C2>
class SigslotTester2 : public sigslot::has_slots<> {
 public:
  SigslotTester2(sigslot::signal2<A1, A2>* signal,
                C1* capture1, C2* capture2)
      : callback_count_(0),
      capture1_(capture1), capture2_(capture2) {
    signal->connect(this, &SigslotTester2::OnSignalCallback);
  }

  int callback_count() const { return callback_count_; }

 private:
  void OnSignalCallback(A1 arg1, A2 arg2) {
    callback_count_++;
    *capture1_ = arg1;
    *capture2_ = arg2;
  }

  int callback_count_;
  C1* capture1_;
  C2* capture2_;

  DISALLOW_COPY_AND_ASSIGN(SigslotTester2);
};

template <class A1, class A2, class A3, class C1, class C2, class C3>
class SigslotTester3 : public sigslot::has_slots<> {
 public:
  SigslotTester3(sigslot::signal3<A1, A2, A3>* signal,
                C1* capture1, C2* capture2, C3* capture3)
      : callback_count_(0),
      capture1_(capture1), capture2_(capture2), capture3_(capture3) {
    signal->connect(this, &SigslotTester3::OnSignalCallback);
  }

  int callback_count() const { return callback_count_; }

 private:
  void OnSignalCallback(A1 arg1, A2 arg2, A3 arg3) {
    callback_count_++;
    *capture1_ = arg1;
    *capture2_ = arg2;
    *capture3_ = arg3;
  }

  int callback_count_;
  C1* capture1_;
  C2* capture2_;
  C3* capture3_;

  DISALLOW_COPY_AND_ASSIGN(SigslotTester3);
};

template <class A1, class A2, class A3, class A4, class C1, class C2, class C3,
    class C4>
class SigslotTester4 : public sigslot::has_slots<> {
 public:
  SigslotTester4(sigslot::signal4<A1, A2, A3, A4>* signal,
                C1* capture1, C2* capture2, C3* capture3, C4* capture4)
      : callback_count_(0),
      capture1_(capture1), capture2_(capture2), capture3_(capture3),
          capture4_(capture4) {
    signal->connect(this, &SigslotTester4::OnSignalCallback);
  }

  int callback_count() const { return callback_count_; }

 private:
  void OnSignalCallback(A1 arg1, A2 arg2, A3 arg3, A4 arg4) {
    callback_count_++;
    *capture1_ = arg1;
    *capture2_ = arg2;
    *capture3_ = arg3;
    *capture4_ = arg4;
  }

  int callback_count_;
  C1* capture1_;
  C2* capture2_;
  C3* capture3_;
  C4* capture4_;

  DISALLOW_COPY_AND_ASSIGN(SigslotTester4);
};

template <class A1, class A2, class A3, class A4, class A5, class C1, class C2,
    class C3, class C4, class C5>
class SigslotTester5 : public sigslot::has_slots<> {
 public:
  SigslotTester5(sigslot::signal5<A1, A2, A3, A4, A5>* signal,
                C1* capture1, C2* capture2, C3* capture3, C4* capture4,
                    C5* capture5)
      : callback_count_(0),
      capture1_(capture1), capture2_(capture2), capture3_(capture3),
          capture4_(capture4), capture5_(capture5) {
    signal->connect(this, &SigslotTester5::OnSignalCallback);
  }

  int callback_count() const { return callback_count_; }

 private:
  void OnSignalCallback(A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5) {
    callback_count_++;
    *capture1_ = arg1;
    *capture2_ = arg2;
    *capture3_ = arg3;
    *capture4_ = arg4;
    *capture5_ = arg5;
  }

  int callback_count_;
  C1* capture1_;
  C2* capture2_;
  C3* capture3_;
  C4* capture4_;
  C5* capture5_;

  DISALLOW_COPY_AND_ASSIGN(SigslotTester5);
};
}  // namespace talk_base

#endif  // TALK_BASE_SIGSLOTTESTER_H_
