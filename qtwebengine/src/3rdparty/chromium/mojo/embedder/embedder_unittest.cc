// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/embedder/embedder.h"

#include <string.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/synchronization/waitable_event.h"
#include "mojo/common/test/multiprocess_test_helper.h"
#include "mojo/embedder/platform_channel_pair.h"
#include "mojo/embedder/test_embedder.h"
#include "mojo/public/c/system/core.h"
#include "mojo/system/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace embedder {
namespace {

class ScopedTestChannel {
 public:
  // Creates a channel that lives on a given I/O thread (determined by the given
  // |TaskRunner|) attached to the given |platform_handle|. After construction,
  // |bootstrap_message_pipe()| gives the Mojo handle for the bootstrap message
  // pipe on this channel; it is up to the caller to close this handle.
  // Note: The I/O thread must outlive this object (and its message loop must
  // continue pumping messages while this object is alive).
  ScopedTestChannel(scoped_refptr<base::TaskRunner> io_thread_task_runner,
                    ScopedPlatformHandle platform_handle)
      : io_thread_task_runner_(io_thread_task_runner),
        bootstrap_message_pipe_(MOJO_HANDLE_INVALID),
        did_create_channel_event_(true, false),
        channel_info_(NULL) {
    bootstrap_message_pipe_ = CreateChannel(
        platform_handle.Pass(), io_thread_task_runner_,
        base::Bind(&ScopedTestChannel::DidCreateChannel,
                   base::Unretained(this)), NULL).release().value();
    CHECK_NE(bootstrap_message_pipe_, MOJO_HANDLE_INVALID);
  }

  // Destructor: Shuts down the channel. (As noted above, for this to happen,
  // the I/O thread must be alive and pumping messages.)
  ~ScopedTestChannel() {
    system::test::PostTaskAndWait(
        io_thread_task_runner_,
        FROM_HERE,
        base::Bind(&ScopedTestChannel::DestroyChannel, base::Unretained(this)));
  }

  // Waits for channel creation to be completed.
  void WaitForChannelCreationCompletion() {
    did_create_channel_event_.Wait();
  }

  MojoHandle bootstrap_message_pipe() const { return bootstrap_message_pipe_; }

  // Call only after |WaitForChannelCreationCompletion()|. Use only to check
  // that it's not null.
  const ChannelInfo* channel_info() const { return channel_info_; }

 private:
  void DidCreateChannel(ChannelInfo* channel_info) {
    CHECK(channel_info);
    CHECK(!channel_info_);
    channel_info_ = channel_info;
    did_create_channel_event_.Signal();
  }

  void DestroyChannel() {
    CHECK(channel_info_);
    DestroyChannelOnIOThread(channel_info_);
    channel_info_ = NULL;
  }

  scoped_refptr<base::TaskRunner> io_thread_task_runner_;

  // Valid from creation until whenever it gets closed (by the "owner" of this
  // object).
  // Note: We don't want use the C++ wrappers here, since we want to test the
  // API at the lowest level.
  MojoHandle bootstrap_message_pipe_;

  // Set after channel creation has been completed (i.e., the callback to
  // |CreateChannel()| has been called).
  base::WaitableEvent did_create_channel_event_;

  // Valid after channel creation completion until destruction.
  ChannelInfo* channel_info_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTestChannel);
};

class EmbedderTest : public testing::Test {
 public:
  EmbedderTest() : test_io_thread_(system::test::TestIOThread::kAutoStart) {}
  virtual ~EmbedderTest() {}

 protected:
  system::test::TestIOThread* test_io_thread() { return &test_io_thread_; }

 private:
  system::test::TestIOThread test_io_thread_;

  DISALLOW_COPY_AND_ASSIGN(EmbedderTest);
};

TEST_F(EmbedderTest, ChannelsBasic) {
  Init();

  {
    PlatformChannelPair channel_pair;
    ScopedTestChannel server_channel(test_io_thread()->task_runner(),
                                     channel_pair.PassServerHandle());
    MojoHandle server_mp = server_channel.bootstrap_message_pipe();
    EXPECT_NE(server_mp, MOJO_HANDLE_INVALID);
    ScopedTestChannel client_channel(test_io_thread()->task_runner(),
                                     channel_pair.PassClientHandle());
    MojoHandle client_mp = client_channel.bootstrap_message_pipe();
    EXPECT_NE(client_mp, MOJO_HANDLE_INVALID);

    // We can write to a message pipe handle immediately.
    const char kHello[] = "hello";
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWriteMessage(server_mp, kHello,
                               static_cast<uint32_t>(sizeof(kHello)), NULL, 0,
                               MOJO_WRITE_MESSAGE_FLAG_NONE));

    // Now wait for the other side to become readable.
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWait(client_mp, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));

    char buffer[1000] = {};
    uint32_t num_bytes = static_cast<uint32_t>(sizeof(buffer));
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoReadMessage(client_mp, buffer, &num_bytes, NULL, NULL,
                              MOJO_READ_MESSAGE_FLAG_NONE));
    EXPECT_EQ(sizeof(kHello), num_bytes);
    EXPECT_STREQ(kHello, buffer);

    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(server_mp));
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(client_mp));

    // By this point, these waits should basically be no-ops (since we've waited
    // for the client message pipe to become readable, which implies that both
    // the server and client channels were completely created).
    server_channel.WaitForChannelCreationCompletion();
    client_channel.WaitForChannelCreationCompletion();
    EXPECT_TRUE(server_channel.channel_info() != NULL);
    EXPECT_TRUE(client_channel.channel_info() != NULL);
  }

  EXPECT_TRUE(test::Shutdown());
}

TEST_F(EmbedderTest, ChannelsHandlePassing) {
  Init();

  {
    PlatformChannelPair channel_pair;
    ScopedTestChannel server_channel(test_io_thread()->task_runner(),
                                     channel_pair.PassServerHandle());
    MojoHandle server_mp = server_channel.bootstrap_message_pipe();
    EXPECT_NE(server_mp, MOJO_HANDLE_INVALID);
    ScopedTestChannel client_channel(test_io_thread()->task_runner(),
                                     channel_pair.PassClientHandle());
    MojoHandle client_mp = client_channel.bootstrap_message_pipe();
    EXPECT_NE(client_mp, MOJO_HANDLE_INVALID);

    MojoHandle h0, h1;
    EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(NULL, &h0, &h1));

    // Write a message to |h0| (attaching nothing).
    const char kHello[] = "hello";
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWriteMessage(h0, kHello,
                               static_cast<uint32_t>(sizeof(kHello)), NULL, 0,
                               MOJO_WRITE_MESSAGE_FLAG_NONE));

    // Write one message to |server_mp|, attaching |h1|.
    const char kWorld[] = "world!!!";
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWriteMessage(server_mp, kWorld,
                               static_cast<uint32_t>(sizeof(kWorld)), &h1, 1,
                               MOJO_WRITE_MESSAGE_FLAG_NONE));
    h1 = MOJO_HANDLE_INVALID;

    // Write another message to |h0|.
    const char kFoo[] = "foo";
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWriteMessage(h0, kFoo,
                               static_cast<uint32_t>(sizeof(kFoo)), NULL, 0,
                               MOJO_WRITE_MESSAGE_FLAG_NONE));

    // Wait for |client_mp| to become readable.
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWait(client_mp, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));

    // Read a message from |client_mp|.
    char buffer[1000] = {};
    uint32_t num_bytes = static_cast<uint32_t>(sizeof(buffer));
    MojoHandle handles[10] = {};
    uint32_t num_handles = arraysize(handles);
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoReadMessage(client_mp, buffer, &num_bytes, handles,
                              &num_handles, MOJO_READ_MESSAGE_FLAG_NONE));
    EXPECT_EQ(sizeof(kWorld), num_bytes);
    EXPECT_STREQ(kWorld, buffer);
    EXPECT_EQ(1u, num_handles);
    EXPECT_NE(handles[0], MOJO_HANDLE_INVALID);
    h1 = handles[0];

    // Wait for |h1| to become readable.
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWait(h1, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));

    // Read a message from |h1|.
    memset(buffer, 0, sizeof(buffer));
    num_bytes = static_cast<uint32_t>(sizeof(buffer));
    memset(handles, 0, sizeof(handles));
    num_handles = arraysize(handles);
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoReadMessage(h1, buffer, &num_bytes, handles, &num_handles,
                              MOJO_READ_MESSAGE_FLAG_NONE));
    EXPECT_EQ(sizeof(kHello), num_bytes);
    EXPECT_STREQ(kHello, buffer);
    EXPECT_EQ(0u, num_handles);

    // Wait for |h1| to become readable (again).
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWait(h1, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));

    // Read the second message from |h1|.
    memset(buffer, 0, sizeof(buffer));
    num_bytes = static_cast<uint32_t>(sizeof(buffer));
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoReadMessage(h1, buffer, &num_bytes, NULL, NULL,
                              MOJO_READ_MESSAGE_FLAG_NONE));
    EXPECT_EQ(sizeof(kFoo), num_bytes);
    EXPECT_STREQ(kFoo, buffer);

    // Write a message to |h1|.
    const char kBarBaz[] = "barbaz";
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWriteMessage(h1, kBarBaz,
                               static_cast<uint32_t>(sizeof(kBarBaz)), NULL, 0,
                               MOJO_WRITE_MESSAGE_FLAG_NONE));

    // Wait for |h0| to become readable.
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWait(h0, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));

    // Read a message from |h0|.
    memset(buffer, 0, sizeof(buffer));
    num_bytes = static_cast<uint32_t>(sizeof(buffer));
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoReadMessage(h0, buffer, &num_bytes, NULL, NULL,
                              MOJO_READ_MESSAGE_FLAG_NONE));
    EXPECT_EQ(sizeof(kBarBaz), num_bytes);
    EXPECT_STREQ(kBarBaz, buffer);

    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(server_mp));
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(client_mp));
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h0));
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h1));

    server_channel.WaitForChannelCreationCompletion();
    client_channel.WaitForChannelCreationCompletion();
    EXPECT_TRUE(server_channel.channel_info() != NULL);
    EXPECT_TRUE(client_channel.channel_info() != NULL);
  }

  EXPECT_TRUE(test::Shutdown());
}

// The sequence of messages sent is:
//       server_mp   client_mp   mp0         mp1         mp2         mp3
//   1.  "hello"
//   2.              "world!"
//   3.                          "FOO"
//   4.  "Bar"+mp1
//   5.  (close)
//   6.              (close)
//   7.                                                              "baz"
//   8.                                                              (closed)
//   9.                                      "quux"+mp2
//  10.                          (close)
//  11.                                      (wait/cl.)
//  12.                                                  (wait/cl.)
TEST_F(EmbedderTest, MultiprocessChannels) {
  Init();
  mojo::test::MultiprocessTestHelper multiprocess_test_helper;
  multiprocess_test_helper.StartChild("MultiprocessChannelsClient");

  {
    ScopedTestChannel server_channel(
        test_io_thread()->task_runner(),
        multiprocess_test_helper.server_platform_handle.Pass());
    MojoHandle server_mp = server_channel.bootstrap_message_pipe();
    EXPECT_NE(server_mp, MOJO_HANDLE_INVALID);
    server_channel.WaitForChannelCreationCompletion();
    EXPECT_TRUE(server_channel.channel_info() != NULL);

    // 1. Write a message to |server_mp| (attaching nothing).
    const char kHello[] = "hello";
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWriteMessage(server_mp, kHello,
                               static_cast<uint32_t>(sizeof(kHello)), NULL, 0,
                               MOJO_WRITE_MESSAGE_FLAG_NONE));

    // TODO(vtl): If the scope were ended immediately here (maybe after closing
    // |server_mp|), we die with a fatal error in |Channel::HandleLocalError()|.

    // 2. Read a message from |server_mp|.
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWait(server_mp, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));
    char buffer[1000] = {};
    uint32_t num_bytes = static_cast<uint32_t>(sizeof(buffer));
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoReadMessage(server_mp, buffer, &num_bytes, NULL, NULL,
                              MOJO_READ_MESSAGE_FLAG_NONE));
    const char kWorld[] = "world!";
    EXPECT_EQ(sizeof(kWorld), num_bytes);
    EXPECT_STREQ(kWorld, buffer);

    // Create a new message pipe (endpoints |mp0| and |mp1|).
    MojoHandle mp0, mp1;
    EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(NULL, &mp0, &mp1));

    // 3. Write something to |mp0|.
    const char kFoo[] = "FOO";
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWriteMessage(mp0, kFoo,
                               static_cast<uint32_t>(sizeof(kFoo)), NULL, 0,
                               MOJO_WRITE_MESSAGE_FLAG_NONE));

    // 4. Write a message to |server_mp|, attaching |mp1|.
    const char kBar[] = "Bar";
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWriteMessage(server_mp, kBar,
                               static_cast<uint32_t>(sizeof(kBar)), &mp1, 1,
                               MOJO_WRITE_MESSAGE_FLAG_NONE));
    mp1 = MOJO_HANDLE_INVALID;

    // 5. Close |server_mp|.
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(server_mp));

    // 9. Read a message from |mp0|, which should have |mp2| attached.
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWait(mp0, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));
    memset(buffer, 0, sizeof(buffer));
    num_bytes = static_cast<uint32_t>(sizeof(buffer));
    MojoHandle mp2 = MOJO_HANDLE_INVALID;
    uint32_t num_handles = 1;
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoReadMessage(mp0, buffer, &num_bytes, &mp2, &num_handles,
                              MOJO_READ_MESSAGE_FLAG_NONE));
    const char kQuux[] = "quux";
    EXPECT_EQ(sizeof(kQuux), num_bytes);
    EXPECT_STREQ(kQuux, buffer);
    EXPECT_EQ(1u, num_handles);
    EXPECT_NE(mp2, MOJO_HANDLE_INVALID);

    // 7. Read a message from |mp2|.
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWait(mp2, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));
    memset(buffer, 0, sizeof(buffer));
    num_bytes = static_cast<uint32_t>(sizeof(buffer));
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoReadMessage(mp2, buffer, &num_bytes, NULL, NULL,
                              MOJO_READ_MESSAGE_FLAG_NONE));
    const char kBaz[] = "baz";
    EXPECT_EQ(sizeof(kBaz), num_bytes);
    EXPECT_STREQ(kBaz, buffer);

    // 10. Close |mp0|.
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(mp0));

    // 12. Wait on |mp2| (which should eventually fail) and then close it.
// TODO(vtl): crbug.com/351768
#if 0
    EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
              MojoWait(mp2, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));
#endif
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(mp2));
  }

  EXPECT_TRUE(multiprocess_test_helper.WaitForChildTestShutdown());
  EXPECT_TRUE(test::Shutdown());
}

MOJO_MULTIPROCESS_TEST_CHILD_TEST(MultiprocessChannelsClient) {
  embedder::ScopedPlatformHandle client_platform_handle =
      mojo::test::MultiprocessTestHelper::client_platform_handle.Pass();
  EXPECT_TRUE(client_platform_handle.is_valid());

  system::test::TestIOThread
      test_io_thread(system::test::TestIOThread::kAutoStart);
  Init();

  {
    ScopedTestChannel client_channel(test_io_thread.task_runner(),
                                     client_platform_handle.Pass());
    MojoHandle client_mp = client_channel.bootstrap_message_pipe();
    EXPECT_NE(client_mp, MOJO_HANDLE_INVALID);
    client_channel.WaitForChannelCreationCompletion();
    CHECK(client_channel.channel_info() != NULL);

    // 1. Read the first message from |client_mp|.
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWait(client_mp, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));
    char buffer[1000] = {};
    uint32_t num_bytes = static_cast<uint32_t>(sizeof(buffer));
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoReadMessage(client_mp, buffer, &num_bytes, NULL, NULL,
                              MOJO_READ_MESSAGE_FLAG_NONE));
    const char kHello[] = "hello";
    EXPECT_EQ(sizeof(kHello), num_bytes);
    EXPECT_STREQ(kHello, buffer);

    // 2. Write a message to |client_mp| (attaching nothing).
    const char kWorld[] = "world!";
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWriteMessage(client_mp, kWorld,
                               static_cast<uint32_t>(sizeof(kWorld)), NULL, 0,
                               MOJO_WRITE_MESSAGE_FLAG_NONE));

    // 4. Read a message from |client_mp|, which should have |mp1| attached.
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWait(client_mp, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));
    // TODO(vtl): If the scope were to end here (and |client_mp| closed), we'd
    // die (again due to |Channel::HandleLocalError()|).
    memset(buffer, 0, sizeof(buffer));
    num_bytes = static_cast<uint32_t>(sizeof(buffer));
    MojoHandle mp1 = MOJO_HANDLE_INVALID;
    uint32_t num_handles = 1;
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoReadMessage(client_mp, buffer, &num_bytes, &mp1, &num_handles,
                              MOJO_READ_MESSAGE_FLAG_NONE));
    const char kBar[] = "Bar";
    EXPECT_EQ(sizeof(kBar), num_bytes);
    EXPECT_STREQ(kBar, buffer);
    EXPECT_EQ(1u, num_handles);
    EXPECT_NE(mp1, MOJO_HANDLE_INVALID);
    // TODO(vtl): If the scope were to end here (and the two handles closed),
    // we'd die due to |Channel::RunRemoteMessagePipeEndpoint()| not handling
    // write errors (assuming the parent had closed the pipe).

    // 6. Close |client_mp|.
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(client_mp));

    // Create a new message pipe (endpoints |mp2| and |mp3|).
    MojoHandle mp2, mp3;
    EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(NULL, &mp2, &mp3));

    // 7. Write a message to |mp3|.
    const char kBaz[] = "baz";
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWriteMessage(mp3, kBaz,
                               static_cast<uint32_t>(sizeof(kBaz)), NULL, 0,
                               MOJO_WRITE_MESSAGE_FLAG_NONE));

    // 8. Close |mp3|.
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(mp3));

    // 9. Write a message to |mp1|, attaching |mp2|.
    const char kQuux[] = "quux";
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWriteMessage(mp1, kQuux,
                               static_cast<uint32_t>(sizeof(kQuux)), &mp2, 1,
                               MOJO_WRITE_MESSAGE_FLAG_NONE));
    mp2 = MOJO_HANDLE_INVALID;

    // 3. Read a message from |mp1|.
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWait(mp1, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));
    memset(buffer, 0, sizeof(buffer));
    num_bytes = static_cast<uint32_t>(sizeof(buffer));
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoReadMessage(mp1, buffer, &num_bytes, NULL, NULL,
                              MOJO_READ_MESSAGE_FLAG_NONE));
    const char kFoo[] = "FOO";
    EXPECT_EQ(sizeof(kFoo), num_bytes);
    EXPECT_STREQ(kFoo, buffer);

    // 11. Wait on |mp1| (which should eventually fail) and then close it.
    EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
              MojoWait(mp1, MOJO_HANDLE_SIGNAL_READABLE,
                       MOJO_DEADLINE_INDEFINITE));
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(mp1));
  }

  EXPECT_TRUE(test::Shutdown());
}

// TODO(vtl): Test immediate write & close.
// TODO(vtl): Test broken-connection cases.

}  // namespace
}  // namespace embedder
}  // namespace mojo
