// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Lightweight header that provides utl::format_as — a bridge between
// types with operator<< and the fmt/spdlog formatting library.
//
// Use this instead of Logger.h when a header only needs format_as
// and does not use the Logger class itself.

#pragma once

#include <ostream>
#include <type_traits>

#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/ostr.h"

// Define format_as for fmt > v10
#if !SWIG && FMT_VERSION >= 100000

namespace utl {

struct test_ostream
{
 public:
  template <class T>
  static auto test(int) -> decltype(std::declval<std::ostream>()
                                        << std::declval<T>(),
                                    std::true_type());

  template <class>
  static auto test(...) -> std::false_type;
};

template <class T,
          class = std::enable_if_t<decltype(test_ostream::test<T>(0))::value>>
auto format_as(const T& t)
{
  return fmt::streamed(t);
}

}  // namespace utl

#else
namespace utl {

// Uncallable class
template <class T>
class format_as
{
};

}  // namespace utl
#endif
