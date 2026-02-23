// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbMarker.h"

#include <cstdint>
#include <cstring>
#include <string>

#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbITerm.h"
#include "dbInst.h"
#include "dbMarkerCategory.h"
#include "dbNet.h"
#include "dbObstruction.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbVector.h"
#include "odb/db.h"
// User Code Begin Includes
#include <fstream>
#include <set>
#include <variant>
#include <vector>

#include "dbChip.h"
#include "dbCore.h"
#include "odb/dbChipCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/geom.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbMarker>;

bool _dbMarker::operator==(const _dbMarker& rhs) const
{
  if (flags_.visited != rhs.flags_.visited) {
    return false;
  }
  if (flags_.visible != rhs.flags_.visible) {
    return false;
  }
  if (flags_.waived != rhs.flags_.waived) {
    return false;
  }
  if (parent_ != rhs.parent_) {
    return false;
  }
  if (layer_ != rhs.layer_) {
    return false;
  }
  if (comment_ != rhs.comment_) {
    return false;
  }
  if (line_number_ != rhs.line_number_) {
    return false;
  }

  // User Code Begin ==
  if (shapes_ != rhs.shapes_) {
    return false;
  }
  // User Code End ==
  return true;
}

bool _dbMarker::operator<(const _dbMarker& rhs) const
{
  // User Code Begin <
  if (sources_ >= rhs.sources_) {
    return false;
  }
  if (shapes_ >= rhs.shapes_) {
    return false;
  }
  // User Code End <
  return true;
}

_dbMarker::_dbMarker(_dbDatabase* db)
{
  flags_ = {};
  line_number_ = -1;
  // User Code Begin Constructor
  flags_.visible = true;
  // User Code End Constructor
}

dbIStream& operator>>(dbIStream& stream, _dbMarker& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.parent_;
  stream >> obj.layer_;
  stream >> obj.comment_;
  stream >> obj.line_number_;
  // User Code Begin >>
  // handle shapes
  int item_count;
  stream >> item_count;
  for (std::size_t i = 0; i < item_count; i++) {
    std::string db_type;
    stream >> db_type;
    uint32_t db_id;
    stream >> db_id;

    obj.sources_.emplace(dbObject::getType(db_type.c_str(), obj.getLogger()),
                         db_id);
  }

  stream >> item_count;
  for (std::size_t i = 0; i < item_count; i++) {
    _dbMarker::ShapeType type;
    stream >> type;

    switch (type) {
      case _dbMarker::ShapeType::kPoint: {
        Point pt;
        stream >> pt;
        obj.shapes_.emplace_back(pt);
        break;
      }
      case _dbMarker::ShapeType::kLine: {
        Line l;
        stream >> l;
        obj.shapes_.emplace_back(l);
        break;
      }
      case _dbMarker::ShapeType::kRect: {
        Rect r;
        stream >> r;
        obj.shapes_.emplace_back(r);
        break;
      }
      case _dbMarker::ShapeType::kPolygon: {
        Polygon p;
        stream >> p;
        obj.shapes_.emplace_back(p);
        break;
      }
    }
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbMarker& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.parent_;
  stream << obj.layer_;
  stream << obj.comment_;
  stream << obj.line_number_;
  // User Code Begin <<
  // handle sources
  stream << static_cast<int>(obj.sources_.size());
  for (const auto& [db_type, dbid] : obj.sources_) {
    stream << std::string(dbObject::getTypeName(db_type));
    stream << dbid;
  }

  // handle shapes
  stream << static_cast<int>(obj.shapes_.size());
  for (const dbMarker::MarkerShape& shape : obj.shapes_) {
    if (std::holds_alternative<Point>(shape)) {
      stream << _dbMarker::ShapeType::kPoint;
      stream << std::get<Point>(shape);
    } else if (std::holds_alternative<Line>(shape)) {
      stream << _dbMarker::ShapeType::kLine;
      stream << std::get<Line>(shape);
    } else if (std::holds_alternative<Rect>(shape)) {
      stream << _dbMarker::ShapeType::kRect;
      stream << std::get<Rect>(shape);
    } else {
      stream << _dbMarker::ShapeType::kPolygon;
      stream << std::get<Polygon>(shape);
    }
  }
  // User Code End <<
  return stream;
}

void _dbMarker::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["comment"].add(comment_);
  info.children["sources"].add(sources_);
  info.children["shapes"].add(shapes_);
  // User Code End collectMemInfo
}

// User Code Begin PrivateMethods

_dbBlock* _dbMarker::getBlock() const
{
  dbMarker* marker = (dbMarker*) this;
  _dbMarkerCategory* category = (_dbMarkerCategory*) marker->getCategory();
  return category->getBlock();
}

_dbChip* _dbMarker::getChip() const
{
  dbMarker* marker = (dbMarker*) this;
  _dbMarkerCategory* category = (_dbMarkerCategory*) marker->getCategory();
  return category->getChip();
}

void _dbMarker::writeTR(std::ofstream& report) const
{
  dbBlock* block = (dbBlock*) getBlock();
  const double dbus = block->getDbUnitsPerMicron();

  dbMarker* marker = (dbMarker*) this;

  report << "violation type: " << marker->getCategory()->getName() << '\n';
  report << "\tsrcs:";
  for (dbObject* src : marker->getSources()) {
    switch (src->getObjectType()) {
      case dbNetObj:
        report << " net:" << static_cast<dbNet*>(src)->getName();
        break;
      case dbInstObj:
        report << " inst:" << static_cast<dbInst*>(src)->getName();
        break;
      case dbITermObj:
        report << " iterm:" << static_cast<dbITerm*>(src)->getName();
        break;
      case dbBTermObj:
        report << " bterm:" << static_cast<dbBTerm*>(src)->getName();
        break;
      case dbObstructionObj:
        report << " obstruction: ";
        break;
      default:
        getLogger()->error(
            utl::ODB, 295, "Unsupported object type: {}", src->getTypeName());
    }
  }
  report << '\n';

  if (!marker->getComment().empty()) {
    report << "\tcomment: " << marker->getComment() << '\n';
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
  report << '\n';
}

void _dbMarker::populatePTree(_dbMarkerCategory::PropertyTree& tree) const
{
  dbBlock* block = (dbBlock*) getBlock();
  dbMarker* marker = (dbMarker*) this;

  _dbMarkerCategory::PropertyTree marker_tree;

  const double dbus = block->getDbUnitsPerMicron();
  auto add_point = [dbus](_dbMarkerCategory::PropertyTree& tree,
                          const Point& pt) {
    _dbMarkerCategory::PropertyTree pt_tree;

    pt_tree.put("x", fmt::format("{:.4f}", pt.x() / dbus));
    pt_tree.put("y", fmt::format("{:.4f}", pt.y() / dbus));

    tree.push_back(_dbMarkerCategory::PropertyTree::value_type("", pt_tree));
  };

  if (!marker->getComment().empty()) {
    marker_tree.put("comment", marker->getComment());
  }

  marker_tree.put("visited", marker->isVisited());
  marker_tree.put("visible", marker->isVisible());
  marker_tree.put("waived", marker->isWaived());

  if (marker->getLineNumber() > 0) {
    marker_tree.put("line_number", marker->getLineNumber());
  }

  dbTechLayer* layer = marker->getTechLayer();
  if (layer != nullptr) {
    marker_tree.put("layer", layer->getName());
  }

  _dbMarkerCategory::PropertyTree shapes_tree;
  for (const dbMarker::MarkerShape& shape : marker->getShapes()) {
    _dbMarkerCategory::PropertyTree shape_tree;
    _dbMarkerCategory::PropertyTree point_tree;

    if (std::holds_alternative<Point>(shape)) {
      shape_tree.put("type", "point");
      add_point(point_tree, std::get<Point>(shape));
    } else if (std::holds_alternative<Line>(shape)) {
      shape_tree.put("type", "line");
      for (const Point& pt : std::get<Line>(shape).getPoints()) {
        add_point(point_tree, pt);
      }
    } else if (std::holds_alternative<Rect>(shape)) {
      shape_tree.put("type", "box");
      const Rect rect = std::get<Rect>(shape);
      for (const Point& pt : {rect.ll(), rect.ur()}) {
        add_point(point_tree, pt);
      }
    } else {
      shape_tree.put("type", "polygon");
      for (const Point& pt : std::get<Polygon>(shape).getPoints()) {
        add_point(point_tree, pt);
      }
    }

    shape_tree.add_child("points", point_tree);

    shapes_tree.push_back(
        _dbMarkerCategory::PropertyTree::value_type("", shape_tree));
  }

  marker_tree.add_child("shape", shapes_tree);

  _dbMarkerCategory::PropertyTree sources_tree;
  for (dbObject* src : marker->getSources()) {
    _dbMarkerCategory::PropertyTree source_info_tree;

    switch (src->getObjectType()) {
      case dbNetObj:
        source_info_tree.put("type", "net");
        source_info_tree.put("name", static_cast<dbNet*>(src)->getName());
        break;
      case dbInstObj:
        source_info_tree.put("type", "inst");
        source_info_tree.put("name", static_cast<dbInst*>(src)->getName());
        break;
      case dbITermObj:
        source_info_tree.put("type", "iterm");
        source_info_tree.put("name", static_cast<dbITerm*>(src)->getName());
        break;
      case dbBTermObj:
        source_info_tree.put("type", "bterm");
        source_info_tree.put("name", static_cast<dbBTerm*>(src)->getName());
        break;
      case dbObstructionObj:
        source_info_tree.put("type", "obstruction");
        break;
      default:
        getLogger()->error(
            utl::ODB, 278, "Unsupported object type: {}", src->getTypeName());
    }

    sources_tree.push_back(
        _dbMarkerCategory::PropertyTree::value_type("", source_info_tree));
  }

  marker_tree.add_child("sources", sources_tree);

  tree.push_back(_dbMarkerCategory::PropertyTree::value_type("", marker_tree));
}

void _dbMarker::fromPTree(const _dbMarkerCategory::PropertyTree& tree)
{
  dbBlock* block = (dbBlock*) getBlock();
  const int dbus = block->getDbUnitsPerMicron();

  const auto comment = tree.get_optional<std::string>("comment");
  if (comment) {
    comment_ = comment.value();
  }

  const auto visited = tree.get_optional<bool>("visited");
  if (visited) {
    flags_.visited = visited.value();
  }
  const auto visible = tree.get_optional<bool>("visible");
  if (visible) {
    flags_.visible = visible.value();
  }
  const auto waived = tree.get_optional<bool>("waived");
  if (waived) {
    flags_.waived = waived.value();
  }

  const auto line_number = tree.get_optional<int>("line_number");
  if (line_number) {
    line_number_ = line_number.value();
  }

  const auto layer_name = tree.get_optional<std::string>("layer");
  dbTechLayer* layer = nullptr;
  if (layer_name) {
    dbTech* tech = block->getTech();
    layer = tech->findLayer(layer_name.value().c_str());
    if (layer == nullptr) {
      getLogger()->warn(
          utl::ODB, 255, "Unable to find tech layer: {}", layer_name.value());
    } else {
      layer_ = layer->getId();
    }
  }

  const auto shape = tree.get_child_optional("shape");
  if (shape) {
    for (const auto& [_, shape] : shape.value()) {
      const auto shape_type = shape.get_optional<std::string>("type");
      const auto points = shape.get_child_optional("points");
      if (!shape_type || !points) {
        getLogger()->warn(utl::ODB, 266, "Unable to process violation shape");
        continue;
      }

      std::vector<Point> pts;
      bool valid_points = true;
      for (const auto& [_, pt] : points.value()) {
        const auto x = pt.get_optional<double>("x");
        const auto y = pt.get_optional<double>("y");
        if (!x || !y) {
          valid_points = false;
          break;
        }
        pts.emplace_back(dbus * x.value(), dbus * y.value());
      }

      if (!valid_points) {
        getLogger()->warn(utl::ODB,
                          294,
                          "Unable to process violation shape {} points",
                          shape_type.value());
      }

      if (shape_type.value() == "point") {
        shapes_.emplace_back(pts[0]);
      } else if (shape_type.value() == "line") {
        shapes_.emplace_back(Line(pts[0], pts[1]));
      } else if (shape_type.value() == "box") {
        shapes_.emplace_back(Rect(pts[0], pts[1]));
      } else if (shape_type.value() == "polygon") {
        shapes_.emplace_back(Polygon(pts));
      } else {
        getLogger()->warn(utl::ODB,
                          256,
                          "Unable to find shape of violation: {}",
                          shape_type.value());
      }
    }
  }

  dbMarker* marker = (dbMarker*) this;

  auto sources = tree.get_child_optional("sources");
  if (sources) {
    for (const auto& [ignore, source] : sources.value()) {
      const auto src_type = source.get_optional<std::string>("type");

      if (!src_type) {
        getLogger()->warn(utl::ODB, 296, "Unable to process violation source");
        continue;
      }

      const auto src_name = source.get_optional<std::string>("name");

      bool src_found = false;
      if (src_type.value() == "net") {
        odb::dbNet* net = block->findNet(src_name.value().c_str());
        if (net != nullptr) {
          marker->addSource(net);
          src_found = true;
        } else {
          getLogger()->warn(
              utl::ODB, 257, "Unable to find net: {}", src_name.value());
        }
      } else if (src_type.value() == "inst") {
        odb::dbInst* inst = block->findInst(src_name.value().c_str());
        if (inst != nullptr) {
          marker->addSource(inst);
          src_found = true;
        } else {
          getLogger()->warn(
              utl::ODB, 258, "Unable to find instance: {}", src_name.value());
        }
      } else if (src_type.value() == "iterm") {
        odb::dbITerm* iterm = block->findITerm(src_name.value().c_str());
        if (iterm != nullptr) {
          marker->addSource(iterm);
          src_found = true;
        } else {
          getLogger()->warn(
              utl::ODB, 259, "Unable to find iterm: {}", src_name.value());
        }
      } else if (src_type.value() == "bterm") {
        odb::dbBTerm* bterm = block->findBTerm(src_name.value().c_str());
        if (bterm != nullptr) {
          marker->addSource(bterm);
          src_found = true;
        } else {
          getLogger()->warn(
              utl::ODB, 262, "Unable to find bterm: {}", src_name.value());
        }
      } else if (src_type.value() == "obstruction") {
        src_found = marker->addObstructionFromBlock(block);
      } else {
        getLogger()->warn(
            utl::ODB, 264, "Unknown source type: {}", src_type.value());
      }

      if (!src_found && !src_name) {
        getLogger()->warn(
            utl::ODB, 265, "Failed to add source item: {}", src_name.value());
      }
    }
  }
}

// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbMarker - Methods
//
////////////////////////////////////////////////////////////////////

void dbMarker::setComment(const std::string& comment)
{
  _dbMarker* obj = (_dbMarker*) this;

  obj->comment_ = comment;
}

std::string dbMarker::getComment() const
{
  _dbMarker* obj = (_dbMarker*) this;
  return obj->comment_;
}

void dbMarker::setLineNumber(int line_number)
{
  _dbMarker* obj = (_dbMarker*) this;

  obj->line_number_ = line_number;
}

int dbMarker::getLineNumber() const
{
  _dbMarker* obj = (_dbMarker*) this;
  return obj->line_number_;
}

void dbMarker::setVisited(bool visited)
{
  _dbMarker* obj = (_dbMarker*) this;

  obj->flags_.visited = visited;
}

bool dbMarker::isVisited() const
{
  _dbMarker* obj = (_dbMarker*) this;

  return obj->flags_.visited;
}

void dbMarker::setVisible(bool visible)
{
  _dbMarker* obj = (_dbMarker*) this;

  obj->flags_.visible = visible;
}

bool dbMarker::isVisible() const
{
  _dbMarker* obj = (_dbMarker*) this;

  return obj->flags_.visible;
}

void dbMarker::setWaived(bool waived)
{
  _dbMarker* obj = (_dbMarker*) this;

  obj->flags_.waived = waived;
}

bool dbMarker::isWaived() const
{
  _dbMarker* obj = (_dbMarker*) this;

  return obj->flags_.waived;
}

// User Code Begin dbMarkerPublicMethods

std::string dbMarker::getName() const
{
  _dbMarker* obj = (_dbMarker*) this;

  dbTechLayer* layer = getTechLayer();
  std::string name;
  if (layer != nullptr) {
    name += "Layer: " + layer->getName();
  }
  std::string sources;
  for (dbObject* src : getSources()) {
    if (!sources.empty()) {
      sources += ", ";
    }
    switch (src->getObjectType()) {
      case dbNetObj:
        sources += static_cast<dbNet*>(src)->getName();
        break;
      case dbInstObj:
        sources += static_cast<dbInst*>(src)->getName();
        break;
      case dbITermObj:
        sources += static_cast<dbITerm*>(src)->getName();
        break;
      case dbBTermObj:
        sources += static_cast<dbBTerm*>(src)->getName();
        break;
      case dbObstructionObj:
        sources += "obstruction";
        break;
      case dbChipInstObj:
        sources += static_cast<dbChipInst*>(src)->getName();
        break;
      case dbChipConnObj: {
        const dbChipConn* conn = static_cast<dbChipConn*>(src);
        sources += fmt::format(
            "{}:{}", conn->getParentChip()->getName(), conn->getName());
        break;
      }
      case dbChipRegionInstObj: {
        const dbChipRegionInst* region = static_cast<dbChipRegionInst*>(src);
        sources += fmt::format("{}.regions.{}",
                               region->getChipInst()->getName(),
                               region->getChipRegion()->getName());
        break;
      }
      default:
        obj->getLogger()->error(
            utl::ODB, 290, "Unsupported object type: {}", src->getTypeName());
    }
  }
  if (!name.empty() && !sources.empty()) {
    name += " ";
  }
  name += sources;

  if (name.empty()) {
    name = getCategory()->getName();
    name += " " + std::to_string(obj->getId());
  }

  return name;
}

dbMarkerCategory* dbMarker::getCategory() const
{
  _dbMarker* marker = (_dbMarker*) this;
  return (dbMarkerCategory*) marker->getOwner();
}

std::vector<dbMarker::MarkerShape> dbMarker::getShapes() const
{
  _dbMarker* marker = (_dbMarker*) this;
  return marker->shapes_;
}

void dbMarker::addShape(const Point& pt)
{
  _dbMarker* marker = (_dbMarker*) this;
  marker->shapes_.emplace_back(pt);
}

void dbMarker::addShape(const Line& line)
{
  _dbMarker* marker = (_dbMarker*) this;
  marker->shapes_.emplace_back(line);
}

void dbMarker::addShape(const Rect& rect)
{
  _dbMarker* marker = (_dbMarker*) this;
  marker->shapes_.emplace_back(rect);
}

void dbMarker::addShape(const Polygon& polygon)
{
  _dbMarker* marker = (_dbMarker*) this;
  marker->shapes_.emplace_back(polygon);
}

void dbMarker::setTechLayer(dbTechLayer* layer)
{
  _dbMarker* marker = (_dbMarker*) this;
  if (layer == nullptr) {
    marker->layer_ = 0;
  } else {
    _dbTechLayer* _layer = (_dbTechLayer*) layer;
    marker->layer_ = _layer->getId();
  }
}

void dbMarker::addSource(dbObject* obj)
{
  if (obj == nullptr) {
    return;
  }
  _dbMarker* marker = (_dbMarker*) this;
  marker->sources_.emplace(obj->getObjectType(), obj->getId());
}

bool dbMarker::addObstructionFromBlock(dbBlock* block)
{
  if (block == nullptr) {
    return false;
  }

  const Rect bbox = getBBox();

  _dbMarker* marker = (_dbMarker*) this;
  bool found = false;
  for (const auto obs : block->getObstructions()) {
    auto obs_bbox = obs->getBBox();
    if (obs_bbox->getTechLayer()->getId() == marker->layer_) {
      odb::Rect obs_rect = obs_bbox->getBox();
      if (obs_rect.intersects(bbox)) {
        addSource(obs);
        found = true;
      }
    }
  }
  if (!found) {
    marker->getLogger()->warn(utl::ODB, 263, "Unable to find obstruction");
  }

  return found;
}

dbTechLayer* dbMarker::getTechLayer() const
{
  _dbMarker* marker = (_dbMarker*) this;
  if (marker->layer_ == 0) {
    return nullptr;
  }

  dbBlock* block = (dbBlock*) marker->getBlock();
  _dbTech* tech = (_dbTech*) block->getTech();
  return (dbTechLayer*) tech->layer_tbl_->getPtr(marker->layer_);
}

Rect dbMarker::getBBox() const
{
  Rect bbox;
  bbox.mergeInit();

  for (const MarkerShape& shape : getShapes()) {
    if (std::holds_alternative<Point>(shape)) {
      Point pt = std::get<Point>(shape);
      bbox.merge(Rect(pt, pt));
    } else if (std::holds_alternative<Line>(shape)) {
      for (const Point& pt : std::get<Line>(shape).getPoints()) {
        bbox.merge(Rect(pt, pt));
      }
    } else if (std::holds_alternative<Rect>(shape)) {
      bbox.merge(std::get<Rect>(shape));
    } else {
      bbox.merge(std::get<Polygon>(shape).getEnclosingRect());
    }
  }

  return bbox;
}

std::set<dbObject*> dbMarker::getSources() const
{
  _dbMarker* marker = (_dbMarker*) this;
  _dbBlock* block = marker->getBlock();
  _dbChip* chip = marker->getChip();

  std::set<dbObject*> objs;
  if (block) {
    for (const auto& [db_type, id] : marker->sources_) {
      dbObjectTable* table = block->getObjectTable(db_type);
      if (table != nullptr && table->validObject(id)) {
        objs.insert(table->getObject(id));
      }
    }
  } else {
    for (const auto& [db_type, id] : marker->sources_) {
      dbObjectTable* table = chip->getObjectTable(db_type);
      if (table != nullptr && table->validObject(id)) {
        objs.insert(table->getObject(id));
      }
    }
  }
  return objs;
}

dbMarker* dbMarker::create(dbMarkerCategory* category)
{
  _dbMarkerCategory* _category = (_dbMarkerCategory*) category;

  if (_category->hasMaxMarkerLimit()
      && _category->marker_tbl_->size() >= _category->max_markers_) {
    return nullptr;
  }

  _dbMarker* marker = _category->marker_tbl_->create();

  _dbChip* chip = marker->getChip();
  for (auto cb : chip->callbacks_) {
    cb->inDbMarkerCreate((dbMarker*) marker);
  }

  return (dbMarker*) marker;
}

void dbMarker::destroy(dbMarker* marker)
{
  _dbMarker* _marker = (_dbMarker*) marker;

  _dbChip* chip = _marker->getChip();
  for (auto cb : chip->callbacks_) {
    cb->inDbMarkerDestroy(marker);
  }

  _dbMarkerCategory* category = (_dbMarkerCategory*) _marker->getOwner();
  category->marker_tbl_->destroy(_marker);
}

dbIStream& operator>>(dbIStream& stream, _dbMarker::ShapeType& obj)
{
  int type;
  stream >> type;
  obj = (_dbMarker::ShapeType) type;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbMarker::ShapeType& obj)
{
  stream << (int) obj;
  return stream;
}

// User Code End dbMarkerPublicMethods
}  // namespace odb
   // Generator Code End Cpp
