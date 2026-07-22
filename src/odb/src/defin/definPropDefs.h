// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "definBase.h"
#include "definTypes.h"

namespace odb {

/**********************************************************
 *
 * DEF PROPERTY DEFINITIONS are stored as hierachical
 * properties using the following structure:
 *
 *                       [__ADS_DEF_PROPERTY_DEFINITIONS__]
 *                                    +
 *                                    |
 *                                    |
 *                           +--------+------+
 *                      [COMPONENT] [NET] [GROUP] ....
 *                           +        |      |
 *                           |       ...    ...
 *                           |
 *                    +------+
 *                 [NAME] (type encoded as property-type)
 *                    +
 *                    |
 *                +---+--+-----+
 *             [VALUE] [MIN] [MAX] (Optional properties)
 *
 **********************************************************/

class dbProperty;

class definPropDefs : public definBase
{
  dbProperty* _defs;
  dbProperty* _prop;

 public:
  virtual void beginDefinitions();
  virtual void begin(const char* obj_type,
                     const char* name,
                     defPropType prop_type);
  virtual void value(const char* value);
  virtual void value(int value);
  virtual void value(double value);
  virtual void range(int minV, int maxV);
  virtual void range(double minV, double maxV);
  virtual void end();
  virtual void endDefinitions();
};

}  // namespace odb
