///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, OpenRoad Project
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

//Generator Code Begin 1
#include "{{klass.name}}.h"
#include "db.h"
#include "dbDiff.hpp"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"

{% for include in klass.cpp_includes %}
  #include "{{include}}"
{% endfor %}
//User Code Begin includes
//User Code End includes
namespace odb {

//User Code Begin definitions
//User Code End definitions
  template class dbTable<_{{klass.name}}>; 
    
  bool _{{klass.name}}::operator==(const _{{klass.name}}& rhs) const
  {
    {% for field in klass.fields %}
      {% if field.bitFields %}
        {% for innerField in klass.structs[0].fields %}
          {% for component in innerField.components %}
            {% if 'no-cmp' not in innerField.flags %}
              {% if innerField.table %}
                if({{field.name}}->{{component}}!=rhs.{{field.name}}->{{component}})
              {% else %}
                if({{field.name}}.{{component}}!=rhs.{{field.name}}.{{component}})
              {% endif %}
                return false;
    
            {% endif %}
          {% endfor %}
        {% endfor %}



      {% else %}

        {% for component in field.components %}
          {% if 'no-cmp' not in field.flags %}
            {% if field.table %}
              if(*{{component}}!=*rhs.{{component}})
            {% else %}
              if({{component}}!=rhs.{{component}})
            {% endif %}
                return false;
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
            
              if({{field.name}}.{{component}}>=rhs.{{field.name}}.{{component}})
                return false;
              
            {% endif %}
          {% endfor %}
        {% endfor %}
        
      {% else %}
        
        {% for component in field.components %}
          {% if 'cmpgt' in field.flags %}
            if({{component}}>=rhs.{{component}})
              return false;
          {% endif %}
        {% endfor %}
      {% endif %}
    {% endfor %}

    //User Code Begin <
    //User Code End <
    return true;
  }
  void _{{klass.name}}::differences(dbDiff& diff, const char* field, const _{{klass.name}}& rhs) const
  {

    DIFF_BEGIN
    
    {% for field in klass.fields %}
      {% if field.bitFields %}
        {% for innerField in klass.structs[0].fields %}
          {% for component in innerField.components %}
            {% if 'no-diff' not in innerField.flags %}
              DIFF_FIELD({{field.name}}.{{component}});
            {% endif %}
          {% endfor %}
        {% endfor %}
      {% else %}
        {% for component in field.components %}
          {% if 'no-diff' not in field.flags %}
            {% if field.table %}
              DIFF_TABLE{%if 'no-deep' in field.flags%}_NO_DEEP{%endif%}({{component}});
            {% elif 'isHashTable' in field and field.isHashTable %}
              DIFF_HASH_TABLE{% if 'no-deep' in field.flags %}_NO_DEEP{% endif %}({{component}});
            {% else %}
              DIFF_FIELD{% if 'no-deep' in field.flags %}_NO_DEEP{% endif %}({{component}});
            {% endif %}
          {% endif %}
        {% endfor %}
      {% endif %}
    {% endfor %}
    //User Code Begin differences
    //User Code End differences
    DIFF_END
  }
  void _{{klass.name}}::out(dbDiff& diff, char side, const char* field) const
  {
    DIFF_OUT_BEGIN
    {% for field in klass.fields %}
      {% if field.bitFields %}
        {% for innerField in klass.structs[0].fields %}
          {% for component in innerField.components %}
            {% if 'no-diff' not in innerField.flags %}
              DIFF_OUT_FIELD({{field.name}}.{{component}});
            {% endif %}
          {% endfor %}
        {% endfor %}
      {% else %}
        {% for component in field.components %}
          {% if 'no-diff' not in field.flags %}
            {% if field.table %}
              DIFF_OUT_TABLE{% if 'no-deep' in field.flags %}_NO_DEEP{% endif %}({{component}});
            {% elif field.isHashTable %}
              DIFF_OUT_HASH_TABLE{% if 'no-deep' in field.flags %}_NO_DEEP{% endif %}({{component}});
            {% else %}
              DIFF_OUT_FIELD{% if 'no-deep' in field.flags %}_NO_DEEP{% endif %}({{component}});
            {% endif %}
          {% endif %}
        {% endfor %}
      {% endif %}
    {% endfor %}

    //User Code Begin out
    //User Code End out
    DIFF_END
  }
  _{{klass.name}}::_{{klass.name}}(_dbDatabase* db)
  {
    {% for field in klass.fields %}
      {% if field.bitFields %}
        {% if field.numBits == 32 %}
          uint32_t* {{field.name}}_bit_field = (uint32_t*) &{{field.name}};
        {% else %}
          uint64_t* {{field.name}}_bit_field = (uint64_t*) &{{field.name}};
        {% endif %}
        *{{field.name}}_bit_field = 0;
      {% elif field.table %}
        {{field.name}} = new dbTable<_{{field.type}}>(db, this, (GetObjTbl_t) &_{{klass.name}}::getObjectTable, {{field.type}}Obj);
        ZALLOCATED({{field.name}});
      {% elif field.isHashTable %}
        {{field.name}}.setTable({{field.table_name}});
      {% endif %}
    {% endfor %}
    //User Code Begin constructor
    //User Code End constructor
  }
  _{{klass.name}}::_{{klass.name}}(_dbDatabase* db, const _{{klass.name}}& r)
  {
    {% for field in klass.fields %}
      {% for component in field.components %}
        {% if field.table %}
          {{field.name}} = new dbTable<_{{field.type}}>(db, this, *r.{{field.name}});
          ZALLOCATED({{field.name}});
        {% elif field.isHashTable %}
          {{field.name}}.setTable({{field.table_name}});
        {% else %}
          {{component}}=r.{{component}};
        {% endif %}
      {% endfor %}
    {% endfor %}
    //User Code Begin CopyConstructor
    //User Code End CopyConstructor
  }
  
  {% for i in range(klass.constructors|length) %}
    _{{klass.name}}::_{{klass.name}}(_dbDatabase* db{% for arg in klass.constructors[i].args %},{{arg.type}} {{arg.name}}{% endfor %})
    {
      {% for arg in klass.constructors[i].args %}
        {% if arg.get('field') is not none %}
          this->{{arg.field}}={{arg.name}};
        {% endif %}
      {% endfor %}
      //User Code Begin CustomConstructor{{i}}
      //User Code End CustomConstructor{{i}}
    }
  {% endfor %}
  dbIStream& operator>>(dbIStream& stream, _{{klass.name}}& obj)
  {
    {% for field in klass.fields %}
      {% if field.bitFields %}
        {% if field.numBits == 32 %}
          uint32_t* {{field.name}}_bit_field = (uint32_t*) &obj.{{field.name}};
        {% else %}
          uint64_t* {{field.name}}_bit_field = (uint64_t*) &obj.{{field.name}};
        {% endif %}
        stream >> *{{field.name}}_bit_field;
      {% else %}
        {% if 'no-serial' not in field.flags %}
          stream >> {% if field.table %}*{% endif %}obj.{{field.name}};
        {% endif %}
      {% endif %}
    {% endfor %}
    //User Code Begin >>
    //User Code End >>
    return stream;
  }
  dbOStream& operator<<(dbOStream& stream, const _{{klass.name}}& obj)
  {
    {% for field in klass.fields %}
      {% if field.bitFields %}
        {% if field.numBits == 32 %}
          uint32_t* {{field.name}}_bit_field = (uint32_t*) &obj.{{field.name}};
        {% else %}
          uint64_t* {{field.name}}_bit_field = (uint64_t*) &obj.{{field.name}};
        {% endif %}
        stream << *{{field.name}}_bit_field;
      {% else %}
        {% if 'no-serial' not in field.flags %}
          stream << {% if field.table %}*{% endif %}obj.{{field.name}};
        {% endif %}
      {% endif %}
    {% endfor %}
    //User Code Begin <<
    //User Code End <<
    return stream;
  }

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
  _{{klass.name}}::~_{{klass.name}}()
  {
    {% for field in klass.fields %}
      {% if field.name == '_name' %}
        if(_name)
          free((void*) _name);
      {% endif %}
      {% if field.table %}
        delete {{field.name}};
      {% endif %}
    {% endfor %}
    //User Code Begin Destructor
    //User Code End Destructor
  }
  ////////////////////////////////////////////////////////////////////
  //
  // {{klass.name}} - Methods
  //
  ////////////////////////////////////////////////////////////////////
  {% for field in klass.fields %}
  
    {% if 'no-set' not in field.flags %}
      void {{klass.name}}::{{field.setterFunctionName}}( {{field.setterArgumentType}} {{field.name}} )
      {
  
        _{{klass.name}}* obj = (_{{klass.name}}*)this;
  
        {% if field.isRef %}
  
          obj->{{field.name}}={{field.name}}->getImpl()->getOID();
  
        {% else %}
          obj->{{field.name}}={{field.name}};
  
        {% endif %}
    }
    {% endif %}
  
    {% if 'no-get' not in field.flags %}
      {% if field.dbSetGetter %}
        dbSet<{{field.type}}> {{klass.name}}::get{{field.functional_name}}() const
        {
          _{{klass.name}}* obj = (_{{klass.name}}*)this;
          return dbSet<{{field.type}}>(obj, obj->{{field.name}});
        }
      {% elif field.isDbVector %}
        void {{klass.name}}::{{field.getterFunctionName}}({{field.getterReturnType}}& tbl) const
        {
          _{{klass.name}}* obj = (_{{klass.name}}*)this;
          tbl = obj->{{field.name}};
        }
      {% elif field.isHashTable %}
        {{field.getterReturnType}} {{klass.name}}::{{field.getterFunctionName}}(const char* name) const
        {
          _{{klass.name}}* obj = (_{{klass.name}}*)this;
          return ({{field.getterReturnType}}) obj->{{field.name}}.find(name);
        }
      {% else %}
        {{field.getterReturnType}} {{klass.name}}::{{field.getterFunctionName}}({% if field.isHashTable %}const char* name{% endif %}) const
        {
          _{{klass.name}}* obj = (_{{klass.name}}*)this;
          {% if field.isRef %}
            if(obj->{{field.name}} == 0)
              return NULL;
            _{{field.parent}}* par = (_{{field.parent}}*) obj->getOwner();
            return ({{field.refType}}) par->_{{field.refType[2:-1].lower()}}_tbl->getPtr(obj->{{field.name}});
          {% elif field.isHashTable %}
            return {{field.getterReturnType}} obj->{{field.name}}.find(name);
          {% else %}
            return obj->{{field.name}};
          {% endif %}
        }
      {% endif %}
    {% endif %}
  {% endfor %}

  {% for _struct in klass.structs %}
    {% if  _struct.in_class %}
      {% for field in _struct.fields %}
      
        {% if 'no-get' not in field.flags %}
          {% if field.dbSetGetter %}
            dbSet<{{field.type}}> {{klass.name}}::get{{field.functional_name}}() const
            {
              _{{klass.name}}* obj = (_{{klass.name}}*)this;
              return dbSet<{{field.type}}>(obj, obj->{{field.name}});
            }
          {% else %}
            {{field.getterReturnType}} {{klass.name}}::{{field.getterFunctionName}}({% if field.isHashTable %}const char* name{% endif %}) const
            {
              _{{klass.name}}* obj = (_{{klass.name}}*)this;
              {% if field.isRef %}
                if(obj->{{field.name}} == 0)
                  return NULL;
                _{{field.parent}}* par = (_{{field.parent}}*) obj->getOwner();
                return ({{field.refType}}) par->_{{field.refType[2:-1].lower()}}_tbl->getPtr(obj->{{field.name}});
              {% elif field.isHashTable %}
                return {{field.getterReturnType}} obj->{{field.name}}.find(name);
              {% else %}
                return obj->{{field.name}};
              {% endif %}
          }
          {% endif %}
        {% endif %}


      {% endfor %}



  {% for _struct in klass.structs %}

    {% if  _struct.in_class %}
      {% for field in _struct.fields %}
      
        {% if 'no-set' not in field.flags %}
          void {{klass.name}}::{{field.setterFunctionName}}( {{field.setterArgumentType}} {{field.name}} )
          {
        
            _{{klass.name}}* obj = (_{{klass.name}}*)this;
        
            obj->{{_struct.in_class_name}}.{{field.name}}={{field.name}};
        
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
}
//Generator Code End 1
