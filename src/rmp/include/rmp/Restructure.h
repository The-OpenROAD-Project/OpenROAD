/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
#include <iostream>
#include <string>

#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"

namespace abc {
}  // namespace abc

namespace dst {
class Distributed;
}
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
  DELAY_4,
  DELAY_5,
  DELAY_6,
  DELAY_7
};

const int MAX_ITERATIONS = 3;

class RestructureCallBack;

class Restructure
{
 public:
  Restructure() = default;
  ~Restructure();

  void init(utl::Logger* logger,
            sta::dbSta* open_sta,
            odb::dbDatabase* db,
            dst::Distributed* dist,
            rsz::Resizer* resizer);
  void reset();
  void run(char* liberty_file_name,
           float slack_threshold,
           unsigned max_depth,
           char* workdir_name,
           char* abc_logfile,
           const char* post_abc_script);

  void setMode(const char* mode_name);
  void setTieLoPort(sta::LibertyPort* loport);
  void setTieHiPort(sta::LibertyPort* hiport);
  void runABCJob(const Mode mode,
                 const ushort iterations,
                 int& num_instances,
                 int& level_gain,
                 float& delay,
                 std::string& blif_path);
  void addLibFile(const std::string& lib_file);
  void setDistributed(const std::string& host, unsigned short port);

 private:
  void deleteComponents();
  void getBlob(unsigned max_depth);
  void runABC();
  bool writeAbcScript(std::string file_name,
                      Mode mode,
                      const ushort iterations);
  void writeOptCommands(std::ofstream& script,
                        Mode mode,
                        const ushort iterations);
  void initDB();
  void getEndPoints(sta::PinSet& ends, bool area_mode, unsigned max_depth);
  int countConsts(odb::dbBlock* top_block);
  void removeConstCells();
  void removeConstCell(odb::dbInst* inst);
  bool readAbcLog(std::string abc_file_name, int& level_gain, float& delay_val);

  Logger* logger_;
  std::string logfile_;
  std::string locell_;
  std::string loport_;
  std::string hicell_;
  std::string hiport_;
  std::string work_dir_name_;
  std::string post_abc_script_;
  std::string worst_vertix_;

  // db vars
  sta::dbSta* open_sta_;
  odb::dbDatabase* db_;
  rsz::Resizer* resizer_;
  odb::dbBlock* block_ = nullptr;

  std::string input_blif_file_name_;
  std::string output_blif_file_name_;
  std::vector<std::string> lib_file_names_;
  std::set<odb::dbInst*> path_insts_;

  Mode opt_mode_;
  bool is_area_mode_;

  // dst vars
  dst::Distributed* dist_;
  std::string dist_host_;
  unsigned short dist_port_;
  bool use_cloud_{false};
  friend class RestructureCallBack;
};

inline std::ostream& operator<<(std::ostream& out, const Mode& c)
{
  switch (c) {
    case Mode::AREA_1:
      out << "AREA_1";
      break;
    case Mode::AREA_2:
      out << "AREA_2";
      break;
    case Mode::AREA_3:
      out << "AREA_3";
      break;
    case Mode::DELAY_1:
      out << "DELAY_1";
      break;
    case Mode::DELAY_2:
      out << "DELAY_2";
      break;
    case Mode::DELAY_3:
      out << "DELAY_3";
      break;
    case Mode::DELAY_4:
      out << "DELAY_4";
      break;
    case Mode::DELAY_5:
      out << "DELAY_5";
      break;
    case Mode::DELAY_6:
      out << "DELAY_6";
      break;
    case Mode::DELAY_7:
      out << "DELAY_7";
      break;
  }
  return out;
}

}  // namespace rmp
