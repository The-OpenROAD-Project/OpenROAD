// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "{{itr.name}}.h"
#include "{{itr.parentObject}}.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"
{% for include in itr.includes %}
  #include "{{include}}"
{% endfor %}
// User Code Begin Includes
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// {{itr.name}} - Methods
//
////////////////////////////////////////////////////////////////////


bool {{itr.name}}::reversible() const
{
  return {{itr.reversible}};
}

bool {{itr.name}}::orderReversed() const
{
  return {{itr.orderReversed}};
}

void {{itr.name}}::reverse(dbObject* parent)
{
  //User Code Begin reverse
  //User Code End reverse
}

uint {{itr.name}}::sequential() const
{
  return {{itr.sequential}};
}

uint {{itr.name}}::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = {{itr.name}}::begin(parent); id != {{itr.name}}::end(parent);
       id = {{itr.name}}::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint {{itr.name}}::begin(dbObject* parent) const
{
  //User Code Begin begin
  //User Code End begin
}

uint {{itr.name}}::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint {{itr.name}}::next(uint id, ...) const
{
  //User Code Begin next
  //User Code End next
}

dbObject* {{itr.name}}::getObject(uint id, ...)
{
  return {{itr.tableName}}_->getPtr(id);
}
// User Code Begin Methods
// User Code End Methods
}  // namespace odb
// Generator Code End Cpp
