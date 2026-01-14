// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbDatabaseObserver.h"
#include "sta/Sta.hh"

namespace ord {
class OpenRoad;
}

namespace utl {
class Logger;
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
class PatternMatch;
class TestCell;

using utl::Logger;

using odb::dbBlock;
using odb::dbBlockCallBackObj;
using odb::dbBTerm;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbITerm;
using odb::dbLib;
using odb::dbMaster;
using odb::dbNet;
using odb::dbTech;

// Handles registering and unregistering with dbSta
class dbStaState : public sta::StaState
{
 public:
  void init(dbSta* sta);
  ~dbStaState() override;

 protected:
  dbSta* sta_ = nullptr;
};

enum BufferUse
{
  DATA,
  CLOCK
};

class BufferUseAnalyser
{
 public:
  BufferUseAnalyser();

  BufferUse getBufferUse(sta::LibertyCell* buffer);

 private:
  std::unique_ptr<sta::PatternMatch> clkbuf_pattern_;
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

  dbDatabase* db() { return db_; }
  dbNetwork* getDbNetwork() { return db_network_; }
  dbStaReport* getDbReport() { return db_report_; }

  Slack netSlack(const dbNet* net, const MinMax* min_max);

  // From ord::OpenRoad::Observer
  void postReadLef(odb::dbTech* tech, odb::dbLib* library) override;
  void postReadDef(odb::dbBlock* block) override;
  void postReadDb(odb::dbDatabase* db) override;
  void postRead3Dbx(odb::dbChip* chip) override;

  // Find clock nets connected by combinational gates from the clock roots.
  std::set<dbNet*> findClkNets();
  std::set<dbNet*> findClkNets(const Clock* clk);

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
                            std::vector<dbInst*>& insts);
  void countPhysicalOnlyInstancesByType(InstTypeMap& inst_type_stats,
                                        std::vector<dbInst*>& insts);
  void addInstanceByTypeInstance(odb::dbInst* inst,
                                 InstTypeMap& inst_type_stats);

  void reportTimingHistogram(int num_bins, const MinMax* min_max) const;

  // Create a logic depth histogram report.
  void reportLogicDepthHistogram(int num_bins,
                                 bool exclude_buffers,
                                 bool exclude_inverters) const;

  BufferUse getBufferUse(sta::LibertyCell* buffer);

  // Sanity checkers
  int checkSanity();
  void checkSanityDrvrVertexEdges(const Pin* pin) const;
  void checkSanityDrvrVertexEdges(const odb::dbObject* term) const;
  void checkSanityDrvrVertexEdges() const;

  using Sta::netSlack;
  using Sta::replaceCell;

 private:
  void makeReport() override;
  void makeNetwork() override;
  void makeSdcNetwork() override;

  void replaceCell(Instance* inst,
                   Cell* to_cell,
                   LibertyCell* to_lib_cell) override;

  dbDatabase* db_ = nullptr;
  Logger* logger_ = nullptr;

  dbNetwork* db_network_ = nullptr;
  dbStaReport* db_report_ = nullptr;
  std::unique_ptr<dbStaCbk> db_cbk_;
  std::set<dbStaState*> sta_states_;

  std::unique_ptr<BufferUseAnalyser> buffer_use_analyser_;
};

// Utilities for TestCell

sta::LibertyPort* getLibertyScanEnable(const LibertyCell* lib_cell);
sta::LibertyPort* getLibertyScanIn(const LibertyCell* lib_cell);
sta::LibertyPort* getLibertyScanOut(const LibertyCell* lib_cell);

}  // namespace sta
