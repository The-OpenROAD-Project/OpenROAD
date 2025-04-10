// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <limits>

namespace dpo {

class DetailedSeg
{
 public:
  void setSegId(int segId) { segId_ = segId; }
  int getSegId() const { return segId_; }

  void setRowId(int rowId) { rowId_ = rowId; }
  int getRowId() const { return rowId_; }

  void setRegId(int regId) { regId_ = regId; }
  int getRegId() const { return regId_; }

  void setMinX(int xmin) { xmin_ = xmin; }
  int getMinX() const { return xmin_; }

  void setMaxX(int xmax) { xmax_ = xmax; }
  int getMaxX() const { return xmax_; }

  int getWidth() const { return xmax_ - xmin_; }

  void setUtil(int util) { util_ = util; }
  int getUtil() const { return util_; }
  void addUtil(int amt) { util_ += amt; }
  void remUtil(int amt) { util_ -= amt; }

 private:
  // Some identifiers...
  int segId_ = -1;
  int rowId_ = -1;
  int regId_ = 0;
  // Span of segment...
  int xmin_ = std::numeric_limits<int>::max();
  int xmax_ = std::numeric_limits<int>::lowest();
  // Total width of cells in segment...
  int util_ = 0;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

}  // namespace dpo
