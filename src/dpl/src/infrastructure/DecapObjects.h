// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include "Coordinates.h"
#include "dpl/Opendp.h"
#include "odb/db.h"

namespace dpl {

struct GapInfo
{
  DbuX x;
  DbuX width;
  DbuY height;
  bool is_filled{false};
  GapInfo(const DbuX& x, const DbuX& width, const DbuY& height)
      : x(x), width(width), height(height)
  {
  }
};

struct DecapCell
{
  odb::dbMaster* master;
  double capacitance;
  DecapCell(odb::dbMaster* master, double& capacitance)
      : master(master), capacitance(capacitance)
  {
  }
};

struct IRDrop
{
  DbuPt position;
  double value;
  IRDrop(const DbuPt& position, double& value)
      : position(position), value(value)
  {
  }
};

}  // namespace dpl
