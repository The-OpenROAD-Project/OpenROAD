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

#include <cstdio>

#include "gui/gui.h"

namespace gui {

Gui* Gui::singleton_ = nullptr;

Gui* gui::Gui::get()
{
  return singleton_;
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

void gui::Gui::addCustomVisibilityControl(const std::string& name,
                                          bool initially_visible)
{
}

bool gui::Gui::checkCustomVisibilityControl(const std::string& name)
{
  return false;
}

void Gui::status(const std::string& /* message */)
{
}

Renderer::~Renderer()
{
}

Selected Gui::makeSelected(std::any /* object */, void* /* additional_data */)
{
  return Selected();
}

void Gui::setSelected(Selected selection)
{
}

void Gui::registerDescriptor(const std::type_info& type,
                        const Descriptor* descriptor)
{
}

// using namespace odb;
int startGui(int argc, char* argv[])
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
}

}  // namespace ord
