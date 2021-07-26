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

void add_ruler(double x0, double y0, double x1, double y1)
{
  if (!check_gui("add_ruler")) {
    return;
  }
  odb::Point ll = make_point(x0, y0);
  odb::Point ur = make_point(x1, y1);
  auto gui = gui::Gui::get();
  gui->addRuler(ll.x(), ll.y(), ur.x(), ur.y());  
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
%} // inline

