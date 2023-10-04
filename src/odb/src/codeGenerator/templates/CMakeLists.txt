# Generator Code Begin cpp
{% for class in schema.classes|sort(attribute='name') %}
    {{class.name}}.cpp
{% endfor %}
{% for itr in schema.iterators|sort(attribute='name') %}
    {{itr.name}}.cpp
{% endfor %}
# Generator Code End cpp
