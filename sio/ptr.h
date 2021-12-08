#ifndef SIO_PTR_H
#define SIO_PTR_H

#include "absl/meta/type_traits.h"

namespace sio {

/* 
C pointers are great at pointing "things", but they failed expressing:
    - ownerships
    - nullness
These two properties are orthogonal.

Pointer annotations introduced:
    - T* or Ref<T*>  denotes pointers without ownership & cannot be nullptr
    - Owner<T*>      denotes pointers with    ownership & cannot be nullptr
    - Opt<T*>        denotes pointers without ownership & can    be nullptr
    - Opt<Owner<T*>> denotes pointers with    ownership & can    be nullptr
With consistent use of this convention, raw pointers (T*) have less semantic burden.

Note these types are:
    - merely annotations(via aliasing), rather than compiler guarentees.
    - with zero runtime costs, can work seamlessly with C API/ABI
*/

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Owner = T;

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Opt = T;

/*
template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Ref = T;
*/

#define SIO_UNDEFINED nullptr

};

#endif
