///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
// All rights reserved.
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
#pragma once

#include <utility>

namespace dft {

enum class EitherSide
{
  Left,
  Right
};

template <typename L>
class Left
{
 public:
  explicit Left(L left) : left_(left) {}
  Left(Left&& other) noexcept : left_(std::move(other.left_)) {}
  Left(const Left&) = delete;  // no copy
  L left_;
};

template <typename R>
class Right
{
 public:
  explicit Right(R right) : right_(right) {}
  Right(Right&& other) noexcept : right_(std::move(other.right_)) {}
  Right(const Right&) = delete;  // no copy
  R right_;
};

// Like Haskell's Either. We can represent two values but only one at the time.
// Access each one of the values with left or right, but first check what side
// is available with getSide:
//
// Example
//
// auto intEither = Either<int, double>(Left(1));
// auto doubleEither = Either<int, double>(Right(1.5));
//
// switch (intEither) {
//    case EitherSide::Left:
//      // do something with the int
//      left();
//    case EitherSide::Right:
//      // do something with the double
//      right();
// }
//
// Since we use a union, you can't use C++ classes directly, only native types
// and pointers
template <typename L, typename R>
class Either
{
 public:
  explicit Either(Left<L>&& left) noexcept
      : left_(std::move(left.left_)), side_(EitherSide::Left)
  {
  }
  explicit Either(Right<R>&& right) noexcept
      : right_(std::move(right.right_)), side_(EitherSide::Right)
  {
  }
  Either(const Either&) = delete;  // no copy

  // Move
  Either(Either<L, R>&& other) noexcept
  {
    switch (other.getSide()) {
      case EitherSide::Left:
        left_ = std::move(other.left_);
        break;
      case EitherSide::Right:
        right_ = std::move(other.right_);
        break;
    }
  }

  EitherSide getSide() const { return side_; }

  const L& left() const { return left_; }

  const R& right() const { return right_; }

 private:
  union
  {
    R right_;
    L left_;
  };

  EitherSide side_;
};

}  // namespace dft
