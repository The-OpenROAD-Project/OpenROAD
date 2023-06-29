//Generator Code Begin DbObjectType
{% for klass in schema.classes %}
  {{klass.name}}Obj,
{% endfor %}
//Generator Code End DbObjectType
