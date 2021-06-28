/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
// All rights reserved.
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

#include <functional>
#include <string>

#include "db_sta/dbSta.hh"

namespace ord {
class OpenRoad;
}  // namespace ord

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
class dbBlock;
class dbInst;
class dbNet;
class dbITerm;
}  // namespace odb

namespace sta {
class dbSta;
}  // namespace sta

namespace rmp {

using utl::Logger;

enum class Mode
{
  AREA_1 = 0,
  AREA_2,
  AREA_3,
  DELAY_1,
  DELAY_2,
  DELAY_3,
  DELAY_4
};

class Restructure
{
 public:
  Restructure() = default;
  ~Restructure();

  void init(ord::OpenRoad* openroad);
  void reset();
  void run(float slack_threshold, unsigned max_depth);

  void setMode(const char* mode_name);
  void setLogfile(const char* file_name);
  void setTieLoPort(sta::LibertyPort* loport);
  void setTieHiPort(sta::LibertyPort* hiport);

 private:
  void makeComponents();
  void deleteComponents();
  void getBlob(unsigned max_depth);
  void runABC();
  void postABC(float worst_slack);
  bool writeAbcScript(std::string file_name);
  void writeOptCommands(std::ofstream& script);
  void initDB();
  void getEndPoints(sta::PinSet& ends, bool area_mode, unsigned max_depth);
  int countConsts(odb::dbBlock* top_block);
  void removeConstCells();
  void removeConstCell(odb::dbInst* inst);

  Logger* logger_;
  std::string logfile_;
  std::string locell_;
  std::string loport_;
  std::string hicell_;
  std::string hiport_;

  // db vars
  sta::dbSta* open_sta_;
  odb::dbDatabase* db_;
  odb::dbBlock* block_ = nullptr;

  std::string input_blif_file_name_;
  std::string output_blif_file_name_;
  std::vector<std::string> lib_file_names_;
  std::set<odb::dbInst*> path_insts_;

  Mode opt_mode_;
};

}  // namespace rmp
