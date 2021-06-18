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

namespace lefTechLayerSpacingTablePrl {
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

void addInfluence(boost::fusion::vector<double, double, double>& params,
                  odb::lefTechLayerSpacingTablePrlParser* parser,
                  odb::lefin* lefin)
{
  auto width = lefin->dbdist(at_c<0>(params));
  auto distance = lefin->dbdist(at_c<1>(params));
  auto spacing = lefin->dbdist(at_c<2>(params));
  parser->influence_tbl.push_back(std::make_tuple(width, distance, spacing));
}
void addLength(double val,
               odb::lefTechLayerSpacingTablePrlParser* parser,
               odb::lefin* lefin)
{
  parser->length_tbl.push_back(lefin->dbdist(val));
}
void addWidth(double val,
              odb::lefTechLayerSpacingTablePrlParser* parser,
              odb::lefin* lefin)
{
  parser->width_tbl.push_back(lefin->dbdist(val));
  parser->curWidthIdx++;
  parser->spacing_tbl.push_back(std::vector<int>());
}
void addExcluded(boost::fusion::vector<double, double>& params,
                 odb::lefTechLayerSpacingTablePrlParser* parser,
                 odb::lefin* lefin)
{
  auto low = lefin->dbdist(at_c<0>(params));
  auto high = lefin->dbdist(at_c<1>(params));
  parser->within_map[parser->curWidthIdx] = {low, high};
}
void addSpacing(double val,
                odb::lefTechLayerSpacingTablePrlParser* parser,
                odb::lefin* lefin)
{
  parser->spacing_tbl[parser->curWidthIdx].push_back(lefin->dbdist(val));
}
void setEolWidth(double val,
                 odb::dbTechLayerSpacingTablePrlRule* rule,
                 odb::lefin* lefin)
{
  rule->setEolWidth(lefin->dbdist(val));
}
template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefin* lefin,
           odb::lefTechLayerSpacingTablePrlParser* parser)
{
  odb::dbTechLayerSpacingTablePrlRule* rule
      = odb::dbTechLayerSpacingTablePrlRule::create(layer);
  qi::rule<std::string::iterator, space_type> SPACINGTABLE
      = (lit("SPACINGTABLE") >> lit("INFLUENCE")
         >> +(lit("WIDTH") >> double_ >> lit("WITHIN") >> double_
              >> lit("SPACING")
              >> double_)[boost::bind(&addInfluence, _1, parser, lefin)]
         >> lit(";"));

  qi::rule<std::string::iterator, space_type> spacingTableRule
      = (lit("SPACINGTABLE") >> lit("PARALLELRUNLENGTH")
         >> -lit("WRONGDIRECTION")[boost::bind(
             &odb::dbTechLayerSpacingTablePrlRule::setWrongDirection,
             rule,
             true)]
         >> -lit("SAMEMASK")[boost::bind(
             &odb::dbTechLayerSpacingTablePrlRule::setSameMask, rule, true)]
         >> -(lit("EXCEPTEOL")[boost::bind(
                  &odb::dbTechLayerSpacingTablePrlRule::setExceeptEol,
                  rule,
                  true)]
              >> double_[boost::bind(&setEolWidth, _1, rule, lefin)])
         >> +double_[boost::bind(&addLength, _1, parser, lefin)]
         >> +(lit("WIDTH") >> double_[boost::bind(&addWidth, _1, parser, lefin)]
              >> -(lit("EXCEPTWITHIN") >> double_
                   >> double_)[boost::bind(&addExcluded, _1, parser, lefin)]
              >> +double_[boost::bind(&addSpacing, _1, parser, lefin)])
         >> -SPACINGTABLE >> lit(";"));

  bool valid = qi::phrase_parse(first, last, spacingTableRule, space);

  if (!valid)  // fail if we did not get a full match
    odb::dbTechLayerSpacingTablePrlRule::destroy(rule);
  else {
    if (parser->spacing_tbl.size() != parser->width_tbl.size())
      valid = false;
    for (auto spacing : parser->spacing_tbl)
      if (spacing.size() != parser->length_tbl.size())
        valid = false;
    if (valid) {
      rule->setTable(parser->width_tbl,
                     parser->length_tbl,
                     parser->spacing_tbl,
                     parser->within_map);
      rule->setSpacingTableInfluence(parser->influence_tbl);
    } else
      odb::dbTechLayerSpacingTablePrlRule::destroy(rule);
  }
  return valid && first == last;
}
}  // namespace lefTechLayerSpacingTablePrl

namespace odb {

bool lefTechLayerSpacingTablePrlParser::parse(std::string s,
                                              dbTechLayer* layer,
                                              odb::lefin* l)
{
  return lefTechLayerSpacingTablePrl::parse(s.begin(), s.end(), layer, l, this);
}

}  // namespace odb
