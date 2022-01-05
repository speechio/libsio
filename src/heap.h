#ifndef SIO_HEAP_H
#define SIO_HEAP_H

#include "sio/ptr.h"
#include "sio/check.h"

namespace sio {

template <typename T>
class SlabAllocator {
public:
	SlabAllocator(size_t elems_per_cache) {
		static_assert(sizeof(T) >= sizeof(FreeNode*) && "element size should be larger than a pointer");
		SIO_CHECK_GE(elems_per_cache, 1);
		elem_bytes_ = sizeof(T);
		elems_per_cache_ = elems_per_cache;
		num_used_ = 0;
	}

	~SlabAllocator() {
		for(int i = 0; i < caches_.size(); i++) {
			delete[] caches_[i];
		}
	}

	inline T* Alloc() {
		if (free_list_.IsEmpty()) {
			char* p = new char[elem_bytes_ * elems_per_cache_];
			caches_.push_back( (Owner<char*>)p );
			for (int i = 0; i < elems_per_cache_; i++) {
				free_list_.Push((FreeNode*)(p + i * elem_bytes_));
			}
		}
		num_used_++;
		return (T*)free_list_.Pop();
	}

	inline void Free(T* p) {
		num_used_--;
		free_list_.Push( (FreeNode*)p );
	}

	size_t NumUsed() {
		return num_used_;
	}

	size_t NumFree() {
		size_t n = 0;
		for (Optional<FreeNode*> p = free_list_.head; p != nullptr; p = p->next) {
			n++;
		}
		return n;
	}

private:
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
			SIO_CHECK(!IsEmpty()); // grow should happen outside, in Alloc()
			FreeNode* p = head;
			head = head->next;
			return p;
		}
	};

private:
	size_t elem_bytes_;
	size_t elems_per_cache_;
	std::vector<Owner<char*>> caches_;

	FreeList free_list_;
	size_t num_used_;
};

} // namespace sio
#endif
