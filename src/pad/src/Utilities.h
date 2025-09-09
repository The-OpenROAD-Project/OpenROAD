// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

namespace odb {
class dbNet;
}  // namespace odb

namespace pad {

class Utilities
{
 public:
  static void makeSpecial(odb::dbNet* net);
};

}  // namespace pad
