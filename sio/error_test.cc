#include "sio/error.h"

#include <gtest/gtest.h>
#include <string>

namespace sio {

TEST(Error, Basic) {
    Error err = Error::OK;
    std::cerr << ErrorCStr(err) << "\n";
    EXPECT_EQ(std::string(ErrorCStr(err)), "(OK)");
}

TEST(Error, Panic) {
    //Error err = Error::OutOfMemory;
    //if (err != Error::OK) {
    //  SIO_PANIC(err);
    //}
}

} // namespace sio
