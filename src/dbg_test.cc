#include "gtest/gtest.h"

#include "sio/type.h"
#include "sio/str.h"
#include "sio/vec.h"
#include "sio/map.h"

//#define DBG_MACRO_DISABLE
#include "sio/dbg.h"

TEST(dbg_macro, Basic) {
  using namespace sio;
  int i = 1, j = -1;
  while(dbg(i < 2)) {
    dbg(i++);
    j--;
  }
  dbg(i == 2);
  dbg(i, j);
  std::cout << i << " " << j << "\n";

  Map<i32, Str> m = {
    {1,"abc"}, 
    {2,"def"}
  };

  Vec<Str> v = {"s1", "s2", "s3"};

  dbg("testing debug macro...", i, m, v);
}