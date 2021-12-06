#ifndef SIO_LOG_H
#define SIO_LOG_H

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <mutex>

#include "absl/base/optimization.h"

namespace sio {

/* SIO_FUNC_REPR */
#if defined(_MSC_VER)
#define SIO_FUNC_REPR __FUNCSIG__
#elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__) || defined(__PRETTY_FUNCTION__)
#define SIO_FUNC_REPR __PRETTY_FUNCTION__
#else
#define SIO_FUNC_REPR __func__
#endif


/* SIO_FILE_REPR */
constexpr const char* Basename(const char* fname, int offset) {
  return offset == 0 || fname[offset - 1] == '/' || fname[offset - 1] == '\\'
             ? fname + offset
             : Basename(fname, offset - 1);
}
#define SIO_FILE_REPR  ::sio::Basename(__FILE__, sizeof(__FILE__) - 1)


/* Log severity levels */
enum class LogSeverity : int {
  kDebug = 0,
  kInfo = 1,
  kWarning = 2,
  kError = 3,
  kFatal = 4,
};

constexpr const char* LogSeverityRepr(LogSeverity s) {
  return s == LogSeverity::kDebug
         ? "[D]"
         : s == LogSeverity::kInfo
           ? "[I]"
           : s == LogSeverity::kWarning
             ? "[WARNING]"
             : s == LogSeverity::kError
               ? "[ERROR]"
               : s == LogSeverity::kFatal 
                 ? "[FATAL]" 
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
  Logger(const char *file, const char *func, size_t line, LogSeverity severity, std::ostream& os)
  : file_(file), func_(func), line_(line), severity_(severity), os_(os)
  {
    if (severity_ >= CurrentLogVerbosity()) {
      buf_ << LogSeverityRepr(severity_) << " ";
    }
  }

  template <typename T>
  Logger &operator<<(const T &val) {
    buf_ << val;
    return *this;
  }

  ~Logger() {
    if (severity_ >= CurrentLogVerbosity()) {
      if (severity_ >= LogSeverity::kWarning) {
        buf_ << " [" << file_ << ":" << line_ << ":" << func_ << "]";
      }
      os_ << buf_.str() << "\n";
    }
    if (severity_ >= LogSeverity::kFatal) abort();
  }

 private:
  const char* file_;
  const char* func_;
  size_t line_;
  LogSeverity severity_;
  std::ostream& os_;

  std::ostringstream buf_;
};

#define SIO_DEBUG   sio::Logger(SIO_FILE_REPR, SIO_FUNC_REPR, __LINE__, sio::LogSeverity::kDebug,   std::cerr)
#define SIO_INFO    sio::Logger(SIO_FILE_REPR, SIO_FUNC_REPR, __LINE__, sio::LogSeverity::kInfo,    std::cerr)
#define SIO_WARNING sio::Logger(SIO_FILE_REPR, SIO_FUNC_REPR, __LINE__, sio::LogSeverity::kWarning, std::cerr)
#define SIO_ERROR   sio::Logger(SIO_FILE_REPR, SIO_FUNC_REPR, __LINE__, sio::LogSeverity::kError,   std::cerr)
#define SIO_FATAL   sio::Logger(SIO_FILE_REPR, SIO_FUNC_REPR, __LINE__, sio::LogSeverity::kFatal,   std::cerr)


/* Checking utils */
#define SIO_CHECK(expr, message) do {                           \
  if (ABSL_PREDICT_FALSE(!(expr))) {                            \
    SIO_ERROR << "Check {" << #expr << "} failed: " << message; \
  }                                                             \
} while(0)

#define P_COND(cond) SIO_CHECK(cond, "Precondition")
#define Q_COND(cond) SIO_CHECK(cond, "Postcondition")
#define INVAR(cond)  SIO_CHECK(cond, "Invariant")

} // namespace sio
#endif

