//Generator Code Begin ClassDeclarations
{% for klass in schema.classes %}
class {{klass.name}};
{% endfor %}
//Generator Code End ClassDeclarations
//Generator Code Begin ClassDefinition
{% for klass in schema.classes %}

class {{klass.name}} : public dbObject
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
    enum {{ _enum.name }}
    {
      {% for value in _enum["values"]%}
      {% if not loop.first %},{%endif%}{{value}}
      {% endfor %}
    };
    {% endif %}
  {% endfor %}
  {% for field in klass.fields %}
    {% if 'no-set' not in field.flags %}
      void {{field.setterFunctionName}} ({{field.setterArgumentType}} {{field.name}} );
  
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
        void {{field.setterFunctionName}} ({{field.setterArgumentType}} {{field.name}} );
      
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
