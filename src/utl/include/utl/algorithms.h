// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "boost/generator_iterator.hpp"
#include "boost/random.hpp"

namespace utl {

// std::shuffle produces different results on different platforms.
// This custom shuffle function is used for consistency in different platforms.
template <class RandomIt, class URBG>
void shuffle(RandomIt first, RandomIt last, URBG&& g)
{
  int n = last - first;
  if (n <= 1) {
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

template <typename T>
constexpr T snapDown(const T value, const T step, const T origin = T{0})
{
  if constexpr (std::is_integral_v<T>) {
    return ((value - origin) / step) * step + origin;
  }
  return std::floor((value - origin) / step) * step + origin;
}

template <typename T>
constexpr T snapUp(const T value, const T step, const T origin = T{0})
{
  if constexpr (std::is_integral_v<T>) {
    return snapDown(value + step - 1, step, origin);
  }
  return std::ceil((value - origin) / step) * step + origin;
}

/**
 * @brief Sorts a container and removes duplicate elements.
 * @param c The container to modify.
 * @param comp The comparator (e.g., std::greater<>{}).
 * @param proj A projection (e.g., &MyStruct::id).
 */
template <typename Container,
          typename Comp = std::ranges::less,
          typename Proj = std::identity>
void sort_and_unique(Container& c, Comp comp = {}, Proj proj = {})
{
  std::ranges::sort(c, comp, proj);
  auto [first, last] = std::ranges::unique(c, std::ranges::equal_to{}, proj);
  c.erase(first, last);
}

// Useful when you want to sort with embedded numbers compared
// numerically.  For example FILL1 < FILL4 < FILL16.
inline bool natural_compare(std::string_view a, std::string_view b)
{
  auto it_a = a.begin();
  auto it_b = b.begin();

  while (it_a != a.end() && it_b != b.end()) {
    if (std::isdigit(*it_a) && std::isdigit(*it_b)) {
      // Both are digits: extract and compare as numbers
      uint64_t num_a = 0;
      auto res_a = std::from_chars(&*it_a, a.data() + a.size(), num_a);
      uint64_t num_b = 0;
      auto res_b = std::from_chars(&*it_b, b.data() + b.size(), num_b);

      if (num_a != num_b) {
        return num_a < num_b;
      }

      // Move iterators forward by the number of digits consumed
      it_a += (res_a.ptr - &*it_a);
      it_b += (res_b.ptr - &*it_b);
    } else {
      // At least one is text: compare characters
      if (*it_a != *it_b) {
        return *it_a < *it_b;
      }
      ++it_a;
      ++it_b;
    }
  }
  // If one string is a prefix of the other, the shorter one comes first
  return a.size() < b.size();
}

}  // namespace utl
