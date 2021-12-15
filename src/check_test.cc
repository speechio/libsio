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
  while (i != 2) {
    sum += v[i++];
  }
  SIO_Q_COND(i == 2);
}

namespace { // seal test func scope in anonymous namespace
Error test_error_func() {
  return Error::None;
  //return Error::OOM;
}
}

TEST(Check, Error) {
  Error err = test_error_func();
  SIO_Q_COND(!err);

  //FILE *fp = nullptr;
  //SIO_CHECK(fp != nullptr, "cannot open file", Error::InvalidFileHandle);

  //SIO_CHECK(false, "", Error::Unknown);
}

} // namespace sio