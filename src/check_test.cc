#include <gtest/gtest.h>
#include <cstdio>

#include "sio/error.h"
#include "sio/check.h"
#include "sio/type.h"
#include "sio/vec.h"

namespace sio {

TEST(Check, HoareLogic) {
  Vec<int> v = {1,2};
  i32 i = 0;
  i32 sum = 0;
  SIO_INVAR("sum = sum of v[0,i)");
  SIO_P_COND(i == 0);
  while (i != v.size()) {
    sum += v[i++];
  }
  SIO_Q_COND(sum == 3) << "1 + 2 != 3, something wrong. ";
}

namespace { // seal test func scope in anonymous namespace
Error test_error_func() {
  return Error::OK;
  //return Error::OutOfMemory;
}
}

TEST(Check, Error) {
  Error err = test_error_func();
  SIO_CHECK(!err) << "an error occured.";
}

} // namespace sio
