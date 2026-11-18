// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definPropDefs.h"

#include <cassert>

#include "definTypes.h"
#include "odb/db.h"
#include "odb/dbSet.h"

namespace odb {

void definPropDefs::beginDefinitions()
{
  _defs = nullptr;
}

void definPropDefs::endDefinitions()
{
  if (_defs == nullptr) {
    return;
  }

  dbSet<dbProperty> objects = dbProperty::getProperties(_defs);

  if (objects.orderReversed()) {
    objects.reverse();
  }

  dbSet<dbProperty>::iterator itr;

  for (itr = objects.begin(); itr != objects.end(); ++itr) {
    dbSet<dbProperty> props = dbProperty::getProperties(*itr);

    if (props.orderReversed()) {
      props.reverse();
    }
  }
}

void definPropDefs::begin(const char* obj_type,
                          const char* name,
                          defPropType prop_type)
{
  if (_defs == nullptr) {
    _defs = dbProperty::find(_block, "__ADS_DEF_PROPERTY_DEFINITIONS__");

    if (_defs == nullptr) {
      _defs = dbIntProperty::create(
          _block, "__ADS_DEF_PROPERTY_DEFINITIONS__", 0);
    }
  }

  dbProperty* obj = dbProperty::find(_defs, obj_type);

  if (obj == nullptr) {
    obj = dbIntProperty::create(_defs, obj_type, 0);
  }

  _prop = dbProperty::find(obj, name);

  if (_prop) {
    dbProperty::destroy(_prop);
  }

  switch (prop_type) {
    case DEF_INTEGER:
      _prop = dbIntProperty::create(obj, name, 0);
      break;

    case DEF_REAL:
      _prop = dbDoubleProperty::create(obj, name, 0.0);
      break;

    case DEF_STRING:
      _prop = dbStringProperty::create(obj, name, "");
      break;

    default:
      assert(0);
  }
}

void definPropDefs::value(const char* value)
{
  dbStringProperty::create(_prop, "VALUE", value);
}

void definPropDefs::value(int value)
{
  dbIntProperty::create(_prop, "VALUE", value);
}

void definPropDefs::value(double value)
{
  dbDoubleProperty::create(_prop, "VALUE", value);
}

void definPropDefs::range(int minV, int maxV)
{
  dbIntProperty::create(_prop, "MIN", minV);
  dbIntProperty::create(_prop, "MAX", maxV);
}

void definPropDefs::range(double minV, double maxV)
{
  dbDoubleProperty::create(_prop, "MIN", minV);
  dbDoubleProperty::create(_prop, "MAX", maxV);
}

void definPropDefs::end()
{
}

}  // namespace odb
