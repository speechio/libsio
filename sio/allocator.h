#ifndef SIO_ALLOCATOR_H
#define SIO_ALLOCATOR_H

#include "sio/ptr.h"
#include "sio/check.h"
#include "sio/vec.h"
#include "sio/dbg_macro.h"

namespace sio {

/*
*                      size1
*            ┌─────┬─────┬─────┐
*            │size0│size0│size0│ ..
*            ├─────┼─────┼─────┘
*            │size0│size0│ ...
*      size2 ├─────┼─────┘
*            │size0│ ...
*            └─────┘
*              ...
*
*  1. A slab caches a 2D matrix of elements, allocated by operating system all at once.
*  2. Each Alloc() yields a row by returning a T* to first elem of the row.
*  3. Each Free() reclaims a row back to internal free list.
*
*/

struct FreeNode {
    Nullable<FreeNode*> next = nullptr;
};

template <typename T>
class SlabAllocator {
    // A slab is a blob of raw memory with size0_ * size1_ * sizeof(T) bytes.
    size_t size0_ = 0; // element size in bytes
    size_t size1_ = 0; // num of elements per alloc
    size_t size2_ = 0; // num of allocs per slab

    vec<vec<char>> slabs_;
    Nullable<FreeNode*> free_list_ = nullptr;

    size_t num_used_ = 0;
    size_t num_free_ = 0;

public:

    void SetSize(size_t size2, size_t size1 = 1, size_t size0 = sizeof(T)) {
        SIO_CHECK(slabs_.empty());
        SIO_CHECK_GE(size0 * size1, sizeof(FreeNode*)); // each allocation should be at least as large as a pointer.
        SIO_CHECK_GE(size2, 1);

        size0_ = size0;
        size1_ = size1;
		size2_ = size2;
    }


    inline T* Alloc() {
        if (free_list_ == nullptr) {
            slabs_.emplace_back();
            vec<char>& slab = slabs_.back();
            slab.resize(size0_ * size1_ * size2_);

            char* p = slab.data();
            for (size_t i = 0; i < size2_; i++) {
                FreeListPush((FreeNode*)p);
                p += size0_ * size1_;
            }
        }

        dbg(num_used_++);
        return (T*) FreeListPop();
    }


    inline void Free(T* p) {
        num_used_--;
        FreeListPush( (FreeNode*)p );
    }


    size_t NumUsed() const { return num_used_; }
    size_t NumFree() const { return num_free_; }


    void Reset() {
        slabs_.clear();
        free_list_ = nullptr;

        num_used_ = 0;
        num_free_ = 0;
    }

private:

    inline void FreeListPush(FreeNode* p) {
        p->next = free_list_;
        free_list_ = p;
        ++num_free_;
    }


    inline FreeNode* FreeListPop() {
        SIO_CHECK(free_list_ != nullptr); // slabs should grow outside Pop(), in Alloc()
        FreeNode* p = free_list_;
        free_list_ = free_list_->next;
        --num_free_;
        return p;
    }

}; // class SlabAllocator
} // namespace sio
#endif
