// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "{{itr.name}}.h"
#include "{{itr.parentObject}}.h"
#include "dbTable.h"
#include "dbTable.hpp"
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


bool {{itr.name}}::reversible()
{
  return {{itr.reversible}};
}

bool {{itr.name}}::orderReversed()
{
  return {{itr.orderReversed}};
}

void {{itr.name}}::reverse(dbObject* parent)
{
  //User Code Begin reverse
  //User Code End reverse
}

uint {{itr.name}}::sequential()
{
  return {{itr.sequential}};
}

uint {{itr.name}}::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = {{itr.name}}::begin(parent); id != {{itr.name}}::end(parent);
       id = {{itr.name}}::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint {{itr.name}}::begin(dbObject* parent)
{
  //User Code Begin begin
  //User Code End begin
}

uint {{itr.name}}::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint {{itr.name}}::next(uint id, ...)
{
  //User Code Begin next
  //User Code End next
}

dbObject* {{itr.name}}::getObject(uint id, ...)
{
  return _{{itr.tableName}}->getPtr(id);
}
// User Code Begin Methods
// User Code End Methods
}  // namespace odb
// Generator Code End Cpp
