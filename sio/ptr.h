#ifndef SIO_PTR_H
#define SIO_PTR_H

#include "absl/meta/type_traits.h"

namespace sio {

/* 
C pointers are great at pointing "things" but failing to express:
    - ownership
    - nullness
Note that these two properties are orthogonal.

To make things clear, annotation types are introduced here as a pointer-convention:
    - T* or Ref<T*>       denotes pointers without ownership & cannot be null
    - Owner<T*>           denotes pointers with    ownership & cannot be null
    - Optional<T*>        denotes pointers without ownership & can    be null
    - Optional<Owner<T*>> denotes pointers with    ownership & can    be null
With consistent use of this convention, raw pointers (T*) have less semantic burden.

Note these types are:
    - merely annotations(via aliasing), rather than compiler guarentees.
    - with zero runtime costs, can work seamlessly with C API/ABI
*/

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Owner = T;

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Optional = T;

/*
template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Ref = T;
*/

#define SIO_UNDEFINED nullptr

};

#endif
