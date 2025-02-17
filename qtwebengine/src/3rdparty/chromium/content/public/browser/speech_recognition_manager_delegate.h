// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_

#include <string>

#include "base/callback_forward.h"
#include "content/public/common/speech_recognition_error.h"

namespace content {

class SpeechRecognitionEventListener;
struct SpeechRecognitionResult;

// Allows embedders to display the current state of recognition, for getting the
// user's permission and for fetching optional request information.
class SpeechRecognitionManagerDelegate {
 public:
  virtual ~SpeechRecognitionManagerDelegate() {}

  // Get the optional diagnostic hardware information if available.
  // This is called on the IO thread.
  virtual void GetDiagnosticInformation(bool* can_report_metrics,
                                        std::string* hardware_info) = 0;

  // Checks (asynchronously) if current setup allows speech recognition.
  // This is called on the IO thread.
  virtual void CheckRecognitionIsAllowed(
      int session_id,
      base::Callback<void(bool ask_user, bool is_allowed)> callback) = 0;

  // Checks whether the delegate is interested (returning a non NULL ptr) or not
  // (returning NULL) in receiving a copy of all sessions events.
  // This is called on the IO thread.
  virtual SpeechRecognitionEventListener* GetEventListener() = 0;

  // Checks whether the speech recognition for the given renderer should filter
  // profanities or not.
  virtual bool FilterProfanities(int render_process_id) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_
