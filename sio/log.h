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
  kDebug = -1,
  kInfo = 0,
  kWarning = 1,
  kError = 2,
  kFatal = 3,
};

constexpr const char* LogSeverityRepr(LogSeverity s) {
  return s == LogSeverity::kDebug
         ? "[D]"
         : s == LogSeverity::kInfo
           ? "[I]"
           : s == LogSeverity::kWarning
             ? "[W]"
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


#define SIO_LOG(severity, ostream, message_format, ...)           \
  do {                                                            \
    if (severity >= sio::CurrentLogVerbosity()) {                 \
      fprintf(ostream,                                            \
        "%s:%d:%s %s " message_format,                            \
        SIO_FILE_REPR, __LINE__,                                  \
        SIO_FUNC_REPR,                                            \
        sio::LogSeverityRepr(severity),                           \
        __VA_ARGS__                                               \
      );                                                          \
      fflush(stderr);                                             \
    }                                                             \
    if (ABSL_PREDICT_FALSE(severity >= sio::LogSeverity::kError)) \
      abort();                                                    \
  } while(0)


#define SIO_DEBUG(...)   SIO_LOG(sio::LogSeverity::kDebug,   stderr, __VA_ARGS__)
#define SIO_INFO(...)    SIO_LOG(sio::LogSeverity::kInfo,    stderr, __VA_ARGS__)
#define SIO_WARNING(...) SIO_LOG(sio::LogSeverity::kWarning, stderr, __VA_ARGS__)
#define SIO_ERROR(...)   SIO_LOG(sio::LogSeverity::kError,   stderr, __VA_ARGS__)
#define SIO_FATAL(...)   SIO_LOG(sio::LogSeverity::kFatal,   stderr, __VA_ARGS__)


#define SIO_CHECK(expr, message) do {                       \
    if (ABSL_PREDICT_FALSE(!(expr))) {                      \
      SIO_ERROR("Check {%s} failed: %s\n", #expr, message); \
    }                                                       \
} while(0)


/* Hoare logic checking utils */
#define P_COND(cond) SIO_CHECK(cond, "Precondition")
#define Q_COND(cond) SIO_CHECK(cond, "Postcondition")
#define INVAR(cond)  SIO_CHECK(cond, "Invariant")

} // namespace sio
#endif

