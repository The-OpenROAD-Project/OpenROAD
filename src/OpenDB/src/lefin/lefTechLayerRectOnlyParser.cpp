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

namespace lefTechLayerRectOnly {
namespace qi      = boost::spirit::qi;
namespace ascii   = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

using boost::fusion::at_c;
using boost::spirit::ascii::space_type;
using boost::spirit::ascii::string;
using boost::spirit::qi::lit;

using qi::double_;
using qi::int_;
// using qi::_1;
using ascii::space;
using phoenix::ref;

template <typename Iterator>
bool parse(Iterator          first,
           Iterator          last,
           odb::dbTechLayer* layer,
           odb::lefin*       lefin)
{
  qi::rule<std::string::iterator, space_type> rightWayOnGridOnlyRule
      = (lit("RECTONLY")[boost::bind(
             &odb::dbTechLayer::setRectOnly, layer, true)]
         >> -lit("EXCEPTNONCOREPINS")[boost::bind(
             &odb::dbTechLayer::setExceptNonCorePins, layer, true)]
         >> lit(";"));

  bool valid = qi::phrase_parse(first, last, rightWayOnGridOnlyRule, space);

  return valid && first == last;
}
}  // namespace lefTechLayerRectOnly

namespace odb {

bool lefTechLayerRectOnlyParser::parse(std::string  s,
                                       dbTechLayer* layer,
                                       odb::lefin*  l)
{
  return lefTechLayerRectOnly::parse(s.begin(), s.end(), layer, l);
}

}  // namespace odb
