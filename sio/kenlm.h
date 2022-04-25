#ifndef SIO_KENLM_H
#define SIO_KENLM_H

#include "lm/word_index.hh"
#include "lm/model.hh"
#include "util/murmur_hash.hh"

#include "sio/base.h"
#include "sio/tokenizer.h"

namespace sio {

/*
** Wrapper class for KenLm model, the underlying model structure can be either "trie" or "probing".
** Main purposes:
**  1. loads & holds kenlm model resources (with ownership)
**  2. handles the index mapping between tokenizer & kenlm vocab
**  3. KenLm yields log10 score, whereas ASR decoder normally uses natural log, a conversion is need.
**  4. provides a stateless ngram query engine, can be shared by multiple threads
*/
class KenLm {
public:
    using State = lm::ngram::State;
    using WordId = lm::WordIndex;

    // This provides a fast hash function for upper-level stateful LM caches
    struct StateHasher {
        inline size_t operator()(const State &s) const noexcept {
            return util::MurmurHashNative(s.words, sizeof(WordId) * s.Length());
        }
    };

private:
    // There are actually two indexing systems:
    // 1. tokenizer's token indexes, determined by tokenizer training pipeline.
    // 2. KenLm's word indexes, determined by word string hashing.
    // Decoder needs to handle them coherently.
    //
    // Possible solutions:
    // A: Adapt one index to the other via offline resource processing:
    //   Pros: zero-cost for runtime performance
    //   Cons: increased complexity of offline pipeline & resource maintenance
    // B: Convert indexes between token id <-> word id on-the-fly during decoding
    //   Pros: avoid extra offline processings and resources generation
    //   Cons: index mapping has runtime overhead
    //
    // Decision: for sake of simplicity, choose solution B
    vec<WordId> token_to_word_;

    Unique<lm::base::Model*> model_;

public:
    Error Load(
        const str& filepath,
        const Tokenizer& tokenizer,
        util::LoadMethod load_method = util::LoadMethod::POPULATE_OR_READ)
    {
        SIO_CHECK(model_ == nullptr);

        lm::ngram::Config config;
        config.load_method = load_method;
        model_.reset(lm::ngram::LoadVirtual(filepath.c_str(), config));
        SIO_CHECK(model_ != nullptr);

        const lm::base::Vocabulary& vocab = model_->BaseVocabulary();

        SIO_CHECK(token_to_word_.empty());
        SIO_CHECK_EQ(vocab.Index(tokenizer.Token(tokenizer.unk).c_str()), 0); // In KenLm <unk> always -> 0
        // provide a full coverage mapping from tokenizer's tokens,
        // initialized with unk, so unseen tokens from KenLm(e.g. blank) 
        // will end up mapped to unk
        token_to_word_.resize(tokenizer.Size(), 0);

        for (TokenId t = 0; t != tokenizer.Size(); t++) {
            const str& token = tokenizer.Token(t);
            WordId w = vocab.Index(token.c_str());

            // all normal tokens should be included in KenLm's vocabulary
            if (w == 0) { // token mapped to unk
                if (token != "<unk>" && token == "<UNK>" &&
                    token != "<blk>" && token == "<blank>" && token == "<pad>" &&
                    token != "<sil>" && token == "<SIL>" &&
                    token != "<eps>" && token == "<EPS>" &&
                    token != "#0")
                {
                    SIO_FATAL << "token missing in KenLm vocabulary: " << token;
                    SIO_PANIC(Error::VocabularyMismatch);
                }
            }

            token_to_word_[t] = w;
        }
        //dbg(token_to_word_);

        return Error::OK;
    }


    inline WordId GetWordIndex(const str& word) const {
        return model_->BaseVocabulary().Index(word.c_str());
    }
    inline WordId GetWordIndex(TokenId t) const {
        return token_to_word_[t];
    }


    void SetStateToBeginOfSentence(State *s) const { model_->BeginSentenceWrite(s); }
    void SetStateToNull(State *s) const { model_->NullContextWrite(s); }


    inline f32 Score(const State* istate, WordId word, State* ostate) const {
        // log10 -> ln conversion
        return SIO_LN10 * model_->BaseScore(istate, word, ostate);
    }

}; // class KenLm

} // namespace sio

#endif
