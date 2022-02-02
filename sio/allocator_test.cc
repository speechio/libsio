#include "gtest/gtest.h"
#include "sio/allocator.h"

namespace sio {

TEST(Allocator, SlabAllocator) {

    struct S {
        char c;
        int i;
        void *p;
    };

    size_t elems_per_cache = 2;
    Owner<SlabAllocator<S>*> p = new SlabAllocator<S>(elems_per_cache);
    EXPECT_TRUE(p->NumUsed() == 0);
    EXPECT_TRUE(p->NumFree() == 0);

    S *s1 = p->Alloc();
    EXPECT_TRUE(p->NumUsed() == 1);
    EXPECT_TRUE(p->NumFree() == 1);

    S *s2 = p->Alloc();
    EXPECT_TRUE(p->NumUsed() == 2);
    EXPECT_TRUE(p->NumFree() == 0);

    p->Free(s2);
    EXPECT_TRUE(p->NumUsed() == 1);
    EXPECT_TRUE(p->NumFree() == 1);

    S *s3 = p->Alloc();
    S *s4 = p->Alloc(); // trigger another slab allocation
    EXPECT_TRUE(p->NumUsed() == 3);
    EXPECT_TRUE(p->NumFree() == 1);

    p->Free(s1);
    p->Free(s3);
    p->Free(s4);
    delete p;
}

} // namespace sio
