#include <gtest/gtest.h>
#include "sio/sio.h"

TEST(SIOTest, Log) {
  using namespace sio;

  Str msg = "This is a log message.";

  SIO_DEBUG << msg;
  SIO_INFO << msg;
  SIO_WARNING << msg;
  SIO_ERROR << msg;
  //SIO_FATAL << msg;

  Vec<i32> v = {1,2};

  index_t i = 0;
  i32 sum = 0;
  INVAR("sum = sum of v[0,i)");
  P_COND(i == 0);
  while (i != 2) {
    sum += v[i++];
  }
  Q_COND(i == 2);
}