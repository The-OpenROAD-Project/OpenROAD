// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include "Coordinates.h"
#include "Opendp.h"

namespace dpl {
class GridNode;
class Padding
{
 public:
  GridX padGlobalLeft() const { return pad_left_; }
  GridX padGlobalRight() const { return pad_right_; }

  void setPaddingGlobal(GridX left, GridX right);
  void setPadding(dbInst* inst, GridX left, GridX right);
  void setPadding(dbMaster* master, GridX left, GridX right);
  bool havePadding() const;

  // Find instance/master/global padding value for an instance.
  GridX padLeft(dbInst* inst) const;
  GridX padLeft(const GridNode* cell) const;
  GridX padRight(dbInst* inst) const;
  GridX padRight(const GridNode* cell) const;
  bool isPaddedType(dbInst* inst) const;
  DbuX paddedWidth(const GridNode* cell) const;

 private:
  using InstPaddingMap = map<dbInst*, pair<GridX, GridX>>;
  using MasterPaddingMap = map<dbMaster*, pair<GridX, GridX>>;

  GridX pad_left_{0};
  GridX pad_right_{0};
  InstPaddingMap inst_padding_map_;
  MasterPaddingMap master_padding_map_;
};

}  // namespace dpl
