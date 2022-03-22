#ifndef SIO_LOGGER_H
#define SIO_LOGGER_H

#include <stdlib.h>
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
    return s == LogSeverity::kDebug   ? "[D]" : 
           s == LogSeverity::kInfo    ? "[I]" :
           s == LogSeverity::kWarning ? "[WARNING]" :
           s == LogSeverity::kError   ? "[ERROR]" :
           s == LogSeverity::kFatal   ? "[FATAL]" :
                                        "[UNKNOWN]";
}

inline LogSeverity CurrentLogVerbosity() {
    static LogSeverity s = LogSeverity::kInfo; // default: kInfo
    static std::once_flag flag;
    std::call_once(flag, [](){
        const char* verbosity = std::getenv("SIO_VERBOSITY");
        if (verbosity == nullptr) return;

        std::string v = verbosity;
        if      (v == "DEBUG")   s = LogSeverity::kDebug;
        else if (v == "INFO")    s = LogSeverity::kInfo;
        else if (v == "WARNING") s = LogSeverity::kWarning;
        else if (v == "ERROR")   s = LogSeverity::kError;
        else if (v == "FATAL")   s = LogSeverity::kFatal;
        else    fprintf(stderr, "Unknown SIO_VERBOSITY: %s", v.c_str());
    });
    return s;
}


/* Logging utils */
class Logger {
public:
    Logger(std::ostream& ostream, const char *file, size_t line, const char *func, LogSeverity severity) :
        ostream_(ostream),  severity_(severity)
    {
        if (severity_ >= CurrentLogVerbosity()) {
            std::ostringstream buf;
            buf << LogSeverityRepr(severity_);
            if (severity_ == LogSeverity::kDebug || severity_ >= LogSeverity::kWarning) {
                buf << "(" << file << ":" << line << ":" << func << ")";
            }
            buf << " ";
            ostream_ << buf.str();
        }
    }

    template <typename T>
    Logger& operator<<(const T& val) {
        if (severity_ >= CurrentLogVerbosity()) {
            ostream_ << val;
        }
        return *this;
    }

    ~Logger() {
        if (severity_ >= CurrentLogVerbosity()) {
            ostream_ << "\n";
        }
    }

private:
    std::ostream& ostream_;
    LogSeverity severity_;
};

#define SIO_DEBUG \
    ::sio::Logger(std::cerr, SIO_FILE_REPR, SIO_LINE_REPR, SIO_FUNC_REPR, ::sio::LogSeverity::kDebug)
#define SIO_INFO \
    ::sio::Logger(std::cerr, SIO_FILE_REPR, SIO_LINE_REPR, SIO_FUNC_REPR, ::sio::LogSeverity::kInfo)
#define SIO_WARNING \
    ::sio::Logger(std::cerr, SIO_FILE_REPR, SIO_LINE_REPR, SIO_FUNC_REPR, ::sio::LogSeverity::kWarning)
#define SIO_ERROR \
    ::sio::Logger(std::cerr, SIO_FILE_REPR, SIO_LINE_REPR, SIO_FUNC_REPR, ::sio::LogSeverity::kError)
#define SIO_FATAL \
    ::sio::Logger(std::cerr, SIO_FILE_REPR, SIO_LINE_REPR, SIO_FUNC_REPR, ::sio::LogSeverity::kFatal)

} // namespace sio
#endif
