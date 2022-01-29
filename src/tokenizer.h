#ifndef SIO_TOKENIZER_H
#define SIO_TOKENIZER_H

//#include <sentencepiece_processor.h>
#include "sio/common.h"
//#include "sio/dbg.h"

/*
SentencePiece C++ API reference:
    https://github.com/google/sentencepiece/blob/master/doc/api.md
*/

namespace sio {
struct Tokenizer {
    Map<i32, Str> index_to_token;
    Map<Str, i32> token_to_index;

    Str blk_token = "<blk>";
    Str unk_token = "<unk>";
    Str bos_token = "<s>";
    Str eos_token = "</s>";

    i32 blk = 0;
    i32 unk = 0;
    i32 bos = 0;
    i32 eos = 0;
    //sentencepiece::SentencePieceProcessor spm;

    void Load(const Str& tokenizer_vocab) {
        std::ifstream is(tokenizer_vocab);
        Str line;
        for (i32 index = 0; std::getline(is, line); index++) {
            Vec<Str> cols = absl::StrSplit(line, absl::ByAnyChar(" \t"), absl::SkipWhitespace());
            SIO_CHECK_EQ(cols.size(), 2); // token prob
            Str token = cols[0];
            index_to_token[index] = token;
            token_to_index[token] = index;
        }

        //dbg(index_to_token);
        //dbg(token_to_index);

        blk = token_to_index.at(blk_token);
        unk = token_to_index.at(unk_token);
        bos = token_to_index.at(bos_token);
        eos = token_to_index.at(eos_token);
    }

    //Error LoadModel(const Str& tokenizer_model) {
    //    const auto status = spm.Load(tokenizer_model);
    //    if (!status.ok()) {
    //        SIO_ERROR(Error::TokenizerLoadFailure) << status.ToString() << "\n";
    //    }
    //    return Error::OK;
    //}


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
