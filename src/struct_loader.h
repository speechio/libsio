#ifndef SIO_STRUCT_LOADER
#define SIO_STRUCT_LOADER

#include <map>

#include "sio/log.h"
#include "sio/check.h"
#include "sio/str.h"
#include "sio/vec.h"
#include "sio/json.h"

namespace sio {
class StructLoader {
 public:
  void Load(const Json& j) {
    for (const auto& kv : bool_map_) {
      auto& k = kv.first;
      auto& v = kv.second;
      const Json *node = FindEntry(&j, k);
      if (node) {
        *v = node->get<bool>();
      }
    }

    for (const auto& kv : int_map_) {
      auto& k = kv.first;
      auto& v = kv.second;
      const Json *node = FindEntry(&j, k);
      if (node) {
        *v = node->get<int>();
      }
    }

    for (const auto& kv : float_map_) {
      auto& k = kv.first;
      auto& v = kv.second;
      const Json *node = FindEntry(&j, k);
      if (node) {
        *v = node->get<float>();
      }
    }

    for (const auto& kv : string_map_) {
      auto& k = kv.first;
      auto& v = kv.second;
      const Json *node = FindEntry(&j, k);
      if (node) {
        *v = node->get<Str>();
      }
    }
  }


  void AddEntry(const StrView module, const StrView field, bool* p) {
    bool_map_[absl::StrCat(module, field)] = p;
  }

  void AddEntry(const StrView module, const StrView field, int* p) {
    int_map_[absl::StrCat(module, field)] = p;
  }

  void AddEntry(const StrView module, const StrView field, float* p) {
    float_map_[absl::StrCat(module, field)] = p;
  }

  void AddEntry(const StrView module, const StrView field, Str* p) {
    string_map_[absl::StrCat(module, field)] = p;
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
  static const Json* FindEntry(const Json *root, StrView entry) {
    SIO_CHECK(root != nullptr);

    // longest path match
    Vec<Str> steps = absl::StrSplit(entry, ".");
    const Json *node = root;
    int k = 0;
    while(k != steps.size()) {
      Str& field = steps[k];
      if (field == "") {
        ++k;
        continue;
      }

      if (node->contains(field)) {
        node = &((*node)[field]);
        ++k;
      } else {
        break;
      }
    }

    if (k == steps.size()) {
      return node;
    } else { // non-terminal node
      return nullptr;
    }
  }

 private:
  std::map<Str, bool*> bool_map_;
  std::map<Str, int*> int_map_;
  std::map<Str, float*> float_map_;
  std::map<Str, Str*> string_map_;

}; // End of class StructLoader
}  // End of namespace sio

#endif
