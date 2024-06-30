//Generator Code Begin ObjectNames
{% for klass in schema.classes|sort(attribute='name') %}
  "{{klass.name}}",
{% endfor %}
//Generator Code End ObjectNames
