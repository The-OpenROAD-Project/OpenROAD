/////////////////////////////////////////////////////////////////////////////
// Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu
// Woo(mwoo@eng.ucsd.edu)
//          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
// Rewrite by James Cherry, Parallax Software, Inc.
//
// Copyright (c) 2019, The Regents of the University of California
// Copyright (c) 2018, SangGi Do and Mingyu Woo
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <boost/operators.hpp>
#include <cmath>
#include <functional>
#include <ostream>

namespace dpl {

using std::abs;

// Strongly type the difference between pixel and DBU locations.
//
// Multiplication and division are intentionally not defined as they
// are often, but not always, used to convert between DBUs and pixels
// and such operations must be explicit about the resulting type.

template <typename T>
struct TypedCoordinate : public boost::totally_ordered<TypedCoordinate<T>>,
                         public boost::totally_ordered<TypedCoordinate<T>, int>,
                         public boost::additive<TypedCoordinate<T>>,
                         public boost::additive<TypedCoordinate<T>, int>,
                         public boost::modable<TypedCoordinate<T>>,
                         public boost::modable<TypedCoordinate<T>, int>,
                         public boost::incrementable<TypedCoordinate<T>>,
                         public boost::decrementable<TypedCoordinate<T>>
{
  explicit TypedCoordinate(const int v = 0) : v(v) {}
  TypedCoordinate(const TypedCoordinate<T>& v) = default;

  // totally_ordered
  bool operator<(const TypedCoordinate& rhs) const { return v < rhs.v; }
  bool operator<(const int rhs) const { return v < rhs; }
  bool operator>(const int rhs) const { return v > rhs; }
  bool operator==(const TypedCoordinate& rhs) const { return v == rhs.v; }
  bool operator==(const int rhs) const { return v == rhs; }
  // additive
  TypedCoordinate& operator+=(const TypedCoordinate& rhs)
  {
    v += rhs.v;
    return *this;
  }
  TypedCoordinate& operator+=(const int rhs)
  {
    v += rhs;
    return *this;
  }
  TypedCoordinate& operator-=(const TypedCoordinate& rhs)
  {
    v -= rhs.v;
    return *this;
  }
  TypedCoordinate& operator-=(const int rhs)
  {
    v -= rhs;
    return *this;
  }
  // modable
  TypedCoordinate& operator%=(const TypedCoordinate& rhs)
  {
    v %= rhs.v;
    return *this;
  }

  // incrementable
  TypedCoordinate& operator++()
  {
    ++v;
    return *this;
  }

  // decrementable
  TypedCoordinate& operator--()
  {
    --v;
    return *this;
  }

  int v;
};

template <typename T>
inline std::ostream& operator<<(std::ostream& o, const TypedCoordinate<T>& tc)
{
  return o << tc.v;
}

template <typename T>
TypedCoordinate<T> abs(const TypedCoordinate<T>& val)
{
  return TypedCoordinate<T>{std::abs(val.v)};
}

}  // namespace dpl

// Enable use with unordered_map/set
namespace std {

template <typename T>
struct hash<dpl::TypedCoordinate<T>>
{
  std::size_t operator()(const dpl::TypedCoordinate<T>& tc) const noexcept
  {
    return std::hash<int>()(tc.v);
  }
};

}  // namespace std
