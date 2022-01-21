#ifndef SIO_FSM_H
#define SIO_FSM_H

#include "base/io-funcs.h"

#include "sio/common.h"

namespace sio {
struct Fsm {
    using StateId = index_t;
    using ArcId   = index_t;
    using LabelId = i32;
    using Weight  = f32;

    struct State {
        ArcId arcs = 0;
    };

    struct ArcAux {
        LabelId label = 0;
    };

    struct Arc {
        StateId src = 0;
        StateId dst = 0;
        LabelId label = 0;
        Weight weight = 0.0f;

        ArcAux aux;
    };

    class ArcIterator {
      private:
        const Arc* cur_ = nullptr;
        const Arc* end_ = nullptr;

      public:
        ArcIterator(const Arc* begin, const Arc* end) : cur_(begin), end_(end) {}
        bool Done() { return cur_ >= end_; }
        void Next() { ++cur_; }
        const Arc& Value() { return *cur_; }
    };

    Str version; // TODO: make version a part of binary header

    StateId start_state = 0;  /* conform to K2 */
    StateId final_state = 0;

    Vec<State> states;
    Vec<Arc> arcs;


    inline bool Empty() const {
        return states.size() == 0;
    }


    StateId Start() const {
        SIO_CHECK(!Empty());
        return start_state;
    }


    inline StateId Final() const {
        SIO_CHECK(!Empty());
        return final_state;
    }


    inline const State& GetState(StateId i) const {
        SIO_CHECK(!Empty());
        SIO_CHECK_LT(i, states.size() - 1); // block external access to guardian state 
        return states[i];
    }


    ArcIterator GetArcIterator(StateId i) const {
        SIO_CHECK(!Empty());
        return ArcIterator(
            &arcs[states[i].arcs],
            &arcs[states[i+1].arcs]
        );
    }


    inline size_t NumArcsOf(StateId i) const {
        SIO_CHECK(!Empty());
        return states[i+1].arcs - states[i].arcs;
    }


    u64 NumStates() const {
        SIO_CHECK(!Empty());
        return states.size() - 1;
    }


    u64 NumArcs() const {
        SIO_CHECK(!Empty());
        return arcs.size(); 
    }


    Error Load(std::istream &is, bool binary) {
        SIO_CHECK(Empty()) << "Fsm already loaded ?";

        using kaldi::ExpectToken;
        using kaldi::ReadBasicType;

        ExpectToken(is, binary, "<Fsm>");

        /*
        TODO: version handling here
        */

        ExpectToken(is, binary, "<Start>");
        ReadBasicType(is, binary, &start_state);
        SIO_CHECK(start_state == 0);

        ExpectToken(is, binary, "<Final>");
        ReadBasicType(is, binary, &final_state);

        u64 num_states = 0;
        ExpectToken(is, binary, "<NumStates>");
        ReadBasicType(is, binary, &num_states);
        SIO_CHECK_EQ(final_state, num_states - 1);

        ExpectToken(is, binary, "<States>");
        auto num_states_plus_one = num_states + 1; // one extra guardian-state at the end
        states.resize(num_states_plus_one);
        is.read(reinterpret_cast<char*>(states.data()), num_states_plus_one * sizeof(State));

        u64 num_arcs = 0;
        ExpectToken(is, binary, "<NumArcs>");
        ReadBasicType(is, binary, &num_arcs);

        ExpectToken(is, binary, "<Arcs>");
        arcs.resize(num_arcs);
        is.read(reinterpret_cast<char*>(arcs.data()), num_arcs * sizeof(Arc));

        return Error::OK;
    }

    int Dump(std::ostream &os, bool binary) const {
        SIO_CHECK(!Empty()) << "Dumping empty Fsm ?";
        using kaldi::WriteToken;
        using kaldi::WriteBasicType;

        WriteToken(os, binary, "<Fsm>");

        /*
        TODO: version handling here
        */

        WriteToken(os, binary, "<Start>");
        WriteBasicType(os, binary, Start());

        WriteToken(os, binary, "<Final>");
        WriteBasicType(os, binary, Final());

        WriteToken(os, binary, "<NumStates>");
        WriteBasicType(os, binary, NumStates());

        WriteToken(os, binary, "<NumArcs>");
        WriteBasicType(os, binary, NumArcs());

        WriteToken(os, binary, "<States>");
        auto num_states_plus_one = NumStates() + 1; // one extra guardian-state at the end
        os.write(reinterpret_cast<const char*>(states.data()), num_states_plus_one * sizeof(State));

        WriteToken(os, binary, "<Arcs>");
        os.write(reinterpret_cast<const char*>(arcs.data()), NumArcs() * sizeof(Arc));

        return 0;
    }

}; // struct Fsm
} // namespace sio
#endif
