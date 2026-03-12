// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "definBase.h"
#include "definTypes.h"

namespace odb {

class definGCell : public definBase
{
 public:
  virtual void gcell(defDirection dir, int orig, int count, int step);
};

}  // namespace odb
