#ifndef SIO_TOKENIZER_H
#define SIO_TOKENIZER_H

#include <fstream>

#include "sio/base.h"

namespace sio {

using TokenId = i32;

class Tokenizer {
    hashmap<TokenId, str> index_to_token_; // TODO: consider vector implementation
    hashmap<str, TokenId> token_to_index_;

public:
    TokenId blk = 0;
    TokenId unk = 0;
    TokenId bos = 0;
    TokenId eos = 0;

    Error Load(const str& tokenizer_vocab) {
        std::ifstream is(tokenizer_vocab);
        SIO_CHECK(is.good());
        str line;
        for (TokenId i = 0; std::getline(is, line); i++) {
            vec<str> cols = absl::StrSplit(line, absl::ByAnyChar(" \t"), absl::SkipWhitespace());
            SIO_CHECK_EQ(cols.size(), 2); // sentencepiece's vocab: "token prob"

            str t = cols[0];
            index_to_token_[i] = t;
            token_to_index_[t] = i;

            if (t == "<blk>" || t == "<blank>" || t == "<pad>") {
                blk = i;
            } else if (t == "<unk>" || t == "<UNK>") {
                unk = i;
            } else if (t == "<s>" || t == "<bos>" || t == "<sos>") {
                bos = i;
            } else if (t == "</s>" || t == "<eos>") {
                eos = i;
            }
        }
        //dbg(index_to_token_);
        //dbg(token_to_index_);

        // Post-condition checks
        SIO_CHECK(bos != 0);
        SIO_CHECK(eos != 0);

        return Error::OK;
    }


    size_t Size() const {
        return index_to_token_.size();
    }


    const str& Token(TokenId i) const {
        return index_to_token_.at(i);
    }


    TokenId Index(const str& t) const {
        return token_to_index_.at(t);
    }

}; // class Tokenizer
}  // namespace sio
#endif
