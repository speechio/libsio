#ifndef SIO_JSON_H
#define SIO_JSON_H

// Single-header librariy "Json for Modern C++":
//   homepage:  https://github.com/nlohmann/json
//   license: MIT
//   commit: 825d3230d1f31448be906d3242cad9c3e4044733
//   date of copy: 2021.12.23 
//   document: https://json.nlohmann.me/

#include <nlohmann/json.hpp>

namespace sio {
using Json = nlohmann::json;
}

#endif

