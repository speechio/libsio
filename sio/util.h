#ifndef SIO_UTIL_H
#define SIO_UTIL_H

namespace sio {

template<typename T>
inline void Delete(T& p) {
    delete p;
    p = nullptr;
}

template <typename T>
inline void DeleteArray(T& p) {
    delete [] p;
    p = nullptr;
}

} // namespace sio
#endif

