#include <iostream>
#include "sio/base.h"

using namespace sio;
struct Person {
    i32 age = 0;
    str name = "";
};

template<typename T>
struct Slice {
    ref<T*> items = nullptr;
    i64 len = 0;
    i64 cap = 0;
};

int main() {
    Person x;
    Slice<int> y;
    P_COND(true);
    vec<str> v;
    vec<vec<str>> k;
    str s = absl::StrJoin(v, "-");
    str_view sv = s;
    INVARIANT(true);
  
    std::cout << "Joined string: " << sv << "\n";
    i32 i = 10;
    ref<i32*> p = &i;
    std::cout << absl::StrFormat("%d\n", *p);

    gmap<int, str> m = {{1, "aaa"}, {2, "bbb"}, {3, "ccc"}};
    m[123] = "abcd";

    for (auto& kv : m) {
        std::cout << kv.first << ":" << kv.second << "\n";
    }

    for (auto it = m.begin(); it != m.end(); ++it) {
        std::cout << it->first << ":" << it->second << "\n";
    }

    index_t j = 0;

    Q_COND(true);
}
