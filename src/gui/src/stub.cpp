// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// This file is only used when we can't find Qt5 and are thus
// disabling the GUI.  It is not included when Qt5 is found.

#include <tcl.h>

#include <cstdio>
#include <string>
#include <vector>

#include "gui/gui.h"

namespace gui {

Gui* Gui::singleton_ = nullptr;

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
  return singleton_;
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

Renderer::~Renderer()
{
}

SpectrumGenerator::SpectrumGenerator(double scale) : scale_(scale)
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

}  // namespace gui

namespace ord {

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

}  // namespace ord
