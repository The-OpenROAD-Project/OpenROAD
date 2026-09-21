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
#include <optional>
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

// A width or height of zero means "size it yourself"; that is the wire format
// the Tcl image commands use, and it becomes an unset optional here so the
// backend does not have to know the convention.
std::optional<int> sizeOrAuto(int px)
{
  if (px > 0) {
    return px;
  }
  return std::nullopt;
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

void Gui::addSelectedNet(const std::string& name)
{
  if (!hasUI()) {
    return;
  }
  auto* block = getBlock(db_);
  if (block == nullptr) {
    return;
  }

  auto* net = block->findNet(name.c_str());
  if (net == nullptr) {
    return;
  }

  activeBackend()->addSelected(makeSelected(net));
}

void Gui::addSelectedInst(const std::string& name)
{
  if (!hasUI()) {
    return;
  }
  auto* block = getBlock(db_);
  if (block == nullptr) {
    return;
  }

  auto* inst = block->findInst(name.c_str());
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

void Gui::addInstToHighlightSet(const std::string& name, int highlight_group)
{
  if (!hasUI()) {
    return;
  }
  auto* block = getBlock(db_);
  if (block == nullptr) {
    return;
  }

  auto* inst = block->findInst(name.c_str());
  if (inst == nullptr) {
    logger_->error(utl::GUI, 100, "No instance named {} found.", name);
    return;
  }
  SelectionSet sel_inst_set;
  sel_inst_set.insert(makeSelected(inst));
  activeBackend()->addHighlighted(sel_inst_set, highlight_group);
}

void Gui::addNetToHighlightSet(const std::string& name, int highlight_group)
{
  if (!hasUI()) {
    return;
  }
  auto* block = getBlock(db_);
  if (block == nullptr) {
    return;
  }

  auto* net = block->findNet(name.c_str());
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

void Gui::zoomTo(const odb::Rect& rect_dbu)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->zoomTo(rect_dbu);
}

void Gui::zoomTo(const odb::Point& focus, int diameter)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->zoomTo(focus, diameter);
}

void Gui::zoomIn()
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->zoomIn();
}

void Gui::zoomIn(const odb::Point& focus_dbu)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->zoomIn(focus_dbu);
}

void Gui::zoomOut()
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->zoomOut();
}

void Gui::zoomOut(const odb::Point& focus_dbu)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->zoomOut(focus_dbu);
}

void Gui::centerAt(const odb::Point& focus_dbu)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->centerAt(focus_dbu);
}

void Gui::setResolution(double pixels_per_dbu)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->setResolution(pixels_per_dbu);
}

void Gui::fit()
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->fit();
}

std::string Gui::addLabel(int x,
                          int y,
                          const std::string& text,
                          std::optional<Painter::Color> color,
                          std::optional<int> size,
                          std::optional<Painter::Anchor> anchor,
                          const std::optional<std::string>& name)
{
  if (!hasUI()) {
    return "";
  }
  return activeBackend()->addLabel(x, y, text, color, size, anchor, name);
}

void Gui::deleteLabel(const std::string& name)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->deleteLabel(name);
}

void Gui::clearLabels()
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->clearLabels();
}

std::string Gui::addRuler(int x0,
                          int y0,
                          int x1,
                          int y1,
                          const std::string& label,
                          const std::string& name,
                          bool euclidian)
{
  if (!hasUI()) {
    return "";
  }
  return activeBackend()->addRuler(x0, y0, x1, y1, label, name, euclidian);
}

void Gui::deleteRuler(const std::string& name)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->deleteRuler(name);
}

void Gui::clearRulers()
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->clearRulers();
}

// The display-control queries are the one group that always had a headless
// path: they answered from the web viewer when no window was up.  Dispatching
// through the backend is that same fallback, without the special case -- the
// defaults here are the ones Gui used with nothing installed at all.
void Gui::setDisplayControlsVisible(const std::string& name, bool value)
{
  if (auto* backend = activeBackend()) {
    backend->setDisplayControlVisible(name, value);
  }
}

bool Gui::checkDisplayControlsVisible(const std::string& name)
{
  if (auto* backend = activeBackend()) {
    return backend->checkDisplayControlVisible(name);
  }
  return true;
}

void Gui::setDisplayControlsSelectable(const std::string& name, bool value)
{
  if (auto* backend = activeBackend()) {
    backend->setDisplayControlSelectable(name, value);
  }
}

bool Gui::checkDisplayControlsSelectable(const std::string& name)
{
  if (auto* backend = activeBackend()) {
    return backend->checkDisplayControlSelectable(name);
  }
  return false;
}

void Gui::setDisplayControlsColor(const std::string& name,
                                  const Painter::Color& color)
{
  if (auto* backend = activeBackend()) {
    backend->setDisplayControlColor(name, color);
  }
}

void Gui::saveDisplayControls()
{
  if (auto* backend = activeBackend()) {
    backend->saveDisplayControls();
  }
}

void Gui::restoreDisplayControls()
{
  if (auto* backend = activeBackend()) {
    backend->restoreDisplayControls();
  }
}

void Gui::addFocusNet(odb::dbNet* net)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->addFocusNet(net);
}

void Gui::removeFocusNet(odb::dbNet* net)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->removeFocusNet(net);
}

void Gui::clearFocusNets()
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->clearFocusNets();
}

void Gui::addRouteGuides(odb::dbNet* net)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->addRouteGuides(net);
}

void Gui::removeRouteGuides(odb::dbNet* net)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->removeRouteGuides(net);
}

void Gui::clearRouteGuides()
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->clearRouteGuides();
}

void Gui::addNetTracks(odb::dbNet* net)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->addNetTracks(net);
}

void Gui::removeNetTracks(odb::dbNet* net)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->removeNetTracks(net);
}

void Gui::clearNetTracks()
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->clearNetTracks();
}

void Gui::saveClockTreeImage(const std::string& clock_name,
                             const std::string& filename,
                             const std::string& scene,
                             int width_px,
                             int height_px)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->saveClockTreeImage(
      clock_name, filename, scene, sizeOrAuto(width_px), sizeOrAuto(height_px));
}

void Gui::saveHistogramImage(const std::string& filename,
                             const std::string& mode,
                             int width_px,
                             int height_px)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->saveHistogramImage(
      filename, mode, sizeOrAuto(width_px), sizeOrAuto(height_px));
}

}  // namespace gui
