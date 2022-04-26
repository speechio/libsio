#include "sio/allocator.h"

#include "gtest/gtest.h"
namespace sio {

TEST(Allocator, SlabAllocator) {

    struct S {
        char c;
        int i;
        void *p;
    };

    SlabAllocator<S> pool;
    pool.SetSize(2);

    EXPECT_EQ(pool.NumUsed(), 0);
    EXPECT_EQ(pool.NumFree(), 0);

    S* s1 = pool.Alloc();
    EXPECT_EQ(pool.NumUsed(), 1);
    EXPECT_EQ(pool.NumFree(), 1);

    S* s2 = pool.Alloc();
    EXPECT_EQ(pool.NumUsed(), 2);
    EXPECT_EQ(pool.NumFree(), 0);

    pool.Free(s2);
    EXPECT_EQ(pool.NumUsed(), 1);
    EXPECT_EQ(pool.NumFree(), 1);

    S* s3 = pool.Alloc();
    S* s4 = pool.Alloc(); // trigger another slab growth
    EXPECT_EQ(pool.NumUsed(), 3);
    EXPECT_EQ(pool.NumFree(), 1);

    pool.Free(s1);
    pool.Free(s3);
    pool.Free(s4);

    pool.Reset();
    EXPECT_EQ(pool.NumUsed(), 0);
    EXPECT_EQ(pool.NumFree(), 0);

    S* s5 = pool.Alloc();
    S* s6 = pool.Alloc();
    S* s7 = pool.Alloc();
    EXPECT_EQ(pool.NumUsed(), 3);
    EXPECT_EQ(pool.NumFree(), 1);

    // s5, s6, s7 will not leak, 
    // pool cleanup itself on destruction
}

} // namespace sio
