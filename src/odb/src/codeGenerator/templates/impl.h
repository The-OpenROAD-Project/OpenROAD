{% import 'macros' as macros %}
///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

//Generator Code Begin Header
#pragma once

#include "odb/odb.h"
#include "dbCore.h"

{% for include in klass.h_includes %}
  #include "{{include}}"
{% endfor %}
// User Code Begin Includes
// User Code End Includes

namespace odb {
  class dbIStream;
  class dbOStream;
  class _dbDatabase;
  {% for _class in klass.declared_classes %}
    {% if _class in ["dbTable", "dbHashTable"] %}
      template <class T>
    {% endif %}
    class {{ _class }};
  {% endfor %}
  //User Code Begin Classes
  //User Code End Classes

  {{ macros.make_structs(klass, is_public=False) }}
  // User Code Begin Structs
  // User Code End Structs

  class _{{klass.name}} : public _{{klass.type if klass.type else "dbObject"}}
  {
    public:
    {{ macros.make_enums(klass, is_public=False) }}
    // User Code Begin Enums
    // User Code End Enums
        
    _{{klass.name}}(_dbDatabase*);

    {% if klass.needs_non_default_destructor %}
      ~_{{klass.name}}();
    {% endif %}

    bool operator==(const _{{klass.name}}& rhs) const;
    bool operator!=(const _{{klass.name}}& rhs) const { return !operator==(rhs); }
    bool operator<(const _{{klass.name}}& rhs) const;
    {% if klass.hasTables %}
    dbObjectTable* getObjectTable(dbObjectType type);
    {% endif %}
    void collectMemInfo(MemInfo& info);
    // User Code Begin Methods
    // User Code End Methods

    {% for field in klass.fields %}
      {% if "comment" in field %}
        {{field.comment}}
      {% endif %}
      {% if field.table %}
        dbTable<_{{field.type}}>* {{field.name}};
      {% else %}
        {{field.type}} {{field.name}};
      {% endif %}
    {% endfor %}

    // User Code Begin Fields
    // User Code End Fields
  };
dbIStream& operator>>(dbIStream& stream, _{{klass.name}}& obj);
dbOStream& operator<<(dbOStream& stream, const _{{klass.name}}& obj);
// User Code Begin General
// User Code End General
} // namespace odb
//Generator Code End Header
