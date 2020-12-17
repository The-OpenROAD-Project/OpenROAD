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

#include "definPinProps.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "db.h"

namespace odb {

definPinProps::definPinProps()
{
}

definPinProps::~definPinProps()
{
}

void definPinProps::begin(const char* instance, const char* terminal)
{
  _cur_obj = NULL;

  if (strcmp(instance, "PIN") == 0) {
    _cur_obj = _block->findBTerm(terminal);
  } else {
    dbInst* inst = _block->findInst(instance);

    if (inst)
      _cur_obj = inst->findITerm(terminal);
  }
}

void definPinProps::property(const char* name, const char* value)
{
  if (_cur_obj == NULL)
    return;

  dbProperty* p = dbProperty::find(_cur_obj, name);

  if (p)
    dbProperty::destroy(p);

  dbStringProperty::create(_cur_obj, name, value);
}

void definPinProps::property(const char* name, int value)
{
  if (_cur_obj == NULL)
    return;

  dbProperty* p = dbProperty::find(_cur_obj, name);

  if (p)
    dbProperty::destroy(p);

  dbIntProperty::create(_cur_obj, name, value);
}

void definPinProps::property(const char* name, double value)
{
  if (_cur_obj == NULL)
    return;

  dbProperty* p = dbProperty::find(_cur_obj, name);

  if (p)
    dbProperty::destroy(p);

  dbDoubleProperty::create(_cur_obj, name, value);
}

void definPinProps::end()
{
  if (_cur_obj) {
    dbSet<dbProperty> props = dbProperty::getProperties(_cur_obj);

    if (!props.empty() && props.orderReversed())
      props.reverse();
  }
}

}  // namespace odb
