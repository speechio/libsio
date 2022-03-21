#ifndef SIO_ALLOCATOR_H
#define SIO_ALLOCATOR_H

#include "sio/ptr.h"
#include "sio/check.h"
#include "sio/vec.h"

namespace sio {

struct FreeNode {
    Nullable<FreeNode*> next = nullptr;
};

template <typename T>
class SlabAllocator {
    // A slab is a blob of raw memory with size0_ * size1_ * sizeof(T) bytes.
    size_t size0_ = 4096; // num of allocs per slab
    size_t size1_ = 1;    // num of elems(of type T) per alloc

    Vec<Vec<char>> slabs_;
    Nullable<FreeNode*> free_list_ = nullptr;

    size_t num_used_ = 0;
    size_t num_free_ = 0;

public:

    void SetSlabSize(size_t size0, size_t size1 = 1) {
        SIO_CHECK(slabs_.empty()) << "Allocator already in use, cannot reset slab size.";
        SIO_CHECK_GE(size0, 1);
        SIO_CHECK_GE(size1 * sizeof(T), sizeof(FreeNode*)) \
            << "Cannot support allocations smaller than a pointer.";

        size0_ = size0;
        size1_ = size1;
    }


    inline T* Alloc() {
        if (free_list_ == nullptr) {
            slabs_.resize(slabs_.size() + 1);
            Vec<char>& s = slabs_.back();
            s.resize(size0_ * size1_ * sizeof(T));

            char* p = s.data();
            for (int i = 0; i < size0_; i++) {
                FreeListPush((FreeNode*)p);
                p += size1_ * sizeof(T);
            }
        }

        num_used_++;
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
