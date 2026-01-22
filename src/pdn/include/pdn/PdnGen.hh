// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "utl/Logger.h"

namespace pdn {

enum ExtensionMode
{
  CORE,
  RINGS,
  BOUNDARY,
  FIXED
};

enum StartsWith
{
  GRID,
  POWER,
  GROUND
};

enum PowerSwitchNetworkType
{
  STAR,
  DAISY
};

class VoltageDomain;
class Grid;
class PowerCell;
class PDNRenderer;
class SRoute;

class PdnGen
{
 public:
  PdnGen(odb::dbDatabase* db, utl::Logger* logger);
  ~PdnGen();

  void reset();
  void resetShapes();
  void report();

  // Power cells
  PowerCell* findSwitchedPowerCell(const std::string& name) const;
  void makeSwitchedPowerCell(odb::dbMaster* master,
                             odb::dbMTerm* control,
                             odb::dbMTerm* acknowledge,
                             odb::dbMTerm* switched_power,
                             odb::dbMTerm* alwayson_power,
                             odb::dbMTerm* ground);
  const std::vector<std::unique_ptr<PowerCell>>& getSwitchedPowerCells() const
  {
    return switched_power_cells_;
  }

  // Domains
  std::vector<VoltageDomain*> getDomains() const;
  VoltageDomain* findDomain(const std::string& name);
  void setCoreDomain(odb::dbNet* power,
                     odb::dbNet* switched_power,
                     odb::dbNet* ground,
                     const std::vector<odb::dbNet*>& secondary);
  void makeRegionVoltageDomain(const std::string& name,
                               odb::dbNet* power,
                               odb::dbNet* switched_power,
                               odb::dbNet* ground,
                               const std::vector<odb::dbNet*>& secondary_nets,
                               odb::dbRegion* region);

  // Grids
  void buildGrids(bool trim);
  std::vector<Grid*> findGrid(const std::string& name) const;
  void makeCoreGrid(VoltageDomain* domain,
                    const std::string& name,
                    StartsWith starts_with,
                    const std::vector<odb::dbTechLayer*>& pin_layers,
                    const std::vector<odb::dbTechLayer*>& generate_obstructions,
                    PowerCell* powercell,
                    odb::dbNet* powercontrol,
                    const char* powercontrolnetwork,
                    const std::vector<odb::dbTechLayer*>& pad_pin_layers);
  void makeInstanceGrid(
      VoltageDomain* domain,
      const std::string& name,
      StartsWith starts_with,
      odb::dbInst* inst,
      const std::array<int, 4>& halo,
      bool pg_pins_to_boundary,
      bool default_grid,
      const std::vector<odb::dbTechLayer*>& generate_obstructions,
      bool is_bump);
  void makeExistingGrid(
      const std::string& name,
      const std::vector<odb::dbTechLayer*>& generate_obstructions);

  // Shapes
  void makeRing(Grid* grid,
                odb::dbTechLayer* layer0,
                int width0,
                int spacing0,
                odb::dbTechLayer* layer1,
                int width1,
                int spacing1,
                StartsWith starts_with,
                const std::array<int, 4>& offset,
                const std::array<int, 4>& pad_offset,
                bool extend,
                const std::vector<odb::dbTechLayer*>& pad_pin_layers,
                const std::vector<odb::dbNet*>& nets,
                bool allow_out_of_die);
  void makeFollowpin(Grid* grid,
                     odb::dbTechLayer* layer,
                     int width,
                     ExtensionMode extend);
  void makeStrap(Grid* grid,
                 odb::dbTechLayer* layer,
                 int width,
                 int spacing,
                 int pitch,
                 int offset,
                 int number_of_straps,
                 bool snap,
                 StartsWith starts_with,
                 ExtensionMode extend,
                 const std::vector<odb::dbNet*>& nets,
                 bool allow_out_of_core);
  void makeConnect(
      Grid* grid,
      odb::dbTechLayer* layer0,
      odb::dbTechLayer* layer1,
      int cut_pitch_x,
      int cut_pitch_y,
      const std::vector<odb::dbTechViaGenerateRule*>& vias,
      const std::vector<odb::dbTechVia*>& techvias,
      int max_rows,
      int max_columns,
      const std::vector<odb::dbTechLayer*>& ongrid,
      const std::map<odb::dbTechLayer*, std::pair<int, bool>>& split_cuts,
      const std::string& dont_use_vias);

  void writeToDb(bool add_pins, const std::string& report_file = "") const;
  void ripUp(odb::dbNet* net);

  void setDebugRenderer(bool on);
  void rendererRedraw();
  void setAllowRepairChannels(bool allow);
  void filterVias(const std::string& filter);

  void checkSetup() const;

  void repairVias(const std::set<odb::dbNet*>& nets);

  void createSrouteWires(const char* net,
                         const char* outerNet,
                         odb::dbTechLayer* layer0,
                         odb::dbTechLayer* layer1,
                         int cut_pitch_x,
                         int cut_pitch_y,
                         const std::vector<odb::dbTechViaGenerateRule*>& vias,
                         const std::vector<odb::dbTechVia*>& techvias,
                         int max_rows,
                         int max_columns,
                         const std::vector<odb::dbTechLayer*>& ongrid,
                         const std::vector<int>& metalWidths,
                         const std::vector<int>& metalspaces,
                         const std::vector<odb::dbInst*>& insts);

  PDNRenderer* getDebugRenderer() const { return debug_renderer_.get(); }

 private:
  void trimShapes();
  void updateVias();
  void cleanupVias();

  void checkDesign(odb::dbBlock* block) const;

  std::vector<Grid*> getGrids() const;
  Grid* instanceGrid(odb::dbInst* inst) const;

  VoltageDomain* getCoreDomain() const;
  void ensureCoreDomain();

  void updateRenderer() const;

  bool importUPF(VoltageDomain* domain);
  bool importUPF(Grid* grid, PowerSwitchNetworkType type) const;

  odb::dbDatabase* db_;
  utl::Logger* logger_;

  std::unique_ptr<SRoute> sroute_;
  std::unique_ptr<PDNRenderer> debug_renderer_;

  std::unique_ptr<VoltageDomain> core_domain_;
  std::vector<std::unique_ptr<VoltageDomain>> domains_;
  std::vector<std::unique_ptr<PowerCell>> switched_power_cells_;
};

}  // namespace pdn
