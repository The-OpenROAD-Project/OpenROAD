// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

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
