// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

%{
#include "ord/OpenRoad.hh"
#include "ram/ram.h"
#include "utl/Logger.h"

%}

%include "../../Exception.i"

%inline %{

namespace ram {

void
generate_ram_netlist_cmd(int bytes_per_word,
                         int word_count,
                         const char* storage_cell_name,
                         const char* tristate_cell_name,
                         const char* inv_cell_name,
                         const int read_ports,
                         const char* tapcell_name,
                         const int max_tap_dist)
{
  auto* app = ord::OpenRoad::openRoad();
  auto* ram_gen = app->getRamGen();
  auto* db = app->getDb();

  odb::dbMaster* storage_cell = nullptr;
  if (storage_cell_name[0] != '\0') {
    storage_cell = db->findMaster(storage_cell_name);
    if (!storage_cell) {
      app->getLogger()->error(utl::RAM,
                              4,
                              "Storage cell {} can't be found",
                              storage_cell_name);
    }
  }

  odb::dbMaster* tristate_cell = nullptr;
  if (tristate_cell_name[0] != '\0') {
    tristate_cell = db->findMaster(tristate_cell_name);
    if (!tristate_cell) {
      app->getLogger()->error(utl::RAM,
                              7,
                              "Tristate cell {} can't be found",
                              tristate_cell_name);
    }
  }

  odb::dbMaster* inv_cell = nullptr;
  if (inv_cell_name[0] != '\0') {
    inv_cell = db->findMaster(inv_cell_name);
    if (!inv_cell) {
      app->getLogger()->error(utl::RAM,
                              8,
                              "Inv cell {} can't be found",
                              inv_cell_name);
    }
  }

  odb::dbMaster* tapcell = nullptr;
  if (tapcell_name[0] != '\0') {
    tapcell = db->findMaster(tapcell_name);
    if (!tapcell) {
      app->getLogger()->error(utl::RAM,
                              19,
                              "Tapcell {} can't be found",
                              tapcell_name);
    }
  }

  ram_gen->generate(bytes_per_word, word_count, read_ports,
                    storage_cell, tristate_cell, inv_cell, tapcell,
                    max_tap_dist);
}

} //namespace_ram

%} // inline

