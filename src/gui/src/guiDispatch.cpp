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
#include "odb/db.h"
#include "utl/Logger.h"

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

namespace {

odb::dbBlock* getBlock(odb::dbDatabase* db)
{
  if (db == nullptr) {
    return nullptr;
  }
  auto* chip = db->getChip();
  if (chip == nullptr) {
    return nullptr;
  }
  return chip->getBlock();
}

}  // namespace

void Gui::setSelected(const Selected& selection)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->setSelected(selection);
}

void Gui::removeSelectedByType(const std::string& type)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->removeSelectedByType(type);
}

void Gui::addSelectedNet(const char* name)
{
  if (!hasUI()) {
    return;
  }
  auto* block = getBlock(db_);
  if (block == nullptr) {
    return;
  }

  auto* net = block->findNet(name);
  if (net == nullptr) {
    return;
  }

  activeBackend()->addSelected(makeSelected(net));
}

void Gui::addSelectedInst(const char* name)
{
  if (!hasUI()) {
    return;
  }
  auto* block = getBlock(db_);
  if (block == nullptr) {
    return;
  }

  auto* inst = block->findInst(name);
  if (inst == nullptr) {
    return;
  }

  activeBackend()->addSelected(makeSelected(inst));
}

const SelectionSet& Gui::selection()
{
  if (!hasUI()) {
    static const SelectionSet empty_selection;
    return empty_selection;
  }
  return activeBackend()->selection();
}

const Selected& Gui::getInspectorSelection()
{
  if (!hasUI()) {
    static const Selected empty_selection;
    return empty_selection;
  }
  return activeBackend()->inspectorSelection();
}

bool Gui::anyObjectInSet(bool selection_set, odb::dbObjectType obj_type) const
{
  if (!hasUI()) {
    return false;
  }
  return activeBackend()->anyObjectInSet(selection_set, obj_type);
}

void Gui::selectHighlightConnectedInsts(bool select_flag, int highlight_group)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->selectHighlightConnectedInsts(select_flag, highlight_group);
}

void Gui::selectHighlightConnectedNets(bool select_flag,
                                       bool output,
                                       bool input,
                                       int highlight_group)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->selectHighlightConnectedNets(
      select_flag, output, input, highlight_group);
}

void Gui::selectHighlightConnectedBufferTrees(bool select_flag,
                                              int highlight_group)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->selectHighlightConnectedBufferTrees(select_flag,
                                                       highlight_group);
}

void Gui::addInstToHighlightSet(const char* name, int highlight_group)
{
  if (!hasUI()) {
    return;
  }
  auto* block = getBlock(db_);
  if (block == nullptr) {
    return;
  }

  auto* inst = block->findInst(name);
  if (inst == nullptr) {
    logger_->error(utl::GUI, 100, "No instance named {} found.", name);
    return;
  }
  SelectionSet sel_inst_set;
  sel_inst_set.insert(makeSelected(inst));
  activeBackend()->addHighlighted(sel_inst_set, highlight_group);
}

void Gui::addNetToHighlightSet(const char* name, int highlight_group)
{
  if (!hasUI()) {
    return;
  }
  auto* block = getBlock(db_);
  if (block == nullptr) {
    return;
  }

  auto* net = block->findNet(name);
  if (net == nullptr) {
    logger_->error(utl::GUI, 101, "No net named {} found.", name);
    return;
  }
  SelectionSet selection_set;
  selection_set.insert(makeSelected(net));
  activeBackend()->addHighlighted(selection_set, highlight_group);
}

int Gui::selectAt(const odb::Rect& area, bool append)
{
  if (!hasUI()) {
    return 0;
  }
  return activeBackend()->selectArea(area, append);
}

int Gui::selectNext()
{
  if (!hasUI()) {
    return 0;
  }
  return activeBackend()->selectNext();
}

int Gui::selectPrevious()
{
  if (!hasUI()) {
    return 0;
  }
  return activeBackend()->selectPrevious();
}

void Gui::animateSelection(int repeat)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->selectionAnimation(repeat);
}

void Gui::clearSelections()
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->setSelected(Selected());
}

void Gui::clearHighlights(int highlight_group)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->clearHighlighted(highlight_group);
}

}  // namespace gui
