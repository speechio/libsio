#ifndef SIO_FINITE_STATE_MACHINE_H
#define SIO_FINITE_STATE_MACHINE_H

#include <algorithm>
#include "base/io-funcs.h"

#include "sio/common.h"
#include "sio/tokenizer.h"
//#include "sio/dbg.h"

namespace sio {

using FsmStateId = i32;
using FsmArcId   = i32;
using FsmLabel   = i32;
using FsmScore   = f32;

constexpr FsmLabel kFsmInputEnd = -1; // This follows K2Fsa convention
constexpr FsmLabel kFsmEpsilon = -2;


struct FsmState {
    FsmArcId arcs_begin = 0;
};


struct FsmArc {
    FsmStateId src = 0;
    FsmStateId dst = 0;
    FsmLabel ilabel = 0;
    FsmLabel olabel = 0;
    FsmScore score = 0.0f;

    void Set(FsmStateId src, FsmStateId dst, FsmLabel ilabel, FsmLabel olabel, FsmScore score) {
        this->src = src;
        this->dst = dst;
        this->ilabel = ilabel;
        this->olabel = olabel;
        this->score = score;
    }
};


struct Fsm {
    using StateId = FsmStateId;
    using ArcId   = FsmArcId;
    using Label   = FsmLabel;
    using Score   = FsmScore;

    using State = FsmState;
    using Arc   = FsmArc;

    Str version; // TODO: make version a part of binary header

    // Use i64 instead of size_t, for platform independent binary
    // TODO: bit/little endian compatibility
    i64 num_states = 0;
    i64 num_arcs = 0;

    StateId start_state = 0;
    StateId final_state = 0;

    Vec<State> states;  // one extra sentinel at the end: states.size() = num_states + 1
    Vec<Arc> arcs;


    class ArcIterator {
        const Arc* cur_ = nullptr;
        const Arc* end_ = nullptr;

      public:
        ArcIterator(const Arc* begin, const Arc* end) : cur_(begin), end_(end) {}

        const Arc& Value() const { return *cur_; }

        void Next() { ++cur_; }

        bool Done() const { return cur_ >= end_; }
    };


    inline bool Empty() const { return this->states.empty(); }


    ArcIterator GetArcIterator(StateId i) const {
        SIO_CHECK(!Empty());
        SIO_CHECK_NE(i, this->states.size() - 1); // block external access to sentinel
        return ArcIterator(
            &this->arcs[this->states[i  ].arcs_begin],
            &this->arcs[this->states[i+1].arcs_begin]
        );
    }


    Error Load(std::istream& is) {
        SIO_CHECK(Empty()) << "Reloading is not supported.";
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

        ExpectToken(is, binary, "<Start>");
        ReadBasicType(is, binary, &this->start_state);
        SIO_CHECK(this->start_state == 0); // conform to K2

        ExpectToken(is, binary, "<Final>");
        ReadBasicType(is, binary, &this->final_state);
        SIO_CHECK_EQ(this->final_state, this->num_states - 1); // conform to K2

        ExpectToken(is, binary, "<States>");
        auto num_states_plus_sentinel = this->num_states + 1;
        this->states.resize(num_states_plus_sentinel);
        is.read(reinterpret_cast<char*>(this->states.data()), num_states_plus_sentinel * sizeof(State));

        ExpectToken(is, binary, "<Arcs>");
        this->arcs.resize(this->num_arcs);
        is.read(reinterpret_cast<char*>(this->arcs.data()), this->num_arcs * sizeof(Arc));

        return Error::OK;
    }


    Error Dump(std::ostream& os) const {
        SIO_CHECK(!Empty()) << "Dumping empty Fsm ?";
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

        WriteToken(os, binary, "<Start>");
        WriteBasicType(os, binary, this->start_state);

        WriteToken(os, binary, "<Final>");
        WriteBasicType(os, binary, this->final_state);

        WriteToken(os, binary, "<States>");
        auto num_states_plus_sentinel = this->num_states + 1;
        os.write(reinterpret_cast<const char*>(this->states.data()), num_states_plus_sentinel * sizeof(State));

        WriteToken(os, binary, "<Arcs>");
        os.write(reinterpret_cast<const char*>(this->arcs.data()), this->num_arcs * sizeof(Arc));

        return Error::OK;
    }


    Error LoadFromString(std::istream& is) {
        SIO_CHECK(is.good()) << "Invalid Fsm loading stream.";
        SIO_CHECK(Empty()) << "Reloading is not supported.";
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
            i32 n = 0;
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
            SIO_CHECK_EQ(this->num_arcs, n) << "Num of arcs loaded is inconsistent with header.";

            /* Sort all arcs, first by source state, then by ilabel */
            std::sort(this->arcs.begin(), this->arcs.end(), 
                [](const Arc& x, const Arc& y) { 
                    return (x.src != y.src) ? (x.src < y.src) : (x.ilabel < y.ilabel);
                }
            );
        }

        /* 3: Setup states */
        {
            this->states.resize(this->num_states + 1); // + 1 sentinel
            Vec<i32> out_degree(this->num_states, 0);

            for (const auto& arc : this->arcs) {
                out_degree[arc.src]++;
            }

            // invariant: n = sum{ arcs of this->states[0, s) }
            i32 n = 0;
            for (StateId s = 0; s != this->num_states; s++) {
                this->states[s].arcs_begin = n;
                n += out_degree[s];
            }

            // setup last sentinel state
            this->states.back().arcs_begin = n;
        }

        return Error::OK;
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
            StateId cur_state = 1; // 0 is already occupied by start state
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
                [](const Arc& x, const Arc& y) { 
                    return (x.src != y.src) ? (x.src < y.src) : (x.ilabel < y.ilabel);
                }
            );
        }

        /* 2: Setup states */
        {
            this->num_states = this->final_state + 1;
            this->states.resize(this->num_states + 1); // + 1 sentinel

            Vec<i32> out_degree(this->num_states, 0);
            for (const auto& arc : this->arcs) {
                out_degree[arc.src]++;
            }

            // invariant: n = sum{ arcs of this->states[0, s) }
            i32 n = 0;
            for (StateId s = 0; s != this->num_states; s++) {
                this->states[s].arcs_begin = n;
                n += out_degree[s];
            }
            this->states.back().arcs_begin = n; // setup last sentinel state
        }

        return Error::OK;
    }


    void Print() const {
        printf("%d,%d,%d,%d\n", this->num_states, this->num_arcs, this->start_state, this->final_state);
        for (StateId s = 0; s < this->num_states; s++) {
            for (auto aiter = GetArcIterator(s); !aiter.Done(); aiter.Next()) {
                const Arc& arc = aiter.Value();
                printf("%d\t%d\t%d:%d/%f\n", arc.src, arc.dst, arc.ilabel, arc.olabel, arc.score);
            }
        }
    }


private:

    void AddArc(StateId src, StateId dst, Label ilabel, Label olabel, Score score = 0.0) {
        Arc arc;
        arc.Set(src, dst, ilabel, olabel, score);
        this->arcs.push_back(arc);
    }

}; // struct Fsm
} // namespace sio
#endif

