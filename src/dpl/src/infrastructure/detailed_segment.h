// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <limits>

#include "Coordinates.h"
#include "dpl/Opendp.h"
namespace dpl {
class DetailedSeg
{
 public:
  void setSegId(int segId) { segId_ = segId; }
  int getSegId() const { return segId_; }

  void setRowId(int rowId) { rowId_ = rowId; }
  int getRowId() const { return rowId_; }

  void setRegId(int regId) { regId_ = regId; }
  int getRegId() const { return regId_; }

  void setMinX(DbuX xmin) { xmin_ = xmin; }
  DbuX getMinX() const { return xmin_; }

  void setMaxX(DbuX xmax) { xmax_ = xmax; }
  DbuX getMaxX() const { return xmax_; }

  DbuX getWidth() const { return xmax_ - xmin_; }

  void setUtil(int util) { util_ = util; }
  int getUtil() const { return util_; }
  void addUtil(int amt) { util_ += amt; }
  void remUtil(int amt) { util_ -= amt; }

 private:
  // Some identifiers...
  int segId_ = -1;
  int rowId_ = -1;
  int regId_ = -1;
  // Span of segment...
  DbuX xmin_ = std::numeric_limits<DbuX>::max();
  DbuX xmax_ = std::numeric_limits<DbuX>::lowest();
  // Total width of cells in segment...
  int util_ = 0;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

}  // namespace dpl
