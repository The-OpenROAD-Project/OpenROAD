// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

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

void select_clockviewer_clock(const char* clock_name)
{
  if (!check_gui("select_clockviewer_clock")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->selectClockviewerClock(clock_name);
}

void save_histogram_image(const char* filename, const char* mode, int width_px = 0, int height_px = 0)
{
  if (!check_gui("save_histogram_image")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->saveHistogramImage(filename, mode, width_px, height_px);
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

void set_display_controls(const char* name, const char* display_type, const char* value)
{
  if (!check_gui("set_display_controls")) {
    return;
  }
  auto gui = gui::Gui::get();
  auto logger = ord::OpenRoad::openRoad()->getLogger();
  
  std::string disp_type = display_type;
  std::string str_value = value;
  // make lower case
  std::transform(disp_type.begin(), 
                 disp_type.end(), 
                 disp_type.begin(), 
                 [](char c) { return std::tolower(c); });
  if (disp_type == "visible") {
    bool bval = str_value == "true" || str_value == "1";
    gui->setDisplayControlsVisible(name, bval);
  } else if (disp_type == "selectable") {
    bool bval = str_value == "true" || str_value == "1";
    gui->setDisplayControlsSelectable(name, bval);
  } else if (disp_type == "color") {
    gui::Painter::Color color = gui::Painter::black;
    if (str_value.empty()) {
      logger->error(GUI, 41, "Color is required");
    }
    if (str_value[0] == '#' && (str_value.size() == 7 || str_value.size() == 9)) {
      uint hex_color = 0;
      for (int i = 1; i < str_value.size(); i++) {
        const char c = std::tolower(str_value[i]);
        hex_color *= 16;
        if (c >= '0' && c <= '9') {
          hex_color += c - '0';
        } else if (c >= 'a' && c <= 'f') {
          hex_color += c - 'a' + 10;
        } else {
          logger->error(GUI, 43, "Unable to decode color: {}", str_value);
        }
      }
      if (str_value.size() == 7) {
        hex_color *= 256;
        hex_color += 255;
      }
      color.r = (hex_color & 0xff000000) >> 24;
      color.g = (hex_color & 0x00ff0000) >> 16;
      color.b = (hex_color & 0x0000ff00) >> 8;
      color.a = hex_color & 0x000000ff;
    } else if (str_value == "black") {
      color = gui::Painter::black;
    } else if (str_value == "white") {
      color = gui::Painter::white;
    } else if (str_value == "dark_gray") {
      color = gui::Painter::dark_gray;
    } else if (str_value == "gray") {
      color = gui::Painter::gray;
    } else if (str_value == "light_gray") {
      color = gui::Painter::light_gray;
    } else if (str_value == "red") {
      color = gui::Painter::red;
    } else if (str_value == "green") {
      color = gui::Painter::green;
    } else if (str_value == "blue") {
      color = gui::Painter::blue;
    } else if (str_value == "cyan") {
      color = gui::Painter::cyan;
    } else if (str_value == "magenta") {
      color = gui::Painter::magenta;
    } else if (str_value == "yellow") {
      color = gui::Painter::yellow;
    } else if (str_value == "dark_red") {
      color = gui::Painter::dark_red;
    } else if (str_value == "dark_green") {
      color = gui::Painter::dark_green;
    } else if (str_value == "dark_blue") {
      color = gui::Painter::dark_blue;
    } else if (str_value == "dark_cyan") {
      color = gui::Painter::dark_cyan;
    } else if (str_value == "dark_magenta") {
      color = gui::Painter::dark_magenta;
    } else if (str_value == "dark_yellow") {
      color = gui::Painter::dark_yellow;
    } else if (str_value == "orange") {
      color = gui::Painter::orange;
    } else if (str_value == "purple") {
      color = gui::Painter::purple;
    } else if (str_value == "lime") {
      color = gui::Painter::lime;
    } else if (str_value == "teal") {
      color = gui::Painter::teal;
    } else if (str_value == "pink") {
      color = gui::Painter::pink;
    } else if (str_value == "brown") {
      color = gui::Painter::brown;
    } else if (str_value == "indigo") {
      color = gui::Painter::indigo;
    } else if (str_value == "turquoise") {
      color = gui::Painter::turquoise;
    } else if (str_value == "transparent") {
      color = gui::Painter::transparent;
    } else {
      logger->error(GUI, 42, "Color not recognized: {}", str_value);
    }

    gui->setDisplayControlsColor(name, color);
  } else {
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

void select_marker_category(odb::dbMarkerCategory* category)
{
  if (!check_gui("select_marker_category")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->selectMarkers(category);
}

void select_marker_category(const char* name)
{
  select_marker_category(get_block()->findMarkerCategory(name));
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

void show(const char* script = "", bool interactive = true, bool load_settings = true)
{
  auto gui = gui::Gui::get();
  gui->showGui(script, interactive, load_settings);
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

std::string get_heatmap_string(const std::string& name, const std::string& option)
{
  auto gui = gui::Gui::get();
  auto value = gui->getHeatMapSetting(name, option);
  if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
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

void minimize()
{
  if (!check_gui("minimize")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->minimize();
}

void unminimize()
{
  if (!check_gui("unminimize")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->unminimize();
}

void show_help(const std::string& item)
{
  if (!check_gui("show_help")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->selectHelp(item);
}

void select_chart(const std::string& name)
{
  if (!check_gui("select_chart")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->selectChart(name);
}

void update_timing_report()
{
  if (!check_gui("update_timing_report")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->updateTimingReport();
}

void set_title(std::string title)
{
  auto gui = gui::Gui::get();
  gui->setMainWindowTitle(title);
}

%} // inline
