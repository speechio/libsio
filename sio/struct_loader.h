#ifndef SIO_STRUCT_LOADER
#define SIO_STRUCT_LOADER

#include <map>
#include <fstream>

#include "sio/base.h"
#include "sio/error.h"
#include "sio/logger.h"
#include "sio/check.h"
#include "sio/ptr.h"
#include "sio/type.h"
#include "sio/vec.h"
#include "sio/str.h"
#include "sio/json.h"

namespace sio {
struct StructLoader {
    std::map<Str, bool*> bool_map;
    std::map<Str, int*> int_map;
    std::map<Str, f32*> float_map;
    std::map<Str, Str*> string_map;


    Error Load(const Json& json) {
        for (const auto& kv : bool_map) {
            auto& e = kv.first;
            auto& p = kv.second;
            const Json *node = FindEntry(json, e);
            if (node) {
                *p = node->get<bool>();
            }
        }

        for (const auto& kv : int_map) {
            auto& e = kv.first;
            auto& p = kv.second;
            const Json *node = FindEntry(json, e);
            if (node) {
                *p = node->get<int>();
            }
        }

        for (const auto& kv : float_map) {
            auto& e = kv.first;
            auto& p = kv.second;
            const Json *node = FindEntry(json, e);
            if (node) {
                *p = node->get<f32>();
            }
        }

        for (const auto& kv : string_map) {
            auto& e = kv.first;
            auto& p = kv.second;
            const Json *node = FindEntry(json, e);
            if (node) {
                *p = node->get<Str>();
            }
        }

        return Error::OK;
    }


    Error Load(const Str& json_file) {
        Json j;

        std::ifstream json_stream(json_file);
        SIO_CHECK(json_stream.good()) << "Cannot open file: " << json_file;

        json_stream >> j;
        Load(j);

        return Error::OK;
    }


    void AddEntry(const Str& e, bool* addr) { bool_map[e] = addr; }
    void AddEntry(const Str& e, int* addr) { int_map[e] = addr; }
    void AddEntry(const Str& e, f32* addr) { float_map[e] = addr; }
    void AddEntry(const Str& e, std::string* addr) { string_map[e] = addr; }


    void Print() {
        SIO_INFO << "--------------------";
        for (const auto& kv : bool_map) {
            SIO_INFO << kv.first << " : " << *kv.second << " (bool) ";
        }
        for (const auto& kv : int_map) {
            SIO_INFO << kv.first << " : " << *kv.second << " (int) ";
        }
        for (const auto& kv : float_map) {
            SIO_INFO << kv.first << " : " << *kv.second << " (float) ";
        }
        for (const auto& kv : string_map) {
            SIO_INFO << kv.first << " : " << *kv.second << " (string) ";
        }
        SIO_INFO << "====================";
    }


private:
    static Nullable<const Json*> FindEntry(const Json& json, const StrView entry) {
        // longest path match
        Vec<Str> fields = absl::StrSplit(entry, ".", absl::SkipWhitespace());
        const Json* node = &json;
        int k = 0;
        // Loop invariant: the path from root to node matches fields[0,k)
        while(k != fields.size()) {
            Str& field = fields[k];
            if (node->contains(field)) {
                node = &((*node)[field]);
                ++k;
            } else {
                break;
            }
        }

        if (k == fields.size()) { // successful full match
            return node;
        } else { // non-terminal partial match
            return nullptr;
        }
    }

}; // End of class StructLoader
}  // End of namespace sio

#endif
