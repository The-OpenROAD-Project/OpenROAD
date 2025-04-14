// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cstddef>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb::lefTechLayerCutSpacingTable {

void createOrthongonalSubRule(
    std::vector<boost::fusion::vector<double, double>> params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader)
{
  odb::dbTechLayerCutSpacingTableOrthRule* rule
      = odb::dbTechLayerCutSpacingTableOrthRule::create(parser->layer);
  std::vector<std::pair<int, int>> table;
  table.reserve(params.size());
  for (const auto& item : params)
    table.emplace_back(lefinReader->dbdist(at_c<0>(item)),
                       lefinReader->dbdist(at_c<1>(item)));
  rule->setSpacingTable(table);
}
void createDefSubRule(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule
      = odb::dbTechLayerCutSpacingTableDefRule::create(parser->layer);
}
void setDefault(double value,
                odb::lefTechLayerCutSpacingTableParser* parser,
                odb::lefinReader* lefinReader)
{
  parser->curRule->setDefaultValid(true);
  parser->curRule->setDefault(lefinReader->dbdist(value));
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
  for (const auto& item : params) {
    auto from = at_c<0>(item).c_str();
    auto to = at_c<1>(item).c_str();
    parser->curRule->addPrlForAlignedCutEntry(from, to);
  }
}
void setCenterToCenter(
    std::vector<boost::fusion::vector<std::string, std::string>> params,
    odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule->setCenterToCenterValid(true);
  for (const auto& item : params) {
    auto from = at_c<0>(item).c_str();
    auto to = at_c<1>(item).c_str();
    parser->curRule->addCenterToCenterEntry(from, to);
  }
}
void setCenterAndEdge(
    std::vector<boost::fusion::vector<std::string, std::string>> params,
    odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->curRule->setCenterAndEdgeValid(true);
  for (const auto& item : params) {
    auto from = at_c<0>(item).c_str();
    auto to = at_c<1>(item).c_str();
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
    odb::lefinReader* lefinReader)
{
  parser->curRule->setPrlValid(true);
  auto prl = at_c<0>(params);
  auto dir = at_c<1>(params);
  auto maxxy = at_c<2>(params);
  auto items = at_c<3>(params);
  parser->curRule->setPrl(lefinReader->dbdist(prl));
  if (dir.is_initialized()) {
    if (dir.value() == "HORIZONTAL")
      parser->curRule->setPrlHorizontal(true);
    else
      parser->curRule->setPrlVertical(true);
  }
  if (maxxy.is_initialized())
    parser->curRule->setMaxXY(true);

  for (const auto& item : items) {
    auto from = at_c<0>(item).c_str();
    auto to = at_c<1>(item).c_str();
    auto ccPrl = lefinReader->dbdist(at_c<2>(item));
    parser->curRule->addPrlEntry(from, to, ccPrl);
  }
}
void setExactAlignedSpacing(
    boost::fusion::vector<
        boost::optional<std::string>,
        std::vector<boost::fusion::vector<std::string, double>>> params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader)
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

  for (const auto& item : items) {
    auto cls = at_c<0>(item).c_str();
    auto ext = lefinReader->dbdist(at_c<1>(item));
    parser->curRule->addExactElignedEntry(cls, ext);
  }
}

void setNonOppositeEnclosureSpacing(
    std::vector<boost::fusion::vector<std::string, double>> params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->curRule->setNonOppositeEnclosureSpacingValid(true);
  for (const auto& item : params) {
    auto cls = at_c<0>(item).c_str();
    auto ext = lefinReader->dbdist(at_c<1>(item));
    parser->curRule->addNonOppEncSpacingEntry(cls, ext);
  }
}

void setOppositeEnclosureResizeSpacing(
    std::vector<boost::fusion::vector<std::string, double, double, double>>
        params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->curRule->setOppositeEnclosureResizeSpacingValid(true);
  std::vector<std::tuple<char*, int, int, int>> table;
  for (const auto& item : params) {
    auto cls = at_c<0>(item).c_str();
    auto rsz1 = lefinReader->dbdist(at_c<1>(item));
    auto rsz2 = lefinReader->dbdist(at_c<2>(item));
    auto spacing = lefinReader->dbdist(at_c<3>(item));
    parser->curRule->addOppEncSpacingEntry(cls, rsz1, rsz2, spacing);
  }
}

void setEndExtension(
    boost::fusion::vector<
        double,
        std::vector<boost::fusion::vector<std::string, double>>> params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->curRule->setEndExtensionValid(true);
  auto ext = at_c<0>(params);
  auto items = at_c<1>(params);
  parser->curRule->setExtension(lefinReader->dbdist(ext));
  for (const auto& item : items) {
    auto cls = at_c<0>(item).c_str();
    auto ext = lefinReader->dbdist(at_c<1>(item));
    parser->curRule->addEndExtensionEntry(cls, ext);
  }
}
void setSideExtension(
    std::vector<boost::fusion::vector<std::string, double>> params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->curRule->setSideExtensionValid(true);
  for (const auto& item : params) {
    auto cls = at_c<0>(item).c_str();
    auto ext = lefinReader->dbdist(at_c<1>(item));
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
    odb::lefinReader* lefinReader)
{
  auto colsNamesAndFirstRowName = at_c<0>(params);
  auto firstRowWithOutName = at_c<1>(params);
  auto allRows = at_c<2>(params);
  std::map<std::string, odb::uint> cols;
  std::map<std::string, odb::uint> rows;
  std::vector<std::vector<std::pair<int, int>>> table;
  odb::uint colSz = colsNamesAndFirstRowName.size() - 1;
  for (odb::uint i = 0; i < colSz; i++) {
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
  odb::uint rowSz = allRows.size();
  for (odb::uint i = 0; i < rowSz; i++) {
    std::string name = at_c<0>(allRows[i]);
    auto OPTION = at_c<1>(allRows[i]);
    auto items = at_c<2>(allRows[i]);
    if (OPTION.is_initialized()) {
      name += "/" + OPTION.value();
    }
    rows[name] = i;
    table.push_back(std::vector<std::pair<int, int>>(colSz));
    for (odb::uint j = 0; j < items.size(); j++) {
      auto item = items[j];
      auto spacing1 = at_c<0>(item);
      auto spacing2 = at_c<1>(item);
      int sp1, sp2;
      if (spacing1.which() == 0)
        sp1 = parser->curRule->getDefault();
      else
        sp1 = lefinReader->dbdist(boost::get<double>(spacing1));
      if (spacing2.which() == 0)
        sp2 = parser->curRule->getDefault();
      else
        sp2 = lefinReader->dbdist(boost::get<double>(spacing2));
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
        for (auto& k : table) {
          k.push_back(k[i]);
        }
      }
      cols.erase(it++);
    } else {
      ++it;
    }
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
    odb::lefinReader* lefinReader,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  qi::rule<std::string::iterator, space_type> ORTHOGONAL
      = (lit("SPACINGTABLE") >> lit("ORTHOGONAL")
         >> +(lit("WITHIN") >> double_ >> lit("SPACING") >> double_)
         >> lit(";"))[boost::bind(
          &createOrthongonalSubRule, _1, parser, lefinReader)];

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
              >> double_))[boost::bind(&setPRL, _1, parser, lefinReader)];
  qi::rule<std::string::iterator, space_type> EXTENSION
      = ((lit("ENDEXTENSION") >> double_
          >> *(lit("TO") >> _string >> double_))[boost::bind(
             &setEndExtension, _1, parser, lefinReader)]
         >> -(lit("SIDEEXTENSION")
              >> +(lit("TO") >> _string >> double_))[boost::bind(
             &setSideExtension, _1, parser, lefinReader)]);

  qi::rule<std::string::iterator, space_type> EXACTALIGNEDSPACING
      = (lit("EXACTALIGNEDSPACING")
         >> -(string("HORIZONTAL") | string("VERTICAL"))
         >> +(_string >> double_))[boost::bind(
          &setExactAlignedSpacing, _1, parser, lefinReader)];

  qi::rule<std::string::iterator, space_type> NONOPPOSITEENCLOSURESPACING
      = (lit("NONOPPOSITEENCLOSURESPACING")
         >> +(_string >> double_))[boost::bind(
          &setNonOppositeEnclosureSpacing, _1, parser, lefinReader)];

  qi::rule<std::string::iterator, space_type> OPPOSITEENCLOSURERESIZESPACING
      = (lit("OPPOSITEENCLOSURERESIZESPACING")
         >> +(_string >> double_ >> double_ >> double_))[boost::bind(
          &setOppositeEnclosureResizeSpacing, _1, parser, lefinReader)];

  qi::rule<std::string::iterator, space_type> CUTCLASS
      = (lit("CUTCLASS")
         >> +(_string
              >> -(string("SIDE")
                   | string("END")))  // FIRST ROW AND FIRST ENTRY OF SECOND ROW
         >> +((string("-") | double_)
              >> (string("-") | double_))  // REMAINING SECOND ROW
         >> *(_string >> -(string("SIDE") | string("END"))
              >> +((string("-") | double_)
                   >> (string("-")
                       | double_)))  // REMAINING ROWS (3rd and below)
         )[boost::bind(&setCutClass, _1, parser, lefinReader)];

  qi::rule<std::string::iterator, space_type> DEFAULT
      = (lit("SPACINGTABLE")[boost::bind(&createDefSubRule, parser)]
         >> -(lit("DEFAULT")
              >> double_[boost::bind(&setDefault, _1, parser, lefinReader)])
         >> -lit("SAMEMASK")[boost::bind(&setSameMask, parser)]
         >> -(lit("SAMENET")[boost::bind(&setSameNet, parser)]
              | lit("SAMEMETAL")[boost::bind(&setSameMetal, parser)]
              | lit("SAMEVIA")[boost::bind(&setSameVia, parser)])
         >> -LAYER >> -CENTERTOCENTER >> -CENTERANDEDGE >> -PRL >> -EXTENSION
         >> -EXACTALIGNEDSPACING >> -NONOPPOSITEENCLOSURESPACING
         >> -OPPOSITEENCLOSURERESIZESPACING >> CUTCLASS >> lit(";"));

  qi::rule<std::string::iterator, space_type> LEF58_SPACINGTABLE
      = (+(ORTHOGONAL | DEFAULT));
  bool valid = qi::phrase_parse(first, last, LEF58_SPACINGTABLE, space)
               && first == last;
  if (!valid && parser->curRule != nullptr) {
    if (!incomplete_props.empty()
        && incomplete_props.back().first == parser->curRule)
      incomplete_props.pop_back();
    odb::dbTechLayerCutSpacingTableDefRule::destroy(parser->curRule);
  }

  return valid;
}
}  // namespace odb::lefTechLayerCutSpacingTable

namespace odb {

bool lefTechLayerCutSpacingTableParser::parse(
    std::string s,
    odb::lefinReader* l,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  return lefTechLayerCutSpacingTable::parse(
      s.begin(), s.end(), this, l, incomplete_props);
}

}  // namespace odb
