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
class IRSolver;
class IRDropDataSource;
class DebugGui;

class PDNSim
{
 public:
  using IRDropByPoint = std::map<odb::Point, double>;
  using IRDropByLayer = std::map<odb::dbTechLayer*, IRDropByPoint>;

  PDNSim();
  ~PDNSim();

  void init(utl::Logger* logger,
            odb::dbDatabase* db,
            sta::dbSta* sta,
            rsz::Resizer* resizer);

  void setVsrcCfg(const std::string& vsrc);
  void setNet(odb::dbNet* net) { net_ = net; }
  void setBumpPitchX(float bump_pitch) { bump_pitch_x_ = bump_pitch; }
  void setBumpPitchY(float bump_pitch) { bump_pitch_y_ = bump_pitch; }
  void setNodeDensity(float node_density) { node_density_ = node_density; }
  void setNodeDensityFactor(int node_density_factor)
  {
    node_density_factor_ = node_density_factor;
  }
  void setCorner(sta::Corner* corner) { corner_ = corner; }

  void setNetVoltage(odb::dbNet* net, float voltage);
  void analyzePowerGrid(const std::string& voltage_file,
                        bool enable_em,
                        const std::string& em_file,
                        const std::string& error_file);
  void writeSpice(const std::string& file);
  void getIRDropMap(IRDropByLayer& ir_drop);
  void getIRDropForLayer(odb::dbTechLayer* layer, IRDropByPoint& ir_drop);
  int getMinimumResolution();
  bool checkConnectivity(const std::string& error_file);
  void setDebugGui();

 private:
  std::optional<float> getNetVoltage(odb::dbNet* net,
                                     bool require_voltage) const;
  std::unique_ptr<IRSolver> getIRSolver(bool require_voltage);

  void saveIRDrop(IRSolver* ir_solver);

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  utl::Logger* logger_ = nullptr;
  std::string vsrc_loc_;
  int bump_pitch_x_ = 0;
  int bump_pitch_y_ = 0;
  odb::dbNet* net_ = nullptr;
  sta::Corner* corner_ = nullptr;
  std::map<odb::dbNet*, float> net_voltage_map_;
  IRDropByLayer ir_drop_;
  float node_density_ = -1;
  int node_density_factor_ = 0;
  float min_resolution_ = -1;
  std::unique_ptr<DebugGui> debug_gui_;
  std::unique_ptr<IRDropDataSource> heatmap_;
};
}  // namespace psm
