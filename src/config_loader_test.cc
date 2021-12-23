#include <gtest/gtest.h>
#include "sio/error.h"
#include "sio/str.h"
#include "sio/config_loader.h"

namespace sio {

TEST(ConfigLoader, Basic) {
  struct Foo {
    Str foo_str;
    int foo_int;

    Error Register(ConfigLoader* loader, const std::string module = "") {
      loader->Add(module, ".foo_str", &foo_str);
      loader->Add(module, ".foo_int", &foo_int);
      return Error::OK;
    }
  };

  struct Bar {
    bool online;
    int num_workers;
    float sample_rate;
    std::string model;
    Foo foo;

    Error Register(ConfigLoader* loader, const std::string module = "") {
      loader->Add(module, ".online", &online);
      loader->Add(module, ".num_workers", &num_workers);
      loader->Add(module, ".sample_rate", &sample_rate);
      loader->Add(module, ".model", &model);
      foo.Register(loader, module + ".foo");
      return Error::OK;
    }
  };

  Bar bar;
  ConfigLoader loader;
  bar.Register(&loader);
  loader.Print();
  loader.Load("testdata/stt.json");
  loader.Print();

  EXPECT_EQ(bar.online, true);
  EXPECT_EQ(bar.num_workers, 8);
  EXPECT_EQ(bar.sample_rate, 16000.0);
  EXPECT_EQ(bar.model, "model_dir/model.bin");
  EXPECT_EQ(bar.foo.foo_str, "this is foo string");
  EXPECT_EQ(bar.foo.foo_int, 12345);
}

} // namespace sio