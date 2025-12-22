{% import 'macros.jinja' as macros %}
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

//Generator Code Begin Header
#pragma once


{% for include in klass.h_sys_includes %}
  #include <{{include}}>
{% endfor %}

{% for include in klass.h_includes %}
  #include "{{include}}"
{% endfor %}
// User Code Begin Includes
// User Code End Includes

namespace odb {
  // User Code Begin Consts
  // User Code End Consts
  {% for _class in klass.declared_classes %}
    {% if _class in ["dbTable", "dbHashTable"] %}
      template <class T>
    {% endif %}
    class {{ _class }};
  {% endfor %}
  //User Code Begin Classes
  //User Code End Classes

  //User Code Begin Types
  //User Code End Types

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
      {{field.type}} {{field.name}};
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
