/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

%{
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
#include "gui/gui.h"

using utl::GUI;

bool check_gui(const char* command)
{
  auto logger = ord::OpenRoad::openRoad()->getLogger(); 
  if (!gui::Gui::enabled()) {
    logger->info(GUI, 1, "Command {} is not usable in non-GUI mode", command);
    return false;
  }

  auto db = ord::OpenRoad::openRoad()->getDb();
  if (db == nullptr) {
    logger->error(GUI, 2, "No database loaded");
  }

  return true;
}

odb::dbBlock* get_block()
{
  auto logger = ord::OpenRoad::openRoad()->getLogger();
  auto db = ord::OpenRoad::openRoad()->getDb();
  if (db == nullptr) {
    logger->error(GUI, 3, "No database loaded");
  }
  auto chip = db->getChip();
  if (chip == nullptr) {
    logger->error(GUI, 5, "No chip loaded");
  }
  auto block = chip->getBlock();
  if (block == nullptr) {
    logger->error(GUI, 6, "No block loaded");
  }
  return block;
}

// converts from microns to DBU
odb::Rect make_rect(double xlo, double ylo, double xhi, double yhi)
{
  auto block = get_block();
  int dbuPerUU = block->getDbUnitsPerMicron();
  return odb::Rect(xlo * dbuPerUU, ylo * dbuPerUU, xhi * dbuPerUU, yhi * dbuPerUU);
}

// converts from microns to DBU
odb::Point make_point(double x, double y)
{
  auto block = get_block();
  int dbuPerUU = block->getDbUnitsPerMicron();
  return odb::Point(x * dbuPerUU, y * dbuPerUU);
}

%}

%include "../../Exception.i"
%include <std_string.i>
%include <std_map.i>
namespace std {
  %template(DisplayControlMap) map<string, bool>;
}

%rename(pause) gui_pause;

%inline %{

bool enabled()
{
  return gui::Gui::enabled();
}

void
selection_add_net(const char* name)
{
  if (!check_gui("selection_add_net")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->addSelectedNet(name);
}

void
selection_add_nets(const char* name)
{
  if (!check_gui("selection_add_nets")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->select("Net", name);
}

void
selection_add_inst(const char* name)
{
  if (!check_gui("selection_add_inst")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->addSelectedInst(name);
}

void
selection_add_insts(const char* name)
{
  if (!check_gui("selection_add_insts")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->select("Inst", name);
}

void highlight_inst(const char* name, int highlight_group = 0)
{
  if (!check_gui("highlight_inst")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->addInstToHighlightSet(name, highlight_group);
}

void highlight_net(const char* name, int highlight_group = 0)
{
  if (!check_gui("highlight_net")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->addNetToHighlightSet(name, highlight_group);
}

const std::string add_ruler(
  double x0, 
  double y0, 
  double x1, 
  double y1, 
  const std::string& label = "", 
  const std::string& name = "",
  bool euclidian = true)
{
  if (!check_gui("add_ruler")) {
    return "";
  }
  odb::Point ll = make_point(x0, y0);
  odb::Point ur = make_point(x1, y1);
  auto gui = gui::Gui::get();
  return gui->addRuler(ll.x(), ll.y(), ur.x(), ur.y(), label, name, euclidian);
}

void delete_ruler(const std::string& name)
{
  if (!check_gui("delete_ruler")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->deleteRuler(name);  
}

void zoom_to(double xlo, double ylo, double xhi, double yhi)
{
  if (!check_gui("zoom_to")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->zoomTo(make_rect(xlo, ylo, xhi, yhi));
}

void zoom_in()
{
  if (!check_gui("zoom_in")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->zoomIn();
}

void zoom_in(double x, double y)
{
  if (!check_gui("zoom_in")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->zoomIn(make_point(x, y));
}

void zoom_out()
{
  if (!check_gui("zoom_out")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->zoomOut();
}

void zoom_out(double x, double y)
{
  if (!check_gui("zoom_out")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->zoomIn(make_point(x, y));
}

void center_at(double x, double y)
{
  if (!check_gui("center_at")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->centerAt(make_point(x, y));
}

void set_resolution(double dbu_per_pixel)
{
  if (!check_gui("set_resolution")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->setResolution(1 / dbu_per_pixel);
}

void design_created()
{
  if (!check_gui("design_created")) {
    return;
  }
  ord::OpenRoad::openRoad()->designCreated();
}

void fit()
{
  if (!check_gui("fit")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->fit();
}

void save_image(const char* filename, double xlo, double ylo, double xhi, double yhi, int width_px = 0, double dbu_per_pixel = 0, const std::map<std::string, bool>& display_settings = {})
{
  auto gui = gui::Gui::get();
  gui->saveImage(filename, make_rect(xlo, ylo, xhi, yhi), width_px, dbu_per_pixel, display_settings);
}

void save_clocktree_image(const char* filename, const char* clock_name, const char* corner = "", int width_px = 0, int height_px = 0)
{
  if (!check_gui("save_clocktree_image")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->saveClockTreeImage(clock_name, filename, corner, width_px, height_px);
}

void clear_rulers()
{
  if (!check_gui("clear_rulers")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->clearRulers();
}

void clear_selections()
{
  if (!check_gui("clear_selections")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->clearSelections();
}

void clear_highlights(int highlight_group = 0)
{
  if (!check_gui("clear_highlights")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->clearHighlights(highlight_group);
}

void set_display_controls(const char* name, const char* display_type, bool value)
{
  if (!check_gui("set_display_controls")) {
    return;
  }
  auto gui = gui::Gui::get();
  
  std::string disp_type = display_type;
  // make lower case
  std::transform(disp_type.begin(), 
                 disp_type.end(), 
                 disp_type.begin(), 
                 [](char c) { return std::tolower(c); });
  if (disp_type == "visible") {
    gui->setDisplayControlsVisible(name, value);
  } else if (disp_type == "selectable") {
    gui->setDisplayControlsSelectable(name, value);
  } else {
    auto logger = ord::OpenRoad::openRoad()->getLogger();
    logger->error(GUI, 7, "Unknown display control type: {}", display_type);
  }
}

bool check_display_controls(const char* name, const char* display_type)
{
  if (!check_gui("check_display_controls")) {
    return false;
  }
  auto gui = gui::Gui::get();
  
  std::string disp_type = display_type;
  // make lower case
  std::transform(disp_type.begin(), 
                 disp_type.end(), 
                 disp_type.begin(), 
                 [](char c) { return std::tolower(c); });
  if (disp_type == "visible") {
    return gui->checkDisplayControlsVisible(name);
  } else if (disp_type == "selectable") {
    return gui->checkDisplayControlsSelectable(name);
  } else {
    auto logger = ord::OpenRoad::openRoad()->getLogger();
    logger->error(GUI, 9, "Unknown display control type: {}", display_type);
  }
  
  return false;
}

void save_display_controls()
{
  if (!check_gui("set_display_controls")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->saveDisplayControls();
}

void restore_display_controls()
{
  if (!check_gui("restore_display_controls")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->restoreDisplayControls();
}

const std::string create_toolbar_button(const char* name, const char* text, const char* script, bool echo)
{
  if (!check_gui("create_toolbar_button")) {
    return "";
  }
  auto gui = gui::Gui::get();
  return gui->addToolbarButton(name, text, script, echo);
}

void remove_toolbar_button(const char* name)
{
  if (!check_gui("remove_toolbar_button")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->removeToolbarButton(name);
}

const std::string create_menu_item(const char* name, 
                                   const char* path, 
                                   const char* text, 
                                   const char* script, 
                                   const char* shortcut, 
                                   bool echo)
{
  if (!check_gui("create_menu_item")) {
    return "";
  }
  auto gui = gui::Gui::get();
  return gui->addMenuItem(name, path, text, script, shortcut, echo);
}

void remove_menu_item(const char* name)
{
  if (!check_gui("remove_menu_item")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->removeMenuItem(name);
}

const std::string input_dialog(const char* title, const char* question)
{
  if (!check_gui("input_dialog")) {
    return "";
  }
  auto gui = gui::Gui::get();
  return gui->requestUserInput(title, question);
}

// glib has pause() so this is %rename'd to pause in the scripting
// language to avoid conflicts in C++.
void gui_pause(int timeout = 0)
{
  if (!check_gui("pause")) {
    return;
  }
  auto gui = gui::Gui::get();
  return gui->pause(timeout);
}

void load_drc(const char* filename)
{
  if (!check_gui("load_drc")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->loadDRC(filename);
}

void show_widget(const char* name)
{
  if (!check_gui("show_widget")) {
    return;
  }
  auto gui = gui::Gui::get();
  return gui->showWidget(name, true);
}

void hide_widget(const char* name)
{
  if (!check_gui("hide_widget")) {
    return;
  }
  auto gui = gui::Gui::get();
  return gui->showWidget(name, false);
}

void show(const char* script = "", bool interactive = true)
{
  auto gui = gui::Gui::get();
  gui->showGui(script, interactive);
}

void hide()
{
  if (!check_gui("hide")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->hideGui();
}

const std::string get_selection_property(const std::string& prop_name)
{
  if (!check_gui("get_selection_property")) {
    return "";
  }
  auto gui = gui::Gui::get();
  
  const gui::Selected& selected = gui->getInspectorSelection();
  if (!selected) {
    auto logger = ord::OpenRoad::openRoad()->getLogger();
    logger->error(GUI, 36, "Nothing selected");
  }
  
  const std::any& prop = selected.getProperty(prop_name);
  if (!prop.has_value()) {
    auto logger = ord::OpenRoad::openRoad()->getLogger();
    logger->error(GUI, 37, "Unknown property: {}", prop_name);
  }

  std::string prop_text = gui::Descriptor::Property::toString(prop);

  if (prop_name == "BBox") {
    // need to reformat to make it useable for TCL
    // remove () and space
    for (const char ch : {'(', ')', ' '}) {
      std::size_t pos;
      while ((pos = prop_text.find(ch)) != std::string::npos) {
        prop_text.erase(pos, 1);
      }
    }
    // replace commas with spaces
    std::size_t pos;
    while ((pos = prop_text.find(',')) != std::string::npos) {
      prop_text.replace(pos, 1, " ");
    }
  }
  
  return prop_text;
}

int select_at(double x0, double y0, double x1, double y1, bool append = true)
{
  if (!check_gui("select_at")) {
    return 0;
  }  
  auto gui = gui::Gui::get();
  return gui->selectAt(make_rect(x0, y0, x1, y1), append);
}

int select_at(double x, double y, bool append = true)
{
  return select_at(x, y, x, y, append);
}

int select_next()
{
  if (!check_gui("select_next")) {
    return 0;
  }  
  auto gui = gui::Gui::get();
  return gui->selectNext();
}

int select_previous()
{
  if (!check_gui("select_previous")) {
    return 0;
  }  
  auto gui = gui::Gui::get();
  return gui->selectPrevious();
}

int select(const std::string& type,
           const std::string& name_filter = "",
           const std::string& attribute = "",
           const std::string& value = "",
           bool case_sensitive = true,
           int highlight_group = -1)
{
  if (!check_gui("select")) {
    return 0;
  }

  auto gui = gui::Gui::get();
  return gui->select(type, name_filter, attribute, value, case_sensitive, highlight_group);
}

void selection_animate(int repeat = 0)
{
  if (!check_gui("selection_animate")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->animateSelection(repeat);
}

bool get_heatmap_bool(const std::string& name, const std::string& option)
{
  auto gui = gui::Gui::get();
  auto value = gui->getHeatMapSetting(name, option);
  if (std::holds_alternative<bool>(value)) {
      return std::get<bool>(value);
  } else {
    auto logger = ord::OpenRoad::openRoad()->getLogger();
    logger->error(GUI, 90, "Heatmap setting \"{}\" is not a boolean", option);
  }
  return false;
}

int get_heatmap_int(const std::string& name, const std::string& option)
{
  auto gui = gui::Gui::get();
  auto value = gui->getHeatMapSetting(name, option);
  if (std::holds_alternative<int>(value)) {
      return std::get<int>(value);
  } else {
    auto logger = ord::OpenRoad::openRoad()->getLogger();
    logger->error(GUI, 91, "Heatmap setting \"{}\" is not an integer", option);
  }
  return 0;
}

double get_heatmap_double(const std::string& name, const std::string& option)
{
  auto gui = gui::Gui::get();
  auto value = gui->getHeatMapSetting(name, option);
  if (std::holds_alternative<double>(value)) {
      return std::get<double>(value);
  } else {
    auto logger = ord::OpenRoad::openRoad()->getLogger();
    logger->error(GUI, 92, "Heatmap setting \"{}\" is not a double", option);
  }
  return 0.0;
}

const char* get_heatmap_string(const std::string& name, const std::string& option)
{
  auto gui = gui::Gui::get();
  auto value = gui->getHeatMapSetting(name, option);
  if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value).c_str();
  } else {
    auto logger = ord::OpenRoad::openRoad()->getLogger();
    logger->error(GUI, 93, "Heatmap setting \"{}\" is not a string", option);
  }
  return "";
}

void set_heatmap(const std::string& name, const std::string& option, double value = 0.0)
{
  auto gui = gui::Gui::get();
  gui->setHeatMapSetting(name, option, value);
}

void set_heatmap(const std::string& name, const std::string& option, const std::string& value)
{
  auto gui = gui::Gui::get();
  gui->setHeatMapSetting(name, option, value);
}

void dump_heatmap(const std::string& name, const std::string& file)
{
  auto gui = gui::Gui::get();
  gui->dumpHeatMap(name, file);
}

void timing_cone(odb::dbITerm* iterm, bool fanin, bool fanout)
{
  if (!check_gui("timing_cone")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->timingCone(iterm, fanin, fanout);
}

void timing_cone(odb::dbBTerm* bterm, bool fanin, bool fanout)
{
  if (!check_gui("timing_cone")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->timingCone(bterm, fanin, fanout);
}

void focus_net(odb::dbNet* net)
{
  if (!check_gui("focus_net")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->addFocusNet(net);
}

void remove_focus_net(odb::dbNet* net)
{
  if (!check_gui("remove_focus_net")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->removeFocusNet(net);
}

void clear_focus_nets()
{
  if (!check_gui("clear_focus_nets")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->clearFocusNets();
}

void trigger_action(const std::string& name)
{
  if (!check_gui("trigger_action")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->triggerAction(name);
}

bool supported()
{
  return true;
}

%} // inline
