// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views_content_client/views_content_main_delegate.h"

#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "content/public/common/content_switches.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "ui/views_content_client/views_content_browser_client.h"

#if defined(OS_WIN)
#include "base/logging_win.h"
#endif

namespace ui {
namespace {

#if defined(OS_WIN)
// {83FAC8EE-7A0E-4dbb-A3F6-6F500D7CAB1A}
const GUID kViewsContentClientProviderName =
    { 0x83fac8ee, 0x7a0e, 0x4dbb,
        { 0xa3, 0xf6, 0x6f, 0x50, 0xd, 0x7c, 0xab, 0x1a } };
#endif

}  // namespace

ViewsContentMainDelegate::ViewsContentMainDelegate(
    ViewsContentClient* views_content_client)
    : views_content_client_(views_content_client) {
}

ViewsContentMainDelegate::~ViewsContentMainDelegate() {
}

bool ViewsContentMainDelegate::BasicStartupComplete(int* exit_code) {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);

  content::SetContentClient(&content_client_);

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  bool success = logging::InitLogging(settings);
  CHECK(success);
#if defined(OS_WIN)
  logging::LogEventProvider::Initialize(kViewsContentClientProviderName);
#endif

  return false;
}

void ViewsContentMainDelegate::PreSandboxStartup() {
  base::FilePath ui_test_pak_path;
  DCHECK(PathService::Get(ui::UI_TEST_PAK, &ui_test_pak_path));
  ui::ResourceBundle::InitSharedInstanceWithPakPath(ui_test_pak_path);

  // Load content resources to provide, e.g., sandbox configuration data on Mac.
  base::FilePath content_resources_pak_path;
  PathService::Get(base::DIR_MODULE, &content_resources_pak_path);
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
      content_resources_pak_path.AppendASCII("content_resources.pak"),
      ui::SCALE_FACTOR_100P);
}

content::ContentBrowserClient*
    ViewsContentMainDelegate::CreateContentBrowserClient() {
  browser_client_.reset(new ViewsContentBrowserClient(views_content_client_));
  return browser_client_.get();
}

}  // namespace ui
