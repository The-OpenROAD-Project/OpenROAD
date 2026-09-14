// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Gui's dispatch spine: the singleton, the backend slots, and the methods
// every module outside gui calls.  They forward to whichever GuiBackend is
// installed -- the Qt gui's MainWindow wrapper, or a headless viewer such as
// the web one -- so this file has no Qt in it.
//
// The rest of Gui, the Tcl command surface, is still implemented twice, in
// gui.cpp and stub.cpp.  It moves here slice by slice.

#include <any>
#include <string>
#include <typeinfo>

#include "gui/core.h"
#include "gui/descriptor_registry.h"

namespace gui {

void Gui::resetDbuConversions()
{
  Descriptor::Property::convert_dbu
      = [](int value, bool) { return std::to_string(value); };
  Descriptor::Property::convert_string
      = [](const std::string& value, bool*) { return 0; };
}

Gui* Gui::get()
{
  static Gui* singleton = new Gui();

  return singleton;
}

// Gui's constructor stays with the gif machinery in gui.cpp / stub.cpp:
// GIF holds a unique_ptr<GifWriter>, and gif.h defines non-inline free
// functions, so only one translation unit per link may include it.

bool Gui::enabled()
{
  return Gui::get()->activeBackend() != nullptr;
}

bool Gui::hasUI()
{
  // Ask the backend rather than assuming the slot it sits in implies a
  // window: HeadlessViewer is an alias of GuiBackend, so a viewer without
  // one type-checks into either slot, and every "if (!hasUI()) return;"
  // guard in gui.cpp would then fall through to a null main_window.  Asking
  // activeBackend() rather than backend_ keeps the answer about where calls
  // actually land, which is what the guards care about.
  const GuiBackend* backend = Gui::get()->activeBackend();
  return backend != nullptr && backend->hasWindow();
}

void Gui::setHeadlessViewer(HeadlessViewer* viewer)
{
  headless_viewer_ = viewer;
}

void Gui::registerRenderer(Renderer* renderer)
{
  // Symmetric with unregisterRenderer below.  Re-registering is routine --
  // DRCWidget does it from every showEvent -- and the backend ignores it
  // (DisplayControls::registerRenderer returns early for a renderer it
  // already has), so the only thing a second pass achieved was a redraw.
  if (renderers_.contains(renderer)) {
    return;
  }

  if (auto* backend = activeBackend()) {
    backend->registerRenderer(renderer);
  }

  renderers_.insert(renderer);
  redraw();
}

void Gui::unregisterRenderer(Renderer* renderer)
{
  if (!renderers_.contains(renderer)) {
    return;
  }

  if (auto* backend = activeBackend()) {
    backend->unregisterRenderer(renderer);
  }

  renderers_.erase(renderer);
  redraw();
}

void Gui::redraw()
{
  if (auto* backend = activeBackend()) {
    backend->redraw();
  }
}

void Gui::status(const std::string& message)
{
  // A backend with no status surface drops it.
  if (auto* backend = activeBackend()) {
    backend->status(message);
  }
}

void Gui::pause(int timeout)
{
  if (auto* backend = activeBackend()) {
    backend->pause(timeout);
  }
}

Selected Gui::makeSelected(const std::any& object)
{
  return DescriptorRegistry::instance()->makeSelected(object);
}

void Gui::registerDescriptor(const std::type_info& type,
                             const Descriptor* descriptor)
{
  DescriptorRegistry::instance()->registerDescriptor(type, descriptor);
}

const Descriptor* Gui::getDescriptor(const std::type_info& type) const
{
  return DescriptorRegistry::instance()->getDescriptor(type);
}

void Gui::unregisterDescriptor(const std::type_info& type)
{
  DescriptorRegistry::instance()->unregisterDescriptor(type);
}

}  // namespace gui
