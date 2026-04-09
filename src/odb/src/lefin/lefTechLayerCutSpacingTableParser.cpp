// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb::lefTechLayerCutSpacingTable {

void createOrthogonalSubRule(
    const std::vector<boost::fusion::vector<double, double>>& params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader)
{
  odb::dbTechLayerCutSpacingTableOrthRule* orth_rule
      = odb::dbTechLayerCutSpacingTableOrthRule::create(parser->layer);
  std::vector<std::pair<int, int>> table;
  table.reserve(params.size());
  for (const auto& item : params) {
    table.emplace_back(lefinReader->dbdist(at_c<0>(item)),
                       lefinReader->dbdist(at_c<1>(item)));
  }
  orth_rule->setSpacingTable(table);
}
void createDefSubRule(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->rule = odb::dbTechLayerCutSpacingTableDefRule::create(parser->layer);
}
void setDefault(double value,
                odb::lefTechLayerCutSpacingTableParser* parser,
                odb::lefinReader* lefinReader)
{
  parser->rule->setDefaultValid(true);
  parser->rule->setDefault(lefinReader->dbdist(value));
}
void setLayer(
    const std::string& value,
    odb::lefTechLayerCutSpacingTableParser* parser,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  odb::dbTech* tech = parser->layer->getTech();
  auto secondLayer = tech->findLayer(value.c_str());
  parser->rule->setLayerValid(true);
  if (secondLayer == nullptr) {
    incomplete_props.emplace_back(parser->rule, value);
  } else {
    parser->rule->setSecondLayer(secondLayer);
  }
}
void setPrlForAlignedCut(
    const std::vector<boost::fusion::vector<std::string, std::string>>& params,
    odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->rule->setPrlForAlignedCut(true);
  for (const auto& item : params) {
    auto from = at_c<0>(item).c_str();
    auto to = at_c<1>(item).c_str();
    parser->rule->addPrlForAlignedCutEntry(from, to);
  }
}
void setCenterToCenter(
    const std::vector<boost::fusion::vector<std::string, std::string>>& params,
    odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->rule->setCenterToCenterValid(true);
  for (const auto& item : params) {
    auto from = at_c<0>(item).c_str();
    auto to = at_c<1>(item).c_str();
    parser->rule->addCenterToCenterEntry(from, to);
  }
}
void setCenterAndEdge(
    const std::vector<boost::fusion::vector<std::string, std::string>>& params,
    odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->rule->setCenterAndEdgeValid(true);
  for (const auto& item : params) {
    auto from = at_c<0>(item).c_str();
    auto to = at_c<1>(item).c_str();
    parser->rule->addCenterAndEdgeEntry(from, to);
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
  parser->rule->setPrlValid(true);
  auto prl = at_c<0>(params);
  auto dir = at_c<1>(params);
  auto maxxy = at_c<2>(params);
  auto items = at_c<3>(params);
  parser->rule->setPrl(lefinReader->dbdist(prl));
  if (dir.is_initialized()) {
    if (dir.value() == "HORIZONTAL") {
      parser->rule->setPrlHorizontal(true);
    } else {
      parser->rule->setPrlVertical(true);
    }
  }
  if (maxxy.is_initialized()) {
    parser->rule->setMaxXY(true);
  }

  for (const auto& item : items) {
    auto from = at_c<0>(item).c_str();
    auto to = at_c<1>(item).c_str();
    auto ccPrl = lefinReader->dbdist(at_c<2>(item));
    parser->rule->addPrlEntry(from, to, ccPrl);
  }
}
void setExactAlignedSpacing(
    boost::fusion::vector<
        boost::optional<std::string>,
        std::vector<boost::fusion::vector<std::string, double>>> params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->rule->setExactAlignedSpacingValid(true);
  auto dir = at_c<0>(params);
  auto items = at_c<1>(params);
  if (dir.is_initialized()) {
    if (dir.value() == "HORIZONTAL") {
      parser->rule->setHorizontal(true);
    } else {
      parser->rule->setVertical(true);
    }
  }

  for (const auto& item : items) {
    auto cls = at_c<0>(item).c_str();
    auto ext = lefinReader->dbdist(at_c<1>(item));
    parser->rule->addExactElignedEntry(cls, ext);
  }
}

void setNonOppositeEnclosureSpacing(
    const std::vector<boost::fusion::vector<std::string, double>>& params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->rule->setNonOppositeEnclosureSpacingValid(true);
  for (const auto& item : params) {
    auto cls = at_c<0>(item).c_str();
    auto ext = lefinReader->dbdist(at_c<1>(item));
    parser->rule->addNonOppEncSpacingEntry(cls, ext);
  }
}

void setOppositeEnclosureResizeSpacing(
    const std::vector<
        boost::fusion::vector<std::string, double, double, double>>& params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->rule->setOppositeEnclosureResizeSpacingValid(true);
  std::vector<std::tuple<char*, int, int, int>> table;
  for (const auto& item : params) {
    auto cls = at_c<0>(item).c_str();
    auto rsz1 = lefinReader->dbdist(at_c<1>(item));
    auto rsz2 = lefinReader->dbdist(at_c<2>(item));
    auto spacing = lefinReader->dbdist(at_c<3>(item));
    parser->rule->addOppEncSpacingEntry(cls, rsz1, rsz2, spacing);
  }
}

void setEndExtension(
    boost::fusion::vector<
        double,
        std::vector<boost::fusion::vector<std::string, double>>> params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->rule->setEndExtensionValid(true);
  auto ext = at_c<0>(params);
  auto items = at_c<1>(params);
  parser->rule->setExtension(lefinReader->dbdist(ext));
  for (const auto& item : items) {
    auto cls = at_c<0>(item).c_str();
    auto ext = lefinReader->dbdist(at_c<1>(item));
    parser->rule->addEndExtensionEntry(cls, ext);
  }
}
void setSideExtension(
    const std::vector<boost::fusion::vector<std::string, double>>& params,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->rule->setSideExtensionValid(true);
  for (const auto& item : params) {
    auto cls = at_c<0>(item).c_str();
    auto ext = lefinReader->dbdist(at_c<1>(item));
    parser->rule->addSideExtensionEntry(cls, ext);
  }
}

void setSameMask(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->rule->setSameMask(true);
}
void setSameNet(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->rule->setSameNet(true);
}
void setSameMetal(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->rule->setSameMetal(true);
}
void setSameVia(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->rule->setSameVia(true);
}
void setNoStack(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->rule->setNoStack(true);
}
void setNonZeroEnclosure(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->rule->setNonZeroEnclosure(true);
}
void setNoPrl(odb::lefTechLayerCutSpacingTableParser* parser)
{
  parser->rule->setNoPrl(true);
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
  std::map<std::string, uint32_t> cols;
  std::map<std::string, uint32_t> rows;
  std::vector<std::vector<std::pair<int, int>>> table;
  uint32_t colSz = colsNamesAndFirstRowName.size() - 1;
  for (uint32_t i = 0; i < colSz; i++) {
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
  uint32_t rowSz = allRows.size();
  for (uint32_t i = 0; i < rowSz; i++) {
    std::string name = at_c<0>(allRows[i]);
    auto OPTION = at_c<1>(allRows[i]);
    auto items = at_c<2>(allRows[i]);
    if (OPTION.is_initialized()) {
      name += "/" + OPTION.value();
    }
    rows[name] = i;
    table.emplace_back(colSz);
    for (uint32_t j = 0; j < items.size(); j++) {
      auto item = items[j];
      auto spacing1 = at_c<0>(item);
      auto spacing2 = at_c<1>(item);
      int sp1, sp2;
      if (spacing1.which() == 0) {
        sp1 = parser->rule->getDefault();
      } else {
        sp1 = lefinReader->dbdist(boost::get<double>(spacing1));
      }
      if (spacing2.which() == 0) {
        sp2 = parser->rule->getDefault();
      } else {
        sp2 = lefinReader->dbdist(boost::get<double>(spacing2));
      }
      table[i][j] = {sp1, sp2};
    }
  }
  for (auto it = cols.cbegin(); it != cols.cend();) {
    std::string col = (*it).first;
    int i = (*it).second;
    size_t idx = col.find_last_of('/');
    if (idx == std::string::npos) {
      if (cols.find(col + "/SIDE") != cols.end()) {
        cols[col + "/END"] = i;
      } else if (cols.find(col + "/END") != cols.end()) {
        cols[col + "/SIDE"] = i;
      } else {
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
      if (rows.find(row + "/SIDE") != rows.end()) {
        rows[row + "/END"] = i;
      } else if (rows.find(row + "/END") != rows.end()) {
        rows[row + "/SIDE"] = i;
      } else {
        rows[row + "/SIDE"] = i;
        rows[row + "/END"] = rowSz++;
        table.push_back(table[i]);
      }
      rows.erase(it++);
    } else {
      ++it;
    }
  }
  parser->rule->setSpacingTable(table, rows, cols);
}
void print(const std::string& str)
{
  std::cout << str << '\n';
}

template <typename Iterator>
bool parse(
    Iterator first,
    Iterator last,
    odb::lefTechLayerCutSpacingTableParser* parser,
    odb::lefinReader* lefinReader,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  qi::rule<std::string::const_iterator, space_type> orthogonal_rule
      = (lit("SPACINGTABLE") >> lit("ORTHOGONAL")
         >> +(lit("WITHIN") >> double_ >> lit("SPACING") >> double_)
         >> lit(";"))[boost::bind(
          &createOrthogonalSubRule, _1, parser, lefinReader)];

  qi::rule<std::string::const_iterator, space_type> layer_rule
      = (lit("LAYER") >> _string[boost::bind(
             &setLayer, _1, parser, boost::ref(incomplete_props))]
         >> -lit("NOSTACK")[boost::bind(&setNoStack, parser)]
         >> -(lit("NONZEROENCLOSURE")[boost::bind(&setNonZeroEnclosure, parser)]
              | (lit("PRLFORALIGNEDCUT")
                 >> +(_string >> lit("TO") >> _string))[boost::bind(
                  &setPrlForAlignedCut, _1, parser)]));

  qi::rule<std::string::const_iterator, space_type> center_to_center_rule
      = (lit("CENTERTOCENTER")
         >> +(_string >> lit("TO")
              >> _string))[boost::bind(&setCenterToCenter, _1, parser)];

  qi::rule<std::string::const_iterator, space_type> center_and_edge_rule
      = (lit("CENTERANDEDGE") >> -lit("NOPRL")[boost::bind(&setNoPrl, parser)]
         >> (+(_string >> lit("TO")
               >> _string))[boost::bind(&setCenterAndEdge, _1, parser)]);
  qi::rule<std::string::const_iterator, space_type> prl_rule
      = (lit("PRL") >> double_ >> -(string("HORIZONTAL") | string("VERTICAL"))
         >> -string("MAXXY")
         >> *(_string >> lit("TO") >> _string
              >> double_))[boost::bind(&setPRL, _1, parser, lefinReader)];
  qi::rule<std::string::const_iterator, space_type> extension_rule
      = ((lit("ENDEXTENSION") >> double_
          >> *(lit("TO") >> _string >> double_))[boost::bind(
             &setEndExtension, _1, parser, lefinReader)]
         >> -(lit("SIDEEXTENSION")
              >> +(lit("TO") >> _string >> double_))[boost::bind(
             &setSideExtension, _1, parser, lefinReader)]);

  qi::rule<std::string::const_iterator, space_type> exact_aligned_spacing_rule
      = (lit("EXACTALIGNEDSPACING")
         >> -(string("HORIZONTAL") | string("VERTICAL"))
         >> +(_string >> double_))[boost::bind(
          &setExactAlignedSpacing, _1, parser, lefinReader)];

  qi::rule<std::string::const_iterator, space_type>
      non_opposite_enclosure_spacing_rule
      = (lit("NONOPPOSITEENCLOSURESPACING")
         >> +(_string >> double_))[boost::bind(
          &setNonOppositeEnclosureSpacing, _1, parser, lefinReader)];

  qi::rule<std::string::const_iterator, space_type>
      opposite_enclosure_resize_spacing_rule
      = (lit("OPPOSITEENCLOSURERESIZESPACING")
         >> +(_string >> double_ >> double_ >> double_))[boost::bind(
          &setOppositeEnclosureResizeSpacing, _1, parser, lefinReader)];

  qi::rule<std::string::const_iterator, space_type> cut_class_rule
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

  qi::rule<std::string::const_iterator, space_type> default_rule
      = (lit("SPACINGTABLE")[boost::bind(&createDefSubRule, parser)]
         >> -(lit("DEFAULT")
              >> double_[boost::bind(&setDefault, _1, parser, lefinReader)])
         >> -lit("SAMEMASK")[boost::bind(&setSameMask, parser)]
         >> -(lit("SAMENET")[boost::bind(&setSameNet, parser)]
              | lit("SAMEMETAL")[boost::bind(&setSameMetal, parser)]
              | lit("SAMEVIA")[boost::bind(&setSameVia, parser)])
         >> -layer_rule >> -center_to_center_rule >> -center_and_edge_rule
         >> -prl_rule >> -extension_rule >> -exact_aligned_spacing_rule
         >> -non_opposite_enclosure_spacing_rule
         >> -opposite_enclosure_resize_spacing_rule >> cut_class_rule
         >> lit(";"));

  qi::rule<std::string::const_iterator, space_type> lef58_spacing_table_rule
      = (+(orthogonal_rule | default_rule));
  bool valid = qi::phrase_parse(first, last, lef58_spacing_table_rule, space)
               && first == last;
  if (!valid && parser->rule != nullptr) {
    if (!incomplete_props.empty()
        && incomplete_props.back().first == parser->rule) {
      incomplete_props.pop_back();
    }
    odb::dbTechLayerCutSpacingTableDefRule::destroy(parser->rule);
  }

  return valid;
}
}  // namespace odb::lefTechLayerCutSpacingTable

namespace odb {

bool lefTechLayerCutSpacingTableParser::parse(
    const std::string& s,
    odb::lefinReader* l,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  return lefTechLayerCutSpacingTable::parse(
      s.begin(), s.end(), this, l, incomplete_props);
}

}  // namespace odb