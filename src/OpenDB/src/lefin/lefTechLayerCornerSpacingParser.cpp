/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

namespace lefTechLayerCornerSpacing {
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
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

void setWithin(double value,
               odb::dbTechLayerCornerSpacingRule* rule,
               odb::lefin* lefin)
{
  rule->setCornerOnly(true);
  rule->setWithin(lefin->dbdist(value));
}
void setEolWidth(double value,
                 odb::dbTechLayerCornerSpacingRule* rule,
                 odb::lefin* lefin)
{
  rule->setExceptEol(true);
  rule->setEolWidth(lefin->dbdist(value));
}
void setJogLength(double value,
                  odb::dbTechLayerCornerSpacingRule* rule,
                  odb::lefin* lefin)
{
  rule->setExceptJogLength(true);
  rule->setJogLength(lefin->dbdist(value));
}
void setEdgeLength(double value,
                   odb::dbTechLayerCornerSpacingRule* rule,
                   odb::lefin* lefin)
{
  rule->setEdgeLengthValid(true);
  rule->setEdgeLength(lefin->dbdist(value));
}
void setMinLength(double value,
                  odb::dbTechLayerCornerSpacingRule* rule,
                  odb::lefin* lefin)
{
  rule->setMinLengthValid(true);
  rule->setMinLength(lefin->dbdist(value));
}
void setExceptNotchLength(double value,
                          odb::dbTechLayerCornerSpacingRule* rule,
                          odb::lefin* lefin)
{
  rule->setExceptNotchLengthValid(true);
  rule->setExceptNotchLength(lefin->dbdist(value));
}
void addSpacing(
    boost::fusion::vector<double, double, boost::optional<double>>& params,
    odb::dbTechLayerCornerSpacingRule* rule,
    odb::lefin* lefin)
{
  auto width = lefin->dbdist(at_c<0>(params));
  auto spacing1 = lefin->dbdist(at_c<1>(params));
  auto spacing2 = at_c<2>(params);
  if (spacing2.is_initialized()) {
    rule->addSpacing(width, spacing1, lefin->dbdist(spacing2.value()));
  } else
    rule->addSpacing(width, spacing1, spacing1);
}
template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefin* lefin)
{
  odb::dbTechLayerCornerSpacingRule* rule
      = odb::dbTechLayerCornerSpacingRule::create(layer);
  qi::rule<std::string::iterator, space_type> convexCornerRule
      = (lit("CONVEXCORNER")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setType,
             rule,
             odb::dbTechLayerCornerSpacingRule::CONVEXCORNER)]
         >> -(lit("SAMEMASK")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setSameMask, rule, true)])
         >> -(lit("CORNERONLY")
              >> double_[boost::bind(&setWithin, _1, rule, lefin)])
         >> -(lit("EXCEPTEOL")
              >> double_[boost::bind(&setEolWidth, _1, rule, lefin)]
              >> -(lit("EXCEPTJOGLENGTH")
                   >> double_[boost::bind(&setJogLength, _1, rule, lefin)] >> -(
                       lit("EDGELENGTH")
                       >> double_[boost::bind(&setEdgeLength, _1, rule, lefin)])
                   >> -(lit("INCLUDELSHAPE")[boost::bind(
                       &odb::dbTechLayerCornerSpacingRule::setIncludeShape,
                       rule,
                       true)]))));
  qi::rule<std::string::iterator, space_type> concaveCornerRule
      = (lit("CONCAVECORNER")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setType,
             rule,
             odb::dbTechLayerCornerSpacingRule::CONCAVECORNER)]
         >> -(lit("MINLENGTH")
              >> double_[boost::bind(&setMinLength, _1, rule, lefin)]
              >> -(lit("EXCEPTNOTCH")[boost::bind(
                       &odb::dbTechLayerCornerSpacingRule::setExceptNotch,
                       rule,
                       true)]
                   >> -double_[boost::bind(
                       &setExceptNotchLength, _1, rule, lefin)])));
  qi::rule<std::string::iterator, space_type> exceptSameRule
      = (lit("EXCEPTSAMENET")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setExceptSameNet, rule, true)]
         | lit("EXCEPTSAMEMETAL")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setExceptSameMetal,
             rule,
             true)]);

  qi::rule<std::string::iterator, space_type> spacingRule
      = (lit("WIDTH") >> double_ >> lit("SPACING") >> double_
         >> -double_)[boost::bind(&addSpacing, _1, rule, lefin)];

  qi::rule<std::string::iterator, space_type> cornerSpacingRule
      = (lit("CORNERSPACING") >> (convexCornerRule | concaveCornerRule)
         >> -(exceptSameRule) >> +(spacingRule) >> lit(";"));

  bool valid = qi::phrase_parse(first, last, cornerSpacingRule, space);

  if (!valid)
    odb::dbTechLayerCornerSpacingRule::destroy(rule);

  return valid && first == last;
}
}  // namespace lefTechLayerCornerSpacing

namespace odb {

bool lefTechLayerCornerSpacingParser::parse(std::string s,
                                            dbTechLayer* layer,
                                            odb::lefin* l)
{
  return lefTechLayerCornerSpacing::parse(s.begin(), s.end(), layer, l);
}

}  // namespace odb
