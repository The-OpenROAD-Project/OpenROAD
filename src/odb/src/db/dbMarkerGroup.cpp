///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Generator Code Begin Cpp
#include "dbMarkerGroup.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbMarkerCategory.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbSet.h"
// User Code Begin Includes
#include <boost/regex.hpp>
#include <fstream>
#include <regex>
#include <sstream>

#include "dbHashTable.hpp"
// User Code End Includes
namespace odb {
template class dbTable<_dbMarkerGroup>;

bool _dbMarkerGroup::operator==(const _dbMarkerGroup& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (source_ != rhs.source_) {
    return false;
  }
  if (*categories_tbl_ != *rhs.categories_tbl_) {
    return false;
  }
  if (categories_hash_ != rhs.categories_hash_) {
    return false;
  }

  return true;
}

bool _dbMarkerGroup::operator<(const _dbMarkerGroup& rhs) const
{
  return true;
}

void _dbMarkerGroup::differences(dbDiff& diff,
                                 const char* field,
                                 const _dbMarkerGroup& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(source_);
  DIFF_TABLE(categories_tbl_);
  DIFF_HASH_TABLE(categories_hash_);
  DIFF_END
}

void _dbMarkerGroup::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(source_);
  DIFF_OUT_TABLE(categories_tbl_);
  DIFF_OUT_HASH_TABLE(categories_hash_);

  DIFF_END
}

_dbMarkerGroup::_dbMarkerGroup(_dbDatabase* db)
{
  _name = nullptr;
  categories_tbl_ = new dbTable<_dbMarkerCategory>(
      db,
      this,
      (GetObjTbl_t) &_dbMarkerGroup::getObjectTable,
      dbMarkerCategoryObj);
  categories_hash_.setTable(categories_tbl_);
}

_dbMarkerGroup::_dbMarkerGroup(_dbDatabase* db, const _dbMarkerGroup& r)
{
  _name = r._name;
  _next_entry = r._next_entry;
  source_ = r.source_;
  categories_tbl_
      = new dbTable<_dbMarkerCategory>(db, this, *r.categories_tbl_);
  categories_hash_.setTable(categories_tbl_);
}

dbIStream& operator>>(dbIStream& stream, _dbMarkerGroup& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj.source_;
  stream >> *obj.categories_tbl_;
  stream >> obj.categories_hash_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbMarkerGroup& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj.source_;
  stream << *obj.categories_tbl_;
  stream << obj.categories_hash_;
  return stream;
}

dbObjectTable* _dbMarkerGroup::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbMarkerCategoryObj:
      return categories_tbl_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}

_dbMarkerGroup::~_dbMarkerGroup()
{
  if (_name) {
    free((void*) _name);
  }
  delete categories_tbl_;
}

// User Code Begin PrivateMethods

void _dbMarkerGroup::populatePTree(boost::property_tree::ptree& tree) const
{
  dbMarkerGroup* group = (dbMarkerGroup*) this;
  dbBlock* block = (dbBlock*) getOwner();

  const double dbus = block->getDbUnitsPerMicron();

  boost::property_tree::ptree group_tree;

  auto add_point = [dbus](boost::property_tree::ptree& tree, const Point& pt) {
    boost::property_tree::ptree pt_tree;

    pt_tree.put("x", fmt::format("{:.4f}", pt.x() / dbus));
    pt_tree.put("y", fmt::format("{:.4f}", pt.y() / dbus));

    tree.push_back(boost::property_tree::ptree::value_type("", pt_tree));
  };

  for (dbMarkerCategory* category : group->getMarkerCategorys()) {
    boost::property_tree::ptree category_tree;

    category_tree.put("name", category->getName());
    category_tree.put("description", category->getDescription());

    boost::property_tree::ptree violations_tree;
    for (dbMarker* marker : category->getMarkers()) {
      boost::property_tree::ptree violation_tree;

      if (!marker->getComment().empty()) {
        violation_tree.put("comment", marker->getComment());
      }

      violation_tree.put("type", "polygon");

      dbTechLayer* layer = marker->getTechLayer();
      if (layer != nullptr) {
        violation_tree.put("layer", layer->getName());
      }

      std::string shape_type;
      boost::property_tree::ptree shapes_tree;
      for (const dbMarker::MarkerShape& shape : marker->getShapes()) {
        boost::property_tree::ptree shape_tree;

        if (std::holds_alternative<Point>(shape)) {
          add_point(shape_tree, std::get<Point>(shape));
          shape_type = "point";
        } else if (std::holds_alternative<Line>(shape)) {
          for (const Point& pt : std::get<Line>(shape).getPoints()) {
            add_point(shape_tree, pt);
          }
          if (shape_type == "edge") {
            shape_type = "edge_pair";
          } else {
            shape_type = "edge";
          }
        } else if (std::holds_alternative<Rect>(shape)) {
          const Rect rect = std::get<Rect>(shape);
          for (const Point& pt : {rect.ll(), rect.ur()}) {
            add_point(shape_tree, pt);
          }
          shape_type = "box";
        } else {
          for (const Point& pt : std::get<Polygon>(shape).getPoints()) {
            add_point(shape_tree, pt);
          }
          shape_type = "polygon";
        }

        shapes_tree.push_back(
            boost::property_tree::ptree::value_type("", shape_tree));
      }

      violation_tree.put("type", shape_type);

      violation_tree.add_child("shape", shapes_tree);

      boost::property_tree::ptree sources_tree;
      for (dbNet* net : marker->getNets()) {
        boost::property_tree::ptree source_info_tree;

        source_info_tree.put("type", "net");
        source_info_tree.put("name", net->getName());

        sources_tree.push_back(
            boost::property_tree::ptree::value_type("", source_info_tree));
      }
      for (dbInst* inst : marker->getInsts()) {
        boost::property_tree::ptree source_info_tree;

        source_info_tree.put("type", "inst");
        source_info_tree.put("name", inst->getName());

        sources_tree.push_back(
            boost::property_tree::ptree::value_type("", source_info_tree));
      }
      for (dbITerm* iterm : marker->getITerms()) {
        boost::property_tree::ptree source_info_tree;

        source_info_tree.put("type", "iterm");
        source_info_tree.put("name", iterm->getName());

        sources_tree.push_back(
            boost::property_tree::ptree::value_type("", source_info_tree));
      }
      for (dbBTerm* bterm : marker->getBTerms()) {
        boost::property_tree::ptree source_info_tree;

        source_info_tree.put("type", "bterm");
        source_info_tree.put("name", bterm->getName());

        sources_tree.push_back(
            boost::property_tree::ptree::value_type("", source_info_tree));
      }
      if (!marker->getObstructions().empty()) {
        boost::property_tree::ptree source_info_tree;

        source_info_tree.put("type", "obstruction");

        sources_tree.push_back(
            boost::property_tree::ptree::value_type("", source_info_tree));
      }

      violation_tree.add_child("sources", sources_tree);

      violations_tree.push_back(
          boost::property_tree::ptree::value_type("", violation_tree));
    }

    category_tree.add_child("violations", violations_tree);

    group_tree.push_back(
        boost::property_tree::ptree::value_type("", category_tree));
  }

  tree.add_child(_name, group_tree);
}

void _dbMarkerGroup::writeMarkerGroupsJSON(
    const std::string& path,
    const std::set<_dbMarkerGroup*>& groups)
{
  std::set<dbMarkerGroup*> ordered_groups;
  for (_dbMarkerGroup* group : groups) {
    ordered_groups.insert((dbMarkerGroup*) group);
  }

  boost::property_tree::ptree tree;
  for (dbMarkerGroup* group : ordered_groups) {
    _dbMarkerGroup* group_ = (_dbMarkerGroup*) group;
    group_->populatePTree(tree);
  }

  try {
    boost::property_tree::json_parser::write_json(path, tree);
  } catch (const boost::property_tree::json_parser_error& e1) {
    _dbMarkerGroup* group = (_dbMarkerGroup*) *ordered_groups.begin();
    _dbBlock* block = (_dbBlock*) group->getOwner();
    utl::Logger* logger = block->getLogger();

    logger->error(utl::ODB,
                  268,
                  "Unable to open {} to write markers: {}",
                  path,
                  e1.what());
  }
}

// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbMarkerGroup - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbMarkerGroup::getName() const
{
  _dbMarkerGroup* obj = (_dbMarkerGroup*) this;
  return obj->_name;
}

void dbMarkerGroup::setSource(const std::string& source)
{
  _dbMarkerGroup* obj = (_dbMarkerGroup*) this;

  obj->source_ = source;
}

std::string dbMarkerGroup::getSource() const
{
  _dbMarkerGroup* obj = (_dbMarkerGroup*) this;
  return obj->source_;
}

dbSet<dbMarkerCategory> dbMarkerGroup::getMarkerCategorys() const
{
  _dbMarkerGroup* obj = (_dbMarkerGroup*) this;
  return dbSet<dbMarkerCategory>(obj, obj->categories_tbl_);
}

dbMarkerCategory* dbMarkerGroup::findMarkerCategory(const char* name) const
{
  _dbMarkerGroup* obj = (_dbMarkerGroup*) this;
  return (dbMarkerCategory*) obj->categories_hash_.find(name);
}

// User Code Begin dbMarkerGroupPublicMethods

void dbMarkerGroup::writeJSON(const std::string& path) const
{
  _dbMarkerGroup* obj = (_dbMarkerGroup*) this;
  _dbMarkerGroup::writeMarkerGroupsJSON(path, {obj});
}

void dbMarkerGroup::writeTR(const std::string& path) const
{
  _dbMarkerGroup* group = (_dbMarkerGroup*) this;
  _dbBlock* block_ = (_dbBlock*) group->getOwner();
  dbBlock* block = (dbBlock*) block_;

  std::ofstream report(path);

  if (!report) {
    utl::Logger* logger = block_->getLogger();

    logger->error(
        utl::ODB, 269, "Unable to open {} to write markers: {}", path);
  }

  const double dbus = block->getDbUnitsPerMicron();

  for (dbMarkerCategory* category : getMarkerCategorys()) {
    for (dbMarker* marker : category->getMarkers()) {
      report << "violation type: " << category->getName() << std::endl;
      report << "\tsrcs:";
      for (dbNet* net : marker->getNets()) {
        report << " net:" << net->getName();
      }
      for (dbInst* inst : marker->getInsts()) {
        report << " inst:" << inst->getName();
      }
      for (dbITerm* iterm : marker->getITerms()) {
        report << " iterm:" << iterm->getName();
      }
      for (dbBTerm* bterm : marker->getBTerms()) {
        report << " bterm:" << bterm->getName();
      }
      if (!marker->getObstructions().empty()) {
        report << " obstruction: ";
      }
      report << std::endl;

      if (!marker->getComment().empty()) {
        report << "\tcomment: " << marker->getComment() << std::endl;
      }

      const Rect bbox = marker->getBBox();
      report << "\tbbox = (";
      report << fmt::format("{:.4f}", bbox.xMin() / dbus) << ", ";
      report << fmt::format("{:.4f}", bbox.yMin() / dbus) << ") - (";
      report << fmt::format("{:.4f}", bbox.xMax() / dbus) << ", ";
      report << fmt::format("{:.4f}", bbox.yMax() / dbus) << ") on Layer ";
      if (marker->getTechLayer() != nullptr) {
        report << marker->getTechLayer()->getName();
      } else {
        report << "-";
      }
      report << std::endl;
    }
  }

  report.close();
}

void dbMarkerGroup::fromJSON(dbBlock* block, const std::string& path)
{
  _dbBlock* _block = (_dbBlock*) block;
  utl::Logger* logger = _block->getLogger();

  boost::property_tree::ptree tree;
  try {
    boost::property_tree::json_parser::read_json(path, tree);
  } catch (const boost::property_tree::json_parser_error& e1) {
    logger->error(
        utl::ODB, 238, "Unable to parse JSON file {}: {}", path, e1.what());
  }

  dbTech* tech = block->getTech();
  const int dbUnits = block->getDbUnitsPerMicron();

  // check if DRC key exists
  for (const auto& [name, subtree] : tree) {
    dbMarkerGroup* marker_group = block->findMarkerGroup(name.c_str());
    if (marker_group != nullptr) {
      dbMarkerGroup::destroy(marker_group);
    }
    marker_group = create(block, name.c_str());
    marker_group->setSource(path);

    for (const auto& rule : subtree) {
      auto& drc_rule = rule.second;

      const std::string violation_type = drc_rule.get<std::string>("name", "");
      const std::string violation_text
          = drc_rule.get<std::string>("description", "");
      auto violations_arr = drc_rule.get_child_optional("violations");
      if (!violations_arr) {
        logger->error(utl::ODB,
                      239,
                      "Unable to find the violations key in JSON file {}",
                      path);
      }

      dbMarkerCategory* category
          = marker_group->findMarkerCategory(violation_type.c_str());
      if (category == nullptr) {
        category
            = dbMarkerCategory::create(marker_group, violation_type.c_str());
      }
      category->setDescription(violation_text);

      for (const auto& [_, violation] : violations_arr.value()) {
        dbMarker* marker = dbMarker::create(category);

        std::string layer_str = violation.get<std::string>("layer", "-");
        const std::string shape_type = violation.get<std::string>("type", "-");
        odb::dbTechLayer* layer = nullptr;
        if (!layer_str.empty()) {
          layer = tech->findLayer(layer_str.c_str());
          if (layer == nullptr && layer_str != "-") {
            logger->warn(
                utl::ODB, 255, "Unable to find tech layer: {}", layer_str);
          }
        }
        marker->setTechLayer(layer);

        auto comment = violation.get_optional<std::string>("comment");
        if (comment) {
          marker->setComment(comment.value());
        }

        std::vector<odb::Point> shape_points;
        auto shape = violation.get_child_optional("shape");
        if (shape) {
          for (const auto& [_, pt] : shape.value()) {
            double x = pt.get<double>("x", 0.0);
            double y = pt.get<double>("y", 0.0);
            shape_points.emplace_back(x * dbUnits, y * dbUnits);
          }
        } else {
          logger->warn(utl::ODB, 256, "Unable to find shape of violation");
        }

        if (shape_type == "point") {
          marker->addShape(shape_points[0]);
        } else if (shape_type == "box") {
          marker->addShape(Rect(shape_points[0], shape_points[1]));
        } else if (shape_type == "edge") {
          marker->addShape(Line(shape_points[0], shape_points[1]));
        } else if (shape_type == "edge_pair") {
          marker->addShape(Line(shape_points[0], shape_points[1]));
          marker->addShape(Line(shape_points[2], shape_points[3]));
        } else if (shape_type == "polygon") {
          marker->addShape(shape_points);
        } else {
          logger->error(
              utl::ODB, 266, "Unable to parse violation shape: {}", shape_type);
        }

        auto sources_arr = violation.get_child_optional("sources");
        if (sources_arr) {
          for (const auto& [_, src] : sources_arr.value()) {
            std::string src_type = src.get<std::string>("type", "-");
            std::string src_name = src.get<std::string>("name", "-");

            bool src_found = false;
            if (src_type == "net") {
              odb::dbNet* net = block->findNet(src_name.c_str());
              if (net != nullptr) {
                marker->addNet(net);
                src_found = true;
              } else {
                logger->warn(utl::ODB, 257, "Unable to find net: {}", src_name);
              }
            } else if (src_type == "inst") {
              odb::dbInst* inst = block->findInst(src_name.c_str());
              if (inst != nullptr) {
                marker->addInst(inst);
                src_found = true;
              } else {
                logger->warn(
                    utl::ODB, 258, "Unable to find instance: {}", src_name);
              }
            } else if (src_type == "iterm") {
              odb::dbITerm* iterm = block->findITerm(src_name.c_str());
              if (iterm != nullptr) {
                marker->addITerm(iterm);
                src_found = true;
              } else {
                logger->warn(
                    utl::ODB, 259, "Unable to find iterm: {}", src_name);
              }
            } else if (src_type == "bterm") {
              odb::dbBTerm* bterm = block->findBTerm(src_name.c_str());
              if (bterm != nullptr) {
                marker->addBTerm(bterm);
                src_found = true;
              } else {
                logger->warn(
                    utl::ODB, 262, "Unable to find bterm: {}", src_name);
              }
            } else if (src_type == "obstruction") {
              if (layer != nullptr) {
                bool found = false;
                for (const auto obs : block->getObstructions()) {
                  auto obs_bbox = obs->getBBox();
                  if (obs_bbox->getTechLayer() == layer) {
                    odb::Rect obs_rect = obs_bbox->getBox();
                    if (obs_rect.intersects(marker->getBBox())) {
                      marker->addObstruction(obs);
                      src_found = true;
                      found = true;
                    }
                  }
                }
                if (!found) {
                  logger->warn(utl::ODB, 263, "Unable to find obstruction");
                }
              }
            } else {
              logger->warn(utl::ODB, 264, "Unknown source type: {}", src_type);
            }

            if (!src_found && !src_name.empty()) {
              logger->warn(
                  utl::ODB, 265, "Failed to add source item: {}", src_name);
            }
          }
        }
      }
    }
  }
}

void dbMarkerGroup::fromTR(dbBlock* block,
                           const char* name,
                           const std::string& path)
{
  dbMarkerGroup* marker_group = block->findMarkerGroup(name);
  if (marker_group != nullptr) {
    dbMarkerGroup::destroy(marker_group);
  }
  marker_group = create(block, name);
  marker_group->setSource(path);

  _dbBlock* _block = (_dbBlock*) block;
  utl::Logger* logger = _block->getLogger();

  std::ifstream report(path);
  if (!report.is_open()) {
    logger->error(
        utl::ODB, 30, "Unable to open TritonRoute DRC report: {}", path);
  }

  const std::regex violation_type("\\s*violation type: (.*)");
  const boost::regex srcs("\\s*srcs: (.*)");
  const std::regex comment_line("\\s*(comment|congestion information): (.*)");
  const std::regex bbox_layer("\\s*bbox = (.*) on Layer (.*)");
  const std::regex bbox_corners(
      "\\s*\\(\\s*(.*),\\s*(.*)\\s*\\)\\s*-\\s*\\(\\s*(.*),\\s*(.*)\\s*\\)");

  int line_number = 0;
  dbTech* tech = block->getTech();
  while (!report.eof()) {
    std::string line;
    std::smatch base_match;

    // type of violation
    line_number++;
    std::getline(report, line);
    if (line.empty()) {
      continue;
    }

    int violation_line_number = line_number;
    std::string type;
    if (std::regex_match(line, base_match, violation_type)) {
      type = base_match[1].str();
    } else {
      logger->error(utl::ODB,
                    55,
                    "Unable to parse line as violation type (line: {}): {}",
                    line_number,
                    line);
    }

    // sources of violation
    line_number++;
    int source_line_number = line_number;
    std::getline(report, line);
    std::string sources;
    boost::smatch sources_match;
    if (boost::regex_match(line, sources_match, srcs)) {
      sources = sources_match[1].str();
    } else {
      logger->error(utl::ODB,
                    101,
                    "Unable to parse line as violation source (line: {}): {}",
                    line_number,
                    line);
    }

    line_number++;
    std::getline(report, line);
    std::string comment_information;

    // comment information (optional)
    if (std::regex_match(line, base_match, comment_line)) {
      comment_information = base_match[2].str();
      line_number++;
      std::getline(report, line);
    }

    // bounding box and layer
    if (!std::regex_match(line, base_match, bbox_layer)) {
      logger->error(utl::ODB,
                    223,
                    "Unable to parse line as violation location (line: {}): {}",
                    line_number,
                    line);
    }

    std::string bbox = base_match[1].str();
    odb::dbTechLayer* layer = tech->findLayer(base_match[2].str().c_str());
    if (layer == nullptr && base_match[2].str() != "-") {
      logger->warn(utl::ODB,
                   224,
                   "Unable to find tech layer (line: {}): {}",
                   line_number,
                   base_match[2].str());
    }

    odb::Rect rect;
    if (std::regex_match(bbox, base_match, bbox_corners)) {
      try {
        rect.set_xlo(std::stod(base_match[1].str())
                     * block->getDbUnitsPerMicron());
        rect.set_ylo(std::stod(base_match[2].str())
                     * block->getDbUnitsPerMicron());
        rect.set_xhi(std::stod(base_match[3].str())
                     * block->getDbUnitsPerMicron());
        rect.set_yhi(std::stod(base_match[4].str())
                     * block->getDbUnitsPerMicron());
      } catch (std::invalid_argument&) {
        logger->error(utl::ODB,
                      225,
                      "Unable to parse bounding box (line: {}): {}",
                      line_number,
                      bbox);
      } catch (std::out_of_range&) {
        logger->error(utl::ODB,
                      226,
                      "Unable to parse bounding box (line: {}): {}",
                      line_number,
                      bbox);
      }
    } else {
      logger->error(utl::ODB,
                    227,
                    "Unable to parse bounding box (line: {}): {}",
                    line_number,
                    bbox);
    }

    std::stringstream srcs_stream(sources);
    std::string single_source;
    std::string comment;

    dbMarkerCategory* category = marker_group->findMarkerCategory(type.c_str());
    if (category == nullptr) {
      category = dbMarkerCategory::create(marker_group, type.c_str());
    }

    dbMarker* marker = dbMarker::create(category);
    marker->setTechLayer(layer);
    marker->setLineNumber(violation_line_number);
    marker->addShape(rect);

    // split sources list
    while (getline(srcs_stream, single_source, ' ')) {
      if (single_source.empty()) {
        continue;
      }

      auto ident = single_source.find(':');
      std::string item_type = single_source.substr(0, ident);
      std::string item_name = single_source.substr(ident + 1);

      bool src_found = false;
      if (item_type == "net") {
        dbNet* net = block->findNet(item_name.c_str());
        if (net != nullptr) {
          marker->addNet(net);
          src_found = true;
        } else {
          logger->warn(utl::ODB,
                       234,
                       "Unable to find net (line: {}): {}",
                       source_line_number,
                       item_name);
        }
      } else if (item_type == "inst") {
        dbInst* inst = block->findInst(item_name.c_str());
        if (inst != nullptr) {
          marker->addInst(inst);
          src_found = true;
        } else {
          logger->warn(utl::ODB,
                       235,
                       "Unable to find instance (line: {}): {}",
                       source_line_number,
                       item_name);
        }
      } else if (item_type == "iterm") {
        dbITerm* iterm = block->findITerm(item_name.c_str());
        if (iterm != nullptr) {
          marker->addITerm(iterm);
          src_found = true;
        } else {
          logger->warn(utl::ODB,
                       236,
                       "Unable to find iterm (line: {}): {}",
                       source_line_number,
                       item_name);
        }
      } else if (item_type == "bterm") {
        dbBTerm* bterm = block->findBTerm(item_name.c_str());
        if (bterm != nullptr) {
          marker->addBTerm(bterm);
          src_found = true;
        } else {
          logger->warn(utl::ODB,
                       237,
                       "Unable to find bterm (line: {}): {}",
                       source_line_number,
                       item_name);
        }
      } else if (item_type == "obstruction") {
        bool found = false;
        if (layer != nullptr) {
          for (const auto obs : block->getObstructions()) {
            auto obs_bbox = obs->getBBox();
            if (obs_bbox->getTechLayer() == layer) {
              odb::Rect obs_rect = obs_bbox->getBox();
              if (obs_rect.intersects(rect)) {
                marker->addObstruction(obs);
                src_found = true;
                found = true;
              }
            }
          }
        }
        if (!found) {
          logger->warn(utl::ODB,
                       232,
                       "Unable to find obstruction (line: {})",
                       source_line_number);
        }
      } else {
        logger->warn(utl::ODB,
                     233,
                     "Unknown source type (line: {}): {}",
                     source_line_number,
                     item_type);
      }

      if (!src_found && !item_name.empty()) {
        comment += single_source + " ";
      }
    }

    comment += comment_information;
    marker->setComment(comment);
  }

  report.close();
}

dbMarkerGroup* dbMarkerGroup::create(dbBlock* block, const char* name)
{
  _dbBlock* db_block = (_dbBlock*) block;

  if (db_block->_marker_group_hash.hasMember(name)) {
    return nullptr;
  }

  _dbMarkerGroup* group = db_block->_marker_group_tbl->create();

  group->_name = strdup(name);
  ZALLOCATED(group->_name);

  db_block->_marker_group_hash.insert(group);

  return (dbMarkerGroup*) group;
}

void dbMarkerGroup::destroy(dbMarkerGroup* group)
{
  _dbMarkerGroup* _group = (_dbMarkerGroup*) group;
  _dbBlock* block = (_dbBlock*) _group->getOwner();
  block->_marker_group_hash.remove(_group);
  block->_marker_group_tbl->destroy(_group);
}

// User Code End dbMarkerGroupPublicMethods
}  // namespace odb
// Generator Code End Cpp