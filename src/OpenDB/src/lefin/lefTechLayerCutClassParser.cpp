#include <boost/bind.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <iostream>
#include <string>

#include "db.h"
#include "lefLayerPropParser.h"
#include "lefin.h"

namespace lefTechLayerCutClass {
namespace qi      = boost::spirit::qi;
namespace ascii   = boost::spirit::ascii;
namespace phoenix = boost::phoenix;
using ascii::char_;
using boost::fusion::at_c;
using boost::spirit::ascii::space_type;
using boost::spirit::ascii::string;
using boost::spirit::qi::lit;
using qi::lexeme;

using qi::double_;
using qi::int_;
// using qi::_1;
using ascii::space;
using phoenix::ref;
void addCutClassRule(boost::fusion::vector<std::string,
                                           double,
                                           boost::optional<double>,
                                           boost::optional<int>>& params,
                     odb::dbTechLayer*                            layer,
                     odb::lefin*                                  lefin)
{
  std::string name = at_c<0>(params);
  auto        rule = odb::dbTechLayerCutClassRule::create(layer, name.c_str());
  rule->setWidth(lefin->dbdist(at_c<1>(params)));
  auto length = at_c<2>(params);
  auto cnt    = at_c<3>(params);
  if (length.is_initialized()) {
    rule->setLengthValid(true);
    rule->setLength(lefin->dbdist(length.value()));
  }
  if (cnt.is_initialized()) {
    rule->setCutsValid(true);
    rule->setNumCuts(cnt.value());
  }
}
template <typename Iterator>
bool parse(Iterator          first,
           Iterator          last,
           odb::dbTechLayer* layer,
           odb::lefin*       lefin)
{
  qi::rule<Iterator, std::string(), ascii::space_type> _string;
  _string %= lexeme[+(char_ - ' ')];
  qi::rule<std::string::iterator, space_type> cutClassRule
      = (+(lit("CUTCLASS") >> _string >> lit("WIDTH") >> double_
           >> -(lit("LENGTH") >> double_) >> -(lit("CUTS") >> int_)
           >> lit(";"))[boost::bind(&addCutClassRule, _1, layer, lefin)]);

  bool valid = qi::phrase_parse(first, last, cutClassRule, space);

  return valid && first == last;
}
}  // namespace lefTechLayerCutClass

namespace odb {

bool lefTechLayerCutClassParser::parse(std::string  s,
                                       dbTechLayer* layer,
                                       odb::lefin*  l)
{
  return lefTechLayerCutClass::parse(s.begin(), s.end(), layer, l);
}

}  // namespace odb
