//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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

#include "ruler.h"

#include <boost/geometry.hpp>

#include "odb/db.h"

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
  using point_t = boost::geometry::model::d2::point_xy<int>;
  using linestring_t = boost::geometry::model::linestring<point_t>;
  using polygon_t = boost::geometry::model::polygon<point_t>;

  linestring_t ls;
  boost::geometry::append(ls, point_t(pt0_.x(), pt0_.y()));
  if (!euclidian_) {
    const auto middle = getManhattanJoinPt();
    boost::geometry::append(ls, point_t(middle.x(), middle.y()));
  }
  boost::geometry::append(ls, point_t(pt1_.x(), pt1_.y()));
  polygon_t poly;
  boost::geometry::append(
      poly, point_t(region.xMin() - margin, region.yMin() - margin));
  boost::geometry::append(
      poly, point_t(region.xMax() + margin, region.yMin() - margin));
  boost::geometry::append(
      poly, point_t(region.xMax() + margin, region.yMax() + margin));
  boost::geometry::append(
      poly, point_t(region.xMin() - margin, region.yMax() + margin));
  boost::geometry::append(
      poly, point_t(region.xMin() - margin, region.yMin() - margin));

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

std::string RulerDescriptor::getName(std::any object) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  return ruler->getName();
}

std::string RulerDescriptor::getTypeName() const
{
  return "Ruler";
}

bool RulerDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  bbox = odb::Rect(ruler->getPt0(), ruler->getPt1());
  return true;
}

void RulerDescriptor::highlight(std::any object, Painter& painter) const
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

Descriptor::Properties RulerDescriptor::getProperties(std::any object) const
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

Descriptor::Editors RulerDescriptor::getEditors(std::any object) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  const int dbu_per_uu_ = db_->getChip()->getBlock()->getDbUnitsPerMicron();
  return {
      {"Name", makeEditor([this, ruler](std::any value) {
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
      {"Label", makeEditor([ruler](std::any value) {
         ruler->setLabel(std::any_cast<const std::string>(value));
         return true;
       })},
      {"Point 0 - x", makeEditor([ruler, dbu_per_uu_](const std::any& value) {
         return RulerDescriptor::editPoint(
             value, dbu_per_uu_, ruler->getPt0(), true);
       })},
      {"Point 0 - y", makeEditor([ruler, dbu_per_uu_](const std::any& value) {
         return RulerDescriptor::editPoint(
             value, dbu_per_uu_, ruler->getPt0(), false);
       })},
      {"Point 1 - x", makeEditor([ruler, dbu_per_uu_](const std::any& value) {
         return RulerDescriptor::editPoint(
             value, dbu_per_uu_, ruler->getPt1(), true);
       })},
      {"Point 1 - y", makeEditor([ruler, dbu_per_uu_](const std::any& value) {
         return RulerDescriptor::editPoint(
             value, dbu_per_uu_, ruler->getPt1(), false);
       })},
      {"Euclidian", makeEditor([ruler](const std::any& value) {
         bool euclidian = std::any_cast<bool>(value);
         ruler->setEuclidian(euclidian);
         return true;
       })}};
}

bool RulerDescriptor::editPoint(std::any value,
                                int dbu_per_uu,
                                odb::Point& pt,
                                bool is_x)
{
  double cast_value = 0;
  try {
    cast_value = std::any_cast<double>(value);
  } catch (const std::bad_any_cast&) {
    return false;
  }
  const int new_val = cast_value * dbu_per_uu;
  if (is_x) {
    pt.setX(new_val);
  } else {
    pt.setY(new_val);
  }
  return true;
}

Descriptor::Actions RulerDescriptor::getActions(std::any object) const
{
  auto ruler = std::any_cast<Ruler*>(object);

  return {{"Delete", [ruler]() {
             gui::Gui::get()->deleteRuler(ruler->getName());
             return Selected();  // unselect since this object is now gone
           }}};
}

Selected RulerDescriptor::makeSelected(std::any object) const
{
  if (auto ruler = std::any_cast<Ruler*>(&object)) {
    return Selected(*ruler, this);
  }
  return Selected();
}

bool RulerDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_ruler = std::any_cast<Ruler*>(l);
  auto r_ruler = std::any_cast<Ruler*>(r);

  return l_ruler->getName() < r_ruler->getName();
}

bool RulerDescriptor::getAllObjects(SelectionSet& objects) const
{
  for (auto& ruler : rulers_) {
    objects.insert(makeSelected(ruler.get()));
  }
  return true;
}

}  // namespace gui
