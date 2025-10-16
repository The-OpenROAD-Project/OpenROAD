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

#include <cstddef>
#include <functional>

namespace utl {

namespace detail {

/// \brief Combine a hash value with nothing.
/// It's the boundary condition of this serial functions.
///
/// \param seed Not effect.
inline void hash_combine(std::size_t*)
{
}

/// \brief Combine the given hash value with another obeject.
///
/// \param seed The given hash value. It's a input/output parameter.
/// \param value The object that will be combined with the given hash value.
template <typename T>
inline void hash_combine(std::size_t* seed, const T& value)
{
  *seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (*seed << 6) + (*seed >> 2);
}

/// \brief Combine the given hash value with another objects. It's a recursion。
///
/// \param seed The give hash value. It's a input/output parameter.
/// \param value The object that will be combined with the given hash value.
/// \param args The objects that will be combined with the given hash value.
template <typename T, typename... Types>
inline void hash_combine(std::size_t* seed,
                         const T& value,
                         const Types&... args)
{
  hash_combine(seed, value);
  hash_combine(seed, args...);
}

/// \brief Compute a hash value of the given args.
///
/// \param args The arguments that will be computed hash value.
/// \return The hash value of the given args.
template <typename... Types>
inline std::size_t hash_value(const Types&... args)
{
  std::size_t seed = 0;
  hash_combine(&seed, args...);
  return seed;
}

}  // namespace detail

}  // namespace utl
