#include <gtest/gtest.h>
#include "sio/check.h"
#include "sio/type.h"
#include "sio/vec.h"

TEST(Check, Basic) {
  using namespace sio;

  Vec<int> v = {1,2};
  i32 i = 0;
  i32 sum = 0;
  SIO_INVAR("sum = sum of v[0,i)");
  SIO_P_COND(i == 0);
  while (i != 2) {
    sum += v[i++];
  }
  SIO_Q_COND(i == 2);
}
