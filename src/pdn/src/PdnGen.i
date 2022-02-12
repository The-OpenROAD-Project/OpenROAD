/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
#include "pdn/PdnGen.hh"
#include "odb/db.h"
#include <array>
#include <regex>
#include <memory>
#include <vector>

namespace ord {
// Defined in OpenRoad.i
odb::dbDatabase* getDb();
pdn::PdnGen* getPdnGen();
utl::Logger* getLogger();
} // namespace ord

using std::regex;
using utl::PDN;

%}

%import <std_vector.i>
%import "dbtypes.i"
%include "../../Exception.i"

%include <std_string.i>

%typemap(in) pdn::Strap::Extend,  Strap::Extend {
  char *str = Tcl_GetStringFromObj($input, 0);
  if (strcasecmp(str, "Core") == 0) {
    $1 = pdn::Strap::Extend::CORE;
  } else if (strcasecmp(str, "Rings") == 0) {
    $1 = pdn::Strap::Extend::RINGS;
  } else if (strcasecmp(str, "Boundary") == 0) {
    $1 = pdn::Strap::Extend::BOUNDARY;
  } else {
    $1 = pdn::Strap::Extend::CORE;
  }
}

%inline %{

namespace pdn {

void
set_special_net_iterms() {
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->setSpecialITerms();
}

void
set_special_net_iterms(odb::dbNet* net) {
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->setSpecialITerms(net);
}

void
add_global_connect(const char* inst_pattern, const char* pin_pattern, odb::dbNet* net) {
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->addGlobalConnect(inst_pattern, pin_pattern, net);
}

void
clear_global_connect() {
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->clearGlobalConnect();
}

void
add_global_connect(odb::dbBlock* block, const char* region_name, const char* inst_pattern, const char* pin_pattern, odb::dbNet* net) {
  PdnGen* pdngen = ord::getPdnGen();
  
  odb::dbRegion* region = block->findRegion(region_name);
  if (region == nullptr) {
    ord::getLogger()->error(PDN, 53, "Region {} not found.", region_name);
    return;
  }
  
  for (dbBox* regionBox : region->getBoundaries())
    pdngen->addGlobalConnect(regionBox, inst_pattern, pin_pattern, net);
}

void
global_connect(odb::dbBlock* block) {
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->globalConnect(block);
}

void
global_connect(odb::dbBlock* block, const char* inst_pattern, const char* pin_pattern, odb::dbNet* net) {
  PdnGen* pdngen = ord::getPdnGen();
  
  std::shared_ptr<regex> instReg = std::make_shared<regex>(inst_pattern);
  std::shared_ptr<regex> pinReg = std::make_shared<regex>(pin_pattern);
  
  pdngen->globalConnect(block, instReg, pinReg, net);
}

void
global_connect_region(odb::dbBlock* block, const char* region_name, const char* inst_pattern, const char* pin_pattern, odb::dbNet* net) {
  PdnGen* pdngen = ord::getPdnGen();

  odb::dbRegion* region = block->findRegion(region_name);
  if (region == nullptr) {
    ord::getLogger()->error(PDN, 54, "Region {} not found.", region_name);
    return;
  }
  std::shared_ptr<regex> instReg = std::make_shared<regex>(inst_pattern);
  std::shared_ptr<regex> pinReg = std::make_shared<regex>(pin_pattern);
  
  for (dbBox* regionBox : region->getBoundaries())
    pdngen->globalConnectRegion(block, regionBox, instReg, pinReg, net);
}

void set_core_domain(odb::dbNet* power, odb::dbNet* ground, const std::vector<odb::dbNet*>& secondary_nets)
{
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->setCoreDomain(power, ground, secondary_nets);
}

void make_region_domain(const std::string& name, odb::dbNet* power, odb::dbNet* ground, const std::vector<odb::dbNet*>& secondary_nets, odb::dbRegion* region)
{
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->makeRegionVoltageDomain(name, power, ground, secondary_nets, region);
}

void reset()
{
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->reset();
}

void build_grids(bool trim = true)
{
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->buildGrids(trim);
}

void make_core_grid(pdn::VoltageDomain* domain, const std::string& name, bool starts_with_power, const std::vector<odb::dbTechLayer*>& pin_layers)
{
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->makeCoreGrid(domain, name, starts_with_power, pin_layers);
}

void make_instance_grid(pdn::VoltageDomain* domain, const std::string& name, bool starts_with_power, odb::dbInst* inst, int x0, int y0, int x1, int y1)
{
  PdnGen* pdngen = ord::getPdnGen();
  std::array<int, 4> halo{x0, y0, x1, y1};
  pdngen->makeInstanceGrid(domain, name, starts_with_power, inst, halo);
}

void make_ring(const std::string& grid_name, 
               odb::dbTechLayer* l0, 
               int width0, 
               int spacing0, 
               odb::dbTechLayer* l1, 
               int width1, 
               int spacing1, 
               int core_offset_x0,
               int core_offset_y0,
               int core_offset_x1,
               int core_offset_y1, 
               int pad_offset_x0,
               int pad_offset_y0,
               int pad_offset_x1,
               int pad_offset_y1, 
               bool extend,
               const std::vector<odb::dbTechLayer*>& pad_pin_layers)
{
  PdnGen* pdngen = ord::getPdnGen();
  for (auto* grid : pdngen->findGrid(grid_name)) {
    pdngen->makeRing(grid,
                     {Ring::Layer{l0, width0, spacing0},
                      Ring::Layer{l1, width1, spacing1}},
                     {core_offset_x0, core_offset_y0, core_offset_x1, core_offset_y1},
                     {pad_offset_x0, pad_offset_y0, pad_offset_x1, pad_offset_y1},
                     extend,
                     pad_pin_layers);
  }
}

void make_followpin(const std::string& grid_name, 
                    odb::dbTechLayer* layer, 
                    int width, 
                    pdn::Strap::Extend extend)
{
  PdnGen* pdngen = ord::getPdnGen();
  for (auto* grid : pdngen->findGrid(grid_name)) {
    pdngen->makeFollowpin(grid, layer, width, extend);
  }
}

void make_strap(const std::string& grid_name, 
                odb::dbTechLayer* layer, 
                int width, 
                int spacing, 
                int pitch, 
                int offset, 
                int number_of_straps, 
                bool snap,
                bool use_grid_power_order,
                bool starts_with_power,
                pdn::Strap::Extend extend)
{
  PdnGen* pdngen = ord::getPdnGen();
  for (auto* grid : pdngen->findGrid(grid_name)) {
    pdngen->makeStrap(grid, layer, width, spacing, pitch, offset, number_of_straps, snap, use_grid_power_order, starts_with_power, extend);
  }
}

void make_connect(const std::string& grid_name, 
                  odb::dbTechLayer* layer0, 
                  odb::dbTechLayer* layer1, 
                  int cut_pitch_x, 
                  int cut_pitch_y, 
                  const std::vector<odb::dbTechViaGenerateRule*>& vias, 
                  const std::vector<odb::dbTechVia*>& techvias)
{
  PdnGen* pdngen = ord::getPdnGen();
  for (auto* grid : pdngen->findGrid(grid_name)) {
    pdngen->makeConnect(grid, layer0, layer1, cut_pitch_x, cut_pitch_y, vias, techvias);
  }
}

void debug_renderer(bool on)
{
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->toggleDebugRenderer(on);
}

void write_to_db(bool add_pins)
{
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->writeToDb(add_pins);
}

void rip_up(odb::dbNet* net = nullptr)
{
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->ripUp(net);
}

void report()
{
  PdnGen* pdngen = ord::getPdnGen();
  pdngen->report();
}

pdn::VoltageDomain* find_domain(const std::string& name)
{
  PdnGen* pdngen = ord::getPdnGen();
  return pdngen->findDomain(name);
}

} // namespace

%} // inline
