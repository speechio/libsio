#include <gtest/gtest.h>
#include <fstream>

#include "sio/base.h"
#include "sio/struct_loader.h"

namespace sio {

TEST(StructLoader, Basic) {

  struct Foo {
    Str foo_str;
    int foo_int;

    Error Register(StructLoader* loader, const Str module = "") {
      loader->AddEntry(module, ".foo_str", &foo_str);
      loader->AddEntry(module, ".foo_int", &foo_int);
      return Error::OK;
    }
  };

  struct Bar {
    bool online;
    int num_workers;
    float sample_rate;
    Str model;
    Foo foo;

    Error Register(StructLoader* loader, const Str module = "") {
      loader->AddEntry(module, ".online", &online);
      loader->AddEntry(module, ".num_workers", &num_workers);
      loader->AddEntry(module, ".feature_extractor.sample_rate", &sample_rate);
      loader->AddEntry(module, ".model", &model);
      foo.Register(loader, module + ".foo");
      return Error::OK;
    }
  };

  Bar bar;
  StructLoader loader;
  bar.Register(&loader);

  Json j = R"(
    { 
      "model": "model_dir/model.bin",
      "weights": [1.0, 2.0, 3.0],
      "online": true,
      "feature_extractor": {
        "type": "fbank",
        "sample_rate": 16000.0,
        "dither": 1.0,
        "num_mel_bins": 80,
        "mean_var_norm_file": "testdata/mean_var_norm.txt"
      },
      "num_workers": 8,
      "foo": {
        "foo_str": "this is foo string",
        "foo_int": 12345
      }
    }
  )"_json;

  loader.Print();
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
