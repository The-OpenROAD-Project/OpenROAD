//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "ord/OpenRoad.hh"
#include "odb/db.h"
#include "ruler.h"

namespace gui {

Ruler::Ruler(const odb::Point& pt0, const odb::Point& pt1, const std::string& name, const std::string& label) :
    pt0_(pt0), pt1_(pt1), name_(name), label_(label)
{
  // update name if empty
  if (name_.empty()) {
    static int ruler_idx = 0;
    name_ = "ruler" + std::to_string(ruler_idx++);
  }
}

bool Ruler::operator ==(const Ruler& other) const
{
  return (pt0_ == other.getPt0() && pt1_ == other.getPt1()) ||
         (pt0_ == other.getPt1() && pt1_ == other.getPt0());
}

bool Ruler::fuzzyIntersection(const odb::Point& pt, int margin)
{
  odb::Rect ruler_area(pt0_, pt1_);
  if (ruler_area.isInverted()) {
    if (ruler_area.xMax() < ruler_area.xMin()) {
      int tmp = ruler_area.xMax();
      ruler_area.set_xhi(ruler_area.xMin());
      ruler_area.set_xlo(tmp);
    }

    if (ruler_area.yMax() < ruler_area.yMin()) {
      int tmp = ruler_area.yMax();
      ruler_area.set_yhi(ruler_area.yMin());
      ruler_area.set_ylo(tmp);
    }
  }

  ruler_area.set_xlo(ruler_area.xMin() - margin);
  ruler_area.set_ylo(ruler_area.yMin() - margin);
  ruler_area.set_xhi(ruler_area.xMax() + margin);
  ruler_area.set_yhi(ruler_area.yMax() + margin);

  return ruler_area.intersects(pt);
}

////////////

std::string RulerDescriptor::getName(std::any object) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  return ruler->getName();
}

std::string RulerDescriptor::getTypeName(std::any object) const
{
  return "Ruler";
}

bool RulerDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  bbox = odb::Rect(ruler->getPt0(), ruler->getPt1());
  return true;
}

void RulerDescriptor::highlight(std::any object,
               Painter& painter,
               void* additional_data) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  painter.drawLine(ruler->getPt0(), ruler->getPt1());
}

Descriptor::Properties RulerDescriptor::getProperties(std::any object) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  const double dbu_per_uu_ = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock()->getDbUnitsPerMicron();
  return {{"Label", ruler->getLabel()},
          {"Point 0 - x", ruler->getPt0().x() / dbu_per_uu_},
          {"Point 0 - y", ruler->getPt0().y() / dbu_per_uu_},
          {"Point 1 - x", ruler->getPt1().x() / dbu_per_uu_},
          {"Point 1 - y", ruler->getPt1().y() / dbu_per_uu_}};
}

Descriptor::Editors RulerDescriptor::getEditors(std::any object) const
{
  auto ruler = std::any_cast<Ruler*>(object);
  const int dbu_per_uu_ = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock()->getDbUnitsPerMicron();
  return {
    {"Name", makeEditor([ruler](std::any value){
      auto new_name = std::any_cast<const std::string>(value);
      if (new_name.empty()) {
        return false;
      }
      ruler->setName(new_name);
      return true;
    })},
    {"Label", makeEditor([ruler](std::any value){
      ruler->setLabel(std::any_cast<const std::string>(value));
      return true;
    })},
    {"Point 0 - x", makeEditor([ruler, dbu_per_uu_](std::any value) { return RulerDescriptor::editPoint(value, dbu_per_uu_, ruler->getPt0(), true); })},
    {"Point 0 - y", makeEditor([ruler, dbu_per_uu_](std::any value) { return RulerDescriptor::editPoint(value, dbu_per_uu_, ruler->getPt0(), false); })},
    {"Point 1 - x", makeEditor([ruler, dbu_per_uu_](std::any value) { return RulerDescriptor::editPoint(value, dbu_per_uu_, ruler->getPt1(), true); })},
    {"Point 1 - y", makeEditor([ruler, dbu_per_uu_](std::any value) { return RulerDescriptor::editPoint(value, dbu_per_uu_, ruler->getPt1(), false); })},
  };
}

bool RulerDescriptor::editPoint(std::any value, int dbu_per_uu, odb::Point& pt, bool is_x)
{
  int new_val = std::any_cast<double>(value) * dbu_per_uu;
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
    return Selected(); // unselect since this object is now gone
  }}};
}

Selected RulerDescriptor::makeSelected(std::any object, void* additional_data) const
{
  if (auto ruler = std::any_cast<Ruler*>(&object)) {
    return Selected(*ruler, this, additional_data);
  }
  return Selected();
}

bool RulerDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_ruler = std::any_cast<Ruler*>(l);
  auto r_ruler = std::any_cast<Ruler*>(r);

  return l_ruler->getName() < r_ruler->getName();
}

}  // namespace gui
