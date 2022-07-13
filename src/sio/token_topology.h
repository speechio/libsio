#ifndef SIO_TOKEN_TOPOLOGY_H
#define SIO_TOKEN_TOPOLOGY_H

#include "sio/tokenizer.h"
#include "sio/finite_state_transducer.h"

namespace sio {

Error BuildTokenTopology(const Tokenizer& tokenizer, Fst* topo) {
    SIO_CHECK(topo != nullptr);
    SIO_CHECK(topo->Empty());
    SIO_CHECK_NE(tokenizer.Size(), 0);

    /* 1: Build Fst arcs */
    {
        // 1a: Blank self-loop of start state
        topo->start_state = 0;
        topo->AddArc(topo->start_state, topo->start_state, tokenizer.blk, kFstEpsilon);

        // 1b: Arcs of normal tokens
        FstStateId cur_state = 1; // 0 is already occupied by start state
        // Invariant: arcs for states[0, cur_state) & tokens[0, t) are built.
        for (TokenId t = 0; t != tokenizer.Size(); t++) {
            if (t == tokenizer.blk) continue;
            if (t == tokenizer.unk) continue;
            if (t == tokenizer.bos) continue;
            if (t == tokenizer.eos) continue;

            topo->AddArc(topo->start_state, cur_state,         t,           t          ); // Entering
            topo->AddArc(cur_state,         cur_state,         t,           kFstEpsilon); // Self-loop
            topo->AddArc(cur_state,         topo->start_state, kFstEpsilon, kFstEpsilon); // Leaving
            cur_state++;
        }

        // 1c: "InputEnd" represents the end of input sequence (follows K2Fsa convention)
        topo->final_state = cur_state;
        topo->AddArc(topo->start_state, topo->final_state, kFstFinalSymbol, tokenizer.eos);

        // 1d: Sort all arcs, first by source state, then by ilabel
        std::sort(topo->arcs.begin(), topo->arcs.end(), 
            [](const FstArc& x, const FstArc& y) { 
                return (x.src != y.src) ? (x.src < y.src) : (x.ilabel < y.ilabel);
            }
        );
    }

    /* 2: Setup states */
    {
        i64 num_states = topo->final_state + 1;
        topo->states.reserve(num_states);

        vec<int> odegree(num_states, 0);
        for (const auto& arc : topo->arcs) {
            odegree[arc.src]++;
        }

        // invariant: k = sum( arcs of states[0, s) )
        int k = 0;
        for (FstStateId s = 0; s != num_states; s++) {
            topo->states.push_back({.offset = k, .num_arcs = odegree[s]});
            k += odegree[s];
        }

        SIO_CHECK_EQ(k, topo->arcs.size());
    }

    return Error::OK;
}


}; // namespace sio

#endif
