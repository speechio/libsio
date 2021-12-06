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

  Optional<T*> Head() { return GetNodeFromLink(origin_.Next()); }
  Optional<T*> Tail() { return GetNodeFromLink(origin_.Prev()); }

  Optional<T*> Prev(Ref<T*> node) { return GetNodeFromLink(GetLinkFromNode(node)->Prev()); }
  Optional<T*> Next(Ref<T*> node) { return GetNodeFromLink(GetLinkFromNode(node)->Next()); }

  void InsertHead(Ref<T*> node) { 
    UnlinkNode(node);
    GetLinkFromNode(node)->InsertAfter(&origin_);
  }

  void InsertTail(Ref<T*> node) {
    UnlinkNode(node);
    GetLinkFromNode(node)->InsertBefore(&origin_);
  }

  void InsertBefore(Ref<T*> node, Ref<T*> ref) {
    UnlinkNode(node);
    GetLinkFromNode(node)->InsertBefore(GetLinkFromNode(ref));
  }

  void InsertAfter(Ref<T*> node, Ref<T*> ref) {
    UnlinkNode(node);
    GetLinkFromNode(node)->InsertAfter(GetLinkFromNode(ref));
  }

  void UnlinkNode(Ref<T*> node) {
    Ref<Link*> link = GetLinkFromNode(node);
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
  Ref<Link*> GetLinkFromNode(Ref<T*> node) {
      return Ref<Link*>((size_t)node + offset_);
  }

  Optional<T*> GetNodeFromLink(Ref<Link*> link) {
      return (link == &origin_) ? nullptr : Ref<T*>((size_t)link - offset_);
  }

  Link   origin_;
  size_t offset_;
};

} // namespace sio
#endif
