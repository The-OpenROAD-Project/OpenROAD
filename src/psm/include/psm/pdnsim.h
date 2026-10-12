// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"

namespace odb {
class dbDatabase;
class Point;
class dbNet;
class dbTechLayer;
}  // namespace odb

namespace sta {
class dbSta;
class Scene;
}  // namespace sta
namespace utl {
class Logger;
}
namespace est {
class EstimateParasitics;
}
namespace dpl {
class Opendp;
}
namespace web {
class HeatMapSourceRegistration;
using HeatMapSourceHandle = std::shared_ptr<HeatMapSourceRegistration>;
}  // namespace web

namespace psm {
class IRDropDataSource;
class IRSolver;
class IRSolver3D;
class IRNetwork3D;

enum class GeneratedSourceType
{
  kFull,
  kStraps,
  kBumps
};

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

    // Source resistance
    float resistance = 0.0;  // Ohms
  };

  using IRDropByPoint = std::map<odb::Point, double>;
  using IRDropByLayer = odb::PtrMap<odb::dbTechLayer, IRDropByPoint>;

  PDNSim(utl::Logger* logger,
         odb::dbDatabase* db,
         sta::dbSta* sta,
         est::EstimateParasitics* estimate_parasitics,
         dpl::Opendp* opendp);
  ~PDNSim() override;

  void setNetVoltage(odb::dbNet* net, sta::Scene* corner, double voltage);
  void setInstPower(odb::dbInst* inst, sta::Scene* corner, float power);
  void analyzePowerGrid(odb::dbNet* net,
                        sta::Scene* corner,
                        GeneratedSourceType source_type,
                        const std::string& voltage_file,
                        bool use_prev_solution,
                        bool enable_em,
                        const std::string& em_file,
                        const std::string& error_file,
                        const std::string& voltage_source_file);
  void writeSpiceNetwork(odb::dbNet* net,
                         sta::Scene* corner,
                         GeneratedSourceType source_type,
                         const std::string& spice_file,
                         const std::string& voltage_source_file);
  void getIRDropForLayer(odb::dbNet* net,
                         sta::Scene* corner,
                         odb::dbTechLayer* layer,
                         IRDropByPoint& ir_drop) const;
  bool checkConnectivity(odb::dbNet* net,
                         bool floorplanning,
                         const std::string& error_file,
                         bool require_bterm);
  bool check3DPowerGrid(const std::string& chip_net_name,
                        bool floorplanning = false,
                        const std::string& error_file = "",
                        bool require_bterm = true);
  bool check3DGMatrix(const std::string& chip_net_name);
  void add3DPDNCurrent(const std::string& chip_net_name,
                       const std::string& chip_name,
                       const std::string& port_name,
                       double current);
  bool check3DJVector(const std::string& chip_net_name, bool use_sta = true);
  void setChipletVoltage(const std::string& chip_net_name,
                         const std::string& chip_name,
                         const std::string& port_name,
                         std::optional<double> voltage,
                         sta::Scene* corner,
                         std::optional<double> nominal_voltage = std::nullopt);
  bool analyze3DPowerGrid(const std::string& chip_net_name,
                          sta::Scene* corner,
                          bool use_sta = true,
                          const std::string& error_file = "",
                          const std::string& voltage_file = "");
  double get3DPDNVoltage(const std::string& chip_net_name,
                         const std::string& chip_name,
                         const std::string& port_name);
  void setDebugGui(bool enable);

  void clearSolvers();
  void clear3DPowerGrid();

  void setGeneratedSourceSettings(const GeneratedSourceSettings& settings);

  // from dbBlockCallBackObj
  void inDbPostMoveInst(odb::dbInst*) override;
  void inDbNetDestroy(odb::dbNet*) override;
  void inDbBTermPostConnect(odb::dbBTerm*) override;
  void inDbBTermPostDisConnect(odb::dbBTerm*, odb::dbNet*) override;
  void inDbBPinCreate(odb::dbBPin*) override;
  void inDbBPinAddBox(odb::dbBox*) override;
  void inDbBPinRemoveBox(odb::dbBox*) override;
  void inDbBPinDestroy(odb::dbBPin*) override;
  void inDbSWireAddSBox(odb::dbSBox*) override;
  void inDbSWireRemoveSBox(odb::dbSBox*) override;
  void inDbSWirePostDestroySBoxes(odb::dbSWire*) override;
  void inDbFillCreate(odb::dbFill*) override;

  void getIRDropForLayer(odb::dbNet* net,
                         odb::dbTechLayer* layer,
                         IRDropByPoint& ir_drop) const;

  // Functions of decap cells
  void addDecapMaster(odb::dbMaster* decap_master, double decap_cap);
  void insertDecapCells(double target, const char* net_name);

  odb::dbNet* getLastAnalyzedNet() const { return last_net_; }
  sta::Scene* getLastAnalyzedCorner() const { return last_corner_; }

 private:
  class BlockObserver3D;
  void observe3DBlock(odb::dbBlock* block);
  std::vector<std::unique_ptr<BlockObserver3D>> block_observers_3d_;

  struct Solution3D
  {
    std::unique_ptr<IRSolver3D> solver;
    odb::PtrMap<odb::dbInst, std::map<sta::Scene*, float>> powers;
    odb::PtrMap<odb::dbNet, std::map<sta::Scene*, double>> voltages;
  };

  struct CurrentLoad3D
  {
    std::string chip_name;
    std::string terminal;
    double current;
  };

  struct VoltageSource3D
  {
    std::string chip_name;
    std::string terminal;
    double voltage;
    sta::Scene* corner;
  };

  // Functions of decap cells
  odb::dbTechLayer* getLowestLayer(odb::dbNet* db_net);
  odb::dbNet* findPowerNet(const char* net_name);
  odb::dbChipNet* findChipNet(const std::string& chip_net_name) const;
  odb::dbChipInst* find3DChip(odb::dbChipNet* net,
                              const std::string& chip_name,
                              const std::string& terminal) const;
  std::unique_ptr<IRSolver3D> make3DSolver(odb::dbChipNet* net,
                                           sta::Scene* corner,
                                           bool use_sta);

  void check3DConnectivity(IRNetwork3D& network,
                           bool floorplanning,
                           bool require_bterm,
                           const std::string& error_file);

  IRSolver* getIRSolver(odb::dbNet* net, bool floorplanning);

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  dpl::Opendp* opendp_ = nullptr;
  utl::Logger* logger_ = nullptr;

  web::HeatMapSourceHandle heatmap_source_;

  bool debug_gui_enabled_ = false;

  GeneratedSourceSettings generated_source_settings_;

  odb::PtrMap<odb::dbNet, std::unique_ptr<IRSolver>> solvers_;
  odb::PtrMap<odb::dbChipNet, Solution3D> solvers_3d_;
  std::map<std::string, std::vector<CurrentLoad3D>> user_currents_3d_;
  std::map<std::string, std::vector<VoltageSource3D>> user_voltage_sources_3d_;
  odb::PtrMap<odb::dbNet, std::map<sta::Scene*, double>> user_voltages_;
  odb::PtrMap<odb::dbInst, std::map<sta::Scene*, float>> user_powers_;

  odb::dbNet* last_net_ = nullptr;
  sta::Scene* last_corner_ = nullptr;
};
}  // namespace psm
