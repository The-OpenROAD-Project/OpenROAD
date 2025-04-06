// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "sta/PathRef.hh"

namespace sta {

class AbstractPathRenderer
{
 public:
  virtual ~AbstractPathRenderer() = default;

  virtual void highlight(PathRef* path) = 0;
};

}  // namespace sta
