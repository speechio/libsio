#ifndef SIO_FINITE_STATE_MACHINE_H
#define SIO_FINITE_STATE_MACHINE_H

#include <limits>
#include <algorithm>

#include "base/io-funcs.h"

#include "sio/base.h"
#include "sio/tokenizer.h"

namespace sio {

using FsmStateId = i32;
using FsmArcId   = i32;
using FsmLabel   = i32;
using FsmScore   = f32;


constexpr FsmLabel kFsmInputEnd = -1; // This follows K2Fsa convention
constexpr FsmLabel kFsmEpsilon = std::numeric_limits<FsmLabel>::lowest();


struct FsmState {
    FsmArcId arcs_offset = 0;
};


struct FsmArc {
    FsmStateId src = 0;
    FsmStateId dst = 0;
    FsmLabel ilabel = 0;
    FsmLabel olabel = 0;
    FsmScore score = 0.0f;
};


class FsmArcIterator {
    const FsmArc* cur_ = nullptr;
    const FsmArc* end_ = nullptr;

public:
    FsmArcIterator(const FsmArc* begin, const FsmArc* end) : cur_(begin), end_(end) { }

    const FsmArc& Value() const { return *cur_; }

    void Next() { ++cur_; }

    bool Done() const { return cur_ >= end_; }
};


struct Fsm {
    Str version; // TODO: make version a part of binary header

    // Use i64 instead of size_t, for platform independent binary
    // TODO: bit/little endian compatibility
    i64 num_states = 0;
    i64 num_arcs = 0;

    FsmStateId start_state = 0;
    FsmStateId final_state = 0;

    Vec<FsmState> states;  // one extra sentinel at the end: states.size() = num_states + 1
    Vec<FsmArc> arcs;


    inline bool Empty() const { return this->states.empty(); }
    inline bool ContainEpsilonArc(FsmStateId s) const {
        // Preconditions:
        //   1. kFsmEpsilon should have smallest input symbol id.
        //   2. arcs of a FsmState need to be sorted by ilabels.
        const FsmArc& first_arc = this->arcs[this->states[s].arcs_offset];
        return (first_arc.ilabel == kFsmEpsilon);
    }


    FsmArcIterator GetArcIterator(FsmStateId i) const {
        SIO_CHECK(!Empty());
        SIO_CHECK_NE(i, this->states.size() - 1); // block external access to sentinel
        return FsmArcIterator(
            &this->arcs[this->states[i  ].arcs_offset],
            &this->arcs[this->states[i+1].arcs_offset]
        );
    }


    Error Load(std::istream& is) {
        SIO_CHECK(Empty()); // Can't reload
        SIO_CHECK(is.good());

        using kaldi::ExpectToken;
        using kaldi::ReadBasicType;

        bool binary = true;

        ExpectToken(is, binary, "<Fsm>");

        /*
        TODO: version handling here
        */

        ExpectToken(is, binary, "<NumStates>");
        ReadBasicType(is, binary, &this->num_states);

        ExpectToken(is, binary, "<NumArcs>");
        ReadBasicType(is, binary, &this->num_arcs);

        ExpectToken(is, binary, "<StartState>");
        ReadBasicType(is, binary, &this->start_state);
        SIO_CHECK(this->start_state == 0); // conform to K2

        ExpectToken(is, binary, "<FinalState>");
        ReadBasicType(is, binary, &this->final_state);
        SIO_CHECK_EQ(this->final_state, this->num_states - 1); // conform to K2

        ExpectToken(is, binary, "<States>");
        auto num_states_plus_sentinel = this->num_states + 1;
        this->states.resize(num_states_plus_sentinel);
        is.read(reinterpret_cast<char*>(this->states.data()), num_states_plus_sentinel * sizeof(FsmState));

        ExpectToken(is, binary, "<Arcs>");
        this->arcs.resize(this->num_arcs);
        is.read(reinterpret_cast<char*>(this->arcs.data()), this->num_arcs * sizeof(FsmArc));

        return Error::OK;
    }


    Error Dump(std::ostream& os) const {
        SIO_CHECK(!Empty());
        SIO_CHECK(os.good());

        using kaldi::WriteToken;
        using kaldi::WriteBasicType;

        bool binary = true;

        WriteToken(os, binary, "<Fsm>");

        /*
        TODO: version handling here
        */

        WriteToken(os, binary, "<NumStates>");
        WriteBasicType(os, binary, this->num_states);

        WriteToken(os, binary, "<NumArcs>");
        WriteBasicType(os, binary, this->num_arcs);

        WriteToken(os, binary, "<StartState>");
        WriteBasicType(os, binary, this->start_state);

        WriteToken(os, binary, "<FinalState>");
        WriteBasicType(os, binary, this->final_state);

        WriteToken(os, binary, "<States>");
        auto num_states_plus_sentinel = this->num_states + 1;
        os.write(reinterpret_cast<const char*>(this->states.data()), num_states_plus_sentinel * sizeof(FsmState));

        WriteToken(os, binary, "<Arcs>");
        os.write(reinterpret_cast<const char*>(this->arcs.data()), this->num_arcs * sizeof(FsmArc));

        return Error::OK;
    }


    Error LoadFromText(std::istream& is) {
        SIO_CHECK(is.good());
        SIO_CHECK(Empty());
        SIO_INFO << "Loading Fsm from string stream";

        Str line;
        Vec<Str> cols;

        /* 1: Parse header */
        {
            // header line: num_states, num_arcs, start_state, final_state
            std::getline(is, line);
            cols = absl::StrSplit(line, ',', absl::SkipWhitespace());
            SIO_CHECK_EQ(cols.size(), 4);

            this->num_states  = std::stoi(cols[0]);
            this->num_arcs    = std::stoi(cols[1]);
            this->start_state = std::stoi(cols[2]);
            this->final_state = std::stoi(cols[3]);

            // K2Fsa conformance checks
            SIO_CHECK_EQ(this->start_state, 0);
            SIO_CHECK_EQ(this->final_state, this->num_states - 1);
        }

        /* 2: Parse & load all arcs */
        {
            int n = 0;
            while (std::getline(is, line)) {
                cols = absl::StrSplit(line, absl::ByAnyChar(" \t"), absl::SkipWhitespace());
                SIO_CHECK_EQ(cols.size(), 3);
                //dbg(cols);

                Vec<Str> arc_info = absl::StrSplit(cols[2], '/');
                SIO_CHECK_EQ(arc_info.size(), 2);

                Vec<Str> labels = absl::StrSplit(arc_info[0], ':');
                SIO_CHECK(labels.size() == 1 || labels.size() == 2); // 1:Fsa,  2:Fst

                AddArc(
                    std::stoi(cols[0]),
                    std::stoi(cols[1]),
                    std::stoi(labels[0]),
                    labels.size() == 2 ? std::stoi(labels[1]) : std::stoi(labels[0]),
                    std::stof(arc_info[1])
                );
                n++;
            }
            SIO_CHECK_EQ(this->num_arcs, n); // Num of arcs loaded is inconsistent with header?

            /* Sort all arcs, first by source state, then by ilabel */
            std::sort(this->arcs.begin(), this->arcs.end(), 
                [](const FsmArc& x, const FsmArc& y) { 
                    return (x.src != y.src) ? (x.src < y.src) : (x.ilabel < y.ilabel);
                }
            );
        }

        /* 3: Setup states */
        {
            this->states.resize(this->num_states + 1); // + 1 sentinel
            Vec<int> out_degree(this->num_states, 0);

            for (const auto& arc : this->arcs) {
                out_degree[arc.src]++;
            }

            // invariant: n = sum( arcs of states[0, s) )
            int n = 0;
            for (FsmStateId s = 0; s != this->num_states; s++) {
                this->states[s].arcs_offset = n;
                n += out_degree[s];
            }

            // setup last sentinel state
            this->states.back().arcs_offset = n;
        }

        return Error::OK;
    }


    void DumpToText(std::ostream& os) const {
        os << this->num_states  << ","
           << this->num_arcs    << "," 
           << this->start_state << "," 
           << this->final_state << "\n";

        for (FsmStateId s = 0; s < this->num_states; s++) {
            for (auto aiter = GetArcIterator(s); !aiter.Done(); aiter.Next()) {
                const FsmArc& arc = aiter.Value();
                os << arc.src << "\t"
                   << arc.dst << "\t"
                   << arc.ilabel << ":" << arc.olabel << "/" << arc.score << "\n";
            }
        }
    }


    Error BuildTokenTopology(const Tokenizer& tokenizer) {
        SIO_CHECK(Empty());
        SIO_CHECK_NE(tokenizer.Size(), 0);
        SIO_INFO << "Building token graph T from tokenizer with size: " << tokenizer.Size();

        /* 1: Build Fsm arcs */
        {
            // 1a: Blank self-loop of start state
            this->start_state = 0;
            AddArc(this->start_state, this->start_state, tokenizer.blk, kFsmEpsilon);

            // 1b: Arcs of normal tokens
            FsmStateId cur_state = 1; // 0 is already occupied by start state
            // Invariant: arcs for states[0, cur_state) & tokens[0, t) are built.
            for (TokenId t = 0; t != tokenizer.Size(); t++) {
                if (t == tokenizer.blk) continue;
                if (t == tokenizer.unk) continue;
                if (t == tokenizer.bos) continue;
                if (t == tokenizer.eos) continue;

                AddArc(this->start_state, cur_state,         t,            t          ); // Entering
                AddArc(cur_state,         cur_state,         t,            kFsmEpsilon); // Self-loop
                AddArc(cur_state,         this->start_state, kFsmEpsilon,  kFsmEpsilon); // Leaving
                cur_state++;
            }

            // 1c: "InputEnd" represents the end of input sequence (follows K2Fsa convention)
            this->final_state = cur_state;
            AddArc(this->start_state, this->final_state, kFsmInputEnd, tokenizer.eos);
            this->num_arcs = this->arcs.size();

            // 1d: Sort all arcs, first by source state, then by ilabel
            std::sort(this->arcs.begin(), this->arcs.end(), 
                [](const FsmArc& x, const FsmArc& y) { 
                    return (x.src != y.src) ? (x.src < y.src) : (x.ilabel < y.ilabel);
                }
            );
        }

        /* 2: Setup states */
        {
            this->num_states = this->final_state + 1;
            this->states.resize(this->num_states + 1); // + 1 sentinel

            Vec<int> out_degree(this->num_states, 0);
            for (const auto& arc : this->arcs) {
                out_degree[arc.src]++;
            }

            // invariant: n = sum( arcs of states[0, s) )
            int n = 0;
            for (FsmStateId s = 0; s != this->num_states; s++) {
                this->states[s].arcs_offset = n;
                n += out_degree[s];
            }
            this->states.back().arcs_offset = n; // setup last sentinel state
        }

        return Error::OK;
    }


    void AddArc(FsmStateId src, FsmStateId dst, FsmLabel ilabel, FsmLabel olabel, FsmScore score = 0.0) {
        this->arcs.push_back({src, dst, ilabel, olabel, score});
    }

}; // struct Fsm
} // namespace sio
#endif

