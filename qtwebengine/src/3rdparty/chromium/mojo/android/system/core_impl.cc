// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/android/system/core_impl.h"

#include "base/android/base_jni_registrar.h"
#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "base/android/library_loader/library_loader_hooks.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "jni/CoreImpl_jni.h"
#include "mojo/embedder/embedder.h"
#include "mojo/public/c/environment/async_waiter.h"
#include "mojo/public/c/system/core.h"
#include "mojo/public/cpp/environment/environment.h"

namespace {

// |AsyncWait| is guaranteed never to return 0.
const MojoAsyncWaitID kInvalidHandleCancelID = 0;

struct AsyncWaitCallbackData {
  base::android::ScopedJavaGlobalRef<jobject> core_impl;
  base::android::ScopedJavaGlobalRef<jobject> callback;
  base::android::ScopedJavaGlobalRef<jobject> cancellable;

  AsyncWaitCallbackData(JNIEnv* env, jobject core_impl, jobject callback) {
    this->core_impl.Reset(env, core_impl);
    this->callback.Reset(env, callback);
  }
};

void AsyncWaitCallback(void* data, MojoResult result) {
  scoped_ptr<AsyncWaitCallbackData> callback_data(
      static_cast<AsyncWaitCallbackData*>(data));
  mojo::android::Java_CoreImpl_onAsyncWaitResult(
      base::android::AttachCurrentThread(),
      callback_data->core_impl.obj(),
      result,
      callback_data->callback.obj(),
      callback_data->cancellable.obj());
}

}  // namespace

namespace mojo {
namespace android {

static void Constructor(JNIEnv* env, jobject jcaller) {
  mojo::embedder::Init();
}

static jlong GetTimeTicksNow(JNIEnv* env, jobject jcaller) {
  return MojoGetTimeTicksNow();
}

static jint WaitMany(JNIEnv* env,
                     jobject jcaller,
                     jobject buffer,
                     jlong deadline) {
  // Buffer contains first the list of handles, then the list of flags.
  const void* buffer_start = env->GetDirectBufferAddress(buffer);
  DCHECK(buffer_start);
  const size_t record_size = 8;
  const size_t buffer_size = env->GetDirectBufferCapacity(buffer);
  DCHECK_EQ(buffer_size % record_size, 0u);

  const size_t nb_handles = buffer_size / record_size;
  const MojoHandle* handle_start = static_cast<const MojoHandle*>(buffer_start);
  const MojoHandleSignals* signals_start =
      static_cast<const MojoHandleSignals*>(handle_start + nb_handles);
  return MojoWaitMany(handle_start, signals_start, nb_handles, deadline);
}

static jobject CreateMessagePipe(JNIEnv* env, jobject jcaller) {
  MojoHandle handle1;
  MojoHandle handle2;
  // TODO(vtl): Add support for the options struct.
  MojoResult result = MojoCreateMessagePipe(NULL, &handle1, &handle2);
  return Java_CoreImpl_newNativeCreationResult(env, result, handle1, handle2)
      .Release();
}

static jobject CreateDataPipe(JNIEnv* env,
                              jobject jcaller,
                              jobject options_buffer) {
  const MojoCreateDataPipeOptions* options = NULL;
  if (options_buffer) {
    const void* buffer_start = env->GetDirectBufferAddress(options_buffer);
    DCHECK(buffer_start);
    const size_t buffer_size = env->GetDirectBufferCapacity(options_buffer);
    DCHECK_EQ(buffer_size, sizeof(MojoCreateDataPipeOptions));
    options = static_cast<const MojoCreateDataPipeOptions*>(buffer_start);
    DCHECK_EQ(options->struct_size, buffer_size);
  }
  MojoHandle handle1;
  MojoHandle handle2;
  MojoResult result = MojoCreateDataPipe(options, &handle1, &handle2);
  return Java_CoreImpl_newNativeCreationResult(env, result, handle1, handle2)
      .Release();
}

static jobject CreateSharedBuffer(JNIEnv* env,
                                  jobject jcaller,
                                  jobject options_buffer,
                                  jlong num_bytes) {
  const MojoCreateSharedBufferOptions* options = 0;
  if (options_buffer) {
    const void* buffer_start = env->GetDirectBufferAddress(options_buffer);
    DCHECK(buffer_start);
    const size_t buffer_size = env->GetDirectBufferCapacity(options_buffer);
    DCHECK_EQ(buffer_size, sizeof(MojoCreateSharedBufferOptions));
    options = static_cast<const MojoCreateSharedBufferOptions*>(buffer_start);
    DCHECK_EQ(options->struct_size, buffer_size);
  }
  MojoHandle handle;
  MojoResult result = MojoCreateSharedBuffer(options, num_bytes, &handle);
  return Java_CoreImpl_newNativeCreationResult(env, result, handle, 0)
      .Release();
}

static jint Close(JNIEnv* env, jobject jcaller, jint mojo_handle) {
  return MojoClose(mojo_handle);
}

static jint Wait(JNIEnv* env,
                 jobject jcaller,
                 jint mojo_handle,
                 jint signals,
                 jlong deadline) {
  return MojoWait(mojo_handle, signals, deadline);
}

static jint WriteMessage(JNIEnv* env,
                         jobject jcaller,
                         jint mojo_handle,
                         jobject bytes,
                         jint num_bytes,
                         jobject handles_buffer,
                         jint flags) {
  const void* buffer_start = 0;
  uint32_t buffer_size = 0;
  if (bytes) {
    buffer_start = env->GetDirectBufferAddress(bytes);
    DCHECK(buffer_start);
    DCHECK(env->GetDirectBufferCapacity(bytes) >= num_bytes);
    buffer_size = num_bytes;
  }
  const MojoHandle* handles = 0;
  uint32_t num_handles = 0;
  if (handles_buffer) {
    handles =
        static_cast<MojoHandle*>(env->GetDirectBufferAddress(handles_buffer));
    num_handles = env->GetDirectBufferCapacity(handles_buffer) / 4;
  }
  // Java code will handle invalidating handles if the write succeeded.
  return MojoWriteMessage(
      mojo_handle, buffer_start, buffer_size, handles, num_handles, flags);
}

static jobject ReadMessage(JNIEnv* env,
                           jobject jcaller,
                           jint mojo_handle,
                           jobject bytes,
                           jobject handles_buffer,
                           jint flags) {
  void* buffer_start = 0;
  uint32_t buffer_size = 0;
  if (bytes) {
    buffer_start = env->GetDirectBufferAddress(bytes);
    DCHECK(buffer_start);
    buffer_size = env->GetDirectBufferCapacity(bytes);
  }
  MojoHandle* handles = 0;
  uint32_t num_handles = 0;
  if (handles_buffer) {
    handles =
        static_cast<MojoHandle*>(env->GetDirectBufferAddress(handles_buffer));
    num_handles = env->GetDirectBufferCapacity(handles_buffer) / 4;
  }
  MojoResult result = MojoReadMessage(
      mojo_handle, buffer_start, &buffer_size, handles, &num_handles, flags);
  // Jave code will handle taking ownership of any received handle.
  return Java_CoreImpl_newReadMessageResult(
             env, result, buffer_size, num_handles).Release();
}

static jint ReadData(JNIEnv* env,
                     jobject jcaller,
                     jint mojo_handle,
                     jobject elements,
                     jint elements_capacity,
                     jint flags) {
  void* buffer_start = 0;
  uint32_t buffer_size = elements_capacity;
  if (elements) {
    buffer_start = env->GetDirectBufferAddress(elements);
    DCHECK(buffer_start);
    DCHECK(elements_capacity <= env->GetDirectBufferCapacity(elements));
  }
  MojoResult result =
      MojoReadData(mojo_handle, buffer_start, &buffer_size, flags);
  if (result < 0) {
    return result;
  }
  return buffer_size;
}

static jobject BeginReadData(JNIEnv* env,
                             jobject jcaller,
                             jint mojo_handle,
                             jint num_bytes,
                             jint flags) {
  void const* buffer = 0;
  uint32_t buffer_size = num_bytes;
  MojoResult result =
      MojoBeginReadData(mojo_handle, &buffer, &buffer_size, flags);
  jobject byte_buffer = 0;
  if (result == MOJO_RESULT_OK) {
    byte_buffer =
        env->NewDirectByteBuffer(const_cast<void*>(buffer), buffer_size);
  }
  return Java_CoreImpl_newNativeCodeAndBufferResult(env, result, byte_buffer)
      .Release();
}

static jint EndReadData(JNIEnv* env,
                        jobject jcaller,
                        jint mojo_handle,
                        jint num_bytes_read) {
  return MojoEndReadData(mojo_handle, num_bytes_read);
}

static jint WriteData(JNIEnv* env,
                      jobject jcaller,
                      jint mojo_handle,
                      jobject elements,
                      jint limit,
                      jint flags) {
  void* buffer_start = env->GetDirectBufferAddress(elements);
  DCHECK(buffer_start);
  DCHECK(limit <= env->GetDirectBufferCapacity(elements));
  uint32_t buffer_size = limit;
  MojoResult result =
      MojoWriteData(mojo_handle, buffer_start, &buffer_size, flags);
  if (result < 0) {
    return result;
  }
  return buffer_size;
}

static jobject BeginWriteData(JNIEnv* env,
                              jobject jcaller,
                              jint mojo_handle,
                              jint num_bytes,
                              jint flags) {
  void* buffer = 0;
  uint32_t buffer_size = num_bytes;
  MojoResult result =
      MojoBeginWriteData(mojo_handle, &buffer, &buffer_size, flags);
  jobject byte_buffer = 0;
  if (result == MOJO_RESULT_OK) {
    byte_buffer = env->NewDirectByteBuffer(buffer, buffer_size);
  }
  return Java_CoreImpl_newNativeCodeAndBufferResult(env, result, byte_buffer)
      .Release();
}

static jint EndWriteData(JNIEnv* env,
                         jobject jcaller,
                         jint mojo_handle,
                         jint num_bytes_written) {
  return MojoEndWriteData(mojo_handle, num_bytes_written);
}

static jobject Duplicate(JNIEnv* env,
                         jobject jcaller,
                         jint mojo_handle,
                         jobject options_buffer) {
  const MojoDuplicateBufferHandleOptions* options = 0;
  if (options_buffer) {
    const void* buffer_start = env->GetDirectBufferAddress(options_buffer);
    DCHECK(buffer_start);
    const size_t buffer_size = env->GetDirectBufferCapacity(options_buffer);
    DCHECK_EQ(buffer_size, sizeof(MojoDuplicateBufferHandleOptions));
    options =
        static_cast<const MojoDuplicateBufferHandleOptions*>(buffer_start);
    DCHECK_EQ(options->struct_size, buffer_size);
  }
  MojoHandle handle;
  MojoResult result = MojoDuplicateBufferHandle(mojo_handle, options, &handle);
  return Java_CoreImpl_newNativeCreationResult(env, result, handle, 0)
      .Release();
}

static jobject Map(JNIEnv* env,
                   jobject jcaller,
                   jint mojo_handle,
                   jlong offset,
                   jlong num_bytes,
                   jint flags) {
  void* buffer = 0;
  MojoResult result =
      MojoMapBuffer(mojo_handle, offset, num_bytes, &buffer, flags);
  jobject byte_buffer = 0;
  if (result == MOJO_RESULT_OK) {
    byte_buffer = env->NewDirectByteBuffer(buffer, num_bytes);
  }
  return Java_CoreImpl_newNativeCodeAndBufferResult(env, result, byte_buffer)
      .Release();
}

static int Unmap(JNIEnv* env, jobject jcaller, jobject buffer) {
  void* buffer_start = env->GetDirectBufferAddress(buffer);
  DCHECK(buffer_start);
  return MojoUnmapBuffer(buffer_start);
}

static jobject AsyncWait(JNIEnv* env,
                         jobject jcaller,
                         jint mojo_handle,
                         jint signals,
                         jlong deadline,
                         jobject callback) {
  AsyncWaitCallbackData* callback_data =
      new AsyncWaitCallbackData(env, jcaller, callback);
  MojoAsyncWaitID cancel_id;
  if (static_cast<MojoHandle>(mojo_handle) != MOJO_HANDLE_INVALID) {
    cancel_id = mojo::Environment::GetDefaultAsyncWaiter()->AsyncWait(
        mojo_handle, signals, deadline, AsyncWaitCallback, callback_data);
  } else {
    cancel_id = kInvalidHandleCancelID;
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(
            &AsyncWaitCallback, callback_data, MOJO_RESULT_INVALID_ARGUMENT));
  }
  base::android::ScopedJavaLocalRef<jobject> cancellable =
      Java_CoreImpl_newAsyncWaiterCancellableImpl(
          env, jcaller, cancel_id, reinterpret_cast<intptr_t>(callback_data));
  callback_data->cancellable.Reset(env, cancellable.obj());
  return cancellable.Release();
}

static void CancelAsyncWait(JNIEnv* env,
                            jobject jcaller,
                            jlong id,
                            jlong data_ptr) {
  if (id == 0) {
    // If |id| is |kInvalidHandleCancelID|, the async wait was done on an
    // invalid handle, so the AsyncWaitCallback will be called and will clear
    // the data_ptr.
    return;
  }
  scoped_ptr<AsyncWaitCallbackData> deleter(
      reinterpret_cast<AsyncWaitCallbackData*>(data_ptr));
  mojo::Environment::GetDefaultAsyncWaiter()->CancelWait(id);
}

bool RegisterCoreImpl(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace android
}  // namespace mojo
