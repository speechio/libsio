#ifndef SIO_LOG_H
#define SIO_LOG_H

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <mutex>

#include "sio/macro.h"
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
  	return \
		s == LogSeverity::kDebug   ? "[D]" : 
		s == LogSeverity::kInfo    ? "[I]" :
		s == LogSeverity::kWarning ? "[WARNING]" :
		s == LogSeverity::kError   ? "[ERROR]" :
		s == LogSeverity::kFatal   ? "[FATAL]" :
		                             "[UNKNOWN]";
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
	Logger(std::ostream& ostream, const char *file, size_t line, const char *func, LogSeverity severity) :
		ostream_(ostream), file_(file), line_(line), func_(func), severity_(severity)
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
	}

private:
	std::ostream& ostream_;
	const char* file_;
	size_t line_;
	const char* func_;
	LogSeverity severity_;

	std::ostringstream buf_;
};

#define SIO_DEBUG \
    ::sio::Logger(std::cerr, SIO__FILE__, __LINE__, SIO__FUNC__, ::sio::LogSeverity::kDebug)
#define SIO_INFO \
    ::sio::Logger(std::cerr, SIO__FILE__, __LINE__, SIO__FUNC__, ::sio::LogSeverity::kInfo)
#define SIO_WARNING \
    ::sio::Logger(std::cerr, SIO__FILE__, __LINE__, SIO__FUNC__, ::sio::LogSeverity::kWarning)
#define SIO_ERROR \
    ::sio::Logger(std::cerr, SIO__FILE__, __LINE__, SIO__FUNC__, ::sio::LogSeverity::kError)
#define SIO_FATAL \
    ::sio::Logger(std::cerr, SIO__FILE__, __LINE__, SIO__FUNC__, ::sio::LogSeverity::kFatal)

} // namespace sio
#endif

