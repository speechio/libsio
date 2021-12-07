#ifndef SIO_PTR_H
#define SIO_PTR_H

#include "absl/meta/type_traits.h"

namespace sio {

/* 
C pointers are great at pointing "things", but they failed expressing:
    - ownerships
    - nullness

Pointer annotations are introduced here:
    - T* or Ref<T*>  denotes pointers without ownership & cannot be nullptr
    - Owner<T*>      denotes pointers with    ownership & cannot be nullptr
    - Opt<T*>        denotes pointers without ownership & can    be nullptr
    - Opt<Owner<T*>> denotes pointers with    ownership & can    be nullptr

Explicit and consistent use of these types provides extra semantics that are missing in C pointers.
And throughout this codebase, any C pointer(i.e. T*) should be converted to one of these. 

But notes these types:
    - serve as merely annotations between programmers, rather than compiler guarentees.
    - are just aliasings to C pointers, with zero runtime cost
    - work seamlessly with C API/ABI
*/

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Owner = T;

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Opt = T;

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Ref = T;

#define SIO_UNDEFINED nullptr

};

#endif
