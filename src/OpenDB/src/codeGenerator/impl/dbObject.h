//Generator Code Begin dbObjectType
{% for klass in schema.classes %}
  {{klass.name}}Obj,
{% endfor %}
//Generator Code End dbObjectType
