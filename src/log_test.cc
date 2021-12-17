#include <gtest/gtest.h>
#include "sio/log.h"
#include "sio/str.h"

namespace sio {

TEST(Log, Basic) {
  Str msg = "This is a log message.";

  SIO_DEBUG   << msg;
  SIO_INFO    << msg;
  SIO_WARNING << msg;
  SIO_ERROR(Error::kNone)   << msg;
  SIO_FATAL(Error::kNone)   << msg; // non fatal error won't trigger abort()
}

} // namespace sio
