// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "utl/Logger.h"

namespace rcx {

struct InterChipModel
{
  double resistance{0.0};  // In Ohms.
};

InterChipModel parseInterChipRules(const std::string& rules_file_path,
                                   utl::Logger* logger);

}  // namespace rcx
