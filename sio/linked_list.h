#ifndef SIO_LIST_H
#define SIO_LIST_H

#include "sio/ptr.h"

namespace sio {

class Link {
    Link* prev_ = this;
    Link* next_ = this;

public:
    ~Link() { Unlink(); }

    bool  IsLinked() { return (prev_ != this || next_ != this); }
    Link* Prev() { return prev_; }
    Link* Next() { return next_; }

    void  InsertBefore(Link* position) {
        prev_ = position->prev_;
        next_ = position;

        prev_->next_ = this;
        position->prev_  = this;
    }

    void  InsertAfter(Link* position) {
        prev_ = position;
        next_ = position->next_;

        position->next_  = this;
        next_->prev_ = this;
    }

    void  Unlink() {
        prev_->next_ = next_;
        next_->prev_ = prev_;

        prev_ = this;
        next_ = this;
    }

};


template <typename T>
class LinkedList {
    Link   origin_;
    size_t offset_;

public:
    LinkedList(size_t offset) { offset_ = offset; }
    ~LinkedList() { UnlinkAll(); }

    bool IsEmpty(){ return (Head() == nullptr); }

    // O(n)
    size_t NumElems() {
        size_t n = 0;
        for (Nullable<T*> e = Head(); e != nullptr; e = Next(e)) {
            n++;
        }
        return n;
    }

    Nullable<T*> Head() { return NodeOf(origin_.Next()); }
    Nullable<T*> Tail() { return NodeOf(origin_.Prev()); }

    Nullable<T*> Prev(T* node) { return NodeOf(LinkOf(node)->Prev()); }
    Nullable<T*> Next(T* node) { return NodeOf(LinkOf(node)->Next()); }

    void InsertHead(T* node) { 
        UnlinkNode(node);
        LinkOf(node)->InsertAfter(&origin_);
    }

    void InsertTail(T* node) {
        UnlinkNode(node);
        LinkOf(node)->InsertBefore(&origin_);
    }

    void InsertBefore(T* node, T* position) {
        UnlinkNode(node);
        LinkOf(node)->InsertBefore(LinkOf(position));
    }

    void InsertAfter(T* node, T* position) {
        UnlinkNode(node);
        LinkOf(node)->InsertAfter(LinkOf(position));
    }

    void UnlinkNode(T* node) {
        Link* link = LinkOf(node);
        if (link->IsLinked()) {
            link->Unlink();
        }
    }

    void UnlinkAll() {
        while (!IsEmpty()) {
            UnlinkNode(Head());
        }
    }

private:

    Link* LinkOf(T* node) {
        return (Link*) ((size_t)node + offset_);
    }

    Nullable<T*> NodeOf(Link* link) {
        return (link == &origin_) ? nullptr : (T*) ((size_t)link - offset_);
    }

};

} // namespace sio
#endif
