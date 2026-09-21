// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for slang-elab
//

#include "ir.h"

#include <cstdint>

namespace ir {

Value Net::repeat(uint64_t width)
{
  return Value(*this).repeat(width);
}

}  // namespace ir
