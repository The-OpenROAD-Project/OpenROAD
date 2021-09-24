/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdio.h>
#include <unistd.h>

#include <set>
#include <map>
#include <string>

namespace ord {
class OpenRoad;
}  // namespace ord

namespace odb {
class dbInst;
class dbBlock;
}  // namespace odb

namespace utl {
class Logger;
}

namespace sta {
class dbSta;
class Pin;
}  // namespace sta

using utl::Logger;

namespace rmp {

class Blif
{
 public:
  Blif(Logger* logger,
       sta::dbSta* sta,
       const std::string& const0_cell_,
       const std::string& const0_cell_port_,
       const std::string& const1_cell_,
       const std::string& const1_cell_port_);
  void setReplaceableInstances(std::set<odb::dbInst*>& insts);
  void addReplaceableInstance(odb::dbInst* inst);
  bool writeBlif(const char* file_name, bool write_arrival_requireds = false);
  bool readBlif(const char* file_name, odb::dbBlock* block);
  bool inspectBlif(const char* file_name, int& num_instances);
  float getRequiredTime(sta::Pin* term, bool is_rise);
  float getArrivalTime(sta::Pin* term, bool is_rise);
  void addArrival(sta::Pin* pin, std::string netName);
  void addRequired(sta::Pin* pin, std::string netName);
  static int call_id_;

 private:
  std::set<odb::dbInst*> instances_to_optimize;
  Logger* logger_;
  sta::dbSta* open_sta_ = nullptr;
  std::string const0_cell_;
  std::string const0_cell_port_;
  std::string const1_cell_;
  std::string const1_cell_port_;
  std::map<std::string, std::pair<float,float>> requireds_;
  std::map<std::string, std::pair<float,float>> arrivals_;
};

}  // namespace rmp
