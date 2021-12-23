#ifndef SIO_CONFIG_LOADER
#define SIO_CONFIG_LOADER

#include <map>
#include <fstream>
#include <iostream>

#include "sio/log.h"
#include "sio/check.h"
#include "sio/str.h"
#include "sio/vec.h"
#include "sio/json.h"

namespace sio {
class ConfigLoader {
 public:
  void Load(const std::string& config_file) {
    std::ifstream is(config_file);
    Json j;
    is >> j;

    for (const auto& kv : bool_map_) {
      auto& k = kv.first;
      auto& v = kv.second;
      const Json *p = WalkJsonTree(&j, absl::StrSplit(k, "."));
      if (p) {
        *v = p->get<bool>();
      }
    }

    for (const auto& kv : int_map_) {
      auto& k = kv.first;
      auto& v = kv.second;
      const Json *p = WalkJsonTree(&j, absl::StrSplit(k, "."));
      if (p) {
        *v = p->get<int>();
      }
    }

    for (const auto& kv : float_map_) {
      auto& k = kv.first;
      auto& v = kv.second;
      const Json *p = WalkJsonTree(&j, absl::StrSplit(k, "."));
      if (p) {
        *v = p->get<float>();
      }
    }

    for (const auto& kv : string_map_) {
      auto& k = kv.first;
      auto& v = kv.second;
      const Json *p = WalkJsonTree(&j, absl::StrSplit(k, "."));
      if (p) {
        *v = p->get<Str>();
      }
    }
  }


  void Add(StrView prefix, StrView entry, bool* p) {
    bool_map_[absl::StrCat(prefix, entry)] = p;
  }

  void Add(StrView prefix, StrView entry, int* p) {
    int_map_[absl::StrCat(prefix, entry)] = p;
  }

  void Add(StrView prefix, StrView entry, float* p) {
    float_map_[absl::StrCat(prefix, entry)] = p;
  }

  void Add(StrView prefix, StrView entry, std::string* p) {
    string_map_[absl::StrCat(prefix, entry)] = p;
  }


  void Print() {
    SIO_INFO << "--------------------";
    for (const auto& kv : bool_map_) {
      SIO_INFO << kv.first << " : " << *kv.second;
    }
    for (const auto& kv : int_map_) {
      SIO_INFO << kv.first << " : " << *kv.second;
    }
    for (const auto& kv : float_map_) {
      SIO_INFO << kv.first << " : " << *kv.second;
    }
    for (const auto& kv : string_map_) {
      SIO_INFO << kv.first << " : " << *kv.second;
    }
    SIO_INFO << "====================";
  }


 private:
  const Json* WalkJsonTree(const Json *from, const Vec<Str>& path) {
    SIO_CHECK(from != nullptr);
    SIO_CHECK_GE(path.size(), 2);
    SIO_CHECK_EQ(path[0], "");

    const Json *p = from;
    int i = 1;
    for (; i != path.size(); i++) {
      if (p->contains(path[i])) {
        p = &(*p)[path[i]];
      } else {
        break;
      }
    }

    if (i != path.size()) {
      p = nullptr;
    }

    return p;
  }

 private:
  std::map<std::string, bool*> bool_map_;
  std::map<std::string, int*> int_map_;
  std::map<std::string, float*> float_map_;
  std::map<std::string, std::string*> string_map_;

}; // End of class ConfigLoader
}  // End of namespace sio

#endif
