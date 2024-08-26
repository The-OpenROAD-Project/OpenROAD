//Generator Code Begin ClassDeclarations
{% for klass in schema.classes|sort(attribute='name') %}
class {{klass.name}};
{% endfor %}
//Generator Code End ClassDeclarations
//Generator Code Begin ClassDefinition

{% set classes = schema.classes | sort(attribute='name') %}
{% set non_default_types = [] %}
{% set default_types = [] %}

{% for klass in classes %}
    {% if klass.type is defined and klass.type != 'dbObject' %}
        {% set _ = non_default_types.append(klass) %}
    {% else %}
        {% set _ = default_types.append(klass) %}
    {% endif %}
{% endfor %}

{% for klass in default_types + non_default_types %}

{% if klass.description %}
  {% for line in klass.description %}
    // {{ line }}
  {% endfor %}
{% endif %}
class {{klass.name}} : public {{klass.type if klass.type else "dbObject"}}
{
 public:
  {% for _struct in klass.structs %}
    {% if _struct.public %}
    struct {{ _struct.name }}
    {
      {% for field in _struct.fields %}
        {{field.type}} {{field.name}}{% if "bits" in field %} : {{field.bits}}{% endif %}{% if "default" in field %} = {{field.default}}{% endif %};{% if "comment" in field %} {{field.comment}}{% endif %}
      
      {% endfor %}
    };
    {% endif %}
  {% endfor %}
  {% for _enum in klass.enums %}
    {% if _enum.public %}
    enum {% if _enum.class %} class {% endif %} {{ _enum.name }}{% if "type" in _enum %} :{{ _enum.type }}{% endif %}
    {
      {% for value in _enum["values"]%}
      {% if not loop.first %},{%endif%}{{value}}
      {% endfor %}
    };
    {% endif %}
  {% endfor %}
  // User Code Begin {{klass.name}}Enums
  // User Code End {{klass.name}}Enums
  {% for field in klass.fields %}
    {% if 'no-set' not in field.flags %}
      void {{field.setterFunctionName}} ({% if field.isSetByRef %}const {{field.setterArgumentType}}&{% else %}{{field.setterArgumentType}}{% endif %} {{field.argument}} );
  
    {% endif %}
    {% if 'no-get' not in field.flags %}
      {% if field.dbSetGetter %}
        dbSet<{{field.type}}> get{{field.functional_name}}() const;
      {% elif field.isPassByRef %}
        void {{field.getterFunctionName}}({{field.getterReturnType}}& tbl) const;
      {% elif field.isHashTable %}
        {{field.getterReturnType}} {{field.getterFunctionName}}(const char* name) const;
      {% else %}
        {{field.getterReturnType}} {{field.getterFunctionName}}() const;
      {% endif %}
    {% endif %}
  
  {% endfor %}



  {% for _struct in klass.structs %}
    {% if  _struct.in_class %}
      {% for field in _struct.fields %}
        {% if 'no-set' not in field.flags %}
        void {{field.setterFunctionName}} ({{field.setterArgumentType}} {{field.argument}} );
      
        {% endif %}
        {% if 'no-get' not in field.flags %}
          {{field.getterReturnType}} {{field.getterFunctionName}}() const;
      
        {% endif %}
        {% if field.dbSetGetter %}
          dbSet<{{field.type}}> get{{field.functional_name}}() const;
    
        {% endif %}
      {% endfor %}
    {% endif %}
  {% endfor %}
  //User Code Begin {{klass.name}}
  //User Code End {{klass.name}}
};

{% endfor %}
//Generator Code End ClassDefinition
