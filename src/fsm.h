#ifndef SIO_FSM_H
#define SIO_FSM_H

#include <algorithm>
#include "base/io-funcs.h"

#include "sio/common.h"
#include "sio/tokenizer.h"
//#include "sio/dbg.h"

namespace sio {

#define kFinal -1
#define kEpsilon -2

class Fsm {
public:
    /********** Types **********/
    using StateId = i32;
    using ArcId = i32;
    using Label = i32;
    using Score = f32;


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


private:
    /********** Data Members **********/
    Str version_; // TODO: make version a part of binary header

    StateId start_state_ = 0;
    StateId final_state_ = 0;

    Vec<State> states_;
    Vec<Arc> arcs_;


public:
    /********** Interfaces **********/
    inline bool Empty() const { return states_.size() == 0; }

    inline StateId Start() const { SIO_CHECK(!Empty()); return start_state_; }
    inline StateId Final() const { SIO_CHECK(!Empty()); return final_state_; }

    // Use i64 for return type instead of size_t:
    // * return of NumStates & NumArcs are parts of binary model file
    // * size_t is platform dependent, so fixed-width integers are prefered
    // * TODO: should also support bit/little endian compatibility
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


    void AddArc(StateId src, StateId dst, Label ilabel, Label olabel, Score score) {
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

        Str line;
        Vec<Str> cols;

        // header line: num_states num_arcs start_state final_state
        std::getline(is, line);
        cols = absl::StrSplit(line, ',', absl::SkipWhitespace());
        SIO_CHECK_EQ(cols.size(), 4);
        size_t num_states = std::stoi(cols[0]);
        size_t num_arcs   = std::stoi(cols[1]);
        start_state_ = std::stoi(cols[2]);
        final_state_ = std::stoi(cols[3]);
        //dbg(num_states, num_arcs, start_state_, final_state_);

        // allocate space for states and arcs
        states_.resize(num_states + 1); // plus 1 sentinel
        arcs_.resize(num_arcs);

        // load arcs
        Vec<i32> num_arcs_of_state(num_states, 0);
        ArcId a = 0;
        while (std::getline(is, line)) {
            cols = absl::StrSplit(line, absl::ByAnyChar(" \t"), absl::SkipWhitespace());
            SIO_CHECK_EQ(cols.size(), 3);
            //dbg(cols);

            Vec<Str> arc_info = absl::StrSplit(cols[2], '/');
            SIO_CHECK_EQ(arc_info.size(), 2);

            Vec<Str> labels = absl::StrSplit(arc_info[0], ':');
            SIO_CHECK(labels.size() == 1 || labels.size() == 2); // 1:Fsa,  2:Fst

            Arc &arc = arcs_[a];
            arc.Set(
                std::stoi(cols[0]),
                std::stoi(cols[1]),
                std::stoi(labels[0]),
                labels.size() == 2 ? std::stoi(labels[1]) : arc.ilabel,
                std::stof(arc_info[1])
            );

            ++num_arcs_of_state[arc.src];
            a++;
        }

        // sort labels first by src state, then by dst state
        std::sort(arcs_.begin(), arcs_.end(), 
            [](const Arc& x, const Arc& y) { 
                return (x.src != y.src) ? (x.src < y.src) : (x.dst < y.dst);
            }
        );

        // load states
        StateId s = 0;
        size_t n = 0;
        // invariant: states_[s].arcs_begin = sum of arcs of states_[0, s)
        while (s < num_states) {
            states_[s].arcs_begin = n;
            n += num_arcs_of_state[s++];
        }
        states_[s].arcs_begin = n; // setup sentinel state

        return Error::OK;
    }


    void Print() {
        printf("%d,%d,%d,%d\n", NumStates(), NumArcs(), Start(), Final());
        for (StateId s = 0; s < NumStates(); s++) {
            for (auto ai = GetArcIterator(s); !ai.Done(); ai.Next()) {
                const Arc& arc = ai.Value();
                printf("%d\t%d\t%d:%d/%f\n", arc.src, arc.dst, arc.ilabel, arc.olabel, arc.score);
            }
        }
    }


    Error BuildTokenTopo(const Tokenizer& tokenizer) {
        SIO_CHECK(Empty());
        SIO_CHECK_NE(tokenizer.Size(), 0);
        SIO_INFO << "Building token topo from tokenizer with size: " << tokenizer.Size();

        size_t normal_tokens = 0;
        for (int t = 0; t != tokenizer.Size(); t++) {
            if (!tokenizer.IsSpecialToken(t)) {
                normal_tokens++;
            }
        }

        size_t num_states = normal_tokens + 1/*start state*/ + 1/*final state*/;
        size_t num_states_plus_sentinel = num_states + 1;
        states_.resize(num_states_plus_sentinel);

        Vec<i32> num_arcs_of_state(num_states, 0);

        start_state_ = 0; // This is also "blank" state
        final_state_ = num_states - 1;

        // blank state self-loop
        AddArc(start_state_, start_state_, tokenizer.blk, kEpsilon, 0.0);
        ++num_arcs_of_state[start_state_];

        // build normal tokens
        StateId token_state = 0; 
        for (index_t token = 0; token != tokenizer.Size(); token++) {
            if (tokenizer.IsSpecialToken(token)) {
                continue;
            }

            token_state++;

            // token's entering arc
            AddArc(start_state_, token_state, token, token, 0.0);
            ++num_arcs_of_state[start_state_];

            // token's self-loop arc
            AddArc(token_state, token_state, token, kEpsilon, 0.0);
            ++num_arcs_of_state[token_state];

            // token's leaving arc
            AddArc(token_state, start_state_, kEpsilon, kEpsilon, 0.0);
            ++num_arcs_of_state[token_state];
        }

        // final arc from start to final state
        AddArc(start_state_, final_state_, kFinal, tokenizer.eos, 0.0);
        ++num_arcs_of_state[start_state_];

        // sort labels
        std::sort(arcs_.begin(), arcs_.end(), 
            [](const Arc& x, const Arc& y) { 
                return (x.src != y.src) ? (x.src < y.src) : (x.dst < y.dst);
            }
        );

        // setup states
        StateId s = 0;
        size_t n = 0;
        // invariant: states_[s].arcs_begin = sum of arcs of states_[0, s)
        while (s < num_states) {
            states_[s].arcs_begin = n;
            n += num_arcs_of_state[s++];
        }
        states_[s].arcs_begin = n; // setup sentinel state

        return Error::OK;
    }

}; // class Fsm
} // namespace sio
#endif
