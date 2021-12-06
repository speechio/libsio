#include <gtest/gtest.h>
#include "sio/log.h"
#include "sio/type.h"
#include "sio/str.h"
#include "sio/vec.h"

TEST(Log, Basic) {
  using namespace sio;
  Str msg = "This is a log message.";

  SIO_DEBUG << msg;
  SIO_INFO << msg;
  SIO_WARNING << msg;
  SIO_ERROR << msg;
  //SIO_FATAL << msg;
}
