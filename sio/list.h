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

  Optional<T*> Prev(Ref<T*> node) { return NodeOf(LinkOf(node)->Prev()); }
  Optional<T*> Next(Ref<T*> node) { return NodeOf(LinkOf(node)->Next()); }

  void InsertHead(Ref<T*> node) { 
    UnlinkNode(node);
    LinkOf(node)->InsertAfter(&origin_);
  }

  void InsertTail(Ref<T*> node) {
    UnlinkNode(node);
    LinkOf(node)->InsertBefore(&origin_);
  }

  void InsertBefore(Ref<T*> node, Ref<T*> ref) {
    UnlinkNode(node);
    LinkOf(node)->InsertBefore(LinkOf(ref));
  }

  void InsertAfter(Ref<T*> node, Ref<T*> ref) {
    UnlinkNode(node);
    LinkOf(node)->InsertAfter(LinkOf(ref));
  }

  void UnlinkNode(Ref<T*> node) {
    Ref<Link*> link = LinkOf(node);
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
  Ref<Link*> LinkOf(Ref<T*> node) {
      return Ref<Link*>((size_t)node + offset_);
  }

  Optional<T*> NodeOf(Ref<Link*> link) {
      return (link == &origin_) ? nullptr : Ref<T*>((size_t)link - offset_);
  }

  Link   origin_;
  size_t offset_;
};

} // namespace sio
#endif
