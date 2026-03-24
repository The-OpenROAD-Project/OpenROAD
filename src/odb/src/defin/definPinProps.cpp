// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definPinProps.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "odb/db.h"
#include "odb/dbSet.h"

namespace odb {

definPinProps::definPinProps() : _cur_obj(nullptr)
{
}

void definPinProps::begin(const char* instance, const char* terminal)
{
  _cur_obj = nullptr;

  if (strcmp(instance, "PIN") == 0) {
    _cur_obj = _block->findBTerm(terminal);
  } else {
    dbInst* inst = _block->findInst(instance);

    if (inst) {
      _cur_obj = inst->findITerm(terminal);
    }
  }
}

void definPinProps::property(const char* name, const char* value)
{
  if (_cur_obj == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_obj, name);

  if (p) {
    dbProperty::destroy(p);
  }

  dbStringProperty::create(_cur_obj, name, value);
}

void definPinProps::property(const char* name, int value)
{
  if (_cur_obj == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_obj, name);

  if (p) {
    dbProperty::destroy(p);
  }

  dbIntProperty::create(_cur_obj, name, value);
}

void definPinProps::property(const char* name, double value)
{
  if (_cur_obj == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_obj, name);

  if (p) {
    dbProperty::destroy(p);
  }

  dbDoubleProperty::create(_cur_obj, name, value);
}

void definPinProps::end()
{
  if (_cur_obj) {
    dbSet<dbProperty> props = dbProperty::getProperties(_cur_obj);

    if (!props.empty() && props.orderReversed()) {
      props.reverse();
    }
  }
}

}  // namespace odb
