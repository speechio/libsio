#ifndef SIO_LOG_H
#define SIO_LOG_H

#include <cstdlib>
#include <iostream>
#include <sstream>
#include "absl/base/internal/pretty_function.h"

namespace sio {

enum class LogSeverity : int {
  kDebug = -1,
  kInfo = 0,
  kWarning = 1,
  kError = 2,
  kFatal = 3,
};

constexpr const char* LogSeverityRepr(LogSeverity s) {
  return s == sio::LogSeverity::kDebug
         ? ":[D] "
         : s == sio::LogSeverity::kInfo
           ? ":[I] "
           : s == sio::LogSeverity::kWarning
             ? ":[WARN] "
             : s == sio::LogSeverity::kError
               ? ":[ERROR] "
               : s == sio::LogSeverity::kFatal 
                 ? ":[FATAL] " 
                 : ":[UNKNOWN] ";
}

class Logger {
 public:
  Logger(const char *file, const char *func, uint32_t line, LogSeverity severity, std::ostream& os) :
    severity_(severity),
    os_(os)
  {
    buf_ << file << ":" << func << ":" << line << LogSeverityRepr(severity_);
  }

  template <typename T>
  Logger &operator<<(const T &val) {
    buf_ << val;
    return *this;
  }

  ~Logger() {
    os_ << buf_.str() << "\n";
    if (severity_ >= LogSeverity::kError) abort();
  }

 private:
  LogSeverity severity_;
  std::ostream& os_;
  std::ostringstream buf_;
};

#define SIO_DEBUG \
  sio::Logger(__FILE__, ABSL_PRETTY_FUNCTION, __LINE__, sio::LogSeverity::kDebug, std::cerr)
#define SIO_INFO \
  sio::Logger(__FILE__, ABSL_PRETTY_FUNCTION, __LINE__, sio::LogSeverity::kInfo, std::cerr)
#define SIO_WARNING \
  sio::Logger(__FILE__, ABSL_PRETTY_FUNCTION, __LINE__, sio::LogSeverity::kWarning, std::cerr)
#define SIO_ERROR \
  sio::Logger(__FILE__, ABSL_PRETTY_FUNCTION, __LINE__, sio::LogSeverity::kError, std::cerr)
#define SIO_FATAL \
  sio::Logger(__FILE__, ABSL_PRETTY_FUNCTION, __LINE__, sio::LogSeverity::kFatal, std::cerr)

} // namespace sio
#endif