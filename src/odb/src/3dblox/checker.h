// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include "odb/db.h"

namespace odb {
class dbChip;

// Facade for backward compatibility and ease of use
class Checker
{
 public:
  Checker(utl::Logger* logger);
  ~Checker() = default;
  void check(odb::dbChip* chip);

 private:
  utl::Logger* logger_;
};

}  // namespace odb
