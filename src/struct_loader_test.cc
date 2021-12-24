#include <gtest/gtest.h>
#include <fstream>

#include "sio/error.h"
#include "sio/str.h"
#include "sio/struct_loader.h"

namespace sio {

TEST(StructLoader, Basic) {
  struct Foo {
    Str foo_str;
    int foo_int;

    Error RegisterToLoader(StructLoader* loader, const Str module = "") {
      loader->Register(module, ".foo_str", &foo_str);
      loader->Register(module, ".foo_int", &foo_int);
      return Error::OK;
    }
  };

  struct Bar {
    bool online;
    int num_workers;
    float sample_rate;
    Str model;
    Foo foo;

    Error RegisterToLoader(StructLoader* loader, const Str module = "") {
      loader->Register(module, ".online", &online);
      loader->Register(module, ".num_workers", &num_workers);
      loader->Register(module, ".sample_rate", &sample_rate);
      loader->Register(module, ".model", &model);
      foo.RegisterToLoader(loader, module + ".foo");
      return Error::OK;
    }
  };

  Bar bar;
  StructLoader loader;
  bar.RegisterToLoader(&loader);
  loader.Print();

  std::ifstream is("testdata/stt.json");
  Json j;
  is >> j;
  loader.Load(j);
  loader.Print();

  EXPECT_EQ(bar.online, true);
  EXPECT_EQ(bar.num_workers, 8);
  EXPECT_EQ(bar.sample_rate, 16000.0);
  EXPECT_EQ(bar.model, "model_dir/model.bin");
  EXPECT_EQ(bar.foo.foo_str, "this is foo string");
  EXPECT_EQ(bar.foo.foo_int, 12345);
}

} // namespace sio