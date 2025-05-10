// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "label.h"

#include <QFont>
#include <memory>
#include <string>
#include <vector>

#include "odb/db.h"
#include "options.h"

namespace gui {

Label::Label(const odb::Point& pt,
             const std::string& text,
             const Painter::Anchor& anchor,
             const std::optional<int> size,
             const std::optional<std::string> name)
    : pt_(pt), text_(text), size_(size), anchor_(anchor)
{
  // update name if empty
  if (name.has_value()) {
    name_ = name.value();
  } else {
    static int label_idx = 0;
    name_ = "label" + std::to_string(label_idx++);
  }
}

////////////

LabelDescriptor::LabelDescriptor(
    const std::vector<std::unique_ptr<Label>>& labels,
    odb::dbDatabase* db)
    : labels_(labels), db_(db)
{
}

std::string LabelDescriptor::getName(std::any object) const
{
  auto label = std::any_cast<Label*>(object);
  return label->getName();
}

std::string LabelDescriptor::getTypeName() const
{
  return "Label";
}

bool LabelDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  return false;
}

void LabelDescriptor::highlight(std::any object, Painter& painter) const
{
  auto label = std::any_cast<Label*>(object);

  painter.saveState();
  const auto size = label->getSize();
  QFont font = painter.getOptions()->labelFont();
  if (size) {
    font.setPixelSize(size.value());
  }
  painter.setFont(&font);

  const auto bounds = painter.stringBoundaries(label->getPt().x(),
                                               label->getPt().y(),
                                               label->getAnchor(),
                                               label->getText());
  painter.drawRect(bounds);

  painter.restoreState();
}

Descriptor::Properties LabelDescriptor::getProperties(std::any object) const
{
  auto label = std::any_cast<Label*>(object);

  Descriptor::Properties props{
      {"Text", label->getText()},
      {"x", Property::convert_dbu(label->getPt().x(), true)},
      {"y", Property::convert_dbu(label->getPt().y(), true)}};

  props.push_back({"Size", label->getSize().value_or(-1)});
  props.push_back({"Anchor", anchorToString(label->getAnchor())});

  return props;
}

Descriptor::Editors LabelDescriptor::getEditors(std::any object) const
{
  auto label = std::any_cast<Label*>(object);
  const int dbu_per_uu_ = db_->getChip()->getBlock()->getDbUnitsPerMicron();

  std::vector<Descriptor::EditorOption> anchor_options;
  for (const auto& anc : {Painter::Anchor::BOTTOM_LEFT,
                          Painter::Anchor::BOTTOM_RIGHT,
                          Painter::Anchor::TOP_LEFT,
                          Painter::Anchor::TOP_RIGHT,
                          Painter::Anchor::CENTER,
                          Painter::Anchor::BOTTOM_CENTER,
                          Painter::Anchor::TOP_CENTER,
                          Painter::Anchor::LEFT_CENTER,
                          Painter::Anchor::RIGHT_CENTER}) {
    anchor_options.push_back({anchorToString(anc), anc});
  }

  return {{"Name", makeEditor([this, label](std::any value) {
             auto new_name = std::any_cast<const std::string>(value);
             if (new_name.empty()) {
               return false;
             }
             for (const auto& check_label : labels_) {
               if (check_label->getName() == new_name) {
                 return false;
               }
             }
             label->setName(new_name);
             return true;
           })},
          {"Text", makeEditor([this, label](std::any value) {
             auto new_text = std::any_cast<const std::string>(value);
             if (new_text.empty()) {
               return false;
             }
             label->setText(new_text);
             return true;
           })},
          {"Size", makeEditor([this, label](std::any value) {
             try {
               auto new_size = static_cast<int>(std::any_cast<double>(value));
               if (new_size <= 0) {
                 return false;
               }
               label->setSize(new_size);
             } catch (const std::bad_any_cast&) {
               return false;
             }
             return true;
           })},
          {"Anchor",
           makeEditor(
               [this, label](std::any value) {
                 auto anchor = std::any_cast<Painter::Anchor>(value);
                 label->setAnchor(anchor);
                 return true;
               },
               anchor_options)},
          {"x", makeEditor([label, dbu_per_uu_](const std::any& value) {
             return LabelDescriptor::editPoint(
                 value, dbu_per_uu_, label->getPt(), true);
           })},
          {"y", makeEditor([label, dbu_per_uu_](const std::any& value) {
             return LabelDescriptor::editPoint(
                 value, dbu_per_uu_, label->getPt(), false);
           })}};
}

bool LabelDescriptor::editPoint(std::any value,
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

std::string LabelDescriptor::anchorToString(const Painter::Anchor& anchor)
{
  switch (anchor) {
    case Painter::Anchor::BOTTOM_LEFT:
      return "bottom left";
    case Painter::Anchor::BOTTOM_RIGHT:
      return "bottom right";
    case Painter::Anchor::TOP_LEFT:
      return "top left";
    case Painter::Anchor::TOP_RIGHT:
      return "top right";
    case Painter::Anchor::CENTER:
      return "center";
    case Painter::Anchor::BOTTOM_CENTER:
      return "bottom center";
    case Painter::Anchor::TOP_CENTER:
      return "top center";
    case Painter::Anchor::LEFT_CENTER:
      return "left center";
    case Painter::Anchor::RIGHT_CENTER:
      return "right center";
  }

  return "";
}

Descriptor::Actions LabelDescriptor::getActions(std::any object) const
{
  auto label = std::any_cast<Label*>(object);

  return {{"Delete", [label]() {
             gui::Gui::get()->deleteRuler(label->getName());
             return Selected();  // unselect since this object is now gone
           }}};
}

Selected LabelDescriptor::makeSelected(std::any object) const
{
  if (auto label = std::any_cast<Label*>(&object)) {
    return Selected(*label, this);
  }
  return Selected();
}

bool LabelDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_ruler = std::any_cast<Label*>(l);
  auto r_ruler = std::any_cast<Label*>(r);

  return l_ruler->getName() < r_ruler->getName();
}

bool LabelDescriptor::getAllObjects(SelectionSet& objects) const
{
  for (auto& label : labels_) {
    objects.insert(makeSelected(label.get()));
  }
  return true;
}

}  // namespace gui
