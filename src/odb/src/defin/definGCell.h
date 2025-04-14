// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "definBase.h"
#include "odb/odb.h"

namespace odb {

class definGCell : public definBase
{
 public:
  /// GCell interface methods
  virtual void gcell(defDirection dir, int orig, int count, int step);

  definGCell();
  ~definGCell() override;
  void init() override;
};

}  // namespace odb
