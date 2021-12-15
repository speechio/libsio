#include <gtest/gtest.h>
#include "sio/log.h"
#include "sio/str.h"

namespace sio {

TEST(Log, Basic) {
  Str msg = "This is a log message.";

  SIO_DEBUG   << msg;
  SIO_INFO    << msg;
  SIO_WARNING << msg;
  SIO_ERROR   << msg;
  SIO_FATAL   << msg;
}

} // namespace sio
