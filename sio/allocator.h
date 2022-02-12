#ifndef SIO_ALLOCATOR_H
#define SIO_ALLOCATOR_H

#include "sio/ptr.h"
#include "sio/check.h"
#include "sio/vec.h"
#include "sio/dbg.h"

namespace sio {

struct FreeNode {
    Optional<FreeNode*> next = nullptr;
};


struct FreeList {
    Optional<FreeNode*> head = nullptr;

    inline bool IsEmpty() {
        return (head == nullptr);
    }

    inline void Push(FreeNode* p) {
        p->next = head;
        head = p;
    }

    inline FreeNode* Pop() {
        SIO_CHECK(!IsEmpty()); // caches should grow outside Pop(), in Alloc()
        FreeNode* p = head;
        head = head->next;
        return p;
    }
};


template <typename T>
class SlabAllocator {
    size_t size0_ = 4096; // default 4K items per cache
    size_t size1_ = 1;    // one item could contain 1 or more elems of type T
    Vec<Vec<char>> caches_;

    FreeList free_list_;
    size_t num_used_ = 0;

public:

    void SetCacheSize(size_t size0, size_t size1 = 1) {
        SIO_CHECK(caches_.empty()) << "Cache size need to be set before uses.";
        SIO_CHECK_GE(size0, 1);
        SIO_CHECK_GE(sizeof(T) * size1, sizeof(FreeNode*)) << "Size of allocator item should be larger than a pointer";

        size0_ = size0;
        size1_ = size1;
    }


    inline T* Alloc() {
        if (free_list_.IsEmpty()) {
            // add new cache
            caches_.resize(caches_.size() + 1);
            Vec<char>& c = caches_.back();
            c.resize(size0_ * size1_ * sizeof(T));

            // link new cache elems with free list
            char* p = c.data();
            for (int i = 0; i < size0_; i++) {
                free_list_.Push( (FreeNode*)(p + i * size1_ * sizeof(T)) );
            }
        }
        num_used_++;
        return (T*)free_list_.Pop();
    }


    inline void Free(T* p) {
        num_used_--;
        free_list_.Push( (FreeNode*)p );
    }


    size_t NumUsed() const {
        return num_used_;
    }


    size_t NumFree() const {
        size_t n = 0;
        for (Optional<FreeNode*> p = free_list_.head; p != nullptr; p = p->next) {
            n++;
        }
        return n;
    }


    void clear() {
        caches_.clear();
        free_list_.head = nullptr;
        num_used_ = 0;
    }

}; // class SlabAllocator
} // namespace sio
#endif
