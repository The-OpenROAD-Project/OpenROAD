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
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/include/make_vector.hpp>
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
#include <vector>

#include "db.h"
#include "lefLayerPropParser.h"
#include "lefin.h"

namespace lefTechLayerCutSpacingTable {
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;
using ascii::char_;
using boost::fusion::at_c;
using boost::spirit::ascii::alpha;
using boost::spirit::ascii::space_type;
using boost::spirit::ascii::string;
using boost::spirit::qi::lit;
using qi::lexeme;

using qi::double_;
using qi::int_;
// using qi::_1;
using ascii::space;
using phoenix::ref;
void createOrthongonalSubRule(
    std::vector<boost::fusion::vector<double, double>> params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefin* lefin)
{
  odb::dbTechLayerCutSpacingTableOrthRule* rule
      = odb::dbTechLayerCutSpacingTableOrthRule::create(parser->layer);
  std::vector<std::pair<int, int>> table;
  for (auto item : params)
    table.push_back(
        {lefin->dbdist(at_c<0>(item)), lefin->dbdist(at_c<1>(item))});
  rule->setSpacingTable(table);
}
void createDefSubRule(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule
      = odb::dbTechLayerCutSpacingTableDefRule::create(parser->layer);
}
void setDefault(double value,
                odb::lefTechLayerCutSpacingTableParser* parser,
                odb::lefin* lefin)
{
  parser->curRule->setDefaultValid(true);
  parser->curRule->setDefault(lefin->dbdist(value));
}
void setLayer(
    std::string value,
    odb::lefTechLayerCutSpacingTableParser* parser,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  odb::dbTech* tech = parser->layer->getTech();
  auto secondLayer = tech->findLayer(value.c_str());
  parser->curRule->setLayerValid(true);
  if (secondLayer == nullptr) {
    incomplete_props.push_back({parser->curRule, value});
  } else {
    parser->curRule->setSecondLayer(secondLayer);
  }
}
void setPrlForAlignedCut(
    std::vector<boost::fusion::vector<std::string, std::string>> params,
    odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule->setPrlForAlignedCut(true);
  for (auto item : params) {
    auto from = parser->layer->findTechLayerCutClassRule(at_c<0>(item).c_str());
    auto to = parser->layer->findTechLayerCutClassRule(at_c<1>(item).c_str());
    if (from != nullptr && to != nullptr)
      parser->curRule->addPrlForAlignedCutEntry(from, to);
  }
}
void setCenterToCenter(
    std::vector<boost::fusion::vector<std::string, std::string>> params,
    odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule->setCenterToCenterValid(true);
  for (auto item : params) {
    auto from = parser->layer->findTechLayerCutClassRule(at_c<0>(item).c_str());
    auto to = parser->layer->findTechLayerCutClassRule(at_c<1>(item).c_str());
    if (from != nullptr && to != nullptr)
      parser->curRule->addCenterToCenterEntry(from, to);
  }
}
void setCenterAndEdge(
    std::vector<boost::fusion::vector<std::string, std::string>> params,
    odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule->setCenterAndEdgeValid(true);
  for (auto item : params) {
    auto from = parser->layer->findTechLayerCutClassRule(at_c<0>(item).c_str());
    auto to = parser->layer->findTechLayerCutClassRule(at_c<1>(item).c_str());
    if (from != nullptr && to != nullptr)
      parser->curRule->addCenterAndEdgeEntry(from, to);
  }
}
void setPRL(
    boost::fusion::vector<
        double,
        boost::optional<std::string>,
        boost::optional<std::string>,
        std::vector<boost::fusion::vector<std::string, std::string, double>>>
        params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefin* lefin)
{
  parser->curRule->setPrlValid(true);
  auto prl = at_c<0>(params);
  auto dir = at_c<1>(params);
  auto maxxy = at_c<2>(params);
  auto items = at_c<3>(params);
  parser->curRule->setPrl(lefin->dbdist(prl));
  if (dir.is_initialized()) {
    if (dir.value() == "HORIZONTAL")
      parser->curRule->setPrlHorizontal(true);
    else
      parser->curRule->setPrlVertical(true);
  }
  if (maxxy.is_initialized())
    parser->curRule->setMaxXY(true);

  for (auto item : items) {
    auto from = parser->layer->findTechLayerCutClassRule(at_c<0>(item).c_str());
    auto to = parser->layer->findTechLayerCutClassRule(at_c<1>(item).c_str());
    auto ccPrl = lefin->dbdist(at_c<2>(item));
    if (from != nullptr && to != nullptr)
      parser->curRule->addPrlEntry(from, to, ccPrl);
  }
}
void setExactAlignedSpacing(
    boost::fusion::vector<
        boost::optional<std::string>,
        std::vector<boost::fusion::vector<std::string, double>>> params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefin* lefin)
{
  parser->curRule->setExactAlignedSpacingValid(true);
  auto dir = at_c<0>(params);
  auto items = at_c<1>(params);
  if (dir.is_initialized()) {
    if (dir.value() == "HORIZONTAL")
      parser->curRule->setHorizontal(true);
    else
      parser->curRule->setVertical(true);
  }

  for (auto item : items) {
    auto cls = parser->layer->findTechLayerCutClassRule(at_c<0>(item).c_str());
    auto ext = lefin->dbdist(at_c<1>(item));
    if (cls != nullptr)
      parser->curRule->addExactElignedEntry(cls, ext);
  }
}

void setNonOppositeEnclosureSpacing(
    std::vector<boost::fusion::vector<std::string, double>> params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefin* lefin)
{
  parser->curRule->setNonOppositeEnclosureSpacingValid(true);
  for (auto item : params) {
    auto cls = parser->layer->findTechLayerCutClassRule(at_c<0>(item).c_str());
    auto ext = lefin->dbdist(at_c<1>(item));
    if (cls != nullptr)
      parser->curRule->addExactElignedEntry(cls, ext);
  }
}

void setOppositeEnclosureResizeSpacing(
    std::vector<boost::fusion::vector<std::string, double, double, double>>
        params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefin* lefin)
{
  parser->curRule->setOppositeEnclosureResizeSpacingValid(true);
  std::vector<std::tuple<char*, int, int, int>> table;
  for (auto item : params) {
    auto cls = parser->layer->findTechLayerCutClassRule(at_c<0>(item).c_str());
    auto rsz1 = lefin->dbdist(at_c<1>(item));
    auto rsz2 = lefin->dbdist(at_c<2>(item));
    auto spacing = lefin->dbdist(at_c<3>(item));
    if (cls != nullptr)
      parser->curRule->addOppEncSpacingEntry(cls, rsz1, rsz2, spacing);
  }
}

void setEndExtension(
    boost::fusion::vector<
        double,
        std::vector<boost::fusion::vector<std::string, double>>> params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefin* lefin)
{
  parser->curRule->setEndExtensionValid(true);
  auto ext = at_c<0>(params);
  auto items = at_c<1>(params);
  parser->curRule->setExtension(lefin->dbdist(ext));
  for (auto item : items) {
    auto cls = parser->layer->findTechLayerCutClassRule(at_c<0>(item).c_str());
    auto ext = lefin->dbdist(at_c<1>(item));
    if (cls != nullptr)
      parser->curRule->addEndExtensionEntry(cls, ext);
  }
}
void setSideExtension(
    std::vector<boost::fusion::vector<std::string, double>> params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefin* lefin)
{
  parser->curRule->setSideExtensionValid(true);
  for (auto item : params) {
    auto cls = parser->layer->findTechLayerCutClassRule(at_c<0>(item).c_str());
    auto ext = lefin->dbdist(at_c<1>(item));
    if (cls != nullptr)
      parser->curRule->addSideExtensionEntry(cls, ext);
  }
}

void setSameMask(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule->setSameMask(true);
}
void setSameNet(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule->setSameNet(true);
}
void setSameMetal(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule->setSameMetal(true);
}
void setSameVia(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule->setSameVia(true);
}
void setNoStack(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule->setNoStack(true);
}
void setNonZeroEnclosure(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule->setNonZeroEnclosure(true);
}
void setNoPrl(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule->setNoPrl(true);
}

void setCutClass(
    boost::fusion::vector<
        std::vector<
            boost::fusion::vector<std::string, boost::optional<std::string>>>,
        std::vector<boost::fusion::vector<boost::variant<std::string, double>,
                                          boost::variant<std::string, double>>>,
        std::vector<boost::fusion::vector<
            std::string,
            boost::optional<std::string>,
            std::vector<boost::fusion::vector<
                boost::variant<std::string, double>,
                boost::variant<std::string, double>>>>>>& params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefin* lefin)
{
  auto colsNamesAndFirstRowName = at_c<0>(params);
  auto firstRowWithOutName = at_c<1>(params);
  auto allRows = at_c<2>(params);
  std::map<std::string, uint> cols;
  std::map<std::string, uint> rows;
  std::vector<std::vector<std::pair<int, int>>> table;
  uint colSz = colsNamesAndFirstRowName.size() - 1;
  for (uint i = 0; i < colSz; i++) {
    std::string name = at_c<0>(colsNamesAndFirstRowName[i]);
    auto OPTION = at_c<1>(colsNamesAndFirstRowName[i]);
    if (OPTION.is_initialized()) {
      name += "/" + OPTION.value();
    }
    cols[name] = i;
  }

  boost::fusion::vector<
      std::string,
      boost::optional<std::string>,
      std::vector<boost::fusion::vector<boost::variant<std::string, double>,
                                        boost::variant<std::string, double>>>>
      firstRow(at_c<0>(colsNamesAndFirstRowName[colSz]),
               at_c<1>(colsNamesAndFirstRowName[colSz]),
               firstRowWithOutName);
  allRows.insert(allRows.begin(), firstRow);
  uint rowSz = allRows.size();
  for (uint i = 0; i < rowSz; i++) {
    std::string name = at_c<0>(allRows[i]);
    auto OPTION = at_c<1>(allRows[i]);
    auto items = at_c<2>(allRows[i]);
    if (OPTION.is_initialized()) {
      name += "/" + OPTION.value();
    }
    rows[name] = i;
    table.push_back(std::vector<std::pair<int, int>>(colSz));
    for (uint j = 0; j < items.size(); j++) {
      auto item = items[j];
      auto spacing1 = at_c<0>(item);
      auto spacing2 = at_c<1>(item);
      int sp1, sp2;
      if (spacing1.which() == 0)
        sp1 = parser->curRule->getDefault();
      else
        sp1 = lefin->dbdist(boost::get<double>(spacing1));
      if (spacing2.which() == 0)
        sp2 = parser->curRule->getDefault();
      else
        sp2 = lefin->dbdist(boost::get<double>(spacing2));
      table[i][j] = {sp1, sp2};
    }
  }
  for (auto it = cols.cbegin(); it != cols.cend();) {
    std::string col = (*it).first;
    int i = (*it).second;
    size_t idx = col.find_last_of('/');
    if (idx == std::string::npos) {
      if (cols.find(col + "/SIDE") != cols.end())
        cols[col + "/END"] = i;
      else if (cols.find(col + "/END") != cols.end())
        cols[col + "/SIDE"] = i;
      else {
        cols[col + "/SIDE"] = i;
        cols[col + "/END"] = colSz++;
        for (size_t k = 0; k < table.size(); k++)
          table[k].push_back(table[k][i]);
      }
      cols.erase(it++);
    } else
      ++it;
  }
  for (auto it = rows.cbegin(); it != rows.cend();) {
    std::string row = (*it).first;
    int i = (*it).second;
    size_t idx = row.find_last_of('/');
    if (idx == std::string::npos) {
      if (rows.find(row + "/SIDE") != rows.end())
        rows[row + "/END"] = i;
      else if (rows.find(row + "/END") != rows.end())
        rows[row + "/SIDE"] = i;
      else {
        rows[row + "/SIDE"] = i;
        rows[row + "/END"] = rowSz++;
        table.push_back(table[i]);
      }
      rows.erase(it++);
    } else
      ++it;
  }
  parser->curRule->setSpacingTable(table, rows, cols);
}
void print(std::string str)
{
  std::cout << str << std::endl;
}

template <typename Iterator>
bool parse(
    Iterator first,
    Iterator last,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefin* lefin,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  qi::rule<Iterator, std::string(), ascii::space_type> _string;
  _string %= lexeme[(alpha >> *(char_ - ' ' - '\n'))];

  qi::rule<std::string::iterator, space_type> ORTHOGONAL
      = (lit("SPACINGTABLE") >> lit("ORTHOGONAL")
         >> +(lit("WITHIN") >> double_ >> lit("SPACING") >> double_) >> lit(
             ";"))[boost::bind(&createOrthongonalSubRule, _1, parser, lefin)];

  qi::rule<std::string::iterator, space_type> LAYER
      = (lit("LAYER") >> _string[boost::bind(
             &setLayer, _1, parser, boost::ref(incomplete_props))]
         >> -lit("NOSTACK")[boost::bind(&setNoStack, parser)]
         >> -(lit("NONZEROENCLOSURE")[boost::bind(&setNonZeroEnclosure, parser)]
              | (lit("PRLFORALIGNEDCUT")
                 >> +(_string >> lit("TO") >> _string))[boost::bind(
                  &setPrlForAlignedCut, _1, parser)]));

  qi::rule<std::string::iterator, space_type> CENTERTOCENTER
      = (lit("CENTERTOCENTER")
         >> +(_string >> lit("TO")
              >> _string))[boost::bind(&setCenterToCenter, _1, parser)];

  qi::rule<std::string::iterator, space_type> CENTERANDEDGE
      = (lit("CENTERANDEDGE") >> -lit("NOPRL")[boost::bind(&setNoPrl, parser)]
         >> (+(_string >> lit("TO")
               >> _string))[boost::bind(&setCenterAndEdge, _1, parser)]);
  qi::rule<std::string::iterator, space_type> PRL
      = (lit("PRL") >> double_ >> -(string("HORIZONTAL") | string("VERTICAL"))
         >> -string("MAXXY")
         >> *(_string >> lit("TO") >> _string
              >> double_))[boost::bind(&setPRL, _1, parser, lefin)];
  qi::rule<std::string::iterator, space_type> EXTENSION
      = ((lit("ENDEXTENSION") >> double_
          >> *(lit("TO") >> _string
               >> double_))[boost::bind(&setEndExtension, _1, parser, lefin)]
         >> -(lit("SIDEEXTENSION")
              >> +(lit("TO") >> _string >> double_))[boost::bind(
             &setSideExtension, _1, parser, lefin)]);

  qi::rule<std::string::iterator, space_type> EXACTALIGNEDSPACING
      = (lit("EXACTALIGNEDSPACING")
         >> -(string("HORIZONTAL") | string("VERTICAL"))
         >> +(_string >> double_))[boost::bind(
          &setExactAlignedSpacing, _1, parser, lefin)];

  qi::rule<std::string::iterator, space_type> NONOPPOSITEENCLOSURESPACING
      = (lit("NONOPPOSITEENCLOSURESPACING")
         >> +(_string >> double_))[boost::bind(
          &setNonOppositeEnclosureSpacing, _1, parser, lefin)];

  qi::rule<std::string::iterator, space_type> OPPOSITEENCLOSURERESIZESPACING
      = (lit("OPPOSITEENCLOSURERESIZESPACING")
         >> +(_string >> double_ >> double_ >> double_))[boost::bind(
          &setOppositeEnclosureResizeSpacing, _1, parser, lefin)];

  qi::rule<std::string::iterator, space_type> CUTCLASS
      = (lit("CUTCLASS") >> +(_string >> -(string("SIDE") | string("END")))
         >> +((string("-") | double_) >> (string("-") | double_))
         >> *(_string >> -(string("SIDE") | string("END"))
              >> +((string("-") | double_) >> (string("-") | double_))))
          [boost::bind(&setCutClass, _1, parser, lefin)];

  qi::rule<std::string::iterator, space_type> DEFAULT
      = (lit("SPACINGTABLE")[boost::bind(&createDefSubRule, parser)]
         >> -(lit("DEFAULT")
              >> double_[boost::bind(&setDefault, _1, parser, lefin)])
         >> -lit("SAMEMASK")[boost::bind(&setSameMask, parser)]
         >> -(lit("SAMENET")[boost::bind(&setSameNet, parser)]
              | lit("SAMEMETAL")[boost::bind(&setSameMetal, parser)]
              | lit("SAMEVIA")[boost::bind(&setSameVia, parser)])
         >> -LAYER >> -CENTERTOCENTER >> -CENTERANDEDGE >> -PRL >> -EXTENSION
         >> -EXACTALIGNEDSPACING >> -NONOPPOSITEENCLOSURESPACING
         >> -OPPOSITEENCLOSURERESIZESPACING >> CUTCLASS >> lit(";"));

  qi::rule<std::string::iterator, space_type> LEF58_SPACINGTABLE
      = (+(ORTHOGONAL | DEFAULT));
  bool valid = qi::phrase_parse(first, last, LEF58_SPACINGTABLE, space);
  if (!valid && parser->curRule != nullptr) {
    if (!incomplete_props.empty()
        && incomplete_props.back().first == parser->curRule)
      incomplete_props.pop_back();
    odb::dbTechLayerCutSpacingTableDefRule::destroy(parser->curRule);
  }

  return valid && first == last;
}
}  // namespace lefTechLayerCutSpacingTable

namespace odb {

bool lefTechLayerCutSpacingTableParser::parse(
    std::string s,
    odb::lefin* l,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  return lefTechLayerCutSpacingTable::parse(
      s.begin(), s.end(), this, l, incomplete_props);
}

}  // namespace odb
