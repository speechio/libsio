#include <gtest/gtest.h>
#include <string>

#include "sio/error.h"

namespace sio {

TEST(Error, Basic) {
	Error err = Error::OK;
	std::cerr << error_cstr(err) << "\n";
	EXPECT_EQ(std::string(error_cstr(err)), "(OK)");
}

TEST(Error, Panic) {
	//Error err = Error::OutOfMemory;
	//if (!!err) {
	//  SIO_PANIC(err);
	//}
}

TEST(Error, Unreachable) {
	//SIO_UNREACHABLE();
}

} // namespace sio
