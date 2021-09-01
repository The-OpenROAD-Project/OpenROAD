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
  auto gui = gui::Gui::get();
  if (gui == nullptr) {
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
%include "std_string.i"

%inline %{

bool enabled()
{
  auto gui = gui::Gui::get();
  return gui != nullptr;
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
  gui->addSelectedNets(name);
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
  gui->addSelectedInsts(name);
}

void highlight_inst(const char* name, int highlightGroup)
{
  if (!check_gui("highlight_inst")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->addInstToHighlightSet(name, highlightGroup);
}

void highlight_net(const char* name, int highlightGroup=0)
{
  if (!check_gui("highlight_net")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->addNetToHighlightSet(name, highlightGroup);
}

const std::string add_ruler(
  double x0, 
  double y0, 
  double x1, 
  double y1, 
  const std::string& label = "", 
  const std::string& name = "")
{
  if (!check_gui("add_ruler")) {
    return "";
  }
  odb::Point ll = make_point(x0, y0);
  odb::Point ur = make_point(x1, y1);
  auto gui = gui::Gui::get();
  return gui->addRuler(ll.x(), ll.y(), ur.x(), ur.y(), label, name);  
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
  auto gui = gui::Gui::get();
  gui->load_design();
}

void fit()
{
  if (!check_gui("fit")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->fit();
}

void save_image(const char* filename)
{
  if (!check_gui("save_image")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->saveImage(filename);
}

void save_image(const char* filename, double xlo, double ylo, double xhi, double yhi)
{
  if (!check_gui("save_image")) {
    return;
  }
  auto gui = gui::Gui::get();
  gui->saveImage(filename, make_rect(xlo, ylo, xhi, yhi));
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

const std::string input_dialog(const char* title, const char* question)
{
  if (!check_gui("input_dialog")) {
    return "";
  }
  auto gui = gui::Gui::get();
  return gui->requestUserInput(title, question);
}

void pause(int timeout = 0)
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

%} // inline

