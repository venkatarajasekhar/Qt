// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/important_file_writer.h"

#include <stdio.h>

#include <string>

#include "base/bind.h"
#include "base/critical_closure.h"
#include "base/file_util.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_runner.h"
#include "base/task_runner_util.h"
#include "base/threading/thread.h"
#include "base/time/time.h"

namespace base {

namespace {

const int kDefaultCommitIntervalMs = 10000;

enum TempFileFailure {
  FAILED_CREATING,
  FAILED_OPENING,
  FAILED_CLOSING,
  FAILED_WRITING,
  FAILED_RENAMING,
  TEMP_FILE_FAILURE_MAX
};

void LogFailure(const FilePath& path, TempFileFailure failure_code,
                const std::string& message) {
  UMA_HISTOGRAM_ENUMERATION("ImportantFile.TempFileFailures", failure_code,
                            TEMP_FILE_FAILURE_MAX);
  DPLOG(WARNING) << "temp file failure: " << path.value().c_str()
                 << " : " << message;
}

}  // namespace

// static
bool ImportantFileWriter::WriteFileAtomically(const FilePath& path,
                                              const std::string& data) {
  // Write the data to a temp file then rename to avoid data loss if we crash
  // while writing the file. Ensure that the temp file is on the same volume
  // as target file, so it can be moved in one step, and that the temp file
  // is securely created.
  FilePath tmp_file_path;
  if (!base::CreateTemporaryFileInDir(path.DirName(), &tmp_file_path)) {
    LogFailure(path, FAILED_CREATING, "could not create temporary file");
    return false;
  }

  File tmp_file(tmp_file_path, File::FLAG_OPEN | File::FLAG_WRITE);
  if (!tmp_file.IsValid()) {
    LogFailure(path, FAILED_OPENING, "could not open temporary file");
    return false;
  }

  // If this happens in the wild something really bad is going on.
  CHECK_LE(data.length(), static_cast<size_t>(kint32max));
  int bytes_written = tmp_file.Write(0, data.data(),
                                     static_cast<int>(data.length()));
  tmp_file.Flush();  // Ignore return value.
  tmp_file.Close();

  if (bytes_written < static_cast<int>(data.length())) {
    LogFailure(path, FAILED_WRITING, "error writing, bytes_written=" +
               IntToString(bytes_written));
    base::DeleteFile(tmp_file_path, false);
    return false;
  }

  if (!base::ReplaceFile(tmp_file_path, path, NULL)) {
    LogFailure(path, FAILED_RENAMING, "could not rename temporary file");
    base::DeleteFile(tmp_file_path, false);
    return false;
  }

  return true;
}

ImportantFileWriter::ImportantFileWriter(const FilePath& path,
                                         base::SequencedTaskRunner* task_runner)
    : path_(path),
      task_runner_(task_runner),
      serializer_(NULL),
      commit_interval_(TimeDelta::FromMilliseconds(kDefaultCommitIntervalMs)),
      weak_factory_(this) {
  DCHECK(CalledOnValidThread());
  DCHECK(task_runner_.get());
}

ImportantFileWriter::~ImportantFileWriter() {
  // We're usually a member variable of some other object, which also tends
  // to be our serializer. It may not be safe to call back to the parent object
  // being destructed.
  DCHECK(!HasPendingWrite());
}

bool ImportantFileWriter::HasPendingWrite() const {
  DCHECK(CalledOnValidThread());
  return timer_.IsRunning();
}

void ImportantFileWriter::WriteNow(const std::string& data) {
  DCHECK(CalledOnValidThread());
  if (data.length() > static_cast<size_t>(kint32max)) {
    NOTREACHED();
    return;
  }

  if (HasPendingWrite())
    timer_.Stop();

  if (!PostWriteTask(data)) {
    // Posting the task to background message loop is not expected
    // to fail, but if it does, avoid losing data and just hit the disk
    // on the current thread.
    NOTREACHED();

    WriteFileAtomically(path_, data);
  }
}

void ImportantFileWriter::ScheduleWrite(DataSerializer* serializer) {
  DCHECK(CalledOnValidThread());

  DCHECK(serializer);
  serializer_ = serializer;

  if (!timer_.IsRunning()) {
    timer_.Start(FROM_HERE, commit_interval_, this,
                 &ImportantFileWriter::DoScheduledWrite);
  }
}

void ImportantFileWriter::DoScheduledWrite() {
  DCHECK(serializer_);
  std::string data;
  if (serializer_->SerializeData(&data)) {
    WriteNow(data);
  } else {
    DLOG(WARNING) << "failed to serialize data to be saved in "
                  << path_.value().c_str();
  }
  serializer_ = NULL;
}

void ImportantFileWriter::RegisterOnNextSuccessfulWriteCallback(
    const base::Closure& on_next_successful_write) {
  DCHECK(on_next_successful_write_.is_null());
  on_next_successful_write_ = on_next_successful_write;
}

bool ImportantFileWriter::PostWriteTask(const std::string& data) {
  // TODO(gab): This code could always use PostTaskAndReplyWithResult and let
  // ForwardSuccessfulWrite() no-op if |on_next_successful_write_| is null, but
  // PostTaskAndReply causes memory leaks in tests (crbug.com/371974) and
  // suppressing all of those is unrealistic hence we avoid most of them by
  // using PostTask() in the typical scenario below.
  if (!on_next_successful_write_.is_null()) {
    return base::PostTaskAndReplyWithResult(
        task_runner_,
        FROM_HERE,
        MakeCriticalClosure(
            Bind(&ImportantFileWriter::WriteFileAtomically, path_, data)),
        Bind(&ImportantFileWriter::ForwardSuccessfulWrite,
             weak_factory_.GetWeakPtr()));
  }
  return task_runner_->PostTask(
      FROM_HERE,
      MakeCriticalClosure(
          Bind(IgnoreResult(&ImportantFileWriter::WriteFileAtomically),
               path_, data)));
}

void ImportantFileWriter::ForwardSuccessfulWrite(bool result) {
  DCHECK(CalledOnValidThread());
  if (result && !on_next_successful_write_.is_null()) {
    on_next_successful_write_.Run();
    on_next_successful_write_.Reset();
  }
}

}  // namespace base
