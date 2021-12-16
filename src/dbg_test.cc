#include "gtest/gtest.h"

#include "sio/type.h"
#include "sio/str.h"
#include "sio/vec.h"
#include "sio/map.h"

//#define DBG_MACRO_DISABLE
#include "sio/dbg.h"

TEST(dbg_macro, Basic) {
  using namespace sio;
  int i = 0, j = 0;
  while(dbg(i < 2)) { // with DBG_MACRO_DISABLE, dbg() is no-op
    dbg(i++);
    j--;
  }
  EXPECT_EQ(i,  2);
  EXPECT_EQ(dbg("j value check in EXPECT_EQ:", j), -2); // comma expression use last one as expr value

  Map<i32, Str> m = {
    {1,"abc"}, 
    {2,"def"}
  };

  Vec<Str> v = {"s1", "s2", "s3"};

  dbg("---- dbg macro ----", i, m, v, "==== dbg macro ====");
}