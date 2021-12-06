#ifndef SIO_LINK_H
#define SIO_LINK_H
#include "sio/ptr.h"
namespace sio {
class Link {
public:
  Link();
  ~Link();

  bool  IsLinked();
  Ref<Link*> Prev();
  Ref<Link*> Next();
  void  InsertBefore(Ref<Link*> ref);
  void  InsertAfter(Ref<Link*> ref);
  void  Unlink();

private:
  Ref<Link*> prev_;
  Ref<Link*> next_;
};

} // namespace sio
#endif
