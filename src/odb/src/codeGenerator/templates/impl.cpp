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

// Generator Code Begin Cpp
#include "{{klass.name}}.h"
#include "odb/db.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"

{% if klass.hasBitFields %}
  #include <cstring>
  #include <cstdint>
{% endif %}

{% for include in klass.cpp_includes %}
  #include "{{include}}"
{% endfor %}
// User Code Begin Includes
// User Code End Includes
namespace odb {
  template class dbTable<_{{klass.name}}>; 
    
  bool _{{klass.name}}::operator==(const _{{klass.name}}& rhs) const
  {
    {% for field in klass.fields %}
      {% if field.bitFields %}
        {% for innerField in klass.structs[0].fields %}
          {% for component in innerField.components %}
            {% if 'no-cmp' not in innerField.flags %}
              {% if innerField.table %}
                if ({{field.name}}->{{component}}!=rhs.{{field.name}}->{{component}}) {
              {% else %}
                if ({{field.name}}.{{component}}!=rhs.{{field.name}}.{{component}}) {
              {% endif %}
                  return false;
                }  
            {% endif %}
          {% endfor %}
        {% endfor %}
      {% else %}
        {% for component in field.components %}
          {% if 'no-cmp' not in field.flags %}
            {% if field.table %}
              if (*{{component}}!=*rhs.{{component}}) {
            {% else %}
              if ({{component}}!=rhs.{{component}}) {
            {% endif %}
                return false;
              }
          {% endif %}
        {% endfor %}
      {% endif %}
    {% endfor %}

    //User Code Begin ==
    //User Code End ==
    return true;
  }

  bool _{{klass.name}}::operator<(const _{{klass.name}}& rhs) const
  {
    {% for field in klass.fields %}
      {% if field.bitFields %}
        {% for innerField in klass.structs[0].fields %}
          {% for component in innerField.components %}
            {% if 'cmpgt' in innerField.flags %}
              if ({{field.name}}.{{component}}>=rhs.{{field.name}}.{{component}}) {
                return false;
              }
            {% endif %}
          {% endfor %}
        {% endfor %}
      {% else %}
        {% for component in field.components %}
          {% if 'cmpgt' in field.flags %}
            if ({{component}}>=rhs.{{component}}) {
              return false;
            }
          {% endif %}
        {% endfor %}
      {% endif %}
    {% endfor %}

    //User Code Begin <
    //User Code End <
    return true;
  }

  _{{klass.name}}::_{{klass.name}}(_dbDatabase* db)
  {
    {% for field in klass.fields %}
      {% if field.bitFields %}
        {{field.name}} = {};
      {% elif field.table %}
        {{field.name}} = new dbTable<_{{field.type}}>(db, this, (GetObjTbl_t) &_{{klass.name}}::getObjectTable, {{field.type}}Obj);
      {% elif field.isHashTable %}
        {{field.name}}.setTable({{field.table_name}});
      {% elif 'default' in field%}
        {{field.name}} = {{field.default}};
      {% endif %}
    {% endfor %}
    // User Code Begin Constructor
    // User Code End Constructor
  }

  {% import 'serializer_in.cpp' as si %}
  {% for _struct in klass.structs %}
    {% if 'flags' not in _struct
                    or ('no-serializer' not in _struct['flags']
                        and 'no-serializer-in' not in _struct['flags']) %}
      {{- si.serializer_in(_struct, klass, _struct.name)}}
    {% endif %}
  {% endfor %}
  {{- si.serializer_in(klass)}}

  {% import 'serializer_out.cpp' as so %}
  {% for _struct in klass.structs %}
    {% if 'flags' not in _struct
                    or ('no-serializer' not in _struct['flags']
                        and 'no-serializer-out' not in _struct['flags']) %}
      {{- so.serializer_out(_struct, klass, _struct.name)}}
    {% endif %}
  {% endfor %}
  {{- so.serializer_out(klass)}}

  {% if klass.hasTables %}
  dbObjectTable* _{{klass.name}}::getObjectTable(dbObjectType type)
  {
    switch (type) {
      {% for field in klass.fields %}
        {% if field.table %}
          case {{field.type}}Obj:
            return {{field.name}};
        {% endif %}
      {% endfor %}
      //User Code Begin getObjectTable
      //User Code End getObjectTable
      default:
        break;
    }
    return getTable()->getObjectTable(type);
  }
  {% endif %}
  void _{{klass.name}}::collectMemInfo(MemInfo& info)
  {
    info.cnt++;
    info.size += sizeof(*this);

    {% for field in klass.fields %}
      {% if field.table %} 
        {{field.name}}->collectMemInfo(info.children_["{{field.name}}"]);
      {% endif %}
    {% endfor %}

    //User Code Begin collectMemInfo
    //User Code End collectMemInfo
  }

  {% if klass.needs_non_default_destructor %}
    _{{klass.name}}::~_{{klass.name}}()
    {
      {% for field in klass.fields %}
        {% if field.name == '_name' and 'no-destruct' not in field.flags %}
          if (_name) {
            free((void*) _name);
          }
        {% endif %}
        {% if field.table %}
          delete {{field.name}};
        {% endif %}
      {% endfor %}
      //User Code Begin Destructor
      //User Code End Destructor
    }
  {% endif %}

  //User Code Begin PrivateMethods
  //User Code End PrivateMethods

  ////////////////////////////////////////////////////////////////////
  //
  // {{klass.name}} - Methods
  //
  ////////////////////////////////////////////////////////////////////
  {% for field in klass.fields %}
  
    {% if 'no-set' not in field.flags %}
      void {{klass.name}}::{{field.setterFunctionName}}( {% if field.isSetByRef %}const {{field.setterArgumentType}}&{% else %}{{field.setterArgumentType}}{% endif %} {{field.argument}} )
      {
    
        _{{klass.name}}* obj = (_{{klass.name}}*)this;
    
        {% if field.isRef %}
    
          obj->{{field.name}}={{field.argument}}->getImpl()->getOID();
    
        {% else %}

          obj->{{field.name}}={{field.argument}};
    
        {% endif %}
      }
    {% endif %}
  
    {% if 'no-get' not in field.flags %}
      {{- macros.getter_signature(field, klass) -}}
      {
      {% if field.dbSetGetter %}
          _{{klass.name}}* obj = (_{{klass.name}}*)this;
          return dbSet<{{field.type}}>(obj, obj->{{field.name}});
      {% elif field.isPassByRef %}
          _{{klass.name}}* obj = (_{{klass.name}}*)this;
          tbl = obj->{{field.name}};
      {% elif field.isHashTable %}
          _{{klass.name}}* obj = (_{{klass.name}}*)this;
          return ({{field.getterReturnType}}) obj->{{field.name}}.find(name);
      {% else %}
          _{{klass.name}}* obj = (_{{klass.name}}*)this;
          {% if field.isRef %}
            if(obj->{{field.name}} == 0) {
              return nullptr;
            }
            _{{field.parent}}* par = (_{{field.parent}}*) obj->getOwner();
            return ({{field.refType}}) par->{{field.refTable}}->getPtr(obj->{{field.name}});
          {% elif field.isHashTable %}
            return {{field.getterReturnType}} obj->{{field.name}}.find(name);
          {% else %}
            return obj->{{field.name}};
          {% endif %}
      {% endif %}
      }
    {% endif %}
  {% endfor %}

  {% for _struct in klass.structs %}
    {%  if  _struct.in_class %}
      {% for field in _struct.fields %}
      
      {% if 'no-set' not in field.flags %}
        void {{klass.name}}::{{field.setterFunctionName}}( {{field.setterArgumentType}} {{field.argument}} )
        {
      
          _{{klass.name}}* obj = (_{{klass.name}}*)this;
      
          obj->{{_struct.in_class_name}}.{{field.name}}={{field.argument}};
      
        }
      {% endif %}
    
      {% if 'no-get' not in field.flags %}
        {{field.getterReturnType}} {{klass.name}}::{{field.getterFunctionName}}() const
        {
          _{{klass.name}}* obj = (_{{klass.name}}*)this;
          
          return obj->{{_struct.in_class_name}}.{{field.name}};
        }
      {% endif %}
    
    
      {% endfor %}
    {% endif %}
  {% endfor %}

  //User Code Begin {{klass.name}}PublicMethods
  //User Code End {{klass.name}}PublicMethods
} // namespace odb
// Generator Code End Cpp
