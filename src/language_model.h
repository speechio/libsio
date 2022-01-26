#ifndef SIO_LANGUAGE_MODEL_H
#define SIO_LANGUAGE_MODEL_H

#include <base/kaldi-common.h>
#include <fst/fstlib.h>
#include <fst/fst-decl.h>
#include <lm/const-arpa-lm.h>

namespace sio {

// virtual interface for generic language model in FST representation
template<class Arc>
class LmFst {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;

  virtual StateId Start() = 0;

  virtual Weight Final(StateId s) = 0;

  virtual bool GetArc(StateId s, Label ilabel, Arc *oarc) = 0;

  virtual ~LmFst() { }
};


template<class Arc>
class NgramLmFst: public LmFst<Arc> {
 public:
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;

  explicit NgramLmFst(const fst::Fst<Arc> *fst) : fst_(fst) {
//#ifdef KALDI_PARANOID
    SIO_CHECK(fst_.Properties(fst::kILabelSorted|fst::kIDeterministic, true) ==
                (fst::kILabelSorted|fst::kIDeterministic) &&
                "Input FST is not i-label sorted and deterministic.");
//#endif
  }

  virtual ~NgramLmFst() { }

  StateId Start() { return fst_->Start(); }

  bool GetArc(StateId s, Label ilabel, Arc *oarc) {
    SIO_CHECK(ilabel != 0); //  We don't allow GetArc for epsilon.

    fst::SortedMatcher<fst::Fst<Arc> > sm(*fst_, fst::MATCH_INPUT, 1);
    sm.SetState(s);
    if (sm.Find(ilabel)) {
      const Arc &arc = sm.Value();
      *oarc = arc;
      return true;
    } else {
      Weight backoff_w;
      StateId backoff_state = GetBackoffState(s, &backoff_w);
      if (backoff_state == fst::kNoStateId) return false;
      if (!this->GetArc(backoff_state, ilabel, oarc)) return false;
      oarc->weight = Times(oarc->weight, backoff_w);
      return true;
    }
  }

  Weight Final(StateId state) {
    Weight w = fst_->Final(state);
    if (w != Weight::Zero()) return w;
    Weight backoff_w;
    StateId backoff_state = GetBackoffState(state, &backoff_w);
    if (backoff_state == fst::kNoStateId) return Weight::Zero();
    else return Times(backoff_w, this->Final(backoff_state));
  }

 private:
  inline StateId GetBackoffState(StateId s, Weight *w) {
    fst::ArcIterator<fst::Fst<Arc> > aiter(*fst_, s);
    if (aiter.Done()) // no arcs.
      return fst::kNoStateId;
    const Arc &arc = aiter.Value();
    if (arc.ilabel == 0) {
      *w = arc.weight;
      return arc.nextstate;
    } else {
      return fst::kNoStateId;
    }
  }

  const fst::Fst<Arc> *fst_;
};


template<class Arc>
class ScaleCacheLmFst: public LmFst<Arc> {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;

  /// We don't take ownership of this pointer.  The argument is "really" const.
  ScaleCacheLmFst(LmFst<Arc> *fst, float scale, 
                  StateId num_cached_arcs = 100000)
   : fst_(fst),
     scale_(scale),
     num_cached_arcs_(num_cached_arcs),
     cached_arcs_(num_cached_arcs)
  {
    SIO_CHECK(num_cached_arcs > 0);
    for (StateId i = 0; i < num_cached_arcs; i++)
      cached_arcs_[i].first = fst::kNoStateId; // Invalidate all elements of the cache.
  }

  virtual StateId Start() { return fst_->Start(); }

  /// We don't bother caching the final-probs, just the arcs.
  virtual Weight Final(StateId s) {
    // Note: Weight is indirectly a typedef to TropicalWeight.
    Weight final = fst_->Final(s);
    if (final == Weight::Zero()) {
      return Weight::Zero();
    } else {
      return fst::TropicalWeight(final.Value() * scale_);
    }
  }

  virtual bool GetArc(StateId s, Label ilabel, Arc *oarc) {
    // Note: we don't cache anything in case a requested arc does not exist.
    // In the uses that we imagine this will be put to, essentially all the
    // requested arcs will exist.  This only affects efficiency.
    SIO_CHECK(s >= 0 && ilabel != 0);
    size_t index = this->GetIndex(s, ilabel);
    if (cached_arcs_[index].first == s &&
        cached_arcs_[index].second.ilabel == ilabel) {
      *oarc = cached_arcs_[index].second;
      oarc->weight = fst::TropicalWeight(oarc->weight.Value() * scale_);
      return true;
    } else {
      Arc arc;
      if (fst_->GetArc(s, ilabel, &arc)) {
        cached_arcs_[index].first = s;
        cached_arcs_[index].second = arc;
        *oarc = arc;
        oarc->weight = fst::TropicalWeight(oarc->weight.Value() * scale_);
        return true;
      } else {
        return false;
      }
    }
  }

 private:
  // Get index for cached arc.
  inline size_t GetIndex(StateId src_state, Label ilabel) {
    const StateId p1 = 26597, p2 = 50329; // these are two
    // values that I drew at random from a table of primes.
    // note: num_cached_arcs_ > 0.

    // We cast to size_t before the modulus, to ensure the
    // result is positive.
    return static_cast<size_t>(src_state * p1 + ilabel * p2) %
        static_cast<size_t>(num_cached_arcs_);
  }

  LmFst<Arc> *fst_;
  float scale_;
  StateId num_cached_arcs_;
  std::vector<std::pair<StateId, Arc> > cached_arcs_;
};

template<class Arc>
class ComposeLmFst: public LmFst<Arc> {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;

  /// Note: constructor does not "take ownership" of the input fst's.  The input
  /// fst's should be treated as const, in that their contents do not change,
  /// but they are not const as the DeterministicOnDemandFst's data-access
  /// functions are not const, for reasons relating to caching.
  ComposeLmFst(LmFst<Arc> *fst1, LmFst<Arc> *fst2) 
   : fst1_(fst1), fst2_(fst2) 
  {
    SIO_CHECK(fst1 != NULL && fst2 != NULL);
    if (fst1_->Start() == -1 || fst2_->Start() == -1) {
      start_state_ = -1;
      next_state_ = 0; // actually we don't care about this value.
    } else {
      start_state_ = 0;
      std::pair<StateId,StateId> start_pair(fst1_->Start(), fst2_->Start());
      state_map_[start_pair] = start_state_;
      state_vec_.push_back(start_pair);
      next_state_ = 1;
    }
  }

  virtual StateId Start() { return start_state_; }

  virtual Weight Final(StateId s) {
    SIO_CHECK(s < static_cast<StateId>(state_vec_.size()));
    const std::pair<StateId, StateId> &pr (state_vec_[s]);
    return Times(fst1_->Final(pr.first), fst2_->Final(pr.second));  
  }

  virtual bool GetArc(StateId s, Label ilabel, Arc *oarc) {
    typedef typename MapType::iterator IterType;
    SIO_CHECK(ilabel != 0 &&
          "This program expects epsilon-free compact lattices as input");
    SIO_CHECK(s < static_cast<StateId>(state_vec_.size()));
    const std::pair<StateId, StateId> pr (state_vec_[s]);

    Arc arc1;
    if (!fst1_->GetArc(pr.first, ilabel, &arc1)) return false;
    if (arc1.olabel == 0) { // There is no output label on the
      // arc, so only the first state changes.
      std::pair<const std::pair<StateId, StateId>, StateId> new_value(
          std::pair<StateId, StateId>(arc1.nextstate, pr.second),
          next_state_);

      std::pair<IterType, bool> result = state_map_.insert(new_value);
      oarc->ilabel = ilabel;
      oarc->olabel = 0;
      oarc->nextstate = result.first->second;
      oarc->weight = arc1.weight;
      if (result.second == true) { // was inserted
        next_state_++;
        const std::pair<StateId, StateId> &new_pair (new_value.first);
        state_vec_.push_back(new_pair);
      }
      return true;
    }
    // There is an output label, so we need to traverse an arc on the
    // second fst also.
    Arc arc2;
    if (!fst2_->GetArc(pr.second, arc1.olabel, &arc2)) return false;
    std::pair<const std::pair<StateId, StateId>, StateId> new_value(
        std::pair<StateId, StateId>(arc1.nextstate, arc2.nextstate),
        next_state_);
    std::pair<IterType, bool> result =
        state_map_.insert(new_value);
    oarc->ilabel = ilabel;
    oarc->olabel = arc2.olabel;
    oarc->nextstate = result.first->second;
    oarc->weight = Times(arc1.weight, arc2.weight);
    if (result.second == true) { // was inserted
      next_state_++;
      const std::pair<StateId, StateId> &new_pair (new_value.first);
      state_vec_.push_back(new_pair);
    }
    return true;
  }

 private:
  LmFst<Arc> *fst1_;
  LmFst<Arc> *fst2_;
  typedef unordered_map<std::pair<StateId, StateId>, StateId, kaldi::PairHasher<StateId> > MapType;
  MapType state_map_;
  std::vector<std::pair<StateId, StateId> > state_vec_; // maps from
  // StateId to pair.
  StateId next_state_;
  StateId start_state_;
};

/**
 This class wraps a ConstArpaLm format language model with the interface defined
 in LmFst
 */
template <class Arc>
class ConstArpaLmFst : public LmFst<Arc> {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;

  explicit ConstArpaLmFst(const kaldi::ConstArpaLm* lm) 
   : lm_(*lm) 
  {
    std::vector<Label> bos_state(1, lm_.BosSymbol());
    state_to_wseq_.push_back(bos_state);
    wseq_to_state_[bos_state] = 0;
    start_state_ = 0;
  }

  // We cannot use "const" because the pure virtual function in the interface is
  // not const.
  virtual StateId Start() { return start_state_; }

  // We cannot use "const" because the pure virtual function in the interface is
  // not const.
  virtual Weight Final(StateId s) {
    // At this point, we should have created the state.
    SIO_CHECK(static_cast<size_t>(s) < state_to_wseq_.size());
    const std::vector<Label>& wseq = state_to_wseq_[s];
    float logprob = lm_.GetNgramLogprob(lm_.EosSymbol(), wseq);
    return Weight(-logprob);  
  }

  virtual bool GetArc(StateId s, Label ilabel, Arc* oarc) {
    // At this point, we should have created the state.
    SIO_CHECK(static_cast<size_t>(s) < state_to_wseq_.size());
    std::vector<Label> wseq = state_to_wseq_[s];

    float logprob = lm_.GetNgramLogprob(ilabel, wseq);
    if (logprob == -std::numeric_limits<float>::infinity()) {
      return false;
    }

    // Locates the next state in ConstArpaLm. Note that OOV and backoff have been
    // taken care of in ConstArpaLm.
    wseq.push_back(ilabel);
    while (wseq.size() >= lm_.NgramOrder()) {
      // History state has at most lm_.NgramOrder() -1 words in the state.
      wseq.erase(wseq.begin(), wseq.begin() + 1);
    }
    while (!lm_.HistoryStateExists(wseq)) {
      SIO_CHECK(wseq.size() > 0);
      wseq.erase(wseq.begin(), wseq.begin() + 1);
    }

    std::pair<const std::vector<Label>, StateId> wseq_state_pair(
      wseq, static_cast<Label>(state_to_wseq_.size()));

    // Attemps to insert the current <wseq_state_pair>. If the pair already exists
    // then it returns false.
    typedef typename MapType::iterator IterType;
    std::pair<IterType, bool> result = wseq_to_state_.insert(wseq_state_pair);

    // If the pair was just inserted, then also add it to <state_to_wseq_>.
    if (result.second == true)
      state_to_wseq_.push_back(wseq);

    // Creates the arc.
    oarc->ilabel = ilabel;
    oarc->olabel = ilabel;
    oarc->nextstate = result.first->second;
    oarc->weight = Weight(-logprob);

    return true;  
  }

 private:
  typedef unordered_map<std::vector<Label>, StateId, kaldi::VectorHasher<Label> > MapType;
  StateId start_state_;
  MapType wseq_to_state_;
  std::vector<std::vector<Label> > state_to_wseq_;
  const kaldi::ConstArpaLm& lm_;
};

} // namespace sio

#endif
