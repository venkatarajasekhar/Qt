// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cdm/ppapi/cdm_file_io_test.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"

namespace media {

#define FILE_IO_DVLOG(level) DVLOG(level) << "File IO Test: "

const uint8 kData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
const uint32 kDataSize = arraysize(kData);

const uint8 kBigData[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
                           0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
                           0x00 };
const uint32 kBigDataSize = arraysize(kBigData);

// Must be > kReadSize in cdm_file_io_impl.cc.
const uint32 kLargeDataSize = 9 * 1024 + 7;

// Macros to help add test cases/steps.
#define START_TEST_CASE(test_name)                                     \
  do {                                                                 \
    FileIOTest test_case(create_file_io_cb_, "FileIOTest." test_name); \
    CREATE_FILE_IO  // Create FileIO for each test case.

#define ADD_TEST_STEP(type, status, data, data_size)                   \
    test_case.AddTestStep(FileIOTest::type, cdm::FileIOClient::status, \
                          (data), (data_size));

#define END_TEST_CASE                 \
    remaining_tests_.push_back(test_case); \
  } while(0);

#define CREATE_FILE_IO \
  ADD_TEST_STEP(ACTION_CREATE, kSuccess, NULL, 0)

#define OPEN_FILE \
  ADD_TEST_STEP(ACTION_OPEN, kSuccess, NULL, 0)

#define EXPECT_FILE_OPENED(status) \
  ADD_TEST_STEP(RESULT_OPEN, status, NULL, 0)

#define READ_FILE \
  ADD_TEST_STEP(ACTION_READ, kSuccess, NULL, 0)

#define EXPECT_FILE_READ(status, data, data_size) \
  ADD_TEST_STEP(RESULT_READ, status, data, data_size)

#define WRITE_FILE(data, data_size) \
  ADD_TEST_STEP(ACTION_WRITE, kSuccess, data, data_size)

#define EXPECT_FILE_WRITTEN(status) \
  ADD_TEST_STEP(RESULT_WRITE, status, NULL, 0)

#define CLOSE_FILE \
  ADD_TEST_STEP(ACTION_CLOSE, kSuccess, NULL, 0)

// FileIOTestRunner implementation.

FileIOTestRunner::FileIOTestRunner(const CreateFileIOCB& create_file_io_cb)
    : create_file_io_cb_(create_file_io_cb),
      total_num_tests_(0),
      num_passed_tests_(0) {
  // Generate |large_data_|.
  large_data_.resize(kLargeDataSize);
  for (size_t i = 0; i < kLargeDataSize; ++i)
    large_data_[i] = i % kuint8max;

  AddTests();
}

FileIOTestRunner::~FileIOTestRunner() {
  if (remaining_tests_.empty())
    return;

  DCHECK_LT(num_passed_tests_, total_num_tests_);
  FILE_IO_DVLOG(1) << "Not Finished (probably due to timeout). "
                   << num_passed_tests_ << " passed in "
                   << total_num_tests_ << " tests.";
}

// Note: Consecutive expectations (EXPECT*) can happen in any order.
void FileIOTestRunner::AddTests() {
  START_TEST_CASE("ReadBeforeOpeningFile")
    READ_FILE
    EXPECT_FILE_READ(kError, NULL, 0)
  END_TEST_CASE

  START_TEST_CASE("WriteBeforeOpeningFile")
    WRITE_FILE(kData, kDataSize)
    EXPECT_FILE_WRITTEN(kError)
  END_TEST_CASE

  START_TEST_CASE("ReadBeforeFileOpened")
    OPEN_FILE
    READ_FILE
    EXPECT_FILE_OPENED(kSuccess)
    EXPECT_FILE_READ(kError, NULL, 0)
  END_TEST_CASE

  START_TEST_CASE("WriteBeforeFileOpened")
    OPEN_FILE
    WRITE_FILE(kData, kDataSize)
    EXPECT_FILE_WRITTEN(kError)
    EXPECT_FILE_OPENED(kSuccess)
  END_TEST_CASE

  START_TEST_CASE("ReadDuringPendingRead")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(kData, kDataSize)
    EXPECT_FILE_WRITTEN(kSuccess)
    READ_FILE
    READ_FILE
    EXPECT_FILE_READ(kInUse, NULL, 0)
    EXPECT_FILE_READ(kSuccess, kData, kDataSize)
  END_TEST_CASE

  START_TEST_CASE("ReadDuringPendingWrite")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(kData, kDataSize)
    READ_FILE
    EXPECT_FILE_READ(kInUse, NULL, 0)
    EXPECT_FILE_WRITTEN(kSuccess)
  END_TEST_CASE

  START_TEST_CASE("WriteDuringPendingRead")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    READ_FILE
    WRITE_FILE(kData, kDataSize)
    EXPECT_FILE_WRITTEN(kInUse)
    EXPECT_FILE_READ(kSuccess, NULL, 0)
  END_TEST_CASE

  START_TEST_CASE("WriteDuringPendingWrite")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(kData, kDataSize)
    WRITE_FILE(kBigData, kBigDataSize)
    EXPECT_FILE_WRITTEN(kInUse)
    EXPECT_FILE_WRITTEN(kSuccess)
  END_TEST_CASE

  START_TEST_CASE("ReadEmptyFile")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    READ_FILE
    EXPECT_FILE_READ(kSuccess, NULL, 0)
  END_TEST_CASE

  START_TEST_CASE("WriteAndRead")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(kData, kDataSize)
    EXPECT_FILE_WRITTEN(kSuccess)
    READ_FILE
    EXPECT_FILE_READ(kSuccess, kData, kDataSize)
  END_TEST_CASE

  START_TEST_CASE("WriteZeroBytes")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(NULL, 0)
    EXPECT_FILE_WRITTEN(kSuccess)
    READ_FILE
    EXPECT_FILE_READ(kSuccess, NULL, 0)
  END_TEST_CASE

  START_TEST_CASE("WriteAndReadLargeData")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(&large_data_[0], kLargeDataSize)
    EXPECT_FILE_WRITTEN(kSuccess)
    READ_FILE
    EXPECT_FILE_READ(kSuccess, &large_data_[0], kLargeDataSize)
  END_TEST_CASE

  START_TEST_CASE("OverwriteZeroBytes")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(kData, kDataSize)
    EXPECT_FILE_WRITTEN(kSuccess)
    READ_FILE
    EXPECT_FILE_READ(kSuccess, kData, kDataSize)
    WRITE_FILE(NULL, 0)
    EXPECT_FILE_WRITTEN(kSuccess)
    READ_FILE
    EXPECT_FILE_READ(kSuccess, NULL, 0)
  END_TEST_CASE

  START_TEST_CASE("OverwriteWithSmallerData")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(kBigData, kBigDataSize)
    EXPECT_FILE_WRITTEN(kSuccess)
    WRITE_FILE(kData, kDataSize)
    EXPECT_FILE_WRITTEN(kSuccess)
    READ_FILE
    EXPECT_FILE_READ(kSuccess, kData, kDataSize)
  END_TEST_CASE

  START_TEST_CASE("OverwriteWithLargerData")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(kData, kDataSize)
    EXPECT_FILE_WRITTEN(kSuccess)
    WRITE_FILE(kBigData, kBigDataSize)
    EXPECT_FILE_WRITTEN(kSuccess)
    READ_FILE
    EXPECT_FILE_READ(kSuccess, kBigData, kBigDataSize)
  END_TEST_CASE

  START_TEST_CASE("ReadExistingFile")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(kData, kDataSize)
    EXPECT_FILE_WRITTEN(kSuccess)
    CLOSE_FILE
    CREATE_FILE_IO
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    READ_FILE
    EXPECT_FILE_READ(kSuccess, kData, kDataSize)
  END_TEST_CASE

  START_TEST_CASE("ReopenFileInTheSameFileIO")
    OPEN_FILE
    OPEN_FILE
    EXPECT_FILE_OPENED(kError)  // The second Open() failed.
    EXPECT_FILE_OPENED(kSuccess)  // The first Open() succeeded.
  END_TEST_CASE

  START_TEST_CASE("ReopenFileInSeparateFileIO")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(kData, kDataSize)
    EXPECT_FILE_WRITTEN(kSuccess)
    CREATE_FILE_IO  // Create a second FileIO without closing the first one.
    OPEN_FILE
    EXPECT_FILE_OPENED(kInUse)
  END_TEST_CASE

  START_TEST_CASE("CloseAfterCreation")
    CLOSE_FILE
  END_TEST_CASE

  START_TEST_CASE("CloseDuringPendingOpen")
    OPEN_FILE
    CLOSE_FILE
  END_TEST_CASE

  START_TEST_CASE("CloseDuringPendingWrite")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(kData, kDataSize)
    CLOSE_FILE
  END_TEST_CASE

  START_TEST_CASE("CloseDuringPendingRead")
    OPEN_FILE
    EXPECT_FILE_OPENED(kSuccess)
    WRITE_FILE(kData, kDataSize)
    EXPECT_FILE_WRITTEN(kSuccess)
    READ_FILE
    CLOSE_FILE
  END_TEST_CASE
}

void FileIOTestRunner::RunAllTests(const CompletionCB& completion_cb) {
  completion_cb_ = completion_cb;
  total_num_tests_ = remaining_tests_.size();
  RunNextTest();
}

void FileIOTestRunner::RunNextTest() {
  if (remaining_tests_.empty()) {
    FILE_IO_DVLOG(1) << num_passed_tests_ << " passed and "
                     << (total_num_tests_ - num_passed_tests_) << " failed in "
                     << total_num_tests_ << " tests.";
    bool success = (num_passed_tests_ == total_num_tests_);
    base::ResetAndReturn(&completion_cb_).Run(success);
    return;
  }

  remaining_tests_.front().Run(
      base::Bind(&FileIOTestRunner::OnTestComplete, base::Unretained(this)));
}

void FileIOTestRunner::OnTestComplete(bool success) {
  if (success)
    num_passed_tests_++;
  remaining_tests_.pop_front();
  RunNextTest();
}

// FileIOTest implementation.

FileIOTest::FileIOTest(const CreateFileIOCB& create_file_io_cb,
                       const std::string& test_name)
    : create_file_io_cb_(create_file_io_cb),
      test_name_(test_name) {}

FileIOTest::~FileIOTest() {}

void FileIOTest::AddTestStep(
    StepType type, Status status, const uint8* data, uint32 data_size) {
  test_steps_.push_back(TestStep(type, status, data, data_size));
}

void FileIOTest::Run(const CompletionCB& completion_cb) {
  FILE_IO_DVLOG(3) << "Run " << test_name_;
  completion_cb_ = completion_cb;
  DCHECK(!test_steps_.empty() && !IsResult(test_steps_.front()));
  RunNextStep();
}

void FileIOTest::OnOpenComplete(Status status) {
  OnResult(TestStep(RESULT_OPEN, status, NULL, 0));
}

void FileIOTest::OnReadComplete(Status status,
                                const uint8_t* data,
                                uint32_t data_size) {
  OnResult(TestStep(RESULT_READ, status, data, data_size));
}

void FileIOTest::OnWriteComplete(Status status) {
  OnResult(TestStep(RESULT_WRITE, status, NULL, 0));
}

bool FileIOTest::IsResult(const TestStep& test_step) {
  switch (test_step.type) {
    case RESULT_OPEN:
    case RESULT_READ:
    case RESULT_WRITE:
      return true;
    case ACTION_CREATE:
    case ACTION_OPEN:
    case ACTION_READ:
    case ACTION_WRITE:
    case ACTION_CLOSE:
      return false;
  }
  NOTREACHED();
  return false;
}

bool FileIOTest::MatchesResult(const TestStep& a, const TestStep& b) {
  DCHECK(IsResult(a) && IsResult(b));
  if (a.type != b.type || a.status != b.status)
    return false;

  if (a.type != RESULT_READ || a.status != cdm::FileIOClient::kSuccess)
    return true;

  return (a.data_size == a.data_size &&
          std::equal(a.data, a.data + a.data_size, b.data));
}

void FileIOTest::RunNextStep() {
  // Run all actions in the current action group.
  while (!test_steps_.empty()) {
    // Start to wait for test results when the next step is a test result.
    if (IsResult(test_steps_.front()))
      return;

    TestStep test_step = test_steps_.front();
    test_steps_.pop_front();

    cdm::FileIO* file_io = file_io_stack_.empty()? NULL : file_io_stack_.top();

    switch (test_step.type) {
      case ACTION_CREATE:
        file_io = create_file_io_cb_.Run(this);
        if (!file_io) {
          FILE_IO_DVLOG(3) << "Cannot create FileIO object.";
          OnTestComplete(false);
          return;
        }
        file_io_stack_.push(file_io);
        break;
      case ACTION_OPEN:
        // Use test name as the test file name.
        file_io->Open(test_name_.data(), test_name_.size());
        break;
      case ACTION_READ:
        file_io->Read();
        break;
      case ACTION_WRITE:
        file_io->Write(test_step.data, test_step.data_size);
        break;
      case ACTION_CLOSE:
        file_io->Close();
        file_io_stack_.pop();
        break;
      default:
        NOTREACHED();
    }
  }

  OnTestComplete(true);
}

void FileIOTest::OnResult(const TestStep& result) {
  DCHECK(IsResult(result));
  if (!CheckResult(result)) {
    OnTestComplete(false);
    return;
  }

  RunNextStep();
}

bool FileIOTest::CheckResult(const TestStep& result) {
  if (test_steps_.empty() || !IsResult(test_steps_.front()))
    return false;

  // If there are multiple results expected, the order does not matter.
  std::list<TestStep>::iterator iter = test_steps_.begin();
  for (; iter != test_steps_.end(); ++iter) {
    if (!IsResult(*iter))
      return false;

    if (!MatchesResult(*iter, result))
      continue;

    test_steps_.erase(iter);
    return true;
  }

  return false;
}

void FileIOTest::OnTestComplete(bool success) {
  while (!file_io_stack_.empty()) {
    file_io_stack_.top()->Close();
    file_io_stack_.pop();
  }
  FILE_IO_DVLOG(3) << test_name_ << (success ? " PASSED" : " FAILED");
  base::ResetAndReturn(&completion_cb_).Run(success);
}

}  // namespace media
