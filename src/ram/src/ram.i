// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

%{

#include <vector>
#include <string>

#include "ord/OpenRoad.hh"
#include "ram/ram.h"
#include "utl/Logger.h"

namespace ord {
// Defined in OpenRoad.i
odb::dbDatabase* getDb();
ram::RamGen* getRamGen();
utl::Logger* getLogger();
} // namespace ord

%}

%include <std_vector.i>
%include <std_string.i>
%template(vector_str) std::vector<std::string>;
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
  RamGen* ram_gen = ord::getRamGen();
  odb::dbDatabase* db = ord::getDb();
  utl::Logger* logger = ord::getLogger();

  odb::dbMaster* storage_cell = nullptr;
  if (storage_cell_name[0] != '\0') {
    storage_cell = db->findMaster(storage_cell_name);
    if (!storage_cell) {
      logger->error(utl::RAM,
                    4,
                    "Storage cell {} can't be found",
                    storage_cell_name);
    }
  }

  odb::dbMaster* tristate_cell = nullptr;
  if (tristate_cell_name[0] != '\0') {
    tristate_cell = db->findMaster(tristate_cell_name);
    if (!tristate_cell) {
      logger->error(utl::RAM,
                    7,
                    "Tristate cell {} can't be found",
                    tristate_cell_name);
    }
  }

  odb::dbMaster* inv_cell = nullptr;
  if (inv_cell_name[0] != '\0') {
    inv_cell = db->findMaster(inv_cell_name);
    if (!inv_cell) {
      logger->error(utl::RAM,
                    8,
                    "Inv cell {} can't be found",
                    inv_cell_name);
    }
  }

  odb::dbMaster* tapcell = nullptr;
  if (tapcell_name[0] != '\0') {
    tapcell = db->findMaster(tapcell_name);
    if (!tapcell) {
      logger->error(utl::RAM,
                    19,
                    "Tapcell {} can't be found",
                    tapcell_name);
    }
  }

  ram_gen->generate(bytes_per_word, word_count, read_ports,
                    storage_cell, tristate_cell, inv_cell, tapcell,
                    max_tap_dist);
}

void ram_pdngen(const char* power_pin, const char* ground_pin, 
                const char* route_name, int route_width, 
                const char* ver_name, int ver_width, int ver_pitch,
                const char* hor_name, int hor_width, int hor_pitch)
 
{
  RamGen* ram_gen = ord::getRamGen();
  ram_gen->ramPdngen(power_pin, ground_pin, 
                     route_name, route_width,
                     ver_name, ver_width, ver_pitch, 
                     hor_name, hor_width, hor_pitch);
}

void ram_pinplacer(const char* ver_name, const char* hor_name)
{
  RamGen* ram_gen = ord::getRamGen();
  ram_gen->ramPinplacer(ver_name, hor_name);
}

void ram_filler(const std::vector<std::string>& filler_cells)
{
  RamGen* ram_gen = ord::getRamGen();
  ram_gen->ramFiller(filler_cells);
}

void ram_routing()
{
  const int thread_count = ord::OpenRoad::openRoad()->getThreadCount();
  RamGen* ram_gen = ord::getRamGen();
  ram_gen->ramRouting(thread_count);
}

void set_behavioral_verilog_filename(const char* filename)
{
  RamGen* ram_gen = ord::getRamGen();
  ram_gen->setBehavioralVerilogFilename(filename);
}

} //namespace_ram

%} // inline

