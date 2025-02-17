/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_BASE_THREAD_H_
#define WEBRTC_BASE_THREAD_H_

#include <algorithm>
#include <list>
#include <string>
#include <vector>

#if defined(WEBRTC_POSIX)
#include <pthread.h>
#endif
#include "webrtc/base/constructormagic.h"
#include "webrtc/base/event.h"
#include "webrtc/base/messagequeue.h"

#if defined(WEBRTC_WIN)
#include "webrtc/base/win32.h"
#endif

namespace rtc {

class Thread;

class ThreadManager {
 public:
  ThreadManager();
  ~ThreadManager();

  static ThreadManager* Instance();

  Thread* CurrentThread();
  void SetCurrentThread(Thread* thread);

  // Returns a thread object with its thread_ ivar set
  // to whatever the OS uses to represent the thread.
  // If there already *is* a Thread object corresponding to this thread,
  // this method will return that.  Otherwise it creates a new Thread
  // object whose wrapped() method will return true, and whose
  // handle will, on Win32, be opened with only synchronization privileges -
  // if you need more privilegs, rather than changing this method, please
  // write additional code to adjust the privileges, or call a different
  // factory method of your own devising, because this one gets used in
  // unexpected contexts (like inside browser plugins) and it would be a
  // shame to break it.  It is also conceivable on Win32 that we won't even
  // be able to get synchronization privileges, in which case the result
  // will have a NULL handle.
  Thread *WrapCurrentThread();
  void UnwrapCurrentThread();

 private:
#if defined(WEBRTC_POSIX)
  pthread_key_t key_;
#endif

#if defined(WEBRTC_WIN)
  DWORD key_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ThreadManager);
};

struct _SendMessage {
  _SendMessage() {}
  Thread *thread;
  Message msg;
  bool *ready;
};

enum ThreadPriority {
  PRIORITY_IDLE = -1,
  PRIORITY_NORMAL = 0,
  PRIORITY_ABOVE_NORMAL = 1,
  PRIORITY_HIGH = 2,
};

class Runnable {
 public:
  virtual ~Runnable() {}
  virtual void Run(Thread* thread) = 0;

 protected:
  Runnable() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Runnable);
};

// WARNING! SUBCLASSES MUST CALL Stop() IN THEIR DESTRUCTORS!  See ~Thread().

class Thread : public MessageQueue {
 public:
  explicit Thread(SocketServer* ss = NULL);
  // NOTE: ALL SUBCLASSES OF Thread MUST CALL Stop() IN THEIR DESTRUCTORS (or
  // guarantee Stop() is explicitly called before the subclass is destroyed).
  // This is required to avoid a data race between the destructor modifying the
  // vtable, and the Thread::PreRun calling the virtual method Run().
  virtual ~Thread();

  static Thread* Current();

  bool IsCurrent() const {
    return Current() == this;
  }

  // Sleeps the calling thread for the specified number of milliseconds, during
  // which time no processing is performed. Returns false if sleeping was
  // interrupted by a signal (POSIX only).
  static bool SleepMs(int millis);

  // Sets the thread's name, for debugging. Must be called before Start().
  // If |obj| is non-NULL, its value is appended to |name|.
  const std::string& name() const { return name_; }
  bool SetName(const std::string& name, const void* obj);

  // Sets the thread's priority. Must be called before Start().
  ThreadPriority priority() const { return priority_; }
  bool SetPriority(ThreadPriority priority);

  // Starts the execution of the thread.
  bool Start(Runnable* runnable = NULL);

  // Tells the thread to stop and waits until it is joined.
  // Never call Stop on the current thread.  Instead use the inherited Quit
  // function which will exit the base MessageQueue without terminating the
  // underlying OS thread.
  virtual void Stop();

  // By default, Thread::Run() calls ProcessMessages(kForever).  To do other
  // work, override Run().  To receive and dispatch messages, call
  // ProcessMessages occasionally.
  virtual void Run();

  virtual void Send(MessageHandler *phandler, uint32 id = 0,
      MessageData *pdata = NULL);

  // Convenience method to invoke a functor on another thread.  Caller must
  // provide the |ReturnT| template argument, which cannot (easily) be deduced.
  // Uses Send() internally, which blocks the current thread until execution
  // is complete.
  // Ex: bool result = thread.Invoke<bool>(&MyFunctionReturningBool);
  template <class ReturnT, class FunctorT>
  ReturnT Invoke(const FunctorT& functor) {
    FunctorMessageHandler<ReturnT, FunctorT> handler(functor);
    Send(&handler);
    return handler.result();
  }

  // From MessageQueue
  virtual void Clear(MessageHandler *phandler, uint32 id = MQID_ANY,
                     MessageList* removed = NULL);
  virtual void ReceiveSends();

  // ProcessMessages will process I/O and dispatch messages until:
  //  1) cms milliseconds have elapsed (returns true)
  //  2) Stop() is called (returns false)
  bool ProcessMessages(int cms);

  // Returns true if this is a thread that we created using the standard
  // constructor, false if it was created by a call to
  // ThreadManager::WrapCurrentThread().  The main thread of an application
  // is generally not owned, since the OS representation of the thread
  // obviously exists before we can get to it.
  // You cannot call Start on non-owned threads.
  bool IsOwned();

#if defined(WEBRTC_WIN)
  HANDLE GetHandle() const {
    return thread_;
  }
  DWORD GetId() const {
    return thread_id_;
  }
#elif defined(WEBRTC_POSIX)
  pthread_t GetPThread() {
    return thread_;
  }
#endif

  // This method should be called when thread is created using non standard
  // method, like derived implementation of rtc::Thread and it can not be
  // started by calling Start(). This will set started flag to true and
  // owned to false. This must be called from the current thread.
  // NOTE: These methods should be used by the derived classes only, added here
  // only for testing.
  bool WrapCurrent();
  void UnwrapCurrent();

  // Expose private method running() for tests.
  //
  // DANGER: this is a terrible public API.  Most callers that might want to
  // call this likely do not have enough control/knowledge of the Thread in
  // question to guarantee that the returned value remains true for the duration
  // of whatever code is conditionally executing because of the return value!
  bool RunningForTest() { return running(); }
  // This is a legacy call-site that probably doesn't need to exist in the first
  // place.
  // TODO(fischman): delete once the ASSERT added in channelmanager.cc sticks
  // for a month (ETA 2014/06/22).
  bool RunningForChannelManager() { return running(); }

 protected:
  // Blocks the calling thread until this thread has terminated.
  void Join();

 private:
  static void *PreRun(void *pv);

  // ThreadManager calls this instead WrapCurrent() because
  // ThreadManager::Instance() cannot be used while ThreadManager is
  // being created.
  bool WrapCurrentWithThreadManager(ThreadManager* thread_manager);

  // Return true if the thread was started and hasn't yet stopped.
  bool running() { return running_.Wait(0); }

  std::list<_SendMessage> sendlist_;
  std::string name_;
  ThreadPriority priority_;
  Event running_;  // Signalled means running.

#if defined(WEBRTC_POSIX)
  pthread_t thread_;
#endif

#if defined(WEBRTC_WIN)
  HANDLE thread_;
  DWORD thread_id_;
#endif

  bool owned_;

  friend class ThreadManager;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

// AutoThread automatically installs itself at construction
// uninstalls at destruction, if a Thread object is
// _not already_ associated with the current OS thread.

class AutoThread : public Thread {
 public:
  explicit AutoThread(SocketServer* ss = 0);
  virtual ~AutoThread();

 private:
  DISALLOW_COPY_AND_ASSIGN(AutoThread);
};

// Win32 extension for threads that need to use COM
#if defined(WEBRTC_WIN)
class ComThread : public Thread {
 public:
  ComThread() {}
  virtual ~ComThread() { Stop(); }

 protected:
  virtual void Run();

 private:
  DISALLOW_COPY_AND_ASSIGN(ComThread);
};
#endif

// Provides an easy way to install/uninstall a socketserver on a thread.
class SocketServerScope {
 public:
  explicit SocketServerScope(SocketServer* ss) {
    old_ss_ = Thread::Current()->socketserver();
    Thread::Current()->set_socketserver(ss);
  }
  ~SocketServerScope() {
    Thread::Current()->set_socketserver(old_ss_);
  }

 private:
  SocketServer* old_ss_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(SocketServerScope);
};

}  // namespace rtc

#endif  // WEBRTC_BASE_THREAD_H_
