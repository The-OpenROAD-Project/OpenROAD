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

#include "definNonDefaultRule.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "db.h"

namespace odb {

definNonDefaultRule::definNonDefaultRule()
    : _cur_rule(NULL), _cur_layer_rule(NULL)
{
  init();
}

definNonDefaultRule::~definNonDefaultRule()
{
}

void definNonDefaultRule::init()
{
  definBase::init();
}

void definNonDefaultRule::beginRule(const char* name)
{
  _cur_layer_rule = NULL;
  _cur_rule       = dbTechNonDefaultRule::create(_block, name);

  if (_cur_rule == NULL) {
    notice(0, "error: Duplicate NONDEFAULTRULE %s\n", name);
    ++_errors;
  }
}

void definNonDefaultRule::hardSpacing()
{
  if (_cur_rule == NULL)
    return;

  _cur_rule->setHardSpacing(true);
}

void definNonDefaultRule::via(const char* name)
{
  if (_cur_rule == NULL)
    return;

  dbTechVia* via = _tech->findVia(name);

  if (via == NULL) {
    notice(0, "error: Cannot find tech-via %s\n", name);
    ++_errors;
    return;
  }

  _cur_rule->addUseVia(via);
}

void definNonDefaultRule::viaRule(const char* name)
{
  if (_cur_rule == NULL)
    return;

  dbTechViaGenerateRule* rule = _tech->findViaGenerateRule(name);

  if (rule == NULL) {
    notice(0, "error: Cannot find tech-via-genreate rule %s\n", name);
    ++_errors;
    return;
  }

  _cur_rule->addUseViaRule(rule);
}

void definNonDefaultRule::minCuts(const char* name, int count)
{
  if (_cur_rule == NULL)
    return;

  dbTechLayer* layer = _tech->findLayer(name);

  if (layer == NULL) {
    notice(0, "error: Cannot find layer %s\n", name);
    ++_errors;
    return;
  }

  _cur_rule->setMinCuts(layer, count);
}

void definNonDefaultRule::beginLayerRule(const char* name, int width)
{
  if (_cur_rule == NULL)
    return;

  dbTechLayer* layer = _tech->findLayer(name);

  if (layer == NULL) {
    notice(0, "error: Cannot find layer %s\n", name);
    ++_errors;
    return;
  }

  _cur_layer_rule = dbTechLayerRule::create(_cur_rule, layer);

  if (_cur_layer_rule == NULL) {
    notice(0,
           "error: Duplicate layer rule (%s) in non-default-rule statement.\n",
           name);
    ++_errors;
    return;
  }

  _cur_layer_rule->setWidth(dbdist(width));
}

void definNonDefaultRule::spacing(int s)
{
  if (_cur_layer_rule == NULL)
    return;

  _cur_layer_rule->setSpacing(dbdist(s));
}

void definNonDefaultRule::wireExt(int e)
{
  if (_cur_layer_rule == NULL)
    return;

  _cur_layer_rule->setWireExtension(dbdist(e));
}

void definNonDefaultRule::endLayerRule()
{
}

void definNonDefaultRule::property(const char* name, const char* value)
{
  if (_cur_rule == NULL)
    return;

  dbProperty* p = dbProperty::find(_cur_rule, name);
  if (p)
    dbProperty::destroy(p);

  dbStringProperty::create(_cur_rule, name, value);
}

void definNonDefaultRule::property(const char* name, int value)
{
  if (_cur_rule == NULL)
    return;

  dbProperty* p = dbProperty::find(_cur_rule, name);
  if (p)
    dbProperty::destroy(p);

  dbIntProperty::create(_cur_rule, name, value);
}

void definNonDefaultRule::property(const char* name, double value)
{
  if (_cur_rule == NULL)
    return;

  dbProperty* p = dbProperty::find(_cur_rule, name);

  if (p)
    dbProperty::destroy(p);

  dbDoubleProperty::create(_cur_rule, name, value);
}

void definNonDefaultRule::endRule()
{
  if (_cur_rule) {
    dbSet<dbProperty> props = dbProperty::getProperties(_cur_rule);

    if (!props.empty() && props.orderReversed())
      props.reverse();
  }
}

}  // namespace odb
