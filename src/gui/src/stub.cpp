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

// This file is only used when we can't find Qt5 and are thus
// disabling the GUI.  It is not included when Qt5 is found.

#include <tcl.h>

#include <cstdio>

#include "gui/gui.h"
#include "ord/OpenRoad.hh"

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

////

Gui::Gui()
    : continue_after_close_(false),
      logger_(nullptr),
      db_(nullptr),
      pin_density_heat_map_(nullptr),
      placement_density_heat_map_(nullptr)
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

class OpenRoad;
void initGui(OpenRoad* openroad)
{
  auto interp = openroad->tclInterp();
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
}

}  // namespace ord
