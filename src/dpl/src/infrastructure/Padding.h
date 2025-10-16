// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <utility>

#include "Coordinates.h"
#include "dpl/Opendp.h"
#include "odb/db.h"

namespace dpl {
class Node;
class Padding
{
 public:
  GridX padGlobalLeft() const { return pad_left_; }
  GridX padGlobalRight() const { return pad_right_; }

  void setPaddingGlobal(GridX left, GridX right);
  void setPadding(odb::dbInst* inst, GridX left, GridX right);
  void setPadding(odb::dbMaster* master, GridX left, GridX right);
  bool havePadding() const;

  // Find instance/master/global padding value for an instance.
  GridX padLeft(odb::dbInst* inst) const;
  GridX padLeft(const Node* cell) const;
  GridX padRight(odb::dbInst* inst) const;
  GridX padRight(const Node* cell) const;
  bool isPaddedType(odb::dbInst* inst) const;
  DbuX paddedWidth(const Node* cell) const;

 private:
  using InstPaddingMap = std::map<odb::dbInst*, std::pair<GridX, GridX>>;
  using MasterPaddingMap = std::map<odb::dbMaster*, std::pair<GridX, GridX>>;

  GridX pad_left_{0};
  GridX pad_right_{0};
  InstPaddingMap inst_padding_map_;
  MasterPaddingMap master_padding_map_;
};

}  // namespace dpl
