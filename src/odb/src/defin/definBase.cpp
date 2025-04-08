// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definBase.h"

#include "defiUtil.hpp"
#include "odb/db.h"

namespace odb {

definBase::definBase()
{
  _mode = defin::DEFAULT;
  _tech = nullptr;
  _block = nullptr;
  _logger = nullptr;
  _errors = 0;
  _dist_factor = 10;
}

void definBase::init()
{
  _mode = defin::DEFAULT;
  _tech = nullptr;
  _block = nullptr;
  _logger = nullptr;
  _errors = 0;
  _dist_factor = 10;
}

void definBase::units(int d)
{
  int dbu = _tech->getDbUnitsPerMicron();
  _dist_factor = dbu / d;
}

void definBase::setTech(dbTech* tech)
{
  _tech = tech;
  int dbu = _tech->getDbUnitsPerMicron();
  _dist_factor = dbu / 100;
}

void definBase::setBlock(dbBlock* block)
{
  _block = block;
}

void definBase::setLogger(utl::Logger* logger)
{
  _logger = logger;
}

void definBase::setMode(defin::MODE mode)
{
  _mode = mode;
}

dbOrientType definBase::translate_orientation(int orient)
{
  switch (orient) {
    case DEF_ORIENT_N:
      return dbOrientType::R0;
    case DEF_ORIENT_S:
      return dbOrientType::R180;
    case DEF_ORIENT_E:
      return dbOrientType::R270;
    case DEF_ORIENT_W:
      return dbOrientType::R90;
    case DEF_ORIENT_FN:
      return dbOrientType::MY;
    case DEF_ORIENT_FS:
      return dbOrientType::MX;
    case DEF_ORIENT_FE:
      return dbOrientType::MYR90;
    case DEF_ORIENT_FW:
      return dbOrientType::MXR90;
  }
  assert(0);
  return dbOrientType::R0;
}

}  // namespace odb
