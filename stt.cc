#include <iostream>
#include "sio/sio.h"
#include "online2/online-timing.h"

using namespace sio;
struct Person {
    i32 age = 0;
    Str name = "";
};

template<typename T>
struct Slice {
    T* items = SIO_UNDEFINED;
    i64 len = 0;
    i64 cap = 0;
};

int main() {
    kaldi::BaseFloat asdf = 0.0f;
    kaldi::OnlineTimingStats timing_stats;
    timing_stats.Print(true);
    Person x;
    Slice<int> y;
    SIO_P_COND(true);
    Vec<Str> v;
    Vec<Vec<Str>> k;
    Str s = absl::StrJoin(v, "-");
    StrView sv = s;
    SIO_INVAR(true);
  
    std::cout << "Joined String: " << sv << "\n";
    i32 i = 10;
    i32* p = &i;
    std::cout << absl::StrFormat("%d\n", *p);

    Map<i64, Str> m = {{1, "aaa"}, {2, "bbb"}, {3, "ccc"}};
    m[123] = "abcd";

    for (auto& kv : m) {
        std::cout << kv.first << ":" << kv.second << "\n";
    }

    for (auto it = m.begin(); it != m.end(); ++it) {
        std::cout << it->first << ":" << it->second << "\n";
    }

    index_t j = 0;

}
