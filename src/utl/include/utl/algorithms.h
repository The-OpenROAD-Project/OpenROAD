// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <boost/generator_iterator.hpp>
#include <boost/random.hpp>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <tuple>

namespace utl {

// std::shuffle produces different results on different platforms.
// This custom shuffle function is used for consistency in different platforms.
template <class RandomIt, class URBG>
void shuffle(RandomIt first, RandomIt last, URBG&& g)
{
  int n = last - first;
  if (n == 0) {
    return;
  }

  boost::uniform_int<> distribution(1, n - 1);
  boost::variate_generator<URBG, boost::uniform_int<>> dice(g, distribution);

  for (int i = n - 1; i > 0; i--) {
    std::swap(first[i], first[dice(i + 1)]);
  }
}

// Lets you do a range-for like in python's enumerate with both
// the index and the iterable paired.
// From https://www.reedbeta.com/blog/python-like-enumerate-in-cpp17
template <typename T,
          typename TIter = decltype(std::begin(std::declval<T>())),
          typename = decltype(std::end(std::declval<T>()))>
constexpr auto enumerate(T&& iterable)
{
  struct iterator
  {
    size_t i;
    TIter iter;
    bool operator!=(const iterator& other) const { return iter != other.iter; }
    void operator++()
    {
      ++i;
      ++iter;
    }
    auto operator*() const { return std::tie(i, *iter); }
  };
  struct iterable_wrapper
  {
    T iterable;
    auto begin() { return iterator{0, std::begin(iterable)}; }
    auto end() { return iterator{0, std::end(iterable)}; }
  };
  return iterable_wrapper{std::forward<T>(iterable)};
}

// This format a number with at most the given precision.  It will
// remove trailing zeros (eg 10.20 -> 10.2 or 11.00 -> 11).
inline std::string to_numeric_string(const double number, const int precision)
{
  std::stringstream ss;
  ss << std::fixed << std::setprecision(precision) << number;

  auto str = ss.str();

  // remove trailing zeros
  str = str.substr(0, str.find_last_not_of('0') + 1);

  // Remove the decimal point if there is nothing after it anymore
  if (str.back() == '.') {
    str.pop_back();
  }

  return str;
}

}  // namespace utl
