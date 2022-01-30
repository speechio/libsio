#ifndef SIO_FSM_H
#define SIO_FSM_H

#include <algorithm>
#include "base/io-funcs.h"

#include "sio/common.h"
#include "sio/tokenizer.h"
//#include "sio/dbg.h"

namespace sio {
class Fsm {
public:
    /********** Types **********/
    using StateId = i32;
    using ArcId = i32;
    using Label = i32;
    using Score = f32;

    static const Label kInputEnd = -1; // This follows K2Fsa convention
    static const Label kEpsilon = -2;


    struct State {
        ArcId arcs_begin = 0;
    };


    struct Arc {
        StateId src = 0;
        StateId dst = 0;
        Label ilabel = 0;
        Label olabel = 0;
        Score score = 0.0f;

        void Set(StateId src, StateId dst, Label ilabel, Label olabel, Score score) {
            this->src = src;
            this->dst = dst;
            this->ilabel = ilabel;
            this->olabel = olabel;
            this->score = score;
        }
    };


    class ArcIterator {
      private:
        const Arc* cur_ = nullptr;
        const Arc* end_ = nullptr;

      public:
        ArcIterator(const Arc* begin, const Arc* end) : cur_(begin), end_(end) {}

        const Arc& Value() const { return *cur_; }

        void Next() { ++cur_; }

        bool Done() const { return cur_ >= end_; }
    };


    inline bool Empty() const { return states_.size() == 0; }

    inline StateId Start() const { SIO_CHECK(!Empty()); return start_state_; }
    inline StateId Final() const { SIO_CHECK(!Empty()); return final_state_; }

    // Use i64 as return type instead of size_t,
    // because size_t is not suitable for platform independent binary
    // TODO: bit/little endian compatibility
    i64 NumStates() const { SIO_CHECK(!Empty()); return states_.size() - 1; } // -1: last sentinel is not counted as valid Fsm state
    i64 NumArcs()   const { SIO_CHECK(!Empty()); return arcs_.size(); }


    ArcIterator GetArcIterator(StateId i) const {
        SIO_CHECK(!Empty());
        SIO_CHECK_NE(i, states_.size() - 1); // block external access to sentinel
        return ArcIterator(
            &arcs_[states_[i  ].arcs_begin],
            &arcs_[states_[i+1].arcs_begin]
        );
    }


    void AddArc(StateId src, StateId dst, Label ilabel, Label olabel, Score score = 0.0) {
        Arc arc;
        arc.Set(src, dst, ilabel, olabel, score);
        arcs_.push_back(arc);
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
        i64 num_states = 0;
        ReadBasicType(is, binary, &num_states);

        ExpectToken(is, binary, "<NumArcs>");
        i64 num_arcs = 0;
        ReadBasicType(is, binary, &num_arcs);

        ExpectToken(is, binary, "<Start>");
        ReadBasicType(is, binary, &start_state_);
        SIO_CHECK(start_state_ == 0); // conform to K2

        ExpectToken(is, binary, "<Final>");
        ReadBasicType(is, binary, &final_state_);
        SIO_CHECK_EQ(final_state_, num_states - 1); // conform to K2

        ExpectToken(is, binary, "<States>");
        auto num_states_plus_sentinel = num_states + 1;
        states_.resize(num_states_plus_sentinel);
        is.read(reinterpret_cast<char*>(states_.data()), num_states_plus_sentinel * sizeof(State));

        ExpectToken(is, binary, "<Arcs>");
        arcs_.resize(num_arcs);
        is.read(reinterpret_cast<char*>(arcs_.data()), num_arcs * sizeof(Arc));

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
        WriteBasicType(os, binary, NumStates());

        WriteToken(os, binary, "<NumArcs>");
        WriteBasicType(os, binary, NumArcs());

        WriteToken(os, binary, "<Start>");
        WriteBasicType(os, binary, Start());

        WriteToken(os, binary, "<Final>");
        WriteBasicType(os, binary, Final());

        WriteToken(os, binary, "<States>");
        auto num_states_plus_sentinel = NumStates() + 1;
        os.write(reinterpret_cast<const char*>(states_.data()), num_states_plus_sentinel * sizeof(State));

        WriteToken(os, binary, "<Arcs>");
        os.write(reinterpret_cast<const char*>(arcs_.data()), NumArcs() * sizeof(Arc));

        return Error::OK;
    }


    Error LoadFromString(std::istream& is) {
        SIO_CHECK(is.good()) << "Invalid Fsm loading stream.";
        SIO_CHECK(Empty()) << "Reloading is not supported.";
        SIO_INFO << "Loading Fsm from string stream";

        Str line;
        Vec<Str> cols;
        size_t num_states, num_arcs;

        /* 1: Parse header */
        {
            // header line: num_states, num_arcs, start_state, final_state
            std::getline(is, line);
            cols = absl::StrSplit(line, ',', absl::SkipWhitespace());
            SIO_CHECK_EQ(cols.size(), 4);

            num_states   = std::stoi(cols[0]);
            num_arcs     = std::stoi(cols[1]);
            start_state_ = std::stoi(cols[2]);
            final_state_ = std::stoi(cols[3]);

            // K2Fsa conformance checks
            SIO_CHECK_EQ(start_state_, 0);
            SIO_CHECK_EQ(final_state_, num_states - 1);
        }

        /* 2: Parse & load all arcs */
        {
            size_t n = 0;
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
            SIO_CHECK_EQ(num_arcs, n) << "Num of arcs loaded is inconsistent with header.";

            /* Sort all arcs, first by source state, then by dest state */
            std::sort(arcs_.begin(), arcs_.end(), 
                [](const Arc& x, const Arc& y) { 
                    return (x.src != y.src) ? (x.src < y.src) : (x.dst < y.dst);
                }
            );
        }

        /* 3: Setup states */
        {
            size_t num_states_plus_sentinel = num_states + 1;
            states_.resize(num_states_plus_sentinel);

            Vec<i32> out_degree(num_states, 0);
            for (const auto& arc : arcs_) {
                out_degree[arc.src]++;
            }

            size_t n = 0;
            // invariant: n = sum{ arcs of states_[0, s) }
            for (StateId s = 0; s != num_states; s++) {
                states_[s].arcs_begin = n;
                n += out_degree[s];
            }

            // setup last sentinel state
            states_.back().arcs_begin = n;
        }

        return Error::OK;
    }


    Error BuildTokenTopology(const Tokenizer& tokenizer) {
        SIO_CHECK(Empty());
        SIO_CHECK_NE(tokenizer.Size(), 0);
        SIO_INFO << "Building token graph T from tokenizer with size: " << tokenizer.Size();

        size_t num_states;
        /* 1: Build Fsm arcs */
        {
            // 1a: Blank self-loop of start state
            start_state_ = 0;
            AddArc(start_state_, start_state_, tokenizer.blk, kEpsilon);

            // 1b: Arcs of normal tokens
            StateId cur_state = 1; 
            // Invariant: arcs for states[0, cur_state) & tokens[0, t) are built.
            for (Tokenizer::TokenId t = 0; t != tokenizer.Size(); t++) {
                if (t == tokenizer.blk) continue;
                if (t == tokenizer.unk) continue; // TODO: actually unk should be built, shouldn't skip
                if (t == tokenizer.bos) continue;
                if (t == tokenizer.eos) continue;

                AddArc(start_state_, cur_state,    t,         t       ); // Entering
                AddArc(cur_state,    cur_state,    t,         kEpsilon); // Self-loop
                AddArc(cur_state,    start_state_, kEpsilon,  kEpsilon); // Leaving
                cur_state++;
            }

            // 1c: "InputEnd" represents the end of input sequence (follows K2Fsa convention)
            final_state_ = cur_state;
            AddArc(start_state_, final_state_, kInputEnd, tokenizer.eos);

            // 1d: Sort all arcs, first by source state, then by dest state
            std::sort(arcs_.begin(), arcs_.end(), 
                [](const Arc& x, const Arc& y) { 
                    return (x.src != y.src) ? (x.src < y.src) : (x.dst < y.dst);
                }
            );
        }

        /* 2: Setup states */
        {
            num_states = final_state_ + 1;
            size_t num_states_plus_sentinel = num_states + 1;
            states_.resize(num_states_plus_sentinel);

            Vec<i32> out_degree(num_states, 0);
            for (const auto& arc : arcs_) {
                out_degree[arc.src]++;
            }

            size_t n = 0;
            // invariant: n = sum{ arcs of states_[0, s) }
            for (StateId s = 0; s != num_states; s++) {
                states_[s].arcs_begin = n;
                n += out_degree[s];
            }
            states_.back().arcs_begin = n; // setup last sentinel state
        }

        return Error::OK;
    }


    void Print() const {
        printf("%d,%d,%d,%d\n", NumStates(), NumArcs(), Start(), Final());
        for (StateId s = 0; s < NumStates(); s++) {
            for (auto ai = GetArcIterator(s); !ai.Done(); ai.Next()) {
                const Arc& arc = ai.Value();
                printf("%d\t%d\t%d:%d/%f\n", arc.src, arc.dst, arc.ilabel, arc.olabel, arc.score);
            }
        }
    }

private:
    Str version_; // TODO: make version a part of binary header

    StateId start_state_ = 0;
    StateId final_state_ = 0;

    Vec<State> states_;
    Vec<Arc> arcs_;

}; // class Fsm
} // namespace sio
#endif
