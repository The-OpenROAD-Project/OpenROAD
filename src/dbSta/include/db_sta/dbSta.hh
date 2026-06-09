// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "db_sta/DelayFmt.hh"  // IWYU pragma: keep
#include "db_sta/dbNetwork.hh"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbDatabaseObserver.h"
#include "odb/dbObject.h"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

namespace ord {
class OpenRoad;
}

namespace sta {

// std::any and typeid do not work on incomplete types
// talking to the OpenSTA author about implementing these
// upstream instead. https://github.com/llvm/llvm-project/issues/36746
// for llvm bug.
//
// Deleting all the constructors to preserve behavior as opaque pointers.
// This should let RTTI based constructs like std::any to work on these
// types. See
// https://github.com/The-OpenROAD-Project/OpenROAD/pull/7725#discussion_r2201423922
// for more information.
class Library
{
 public:
  Library() = delete;
};
class Cell
{
 public:
  Cell() = delete;
};
class Port
{
 public:
  Port() = delete;
};
class Instance
{
 public:
  Instance() = delete;
};
class Pin
{
 public:
  Pin() = delete;
};
class Term
{
 public:
  Term() = delete;
};
class Net
{
 public:
  Net() = delete;
};
class ViewType
{
 public:
  ViewType() = delete;
};

class dbSta;
class dbNetwork;
class dbStaReport;
class dbStaCbk;
class DbStaLevelizeObserver;
class PatternMatch;
class TestCell;

// Handles registering and unregistering with dbSta
class dbStaState : public sta::StaState
{
 public:
  void init(dbSta* sta);
  ~dbStaState() override;

 protected:
  dbSta* sta_ = nullptr;
};

class dbSta : public Sta, public odb::dbDatabaseObserver
{
 public:
  dbSta(Tcl_Interp* tcl_interp, odb::dbDatabase* db, utl::Logger* logger);
  ~dbSta() override;

  enum InstType
  {
    BLOCK,
    PAD,
    PAD_INPUT,
    PAD_OUTPUT,
    PAD_INOUT,
    PAD_POWER,
    PAD_SPACER,
    PAD_AREAIO,
    ENDCAP,
    FILL,
    TAPCELL,
    BUMP,
    COVER,
    ANTENNA,
    TIE,
    LEF_OTHER,
    STD_CELL,
    STD_BUF,
    STD_BUF_CLK_TREE,
    STD_BUF_TIMING_REPAIR,
    STD_INV,
    STD_INV_CLK_TREE,
    STD_INV_TIMING_REPAIR,
    STD_CLOCK_GATE,
    STD_LEVEL_SHIFT,
    STD_SEQUENTIAL,
    STD_PHYSICAL,
    STD_COMBINATIONAL,
    STD_OTHER
  };

  // Report Instances Type
  struct TypeStats
  {
    int count{0};
    int64_t area{0};
  };
  using InstTypeMap = std::map<InstType, TypeStats>;

  void initVars(Tcl_Interp* tcl_interp,
                odb::dbDatabase* db,
                utl::Logger* logger);

  // Creates a dbSta instance for the given dbBlock using the same context as
  // this dbSta instance (e.g. TCL interpreter, units, etc.)
  std::unique_ptr<dbSta> makeBlockSta(odb::dbBlock* block);

  odb::dbDatabase* db() { return db_; }
  dbNetwork* getDbNetwork() { return db_network_; }
  dbStaReport* getDbReport() { return db_report_; }

  float slack(const odb::dbNet* net, const MinMax* min_max);

  // From ord::OpenRoad::Observer
  void postReadLef(odb::dbTech* tech, odb::dbLib* library) override;
  void postReadDef(odb::dbBlock* block) override;
  void postReadDb(odb::dbDatabase* db) override;
  void postRead3Dbx(odb::dbChip* chip) override;

  // Find clock nets connected by combinational gates from the clock roots.
  odb::PtrSet<odb::dbNet> findClkNets();
  odb::PtrSet<odb::dbNet> findClkNets(const Clock* clk);

  void deleteInstance(Instance* inst) override;
  void deleteNet(Net* net) override;
  void connectPin(Instance* inst, Port* port, Net* net) override;
  void connectPin(Instance* inst, LibertyPort* port, Net* net) override;
  void disconnectPin(Pin* pin) override;

  void updateComponentsState() override;
  void registerStaState(dbStaState* state);
  void unregisterStaState(dbStaState* state);

  std::string getInstanceTypeText(InstType type) const;
  InstType getInstanceType(odb::dbInst* inst);
  void reportCellUsage(odb::dbModule* module,
                       bool verbose,
                       const char* file_name,
                       const char* stage_name);
  void countInstancesByType(odb::dbModule* module,
                            InstTypeMap& inst_type_stats,
                            std::vector<odb::dbInst*>& insts);
  void countPhysicalOnlyInstancesByType(InstTypeMap& inst_type_stats,
                                        std::vector<odb::dbInst*>& insts);
  void addInstanceByTypeInstance(odb::dbInst* inst,
                                 InstTypeMap& inst_type_stats);

  // bin_size: fixed bin width in user time units (e.g., ns).
  //           If 0.0, num_bins is used to determine bin width automatically.
  void reportTimingHistogram(int num_bins,
                             const MinMax* min_max,
                             float bin_size = 0.0) const;

  // Create a logic depth histogram report.
  void reportLogicDepthHistogram(int num_bins,
                                 bool exclude_buffers,
                                 bool exclude_inverters) const;

  // Get the levels of logic for all endpoints.
  std::vector<int> levelsOfLogic(bool exclude_buffers,
                                 bool exclude_inverters) const;

  utl::Logger* getLogger() { return logger_; }

  // Sanity checkers
  int checkSanity();
  void checkSanityNetlistConsistency() const;
  void checkSanityDrvrVertexEdges(const Pin* pin) const;
  void checkSanityDrvrVertexEdges(const odb::dbObject* term) const;
  void checkSanityDrvrVertexEdges() const;

  // Debugging utilities
  void dumpModInstPinSlacks(const char* mod_inst_name,
                            const char* filename,
                            const MinMax* min_max = MinMax::max());
  void dumpModInstGraphConnections(const char* mod_inst_name,
                                   const char* filename);

  using Sta::replaceCell;
  using Sta::slack;

  // Drivers sorted by (level, name) for determinism.
  const VertexSeq& levelizedDrvrVertices();
  // Discard the cached driver-vertex list. Callers that mutate the timing
  // graph outside the LevelizeObserver path (e.g. dbStaCbk on dbInst
  // create/destroy) must invalidate so the next query rebuilds.
  void invalidateLevelizedDrvrVertices();

 private:
  void makeReport() override;
  void makeNetwork() override;
  void makeSdcNetwork() override;
  void makeObservers() override;

  void replaceCell(Instance* inst,
                   Cell* to_cell,
                   LibertyCell* to_lib_cell) override;

  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;

  dbNetwork* db_network_ = nullptr;
  dbStaReport* db_report_ = nullptr;
  std::unique_ptr<dbStaCbk> db_cbk_;
  std::set<dbStaState*> sta_states_;

  VertexSeq levelized_drvr_vertices_;
  bool drvr_vertices_level_valid_ = false;
};

}  // namespace sta
