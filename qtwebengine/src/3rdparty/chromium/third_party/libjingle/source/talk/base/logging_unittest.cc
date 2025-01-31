/*
 * libjingle
 * Copyright 2004--2011, Google Inc.
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

#include "talk/base/fileutils.h"
#include "talk/base/gunit.h"
#include "talk/base/logging.h"
#include "talk/base/pathutils.h"
#include "talk/base/stream.h"
#include "talk/base/thread.h"

namespace talk_base {

// Test basic logging operation. We should get the INFO log but not the VERBOSE.
// We should restore the correct global state at the end.
TEST(LogTest, SingleStream) {
  int sev = LogMessage::GetLogToStream(NULL);

  std::string str;
  StringStream stream(str);
  LogMessage::AddLogToStream(&stream, LS_INFO);
  EXPECT_EQ(LS_INFO, LogMessage::GetLogToStream(&stream));

  LOG(LS_INFO) << "INFO";
  LOG(LS_VERBOSE) << "VERBOSE";
  EXPECT_NE(std::string::npos, str.find("INFO"));
  EXPECT_EQ(std::string::npos, str.find("VERBOSE"));

  LogMessage::RemoveLogToStream(&stream);
  EXPECT_EQ(LogMessage::NO_LOGGING, LogMessage::GetLogToStream(&stream));

  EXPECT_EQ(sev, LogMessage::GetLogToStream(NULL));
}

// Test using multiple log streams. The INFO stream should get the INFO message,
// the VERBOSE stream should get the INFO and the VERBOSE.
// We should restore the correct global state at the end.
TEST(LogTest, MultipleStreams) {
  int sev = LogMessage::GetLogToStream(NULL);

  std::string str1, str2;
  StringStream stream1(str1), stream2(str2);
  LogMessage::AddLogToStream(&stream1, LS_INFO);
  LogMessage::AddLogToStream(&stream2, LS_VERBOSE);
  EXPECT_EQ(LS_INFO, LogMessage::GetLogToStream(&stream1));
  EXPECT_EQ(LS_VERBOSE, LogMessage::GetLogToStream(&stream2));

  LOG(LS_INFO) << "INFO";
  LOG(LS_VERBOSE) << "VERBOSE";

  EXPECT_NE(std::string::npos, str1.find("INFO"));
  EXPECT_EQ(std::string::npos, str1.find("VERBOSE"));
  EXPECT_NE(std::string::npos, str2.find("INFO"));
  EXPECT_NE(std::string::npos, str2.find("VERBOSE"));

  LogMessage::RemoveLogToStream(&stream2);
  LogMessage::RemoveLogToStream(&stream1);
  EXPECT_EQ(LogMessage::NO_LOGGING, LogMessage::GetLogToStream(&stream2));
  EXPECT_EQ(LogMessage::NO_LOGGING, LogMessage::GetLogToStream(&stream1));

  EXPECT_EQ(sev, LogMessage::GetLogToStream(NULL));
}

// Ensure we don't crash when adding/removing streams while threads are going.
// We should restore the correct global state at the end.
class LogThread : public Thread {
 public:
  virtual ~LogThread() {
    Stop();
  }

 private:
  void Run() {
    // LS_SENSITIVE to avoid cluttering up any real logging going on
    LOG(LS_SENSITIVE) << "LOG";
  }
};

TEST(LogTest, MultipleThreads) {
  int sev = LogMessage::GetLogToStream(NULL);

  LogThread thread1, thread2, thread3;
  thread1.Start();
  thread2.Start();
  thread3.Start();

  NullStream stream1, stream2, stream3;
  for (int i = 0; i < 1000; ++i) {
    LogMessage::AddLogToStream(&stream1, LS_INFO);
    LogMessage::AddLogToStream(&stream2, LS_VERBOSE);
    LogMessage::AddLogToStream(&stream3, LS_SENSITIVE);
    LogMessage::RemoveLogToStream(&stream1);
    LogMessage::RemoveLogToStream(&stream2);
    LogMessage::RemoveLogToStream(&stream3);
  }

  EXPECT_EQ(sev, LogMessage::GetLogToStream(NULL));
}


TEST(LogTest, WallClockStartTime) {
  uint32 time = LogMessage::WallClockStartTime();
  // Expect the time to be in a sensible range, e.g. > 2012-01-01.
  EXPECT_GT(time, 1325376000u);
}

// Test the time required to write 1000 80-character logs to an unbuffered file.
TEST(LogTest, Perf) {
  Pathname path;
  EXPECT_TRUE(Filesystem::GetTemporaryFolder(path, true, NULL));
  path.SetPathname(Filesystem::TempFilename(path, "ut"));

  FileStream stream;
  EXPECT_TRUE(stream.Open(path.pathname(), "wb", NULL));
  stream.DisableBuffering();
  LogMessage::AddLogToStream(&stream, LS_SENSITIVE);

  uint32 start = Time(), finish;
  std::string message('X', 80);
  for (int i = 0; i < 1000; ++i) {
    LOG(LS_SENSITIVE) << message;
  }
  finish = Time();

  LogMessage::RemoveLogToStream(&stream);
  stream.Close();
  Filesystem::DeleteFile(path);

  LOG(LS_INFO) << "Average log time: " << TimeDiff(finish, start) << " us";
}

}  // namespace talk_base
