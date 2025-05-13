// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "gui/gui.h"

namespace odb {
class dbDatabase;
class Point;
}  // namespace odb

namespace gui {

class Ruler
{
 public:
  Ruler(const odb::Point& pt0,
        const odb::Point& pt1,
        const std::string& name = "",
        const std::string& label = "");

  odb::Point& getPt0() { return pt0_; }
  odb::Point getPt0() const { return pt0_; }
  odb::Point& getPt1() { return pt1_; }
  odb::Point getPt1() const { return pt1_; }
  odb::Point getManhattanJoinPt() const;
  static odb::Point getManhattanJoinPt(const odb::Point& pt0,
                                       const odb::Point& pt1);
  const std::string getName() const { return name_; }
  void setName(const std::string& name) { name_ = name; }
  const std::string getLabel() const { return label_; }
  void setLabel(const std::string& label) { label_ = label; }
  bool isEuclidian() const { return euclidian_; }
  void setEuclidian(bool euclidian) { euclidian_ = euclidian; }
  double getLength() const;

  std::string getTclCommand(double dbu_to_microns) const;

  bool fuzzyIntersection(const odb::Rect& region, int margin) const;

  bool operator==(const Ruler& other) const;

 private:
  odb::Point pt0_;
  odb::Point pt1_;

  bool euclidian_;

  std::string name_;
  std::string label_;
};

using Rulers = std::vector<std::unique_ptr<Ruler>>;

class RulerDescriptor : public Descriptor
{
 public:
  RulerDescriptor(const std::vector<std::unique_ptr<Ruler>>& rulers,
                  odb::dbDatabase* db);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Actions getActions(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  static bool editPoint(std::any value, odb::Point& pt, bool is_x);

  const std::vector<std::unique_ptr<Ruler>>& rulers_;
  odb::dbDatabase* db_;
};

}  // namespace gui
