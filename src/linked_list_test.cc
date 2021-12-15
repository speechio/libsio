#include "gtest/gtest.h"
#include "sio/linked_list.h"

namespace sio {

TEST(LinkedList, Link) {

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


TEST(LinkedList, Basic) {
  struct A {
    Link link;
    char* c;
    int i;
  };

  Owner<A*> a1 = new A;
  Owner<A*> a2 = new A;

  Owner<LinkedList<A>*> l = new LinkedList<A>(offsetof(A, link));
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

} // namespace sio
