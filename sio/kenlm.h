#ifndef SIO_KENLM_H
#define SIO_KENLM_H

#include "lm/word_index.hh"
#include "lm/model.hh"
#include "util/murmur_hash.hh"

#include "sio/base.h"
#include "sio/tokenizer.h"

namespace sio {

/*
 * Wrapper class for KenLm model, the underlying model structure can be either "trie" or "probing".
 * Main purposes:
 *  1. loads & holds kenlm model resources (with ownership)
 *  2. handles the index mapping between tokenizer & kenlm vocab
 *  3. provides a stateless ngram query engine, can be shared by multiple threads
 */
class KenLM {
public:
    using State = lm::ngram::State;
    using WordId = lm::WordIndex;

    // this provides a fast hash function to upper level fst wrapper class,
    // to maintain the mapping between underlying lm states and fst state indexes
    struct StateHasher {
        inline size_t operator()(const State &s) const noexcept {
            return util::MurmurHashNative(s.words, sizeof(WordId) * s.Length());
        }
    };

private:
    Unique<lm::base::Model*> model_;

    // KenLM has internal word indexing system,
    // need an index mapping from external tokens to kenlm word indexes
    Vec<WordId> token_to_word_;

public:
    Error Load(
        const Str& filepath,
        const Tokenizer& tokenizer,
        util::LoadMethod load_method = util::LoadMethod::POPULATE_OR_READ
    )
    {
        SIO_CHECK(model_ == nullptr);

        lm::ngram::Config config;
        config.load_method = load_method;
        model_.reset(lm::ngram::LoadVirtual(filepath.c_str(), config));
        SIO_CHECK(model_ != nullptr);

        const lm::base::Vocabulary& vocab = model_->BaseVocabulary();

        SIO_CHECK(token_to_word_.empty());
        WordId unk = vocab.Index(tokenizer.Token(tokenizer.unk).c_str());
        SIO_CHECK_EQ(unk, 0); // <unk> is always indexed as 0 in KenLM
        token_to_word_.resize(
            tokenizer.Size(),
            unk
        );  // mapping all tokens to unk as initialization

        for (const auto& kv : tokenizer.index_to_token) {
            TokenId t = kv.first;
            const Str& token = kv.second;

            WordId w = vocab.Index(token.c_str());
            token_to_word_[t] = w;
        }

        //dbg(token_to_word_);

        return Error::OK;
    }


    inline WordId GetWordIndex(const Str& word) const {
        return model_->BaseVocabulary().Index(word.c_str());
    }
    inline WordId GetWordIndex(TokenId t) const {
        return token_to_word_[t];
    }


    void SetStateToBeginOfSentence(State *s) const { model_->BeginSentenceWrite(s); }
    void SetStateToNull(State *s) const { model_->NullContextWrite(s); }


    inline f32 Score(const State* istate, WordId word, State* ostate) const {
        return model_->BaseScore(istate, word, ostate);
    }

}; // class KenLM

} // namespace sio

#endif
