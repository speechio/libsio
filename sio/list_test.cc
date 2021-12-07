#include "gtest/gtest.h"
#include "sio/list.h"

TEST(List, Basic) {
  using namespace sio;
  struct A {
    Link link;
    char* c;
    int i;
  };

  Owner<A*> a1 = new A;
  Owner<A*> a2 = new A;

  Owner<List<A>*> l = new List<A>(offsetof(A, link));
  EXPECT_TRUE(l->IsEmpty());
  
  l->InsertHead(a1);
  EXPECT_TRUE(l->NumElems() == 1);
  EXPECT_TRUE(l->Head() == a1);
  EXPECT_TRUE(l->Tail() == a1);

  l->InsertTail(a2);
  EXPECT_TRUE(l->NumElems() == 2);
  EXPECT_TRUE(l->Head() == a1);
  EXPECT_TRUE(l->Tail() == a2);
  EXPECT_TRUE(l->Next(a1) == a2);
  EXPECT_TRUE(l->Next(a2) == nullptr);
  EXPECT_TRUE(l->Prev(a2) == a1);
  EXPECT_TRUE(l->Prev(a1) == nullptr);

  delete a1;
  EXPECT_TRUE(l->NumElems() == 1);
  EXPECT_TRUE(l->Head() == a2);
  EXPECT_TRUE(l->Head() == l->Tail());

  l->UnlinkNode(a2);
  EXPECT_TRUE(l->IsEmpty());
  delete a2;
  delete l;
}
