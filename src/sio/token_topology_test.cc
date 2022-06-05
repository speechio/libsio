#include "sio/token_topology.h"

#include <gtest/gtest.h>
#include <fstream>

namespace sio {
TEST(TokenTopology, Build) {
    Fst topo;
    Tokenizer tokenizer;
    tokenizer.Load("testdata/tokenizer.vocab");
    BuildTokenTopology(tokenizer, &topo);
    topo.DumpToText(std::cout);
    {
        std::ofstream os("testdata/T3.fst", std::ios::binary);
        topo.Dump(os);
    }
}
} // namespace sio
