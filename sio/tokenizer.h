#ifndef SIO_TOKENIZER_H
#define SIO_TOKENIZER_H

#include <fstream>

#include "sio/common.h"
//#include "sio/dbg.h"

namespace sio {

using TokenId = i32;

struct Tokenizer {
    static const TokenId kUndefined = -1;

    Map<TokenId, Str> index_to_token;
    Map<Str, TokenId> token_to_index;

    TokenId blk = kUndefined;
    TokenId unk = kUndefined;
    TokenId bos = kUndefined;
    TokenId eos = kUndefined;


    Error Load(const Str& tokenizer_vocab) {
        std::ifstream is(tokenizer_vocab);
        SIO_CHECK(is.good());
        Str line;
        for (TokenId index = 0; std::getline(is, line); index++) {
            Vec<Str> cols = absl::StrSplit(line, absl::ByAnyChar(" \t"), absl::SkipWhitespace());
            SIO_CHECK_EQ(cols.size(), 2); // token prob
            Str token = cols[0];
            index_to_token[index] = token;
            token_to_index[token] = index;

            if (token == "<blk>" || token == "<blank>" || token == "<pad>") {
                blk = index;
            } else if (token == "<unk>" || token == "<UNK>") {
                unk = index;
            } else if (token == "<s>" || token == "<bos>" || token == "<sos>") {
                bos = index;
            } else if (token == "</s>" || token == "<eos>") {
                eos = index;
            }
        }

        // Use blk & unk interchangably if only one of them is undefined
        if (blk == kUndefined && unk != kUndefined) { blk = unk; }
        if (unk == kUndefined && blk != kUndefined) { unk = blk; }

        //dbg(index_to_token);
        //dbg(token_to_index);

        // Post-condition checks
        SIO_CHECK(blk != kUndefined);
        SIO_CHECK(unk != kUndefined);
        SIO_CHECK(bos != kUndefined);
        SIO_CHECK(eos != kUndefined);

        return Error::OK;
    }


    size_t Size() const {
        return index_to_token.size();
    }


    const Str& Token(TokenId t) const {
        return index_to_token.at(t);
    }


    TokenId Index(const Str& token) const {
        return token_to_index.at(token);
    }


    bool IsSpecial(TokenId t) const {
        StrView token = Token(t);
        return absl::StartsWith(token, "<") && absl::EndsWith(token, ">");
    }

}; // class Tokenizer
}  // namespace sio
#endif
