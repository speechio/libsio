#include "sio/error.h"

#include <stdio.h>
#include <stdlib.h>

namespace sio {

const char* ErrorMsg(Error err) {
    switch (err) {
        case Error::OK: return "(OK)";
        case Error::OutOfMemory: return "out of memory";
        case Error::AssertionFailure: return "assertion failure";
        case Error::InvalidFileHandle: return "invalid file handle";
        case Error::VocabularyMismatch: return "mismatched vocabulary of tokenizer and KenLM";
        case Error::NoRecognitionResult: return "no recognition result";
        case Error::Unknown: return "(unknown error)";
    }
    return nullptr; /* avoid warning */
}


void Panic(const char* file, size_t line, const char* func, Error err) {
    fprintf(stderr, "[panic](%s:%d:%s) %s\n", file, line, func, ErrorMsg(err));
    fflush(stderr);
    abort();
}

} // namespace sio

