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
%}

%inline %{

bool enabled()
{
  auto gui = gui::Gui::get();
  return gui != nullptr;
}

void
selection_add_net(const char* name)
{
  auto gui = gui::Gui::get();
  if (!gui) {
    ord::OpenRoad::openRoad()->getLogger()->info(GUI, 13, "Command selection_add_net is not usable in non-GUI mode");
    return;
  }
  gui->addSelectedNet(name);
}

void
selection_add_nets(const char* name)
{
  auto gui = gui::Gui::get();
  if (!gui) {
    ord::OpenRoad::openRoad()->getLogger()->info(GUI, 5, "Command selection_add_nets is not usable in non-GUI mode");
    return;
  }
  gui->addSelectedNets(name);
}

void
selection_add_inst(const char* name)
{
  auto gui = gui::Gui::get();
  if (!gui) {
    ord::OpenRoad::openRoad()->getLogger()->info(GUI, 6, "Command selection_add_inst is not usable in non-GUI mode");
    return;
  }
  gui->addSelectedInst(name);
}

void
selection_add_insts(const char* name)
{
  auto gui = gui::Gui::get();
  if (!gui) {
    ord::OpenRoad::openRoad()->getLogger()->info(GUI, 7, "Command selection_add_insts is not usable in non-GUI mode");
    return;
  }
  gui->addSelectedInsts(name);
}

void highlight_inst(const char* name, int highlightGroup)
{
  auto gui = gui::Gui::get();
  if (!gui) {
    ord::OpenRoad::openRoad()->getLogger()->info(GUI, 8, "Command highlight_inst is not usable in non-GUI mode");
    return;
  }
  gui->addInstToHighlightSet(name, highlightGroup);
}

void highlight_net(const char* name, int highlightGroup=0)
{
  auto gui = gui::Gui::get();
  if (!gui) {
    ord::OpenRoad::openRoad()->getLogger()->info(GUI, 9, "Command highlight_net is not usable in non-GUI mode");
    return;
  }
  gui->addNetToHighlightSet(name, highlightGroup);
}

void add_ruler(int x0, int y0, int x1, int y1)
{
  auto gui = gui::Gui::get();
  if (!gui) {
    ord::OpenRoad::openRoad()->getLogger()->info(GUI, 10, "Command add_ruler is not usable in non-GUI mode");
    return ;
  }
  gui->addRuler(x0, y0, x1, y1);  
}

// converts from microns to DBU
void zoom_to(double xlo, double ylo, double xhi, double yhi)
{
  auto gui = gui::Gui::get();
  auto logger = ord::OpenRoad::openRoad()->getLogger();
  if (!gui) {
    logger->info(GUI, 11, "Command zoom_to is not usable in non-GUI mode");
    return ;
  }
  auto db = ord::OpenRoad::openRoad()->getDb();
  if (!db) {
    logger->error(GUI, 1, "No database loaded");
  }
  auto chip = db->getChip();
  if (!chip) {
    logger->error(GUI, 2, "No chip loaded");
  }
  auto block = chip->getBlock();
  if (!block) {
    logger->error(GUI, 3, "No block loaded");
  }

  int dbuPerUU = block->getDbUnitsPerMicron();
  odb::Rect rect(xlo * dbuPerUU, ylo * dbuPerUU, xhi * dbuPerUU, yhi * dbuPerUU);
  gui->zoomTo(rect);
}

void design_created()
{
  auto gui = gui::Gui::get();
  if (!gui) {
    auto logger = ord::OpenRoad::openRoad()->getLogger();
    logger->info(GUI, 12, "Command design_created is not usable in non-GUI mode");
    return;
  }
  gui->load_design();
}

void fit()
{
  auto gui = gui::Gui::get();
  if (!gui) {
    auto logger = ord::OpenRoad::openRoad()->getLogger();
    logger->info(GUI, 14, "Command fit is not usable in non-GUI mode");
    return;
  }
  gui->fit();
}
%} // inline

