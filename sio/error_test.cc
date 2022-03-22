#include "sio/error.h"

#include <gtest/gtest.h>
#include <string.h>

namespace sio {

TEST(Error, Basic) {
    Error err = Error::OK;
    EXPECT_EQ(strcmp((ErrorCStr(err)), "(OK)"), 0);
}

} // namespace sio
