// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "Objects.h"

namespace dpl {

const char* Cell::name() const
{
  return db_inst_->getConstName();
}

int64_t Cell::area() const
{
  dbMaster* master = db_inst_->getMaster();
  return int64_t(master->getWidth()) * master->getHeight();
}

DbuX Cell::siteWidth() const
{
  if (db_inst_) {
    auto site = db_inst_->getMaster()->getSite();
    if (site) {
      return DbuX{site->getWidth()};
    }
  }
  return DbuX{0};
}

bool Cell::isFixed() const
{
  return !db_inst_ || db_inst_->isFixed();
}

bool Cell::isHybrid() const
{
  dbSite* site = getSite();
  return site ? site->isHybrid() : false;
}

bool Cell::isHybridParent() const
{
  dbSite* site = getSite();
  return site ? site->hasRowPattern() : false;
}

dbSite* Cell::getSite() const
{
  if (!db_inst_ || !db_inst_->getMaster()) {
    return nullptr;
  }
  return db_inst_->getMaster()->getSite();
}

bool Cell::isStdCell() const
{
  if (db_inst_ == nullptr) {
    return false;
  }
  dbMasterType type = db_inst_->getMaster()->getType();
  // Use switch so if new types are added we get a compiler warning.
  switch (type.getValue()) {
    case dbMasterType::CORE:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_SPACER:
    case dbMasterType::CORE_WELLTAP:
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
    case dbMasterType::ENDCAP_LEF58_BOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_TOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER:
      return true;
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
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

bool Cell::isBlock() const
{
  return db_inst_ && db_inst_->getMaster()->getType() == dbMasterType::BLOCK;
}

}  // namespace dpl
