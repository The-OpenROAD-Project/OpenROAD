// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Gui's dispatch spine: the singleton, the backend slots, and the methods
// every module outside gui calls.  They forward to whichever GuiBackend is
// installed -- the Qt gui's MainWindow wrapper, or a headless viewer such as
// the web one -- so this file has no Qt in it.
//
// What is left in gui.cpp is the Qt gui's own command surface: its menus and
// widgets, the timing and clock views, and the window itself.

#include <fnmatch.h>

#include <algorithm>
#include <any>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#include "boost/algorithm/string/predicate.hpp"
#include "heatMapRenderer.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"
#include "web/core.h"
#include "web/descriptor_registry.h"
#include "web/heatMap.h"

namespace web {

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

Gui::Gui() : continue_after_close_(false), logger_(nullptr), db_(nullptr)
{
  resetDbuConversions();
}

bool Gui::enabled()
{
  return Gui::get()->activeBackend() != nullptr;
}

bool Gui::hasUI()
{
  // Ask the backend rather than assuming the slot it sits in implies a
  // window: both slots hold a GuiBackend, so a viewer without a window
  // type-checks into either, and every "if (!hasUI()) return;" guard in
  // gui.cpp would then fall through to a null main_window.  Asking
  // activeBackend() rather than backend_ keeps the answer about where calls
  // actually land, which is what the guards care about.
  const GuiBackend* backend = Gui::get()->activeBackend();
  return backend != nullptr && backend->hasWindow();
}

void Gui::setHeadlessViewer(GuiBackend* viewer)
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

// Quotes a string as a Tcl word.  Inside "..." Tcl still expands $variables
// and [commands] and honours backslash escapes, so a path has to be quoted
// before it goes into a generated script: one as ordinary as out/img[list].png
// would otherwise reach save_image rewritten, as out/img.png.  Escaping the
// opening bracket is enough -- a ] with no [ to match is already literal.
std::string quoteTcl(const std::string& str)
{
  std::string quoted = "\"";
  for (const char c : str) {
    if (c == '\\' || c == '"' || c == '$' || c == '[') {
      quoted += '\\';
    }
    quoted += c;
  }
  quoted += '"';
  return quoted;
}

// The "Valid options are: ..." tail of the heat map errors below.
std::string joinWithCommas(const std::vector<std::string>& items)
{
  std::string joined;
  for (const std::string& item : items) {
    if (!joined.empty()) {
      joined += ", ";
    }
    joined += item;
  }
  return joined;
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
    logger_->error(utl::WEB, 105, "No instance named {} found.", name);
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
    logger_->error(utl::WEB, 106, "No net named {} found.", name);
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

void Gui::saveImage(const std::string& filename,
                    const odb::Rect& region,
                    int width_px,
                    double dbu_per_pixel,
                    const std::map<std::string, bool>& display_settings)
{
  if (db_ == nullptr) {
    logger_->error(utl::WEB, 82, "No design loaded.");
  }

  odb::Rect save_region = region;
  const bool use_die_area = region.dx() == 0 || region.dy() == 0;
  const GuiBackend* backend = activeBackend();
  const bool is_offscreen = backend == nullptr || backend->isOffscreen();
  if (is_offscreen && use_die_area) {
    // Onscreen the visible area of the layout viewer is what the user means;
    // offscreen it is not reliable, so use the die area instead.
    auto* chip = db_->getChip();
    if (chip == nullptr) {
      logger_->error(utl::WEB, 97, "No design loaded.");
    }
    // The rect "the whole design" means, matching what the layout viewer fits
    // to (LayoutViewer::getBounds) and what the web renderer frames a
    // zero-area request on (TileGenerator::getFitBounds): every chiplet in the
    // hierarchy, its block bbox AND its die area.  A bbox alone covers the
    // placed SHAPES, not the floorplan, so a design sitting in a corner of a
    // much larger die would be framed on its content alone.
    //
    // The walk is LayoutViewer::getChips()': a stack over dbChipInst ->
    // masterChip, with each instance's own transform applied (that viewer does
    // not compose ancestor transforms either, so a deeper hierarchy is framed
    // the same way in both).
    //
    // Two deliberate departures from that viewer, both about empty rects: this
    // starts from mergeInit() rather than the root chip, and it ignores a chip
    // bbox with no area.  An ordinary design has no chip outline and reports
    // an empty one anchored at the origin, which merged in would drag the
    // region out to (0, 0).
    odb::Rect design;
    design.mergeInit();
    std::vector<std::pair<odb::dbChipInst*, odb::dbChip*>> stack;
    stack.emplace_back(nullptr, chip);
    while (!stack.empty()) {
      auto [chip_inst, cur_chip] = stack.back();
      stack.pop_back();
      if (cur_chip == nullptr) {
        continue;
      }

      const odb::Rect chip_bbox
          = chip_inst != nullptr ? chip_inst->getBBox() : cur_chip->getBBox();
      if (chip_bbox.area() > 0) {
        design.merge(chip_bbox);
      }

      if (odb::dbBlock* cur_block = cur_chip->getBlock()) {
        odb::Rect bbox = cur_block->getBBox()->getBox();
        odb::Rect die = cur_block->getDieArea();
        if (chip_inst != nullptr) {
          chip_inst->getTransform().apply(bbox);
          chip_inst->getTransform().apply(die);
        }
        design.merge(bbox);
        if (die.area() > 0) {
          design.merge(die);
        }
      }

      for (odb::dbChipInst* child : cur_chip->getChipInsts()) {
        stack.emplace_back(child, child->getMasterChip());
      }
    }
    // Nothing reported an extent: fall back to the chip, as before.
    save_region
        = (design.dx() > 0 && design.dy() > 0) ? design : chip->getBBox();

    const double bloat_by = 0.05;  // 5%
    const int bloat = std::min(save_region.dx(), save_region.dy()) * bloat_by;

    save_region.bloat(bloat, save_region);
  }

  if (hasUI()) {
    // Apply the caller's display settings over the current ones, render, and
    // put the panel back the way it was.
    activeBackend()->saveDisplayControls();
    for (const auto& [control, value] : display_settings) {
      setDisplayControlsVisible(control, value);
    }

    activeBackend()->saveImage(filename, save_region, width_px, dbu_per_pixel);

    activeBackend()->restoreDisplayControls();
    return;
  }

  // No window, so there is nothing to render from.  Open one, have it run
  // this same command, and close it again.  save_region is already resolved
  // and non-empty, so the reopened gui skips the die-area fallback above
  // rather than bloating it a second time.
  GuiLauncher* launcher = getLauncher();
  if (launcher == nullptr) {
    // A build with no Qt has no gui to open.
    return;
  }

  const double dbu_per_micron = db_->getDbuPerMicron();

  std::string save_cmds;
  save_cmds = "set ::gui::display_settings [gui::DisplayControlMap]\n";
  for (const auto& [control, value] : display_settings) {
    save_cmds
        += fmt::format(
               "$::gui::display_settings set {} {}", quoteTcl(control), value)
           + "\n";
  }
  save_cmds += "gui::save_image ";
  save_cmds += quoteTcl(filename) + " ";
  save_cmds += std::to_string(save_region.xMin() / dbu_per_micron) + " ";
  save_cmds += std::to_string(save_region.yMin() / dbu_per_micron) + " ";
  save_cmds += std::to_string(save_region.xMax() / dbu_per_micron) + " ";
  save_cmds += std::to_string(save_region.yMax() / dbu_per_micron) + " ";
  save_cmds += std::to_string(width_px) + " ";
  save_cmds += std::to_string(dbu_per_pixel) + " ";
  save_cmds += "$::gui::display_settings\n";
  save_cmds += "rename $::gui::display_settings \"\"\n";
  save_cmds += "unset ::gui::display_settings\n";
  save_cmds += "gui::hide";
  launcher->openAndRun(save_cmds);
}

void Gui::syncHeatMapChips()
{
  if (hasUI() || db_ == nullptr) {
    return;
  }

  // Console and headless sessions do not receive MainWindow::setBlock().
  auto* chip = db_->getChip();
  for (auto* heat_map : heat_maps_) {
    if (heat_map->getChip() != chip) {
      heat_map->setChip(chip);
      heat_map->destroyMap();
    }
  }
}

const std::set<HeatMapDataSource*>& Gui::getHeatMaps()
{
  syncHeatMapChips();
  return heat_maps_;
}

HeatMapDataSource* Gui::getHeatMap(const std::string& name)
{
  syncHeatMapChips();

  HeatMapDataSource* source = nullptr;

  for (auto* heat_map : heat_maps_) {
    if (heat_map->getShortName() == name) {
      source = heat_map;
      break;
    }
  }

  if (source == nullptr) {
    std::vector<std::string> options;
    options.reserve(heat_maps_.size());
    for (auto* heat_map : heat_maps_) {
      options.push_back(heat_map->getShortName());
    }
    logger_->error(utl::WEB,
                   83,
                   "{} is not a known map. Valid options are: {}",
                   name,
                   joinWithCommas(options));
  }

  return source;
}

void Gui::setHeatMapSetting(const std::string& name,
                            const std::string& option,
                            const Renderer::Setting& value)
{
  HeatMapDataSource* source = getHeatMap(name);

  const std::string rebuild_map_option = "rebuild";
  if (option == rebuild_map_option) {
    source->destroyMap();
    source->ensureMap();
  } else {
    auto settings = source->getSettings();

    if (!settings.contains(option)) {
      std::vector<std::string> options{rebuild_map_option};
      for (const auto& [key, kv] : settings) {
        options.push_back(key);
      }
      logger_->error(utl::WEB,
                     84,
                     "{} is not a valid option. Valid options are: {}",
                     option,
                     joinWithCommas(options));
    }

    auto& current_value = settings[option];
    if (std::holds_alternative<bool>(current_value)) {
      // is bool
      if (auto* s = std::get_if<bool>(&value)) {
        settings[option] = *s;
      } else if (auto* s = std::get_if<int>(&value)) {
        settings[option] = *s != 0;
      } else if (auto* s = std::get_if<double>(&value)) {
        settings[option] = *s != 0.0;
      } else {
        logger_->error(utl::WEB, 93, "{} must be a boolean", option);
      }
    } else if (std::holds_alternative<int>(current_value)) {
      // is int
      if (auto* s = std::get_if<int>(&value)) {
        settings[option] = *s;
      } else if (auto* s = std::get_if<double>(&value)) {
        settings[option] = static_cast<int>(*s);
      } else {
        logger_->error(utl::WEB, 94, "{} must be an integer or double", option);
      }
    } else if (std::holds_alternative<double>(current_value)) {
      // is double
      if (auto* s = std::get_if<int>(&value)) {
        settings[option] = static_cast<double>(*s);
      } else if (auto* s = std::get_if<double>(&value)) {
        settings[option] = *s;
      } else {
        logger_->error(utl::WEB, 95, "{} must be an integer or double", option);
      }
    } else {
      // is string
      if (auto* s = std::get_if<std::string>(&value)) {
        settings[option] = *s;
      } else {
        logger_->error(utl::WEB, 96, "{} must be a string", option);
      }
    }
    source->setSettings(settings);
  }

  source->redraw();
}

Renderer::Setting Gui::getHeatMapSetting(const std::string& name,
                                         const std::string& option)
{
  HeatMapDataSource* source = getHeatMap(name);

  const std::string map_has_option = "has_data";
  if (option == map_has_option) {
    return source->hasData();
  }

  auto settings = source->getSettings();

  if (!settings.contains(option)) {
    std::vector<std::string> options;
    options.reserve(settings.size());
    for (const auto& [key, kv] : settings) {
      options.push_back(key);
    }
    logger_->error(utl::WEB,
                   104,
                   "{} is not a valid option. Valid options are: {}",
                   option,
                   joinWithCommas(options));
  }

  return settings[option];
}

void Gui::dumpHeatMap(const std::string& name, const std::string& file)
{
  HeatMapDataSource* source = getHeatMap(name);
  source->dumpToFile(file);
}

bool Gui::filterSelectionProperties(const Descriptor::Properties& properties,
                                    const std::string& attribute,
                                    const std::any& value,
                                    bool& is_valid_attribute)
{
  for (const Descriptor::Property& property : properties) {
    if (attribute == property.name) {
      is_valid_attribute = true;
      if (auto props_selected_set
          = std::any_cast<SelectionSet>(&property.value)) {
        if (Descriptor::Property::toString(value) == "CONNECTED"
            && !props_selected_set->empty()) {
          return true;
        }
        for (const auto& selected : *props_selected_set) {
          if (Descriptor::Property::toString(value) == selected.getName()) {
            return true;
          }
        }
      } else if (auto props_list
                 = std::any_cast<Descriptor::PropertyList>(&property.value)) {
        for (const auto& prop : *props_list) {
          if (Descriptor::Property::toString(prop.first)
                  == Descriptor::Property::toString(value)
              || Descriptor::Property::toString(prop.second)
                     == Descriptor::Property::toString(value)) {
            return true;
          }
        }
      } else if (Descriptor::Property::toString(value)
                 == Descriptor::Property::toString(property.value)) {
        return true;
      }
    }
  }

  return false;
}

int Gui::select(const std::string& type,
                const std::string& name_filter,
                const std::string& attribute,
                const std::any& value,
                bool filter_case_sensitive,
                int highlight_group)
{
  if (!hasUI()) {
    return 0;
  }

  // The same glob the web viewer's find uses, so the two agree about what a
  // name pattern means.  A backslash escapes the character after it, which is
  // how a bus bit is named: req_msg\[0\].  Unescaped brackets are a character
  // class, as in any glob, so req_msg[0] means req_msg0.
  const int match_flags = filter_case_sensitive ? 0 : FNM_CASEFOLD;
  const bool literal = name_filter.find_first_of("*?[\\") == std::string::npos;

  bool found = false;
  int result = 0;
  auto* registry = DescriptorRegistry::instance();
  registry->forEachDescriptor([&](const Descriptor* descriptor) {
    if (found || descriptor->getTypeName() != type) {
      return;
    }
    found = true;
    SelectionSet selected_set;
    descriptor->visitAllObjects([&](const Selected& sel) {
      if (!name_filter.empty()) {
        const std::string sel_name = sel.getName();
        if (literal) {
          if (filter_case_sensitive ? sel_name != name_filter
                                    : !boost::iequals(sel_name, name_filter)) {
            return;
          }
        } else if (fnmatch(name_filter.c_str(), sel_name.c_str(), match_flags)
                   != 0) {
          return;
        }
      }

      if (!attribute.empty()) {
        bool is_valid_attribute = false;
        Descriptor::Properties properties
            = descriptor->getProperties(sel.getObject());
        if (!filterSelectionProperties(
                properties, attribute, value, is_valid_attribute)) {
          return;  // doesn't match the attribute filter
        }

        if (!is_valid_attribute) {
          logger_->error(
              utl::WEB, 92, "Entered attribute {} is not valid.", attribute);
        }
      }
      selected_set.insert(sel);
    });

    activeBackend()->addSelected(selected_set, true);
    if (highlight_group != -1) {
      activeBackend()->addHighlighted(selected_set, highlight_group);
    }

    result = selected_set.size();
  });

  if (!found) {
    logger_->error(utl::WEB, 86, "Unable to find descriptor for: {}", type);
  }
  return result;
}

void Gui::registerHeatMap(HeatMapDataSource* heatmap)
{
  if (heat_maps_.contains(heatmap)) {
    return;
  }
  heat_maps_.insert(heatmap);
  auto renderer = makeHeatMapRenderer(*heatmap);
  heatmap->setRedrawCallback(
      [renderer_ptr = renderer.get()]() { renderer_ptr->redraw(); });
  heatmap->setSetupCallback([heatmap]() {
    // A build with no Qt has no dialog to open; the heat map is still
    // registered and still draws, it just cannot be configured.
    if (Dialogs* dialogs = Gui::get()->getDialogs()) {
      dialogs->showHeatMapSetup(heatmap);
    }
  });
  heatmap->setUnregisterCallback(
      [this](HeatMapDataSource* source) { unregisterHeatMap(source); });
  registerRenderer(renderer.get());
  heat_map_renderers_[heatmap] = std::move(renderer);
  if (auto* backend = activeBackend()) {
    backend->registerHeatMap(heatmap);
  }
}

void Gui::unregisterHeatMap(HeatMapDataSource* heatmap)
{
  if (!heat_maps_.contains(heatmap)) {
    return;
  }

  heatmap->setRedrawCallback({});
  heatmap->setSetupCallback({});
  heatmap->setUnregisterCallback({});
  auto renderer_itr = heat_map_renderers_.find(heatmap);
  if (renderer_itr != heat_map_renderers_.end()) {
    unregisterRenderer(renderer_itr->second.get());
    heat_map_renderers_.erase(renderer_itr);
  }
  if (auto* backend = activeBackend()) {
    backend->unregisterHeatMap(heatmap);
  }
  heat_maps_.erase(heatmap);
}

void Gui::setChartFactory(ChartFactory factory)
{
  chart_factory_ = std::move(factory);
}

Chart* Gui::addChart(const std::string& name,
                     const std::string& x_label,
                     const std::vector<std::string>& y_labels)
{
  if (hasUI()) {
    return activeBackend()->addChart(name, x_label, y_labels);
  }
  // No charts widget, so a module that wants one supplies its own maker.
  if (chart_factory_) {
    return chart_factory_(name, x_label, y_labels);
  }
  return nullptr;
}

void Gui::timingCone(Term term, bool fanin, bool fanout)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->timingCone(term, fanin, fanout);
}

void Gui::timingPathsThrough(const std::set<Term>& terms)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->timingPathsThrough(terms);
}

void Gui::triggerAction(const std::string& name)
{
  if (!hasUI()) {
    return;
  }
  activeBackend()->triggerAction(name);
}

void Gui::initCommon(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger)
{
  db_ = db;
  logger_ = logger;

  auto* registry = DescriptorRegistry::instance();
  registry->setLogger(logger);
  registry->initDescriptors(db, sta);

  registerBuiltinHeatMapSources(sta, logger);
}

}  // namespace web
