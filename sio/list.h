#ifndef SIO_LIST_H
#define SIO_LIST_H

#include "sio/ptr.h"
#include "sio/link.h"

namespace sio {

template <typename T>
class List {
public:
  List(size_t offset) { offset_ = offset; }
  ~List() { UnlinkAll(); }

  bool IsEmpty(){ return (Head() == nullptr); }

  // O(n)
  size_t NumElems() {
    size_t n = 0;
    for (Optional<T*> e = Head(); e != nullptr; e = Next(e)) {
      n++;
    }
    return n;
  }

  Optional<T*> Head() { return NodeOf(origin_.Next()); }
  Optional<T*> Tail() { return NodeOf(origin_.Prev()); }

  Optional<T*> Prev(T* node) { return NodeOf(LinkOf(node)->Prev()); }
  Optional<T*> Next(T* node) { return NodeOf(LinkOf(node)->Next()); }

  void InsertHead(T* node) { 
    UnlinkNode(node);
    LinkOf(node)->InsertAfter(&origin_);
  }

  void InsertTail(T* node) {
    UnlinkNode(node);
    LinkOf(node)->InsertBefore(&origin_);
  }

  void InsertBefore(T* node, T* ref) {
    UnlinkNode(node);
    LinkOf(node)->InsertBefore(LinkOf(ref));
  }

  void InsertAfter(T* node, T* ref) {
    UnlinkNode(node);
    LinkOf(node)->InsertAfter(LinkOf(ref));
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

  Optional<T*> NodeOf(Link* link) {
      return (link == &origin_) ? nullptr : (T*) ((size_t)link - offset_);
  }

  Link   origin_;
  size_t offset_;
};

} // namespace sio
#endif
