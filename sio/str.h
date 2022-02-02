#ifndef SIO_STR_H
#define SIO_STR_H

#include <string>

#include <absl/strings/ascii.h>
#include <absl/strings/charconv.h>
#include <absl/strings/escaping.h>
#include <absl/strings/match.h>
#include <absl/strings/numbers.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_join.h>
#include <absl/strings/str_replace.h>
#include <absl/strings/str_split.h>
#include <absl/strings/strip.h>
#include <absl/strings/substitute.h>
#include <absl/strings/string_view.h>
#include <absl/strings/str_format.h>

namespace sio {
using Str = std::string;
using StrView = absl::string_view;
};

#endif
