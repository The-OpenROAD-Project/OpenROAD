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

// Generator Code Begin Cpp
#include "{{itr.name}}.h"
#include "{{itr.parentObject}}.h"
#include "dbTable.h"
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
       id = {{itr.name}}::next(id))
    ++cnt;

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
