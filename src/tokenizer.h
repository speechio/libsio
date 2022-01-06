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
	Map<int, Str> index_to_token;
	Map<Str, int> token_to_index;
	//sentencepiece::SentencePieceProcessor spm;

	void Load(const Str& tokenizer_vocab) {
		std::ifstream is(tokenizer_vocab);
		Str line;
		for (int index = 0; std::getline(is, line); index++) {
			Vec<Str> cols = absl::StrSplit(line, absl::ByAnyChar(" \t"));
			SIO_CHECK_EQ(cols.size(), 2); // token prob
			Str token = cols[0];
			index_to_token[index] = token;
			token_to_index[token] = index;
		}

		//dbg(index_to_token);
		//dbg(token_to_index);
	}

	//Error LoadModel(const Str& tokenizer_model) {
	//	const auto status = spm.Load(tokenizer_model);
	//	if (!status.ok()) {
	//		SIO_ERROR(Error::TokenizerLoadFailure) << status.ToString() << "\n";
	//	}
	//	return Error::OK;
	//}

	size_t Size() {
		return index_to_token.size();
	}
}; // class Tokenizer
}  // namespace sio
#endif
