// MIT License

// Copyright (c) 2021 biaks (ianiskr@gmail.com)

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <atomic>
#include <type_traits>

namespace utl {

template <typename FloatingType>
inline std::atomic<FloatingType>& atomic_add_for_floating_types(
    std::atomic<FloatingType>& value,
    const FloatingType& add)
{
  FloatingType desired;
  FloatingType expected = value.load(std::memory_order_relaxed);
  do {
    desired = expected + add;
  } while (!value.compare_exchange_weak(expected, desired));
  return value;
}

template <typename FloatingType,
          class = typename std::
              enable_if<std::is_floating_point<FloatingType>::value, int>::type>
inline std::atomic<FloatingType>& operator++(std::atomic<FloatingType>& value)
{
  return atomic_add_for_floating_types(value, 1.0);
}

template <typename FloatingType,
          class = typename std::
              enable_if<std::is_floating_point<FloatingType>::value, int>::type>
inline std::atomic<FloatingType>& operator+=(std::atomic<FloatingType>& value,
                                             const FloatingType& val)
{
  return atomic_add_for_floating_types(value, val);
}

template <typename FloatingType,
          class = typename std::
              enable_if<std::is_floating_point<FloatingType>::value, int>::type>
inline std::atomic<FloatingType>& operator--(std::atomic<FloatingType>& value)
{
  return atomic_add_for_floating_types(value, -1.0);
}

template <typename FloatingType,
          class = typename std::
              enable_if<std::is_floating_point<FloatingType>::value, int>::type>
inline std::atomic<FloatingType>& operator-=(std::atomic<FloatingType>& value,
                                             const FloatingType& val)
{
  return atomic_add_for_floating_types(value, -val);
}

}  // namespace utl
