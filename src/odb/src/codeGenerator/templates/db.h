{% import 'macros' as macros %}
//Generator Code Begin ClassDeclarations
{% for klass in schema.classes|sort(attribute='name') %}
class {{klass.name}};
{% endfor %}
//Generator Code End ClassDeclarations
//Generator Code Begin ClassDefinition

{% set classes = schema.classes | sort(attribute='name') %}

{% for klass in classes|by_base_type %}

{% if klass.description %}
  {% for line in klass.description %}
    // {{ line }}
  {% endfor %}
{% endif %}
class {{klass.name}} : public {{klass.type}}
{
 public:
  {{ macros.make_structs(klass, is_public=True) }}
  {{ macros.make_enums(klass, is_public=True) }}
  // User Code Begin {{klass.name}}Enums
  // User Code End {{klass.name}}Enums
  {% for field in klass.fields %}
    {% if 'no-set' not in field.flags %}
      void {{field.setterFunctionName}}(
      {% if field.isSetByRef %}
          const {{field.setterArgumentType}}&
      {% else %}
          {{field.setterArgumentType}}
      {% endif %}
      {{field.argument}} );
  
    {% endif %}
    {% if 'no-get' not in field.flags %}
      {{ macros.getter_signature(field) }};
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
