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

#ifndef __PDNSim_HEADER__
#define __PDNSim_HEADER__

#include <string>
#include <map>

namespace odb {
class dbDatabase;
}
namespace sta {
class dbSta;
}
namespace utl {
class Logger;
}

namespace psm {

class PDNSim
{
 public:
  PDNSim();
  ~PDNSim();

  void init(utl::Logger* logger, odb::dbDatabase* db, sta::dbSta* sta);
  void reset();

  void import_vsrc_cfg(std::string vsrc);
  void import_out_file(std::string out_file);
  void import_em_out_file(std::string em_out_file);
  void import_enable_em(int enable_em);
  void import_spice_out_file(std::string out_file);
  void set_power_net(std::string net);
  void set_bump_pitch_x(float bump_pitch);
  void set_bump_pitch_y(float bump_pitch);
  void set_pdnsim_net_voltage(std::string net, float voltage);
  int  analyze_power_grid();
  void write_pg_spice();

  int check_connectivity();

 private:
  odb::dbDatabase*             _db;
  sta::dbSta*                  _sta;
  utl::Logger*                 _logger;
  std::string                  _vsrc_loc;
  std::string                  _out_file;
  std::string                  _em_out_file;
  int                          _enable_em;
  int                          _bump_pitch_x;
  int                          _bump_pitch_y;
  std::string                  _spice_out_file;
  std::string                  _power_net;
  std::map<std::string, float> _net_voltage_map;
};
}  // namespace psm

#endif
