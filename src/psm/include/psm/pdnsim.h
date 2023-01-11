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
#include <string>

namespace odb {
class dbDatabase;
class Point;
class dbTechLayer;
}  // namespace odb

namespace sta {
class dbSta;
}
namespace utl {
class Logger;
}

namespace psm {
class IRDropDataSource;
class DebugGui;

class PDNSim
{
 public:
  using IRDropByPoint = std::map<odb::Point, double>;
  using IRDropByLayer = std::map<odb::dbTechLayer*, IRDropByPoint>;

  PDNSim();
  ~PDNSim();

  void init(utl::Logger* logger, odb::dbDatabase* db, sta::dbSta* sta);

  void import_vsrc_cfg(std::string vsrc);
  void import_out_file(std::string out_file);
  void import_em_out_file(std::string em_out_file);
  void import_enable_em(bool enable_em);
  void import_spice_out_file(std::string out_file);
  void set_power_net(std::string net);
  void set_bump_pitch_x(float bump_pitch);
  void set_bump_pitch_y(float bump_pitch);
  void set_node_density(float node_density);
  void set_node_density_factor(int node_density_factor);
  void set_pdnsim_net_voltage(std::string net, float voltage);
  void analyze_power_grid();
  void write_pg_spice();
  void getIRDropMap(IRDropByLayer& ir_drop);
  void getIRDropForLayer(odb::dbTechLayer* layer, IRDropByPoint& ir_drop);
  int getMinimumResolution();
  bool check_connectivity();
  void setDebugGui();

 private:
  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  utl::Logger* logger_;
  std::string vsrc_loc_;
  std::string out_file_;
  std::string em_out_file_;
  bool enable_em_;
  int bump_pitch_x_;
  int bump_pitch_y_;
  std::string spice_out_file_;
  std::string power_net_;
  std::map<std::string, float> net_voltage_map_;
  IRDropByLayer ir_drop_;
  float node_density_;
  int node_density_factor_;
  float min_resolution_;
  std::unique_ptr<DebugGui> debug_gui_;
  std::unique_ptr<IRDropDataSource> heatmap_;
};
}  // namespace psm
