#include "lm/word_index.hh"
#include "lm/model.hh"

namespace sio {
class KenLmFst {
    Unique<lm::base::Model*> kenlm_;
};
} // namespace sio

