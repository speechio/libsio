#include "sio/link.h"
namespace sio {

Link::Link() :
  prev_(this),
  next_(this)
{ }

Link::~Link() {
  Unlink();
}

Ref<Link*> Link::Prev() { 
  return prev_;
}

Ref<Link*> Link::Next() { 
  return next_;
}
  
void Link::InsertBefore(Ref<Link*> ref) {
  prev_ = ref->prev_;
  next_ = ref;

  prev_->next_ = this;
  ref->prev_  = this;
}

void Link::InsertAfter(Ref<Link*> ref) {
  prev_ = ref;
  next_ = ref->next_;

  ref->next_  = this;
  next_->prev_ = this;
}

bool Link::IsLinked() {
  return (prev_ != this || next_ != this);
} 

void Link::Unlink() {
  prev_->next_ = next_;
  next_->prev_ = prev_;

  prev_ = this;
  next_ = this;
}

} // namespace sio