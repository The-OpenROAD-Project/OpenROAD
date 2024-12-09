/*
BSD 3-Clause License

Copyright (c) 2020, The Regents of the University of Minnesota

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "dpl/Opendp.h"
#include "odb/dbBlockCallBackObj.h"

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
                         const std::string& error_file);
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
