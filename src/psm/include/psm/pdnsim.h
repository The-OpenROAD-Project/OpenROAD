// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "odb/dbBlockCallBackObj.h"
#include "odb/dbCompare.h"

namespace odb {
class dbDatabase;
class Point;
class dbNet;
class dbTechLayer;
}  // namespace odb

namespace sta {
class dbSta;
class Corner;
}  // namespace sta
namespace utl {
class Logger;
}
namespace rsz {
class Resizer;
}
namespace dpl {
class Opendp;
}

namespace psm {
class IRDropDataSource;
class IRSolver;

enum class GeneratedSourceType
{
  FULL,
  STRAPS,
  BUMPS
};

using odb::dbMaster;

class PDNSim : public odb::dbBlockCallBackObj
{
 public:
  struct GeneratedSourceSettings
  {
    // Bumps
    int bump_dx = 140;
    int bump_dy = 140;
    int bump_size = 70;
    int bump_interval = 3;

    // Straps
    int strap_track_pitch = 10;
  };

  using IRDropByPoint = std::map<odb::Point, double>;
  using IRDropByLayer = std::map<odb::dbTechLayer*, IRDropByPoint>;

  PDNSim();
  ~PDNSim() override;

  void init(utl::Logger* logger,
            odb::dbDatabase* db,
            sta::dbSta* sta,
            rsz::Resizer* resizer,
            dpl::Opendp* opendp);

  void setNetVoltage(odb::dbNet* net, sta::Corner* corner, double voltage);
  void setInstPower(odb::dbInst* inst, sta::Corner* corner, float power);
  void analyzePowerGrid(odb::dbNet* net,
                        sta::Corner* corner,
                        GeneratedSourceType source_type,
                        const std::string& voltage_file,
                        bool use_prev_solution,
                        bool enable_em,
                        const std::string& em_file,
                        const std::string& error_file,
                        const std::string& voltage_source_file);
  void writeSpiceNetwork(odb::dbNet* net,
                         sta::Corner* corner,
                         GeneratedSourceType source_type,
                         const std::string& spice_file,
                         const std::string& voltage_source_file);
  void getIRDropForLayer(odb::dbNet* net,
                         sta::Corner* corner,
                         odb::dbTechLayer* layer,
                         IRDropByPoint& ir_drop) const;
  bool checkConnectivity(odb::dbNet* net,
                         bool floorplanning,
                         const std::string& error_file,
                         bool require_bterm);
  void setDebugGui(bool enable);

  void clearSolvers();

  void setGeneratedSourceSettings(const GeneratedSourceSettings& settings);

  // from dbBlockCallBackObj
  void inDbPostMoveInst(odb::dbInst*) override;
  void inDbNetDestroy(odb::dbNet*) override;
  void inDbBTermPostConnect(odb::dbBTerm*) override;
  void inDbBTermPostDisConnect(odb::dbBTerm*, odb::dbNet*) override;
  void inDbBPinDestroy(odb::dbBPin*) override;
  void inDbSWireAddSBox(odb::dbSBox*) override;
  void inDbSWireRemoveSBox(odb::dbSBox*) override;
  void inDbSWirePostDestroySBoxes(odb::dbSWire*) override;

  void getIRDropForLayer(odb::dbNet* net,
                         odb::dbTechLayer* layer,
                         IRDropByPoint& ir_drop) const;

  // Functions of decap cells
  void addDecapMaster(dbMaster* decap_master, double decap_cap);
  void insertDecapCells(double target, const char* net_name);

 private:
  // Functions of decap cells
  odb::dbTechLayer* getLowestLayer(odb::dbNet* db_net);
  odb::dbNet* findPowerNet(const char* net_name);

  IRSolver* getIRSolver(odb::dbNet* net, bool floorplanning);

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  dpl::Opendp* opendp_ = nullptr;
  utl::Logger* logger_ = nullptr;

  std::unique_ptr<IRDropDataSource> heatmap_;

  bool debug_gui_enabled_ = false;

  GeneratedSourceSettings generated_source_settings_;

  std::map<odb::dbNet*, std::unique_ptr<IRSolver>> solvers_;
  std::map<odb::dbNet*, std::map<sta::Corner*, double>> user_voltages_;
  std::map<odb::dbInst*, std::map<sta::Corner*, float>> user_powers_;

  sta::Corner* last_corner_ = nullptr;
};
}  // namespace psm
