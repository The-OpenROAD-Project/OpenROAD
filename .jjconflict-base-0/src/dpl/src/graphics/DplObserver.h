// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include "dpl/Opendp.h"
#include "odb/db.h"

namespace dpl {

class Opendp;
class Node;

class DplObserver
{
 public:
  virtual ~DplObserver() = default;

  virtual void startPlacement(odb::dbBlock* block) = 0;
  virtual void placeInstance(odb::dbInst* instance) = 0;
  virtual void binSearch(const Node* cell,
                         GridX xl,
                         GridY yl,
                         GridX xh,
                         GridY yh)
      = 0;
  virtual void endPlacement() = 0;
};

}  // namespace dpl
