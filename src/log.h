#ifndef SIO_LOG_H
#define SIO_LOG_H

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <mutex>

#include "sio/base.h"
#include "sio/error.h"

namespace sio {

/* Log severity levels */
enum class LogSeverity : int {
  kDebug = 0,
  kInfo = 1,
  kWarning = 2,
  kError = 3,
  kFatal = 4,
};

constexpr const char* LogSeverityRepr(LogSeverity s) {
  return s == LogSeverity::kDebug ? "[D]"
         : s == LogSeverity::kInfo ? "[I]"
           : s == LogSeverity::kWarning ? "[WARNING]"
             : s == LogSeverity::kError ? "[ERROR]"
               : s == LogSeverity::kFatal ? "[FATAL]" 
                 : "[UNKNOWN]";
}

inline LogSeverity CurrentLogVerbosity() {
  static LogSeverity res = LogSeverity::kInfo; // default: kInfo
  static std::once_flag flag;
  std::call_once(flag, [](){
    const char* verbosity = std::getenv("SIO_VERBOSITY");
    if (verbosity == nullptr) return;

    std::string v = verbosity;
    if      (v == "DEBUG")   res = LogSeverity::kDebug;
    else if (v == "INFO")    res = LogSeverity::kInfo;
    else if (v == "WARNING") res = LogSeverity::kWarning;
    else if (v == "ERROR")   res = LogSeverity::kError;
    else if (v == "FATAL")   res = LogSeverity::kFatal;
    else    fprintf(stderr, "Unknown SIO_VERBOSITY: %s", v.c_str());
  });
  return res;
}


/* Logging utils */
class Logger {
 public:
  Logger(std::ostream& ostream, const char *file, const char *func, size_t line, LogSeverity severity, Error err)
  : ostream_(ostream), file_(file), func_(func), line_(line), severity_(severity), err_(err)
  {
    if (severity_ >= CurrentLogVerbosity()) {
      buf_ << LogSeverityRepr(severity_);
      if (severity_ == LogSeverity::kDebug || severity_ >= LogSeverity::kWarning) {
        buf_ << "(" << file_ << ":" << line_ << ":" << func_ << ")";
      }
      buf_ << " ";
    }
  }

  template <typename T>
  Logger &operator<<(const T &val) {
    buf_ << val;
    return *this;
  }

  ~Logger() {
    if (severity_ >= CurrentLogVerbosity()) {
      ostream_ << buf_.str() << "\n";
    }
    if (severity_ >= LogSeverity::kError) {
      panic(file_, line_, func_, err_);
    }
  }

 private:
  std::ostream& ostream_;
  const char* file_;
  const char* func_;
  size_t line_;
  LogSeverity severity_;
  Error err_;

  std::ostringstream buf_;
};

#define SIO_DEBUG \
    sio::Logger(std::cerr, SIO_FILE_REPR, SIO_FUNC_REPR, __LINE__, sio::LogSeverity::kDebug, sio::Error::None)
#define SIO_INFO \
    sio::Logger(std::cerr, SIO_FILE_REPR, SIO_FUNC_REPR, __LINE__, sio::LogSeverity::kInfo, sio::Error::None)
#define SIO_WARNING \
    sio::Logger(std::cerr, SIO_FILE_REPR, SIO_FUNC_REPR, __LINE__, sio::LogSeverity::kWarning, sio::Error::None)
#define SIO_ERROR(err) \
    sio::Logger(std::cerr, SIO_FILE_REPR, SIO_FUNC_REPR, __LINE__, sio::LogSeverity::kError, err)
#define SIO_FATAL(err) \
    sio::Logger(std::cerr, SIO_FILE_REPR, SIO_FUNC_REPR, __LINE__, sio::LogSeverity::kFatal, err)

} // namespace sio
#endif

