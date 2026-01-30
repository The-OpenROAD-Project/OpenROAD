// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// This file is only used when we can't find Qt5 and are thus
// disabling the GUI.  It is not included when Qt5 is found.

#include <any>
#include <cstdio>
#include <map>
#include <optional>
#include <string>
#include <typeinfo>
#include <vector>

#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "tcl.h"

// empty gif writer class
struct GifWriter
{
};

namespace gui {

// Used by toString to convert dbu to microns
DBUToString Descriptor::Property::convert_dbu
    = [](int value, bool) { return std::to_string(value); };
StringToDBU Descriptor::Property::convert_string
    = [](const std::string& value, bool*) { return 0; };

// empty heat map class
class PinDensityDataSource
{
};

// empty heat map class
class PlacementDensityDataSource
{
};

class PowerDensityDataSource
{
};

////

Gui::Gui() : continue_after_close_(false), logger_(nullptr), db_(nullptr)
{
}

Gui* gui::Gui::get()
{
  return nullptr;
}

bool gui::Gui::enabled()
{
  return false;
}

void gui::Gui::registerRenderer(gui::Renderer*)
{
}

void gui::Gui::unregisterRenderer(gui::Renderer*)
{
}

void gui::Gui::zoomTo(const odb::Rect& rect_dbu)
{
}

void gui::Gui::redraw()
{
}

void gui::Gui::pause(int timeout)
{
}

void Gui::status(const std::string& /* message */)
{
}

void Gui::triggerAction(const std::string& /* action */)
{
}

void Renderer::redraw()
{
}

Renderer::~Renderer() = default;

SpectrumGenerator::SpectrumGenerator(double scale) : scale_(scale)
{
}

void DiscreteLegend::addLegendKey(const Painter::Color& color,
                                  const std::string& text)
{
}

void DiscreteLegend::draw(Painter& painter) const
{
}

bool Renderer::checkDisplayControl(const std::string& /* name */)
{
  return false;
}

void Renderer::addDisplayControl(
    const std::string& /* name */,
    bool /* initial_visible */,
    const DisplayControlCallback& /* setup */,
    const std::vector<std::string>& /* mutual_exclusivity */)
{
}

Renderer::Settings Renderer::getSettings()
{
  return {};
}

void Renderer::setSettings(const Renderer::Settings& /* settings */)
{
}

Selected Gui::makeSelected(const std::any& /* object */)
{
  return Selected();
}

void Gui::setSelected(const Selected& selection)
{
}

const SelectionSet& Gui::selection()
{
  static SelectionSet dummy;
  return dummy;
}

void Gui::registerDescriptor(const std::type_info& type,
                             const Descriptor* descriptor)
{
}

void Gui::unregisterDescriptor(const std::type_info& type)
{
}

const Descriptor* Gui::getDescriptor(const std::type_info& /* type */) const
{
  return nullptr;
}

void Gui::removeSelectedByType(const std::string& /* type */)
{
}

std::string Descriptor::Property::toString(const std::any& /* value */)
{
  return "";
}

// using namespace odb;
int startGui(int& argc,
             char* argv[],
             Tcl_Interp* interp,
             const std::string& script,
             bool interactive,
             bool load_settings,
             bool minimize)
{
  printf(
      "[ERROR] This code was compiled with the GUI disabled.  Please recompile "
      "with Qt5 if you want the GUI.\n");

  return 1;  // return unix err
}

void initGui(Tcl_Interp* interp,
             odb::dbDatabase* db,
             sta::dbSta* sta,
             utl::Logger* logger)
{
  // Tcl requires this to be a writable string
  std::string cmd_save_image(
      "proc save_image { args } {"
      "  utl::error GUI 4 \"Command save_image is not available as OpenROAD "
      "was not compiled with QT support.\""
      "}");
  Tcl_Eval(interp, cmd_save_image.c_str());
  std::string cmd_supported(
      "namespace eval gui {"
      "  proc supported {} {"
      "    return 0"
      "  }"
      "}");
  Tcl_Eval(interp, cmd_supported.c_str());
  std::string enabled_supported(
      "namespace eval gui {"
      "  proc enabled {} {"
      "    return 0"
      "  }"
      "}");
  Tcl_Eval(interp, enabled_supported.c_str());
}

int Gui::gifStart(const std::string& filename)
{
  return 0;
}

void Gui::gifEnd(std::optional<int> key)
{
}

void Gui::gifAddFrame(std::optional<int> key,
                      const odb::Rect& region,
                      int width_px,
                      double dbu_per_pixel,
                      std::optional<int> delay)
{
}

void Gui::deleteLabel(const std::string& name)
{
}

std::string Gui::addLabel(int x,
                          int y,
                          const std::string& text,
                          std::optional<Painter::Color> color,
                          std::optional<int> size,
                          std::optional<Painter::Anchor> anchor,
                          const std::optional<std::string>& name)
{
  return "";
}

Chart* Gui::addChart(const std::string& name,
                     const std::string& x_label,
                     const std::vector<std::string>& y_labels)
{
  return nullptr;
}

void Gui::saveImage(const std::string& filename,
                    const odb::Rect& region,
                    int width_px,
                    double dbu_per_pixel,
                    const std::map<std::string, bool>& display_settings)
{
}

void Gui::clearSelections()
{
}

int Gui::select(const std::string& type,
                const std::string& name_filter,
                const std::string& attribute,
                const std::any& value,
                bool filter_case_sensitive,
                int highlight_group)
{
  return 0;
}

void Gui::setDisplayControlsVisible(const std::string& name, bool value)
{
}

void Gui::clearHighlights(int highlight_group)
{
}

void Gui::addNetToHighlightSet(const char* name, int highlight_group)
{
}

}  // namespace gui
