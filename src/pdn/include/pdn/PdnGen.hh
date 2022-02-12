/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
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

#pragma once

#include <array>
#include <memory>
#include <map>
#include <regex>

#include "odb/db.h"
#include "utl/Logger.h"

#include "domain.h"
#include "grid.h"
#include "renderer.h"

namespace pdn {

using odb::dbMaster;
using odb::dbNet;
using odb::dbBlock;
using odb::dbInst;
using odb::dbBox;
using odb::dbMTerm;
using odb::dbDatabase;

using utl::Logger;

using std::regex;

class PdnGen {
public:
  PdnGen();
  ~PdnGen() {};

  void init(dbDatabase* db, Logger* logger);

  void setSpecialITerms();
  void setSpecialITerms(dbNet *net);

  void addGlobalConnect(const char* instPattern, const char* pinPattern, dbNet* net);
  void addGlobalConnect(dbBox* region, const char* instPattern, const char* pinPattern, dbNet* net);
  void clearGlobalConnect();

  void globalConnect(dbBlock* block);
  void globalConnect(dbBlock* block, std::shared_ptr<regex>& instPattern, std::shared_ptr<regex>& pinPattern, dbNet* net);
  void globalConnectRegion(dbBlock* block, dbBox* region, std::shared_ptr<regex>& instPattern, std::shared_ptr<regex>& pinPattern, dbNet* net);

  void reset();
  void report();

  // Domains
  std::vector<VoltageDomain*> getDomains();
  std::vector<VoltageDomain*> getConstDomains() const;
  VoltageDomain* findDomain(const std::string& name);
  void setCoreDomain(odb::dbNet* power, odb::dbNet* ground, const std::vector<odb::dbNet*>& secondary);
  void makeRegionVoltageDomain(const std::string& name, odb::dbNet* power, odb::dbNet* ground, const std::vector<odb::dbNet*>& secondary_nets, odb::dbRegion* region);

  // Grids
  void buildGrids(bool trim);
  std::vector<Grid*> findGrid(const std::string& name) const;
  void makeCoreGrid(VoltageDomain* domain, const std::string& name, bool starts_with_power, const std::vector<odb::dbTechLayer*>& pin_layers);
  void makeInstanceGrid(VoltageDomain* domain, const std::string& name, bool starts_with_power, odb::dbInst* inst, const std::array<int, 4>& halo);

  // Shapes
  void makeRing(Grid* grid, const std::array<Ring::Layer, 2>& layers, const std::array<int, 4>& offset, const std::array<int, 4>& pad_offset, bool extend, const std::vector<odb::dbTechLayer*>& pad_pin_layers);
  void makeFollowpin(Grid* grid, odb::dbTechLayer* layer, int width, Strap::Extend extend);
  void makeStrap(Grid* grid, odb::dbTechLayer* layer, int width, int spacing, int pitch, int offset, int number_of_straps, bool snap, bool use_grid_power_order, bool power_first,  Strap::Extend extend);
  void makeConnect(Grid* grid, odb::dbTechLayer* layer0, odb::dbTechLayer* layer1, int cut_pitch_x, int cut_pitch_y, const std::vector<odb::dbTechViaGenerateRule*>& vias, const std::vector<odb::dbTechVia*>& techvias);

  void writeToDb(bool add_pins) const;
  void ripUp(odb::dbNet* net);

  void toggleDebugRenderer(bool on);

private:
  using regexPairs = std::vector<std::pair<std::shared_ptr<regex>, std::shared_ptr<regex>>>;
  using netRegexPairs = std::map<dbNet*, std::shared_ptr<regexPairs>>;
  using regionNetRegexPairs = std::map<dbBox*, std::shared_ptr<netRegexPairs>>;

  void findInstsInArea(dbBlock* block, dbBox* region, std::shared_ptr<regex>& instPattern, std::vector<dbInst*>& insts);
  void buildMasterPinMatchingMap(dbBlock* block, std::shared_ptr<regex>& pinPattern, std::map<dbMaster*, std::vector<dbMTerm*>>& masterMap);

  void globalConnectRegion(dbBlock* block, dbBox* region, std::shared_ptr<netRegexPairs>);

  void rendererRedraw();
  void makeInitialObstructions(ShapeTreeMap& obs);
  void makeInitialShapes(ShapeTreeMap& shapes);
  void trimShapes();
  void cleanupVias();

  std::vector<Grid*> getGrids() const;

  odb::dbDatabase* db_;
  utl::Logger* logger_;

  std::unique_ptr<regionNetRegexPairs> global_connect_;

  std::unique_ptr<PDNRenderer> debug_renderer_;

  std::unique_ptr<CoreVoltageDomain> core_domain_;
  std::vector<std::unique_ptr<VoltageDomain>> domains_;
};

} // namespace
