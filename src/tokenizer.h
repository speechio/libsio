#ifndef SIO_TOKENIZER_H
#define SIO_TOKENIZER_H

#include "sio/common.h"
//#include "sio/dbg.h"

namespace sio {
struct Tokenizer {
    static const i32 kUndefined = -1;

    Map<i32, Str> index_to_token;
    Map<Str, i32> token_to_index;

    i32 blk = kUndefined;
    i32 unk = kUndefined;
    i32 bos = kUndefined;
    i32 eos = kUndefined;


    Error Load(const Str& tokenizer_vocab) {
        std::ifstream is(tokenizer_vocab);
        Str line;
        for (i32 index = 0; std::getline(is, line); index++) {
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


    const Str& Token(i32 index) const {
        return index_to_token.at(index);
    }


    i32 Index(const Str& token) const {
        return token_to_index.at(token);
    }


    bool IsSpecial(i32 token_index) const {
        StrView token = Token(token_index);
        return absl::StartsWith(token, "<") && absl::EndsWith(token, ">");
    }

}; // class Tokenizer
}  // namespace sio
#endif
