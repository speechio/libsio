#include <gtest/gtest.h>
#include "sio/sio.h"

TEST(SIOTest, Log) {
  using namespace sio;

  Str debug_msg = "This is debug.";
  Str info_msg = "This is info.";
  Str warning_msg = "This is warning.";
  Str error_msg = "This is error.";
  Str fatal_msg = "This is fatal.";

  SIO_DEBUG("%s\n", debug_msg.c_str());
  SIO_INFO("%s\n", info_msg.c_str());
  SIO_WARNING("%s\n", warning_msg.c_str());
  SIO_WARNING("%d\n", 32);
  //SIO_ERROR("%s\n", error_msg);
  //SIO_ERROR("%s\n", fatal_msg);

  Vec<i32> v;
  v.push_back(1);
  v.push_back(2);

  index_t i = 0;
  i32 sum = 0;
  //INVAR("sum = sum of v[0,i)");
  P_COND(i == 0);
  while (i != 2) {
    sum += v[i++];
  }
  Q_COND(i == 2);
}