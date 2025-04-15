// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "sta/Path.hh"

namespace sta {

class AbstractPathRenderer
{
 public:
  virtual ~AbstractPathRenderer() = default;

  virtual void highlight(Path* path) = 0;
};

}  // namespace sta
