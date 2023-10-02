# Generator Code Begin cpp
{% for class in schema.classes %}
    {{class.name}}.cpp
{% endfor %}
{% for itr in schema.iterators %}
    {{itr.name}}.cpp
{% endfor %}
# Generator Code End cpp
