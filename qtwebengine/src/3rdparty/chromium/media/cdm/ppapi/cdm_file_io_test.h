// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CDM_PPAPI_CDM_FILE_IO_TEST_H_
#define MEDIA_CDM_PPAPI_CDM_FILE_IO_TEST_H_

#include <list>
#include <stack>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "media/cdm/ppapi/api/content_decryption_module.h"

namespace media {

typedef base::Callback<void(bool success)> CompletionCB;
typedef base::Callback<cdm::FileIO*(cdm::FileIOClient* client)> CreateFileIOCB;

// A customizable test class that tests cdm::FileIO implementation.
// - To create a test, call AddTestStep() to add a test step. A test step can be
//   either an action to make (use ACTION_* types), or a result to verify (use
//   RESULT_* types).
// - To run the test, simply call Run() with a completion callback. The result
//   will be reported in the completion callback when the test is finished.
//
// The following rules apply to the test steps:
// - Test steps are ordered (with the exception that results in a result group
//   is not ordered).
// - Consecutive action steps form an "action group". Consecutively result
//   steps form a "result group". An action group is followed by a result
//   group and vice versa.
// - A test must start with an action group.
// - To process an action group, the test runner runs (and clears) all steps
//   in the group in the order they were added. Then it waits for test
//   results.
// - When a cdm::FileIOClient method is called, the test runner compares the
//   result with all results in the current result group. If no result in that
//   group matches the test result, the test fails. Otherwise, the matching
//   result is cleared from the group. If the group is empty, the test runner
//   starts to process the next action group. Otherwise, the test runner keeps
//   waiting for the next test result.
// - After all steps are cleared, the test passes.
class FileIOTest : public cdm::FileIOClient {
 public:
  // Types of allowed test steps:
  // - ACTION_* specifies the next step to test.
  // - RESULT_* specifies the expected result of the previous step(s).
  enum StepType {
    ACTION_CREATE,
    ACTION_OPEN,  // |test_name_| will be used used as the file name to open.
    RESULT_OPEN,
    ACTION_READ,
    RESULT_READ,
    ACTION_WRITE,
    RESULT_WRITE,
    ACTION_CLOSE  // If ACTION_CLOSE is not specified, FileIO::Close() will be
                  // automatically called at the end of the test.
  };

  FileIOTest(const CreateFileIOCB& create_file_io_cb,
             const std::string& test_name);
  ~FileIOTest();

  // Adds a test step in this test. |this| object doesn't take the ownership of
  // |data|, which should be valid throughout the lifetime of |this| object.
  void AddTestStep(
      StepType type, Status status, const uint8* data, uint32 data_size);

  // Runs this test case and returns the test result through |completion_cb|.
  void Run(const CompletionCB& completion_cb);

 private:
  struct TestStep {
    // |this| object doesn't take the ownership of |data|, which should be valid
    // throughout the lifetime of |this| object.
    TestStep(StepType type, Status status, const uint8* data, uint32 data_size)
        : type(type), status(status), data(data), data_size(data_size) {}

    StepType type;

    // Expected status for RESULT* steps.
    Status status;

    // Data to write in ACTION_WRITE, or read data in RESULT_READ.
    const uint8* data;
    uint32 data_size;
  };

  // Returns whether |test_step| is a RESULT_* step.
  static bool IsResult(const TestStep& test_step);

  // Returns whether two results match.
  static bool MatchesResult(const TestStep& a, const TestStep& b);

  // cdm::FileIOClient implementation.
  virtual void OnOpenComplete(Status status) OVERRIDE;
  virtual void OnReadComplete(Status status,
                              const uint8_t* data,
                              uint32_t data_size) OVERRIDE;
  virtual void OnWriteComplete(Status status) OVERRIDE;

  // Runs the next step in this test case.
  void RunNextStep();

  void OnResult(const TestStep& result);

  // Checks whether the test result matches this step. This can only be called
  // when this step is a RESULT_* step.
  bool CheckResult(const TestStep& result);

  void OnTestComplete(bool success);

  CreateFileIOCB create_file_io_cb_;
  CompletionCB completion_cb_;

  std::string test_name_;
  std::list<TestStep> test_steps_;

  // All opened cdm::FileIO objects. We keep multiple cdm::FileIO objects open
  // so that we can test multiple cdm::FileIO objects accessing the same file.
  // In the current implementation, all ACTION_* are performed on the latest
  // opened cdm::FileIO object, hence the stack.
  std::stack<cdm::FileIO*> file_io_stack_;
};

// Tests cdm::FileIO implementation.
class FileIOTestRunner {
 public:
  explicit FileIOTestRunner(const CreateFileIOCB& create_file_io_cb);
  ~FileIOTestRunner();

  void AddTests();

  // Run all tests. When tests are completed, the result will be reported in the
  // |completion_cb|.
  void RunAllTests(const CompletionCB& completion_cb);

 private:
  void OnTestComplete(bool success);
  void RunNextTest();

  CreateFileIOCB create_file_io_cb_;
  CompletionCB completion_cb_;
  std::list<FileIOTest> remaining_tests_;
  std::vector<uint8> large_data_;
  size_t total_num_tests_;  // Total number of tests.
  size_t num_passed_tests_;  // Number of passed tests.

  DISALLOW_COPY_AND_ASSIGN (FileIOTestRunner);
};

}  // namespace media

#endif  // MEDIA_CDM_PPAPI_CDM_FILE_IO_TEST_H_
