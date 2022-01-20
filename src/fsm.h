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

    Str version_; // TODO: make version a part of binary header

    StateId start_ = 0;  /* conform to K2 */

    Vec<State> states_;
    Vec<Arc> arcs_;


    StateId Start() { return start_; }


    inline State& GetState(StateId i) { return states_[i]; }


    inline Arc& GetArc(ArcId j) { return arcs_[j]; }


    inline size_t NumArcsOf(StateId i) {
        return GetState(i+1).arcs - GetState(i).arcs;
    }


    u64 NumStates() {
        return states_.size() == 0 ? 0 : states_.size() - 1;
    }


    u64 NumArcs() { return arcs_.size(); }


    Error Load(std::istream &is, bool binary) {
        using kaldi::ExpectToken;
        using kaldi::ReadBasicType;

        SIO_CHECK_EQ(states_.size(), 0) << "Fsm already loaded ?";
        SIO_CHECK_EQ(arcs_.size(), 0) << "Fsm already loaded ?";;

        ExpectToken(is, binary, "<Fsm>");

        /*
        TODO: version handling here
        */

        ExpectToken(is, binary, "<Start>");
        ReadBasicType(is, binary, &start_);

        u64 num_states = 0;
        ExpectToken(is, binary, "<NumStates>");
        ReadBasicType(is, binary, &num_states);
        auto num_states_plus_guardian = num_states + 1; // one extra guardian-state at the end
        states_.resize(num_states_plus_guardian);
        ExpectToken(is, binary, "<States>");
        is.read(reinterpret_cast<char*>(states_.data()), num_states_plus_guardian * sizeof(State));


        u64 num_arcs = 0;
        ExpectToken(is, binary, "<NumArcs>");
        ReadBasicType(is, binary, &num_arcs);
        arcs_.resize(num_arcs);
        ExpectToken(is, binary, "<Arcs>");
        is.read(reinterpret_cast<char*>(arcs_.data()), num_arcs * sizeof(Arc));

        return Error::OK;
    }

    int Dump(std::ostream &os, bool binary) {
        using kaldi::WriteToken;
        using kaldi::WriteBasicType;

        WriteToken(os, binary, "<Fsm>");

        /*
        TODO: version handling here
        */

        WriteToken(os, binary, "<Start>");
        WriteBasicType(os, binary, Start());

        WriteToken(os, binary, "<NumStates>");
        WriteBasicType(os, binary, NumStates());

        WriteToken(os, binary, "<NumArcs>");
        WriteBasicType(os, binary, NumArcs());

        WriteToken(os, binary, "<States>");
        auto num_states_plus_guardian = NumStates() + 1; // one extra guardian-state at the end
        os.write(reinterpret_cast<const char*>(states_.data()), num_states_plus_guardian * sizeof(State));

        WriteToken(os, binary, "<Arcs>");
        os.write(reinterpret_cast<const char*>(arcs_.data()), NumArcs() * sizeof(Arc));

        return 0;
    }

}; // struct Fsm
} // namespace sio
#endif
