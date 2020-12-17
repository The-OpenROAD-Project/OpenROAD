///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
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

#include "db.h"
#include "definPropDefs.h"

namespace odb {

void definPropDefs::beginDefinitions()
{
  _defs = NULL;
}

void definPropDefs::endDefinitions()
{
  if (_defs == NULL)
    return;

  dbSet<dbProperty> objects = dbProperty::getProperties(_defs);

  if (objects.orderReversed())
    objects.reverse();

  dbSet<dbProperty>::iterator itr;

  for (itr = objects.begin(); itr != objects.end(); ++itr) {
    dbSet<dbProperty> props = dbProperty::getProperties(*itr);

    if (props.orderReversed())
      props.reverse();
  }
}

void definPropDefs::begin(const char* obj_type,
                          const char* name,
                          defPropType prop_type)
{
  if (_defs == NULL) {
    _defs = dbProperty::find(_block, "__ADS_DEF_PROPERTY_DEFINITIONS__");

    if (_defs == NULL)
      _defs = dbIntProperty::create(
          _block, "__ADS_DEF_PROPERTY_DEFINITIONS__", 0);
  }

  dbProperty* obj = dbProperty::find(_defs, obj_type);

  if (obj == NULL) {
      obj = dbIntProperty::create(_defs, obj_type, 0);
  }

  _prop = dbProperty::find(obj, name);

  if (_prop)
    dbProperty::destroy(_prop);

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
