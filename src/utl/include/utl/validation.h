/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <type_traits>

#include "utl/Logger.h"

namespace utl {

class Validator
{
 public:
  enum EndType
  {
    OPEN,
    CLOSED
  };

  Validator(Logger* logger, ToolId tool) : logger_(logger), tool_(tool) {}

  // Check value is in [limit, +inf] (CLOSED) or (limit, +inf] (OPEN)
  template <typename T>
  void check_above(const char* name,
                   T value,
                   T limit,
                   int id,
                   EndType end = CLOSED)
  {
    static_assert(std::is_arithmetic_v<T>, "Must be arithmetic type");
    if (value < limit || (end == OPEN && value == limit)) {
      logger_->error(tool_,
                     id,
                     "{} {} not {} {}",
                     name,
                     value,
                     end == CLOSED ? ">=" : ">",
                     limit);
    }
  }

  // Check value is in [-inf, limit] (CLOSED) or [-inf, limit) (OPEN)
  template <typename T>
  void check_below(const char* name,
                   T value,
                   T limit,
                   int id,
                   EndType end = CLOSED)
  {
    static_assert(std::is_arithmetic_v<T>, "Must be arithmetic type");
    if (value > limit || (end == OPEN && value == limit)) {
      logger_->error(tool_,
                     id,
                     "{} {} not {} {}",
                     name,
                     value,
                     end == CLOSED ? "<=" : "<",
                     limit);
    }
  }

  // Check value is in:
  //   [low_limit, high_limit] (CLOSED, CLOSED) or
  //   [low_limit, high_limit) (CLOSED, OPEN) or
  //   (low_limit, high_limit] (OPEN, CLOSED) or
  //   (low_limit, high_limit) (OPEN, OPEN) or
  template <typename T>
  void check_range(const char* name,
                   T value,
                   T low_limit,
                   T high_limit,
                   int id,
                   EndType low_end = CLOSED,
                   EndType high_end = CLOSED)
  {
    static_assert(std::is_arithmetic_v<T>, "Must be arithmetic type");
    if (value < low_limit || (low_end == OPEN && value == low_limit)
        || value > high_limit || (high_end == OPEN && value == high_limit)) {
      logger_->error(tool_,
                     id,
                     "{} {} not in {}{}, {}{}",
                     name,
                     value,
                     low_end == CLOSED ? "[" : "(",
                     low_limit,
                     high_limit,
                     high_end == CLOSED ? "]" : ")");
    }
  }

  // Check value is in [0, +inf]
  template <typename T>
  void check_non_negative(const char* name, T value, int id)
  {
    check_above<T>(name, value, 0, id);
  }

  // Check value is in (0, +inf]
  template <typename T>
  void check_positive(const char* name, T value, int id)
  {
    check_above<T>(name, value, 0, id, OPEN);
  }

  // Check value is in [0, 100]
  template <typename T>
  void check_percentage(const char* name, T value, int id)
  {
    check_range<T>(name, value, 0, 100, id);
  }

  void check_non_null(const char* name, const void* ptr, int id)
  {
    if (ptr == nullptr) {
      logger_->error(tool_, id, "{} is not allowed to be null.", name);
    }
  }

 private:
  Logger* logger_;
  ToolId tool_;
};

}  // namespace utl
