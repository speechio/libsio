#ifndef SIO_ERROR_H
#define SIO_ERROR_H

#include <stdio.h>
#include <stdlib.h>

#include "sio/macro.h"

namespace sio {

enum Error {
    OK,
    OutOfMemory,
    AssertionFailure,
    OpenFileFailure,
    VocabularyMismatch,
    NoRecognitionResult,
    Unknown,
}; // enum Error


inline const char* ErrorMsg(Error err) {
    switch (err) {
        case Error::OK: return "(OK)";
        case Error::OutOfMemory: return "out of memory";
        case Error::AssertionFailure: return "assertion failure";
        case Error::OpenFileFailure: return "cannot open file";
        case Error::VocabularyMismatch: return "mismatched vocabulary of tokenizer and KenLM";
        case Error::NoRecognitionResult: return "no recognition result";
        case Error::Unknown: return "(unknown error)";
    }

    return nullptr; /* avoid warning */
}


inline void Panic(const char* file, int line, const char* func, Error err) {
    fprintf(stderr, "[panic](%s:%d:%s) %s\n", file, line, func, ErrorMsg(err));
    fflush(stderr);
    abort();
}

#define SIO_PANIC(err) ::sio::Panic(SIO_FILE_REPR, SIO_LINE_REPR, SIO_FUNC_REPR, err)

} // namespace sio

#endif

