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

#include "rmp/Restructure.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "rmp/blif.h"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

using utl::RMP;

namespace rmp {

void Restructure::init(ord::OpenRoad* openroad)
{
  logger_ = openroad->getLogger();
  db_ = openroad->getDb();
  open_sta_ = openroad->getSta();
  resizer_ = openroad->getResizer();
}

void Restructure::deleteComponents()
{
}

Restructure::~Restructure()
{
  deleteComponents();
}

void Restructure::reset()
{
  lib_file_names_.clear();
  path_insts_.clear();
}

void Restructure::run(char* liberty_file_name,
                      float slack_threshold,
                      unsigned max_depth,
                      char* workdir_name)
{
  reset();
  block_ = db_->getChip()->getBlock();
  if (!block_)
    return;

  sta::Slack worst_slack = slack_threshold;

  lib_file_names_.emplace_back(liberty_file_name);
  work_dir_name_ = workdir_name;
  work_dir_name_ = work_dir_name_ + "/";

  if (!is_area_mode_) // Only in area mode
    removeConstCells();

  getBlob(max_depth);

  if (path_insts_.size()) {
    runABC();

    postABC(worst_slack);
  }
}

void Restructure::getBlob(unsigned max_depth)
{
  open_sta_->ensureGraph();
  open_sta_->ensureLevelized();
  open_sta_->searchPreamble();

  sta::PinSet ends;

  getEndPoints(ends, is_area_mode_, max_depth);
  if (ends.size()) {
    sta::PinSet boundary_points = !is_area_mode_ ? resizer_->findFanins(&ends) : resizer_->findFaninFanouts(&ends);
    // fanin_fanouts.insert(ends.begin(), ends.end()); // Add seq cells
    logger_->report("Found {} pins in extracted logic.", boundary_points.size());
    for (sta::Pin* pin : boundary_points) {
      odb::dbITerm* term = nullptr;
      odb::dbBTerm* port = nullptr;
      open_sta_->getDbNetwork()->staToDb(pin, term, port);
      if (term && term->getInst()->getITerms().size() < 10)
        path_insts_.insert(term->getInst());
    }
    logger_->report("Found {} instances for restructuring.",
                    path_insts_.size());
  }
}

void Restructure::runABC()
{
  input_blif_file_name_ = work_dir_name_ + std::string(block_->getConstName())
                          + "_crit_path.blif";
  std::vector<std::string> files_to_remove;

  debugPrint(logger_,
             utl::RMP,
             "remap",
             1,
             "Constants before remap {}",
             countConsts(block_));

  Blif blif_(logger_, open_sta_, locell_, loport_, hicell_, hiport_);
  blif_.setReplaceableInstances(path_insts_);
  blif_.writeBlif(input_blif_file_name_.c_str(), !is_area_mode_);
  debugPrint(
      logger_, RMP, "remap", 1, "Writing blif file {}", input_blif_file_name_);
  files_to_remove.emplace_back(input_blif_file_name_);

  // abc optimization

  int max_threads = ord::OpenRoad::openRoad()->getThreadCount();

  std::vector<Mode> modes;
  std::vector<pid_t> child_proc;

  if (is_area_mode_) {
    // Area Mode
    modes = {Mode::AREA_1, Mode::AREA_2, Mode::AREA_3};
  } else {
    // Delay Mode
    modes = {Mode::DELAY_1, Mode::DELAY_2, Mode::DELAY_3, Mode::DELAY_4};
  }

  child_proc.resize(modes.size(), 0);

  std::string best_blif;
  int best_inst_count = std::numeric_limits<int>::max();
  int best_level_gain = std::numeric_limits<int>::min();
  float best_delay_gain = std::numeric_limits<float>::max();
  bool level_based = false;

  debugPrint(
      logger_, RMP, "remap", 1, "Running ABC with {} threads.", max_threads);

  for (int curr_mode_idx = 0; curr_mode_idx < modes.size();) {
    int max_parallel_runs = (max_threads < modes.size() - curr_mode_idx)
                                ? max_threads
                                : modes.size() - curr_mode_idx;

    // Spawn ABC process(es)
    for (int curr_thread = 0; curr_thread < max_parallel_runs; ++curr_thread) {
      int temp_mode_idx = curr_mode_idx + curr_thread;
      output_blif_file_name_
          = work_dir_name_ + std::string(block_->getConstName())
            + std::to_string(temp_mode_idx) + "_crit_path_out.blif";

      opt_mode_ = modes[temp_mode_idx];

      std::string abc_script_file = work_dir_name_
                                    + std::to_string(temp_mode_idx)
                                    + "ord_abc_script.tcl";
      if (logfile_ == "")
        logfile_ = work_dir_name_ + "abc.log";

      debugPrint(logger_,
                 RMP,
                 "remap",
                 1,
                 "Writing ABC script file {}.",
                 abc_script_file);
      if (writeAbcScript(abc_script_file)) {
        std::string abc_command = std::string("yosys-abc < ") + abc_script_file;
        if (logfile_ != "")
          abc_command
              = abc_command + " > " + logfile_ + std::to_string(temp_mode_idx);

        pid_t child_pid = fork();
        if (child_pid == 0) {  // Begin child
          // Run in child process
          int ret = execlp("sh", "sh", "-c", abc_command.c_str(), 0);
          // Execution of command failed
          logger_->error(
              RMP,
              31,
              "Failed to run ABC with exit code {}. Please check the "
              "messages for details.",
              ret);
          exit(ret);
        }  // End child

        if (child_pid > 0) {
          child_proc[temp_mode_idx] = child_pid;
        } else if (child_pid < 0) {
          logger_->warn(
              RMP,
              29,
              "Failed to create new ABC process, could not fork parent "
              "process. Please check OS messages for details.");
        }

        files_to_remove.emplace_back(abc_script_file);
      }
    }  // end spawn

    // Wait for ABC process(es)
    for (int curr_thread = 0; curr_thread < max_parallel_runs; ++curr_thread) {
      int child_idx = curr_mode_idx + curr_thread;
      pid_t child = child_proc[child_idx];

      if (child == 0) {
        continue;
      }

      int return_status;
      waitpid(child, &return_status, 0);

      if (return_status) {
        child_proc[child_idx] = 0;
        logger_->warn(
            RMP,
            15,
            "ABC failed with code {}. Please check {} log file for details.",
            return_status,
            logfile_ + std::to_string(child_idx));
      }
    }  // end wait

    curr_mode_idx += max_parallel_runs;
  }  // end modes

  // Inspect ABC results to choose blif with least instance count
  for (int curr_mode_idx = 0; curr_mode_idx < modes.size(); curr_mode_idx++) {
    // Skip failed ABC runs
    if (child_proc[curr_mode_idx] == 0) {
      continue;
    }

    output_blif_file_name_
        = work_dir_name_ + std::string(block_->getConstName())
          + std::to_string(curr_mode_idx) + "_crit_path_out.blif";
    std::string abc_log_name = logfile_ + std::to_string(curr_mode_idx);

    int level_gain = 0;
    float delay = std::numeric_limits<float>::max();
    int num_instances = 0;
    bool success = readAbcLog(abc_log_name, level_gain, delay);
    if (success) {
      success
          = blif_.inspectBlif(output_blif_file_name_.c_str(), num_instances);
      logger_->report("Optimized to {} instances in iteration {} with max path depth decrease of {}, delay of {}.",
                      num_instances,
                      curr_mode_idx, level_gain, delay);

      if (success) {
        if (is_area_mode_) {
          if (num_instances < best_inst_count) {
            best_inst_count = num_instances;
            best_blif = output_blif_file_name_;
          }
        } else {
          if (level_based && level_gain > best_level_gain) {
            best_level_gain = level_gain;
            best_blif = output_blif_file_name_;
          }
          // Using only DELAY_4 for delay based gain since other modes not showing good gains
          if ((curr_mode_idx == static_cast <int> (Mode::DELAY_4)) || (!level_based && delay < best_delay_gain)) {
            best_delay_gain = delay;
            best_blif = output_blif_file_name_;
          }

        }
      }
    }
    files_to_remove.emplace_back(output_blif_file_name_);
  }

  if (best_inst_count < std::numeric_limits<int>::max()
     || (level_based && best_level_gain > 4) // upto 4 level gain becomes noise eventually in terms of delay gain
     || (!level_based && best_delay_gain < std::numeric_limits<float>::max())) {
    // read back netlist
    debugPrint(logger_, RMP, "remap", 1, "Reading blif file {}.", best_blif);
    blif_.readBlif(best_blif.c_str(), block_);
    debugPrint(logger_,
               utl::RMP,
               "remap",
               1,
               "Number constants after restructure {}.",
               countConsts(block_));
  } else {
    logger_->info(RMP, 21, "All re-synthesis runs discarded, keeping original netlist.");
  }

  for (auto file_to_remove : files_to_remove) {
    if (!logger_->debugCheck(RMP, "remap", 1))
      std::remove(file_to_remove.c_str());
  }
}

void Restructure::postABC(float worst_slack)
{
  // Leave the parasitics up to date.
  resizer_->estimateWireParasitics();
}
void Restructure::getEndPoints(sta::PinSet& ends,
                               bool area_mode,
                               unsigned max_depth)
{
  open_sta_->ensureGraph();
  open_sta_->searchPreamble();
  auto sta_state = open_sta_->search();
  int path_count = 100000;
  float min_slack = area_mode ? -sta::INF : -sta::INF;
  float max_slack = area_mode ? sta::INF : 0;

  sta::PathEndSeq* path_ends
      = sta_state->findPathEnds(  // from, thrus, to, unconstrained
          nullptr,
          nullptr,
          nullptr,
          false,
          // corner, min_max,
          open_sta_->findCorner("default"),
          sta::MinMaxAll::max(),
          // group_count, endpoint_count, unique_pins
          path_count,
          1,
          true,
          min_slack,
          max_slack,  // slack_min, slack_max,
          true,       // sort_by_slack
          nullptr,    // group_names
          // setup, hold, recovery, removal,
          true,
          true,
          false,
          false,
          // clk_gating_setup, clk_gating_hold
          false,
          false);

  std::size_t path_found = path_ends->size();
  logger_->report("Number of paths for restructure are {}", path_found);
  for (auto& path_end : *path_ends) {
    if (!is_area_mode_) {
      sta::PathExpanded expanded(path_end->path(), open_sta_);
      logger_->report("Found path of depth {}", expanded.size() / 2);
      if (expanded.size() / 2 > max_depth) {
        ends.insert(path_end->vertex(sta_state)->pin());
        // Use only one end point to limit blob size for timing
        break;
      }
    } else {
      ends.insert(path_end->vertex(sta_state)->pin());
    }
  }

  // unconstrained end points
  auto errors = open_sta_->checkTiming(false /*no_input_delay*/,
                                       false /*no_output_delay*/,
                                       false /*reg_multiple_clks*/,
                                       true /*reg_no_clks*/,
                                       true /*unconstrained_endpoints*/,
                                       false /*loops*/,
                                       false /*generated_clks*/);
  debugPrint(logger_, RMP, "remap", 1, "Size of errors = {}", errors.size());
  if (errors.size() && errors[0]->size() > 1) {
    sta::CheckError* error = errors[0];
    bool first = true;
    for (auto pinName : *error) {
      debugPrint(logger_, RMP, "remap", 1, "Unconstrained pin: {}", pinName);
      if (!first && open_sta_->getDbNetwork()->findPin(pinName)) {
        ends.insert(open_sta_->getDbNetwork()->findPin(pinName));
      }
    }
  }
  if (errors.size() > 1 && errors[1]->size() > 1) {
    sta::CheckError* error = errors[1];
    bool first = true;
    for (auto pinName : *error) {
      debugPrint(logger_, RMP, "remap", 1, "Unclocked pin: {}", pinName);
      if (!first && open_sta_->getDbNetwork()->findPin(pinName)) {
        ends.insert(open_sta_->getDbNetwork()->findPin(pinName));
      }
    }
  }
  logger_->report("Found {} end points for restructure", ends.size());
}

int Restructure::countConsts(odb::dbBlock* top_block)
{
  int const_nets = 0;
  for (auto block_net : top_block->getNets()) {
    if (block_net->getSigType().isSupply())
      const_nets++;
  }

  return const_nets;
}

void Restructure::removeConstCells()
{
  if (!hicell_.size() || !locell_.size())
    return;

  odb::dbMaster* hicell_master = nullptr;
  odb::dbMTerm* hiterm = nullptr;
  odb::dbMaster* locell_master = nullptr;
  odb::dbMTerm* loterm = nullptr;

  for (auto&& lib : block_->getDb()->getLibs()) {
    hicell_master = lib->findMaster(hicell_.c_str());

    locell_master = lib->findMaster(locell_.c_str());
    if (locell_master && hicell_master)
      break;
  }
  if (!hicell_master || !locell_master)
    return;

  hiterm = hicell_master->findMTerm(hiport_.c_str());
  loterm = locell_master->findMTerm(loport_.c_str());
  if (!hiterm || !loterm)
    return;

  open_sta_->clearLogicConstants();
  open_sta_->findLogicConstants();
  std::set<odb::dbInst*> constInsts;
  int const_cnt = 1;
  for (auto inst : block_->getInsts()) {
    int outputs = 0;
    int const_outputs = 0;
    auto master = inst->getMaster();
    sta::LibertyCell* cell = open_sta_->getDbNetwork()->libertyCell(
        open_sta_->getDbNetwork()->dbToSta(master));
    if (cell->hasSequentials())
      continue;

    for (auto&& iterm : inst->getITerms()) {
      if (iterm->getSigType() == odb::dbSigType::POWER
          || iterm->getSigType() == odb::dbSigType::GROUND)
        continue;

      if (iterm->getIoType() != odb::dbIoType::OUTPUT)
        continue;
      outputs++;
      auto pin = open_sta_->getDbNetwork()->dbToSta(iterm);
      sta::LogicValue pinVal = open_sta_->simLogicValue(pin);
      if (pinVal == sta::LogicValue::one || pinVal == sta::LogicValue::zero) {
        odb::dbNet* net = iterm->getNet();
        if (net) {
          odb::dbMaster* const_master = (pinVal == sta::LogicValue::one)
                                            ? hicell_master
                                            : locell_master;
          odb::dbMTerm* const_port
              = (pinVal == sta::LogicValue::one) ? hiterm : loterm;
          std::string inst_name = "rmp_const_" + std::to_string(const_cnt);
          debugPrint(logger_,
                     RMP,
                     "remap",
                     2,
                     "Adding cell {} inst {} for {}",
                     const_master->getName(),
                     inst_name,
                     inst->getName());
          auto new_inst
              = odb::dbInst::create(block_, const_master, inst_name.c_str());
          if (new_inst) {
            odb::dbITerm::disconnect(iterm);
            odb::dbITerm::connect(new_inst->getITerm(const_port), net);
          } else
            logger_->warn(RMP, 35, "Could not create instance {}.", inst_name);
        }
        const_outputs++;
        const_cnt++;
      }
    }
    if (outputs > 0 && outputs == const_outputs)
      constInsts.insert(inst);
  }
  open_sta_->clearLogicConstants();

  debugPrint(
      logger_, RMP, "remap", 2, "Removing {} instances...", constInsts.size());

  for (auto inst : constInsts)
    removeConstCell(inst);
  logger_->report("Removed {} instances with constant outputs.",
                  constInsts.size());
}

void Restructure::removeConstCell(odb::dbInst* inst)
{
  for (auto iterm : inst->getITerms())
    odb::dbITerm::disconnect(iterm);
  odb::dbInst::destroy(inst);
}

bool Restructure::writeAbcScript(std::string file_name)
{
  std::ofstream script(file_name.c_str());

  if (!script.is_open()) {
    logger_->error(RMP, 20, "Cannot open file {} for writing.", file_name);
    return false;
  }

  for (auto lib_name : lib_file_names_) {
    std::string read_lib_str = "read_lib " + lib_name + "\n";
    script << read_lib_str;
  }

  script << "read_blif -n " << input_blif_file_name_ << std::endl;

  if (logger_->debugCheck(RMP, "remap", 1))
    script << "write_verilog " << input_blif_file_name_ + std::string(".v")
           << std::endl;

  writeOptCommands(script);

  script << "write_blif " << output_blif_file_name_ << std::endl;

  if (logger_->debugCheck(RMP, "remap", 1))
    script << "write_verilog " << output_blif_file_name_ + std::string(".v")
           << std::endl;

  script.close();

  return true;
}

void Restructure::writeOptCommands(std::ofstream& script)
{
  std::string choice
      = "alias choice \"fraig_store; resyn2; fraig_store; resyn2; fraig_store; "
        "fraig_restore\"";
  std::string choice2
      = "alias choice2 \"fraig_store; balance; fraig_store; resyn2; "
        "fraig_store; resyn2; fraig_store; resyn2; fraig_store; "
        "fraig_restore\"";
  script << "bdd; sop" << std::endl;

  script << "alias resyn2 \"balance; rewrite; refactor; balance; rewrite; "
            "rewrite -z; balance; refactor -z; rewrite -z; balance\""
         << std::endl;
  script << choice << std::endl;
  script << choice2 << std::endl;

  if (opt_mode_ == Mode::AREA_3)
    script << "choice2" << std::endl;  // << "scleanup" << std::endl;
  else
    script << "resyn2" << std::endl;  // << "scleanup" << std::endl;

  switch (opt_mode_) {
    case Mode::DELAY_1: {
      script << "map -D 0.01 -A 0.9 -B 0.2 -M 0 -p" << std::endl;
      script << "buffer -p -c" << std::endl;
      break;
    }
    case Mode::DELAY_2: {
      script << "choice" << std::endl;
      script << "map -D 0.01 -A 0.9 -B 0.2 -M 0 -p" << std::endl;
      script << "choice" << std::endl;
      script << "map -D 0.01" << std::endl;
      script << "buffer -p -c" << std::endl << "topo" << std::endl;
      break;
    }
    case Mode::DELAY_3: {
      script << "choice2" << std::endl;
      script << "map -D 0.01 -A 0.9 -B 0.2 -M 0 -p" << std::endl;
      script << "choice2" << std::endl;
      script << "map -D 0.01" << std::endl;
      script << "buffer -p -c" << std::endl << "topo" << std::endl;
      break;
    }
    case Mode::DELAY_4: {
      script << "choice2" << std::endl;
      script << "amap -F 20 -A 20 -C 5000 -Q 0.1 -m" << std::endl;
      script << "choice2" << std::endl;
      script << "map -D 0.01 -A 0.9 -B 0.2 -M 0 -p" << std::endl;
      script << "buffer -p -c" << std::endl;
      break;
    }
    case Mode::AREA_2:
    case Mode::AREA_3: {
      script << "choice2" << std::endl;
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000" << std::endl;
      script << "choice2" << std::endl;
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000" << std::endl;
      break;
    }
    case Mode::AREA_1:
    default: {
      script << "choice2" << std::endl;
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000" << std::endl;
      break;
    }
  }
  script << "stime -p -c" << std::endl << "print_stats -m" << std::endl;
  script << "upsize {D} -c" << std::endl << "dnsize {D} -c" << std::endl;
  script << "stime -p -c" << std::endl << "print_stats -m" << std::endl;
}

void Restructure::setMode(const char* mode_name)
{
  is_area_mode_ = true;

  if (!strcmp(mode_name, "timing")) {
    is_area_mode_ = false;
    opt_mode_ = Mode::DELAY_1;
  } else if (!strcmp(mode_name, "area"))
    opt_mode_ = Mode::AREA_1;
  else {
    logger_->warn(RMP, 36, "Mode {} not recognized.", mode_name);
  }
}

void Restructure::setLogfile(const char* logfile)
{
  logfile_ = logfile;
}

void Restructure::setTieHiPort(sta::LibertyPort* tieHiPort)
{
  if (tieHiPort) {
    hicell_ = tieHiPort->libertyCell()->name();
    hiport_ = tieHiPort->name();
  }
}

void Restructure::setTieLoPort(sta::LibertyPort* tieLoPort)
{
  if (tieLoPort) {
    locell_ = tieLoPort->libertyCell()->name();
    loport_ = tieLoPort->name();
  }
}

bool Restructure::readAbcLog(std::string abc_file_name, int& level_gain, float& final_delay)
{
  std::ifstream abc_file(abc_file_name);
  if (abc_file.bad()) {
    logger_->error(RMP, 16, "cannot open file {}", abc_file_name);
    return false;
  }
  logger_->report("Reading ABC log {}.", abc_file_name);
  std::string buf;
  const char delimiter = ' ';
  bool status = true;
  std::vector<double> level;
  std::vector<float> delay;

  //read the file line by line
  while (std::getline(abc_file, buf))
  {
      //convert the line in to stream:
      std::istringstream ss(buf);
      std::vector<std::string> tokens;

      //read the line, word by word
      while (std::getline(ss, buf, delimiter))
          tokens.push_back(buf);

      if (!tokens.empty() && tokens[0] == "Error:") {
        status = false;
        logger_->warn(RMP, 25, "ABC run failed, see log file {} for details.", abc_file_name);
        break;
      }
      if (tokens.size() > 7 && tokens[tokens.size()-3] == "lev" && tokens[tokens.size()-2] == "=") {
        level.emplace_back(std::stoi(tokens[tokens.size()-1]));
      }
      if (tokens.size() > 7) {
        std::string prev_token = "";
        for (std::string token : tokens) {
          if (prev_token == "delay" && token.at(0) == '=') {
            std::string delay_str = token;
            if (delay_str.size() > 1) {
              delay_str.erase(delay_str.begin()); // remove first char which is '='
              delay.emplace_back(std::stof(delay_str));
            }
            break;
          }
          prev_token = token;
        }
      }

  }

  if (level.size() > 1) {
    level_gain = level[0] - level[level.size()-1];
  }
  if (delay.size() > 0) {
    final_delay = delay[delay.size()-1]; // last value in file
  }
  return status;

}
}  // namespace rmp
