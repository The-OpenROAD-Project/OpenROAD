// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "gui/descriptor_registry.h"

#include <any>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>

#ifdef __GNUC__
#include <cxxabi.h>
#endif

#include <cstdio>

#include "gui/gui.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace gui {

// Helper to format doubles with %g (equivalent to fmt "{:g}") without
// requiring a dependency on the fmt library.
static std::string format_g(double v)
{
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%g", v);
  return buf;
}

// Static member definitions for Descriptor::Property.
// These provide raw-integer defaults; the full GUI build overrides
// convert_dbu with a proper micron converter in MainWindow::postReadDb().
DBUToString Descriptor::Property::convert_dbu
    = [](int value, bool) { return std::to_string(value); };
StringToDBU Descriptor::Property::convert_string
    = [](const std::string& value, bool*) { return 0; };

DescriptorRegistry* DescriptorRegistry::instance()
{
  static DescriptorRegistry* registry = new DescriptorRegistry();
  return registry;
}

void DescriptorRegistry::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

void DescriptorRegistry::registerDescriptor(const std::type_info& type,
                                            const Descriptor* descriptor)
{
  descriptors_[type] = std::unique_ptr<const Descriptor>(descriptor);
}

const Descriptor* DescriptorRegistry::getDescriptor(
    const std::type_info& type) const
{
  auto it = descriptors_.find(type);
  if (it == descriptors_.end()) {
    if (logger_) {
      logger_->error(
          utl::GUI, 53, "Unable to find descriptor for: {}", type.name());
    }
    return nullptr;
  }
  return it->second.get();
}

void DescriptorRegistry::unregisterDescriptor(const std::type_info& type)
{
  descriptors_.erase(type);
}

Selected DescriptorRegistry::makeSelected(const std::any& object)
{
  if (!object.has_value()) {
    return Selected();
  }

  auto it = descriptors_.find(object.type());
  if (it != descriptors_.end()) {
    return it->second->makeSelected(object);
  }

#ifdef __GNUC__
  char* type_name
      = abi::__cxa_demangle(object.type().name(), nullptr, nullptr, nullptr);
  if (logger_) {
    logger_->warn(
        utl::GUI, 33, "No descriptor is registered for type {}.", type_name);
  }
  free(type_name);
#else
  if (logger_) {
    logger_->warn(utl::GUI,
                  112,
                  "No descriptor is registered for type {}.",
                  object.type().name());
  }
#endif
  return Selected();
}

// See class header for documentation on why these exist.
std::size_t DescriptorRegistry::TypeInfoHasher::operator()(
    const std::type_index& x) const
{
#ifdef __GLIBCXX__
  return std::hash<std::type_index>{}(x);
#else
  return std::hash<std::string_view>{}(std::string_view(x.name()));
#endif
}

bool DescriptorRegistry::TypeInfoComparator::operator()(
    const std::type_index& a,
    const std::type_index& b) const
{
#ifdef __GLIBCXX__
  return a == b;
#else
  return strcmp(a.name(), b.name()) == 0;
#endif
}

// ---- Property::toString (Qt-free) ----

std::string Descriptor::Property::toString(const std::any& value)
{
  if (auto v = std::any_cast<Selected>(&value)) {
    if (*v) {
      return v->getName();
    }
  } else if (auto v = std::any_cast<const char*>(&value)) {
    return *v;
  } else if (auto v = std::any_cast<const std::string>(&value)) {
    return *v;
  } else if (auto v = std::any_cast<int>(&value)) {
    return std::to_string(*v);
  } else if (auto v = std::any_cast<unsigned int>(&value)) {
    return std::to_string(*v);
  } else if (auto v = std::any_cast<double>(&value)) {
    return format_g(*v);
  } else if (auto v = std::any_cast<float>(&value)) {
    return format_g(*v);
  } else if (auto v = std::any_cast<bool>(&value)) {
    return *v ? "True" : "False";
  } else if (auto v = std::any_cast<odb::Rect>(&value)) {
    std::string text = "(";
    text += convert_dbu(v->xMin(), false) + ", ";
    text += convert_dbu(v->yMin(), false) + "), (";
    text += convert_dbu(v->xMax(), false) + ", ";
    text += convert_dbu(v->yMax(), false) + ")";
    return text;
  } else if (auto v = std::any_cast<odb::Point>(&value)) {
    return "(" + convert_dbu(v->x(), false) + "," + convert_dbu(v->y(), false)
           + ")";
  }

  return "<unknown>";
}

// ---- Selected methods ----

Descriptor::Properties Selected::getProperties() const
{
  Descriptor::Properties props = descriptor_->getProperties(object_);
  props.insert(props.begin(), {"Name", getName()});
  props.insert(props.begin(), {"Type", getTypeName()});
  odb::Rect bbox;
  if (getBBox(bbox)) {
    props.push_back({"BBox", bbox});
    props.push_back(
        {"BBox Width, Height",
         std::string("(") + Descriptor::Property::convert_dbu(bbox.dx(), false)
             + ", " + Descriptor::Property::convert_dbu(bbox.dy(), false)
             + ")"});
  }

  return props;
}

void Selected::highlight(Painter& painter,
                         const Painter::Color& pen,
                         int pen_width,
                         const Painter::Color& brush,
                         const Painter::Brush& brush_style) const
{
  painter.setPen(pen, true, pen_width);
  painter.setBrush(brush, brush_style);
  descriptor_->highlight(object_, painter);
}

// Selected::getActions() is defined in init_descriptors.cpp because it
// references Gui::get() / Gui::zoomTo(), which are not available in the
// lightweight gui_descriptors library.

}  // namespace gui
