#ifndef SIO_MEAN_VAR_NORM_H
#define SIO_MEAN_VAR_NORM_H

#include <string>
#include <iostream>
#include <vector>

#include "matrix/kaldi-vector.h"

#include "sio/common.h"

namespace sio {
class MeanVarNorm {
    int dim_ = 0;
    Vec<f64> shift_;
    Vec<f64> scale_;

public:
    Error Load(std::string mean_var_norm_file) {
        /*
        Format of mean_var_norm file, three lines:
        line1: norm_vector_dimension
        line2: shift_ vector, no brackets, elements seperated by comma or whitespaces
        line3: scale_ vector, same as above

        Example mean_var_norm.txt:
        3
        -8.505237579345703 -8.91793441772461 -10.091235160827637
        0.6330024600028992 0.49299687147140503 0.37579503655433655

        */
        std::ifstream is(mean_var_norm_file);
        std::string line;
        Vec<Str> cols;

        // header line
        std::getline(is, line);
        dim_ = std::stoi(line);

        // shift
        std::getline(is, line);
        cols = absl::StrSplit(line, absl::ByAnyChar(" \t,"), absl::SkipWhitespace());
        SIO_CHECK_EQ(cols.size(), dim_) << "mean norm dim should be consistent with header line";
        shift_.clear();
        shift_.resize(dim_, 0.0);
        for (int i = 0; i != dim_; i++) {
            shift_[i] = std::stod(cols[i]);
        }

        // scale
        std::getline(is, line);
        cols = absl::StrSplit(line, absl::ByAnyChar(" \t,"), absl::SkipWhitespace());
        SIO_CHECK_EQ(cols.size(), dim_) << "var norm dim should be consistent with header line";
        scale_.clear();
        scale_.resize(dim_, 0.0);
        for (int i = 0; i != dim_; i++) {
            scale_[i] = std::stod(cols[i]);
        }

        return Error::OK;
    }


    void Normalize(kaldi::VectorBase<f32> *frame) const {
        SIO_CHECK(frame != nullptr) << "null input to mvn ?";
        SIO_CHECK_EQ(frame->Dim(), dim_) << "feature dim inconsistent with mvn dim";
        for (int i = 0; i < dim_; i++) {
            // use quote for elem referencing, see:
            // https://github.com/kaldi-asr/kaldi/blob/master/src/matrix/kaldi-vector.h#L82
            (*frame)(i) += shift_[i];
            (*frame)(i) *= scale_[i];
        }
    }

}; // class MeanVarNorm
}  // namespace sio
#endif
