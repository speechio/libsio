#ifndef SIO_LINK_H
#define SIO_LINK_H

#include "sio/ptr.h"

namespace sio {
class Link {
public:
  ~Link();

  bool  IsLinked();
  Link* Prev();
  Link* Next();
  void  InsertBefore(Link* ref);
  void  InsertAfter(Link* ref);
  void  Unlink();

private:
  Link* prev_ = this;
  Link* next_ = this;
};

} // namespace sio
#endif
