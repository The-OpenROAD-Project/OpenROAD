//Generator Code Begin Less
{% for klass in schema.classes|sort(attribute='name') %}
{% if not klass.do_not_generate_compare %}
template <>
struct less<odb::{{klass.name}}*>
{
  bool operator()(const odb::{{klass.name}}* lhs,
                  const odb::{{klass.name}}* rhs) const
  {
    return odb::compare_by_id(lhs, rhs);
  }
};

{% endif %}
{% endfor %}
//Generator Code End Less
