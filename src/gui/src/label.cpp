// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "label.h"

#include <QFont>
#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "gui/gui.h"
#include "odb/db.h"
#include "options.h"

namespace gui {

Label::Label(const odb::Point& pt,
             const std::string& text,
             const Painter::Anchor& anchor,
             const Painter::Color& color,
             std::optional<int> size,
             std::optional<std::string> name)
    : pt_(pt), text_(text), color_(color), size_(size), anchor_(anchor)
{
  // update name if empty
  if (name.has_value()) {
    name_ = name.value();
  } else {
    static int label_idx = 0;
    name_ = "label" + std::to_string(label_idx++);
  }

  outline_.mergeInit();
}

////////////

LabelDescriptor::LabelDescriptor(
    const std::vector<std::unique_ptr<Label>>& labels,
    odb::dbDatabase* db,
    utl::Logger* logger)
    : labels_(labels), db_(db), logger_(logger)
{
}

std::string LabelDescriptor::getName(const std::any& object) const
{
  auto label = std::any_cast<Label*>(object);
  return label->getName();
}

std::string LabelDescriptor::getTypeName() const
{
  return "Label";
}

bool LabelDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  return false;
}

void LabelDescriptor::highlight(const std::any& object, Painter& painter) const
{
  auto label = std::any_cast<Label*>(object);

  if (label->getOutline().isInverted()) {
    return;
  }

  painter.drawRect(label->getOutline());
}

Descriptor::Properties LabelDescriptor::getProperties(
    const std::any& object) const
{
  auto label = std::any_cast<Label*>(object);

  Descriptor::Properties props{
      {"Text", label->getText()},
      {"x", Property::convert_dbu(label->getPt().x(), true)},
      {"y", Property::convert_dbu(label->getPt().y(), true)}};

  props.push_back({"Size", label->getSize().value_or(-1)});
  props.push_back({"Anchor", Painter::anchorToString(label->getAnchor())});
  props.push_back({"Color", Painter::colorToString(label->getColor())});

  return props;
}

Descriptor::Editors LabelDescriptor::getEditors(const std::any& object) const
{
  auto label = std::any_cast<Label*>(object);

  std::vector<Descriptor::EditorOption> anchor_options;
  for (const auto& [name, anchor] : Painter::anchors()) {
    anchor_options.push_back({name, anchor});
  }

  return {{"Name", makeEditor([this, label](const std::any& value) {
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
          {"Text", makeEditor([label](const std::any& value) {
             auto new_text = std::any_cast<const std::string>(value);
             if (new_text.empty()) {
               return false;
             }
             label->setText(new_text);
             return true;
           })},
          {"Size", makeEditor([label](const std::any& value) {
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
               [label](const std::any& value) {
                 auto anchor = std::any_cast<Painter::Anchor>(value);
                 label->setAnchor(anchor);
                 return true;
               },
               anchor_options)},
          {"Color", makeEditor([this, label](const std::any& value) {
             label->setColor(Painter::stringToColor(
                 std::any_cast<const std::string>(value), logger_));
             return true;
           })},
          {"x", makeEditor([label](const std::any& value) {
             return LabelDescriptor::editPoint(value, label->getPt(), true);
           })},
          {"y", makeEditor([label](const std::any& value) {
             return LabelDescriptor::editPoint(value, label->getPt(), false);
           })}};
}

bool LabelDescriptor::editPoint(const std::any& value,
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

Descriptor::Actions LabelDescriptor::getActions(const std::any& object) const
{
  auto label = std::any_cast<Label*>(object);

  return {{"Delete", [label]() {
             gui::Gui::get()->deleteLabel(label->getName());
             return Selected();  // unselect since this object is now gone
           }}};
}

Selected LabelDescriptor::makeSelected(const std::any& object) const
{
  if (auto label = std::any_cast<Label*>(&object)) {
    return Selected(*label, this);
  }
  return Selected();
}

bool LabelDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_label = std::any_cast<Label*>(l);
  auto r_label = std::any_cast<Label*>(r);

  return l_label->getName() < r_label->getName();
}

void LabelDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  for (auto& label : labels_) {
    func({label.get(), this});
  }
}

}  // namespace gui
