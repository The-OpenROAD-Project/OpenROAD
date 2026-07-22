// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definRegion.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "definTypes.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace odb {

void definRegion::begin(const char* name)
{
  _cur_region = _block->findRegion(name);
  std::string region_name(name);

  if (_cur_region) {
    _logger->warn(utl::ODB, 152, "Region \"{}\" already exists", name);
    ++_errors;
    _cur_region = nullptr;
    return;
  }

  _cur_region = dbRegion::create(_block, region_name.c_str());
}

void definRegion::boundary(int x1, int y1, int x2, int y2)
{
  if (_cur_region) {
    dbBox::create(_cur_region, dbdist(x1), dbdist(y1), dbdist(x2), dbdist(y2));
  }
}

void definRegion::type(defRegionType type)
{
  if (_cur_region) {
    if (type == DEF_GUIDE) {
      _cur_region->setRegionType(dbRegionType(dbRegionType::SUGGESTED));

    } else if (type == DEF_FENCE) {
      _cur_region->setRegionType(dbRegionType(dbRegionType::EXCLUSIVE));
    }
  }
}

void definRegion::property(const char* name, const char* value)
{
  if (_cur_region == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_region, name);
  if (p) {
    dbProperty::destroy(p);
  }

  dbStringProperty::create(_cur_region, name, value);
}

void definRegion::property(const char* name, int value)
{
  if (_cur_region == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_region, name);
  if (p) {
    dbProperty::destroy(p);
  }

  dbIntProperty::create(_cur_region, name, value);
}

void definRegion::property(const char* name, double value)
{
  if (_cur_region == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_region, name);

  if (p) {
    dbProperty::destroy(p);
  }

  dbDoubleProperty::create(_cur_region, name, value);
}

void definRegion::end()
{
  if (_cur_region) {
    dbSet<dbBox> boxes = _cur_region->getBoundaries();

    if (boxes.reversible() && boxes.orderReversed()) {
      boxes.reverse();
    }

    dbSet<dbProperty> props = dbProperty::getProperties(_cur_region);

    if (!props.empty() && props.orderReversed()) {
      props.reverse();
    }
  }
}

}  // namespace odb
