#ifndef SIO_LOGGER_H
#define SIO_LOGGER_H

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <mutex>

#include "sio/base.h"

namespace sio {

/* Log severity levels */
enum class LogSeverity : int {
    kInfo = 0,
    kWarning = 1,
    kError = 2,
    kFatal = 3,
};


constexpr const char* LogSeverityRepr(LogSeverity s) {
    return s == LogSeverity::kInfo    ? "[I]" :
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
        if      (v == "INFO")    s = LogSeverity::kInfo;
        else if (v == "WARNING") s = LogSeverity::kWarning;
        else if (v == "ERROR")   s = LogSeverity::kError;
        else if (v == "FATAL")   s = LogSeverity::kFatal;
        else    fprintf(stderr, "Unknown SIO_VERBOSITY: %s\n", v.c_str());
    });
    return s;
}


class Logger {
public:
    Logger(std::ostream& ostream, const char *file, size_t line, const char *func, LogSeverity severity) :
        ostream_(ostream),  severity_(severity)
    {
        if (severity_ >= CurrentLogVerbosity()) {
            std::ostringstream buf;
            buf << LogSeverityRepr(severity_);

            if (severity_ >= LogSeverity::kWarning) {
                buf << "(" << file << ":" << line << ":" << func << ")";
            }

            buf << " ";
            ostream_ << buf.str();
        }
    }

    ~Logger() {
        if (severity_ >= CurrentLogVerbosity()) {
            ostream_ << "\n";
        }
    }

    template <typename T>
    Logger& operator<<(const T& val) {
        if (severity_ >= CurrentLogVerbosity()) {
            ostream_ << val;
        }
        return *this;
    }

private:
    std::ostream& ostream_;
    LogSeverity severity_;
};


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
