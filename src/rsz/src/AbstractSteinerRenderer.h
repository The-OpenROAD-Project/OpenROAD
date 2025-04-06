// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

namespace rsz {

class SteinerTree;

class AbstractSteinerRenderer
{
 public:
  virtual ~AbstractSteinerRenderer() = default;
  virtual void highlight(SteinerTree* tree) = 0;
};

}  // namespace rsz
