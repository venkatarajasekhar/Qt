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

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define snprintf _snprintf
#undef ERROR  // wingdi.h
#endif

#ifdef OSX
#include <CoreServices/CoreServices.h>
#elif defined(ANDROID)
#include <android/log.h>
static const char kLibjingle[] = "libjingle";
// Android has a 1024 limit on log inputs. We use 60 chars as an
// approx for the header/tag portion.
// See android/system/core/liblog/logd_write.c
static const int kMaxLogLineSize = 1024 - 60;
#endif  // OSX || ANDROID

#include <time.h>

#include <ostream>
#include <iomanip>
#include <limits.h>
#include <vector>

#include "talk/base/logging.h"
#include "talk/base/stream.h"
#include "talk/base/stringencode.h"
#include "talk/base/stringutils.h"
#include "talk/base/timeutils.h"

namespace talk_base {

/////////////////////////////////////////////////////////////////////////////
// Constant Labels
/////////////////////////////////////////////////////////////////////////////

const char * FindLabel(int value, const ConstantLabel entries[]) {
  for (int i = 0; entries[i].label; ++i) {
    if (value == entries[i].value) {
      return entries[i].label;
    }
  }
  return 0;
}

std::string ErrorName(int err, const ConstantLabel * err_table) {
  if (err == 0)
    return "No error";

  if (err_table != 0) {
    if (const char * value = FindLabel(err, err_table))
      return value;
  }

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "0x%08x", err);
  return buffer;
}

/////////////////////////////////////////////////////////////////////////////
// LogMessage
/////////////////////////////////////////////////////////////////////////////

const int LogMessage::NO_LOGGING = LS_ERROR + 1;

#if _DEBUG
static const int LOG_DEFAULT = LS_INFO;
#else  // !_DEBUG
static const int LOG_DEFAULT = LogMessage::NO_LOGGING;
#endif  // !_DEBUG

// Global lock for log subsystem, only needed to serialize access to streams_.
CriticalSection LogMessage::crit_;

// By default, release builds don't log, debug builds at info level
int LogMessage::min_sev_ = LOG_DEFAULT;
int LogMessage::dbg_sev_ = LOG_DEFAULT;

// Don't bother printing context for the ubiquitous INFO log messages
int LogMessage::ctx_sev_ = LS_WARNING;

// The list of logging streams currently configured.
// Note: we explicitly do not clean this up, because of the uncertain ordering
// of destructors at program exit.  Let the person who sets the stream trigger
// cleanup by setting to NULL, or let it leak (safe at program exit).
LogMessage::StreamList LogMessage::streams_;

// Boolean options default to false (0)
bool LogMessage::thread_, LogMessage::timestamp_;

// If we're in diagnostic mode, we'll be explicitly set that way; default=false.
bool LogMessage::is_diagnostic_mode_ = false;

LogMessage::LogMessage(const char* file, int line, LoggingSeverity sev,
                       LogErrorContext err_ctx, int err, const char* module)
    : severity_(sev),
      warn_slow_logs_delay_(WARN_SLOW_LOGS_DELAY) {
  if (timestamp_) {
    uint32 time = TimeSince(LogStartTime());
    // Also ensure WallClockStartTime is initialized, so that it matches
    // LogStartTime.
    WallClockStartTime();
    print_stream_ << "[" << std::setfill('0') << std::setw(3) << (time / 1000)
                  << ":" << std::setw(3) << (time % 1000) << std::setfill(' ')
                  << "] ";
  }

  if (thread_) {
#ifdef WIN32
    DWORD id = GetCurrentThreadId();
    print_stream_ << "[" << std::hex << id << std::dec << "] ";
#endif  // WIN32
  }

  if (severity_ >= ctx_sev_) {
    print_stream_ << Describe(sev) << "(" << DescribeFile(file)
                  << ":" << line << "): ";
  }

  if (err_ctx != ERRCTX_NONE) {
    std::ostringstream tmp;
    tmp << "[0x" << std::setfill('0') << std::hex << std::setw(8) << err << "]";
    switch (err_ctx) {
      case ERRCTX_ERRNO:
        tmp << " " << strerror(err);
        break;
#if WIN32
      case ERRCTX_HRESULT: {
        char msgbuf[256];
        DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM;
        HMODULE hmod = GetModuleHandleA(module);
        if (hmod)
          flags |= FORMAT_MESSAGE_FROM_HMODULE;
        if (DWORD len = FormatMessageA(
            flags, hmod, err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            msgbuf, sizeof(msgbuf) / sizeof(msgbuf[0]), NULL)) {
          while ((len > 0) &&
              isspace(static_cast<unsigned char>(msgbuf[len-1]))) {
            msgbuf[--len] = 0;
          }
          tmp << " " << msgbuf;
        }
        break;
      }
#endif  // WIN32
#if OSX
      case ERRCTX_OSSTATUS: {
        tmp << " " << nonnull(GetMacOSStatusErrorString(err), "Unknown error");
        if (const char* desc = GetMacOSStatusCommentString(err)) {
          tmp << ": " << desc;
        }
        break;
      }
#endif  // OSX
      default:
        break;
    }
    extra_ = tmp.str();
  }
}

LogMessage::~LogMessage() {
  if (!extra_.empty())
    print_stream_ << " : " << extra_;
  print_stream_ << std::endl;

  const std::string& str = print_stream_.str();
  if (severity_ >= dbg_sev_) {
    OutputToDebug(str, severity_);
  }

  uint32 before = Time();
  // Must lock streams_ before accessing
  CritScope cs(&crit_);
  for (StreamList::iterator it = streams_.begin(); it != streams_.end(); ++it) {
    if (severity_ >= it->second) {
      OutputToStream(it->first, str);
    }
  }
  uint32 delay = TimeSince(before);
  if (delay >= warn_slow_logs_delay_) {
    LogMessage slow_log_warning =
        talk_base::LogMessage(__FILE__, __LINE__, LS_WARNING);
    // If our warning is slow, we don't want to warn about it, because
    // that would lead to inifinite recursion.  So, give a really big
    // number for the delay threshold.
    slow_log_warning.warn_slow_logs_delay_ = UINT_MAX;
    slow_log_warning.stream() << "Slow log: took " << delay << "ms to write "
                              << str.size() << " bytes.";
  }
}

uint32 LogMessage::LogStartTime() {
  static const uint32 g_start = Time();
  return g_start;
}

uint32 LogMessage::WallClockStartTime() {
  static const uint32 g_start_wallclock = time(NULL);
  return g_start_wallclock;
}

void LogMessage::LogContext(int min_sev) {
  ctx_sev_ = min_sev;
}

void LogMessage::LogThreads(bool on) {
  thread_ = on;
}

void LogMessage::LogTimestamps(bool on) {
  timestamp_ = on;
}

void LogMessage::LogToDebug(int min_sev) {
  dbg_sev_ = min_sev;
  UpdateMinLogSeverity();
}

void LogMessage::LogToStream(StreamInterface* stream, int min_sev) {
  CritScope cs(&crit_);
  // Discard and delete all previously installed streams
  for (StreamList::iterator it = streams_.begin(); it != streams_.end(); ++it) {
    delete it->first;
  }
  streams_.clear();
  // Install the new stream, if specified
  if (stream) {
    AddLogToStream(stream, min_sev);
  }
}

int LogMessage::GetLogToStream(StreamInterface* stream) {
  CritScope cs(&crit_);
  int sev = NO_LOGGING;
  for (StreamList::iterator it = streams_.begin(); it != streams_.end(); ++it) {
    if (!stream || stream == it->first) {
      sev = _min(sev, it->second);
    }
  }
  return sev;
}

void LogMessage::AddLogToStream(StreamInterface* stream, int min_sev) {
  CritScope cs(&crit_);
  streams_.push_back(std::make_pair(stream, min_sev));
  UpdateMinLogSeverity();
}

void LogMessage::RemoveLogToStream(StreamInterface* stream) {
  CritScope cs(&crit_);
  for (StreamList::iterator it = streams_.begin(); it != streams_.end(); ++it) {
    if (stream == it->first) {
      streams_.erase(it);
      break;
    }
  }
  UpdateMinLogSeverity();
}

void LogMessage::ConfigureLogging(const char* params, const char* filename) {
  int current_level = LS_VERBOSE;
  int debug_level = GetLogToDebug();
  int file_level = GetLogToStream();

  std::vector<std::string> tokens;
  tokenize(params, ' ', &tokens);

  for (size_t i = 0; i < tokens.size(); ++i) {
    if (tokens[i].empty())
      continue;

    // Logging features
    if (tokens[i] == "tstamp") {
      LogTimestamps();
    } else if (tokens[i] == "thread") {
      LogThreads();

    // Logging levels
    } else if (tokens[i] == "sensitive") {
      current_level = LS_SENSITIVE;
    } else if (tokens[i] == "verbose") {
      current_level = LS_VERBOSE;
    } else if (tokens[i] == "info") {
      current_level = LS_INFO;
    } else if (tokens[i] == "warning") {
      current_level = LS_WARNING;
    } else if (tokens[i] == "error") {
      current_level = LS_ERROR;
    } else if (tokens[i] == "none") {
      current_level = NO_LOGGING;

    // Logging targets
    } else if (tokens[i] == "file") {
      file_level = current_level;
    } else if (tokens[i] == "debug") {
      debug_level = current_level;
    }
  }

#ifdef WIN32
  if ((NO_LOGGING != debug_level) && !::IsDebuggerPresent()) {
    // First, attempt to attach to our parent's console... so if you invoke
    // from the command line, we'll see the output there.  Otherwise, create
    // our own console window.
    // Note: These methods fail if a console already exists, which is fine.
    bool success = false;
    typedef BOOL (WINAPI* PFN_AttachConsole)(DWORD);
    if (HINSTANCE kernel32 = ::LoadLibrary(L"kernel32.dll")) {
      // AttachConsole is defined on WinXP+.
      if (PFN_AttachConsole attach_console = reinterpret_cast<PFN_AttachConsole>
            (::GetProcAddress(kernel32, "AttachConsole"))) {
        success = (FALSE != attach_console(ATTACH_PARENT_PROCESS));
      }
      ::FreeLibrary(kernel32);
    }
    if (!success) {
      ::AllocConsole();
    }
  }
#endif  // WIN32

  LogToDebug(debug_level);

#if !defined(__native_client__)  // No logging to file in NaCl.
  scoped_ptr<FileStream> stream;
  if (NO_LOGGING != file_level) {
    stream.reset(new FileStream);
    if (!stream->Open(filename, "wb", NULL) || !stream->DisableBuffering()) {
      stream.reset();
    }
  }

  LogToStream(stream.release(), file_level);
#endif
}

int LogMessage::ParseLogSeverity(const std::string& value) {
  int level = NO_LOGGING;
  if (value == "LS_SENSITIVE") {
    level = LS_SENSITIVE;
  } else if (value == "LS_VERBOSE") {
    level = LS_VERBOSE;
  } else if (value == "LS_INFO") {
    level = LS_INFO;
  } else if (value == "LS_WARNING") {
    level = LS_WARNING;
  } else if (value == "LS_ERROR") {
    level = LS_ERROR;
  } else if (isdigit(value[0])) {
    level = atoi(value.c_str());  // NOLINT
  }
  return level;
}

void LogMessage::UpdateMinLogSeverity() {
  int min_sev = dbg_sev_;
  for (StreamList::iterator it = streams_.begin(); it != streams_.end(); ++it) {
    min_sev = _min(dbg_sev_, it->second);
  }
  min_sev_ = min_sev;
}

const char* LogMessage::Describe(LoggingSeverity sev) {
  switch (sev) {
  case LS_SENSITIVE: return "Sensitive";
  case LS_VERBOSE:   return "Verbose";
  case LS_INFO:      return "Info";
  case LS_WARNING:   return "Warning";
  case LS_ERROR:     return "Error";
  default:           return "<unknown>";
  }
}

const char* LogMessage::DescribeFile(const char* file) {
  const char* end1 = ::strrchr(file, '/');
  const char* end2 = ::strrchr(file, '\\');
  if (!end1 && !end2)
    return file;
  else
    return (end1 > end2) ? end1 + 1 : end2 + 1;
}

void LogMessage::OutputToDebug(const std::string& str,
                               LoggingSeverity severity) {
  bool log_to_stderr = true;
#if defined(OSX) && (!defined(DEBUG) || defined(NDEBUG))
  // On the Mac, all stderr output goes to the Console log and causes clutter.
  // So in opt builds, don't log to stderr unless the user specifically sets
  // a preference to do so.
  CFStringRef key = CFStringCreateWithCString(kCFAllocatorDefault,
                                              "logToStdErr",
                                              kCFStringEncodingUTF8);
  CFStringRef domain = CFBundleGetIdentifier(CFBundleGetMainBundle());
  if (key != NULL && domain != NULL) {
    Boolean exists_and_is_valid;
    Boolean should_log =
        CFPreferencesGetAppBooleanValue(key, domain, &exists_and_is_valid);
    // If the key doesn't exist or is invalid or is false, we will not log to
    // stderr.
    log_to_stderr = exists_and_is_valid && should_log;
  }
  if (key != NULL) {
    CFRelease(key);
  }
#endif
#ifdef WIN32
  // Always log to the debugger.
  // Perhaps stderr should be controlled by a preference, as on Mac?
  OutputDebugStringA(str.c_str());
  if (log_to_stderr) {
    // This handles dynamically allocated consoles, too.
    if (HANDLE error_handle = ::GetStdHandle(STD_ERROR_HANDLE)) {
      log_to_stderr = false;
      DWORD written = 0;
      ::WriteFile(error_handle, str.data(), static_cast<DWORD>(str.size()),
                  &written, 0);
    }
  }
#endif  // WIN32
#ifdef ANDROID
  // Android's logging facility uses severity to log messages but we
  // need to map libjingle's severity levels to Android ones first.
  // Also write to stderr which maybe available to executable started
  // from the shell.
  int prio;
  switch (severity) {
    case LS_SENSITIVE:
      __android_log_write(ANDROID_LOG_INFO, kLibjingle, "SENSITIVE");
      if (log_to_stderr) {
        fprintf(stderr, "SENSITIVE");
        fflush(stderr);
      }
      return;
    case LS_VERBOSE:
      prio = ANDROID_LOG_VERBOSE;
      break;
    case LS_INFO:
      prio = ANDROID_LOG_INFO;
      break;
    case LS_WARNING:
      prio = ANDROID_LOG_WARN;
      break;
    case LS_ERROR:
      prio = ANDROID_LOG_ERROR;
      break;
    default:
      prio = ANDROID_LOG_UNKNOWN;
  }

  int size = str.size();
  int line = 0;
  int idx = 0;
  const int max_lines = size / kMaxLogLineSize + 1;
  if (max_lines == 1) {
    __android_log_print(prio, kLibjingle, "%.*s", size, str.c_str());
  } else {
    while (size > 0) {
      const int len = std::min(size, kMaxLogLineSize);
      // Use the size of the string in the format (str may have \0 in the
      // middle).
      __android_log_print(prio, kLibjingle, "[%d/%d] %.*s",
                          line + 1, max_lines,
                          len, str.c_str() + idx);
      idx += len;
      size -= len;
      ++line;
    }
  }
#endif  // ANDROID
  if (log_to_stderr) {
    fprintf(stderr, "%s", str.c_str());
    fflush(stderr);
  }
}

void LogMessage::OutputToStream(StreamInterface* stream,
                                const std::string& str) {
  // If write isn't fully successful, what are we going to do, log it? :)
  stream->WriteAll(str.data(), str.size(), NULL, NULL);
}

//////////////////////////////////////////////////////////////////////
// Logging Helpers
//////////////////////////////////////////////////////////////////////

void LogMultiline(LoggingSeverity level, const char* label, bool input,
                  const void* data, size_t len, bool hex_mode,
                  LogMultilineState* state) {
  if (!LOG_CHECK_LEVEL_V(level))
    return;

  const char * direction = (input ? " << " : " >> ");

  // NULL data means to flush our count of unprintable characters.
  if (!data) {
    if (state && state->unprintable_count_[input]) {
      LOG_V(level) << label << direction << "## "
                   << state->unprintable_count_[input]
                   << " consecutive unprintable ##";
      state->unprintable_count_[input] = 0;
    }
    return;
  }

  // The ctype classification functions want unsigned chars.
  const unsigned char* udata = static_cast<const unsigned char*>(data);

  if (hex_mode) {
    const size_t LINE_SIZE = 24;
    char hex_line[LINE_SIZE * 9 / 4 + 2], asc_line[LINE_SIZE + 1];
    while (len > 0) {
      memset(asc_line, ' ', sizeof(asc_line));
      memset(hex_line, ' ', sizeof(hex_line));
      size_t line_len = _min(len, LINE_SIZE);
      for (size_t i = 0; i < line_len; ++i) {
        unsigned char ch = udata[i];
        asc_line[i] = isprint(ch) ? ch : '.';
        hex_line[i*2 + i/4] = hex_encode(ch >> 4);
        hex_line[i*2 + i/4 + 1] = hex_encode(ch & 0xf);
      }
      asc_line[sizeof(asc_line)-1] = 0;
      hex_line[sizeof(hex_line)-1] = 0;
      LOG_V(level) << label << direction
                   << asc_line << " " << hex_line << " ";
      udata += line_len;
      len -= line_len;
    }
    return;
  }

  size_t consecutive_unprintable = state ? state->unprintable_count_[input] : 0;

  const unsigned char* end = udata + len;
  while (udata < end) {
    const unsigned char* line = udata;
    const unsigned char* end_of_line = strchrn<unsigned char>(udata,
                                                              end - udata,
                                                              '\n');
    if (!end_of_line) {
      udata = end_of_line = end;
    } else {
      udata = end_of_line + 1;
    }

    bool is_printable = true;

    // If we are in unprintable mode, we need to see a line of at least
    // kMinPrintableLine characters before we'll switch back.
    const ptrdiff_t kMinPrintableLine = 4;
    if (consecutive_unprintable && ((end_of_line - line) < kMinPrintableLine)) {
      is_printable = false;
    } else {
      // Determine if the line contains only whitespace and printable
      // characters.
      bool is_entirely_whitespace = true;
      for (const unsigned char* pos = line; pos < end_of_line; ++pos) {
        if (isspace(*pos))
          continue;
        is_entirely_whitespace = false;
        if (!isprint(*pos)) {
          is_printable = false;
          break;
        }
      }
      // Treat an empty line following unprintable data as unprintable.
      if (consecutive_unprintable && is_entirely_whitespace) {
        is_printable = false;
      }
    }
    if (!is_printable) {
      consecutive_unprintable += (udata - line);
      continue;
    }
    // Print out the current line, but prefix with a count of prior unprintable
    // characters.
    if (consecutive_unprintable) {
      LOG_V(level) << label << direction << "## " << consecutive_unprintable
                  << " consecutive unprintable ##";
      consecutive_unprintable = 0;
    }
    // Strip off trailing whitespace.
    while ((end_of_line > line) && isspace(*(end_of_line-1))) {
      --end_of_line;
    }
    // Filter out any private data
    std::string substr(reinterpret_cast<const char*>(line), end_of_line - line);
    std::string::size_type pos_private = substr.find("Email");
    if (pos_private == std::string::npos) {
      pos_private = substr.find("Passwd");
    }
    if (pos_private == std::string::npos) {
      LOG_V(level) << label << direction << substr;
    } else {
      LOG_V(level) << label << direction << "## omitted for privacy ##";
    }
  }

  if (state) {
    state->unprintable_count_[input] = consecutive_unprintable;
  }
}

//////////////////////////////////////////////////////////////////////

}  // namespace talk_base
