/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024, Precision Innovations Inc.
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "Padding.h"

#include "Objects.h"

namespace dpl {

void Padding::setPaddingGlobal(GridX left, GridX right)
{
  pad_left_ = left;
  pad_right_ = right;
}

void Padding::setPadding(dbInst* inst, GridX left, GridX right)
{
  inst_padding_map_[inst] = {left, right};
}

void Padding::setPadding(dbMaster* master, GridX left, GridX right)
{
  master_padding_map_[master] = {left, right};
}

bool Padding::havePadding() const
{
  return pad_left_ > 0 || pad_right_ > 0 || !master_padding_map_.empty()
         || !inst_padding_map_.empty();
}

bool Padding::isPaddedType(dbInst* inst) const
{
  dbMasterType type = inst->getMaster()->getType();
  // Use switch so if new types are added we get a compiler warning.
  switch (type.getValue()) {
    case dbMasterType::CORE:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_WELLTAP:
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
      return true;
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

GridX Padding::padLeft(const Cell* cell) const
{
  return padLeft(cell->db_inst_);
}

GridX Padding::padLeft(dbInst* inst) const
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

GridX Padding::padRight(const Cell* cell) const
{
  return padRight(cell->db_inst_);
}

GridX Padding::padRight(dbInst* inst) const
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

DbuX Padding::paddedWidth(const Cell* cell) const
{
  return cell->width_
         + gridToDbu(padLeft(cell) + padRight(cell), cell->siteWidth());
}

}  // namespace dpl
