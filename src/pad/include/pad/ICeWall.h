// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/isotropy.h"

namespace utl {
class Logger;
}

namespace pad {

class RDLRouter;
class RDLGui;

enum class PlacementStrategy
{
  DEFAULT,
  BUMP_ALIGNED,
  UNIFORM,
  LINEAR,
  PLACER
};

class ICeWall
{
 public:
  ICeWall(odb::dbDatabase* db, utl::Logger* logger);
  ~ICeWall();

  void makeBumpArray(odb::dbMaster* master,
                     const odb::Point& start,
                     int rows,
                     int columns,
                     int xpitch,
                     int ypitch,
                     const std::string& prefix = "BUMP_");
  void removeBump(odb::dbInst* inst);
  void removeBumpArray(odb::dbMaster* master);

  void assignBump(odb::dbInst* inst,
                  odb::dbNet* net,
                  odb::dbITerm* terminal = nullptr,
                  bool dont_route = false);

  void makeFakeSite(const std::string& name, int width, int height);
  odb::dbRow* findRow(const std::string& name) const;
  void makeIORow(odb::dbSite* horizontal_site,
                 odb::dbSite* vertical_site,
                 odb::dbSite* corner_site,
                 int west_offset,
                 int north_offset,
                 int east_offset,
                 int south_offset,
                 const odb::dbOrientType& rotation_hor,
                 const odb::dbOrientType& rotation_ver,
                 const odb::dbOrientType& rotation_cor,
                 int ring_index);
  void removeIORows();

  void placePad(odb::dbMaster* master,
                const std::string& name,
                odb::dbRow* row,
                int location,
                bool mirror);
  void placePads(const std::vector<odb::dbInst*>& insts,
                 odb::dbRow* row,
                 const PlacementStrategy& mode);
  void placeCorner(odb::dbMaster* master, int ring_index);
  void placeFiller(const std::vector<odb::dbMaster*>& masters,
                   odb::dbRow* row,
                   const std::vector<odb::dbMaster*>& overlapping_masters = {});
  void removeFiller(odb::dbRow* row);

  void placeBondPads(odb::dbMaster* bond,
                     const std::vector<odb::dbInst*>& pads,
                     const odb::dbOrientType& rotation = odb::dbOrientType::R0,
                     const odb::Point& offset = {0, 0},
                     const std::string& prefix = "IO_BOND_");
  void placeTerminals(const std::vector<odb::dbITerm*>& iterms,
                      bool allow_non_top_layer);
  void routeRDL(odb::dbTechLayer* layer,
                odb::dbTechVia* bump_via,
                odb::dbTechVia* pad_via,
                const std::vector<odb::dbNet*>& nets,
                int width = 0,
                int spacing = 0,
                bool allow45 = false,
                float turn_penalty = 2.0,
                int max_iterations = 10);
  void routeRDLDebugGUI(bool enable);
  void routeRDLDebugNet(const char* net);
  void routeRDLDebugPin(const char* pin);

  void connectByAbutment();

  static std::vector<std::pair<odb::dbITerm*, odb::dbITerm*>> getTouchingIterms(
      odb::dbInst* inst0,
      odb::dbInst* inst1);

 private:
  odb::dbBlock* getBlock() const;

  std::vector<odb::dbRow*> getRows() const;
  std::vector<odb::dbInst*> getPadInstsInRow(odb::dbRow* row) const;
  std::vector<odb::dbInst*> getPadInsts() const;

  void makeBTerm(odb::dbNet* net,
                 odb::dbTechLayer* layer,
                 const odb::Rect& shape) const;

  std::set<odb::dbNet*> connectByAbutment(
      const std::vector<std::pair<odb::dbITerm*, odb::dbITerm*>>& connections)
      const;

  void assertMasterType(odb::dbMaster* master,
                        const odb::dbMasterType& type) const;
  void assertMasterType(odb::dbInst* inst, const odb::dbMasterType& type) const;

  std::string getRowName(const std::string& name, int ring_index) const;
  odb::Direction2D::Value getRowEdge(odb::dbRow* row) const;

  // Data members
  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;

  std::map<odb::dbITerm*, odb::dbITerm*> routing_map_;

  std::unique_ptr<RDLRouter> router_;
  std::unique_ptr<RDLGui> router_gui_;
  odb::dbNet* rdl_net_debug_ = nullptr;
  odb::dbITerm* rdl_pin_debug_ = nullptr;

  constexpr static const char* kFakeLibraryName = "FAKE_IO";
  constexpr static const char* kRowNorth = "IO_NORTH";
  constexpr static const char* kRowSouth = "IO_SOUTH";
  constexpr static const char* kRowEast = "IO_EAST";
  constexpr static const char* kRowWest = "IO_WEST";
  constexpr static const char* kCornerNw = "IO_CORNER_NORTH_WEST";
  constexpr static const char* kCornerNe = "IO_CORNER_NORTH_EAST";
  constexpr static const char* kCornerSw = "IO_CORNER_SOUTH_WEST";
  constexpr static const char* kCornerSe = "IO_CORNER_SOUTH_EAST";
  constexpr static const char* kFillPrefix = "IO_FILL_";
};

}  // namespace pad
