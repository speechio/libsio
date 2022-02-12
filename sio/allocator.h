#ifndef SIO_ALLOCATOR_H
#define SIO_ALLOCATOR_H

#include "sio/ptr.h"
#include "sio/check.h"
#include "sio/vec.h"

namespace sio {

struct FreeNode {
    Nullable<FreeNode*> next = nullptr;
};


struct FreeList {
    Nullable<FreeNode*> head = nullptr;
    size_t length = 0;

    inline bool IsEmpty() {
        return (head == nullptr);
    }

    inline void Push(FreeNode* p) {
        p->next = head;
        head = p;
        length++;
    }

    inline FreeNode* Pop() {
        SIO_CHECK(!IsEmpty()); // caches should grow outside Pop(), in Alloc()
        FreeNode* p = head;
        head = head->next;
        length--;
        return p;
    }
};


template <typename T>
class SlabAllocator {
    // One cache is a blob of raw memory with size0_ * size1_ * sizeof(T) bytes.
    size_t size0_ = 4096; // num of allocations in a cache
    size_t size1_ = 1;    // num of elems(of type T) in an allocation
    Vec<Vec<char>> caches_;

    FreeList free_list_;
    size_t num_used_ = 0;

public:

    void SetCacheSize(size_t size0, size_t size1 = 1) {
        SIO_CHECK(caches_.empty()) << "Cache size need to be set before uses.";
        SIO_CHECK_GE(size0, 1);
        SIO_CHECK_GE(size1 * sizeof(T), sizeof(FreeNode*)) \
            << "Cannot support allocations smaller than a pointer.";

        size0_ = size0;
        size1_ = size1;
    }


    inline T* Alloc() {
        if (free_list_.IsEmpty()) {
            caches_.resize(caches_.size() + 1);
            Vec<char>& c = caches_.back();
            c.resize(size0_ * size1_ * sizeof(T));

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


    size_t NumUsed() const { return num_used_; }
    size_t NumFree() const { return free_list_.length; }


    void clear() {
        num_used_ = 0;

        free_list_.head = nullptr;
        free_list_.length = 0;

        caches_.clear();
    }

}; // class SlabAllocator
} // namespace sio
#endif
