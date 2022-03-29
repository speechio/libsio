#ifndef SIO_MEAN_VAR_NORM_H
#define SIO_MEAN_VAR_NORM_H

#include <string>
#include <iostream>
#include <vector>

#include "matrix/kaldi-vector.h"

#include "sio/base.h"

namespace sio {

struct MeanVarNorm {
    int dim = 0;
    Vec<f64> shift;
    Vec<f64> scale;

    Error Load(std::string mean_var_norm_file) {
        /*
        Format of mean_var_norm file, three lines:
        line1: norm_vector_dimension
        line2: shift vector, no brackets, elements seperated by comma or whitespaces
        line3: scale vector, same as above

        Example:
        3
        -8.505237579345703 -8.91793441772461 -10.091235160827637
        0.6330024600028992 0.49299687147140503 0.37579503655433655

        */
        std::ifstream is(mean_var_norm_file);
        SIO_CHECK(is.good());

        std::string line;
        Vec<Str> cols;

        // parse header line
        std::getline(is, line);
        this->dim = std::stoi(line);

        // parse shift vector
        std::getline(is, line);
        cols = absl::StrSplit(line, absl::ByAnyChar(" \t,"), absl::SkipWhitespace());
        SIO_CHECK_EQ(cols.size(), this->dim); // mean norm dim inconsistent with header
        this->shift.clear();
        this->shift.resize(this->dim, 0.0);
        for (int i = 0; i != this->dim; i++) {
            this->shift[i] = std::stod(cols[i]);
        }

        // parse scale vector
        std::getline(is, line);
        cols = absl::StrSplit(line, absl::ByAnyChar(" \t,"), absl::SkipWhitespace());
        SIO_CHECK_EQ(cols.size(), this->dim); // variance norm dim inconsistent with header
        this->scale.clear();
        this->scale.resize(this->dim, 0.0);
        for (int i = 0; i != this->dim; i++) {
            this->scale[i] = std::stod(cols[i]);
        }

        return Error::OK;
    }


    void Normalize(kaldi::VectorBase<f32> *frame) const {
        SIO_CHECK(frame != nullptr);
        SIO_CHECK_EQ(frame->Dim(), this->dim); // feature dim inconsistent with MVN

        for (int i = 0; i < this->dim; i++) {
            // use quote for elem referencing, see:
            // https://github.com/kaldi-asr/kaldi/blob/master/src/matrix/kaldi-vector.h#L82
            (*frame)(i) += this->shift[i];
            (*frame)(i) *= this->scale[i];
        }
    }

}; // struct MeanVarNorm
}  // namespace sio
#endif
