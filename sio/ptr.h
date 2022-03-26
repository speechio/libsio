#ifndef SIO_PTR_H
#define SIO_PTR_H

#include <type_traits>

#include <absl/meta/type_traits.h>
#include <absl/memory/memory.h>

namespace sio {

/* 
 * GSL-like (Guideline Supported Library) pointer annotations for old style-C:
 * 
 * C pointers are great at pointing "things" but failing to express:
 *     - ownership
 *     - nullability
 * Note that these two properties are orthogonal to each other.
 * 
 * So to make C pointers clearer, aliasing annotations are introduced throughout this library:
 *     - T*                  denotes pointers without ownership & cannot be null
 *     - Owner<T*>           denotes pointers with    ownership & cannot be null
 *     - Nullable<T*>        denotes pointers without ownership & can    be null
 *     - Nullable<Owner<T*>> denotes pointers with    ownership & can    be null
 * With consistent use of this convention, raw pointers (T*) are just as safe as reference.
*/

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Owner = T;

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Nullable = T;


/*
 * Other than above helpers to annotate C pointers, 
 * Use C++11 smart pointers as an alternative if they simplify codes
*/
template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Unique = std::unique_ptr<typename std::remove_pointer<T>::type>;

template <typename T, typename = typename absl::enable_if_t<std::is_pointer<T>::value>>
using Shared = std::shared_ptr<typename std::remove_pointer<T>::type>;

}

#endif

