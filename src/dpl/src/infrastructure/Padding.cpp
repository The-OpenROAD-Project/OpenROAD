// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "Padding.h"

#include "Objects.h"
#include "dpl/Opendp.h"
#include "infrastructure/Coordinates.h"
#include "odb/db.h"
#include "odb/dbTypes.h"

namespace dpl {

void Padding::setPaddingGlobal(GridX left, GridX right)
{
  pad_left_ = left;
  pad_right_ = right;
}

void Padding::setPadding(odb::dbInst* inst, GridX left, GridX right)
{
  inst_padding_map_[inst] = {left, right};
}

void Padding::setPadding(odb::dbMaster* master, GridX left, GridX right)
{
  master_padding_map_[master] = {left, right};
}

bool Padding::havePadding() const
{
  return pad_left_ > 0 || pad_right_ > 0 || !master_padding_map_.empty()
         || !inst_padding_map_.empty();
}

bool Padding::isPaddedType(odb::dbInst* inst) const
{
  if (inst == nullptr) {
    return false;
  }

  using odb::dbMasterType;
  dbMasterType type = inst->getMaster()->getType();
  // Use switch so if new types are added we get a compiler warning.
  switch (type.getValue()) {
    case dbMasterType::CORE:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_WELLTAP:
      return true;
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
    case dbMasterType::CORE_SPACER:
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
    case dbMasterType::ENDCAP_LEF58_BOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_TOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER:
      // These classes are completely ignored by the placer.
    case dbMasterType::COVER:
    case dbMasterType::COVER_BUMP:
    case dbMasterType::RING:
    case dbMasterType::PAD:
    case dbMasterType::PAD_AREAIO:
    case dbMasterType::PAD_INPUT:
    case dbMasterType::PAD_OUTPUT:
    case dbMasterType::PAD_INOUT:
    case dbMasterType::PAD_POWER:
    case dbMasterType::PAD_SPACER:
      return false;
  }
  // gcc warniing
  return false;
}

GridX Padding::padLeft(const Node* cell) const
{
  return padLeft(cell->getDbInst());
}

GridX Padding::padLeft(odb::dbInst* inst) const
{
  if (isPaddedType(inst)) {
    auto itr1 = inst_padding_map_.find(inst);
    if (itr1 != inst_padding_map_.end()) {
      return itr1->second.first;
    }
    auto itr2 = master_padding_map_.find(inst->getMaster());
    if (itr2 != master_padding_map_.end()) {
      return itr2->second.first;
    }
    return pad_left_;
  }
  return GridX{0};
}

GridX Padding::padRight(const Node* cell) const
{
  return padRight(cell->getDbInst());
}

GridX Padding::padRight(odb::dbInst* inst) const
{
  if (isPaddedType(inst)) {
    auto itr1 = inst_padding_map_.find(inst);
    if (itr1 != inst_padding_map_.end()) {
      return itr1->second.second;
    }
    auto itr2 = master_padding_map_.find(inst->getMaster());
    if (itr2 != master_padding_map_.end()) {
      return itr2->second.second;
    }
    return pad_right_;
  }
  return GridX{0};
}

DbuX Padding::paddedWidth(const Node* cell) const
{
  return cell->getWidth()
         + gridToDbu(padLeft(cell) + padRight(cell), cell->siteWidth());
}

}  // namespace dpl
