#ifndef SIO_MEAN_VAR_NORM_H
#define SIO_MEAN_VAR_NORM_H

#include <string>
#include <iostream>
#include <vector>

#include "matrix/kaldi-vector.h"

#include "sio/check.h"
#include "sio/str.h"
namespace sio {
struct MeanVarNorm {
	/*
	Format of mean_var_norm file, three lines:
	line1: norm_vector_dimension
	line2: m_norm_shift vector, no brackets, elements seperated by comma or whitespaces
	line3: v_nrom_scale vector, same as above

	Example mean_var_norm.txt:
	3
	-8.505237579345703 -8.91793441772461 -10.091235160827637
	0.6330024600028992 0.49299687147140503 0.37579503655433655

	*/

	Error Load(std::string mean_var_norm_file) {
		std::ifstream is(mean_var_norm_file);

		std::string line;
		std::getline(is, line);
		dim = std::stoi(line);

		std::vector<std::string> cols;

		std::getline(is, line);
		cols = absl::StrSplit(line, absl::ByAnyChar(" \t,"), absl::SkipWhitespace());
		SIO_CHECK_EQ(cols.size(), dim) << "mean norm dim should be consistent with header line";
		m_norm_shift.clear();
		m_norm_shift.reserve(dim);
		for (int i = 0; i != dim; i++) {
			m_norm_shift.push_back(std::stod(cols[i]));
		}

		std::getline(is, line);
		cols = absl::StrSplit(line, absl::ByAnyChar(" \t,"), absl::SkipWhitespace());
		SIO_CHECK_EQ(cols.size(), dim) << "var norm dim should be consistent with header line";
		v_norm_scale.clear();
		v_norm_scale.reserve(dim);
		for (int i = 0; i != dim; i++) {
			v_norm_scale.push_back(std::stod(cols[i]));
		}

		return Error::OK;
	}

	void Normalize(kaldi::VectorBase<float> *frame) const {
		SIO_CHECK(frame != nullptr) << "null input to mvn ?";
		SIO_CHECK_EQ(frame->Dim(), dim) << "feature dim inconsistent with mvn dim";
		for (int i = 0; i < dim; i++) {
			// use quote for elem referencing, see:
			// https://github.com/kaldi-asr/kaldi/blob/master/src/matrix/kaldi-vector.h#L82
			(*frame)(i) += m_norm_shift[i];
			(*frame)(i) *= v_norm_scale[i];
		}
	}

	int dim = 0;
	std::vector<double> m_norm_shift;
	std::vector<double> v_norm_scale;
}; // class MeanVarNorm
}  // namespace sio
#endif
