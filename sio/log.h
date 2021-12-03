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

constexpr const char* LogSeverityName(LogSeverity s) {
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
  Logger(const char *file, const char *func, uint32_t line, LogSeverity severity, std::ostream& os = std::cerr) :
    file_(file),
    func_(func),
    line_(line),
    severity_(severity),
    os_(os)
  {
    buf_ << file << ":" << func << ":" << line << LogSeverityName(severity_);
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
  std::ostream& os_;
  std::ostringstream buf_;
  const char *file_;
  const char *func_;
  uint32_t line_;
  LogSeverity severity_;
};

#define SIO_DEBUG \
  sio::Logger(__FILE__, ABSL_PRETTY_FUNCTION, __LINE__, sio::LogSeverity::kDebug)
#define SIO_INFO \
  sio::Logger(__FILE__, ABSL_PRETTY_FUNCTION, __LINE__, sio::LogSeverity::kInfo)
#define SIO_WARNING \
  sio::Logger(__FILE__, ABSL_PRETTY_FUNCTION, __LINE__, sio::LogSeverity::kWarning)
#define SIO_ERROR \
  sio::Logger(__FILE__, ABSL_PRETTY_FUNCTION, __LINE__, sio::LogSeverity::kError)
#define SIO_FATAL \
  sio::Logger(__FILE__, ABSL_PRETTY_FUNCTION, __LINE__, sio::LogSeverity::kFatal)

} // namespace sio
#endif