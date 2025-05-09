// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"

namespace abc {
}  // namespace abc

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

  void init(utl::Logger* logger,
            sta::dbSta* open_sta,
            odb::dbDatabase* db,
            rsz::Resizer* resizer);
  void reset();
  void run(char* liberty_file_name,
           float slack_threshold,
           unsigned max_depth,
           char* workdir_name,
           char* abc_logfile);

  void setMode(const char* mode_name);
  void setTieLoPort(sta::LibertyPort* loport);
  void setTieHiPort(sta::LibertyPort* hiport);

 private:
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
  bool readAbcLog(std::string abc_file_name, int& level_gain, float& delay_val);

  Logger* logger_;
  std::string logfile_;
  std::string locell_;
  std::string loport_;
  std::string hicell_;
  std::string hiport_;
  std::string work_dir_name_;

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
  int blif_call_id_{0};
};

}  // namespace rmp
