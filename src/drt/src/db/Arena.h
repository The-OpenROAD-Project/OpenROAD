/*
 * Copyright (c) 2025, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include <vector>

template <typename T>
class TypedArena
{
 public:
  static const auto BLOCK_SIZE = (std::size_t) 64 * 1024;

  TypedArena() = default;
  TypedArena(const TypedArena&) = delete;

  template <typename... Args>
  T* make(Args&&... args)
  {
    if (data_.empty() || data_.back().size() == data_.back().capacity()) {
      data_.push_back({});
      data_.back().reserve(BLOCK_SIZE / sizeof(T));
    }
    data_.back().emplace_back(std::forward<Args>(args)...);
    return &data_.back().back();
  }

 private:
  std::vector<std::vector<T>> data_;
};

template <typename... Ts>
class Arena
{
};

template <typename T, typename... Ts>
class Arena<T, Ts...> : public TypedArena<T>, public Arena<Ts...>
{
 public:
  Arena() = default;
  Arena(const Arena&) = delete;

  template <typename U, typename... Args>
  U* make(Args&&... args)
  {
    return TypedArena<U>::make(args...);
  }
};

template <>
class Arena<>
{
};
