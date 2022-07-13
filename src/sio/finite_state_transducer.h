#ifndef SIO_FINITE_STATE_TRANSDUCER_H
#define SIO_FINITE_STATE_TRANSDUCER_H

#include <limits>
#include <algorithm>

#include "base/io-funcs.h"

#include "sio/base.h"
#include "sio/tokenizer.h"

namespace sio {

using FstStateId = i32;
using FstArcId   = i32;
using FstLabel   = i32;
using FstScore   = f32;


enum FstSymbol {
    kFstEpsilon = -32768,
    kFstRho,
    kFstPhi,
    kFstFinalSymbol = -1 // This conforms to K2 Fsa
};


struct FstState {
    FstArcId offset = 0;
    i32 num_arcs = 0;
};


struct FstArc {
    FstStateId src = 0;
    FstStateId dst = 0;
    FstLabel ilabel = kFstEpsilon;
    FstLabel olabel = kFstEpsilon;
    FstScore score = 0.0f;
};


class FstArcIterator {
    const FstArc* cur_ = nullptr;
    const FstArc* end_ = nullptr;

public:
    FstArcIterator(const FstArc* begin, const FstArc* end) : cur_(begin), end_(end) { }

    const FstArc& Value() const { return *cur_; }

    void Next() { ++cur_; }

    bool Done() const { return cur_ >= end_; }
};


struct Fst {
    str version; // TODO: make version a part of binary header

    FstStateId start_state = 0;
    FstStateId final_state = 0;

    vec<FstState> states;
    vec<FstArc> arcs;


    inline bool Empty() const { return this->states.empty(); }

    inline bool ContainEpsilonArc(FstStateId s) const {
        // Preconditions:
        //   1. kFstEpsilon has smallest id in symbol table.
        //   2. arcs of a FstState are sorted by ilabels.
        return this->arcs[this->states[s].offset].ilabel == kFstEpsilon;
    }


    FstArcIterator GetArcIterator(FstStateId i) const {
        SIO_CHECK(!Empty());
        const FstState& s = this->states[i];
        return FstArcIterator(
            &this->arcs[s.offset],
            &this->arcs[s.offset + s.num_arcs]
        );
    }


    Error Load(std::istream& is) {
        SIO_CHECK(Empty()); // Can't reload
        SIO_CHECK(is.good());

        using kaldi::ExpectToken;
        using kaldi::ReadBasicType;

        bool binary = true;

        ExpectToken(is, binary, "<Fst>");

        /*
        TODO: version handling here
        */

        i64 num_states = 0;
        ExpectToken(is, binary, "<NumStates>");
        ReadBasicType(is, binary, &num_states);

        i64 num_arcs = 0;
        ExpectToken(is, binary, "<NumArcs>");
        ReadBasicType(is, binary, &num_arcs);

        ExpectToken(is, binary, "<StartState>");
        ReadBasicType(is, binary, &this->start_state);
        SIO_CHECK(this->start_state == 0); // conform to K2

        ExpectToken(is, binary, "<FinalState>");
        ReadBasicType(is, binary, &this->final_state);
        SIO_CHECK_EQ(this->final_state, num_states - 1); // K2: last state is final state

        ExpectToken(is, binary, "<States>");
        this->states.resize(num_states);
        is.read(reinterpret_cast<char*>(this->states.data()), num_states * sizeof(FstState));

        ExpectToken(is, binary, "<Arcs>");
        this->arcs.resize(num_arcs);
        is.read(reinterpret_cast<char*>(this->arcs.data()), num_arcs * sizeof(FstArc));

        return Error::OK;
    }


    Error Dump(std::ostream& os) const {
        SIO_CHECK(!Empty());
        SIO_CHECK(os.good());

        using kaldi::WriteToken;
        using kaldi::WriteBasicType;

        bool binary = true;

        WriteToken(os, binary, "<Fst>");

        /*
        TODO: version handling here
        */

        WriteToken(os, binary, "<NumStates>");
        WriteBasicType(os, binary, (i64)this->states.size());

        WriteToken(os, binary, "<NumArcs>");
        WriteBasicType(os, binary, (i64)this->arcs.size());

        WriteToken(os, binary, "<StartState>");
        WriteBasicType(os, binary, this->start_state);

        WriteToken(os, binary, "<FinalState>");
        WriteBasicType(os, binary, this->final_state);

        WriteToken(os, binary, "<States>");
        os.write(reinterpret_cast<const char*>(this->states.data()), this->states.size() * sizeof(FstState));

        WriteToken(os, binary, "<Arcs>");
        os.write(reinterpret_cast<const char*>(this->arcs.data()), this->arcs.size() * sizeof(FstArc));

        return Error::OK;
    }


    Error LoadFromText(std::istream& is) {
        SIO_CHECK(is.good());
        SIO_CHECK(Empty());
        SIO_INFO << "Loading Fst from string stream";

        str line;
        vec<str> cols;

        /* 1: Parse header: num_states, num_arcs, start_state, final_state */
        std::getline(is, line);
        cols = absl::StrSplit(line, ',', absl::SkipWhitespace());

        SIO_CHECK_EQ(cols.size(), 4);
        i64 num_states    = std::stoi(cols[0]); 
        i64 num_arcs      = std::stoi(cols[1]);
        this->start_state = std::stoi(cols[2]);
        this->final_state = std::stoi(cols[3]);

        // K2Fsa conformance checks
        SIO_CHECK_EQ(this->start_state, 0);
        SIO_CHECK_EQ(this->final_state, num_states - 1);

        /* 2: Parse & load all arcs */
        {
            this->arcs.reserve(num_arcs);
            while (std::getline(is, line)) {
                // Fsa line format: "src    dst     label/score"
                // Fst line format: "src    dst     ilabel:olabel/score"
                cols = absl::StrSplit(line, absl::ByAnyChar(" \t"), absl::SkipWhitespace());
                SIO_CHECK_EQ(cols.size(), 3);

                vec<str> arc_info = absl::StrSplit(cols[2], '/');
                SIO_CHECK_EQ(arc_info.size(), 2);

                vec<str> labels = absl::StrSplit(arc_info[0], ':');
                SIO_CHECK(labels.size() == 1 || labels.size() == 2); // 1:Fsa,  2:Fst

                AddArc(
                    std::stoi(cols[0]),
                    std::stoi(cols[1]),
                    std::stoi(labels[0]),
                    labels.size() == 2 ? std::stoi(labels[1]) : std::stoi(labels[0]),
                    std::stof(arc_info[1])
                );
            }
            SIO_CHECK_EQ(this->arcs.size(), num_arcs); // must be consistent with header

            /* Sort all arcs, first by source state, then by ilabel */
            std::sort(this->arcs.begin(), this->arcs.end(), 
                [](const FstArc& x, const FstArc& y) { 
                    return (x.src != y.src) ? (x.src < y.src) : (x.ilabel < y.ilabel);
                }
            );
        }

        /* 3: Setup states */
        {
            this->states.reserve(num_states);

            vec<int> odegree(num_states, 0);
            for (const auto& arc : this->arcs) {
                odegree[arc.src]++;
            }

            // invariant: k = sum( arcs of states[0, s) )
            int k = 0;
            for (FstStateId s = 0; s != num_states; s++) {
                this->states.push_back({.offset = k, .num_arcs = odegree[s]});
                k += odegree[s];
            }

            SIO_CHECK_EQ(k, this->arcs.size()); // postcondition
        }

        return Error::OK;
    }


    void DumpToText(std::ostream& os) const {
        os << this->states.size() << ","
           << this->arcs.size()   << "," 
           << this->start_state   << "," 
           << this->final_state   << "\n";

        for (FstStateId s = 0; s < this->states.size(); s++) {
            for (auto aiter = GetArcIterator(s); !aiter.Done(); aiter.Next()) {
                const FstArc& arc = aiter.Value();
                os << arc.src << "\t"
                   << arc.dst << "\t"
                   << arc.ilabel << ":" << arc.olabel << "/" << arc.score << "\n";
            }
        }
    }


    void AddArc(FstStateId src, FstStateId dst, FstLabel ilabel, FstLabel olabel, FstScore score = 0.0) {
        this->arcs.push_back({src, dst, ilabel, olabel, score});
    }

}; // struct Fst
} // namespace sio
#endif

