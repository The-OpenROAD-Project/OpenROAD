// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "ruler.h"

#include <any>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace gui {

Ruler::Ruler(const odb::Point& pt0,
             const odb::Point& pt1,
             const std::string& name,
             const std::string& label)
    : pt0_(pt0), pt1_(pt1), euclidian_(true), name_(name), label_(label)
{
  // update name if empty
  if (name_.empty()) {
    static int ruler_idx = 0;
    name_ = "ruler" + std::to_string(ruler_idx++);
  }
}

bool Ruler::operator==(const Ruler& other) const
{
  return (pt0_ == other.getPt0() && pt1_ == other.getPt1())
         || (pt0_ == other.getPt1() && pt1_ == other.getPt0());
}

bool Ruler::fuzzyIntersection(const odb::Rect& region, int margin) const
{
  using PointType = boost::geometry::model::d2::point_xy<int>;
  using LineStringType = boost::geometry::model::linestring<PointType>;
  using PolygonType = boost::geometry::model::polygon<PointType>;

  LineStringType ls;
  boost::geometry::append(ls, PointType(pt0_.x(), pt0_.y()));
  if (!euclidian_) {
    const auto middle = getManhattanJoinPt();
    boost::geometry::append(ls, PointType(middle.x(), middle.y()));
  }
  boost::geometry::append(ls, PointType(pt1_.x(), pt1_.y()));
  PolygonType poly;
  boost::geometry::append(
      poly, PointType(region.xMin() - margin, region.yMin() - margin));
  boost::geometry::append(
      poly, PointType(region.xMax() + margin, region.yMin() - margin));
  boost::geometry::append(
      poly, PointType(region.xMax() + margin, region.yMax() + margin));
  boost::geometry::append(
      poly, PointType(region.xMin() - margin, region.yMax() + margin));
  boost::geometry::append(
      poly, PointType(region.xMin() - margin, region.yMin() - margin));

  return boost::geometry::intersects(ls, poly);
}

std::string Ruler::getTclCommand(double dbu_to_microns) const
{
  return "gui::add_ruler " + std::to_string(pt0_.x() / dbu_to_microns) + " "
         + std::to_string(pt0_.y() / dbu_to_microns) + " "
         + std::to_string(pt1_.x() / dbu_to_microns) + " "
         + std::to_string(pt1_.y() / dbu_to_microns) + " " + "{" + label_
         + "} {" + name_ + "} " + "{" + (euclidian_ ? "1" : "0") + "}";
}

odb::Point Ruler::getManhattanJoinPt() const
{
  return getManhattanJoinPt(pt0_, pt1_);
}

odb::Point Ruler::getManhattanJoinPt(const odb::Point& pt0,
                                     const odb::Point& pt1)
{
  return odb::Point(pt1.x(), pt0.y());
}

double Ruler::getLength() const
{
  const int x_dist = std::abs(pt0_.x() - pt1_.x());
  const int y_dist = std::abs(pt0_.y() - pt1_.y());
  if (euclidian_) {
    return std::sqrt(std::pow(x_dist, 2) + std::pow(y_dist, 2));
  }
  return x_dist + y_dist;
}

////////////

RulerDescriptor::RulerDescriptor(
    const std::vector<std::unique_ptr<Ruler>>& rulers,
    odb::dbDatabase* db)
    : rulers_(rulers), db_(db)
{
}

std::string RulerDescriptor::getName(const std::any& object) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  return ruler->getName();
}

std::string RulerDescriptor::getTypeName() const
{
  return "Ruler";
}

bool RulerDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  bbox = odb::Rect(ruler->getPt0(), ruler->getPt1());
  return true;
}

void RulerDescriptor::highlight(const std::any& object, Painter& painter) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  if (ruler->isEuclidian()) {
    painter.drawLine(ruler->getPt0(), ruler->getPt1());
  } else {
    const auto middle = ruler->getManhattanJoinPt();
    painter.drawLine(ruler->getPt0(), middle);
    painter.drawLine(middle, ruler->getPt1());
  }
}

Descriptor::Properties RulerDescriptor::getProperties(
    const std::any& object) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  return {{"Label", ruler->getLabel()},
          {"Point 0 - x", Property::convert_dbu(ruler->getPt0().x(), true)},
          {"Point 0 - y", Property::convert_dbu(ruler->getPt0().y(), true)},
          {"Point 1 - x", Property::convert_dbu(ruler->getPt1().x(), true)},
          {"Point 1 - y", Property::convert_dbu(ruler->getPt1().y(), true)},
          {"Delta x",
           Property::convert_dbu(
               std::abs(ruler->getPt1().x() - ruler->getPt0().x()), true)},
          {"Delta y",
           Property::convert_dbu(
               std::abs(ruler->getPt1().y() - ruler->getPt0().y()), true)},
          {"Length", Property::convert_dbu(ruler->getLength(), true)},
          {"Euclidian", ruler->isEuclidian()}};
}

Descriptor::Editors RulerDescriptor::getEditors(const std::any& object) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  return {{"Name", makeEditor([this, ruler](const std::any& value) {
             auto new_name = std::any_cast<const std::string>(value);
             if (new_name.empty()) {
               return false;
             }
             for (const auto& check_ruler : rulers_) {
               if (check_ruler->getName() == new_name) {
                 return false;
               }
             }
             ruler->setName(new_name);
             return true;
           })},
          {"Label", makeEditor([ruler](const std::any& value) {
             ruler->setLabel(std::any_cast<const std::string>(value));
             return true;
           })},
          {"Point 0 - x", makeEditor([ruler](const std::any& value) {
             return RulerDescriptor::editPoint(value, ruler->getPt0(), true);
           })},
          {"Point 0 - y", makeEditor([ruler](const std::any& value) {
             return RulerDescriptor::editPoint(value, ruler->getPt0(), false);
           })},
          {"Point 1 - x", makeEditor([ruler](const std::any& value) {
             return RulerDescriptor::editPoint(value, ruler->getPt1(), true);
           })},
          {"Point 1 - y", makeEditor([ruler](const std::any& value) {
             return RulerDescriptor::editPoint(value, ruler->getPt1(), false);
           })},
          {"Euclidian", makeEditor([ruler](const std::any& value) {
             bool euclidian = std::any_cast<bool>(value);
             ruler->setEuclidian(euclidian);
             return true;
           })}};
}

bool RulerDescriptor::editPoint(const std::any& value,
                                odb::Point& pt,
                                bool is_x)
{
  bool accept;
  const int new_val = Descriptor::Property::convert_string(
      std::any_cast<std::string>(value), &accept);
  if (!accept) {
    return false;
  }
  if (is_x) {
    pt.setX(new_val);
  } else {
    pt.setY(new_val);
  }
  return true;
}

Descriptor::Actions RulerDescriptor::getActions(const std::any& object) const
{
  auto ruler = std::any_cast<Ruler*>(object);

  return {{"Delete", [ruler]() {
             gui::Gui::get()->deleteRuler(ruler->getName());
             return Selected();  // unselect since this object is now gone
           }}};
}

Selected RulerDescriptor::makeSelected(const std::any& object) const
{
  if (auto ruler = std::any_cast<Ruler*>(&object)) {
    return Selected(*ruler, this);
  }
  return Selected();
}

bool RulerDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_ruler = std::any_cast<Ruler*>(l);
  auto r_ruler = std::any_cast<Ruler*>(r);

  return l_ruler->getName() < r_ruler->getName();
}

void RulerDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  for (auto& ruler : rulers_) {
    func({ruler.get(), this});
  }
}

}  // namespace gui
