// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "gui/gui.h"

namespace odb {
class Point;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace gui {

class Label
{
 public:
  Label(const odb::Point& pt,
        const std::string& text,
        const Painter::Anchor& anchor,
        const Painter::Color& color,
        std::optional<int> size = {},
        std::optional<std::string> name = {});

  odb::Point& getPt() { return pt_; }
  odb::Point getPt() const { return pt_; }
  const std::string& getName() const { return name_; }
  void setName(const std::string& name) { name_ = name; }
  const std::string& getText() const { return text_; }
  void setText(const std::string& text) { text_ = text; }
  Painter::Anchor getAnchor() const { return anchor_; }
  void setAnchor(const Painter::Anchor& anchor) { anchor_ = anchor; }
  const std::optional<int>& getSize() const { return size_; }
  void setSize(const std::optional<int>& size) { size_ = size; }
  const Painter::Color& getColor() const { return color_; }
  void setColor(const Painter::Color& color) { color_ = color; }

  const odb::Rect& getOutline() const { return outline_; }
  void setOutline(const odb::Rect& outline) { outline_ = outline; }

 private:
  odb::Point pt_;
  std::string text_;
  Painter::Color color_;
  std::optional<int> size_;
  Painter::Anchor anchor_;

  std::string name_;

  odb::Rect outline_;
};

using Labels = std::vector<std::unique_ptr<Label>>;

class LabelDescriptor : public Descriptor
{
 public:
  LabelDescriptor(const Labels& labels,
                  odb::dbDatabase* db,
                  utl::Logger* logger);

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

  const Labels& labels_;
  odb::dbDatabase* db_;
  utl::Logger* logger_;
};

}  // namespace gui
