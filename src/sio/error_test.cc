#include "sio/error.h"

#include <gtest/gtest.h>
#include <string.h>

namespace sio {

TEST(Error, Basic) {
    Error err = Error::OK;
    EXPECT_EQ(strcmp((ErrorMsg(err)), "(OK)"), 0);
}

TEST(Panic, Basic) {
    //Error err = Error::OutOfMemory;
    //if (err != Error::OK) {
    //  SIO_PANIC(err);
    //}
}

} // namespace sio
