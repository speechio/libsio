#include <gtest/gtest.h>
#include "sio/link.h"

TEST(Link, Basic) {
  using namespace sio;

  Link *l1 = new Link;
  Link *l2 = new Link;
  Link *l3 = new Link;

  EXPECT_TRUE(!l1->IsLinked());
  EXPECT_TRUE(!l2->IsLinked());
  EXPECT_TRUE(!l3->IsLinked());
  
  l1->InsertBefore(l2);
  EXPECT_TRUE(l1->Next() == l2);
  EXPECT_TRUE(l2->Prev() == l1);

  l3->InsertAfter(l2);
  EXPECT_TRUE(l2->Next() == l3);
  EXPECT_TRUE(l3->Prev() == l2);

  EXPECT_TRUE(l1->IsLinked());
  EXPECT_TRUE(l2->IsLinked());
  EXPECT_TRUE(l3->IsLinked());

  delete l2;  // l2 unlinked from l1/l3 via destructor
  EXPECT_TRUE(l1->Next() == l3);
  EXPECT_TRUE(l3->Prev() == l1);

  l3->Unlink();
  EXPECT_TRUE(!l1->IsLinked());
  EXPECT_TRUE(!l3->IsLinked());
  delete l3;

  EXPECT_TRUE(l1->Prev() == l1); // self linked
  EXPECT_TRUE(l1->Next() == l1);
  delete l1;
}
