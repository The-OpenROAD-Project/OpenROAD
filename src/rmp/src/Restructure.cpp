// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2026, The OpenROAD Authors

#include "rmp/Restructure.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "annealing_strategy.h"
#include "base/main/abcapis.h"
#include "cut/abc_init.h"
#include "cut/blif.h"
#include "db_sta/dbSta.hh"
#include "genetic_strategy.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"
#include "zero_slack_strategy.h"

namespace rmp {

using abc::Abc_Frame_t;
using abc::Abc_FrameGetGlobalFrame;
using abc::Abc_Start;
using abc::Abc_Stop;
using cut::Blif;
using utl::RMP;

Restructure::Restructure(utl::Logger* logger,
                         sta::dbSta* open_sta,
                         odb::dbDatabase* db,
                         rsz::Resizer* resizer,
                         est::EstimateParasitics* estimate_parasitics)
{
  logger_ = logger;
  db_ = db;
  open_sta_ = open_sta;
  resizer_ = resizer;
  estimate_parasitics_ = estimate_parasitics;

  cut::abcInit();
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

void Restructure::resynth(sta::Scene* corner)
{
  ZeroSlackStrategy zero_slack_strategy(corner);
  zero_slack_strategy.OptimizeDesign(
      open_sta_, name_generator_, resizer_, logger_);
}

void Restructure::resynthAnnealing(sta::Scene* corner)
{
  AnnealingStrategy annealing_strategy(corner,
                                       slack_threshold_,
                                       annealing_seed_,
                                       annealing_temp_,
                                       annealing_iters_,
                                       annealing_revert_after_,
                                       annealing_init_ops_);
  annealing_strategy.OptimizeDesign(
      open_sta_, name_generator_, resizer_, logger_);
}

void Restructure::resynthGenetic(sta::Scene* corner)
{
  GeneticStrategy genetic_strategy(corner,
                                   slack_threshold_,
                                   genetic_seed_,
                                   genetic_population_size_,
                                   genetic_mutation_probability_,
                                   genetic_crossover_probability_,
                                   genetic_tournament_size_,
                                   genetic_tournament_probability_,
                                   genetic_iters_,
                                   genetic_init_ops_);
  genetic_strategy.OptimizeDesign(
      open_sta_, name_generator_, resizer_, logger_);
}

void Restructure::run(char* liberty_file_name,
                      float slack_threshold,
                      unsigned max_depth,
                      char* workdir_name,
                      char* abc_logfile)
{
  reset();
  block_ = db_->getChip()->getBlock();
  if (!block_) {
    return;
  }

  logfile_ = abc_logfile;
  sta::Slack worst_slack = slack_threshold;

  lib_file_names_.emplace_back(liberty_file_name);
  work_dir_name_ = workdir_name;
  work_dir_name_ = work_dir_name_ + "/";

  if (is_area_mode_) {  // Only in area mode
    removeConstCells();
  }

  getBlob(max_depth);

  if (!path_insts_.empty()) {
    runABC();

    postABC(worst_slack);
  }
}

void Restructure::getBlob(unsigned max_depth)
{
  open_sta_->ensureGraph();
  open_sta_->ensureLevelized();
  open_sta_->searchPreamble();

  sta::PinSet ends(open_sta_->getDbNetwork());

  getEndPoints(ends, is_area_mode_, max_depth);
  if (!ends.empty()) {
    sta::PinSet boundary_points = !is_area_mode_
                                      ? resizer_->findFanins(ends)
                                      : resizer_->findFaninFanouts(ends);
    // fanin_fanouts.insert(ends.begin(), ends.end()); // Add seq cells
    logger_->report("Found {} pins in extracted logic.",
                    boundary_points.size());
    for (const sta::Pin* pin : boundary_points) {
      odb::dbITerm* term = nullptr;
      odb::dbBTerm* port = nullptr;
      odb::dbModITerm* moditerm = nullptr;
      open_sta_->getDbNetwork()->staToDb(pin, term, port, moditerm);
      if (term && !term->getInst()->getMaster()->isBlock()) {
        path_insts_.insert(term->getInst());
      }
    }
    logger_->report("Found {} instances for restructuring.",
                    path_insts_.size());
  }
}

void Restructure::runABC()
{
  const std::string prefix
      = work_dir_name_ + std::string(block_->getConstName());
  input_blif_file_name_ = prefix + "_crit_path.blif";
  std::vector<std::string> files_to_remove;

  debugPrint(logger_,
             utl::RMP,
             "remap",
             1,
             "Constants before remap {}",
             countConsts(block_));

  Blif blif_(
      logger_, open_sta_, locell_, loport_, hicell_, hiport_, ++blif_call_id_);
  blif_.setReplaceableInstances(path_insts_);
  blif_.writeBlif(input_blif_file_name_.c_str(), !is_area_mode_);
  debugPrint(
      logger_, RMP, "remap", 1, "Writing blif file {}", input_blif_file_name_);
  files_to_remove.emplace_back(input_blif_file_name_);

  // abc optimization
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
  float best_delay_gain = std::numeric_limits<float>::max();

  debugPrint(
      logger_, RMP, "remap", 1, "Running ABC with {} modes.", modes.size());

  for (size_t curr_mode_idx = 0; curr_mode_idx < modes.size();
       curr_mode_idx++) {
    output_blif_file_name_
        = prefix + std::to_string(curr_mode_idx) + "_crit_path_out.blif";

    opt_mode_ = modes[curr_mode_idx];

    const std::string abc_script_file
        = prefix + std::to_string(curr_mode_idx) + "ord_abc_script.tcl";
    if (logfile_.empty()) {
      logfile_ = prefix + "abc.log";
    }

    debugPrint(logger_,
               RMP,
               "remap",
               1,
               "Writing ABC script file {}.",
               abc_script_file);

    if (writeAbcScript(abc_script_file)) {
      // call linked abc
      Abc_Start();
      Abc_Frame_t* abc_frame = Abc_FrameGetGlobalFrame();
      const std::string command = "source " + abc_script_file;
      child_proc[curr_mode_idx]
          = Cmd_CommandExecute(abc_frame, command.c_str());
      if (child_proc[curr_mode_idx]) {
        logger_->error(RMP, 6, "Error executing ABC command {}.", command);
        return;
      }
      Abc_Stop();
      // exit linked abc
      files_to_remove.emplace_back(abc_script_file);
    }
  }  // end modes

  // Inspect ABC results to choose blif with least instance count
  for (int curr_mode_idx = 0; curr_mode_idx < modes.size(); curr_mode_idx++) {
    // Skip failed ABC runs
    if (child_proc[curr_mode_idx] != 0) {
      continue;
    }

    output_blif_file_name_
        = prefix + std::to_string(curr_mode_idx) + "_crit_path_out.blif";
    const std::string abc_log_name = logfile_ + std::to_string(curr_mode_idx);

    int level_gain = 0;
    float delay = std::numeric_limits<float>::max();
    int num_instances = 0;
    bool success = readAbcLog(abc_log_name, level_gain, delay);
    if (success) {
      success
          = blif_.inspectBlif(output_blif_file_name_.c_str(), num_instances);
      logger_->report(
          "Optimized to {} instances in iteration {} with max path depth "
          "decrease of {}, delay of {}.",
          num_instances,
          curr_mode_idx,
          level_gain,
          delay);

      if (success) {
        if (is_area_mode_) {
          if (num_instances < best_inst_count) {
            best_inst_count = num_instances;
            best_blif = output_blif_file_name_;
          }
        } else {
          // Using only DELAY_4 for delay based gain since other modes not
          // showing good gains
          if (modes[curr_mode_idx] == Mode::DELAY_4) {
            best_delay_gain = delay;
            best_blif = output_blif_file_name_;
          }
        }
      }
    }
    files_to_remove.emplace_back(output_blif_file_name_);
  }

  if (best_inst_count < std::numeric_limits<int>::max()
      || best_delay_gain < std::numeric_limits<float>::max()) {
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
    logger_->info(
        RMP, 4, "All re-synthesis runs discarded, keeping original netlist.");
  }

  for (const auto& file_to_remove : files_to_remove) {
    if (!logger_->debugCheck(RMP, "remap", 1)) {
      std::error_code err;
      if (std::filesystem::remove(file_to_remove, err); err) {
        logger_->error(RMP, 11, "Fail to remove file {}", file_to_remove);
      }
    }
  }
}

void Restructure::postABC(float worst_slack)
{
  // Leave the parasitics up to date.
  estimate_parasitics_->estimateWireParasitics();
}
void Restructure::getEndPoints(sta::PinSet& ends,
                               bool area_mode,
                               unsigned max_depth)
{
  auto sta_state = open_sta_->search();
  sta::VertexSet& end_points = sta_state->endpoints();
  std::size_t path_found = end_points.size();
  logger_->report("Number of paths for restructure are {}", path_found);
  for (auto& end_point : end_points) {
    if (!is_area_mode_) {
      sta::Path* path
          = open_sta_->vertexWorstSlackPath(end_point, sta::MinMax::max());
      sta::PathExpanded expanded(path, open_sta_);
      // Members in expanded include gate output and net so divide by 2
      logger_->report("Found path of depth {}", expanded.size() / 2);
      if (expanded.size() / 2 > max_depth) {
        ends.insert(end_point->pin());
        // Use only one end point to limit blob size for timing
        break;
      }
    } else {
      ends.insert(end_point->pin());
    }
  }

  // unconstrained end points
  if (is_area_mode_) {
    auto errors = open_sta_->checkTiming(open_sta_->cmdMode(),
                                         false /*no_input_delay*/,
                                         false /*no_output_delay*/,
                                         false /*reg_multiple_clks*/,
                                         true /*reg_no_clks*/,
                                         true /*unconstrained_endpoints*/,
                                         false /*loops*/,
                                         false /*generated_clks*/);
    debugPrint(logger_, RMP, "remap", 1, "Size of errors = {}", errors.size());
    if (!errors.empty() && errors[0]->size() > 1) {
      sta::CheckError* error = errors[0];
      bool first = true;
      for (auto pinName : *error) {
        debugPrint(logger_, RMP, "remap", 1, "Unconstrained pin: {}", pinName);
        if (!first && open_sta_->getDbNetwork()->findPin(pinName)) {
          ends.insert(open_sta_->getDbNetwork()->findPin(pinName));
        }
        first = false;
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
        first = false;
      }
    }
  }
  logger_->report("Found {} end points for restructure", ends.size());
}

int Restructure::countConsts(odb::dbBlock* top_block)
{
  int const_nets = 0;
  for (auto block_net : top_block->getNets()) {
    if (block_net->getSigType().isSupply()) {
      const_nets++;
    }
  }

  return const_nets;
}

void Restructure::removeConstCells()
{
  if (hicell_.empty() || locell_.empty()) {
    return;
  }

  odb::dbMaster* hicell_master = nullptr;
  odb::dbMTerm* hiterm = nullptr;
  odb::dbMaster* locell_master = nullptr;
  odb::dbMTerm* loterm = nullptr;

  for (auto&& lib : block_->getDb()->getLibs()) {
    hicell_master = lib->findMaster(hicell_.c_str());

    locell_master = lib->findMaster(locell_.c_str());
    if (locell_master && hicell_master) {
      break;
    }
  }
  if (!hicell_master || !locell_master) {
    return;
  }

  hiterm = hicell_master->findMTerm(hiport_.c_str());
  loterm = locell_master->findMTerm(loport_.c_str());
  if (!hiterm || !loterm) {
    return;
  }

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
    if (cell->hasSequentials()) {
      continue;
    }

    for (auto&& iterm : inst->getITerms()) {
      if (iterm->getSigType() == odb::dbSigType::POWER
          || iterm->getSigType() == odb::dbSigType::GROUND) {
        continue;
      }

      if (iterm->getIoType() != odb::dbIoType::OUTPUT) {
        continue;
      }
      outputs++;
      auto pin = open_sta_->getDbNetwork()->dbToSta(iterm);
      sta::LogicValue pinVal
          = open_sta_->simLogicValue(pin, open_sta_->cmdMode());
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
            iterm->disconnect();
            new_inst->getITerm(const_port)->connect(net);
          } else {
            logger_->warn(RMP, 9, "Could not create instance {}.", inst_name);
          }
        }
        const_outputs++;
        const_cnt++;
      }
    }
    if (outputs > 0 && outputs == const_outputs) {
      constInsts.insert(inst);
    }
  }
  open_sta_->clearLogicConstants();

  debugPrint(
      logger_, RMP, "remap", 2, "Removing {} instances...", constInsts.size());

  for (auto inst : constInsts) {
    removeConstCell(inst);
  }
  logger_->report("Removed {} instances with constant outputs.",
                  constInsts.size());
}

void Restructure::removeConstCell(odb::dbInst* inst)
{
  for (auto iterm : inst->getITerms()) {
    iterm->disconnect();
  }
  odb::dbInst::destroy(inst);
}

bool Restructure::writeAbcScript(const std::string& file_name)
{
  std::ofstream script(file_name.c_str());

  if (!script.is_open()) {
    logger_->error(RMP, 3, "Cannot open file {} for writing.", file_name);
    return false;
  }

  for (const auto& lib_name : lib_file_names_) {
    // abc read_lib prints verbose by default, -v toggles to off to avoid read
    // time being printed
    std::string read_lib_str = "read_lib -v " + lib_name + "\n";
    script << read_lib_str;
  }

  script << "read_blif -n " << input_blif_file_name_ << '\n';

  if (logger_->debugCheck(RMP, "remap", 1)) {
    script << "write_verilog " << input_blif_file_name_ + std::string(".v")
           << '\n';
  }

  writeOptCommands(script);

  script << "write_blif " << output_blif_file_name_ << '\n';

  if (logger_->debugCheck(RMP, "remap", 1)) {
    script << "write_verilog " << output_blif_file_name_ + std::string(".v")
           << '\n';
  }

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
  script << "bdd; sop\n";

  script << "alias resyn2 \"balance; rewrite; refactor; balance; rewrite; "
            "rewrite -z; balance; refactor -z; rewrite -z; balance\""
         << '\n';
  script << choice << '\n';
  script << choice2 << '\n';

  if (opt_mode_ == Mode::AREA_3) {
    script << "choice2\n";  // << "scleanup" << std::endl;
  } else {
    script << "resyn2\n";  // << "scleanup" << std::endl;
  }

  switch (opt_mode_) {
    case Mode::DELAY_1: {
      script << "map -D 0.01 -A 0.9 -B 0.2 -M 0 -p\n";
      script << "buffer -p -c\n";
      break;
    }
    case Mode::DELAY_2: {
      script << "choice\n";
      script << "map -D 0.01 -A 0.9 -B 0.2 -M 0 -p\n";
      script << "choice\n";
      script << "map -D 0.01\n";
      script << "buffer -p -c\n"
             << "topo\n";
      break;
    }
    case Mode::DELAY_3: {
      script << "choice2\n";
      script << "map -D 0.01 -A 0.9 -B 0.2 -M 0 -p\n";
      script << "choice2\n";
      script << "map -D 0.01\n";
      script << "buffer -p -c\n"
             << "topo\n";
      break;
    }
    case Mode::DELAY_4: {
      script << "choice2\n";
      script << "amap -F 20 -A 20 -C 5000 -Q 0.1 -m\n";
      script << "choice2\n";
      script << "map -D 0.01 -A 0.9 -B 0.2 -M 0 -p\n";
      script << "buffer -p -c\n";
      break;
    }
    case Mode::AREA_2:
    case Mode::AREA_3: {
      script << "choice2\n";
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000\n";
      script << "choice2\n";
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000\n";
      break;
    }
    case Mode::AREA_1:
    default: {
      script << "choice2\n";
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000\n";
      break;
    }
  }
}

void Restructure::setMode(const char* mode_name)
{
  is_area_mode_ = true;

  if (!strcmp(mode_name, "timing")) {
    is_area_mode_ = false;
    opt_mode_ = Mode::DELAY_1;
  } else if (!strcmp(mode_name, "area")) {
    opt_mode_ = Mode::AREA_1;
  } else {
    logger_->warn(RMP, 10, "Mode {} not recognized.", mode_name);
  }
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

bool Restructure::readAbcLog(const std::string& abc_file_name,
                             int& level_gain,
                             float& final_delay)
{
  std::ifstream abc_file(abc_file_name);
  if (abc_file.bad()) {
    logger_->error(RMP, 2, "cannot open file {}", abc_file_name);
    return false;
  }
  debugPrint(
      logger_, utl::RMP, "remap", 1, "Reading ABC log {}.", abc_file_name);
  std::string buf;
  const char delimiter = ' ';
  bool status = true;
  std::vector<double> level;
  std::vector<float> delay;

  // read the file line by line
  while (std::getline(abc_file, buf)) {
    // convert the line in to stream:
    std::istringstream ss(buf);
    std::vector<std::string> tokens;

    // read the line, word by word
    while (std::getline(ss, buf, delimiter)) {
      tokens.push_back(buf);
    }

    if (!tokens.empty() && tokens[0] == "Error:") {
      status = false;
      logger_->warn(RMP,
                    5,
                    "ABC run failed, see log file {} for details.",
                    abc_file_name);
      break;
    }
    if (tokens.size() > 7 && tokens[tokens.size() - 3] == "lev"
        && tokens[tokens.size() - 2] == "=") {
      level.emplace_back(std::stoi(tokens[tokens.size() - 1]));
    }
    if (tokens.size() > 7) {
      std::string prev_token;
      for (std::string token : tokens) {
        if (prev_token == "delay" && token.at(0) == '=') {
          std::string delay_str = token;
          if (delay_str.size() > 1) {
            delay_str.erase(
                delay_str.begin());  // remove first char which is '='
            delay.emplace_back(std::stof(delay_str));
          }
          break;
        }
        prev_token = std::move(token);
      }
    }
  }

  if (level.size() > 1) {
    level_gain = level[0] - level[level.size() - 1];
  }
  if (!delay.empty()) {
    final_delay = delay[delay.size() - 1];  // last value in file
  }
  return status;
}
}  // namespace rmp
