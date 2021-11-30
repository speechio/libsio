#ifndef SIO_PTR_H
#define SIO_PTR_H

#include "absl/meta/type_traits.h"

namespace sio {

/* 
C pointers are great at pointing "things", but they failed expressing:
    - ownerships
    - nullness

New pointer-types are introduced here:
    - owner<T*>           denotes pointers with    ownership
    - nullable<T*>        denotes pointers                     can    be nullptr
    - nullable<owner<T*>> denotes pointers with    ownership & can    be nullptr
    - ref<T*>             denotes pointers without ownership & cannot be nullptr

Explicit and consistent use of these types provides extra semantics that are missing in C pointers.
And throughout this codebase, any C pointer(i.e. T*) should be converted to one of these. 

But notes these types:
    - serve as merely annotations between programmers, rather than compiler guarentees.
    - are just aliasings to C pointers, with zero runtime cost
    - work seamlessly with C API/ABI
*/

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using owner = T;

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using nullable = T;

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using ref = T;

};

#endif
