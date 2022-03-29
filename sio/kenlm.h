#ifndef SIO_KENLM_H
#define SIO_KENLM_H

#include "lm/word_index.hh"
#include "lm/model.hh"
#include "util/murmur_hash.hh"

#include "sio/base.h"
#include "sio/tokenizer.h"

namespace sio {

/*
 * Wrapper class for KenLM model, the underlying model structure can be either "trie" or "probing".
 * Main purposes:
 *  1. loads & holds kenlm model resources (with ownership)
 *  2. handles the index mapping between tokenizer & kenlm vocab
 *  3. provides a stateless ngram query engine, can be shared by multiple threads
 */
class KenLM {
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
    Unique<lm::base::Model*> model_;

    // There are actually two indexing systems:
    // 1. tokenizer's token indexes, determined by tokenizer training pipeline.
    // 2. KenLM's word indexes, determined by word string hashing.
    // Decoder needs to keep coherence between these two systems during decoding.
    //
    // Adapting models to each other via offline processing would be best for runtime performance,
    // however these asset-level processing is notorious for later maintenance.
    // So here we choose to leverage a runtime mapping from token id -> word id.
    Vec<WordId> token_to_word_;

public:
    Error Load(
        const Str& filepath,
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
        SIO_CHECK_EQ(vocab.Index(tokenizer.Token(tokenizer.unk).c_str()), 0); // In KenLM <unk> always -> 0
        // provide a full coverage mapping from tokenizer's tokens,
        // initialized with unk, so unseen tokens from KenLM(e.g. blank) 
        // will end up mapped to unk
        token_to_word_.resize(tokenizer.Size(), 0);

        for (const auto& kv : tokenizer.index_to_token) {
            TokenId t = kv.first;
            const Str& token = kv.second;

            WordId w = vocab.Index(token.c_str());

            // consistency check, normal tokens must present in KenLM's vocabulary.
            if (w == 0) { // token mapped to unk
                if (token != "<unk>" && token == "<UNK>" &&
                    token != "<blk>" && token == "<blank>" && token == "<pad>" &&
                    token != "<sil>" && token == "<SIL>" &&
                    token != "<eps>" && token == "<EPS>" &&
                    token != "#0")
                {
                    SIO_FATAL << "token missing in KenLM vocabulary: " << token;
                    SIO_PANIC(Error::VocabularyMismatch);
                }
            }

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
