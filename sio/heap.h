#ifndef SIO_HEAP_H
#define SIO_HEAP_H

#include "sio/ptr.h"
#include "sio/check.h"

namespace sio {

template <typename T>
class ArenaAllocator {
 public:
  ArenaAllocator(size_t num_elems_per_slab) {
    static_assert(sizeof(T) >= sizeof(FreeNode*) && "element size should be larger than a pointer");
    SIO_P_COND(num_elems_per_slab >= 1);
    elem_bytes_ = sizeof(T);
    num_elems_per_slab_ = num_elems_per_slab;
    num_used_ = 0;
  }

  ~ArenaAllocator() {
    for(int i = 0; i < slabs_.size(); i++) {
      delete[] slabs_[i];
    }
  }

  inline Ref<T*> Alloc() {
    if (free_list_.IsEmpty()) {
      Ref<char*> p = new char[elem_bytes_ * num_elems_per_slab_];
      // C++ standard guarentees the return of "new" is non-null, no need to check
      slabs_.push_back((Owner<char*>)p);
      for (int i = 0; i < num_elems_per_slab_; i++) {
        free_list_.Push((Ref<FreeNode*>)(p + i * elem_bytes_));
      }
    }
    num_used_++;
    return (Ref<T*>) free_list_.Pop();
  }

  inline void Free(Ref<T*> p) {
    num_used_--;
    free_list_.Push(Ref<FreeNode*>(p));
  }

  size_t NumUsed() {
    return num_used_;
  }

  size_t NumFree() {
    size_t n = 0;
    for (Opt<FreeNode*> p = free_list_.head; p != nullptr; p = p->next) {
      n++;
    }
    return n;
  }

 private:
  struct FreeNode {
    Opt<FreeNode*> next = nullptr;
  };

  struct FreeList {
    Opt<FreeNode*> head = nullptr;

    inline bool IsEmpty() {
      return (head == nullptr);
    }

    inline void Push(Ref<FreeNode*> p) {
      p->next = head;
      head = p;
    }

    inline Ref<FreeNode*> Pop() {
      SIO_P_COND(!IsEmpty()); // Exhausted arena should grow in Alloc()
      Ref<FreeNode*> p = head;
      head = head->next;
      return p;
    }
  };

 private:
  size_t elem_bytes_;
  size_t num_elems_per_slab_;
  std::vector<Owner<char*>> slabs_;

  FreeList free_list_;
  size_t num_used_;
};

} // namespace sio
#endif
