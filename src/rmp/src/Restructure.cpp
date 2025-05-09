// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rmp/Restructure.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

#include "base/abc/abc.h"
#include "base/main/abcapis.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "rmp/blif.h"
#include "sta/Corner.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

using utl::RMP;
using namespace abc;

namespace rmp {

void Restructure::init(utl::Logger* logger,
                       sta::dbSta* open_sta,
                       odb::dbDatabase* db,
                       rsz::Resizer* resizer)
{
  logger_ = logger;
  db_ = db;
  open_sta_ = open_sta;
  resizer_ = resizer;
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
                      char* workdir_name,
                      char* abc_logfile)
{
  reset();
  block_ = db_->getChip()->getBlock();
  if (!block_)
    return;

  logfile_ = abc_logfile;
  sta::Slack worst_slack = slack_threshold;

  lib_file_names_.emplace_back(liberty_file_name);
  work_dir_name_ = workdir_name;
  work_dir_name_ = work_dir_name_ + "/";

  if (is_area_mode_)  // Only in area mode
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

  sta::PinSet ends(open_sta_->getDbNetwork());

  getEndPoints(ends, is_area_mode_, max_depth);
  if (ends.size()) {
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
      odb::dbModBTerm* modbterm = nullptr;
      open_sta_->getDbNetwork()->staToDb(pin, term, port, moditerm, modbterm);
      if (term && !term->getInst()->getMaster()->isBlock())
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
        = work_dir_name_ + std::string(block_->getConstName())
          + std::to_string(curr_mode_idx) + "_crit_path_out.blif";

    opt_mode_ = modes[curr_mode_idx];

    const std::string abc_script_file
        = work_dir_name_ + std::to_string(curr_mode_idx) + "ord_abc_script.tcl";
    if (logfile_ == "")
      logfile_ = work_dir_name_ + "abc.log";

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
        logger_->error(RMP, 26, "Error executing ABC command {}.", command);
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
        = work_dir_name_ + std::string(block_->getConstName())
          + std::to_string(curr_mode_idx) + "_crit_path_out.blif";
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
        RMP, 21, "All re-synthesis runs discarded, keeping original netlist.");
  }

  for (const auto& file_to_remove : files_to_remove) {
    if (!logger_->debugCheck(RMP, "remap", 1))
      if (std::remove(file_to_remove.c_str()) != 0) {
        logger_->error(RMP, 37, "Fail to remove file {}", file_to_remove);
      }
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
  auto sta_state = open_sta_->search();
  sta::VertexSet* end_points = sta_state->endpoints();
  std::size_t path_found = end_points->size();
  logger_->report("Number of paths for restructure are {}", path_found);
  for (auto& end_point : *end_points) {
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
    auto errors = open_sta_->checkTiming(false /*no_input_delay*/,
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
            iterm->disconnect();
            new_inst->getITerm(const_port)->connect(net);
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
    iterm->disconnect();
  odb::dbInst::destroy(inst);
}

bool Restructure::writeAbcScript(std::string file_name)
{
  std::ofstream script(file_name.c_str());

  if (!script.is_open()) {
    logger_->error(RMP, 20, "Cannot open file {} for writing.", file_name);
    return false;
  }

  for (const auto& lib_name : lib_file_names_) {
    // abc read_lib prints verbose by default, -v toggles to off to avoid read
    // time being printed
    std::string read_lib_str = "read_lib -v " + lib_name + "\n";
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

bool Restructure::readAbcLog(std::string abc_file_name,
                             int& level_gain,
                             float& final_delay)
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

  // read the file line by line
  while (std::getline(abc_file, buf)) {
    // convert the line in to stream:
    std::istringstream ss(buf);
    std::vector<std::string> tokens;

    // read the line, word by word
    while (std::getline(ss, buf, delimiter))
      tokens.push_back(buf);

    if (!tokens.empty() && tokens[0] == "Error:") {
      status = false;
      logger_->warn(RMP,
                    25,
                    "ABC run failed, see log file {} for details.",
                    abc_file_name);
      break;
    }
    if (tokens.size() > 7 && tokens[tokens.size() - 3] == "lev"
        && tokens[tokens.size() - 2] == "=") {
      level.emplace_back(std::stoi(tokens[tokens.size() - 1]));
    }
    if (tokens.size() > 7) {
      std::string prev_token = "";
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
  if (delay.size() > 0) {
    final_delay = delay[delay.size() - 1];  // last value in file
  }
  return status;
}

//
// Cut Based Restructuring code
//---------------------------
// with abc source integration
//
//
// Flags:
// head_pin -- if set, start cut from pin (single output cut)
//         -- if null,generate a blob.
// unconstrained:
//         -- if true, walk back to primary inputs/state elements in cut
//         generation
//            else walk back through inverters to first level of logic. Generate
//            cut as cartesian product.
//

void Restructure::cutresynthrun(
    char* target_library,  // todo: pass in library object
    char* script,          // optional script
    sta::Pin* head_pin,    // passed in by user. ok to be null !
    bool unconstrained,    // controls cut generation
    bool verbose)
{
  bool single_op = false;
  if (head_pin)
    single_op = true;
  (void) single_op;

  reset();
  logger_->report("Cut based Restructuring.");

#ifdef REGRESS_RESTRUCT
  {
    FILE* cond_file = fopen("cmd.txt", "w+");
    std::string tl = target_library;
    size_t last_idx = tl.find_last_of('/');
    if (last_idx != std::string::npos)
      tl = tl.substr(last_idx + 1);
    std::string default_script
        = "read_lib " + tl + " ;strash ; balance; amap -Q 9.9 ";
    fprintf(
        cond_file, "Script: %s \n", script ? script : default_script.c_str());
    fclose(cond_file);
  }
#endif

  LibRead liberty_library(logger_, target_library, verbose);

  Cut* cut_to_remap = nullptr;
  sta::Sta* sta = sta::Sta::sta();
  sta::Network* network = sta->network();
  std::vector<Cut*> cut_set;

  //
  // Just generate any cuts for discussion with Cho.
  //
  generateUnconstrainedCuts(cut_set);

  logger_->report("Generated {} cuts.", cut_set.size());

  //
  // For demo just pick one cut: choice one with biggest volume
  //

  Cut* best_cut = nullptr;
  for (auto c : cut_set) {
    if (best_cut == nullptr)
      best_cut = c;
    else if (c->volume_.size() > best_cut->volume_.size())
      best_cut = c;
  }
  if (!best_cut)
    return;

  // remap the cut set.
  // only remap non-trivial cuts
  if (best_cut->volume_.size() > 0) {
    logger_->report(
        "Processing cut {} of volume {} with {} roots and {} leaves.",
        best_cut->id_,
        best_cut->volume_.size(),
        best_cut->roots_.size(),
        best_cut->leaves_.size());
    cut_to_remap = best_cut;

    // Get the timing numbers for the cut from the sta
    std::vector<std::pair<const sta::Pin*, TimingRecord*>> timing_requirements;
    annotateCutTiming(cut_to_remap, timing_requirements);

    // Remap the cut with the physical timing constraints
    // onto the target library.
    bool script_present = false;
    std::string script_name;
    if (script && strlen(script) > 0) {
      script_name = script;
      script_present = true;
    }
    logger_->report("PhysRemapping cut {}.", cut_to_remap->id_);
    PhysRemap prm(
        logger_,
        network,
        cut_to_remap,         // the cut to remap
        timing_requirements,  // the actual timing requirements for the cut
        target_library,
        liberty_library,
        script_present,
        script_name,
        true);
    prm.Remap();
  }
}

// Homebrew way of getting start/end points for cut generation

class AccumulateSortedSlackEndPoints : public sta::VertexVisitor
{
 public:
  AccumulateSortedSlackEndPoints(
      sta::Network* nwk,
      std::multimap<float, sta::Vertex*>& sorted_slack_times,
      std::multimap<sta::Vertex*, float>& vertex_slack_times,
      std::set<sta::Vertex*>& end_points)
      : nwk_(nwk),
        sorted_slack_times_(sorted_slack_times),
        vertex_slack_times_(vertex_slack_times),
        end_points_(end_points)
  {
  }
  void visit(sta::Vertex* visit);
  VertexVisitor* copy() const { return nullptr; }

 private:
  sta::Network* nwk_;
  std::multimap<float, sta::Vertex*>& sorted_slack_times_;
  std::multimap<sta::Vertex*, float>& vertex_slack_times_;
  std::set<sta::Vertex*>& end_points_;
  friend class Restructure;
};
void AccumulateSortedSlackEndPoints::visit(sta::Vertex* v)
{
  sta::Sta* sta = sta::Sta::sta();

  sta::Pin* cur_pin = v->pin();
  sta::Slack max_s = sta->pinSlack(cur_pin, sta::MinMax::max());
  sta::Slack min_s = sta->pinSlack(cur_pin, sta::MinMax::min());
  sta::Slack s = (max_s == sta::INF || max_s == -sta::INF) ? min_s : max_s;

#ifdef DEBUG_RESTRUCT
  printf("Design Worst Slack time %s\n", delayAsString(s, sta));
#endif

  if (Restructure::isRegInput(nwk_, v) || Restructure::isPrimary(nwk_, v)) {
    sorted_slack_times_.insert(std::pair<float, sta::Vertex*>(s, v));
    vertex_slack_times_.insert(std::pair<sta::Vertex*, float>(v, s));
    end_points_.insert(v);
  }
}

class AccumulateSortedSlackStartPoints : public sta::VertexVisitor
{
 public:
  AccumulateSortedSlackStartPoints(std::set<sta::Vertex*>& end_points)
      : end_points_(end_points)
  {
  }
  void visit(sta::Vertex* visit);
  VertexVisitor* copy() const { return nullptr; }

 private:
  std::set<sta::Vertex*>& end_points_;
  friend class Restructure;
};
void AccumulateSortedSlackStartPoints::visit(sta::Vertex* v)
{
  end_points_.insert(v);
}

bool Restructure::isRegInput(sta::Network* nwk, sta::Vertex* v)
{
  sta::LibertyPort* port = nwk->libertyPort(v->pin());
  if (port) {
    sta::LibertyCell* cell = port->libertyCell();
    for (auto arc_set : cell->timingArcSets(nullptr, port)) {
      if (arc_set->role()->genericRole() == sta::TimingRole::setup())
        return true;
    }
  }
  return false;
}

bool Restructure::isRegOutput(sta::Network* nwk, sta::Vertex* v)
{
  sta::LibertyPort* port = nwk->libertyPort(v->pin());
  if (port) {
    sta::LibertyCell* cell = port->libertyCell();
    for (auto arc_set : cell->timingArcSets(nullptr, port)) {
      if (arc_set->role()->genericRole() == sta::TimingRole::regClkToQ())
        return true;
    }
  }
  return false;
}

bool Restructure::isPrimary(sta::Network* nwk, sta::Vertex* v)
{
  return (nwk->isTopLevelPort(v->pin()));
}

void Restructure::annotateCutTiming(
    Cut* cut,
    std::vector<std::pair<const sta::Pin*, TimingRecord*>>& timing_requirements)
{
  sta::Sta* sta = sta::Sta::sta();
  sta::NetworkReader* network = sta->networkReader();
  sta::Arrival max_arrival_rise = 0.0;
  sta::Arrival max_arrival_fall = 0.0;
  sta::Required min_required_rise;
  sta::Required min_required_fall;

  // Get the arrival and required times for the exceptions for each vertex.
  sta::Sdc* sdc_nwk = sta->sdc();
  sta::ClockSeq* clock_seq = sdc_nwk->clocks();
  sta::Graph* graph = sta->graph();

  int ip_ix = 0;
  TimingRecord* tr = nullptr;

  //
  // cut inputs: allow for delayed arrival time
  //
  for (auto leaf_pin : cut->leaves_) {
    sta::VertexId v_id = network->vertexId(leaf_pin);
    sta::Vertex* v = graph->vertex(v_id);
    tr = nullptr;
    if (v) {
      // TODO: check out how to get the max arrival time without going through
      // all the path refs
      sta::MinMax* min_max_arrival;
      sta::MinMax* min_max = sta::MinMax::find("max");
      sta::RiseFall* rf_arrival_rise = sta::RiseFall::find("rise");
      sta::RiseFall* rf_arrival_fall = sta::RiseFall::find("fall");
      max_arrival_rise
          = 0;  // sta->vertexArrival(v, rf_arrival_rise, min_max,path);
      max_arrival_fall
          = 0;  // sta->vertexArrival(v, rf_arrival_fall, min_max,path);
      for (auto clk : *clock_seq) {
        sta::ClockEdge* clk_edge_fall = clk->edge(rf_arrival_fall);
        sta::ClockEdge* clk_edge_rise = clk->edge(rf_arrival_rise);
        for (auto path_ap : sta->corners()->pathAnalysisPts()) {
          sta::Arrival v_arrival_rise = sta->vertexArrival(
              v, rf_arrival_rise, clk_edge_rise, path_ap, min_max);
          sta::Arrival v_arrival_fall = sta->vertexArrival(
              v, rf_arrival_fall, clk_edge_fall, path_ap, min_max);
          if (v_arrival_rise > max_arrival_rise)
            max_arrival_rise = v_arrival_rise;
          if (v_arrival_fall > max_arrival_fall)
            max_arrival_fall = v_arrival_fall;
        }
      }
    }
    TimingRecord* tr = nullptr;
    if (graph && v) {
      tr = new TimingRecord();
      tr->arrival_rise = atof(delayAsString(max_arrival_rise, sta));
      tr->arrival_fall = atof(delayAsString(max_arrival_fall, sta));
    }
    timing_requirements.push_back(
        std::pair<const sta::Pin*, TimingRecord*>(leaf_pin, tr));
    ip_ix++;

#ifdef DEBUG_RESTRUCT
    printf("Pin %s Setting arrival rise %s\n",
           network->pathName(leaf_pin),
           delayAsString(tr->arrival_rise, sta));
    printf("Pin %s Setting arrival fall %s\n",
           network->pathName(leaf_pin),
           delayAsString(tr->arrival_fall, sta));
#endif
  }
  // cut outputs: set required time
  for (auto root_pin : cut->roots_) {
    sta::VertexId v_id = network->vertexId(root_pin);
    sta::Vertex* v = graph->vertex(v_id);
    bool got_required = false;

    if (v) {
      sta::MinMax* min_max_arrival;
      sta::MinMax* min_max = sta::MinMax::find("max");
      min_required_rise = sta->vertexRequired(v, min_max);
      min_required_fall = sta->vertexRequired(v, min_max);
      got_required = true;
    }

    TimingRecord* tr = nullptr;
    if (graph && v && got_required) {
      tr = new TimingRecord();
      tr->required_rise = atof(delayAsString(min_required_rise, sta));
      tr->required_fall = atof(delayAsString(min_required_fall, sta));
    }
    timing_requirements.push_back(
        std::pair<const sta::Pin*, TimingRecord*>(root_pin, tr));

#ifdef DEBUG_RESTRUCT
    printf("Pin %s required rise %s (%f)\n",
           network->pathName(root_pin),
           delayAsString(tr->required_rise, sta),
           atof(delayAsString(tr->required_rise, sta)));
    printf("Pin %s required fall %s (%f) \n",
           network->pathName(root_pin),
           delayAsString(tr->required_fall, sta),
           atof(delayAsString(tr->required_fall, sta)));
#endif
  }
}

//
// Cut Generation interfaces
//

/*
  Single output cut generation
  ----------------------------
  Walk back through instance input, building cut set.
  Constrained to last level of non-inverter type gates
*/

// start from an instance
void Restructure::generateWaveFrontSingleOpCutSet(sta::Network* nwk,
                                                  sta::Instance* root,
                                                  std::vector<Cut*>& cut_set)
{
  CutGen cut_generator(nwk, nullptr);
  cut_generator.GenerateInstanceWaveFrontCutSet(root, cut_set);
}

// start from a pin
void Restructure::generateWaveFrontSingleOpCutSet(sta::Network* nwk,
                                                  sta::Pin* root,
                                                  std::vector<Cut*>& cut_set)
{
  if (nwk->direction(root)->isOutput()) {
    sta::Instance* cur_inst = nwk->instance(root);
    generateWaveFrontSingleOpCutSet(nwk, cur_inst, cut_set);
  }
}

//
// Generate a big cut (like a blob)l
//

/*
  Generic Timing Driven Multiple output cut generation
  ---------------------------------------------------
  1. queue = Set up end points sorted by criticallity
  2. Pick most critical end point
  3. Walk back to invariant drivers. Set up cut Leaves
  4. Walk forwards to end points. Set up cut roots.
     (note we might have to then add some extra leaves).
  5. Extract cut and add to cut set.
  6. Remove cut roots from queue.
  7. If queue not empty go to step 1.
  8. Return cut set.
  (This is equivalent to a blob in OpenROAD speak).
*/

void Restructure::generateUnconstrainedCuts(std::vector<Cut*>& cut_set)
{
  sta::Graph* graph = open_sta_->ensureGraph();
  open_sta_->ensureLevelized();
  open_sta_->searchPreamble();
  int cut_id = 0;
  std::set<sta::Vertex*> end_points;
  sta::Network* nwk = open_sta_->getDbNetwork();

  // sort based on slack time: most negative first.
  std::multimap<float, sta::Vertex*> sorted_slack_times;  // slack -> vertex
  std::multimap<sta::Vertex*, float> vertex_slack_times;  // vertex -> slack

  // visitor
  // AccumulateSortedSlackEndPoints accumulate_sorted_slack_end_points(
  //      nwk, sorted_slack_times, vertex_slack_times, end_points);
  sta::Sta* sta = sta::Sta::sta();

  // accumulate the start and end points
  // open_sta_->visitEndpoints(&accumulate_sorted_slack_end_points);
  sta::PinSet endpointPins = open_sta_->endpointPins();
  for (const sta::Pin* pin : endpointPins) {
    sta::Slack max_s = sta->pinSlack(pin, sta::MinMax::max());
    sta::Slack min_s = sta->pinSlack(pin, sta::MinMax::min());
    sta::Slack s = (max_s == sta::INF || max_s == -sta::INF) ? min_s : max_s;

    debugPrint(logger_,
               utl::RMP,
               "remap",
               1,
               "Design Worst Slack time {}",
               delayAsString(s, sta));

    sta::Vertex* v = graph->vertex(nwk->vertexId(pin));
    if (isRegInput(nwk, v) || isPrimary(nwk, v)) {
      sorted_slack_times.insert(std::make_pair(s, v));
      vertex_slack_times.insert(std::make_pair(v, s));
      end_points_.insert(v);
    }
  }

  if (logger_->debugCheck(utl::RMP, "remap", 1)) {
    logger_->report("Dump of end points");
    for (auto sst : sorted_slack_times) {
      logger_->report("Pin slack time{}",
                      // sst.second,
                      delayAsString(sst.first, sta));
    }
  }

  // AccumulateSortedSlackStartPoints accumulate_sorted_slack_start_points(
  //  end_points);
  //  open_sta_->visitStartpoints(&accumulate_sorted_slack_start_points);
  sta::PinSet startpointPins = open_sta_->startpointPins();
  for (const sta::Pin* pin : startpointPins) {
    sta::Vertex* v = graph->vertex(nwk->vertexId(pin));
    end_points_.insert(v);
  }

  debugPrint(logger_,
             utl::RMP,
             "remap",
             1,
             "Starting cut construction for end point list of size {}",
             vertex_slack_times.size());

  logger_->report("Worst slack {}  Best slack {} Size of end point list {}",
                  delayAsString((*sorted_slack_times.begin()).first, sta),
                  delayAsString((*sorted_slack_times.end()).first, sta),
                  vertex_slack_times.size());
  // map stores smallest item first, so this is by default the least slack
  sta::Vertex* head_vertex = (*sorted_slack_times.begin()).second;
  sta::Pin* cur_pin = head_vertex->pin();
  debugPrint(logger_,
             utl::RMP,
             "remap",
             1,
             "Generating cut from pin {} on instance {}",
             nwk->pathName(cur_pin),
             nwk->name(nwk->instance(cur_pin)));

  ResetCutTemporaries();
  // harvest the leaves
  walkBackwardsToTimingEndPointsR(graph, nwk, cur_pin, 0);
  pin_visited_.clear();
  // walk forward from each leaf. Harvest the volume in forward pass
  for (auto leaf_pin_int : leaves_) {
    walkForwardsToTimingEndPointsR(graph, nwk, leaf_pin_int.first, 0);
  }
  // as we walked forwards there might be orphaned inputs,
  // these are the incidental inputs not in the original
  // leaf set, add them to the leaf set
  AmendCutForEscapeLeaves(nwk);

  // check if cut degenerate.
  Cut* cut = extractCut(cut_id);
  logger_->report("Extracted cut {} with {} roots and {} leaves and Volume {} ",
                  cut->id_,
                  cut->roots_.size(),
                  cut->leaves_.size(),
                  cut->volume_.size());
  if (!cut || cut->roots_.size() == 0) {
    logger_->report("done with cut generation. Generated {} cuts", cut_id);
    return;
  }
  cut_set.push_back(cut);
  cut_id++;
  // Check cut meets all required rules
  cut->Check(nwk);
  // just consider one cut for now.
  // so break from loop. Original idea:
  // keep on generating cuts until we have covered the
  // whole chip -- we repeatedly remove from the end vertex set..
  return;
}

void Restructure::AmendCutForEscapeLeaves(sta::Network* nwk)
{
  std::set<sta::Pin*> cut_leaves;
  std::set<sta::Instance*> cut_volume;
  for (auto i : cut_volume_) {
    cut_volume.insert(i);
  }
  for (auto l : leaves_) {
    cut_leaves.insert(l.first);
  }
  for (auto i : cut_volume_) {
    sta::InstancePinIterator* pin_it = nwk->pinIterator(i);
    while (pin_it->hasNext()) {
      sta::Pin* cur_pin = pin_it->next();
      // Check that every driver is either driven by something in the volume
      // or a leaf
      if (nwk->direction(cur_pin)->isInput()) {
        sta::PinSet* drivers = nwk->drivers(cur_pin);
        sta::PinSet::Iterator drvr_iter(drivers);
        if (drivers) {
          sta::Pin* driving_pin = const_cast<sta::Pin*>(drvr_iter.next());
          sta::Instance* driving_instance = nwk->instance(driving_pin);
          // leaf or volume...
          if (!(cut_leaves.find(driving_pin) != cut_leaves.end()
                || cut_volume.find(driving_instance) != cut_volume.end())) {
            leaves_[driving_pin] = leaves_.size();
            cut_leaves.insert(driving_pin);
          }
        }
      }
    }
  }
}

Cut* Restructure::extractCut(int cut_id)
{
  Cut* ret = new Cut();
  ret->leaves_.resize(leaves_.size());
  for (auto i : leaves_)
    ret->leaves_[i.second] = i.first;
  ret->roots_.resize(roots_.size());
  for (auto i : roots_)
    ret->roots_[i.second] = i.first;
  for (auto i : cut_volume_)
    ret->volume_.push_back(i);
  ret->id_ = cut_id;
  return ret;
}

void Restructure::walkBackwardsToTimingEndPointsR(sta::Graph* graph,
                                                  sta::Network* nwk,
                                                  sta::Pin* start_pin,
                                                  int depth)
{
  if (start_pin && pin_visited_.find(start_pin) == pin_visited_.end()) {
    pin_visited_.insert(start_pin);
    sta::Port* start_port = nwk->port(start_pin);
    sta::Vertex* vertex = graph->vertex(nwk->vertexId(start_pin));
    sta::Instance* cur_inst = nwk->instance(start_pin);

    // hit an invariant end point
    // include q outputs of registers as invariants
    if (isRegOutput(nwk, vertex)
        || ((end_points_.find(vertex) != end_points_.end()) && depth != 0)) {
      leaves_[start_pin] = leaves_.size();
      return;
    }
    // hit an invariant point: the output of a primary port (top level port)
    else if (nwk->direction(start_port) == sta::PortDirection::input()
             && isPrimary(nwk, vertex) && depth != 0) {
      leaves_[start_pin] = leaves_.size();
      return;
    }
    // a primary output at depth 0, walk
    else if (nwk->direction(start_port) == sta::PortDirection::output()
             && isPrimary(nwk, vertex) && depth == 0) {
      // traverse back
      sta::PinSet* drivers = nwk->drivers(start_pin);
      if (drivers) {
        sta::PinSet::Iterator drvr_iter(drivers);
        sta::Pin* driving_pin = const_cast<sta::Pin*>(drvr_iter.next());
        walkBackwardsToTimingEndPointsR(graph, nwk, driving_pin, depth + 1);
      }
      return;
    }

    // an instance input
    else if (nwk->direction(start_port) == sta::PortDirection::input()
             && !isPrimary(nwk, vertex)) {
      // traverse back
      sta::PinSet* drivers = nwk->drivers(start_pin);
      if (drivers) {
        sta::PinSet::Iterator drvr_iter(drivers);
        sta::Pin* driving_pin = const_cast<sta::Pin*>(drvr_iter.next());
        walkBackwardsToTimingEndPointsR(graph, nwk, driving_pin, depth + 1);
      }
      return;
    }
    //
    // an instance output
    // go through all the other instance inputs too
    //
    else if (nwk->direction(start_port) == sta::PortDirection::output()
             && !isPrimary(nwk, vertex)) {
      // push up to parent and walk back through input pins
      sta::Instance* cur_inst = nwk->instance(start_pin);
      // insert in volume knowing this is not an end point
      // now go check out the inputs and start backward traversing
      sta::InstancePinIterator* pi = nwk->pinIterator(cur_inst);
      while (pi->hasNext()) {
        sta::Pin* cur_pin = pi->next();
        if (nwk->direction(cur_pin) == sta::PortDirection::input()) {
          // get the drivers
          // walk through
          // traverse back
          sta::PinSet* drivers = nwk->drivers(cur_pin);
          if (drivers) {
            sta::PinSet::Iterator drvr_iter(drivers);
            sta::Pin* driving_pin = const_cast<sta::Pin*>(drvr_iter.next());
            walkBackwardsToTimingEndPointsR(graph, nwk, driving_pin, depth + 1);
          }
        }
      }
    }
  }
}

void Restructure::walkForwardsToTimingEndPointsR(sta::Graph* graph,
                                                 sta::Network* nwk,
                                                 sta::Pin* start_pin,
                                                 int depth)
{
  if (start_pin && pin_visited_.find(start_pin) == pin_visited_.end()) {
    sta::Instance* cur_inst = nwk->instance(start_pin);

    pin_visited_.insert(start_pin);

    sta::Port* start_port = nwk->port(start_pin);
    sta::Vertex* vertex = graph->vertex(nwk->vertexId(start_pin));

    // hit an end point. we are done
    if ((end_points_.find(vertex) != end_points_.end()) && depth != 0) {
      // avoid make clock and reset lines points for resynthesis
      if (!((strstr(nwk->pathName(start_pin), "CK"))
            || (strstr(nwk->pathName(start_pin), "RN"))
            || (strstr(nwk->pathName(start_pin), "QN"))
            || (strstr(nwk->pathName(start_pin), "SN"))
            || (strstr(nwk->pathName(start_pin), "Q")))) {
        if (roots_.find(start_pin) == roots_.end())
          roots_[start_pin] = roots_.size();
      }
      return;
    }

    // primary input
    if (nwk->direction(start_port) == sta::PortDirection::input()
        && isPrimary(nwk, vertex)) {
      // traverse forwards
      sta::PinConnectedPinIterator* connected_pin_iter
          = nwk->connectedPinIterator(start_pin);
      while (connected_pin_iter->hasNext()) {
        sta::Pin* connected_pin
            = const_cast<sta::Pin*>(connected_pin_iter->next());
        walkForwardsToTimingEndPointsR(graph, nwk, connected_pin, depth + 1);
      }
    }

    // output of an instance
    else if (nwk->direction(start_port) == sta::PortDirection::output()) {
      sta::PinConnectedPinIterator* connected_pin_iter
          = nwk->connectedPinIterator(start_pin);
      sta::Instance* cur_inst = nwk->instance(start_pin);
      if (depth != 0)
        cut_volume_.insert(cur_inst);
      while (connected_pin_iter->hasNext()) {
        sta::Pin* connected_pin
            = const_cast<sta::Pin*>(connected_pin_iter->next());
        walkForwardsToTimingEndPointsR(graph, nwk, connected_pin, depth + 1);
      }
    }

    // an instance input. Walk through to the output. Then explore fanout
    else if (nwk->direction(start_port) == sta::PortDirection::input()
             && !isPrimary(nwk, vertex)) {
      sta::Instance* cur_inst = nwk->instance(start_pin);
      if (depth != 0)
        cut_volume_.insert(cur_inst);

      sta::InstancePinIterator* pi = nwk->pinIterator(cur_inst);
      while (pi->hasNext()) {
        sta::Pin* cur_pin = pi->next();
        if (nwk->direction(nwk->port(cur_pin))
            == sta::PortDirection::output()) {
          // get the fanout of the pin
          sta::Net* cur_net = nwk->net(cur_pin);
          if (cur_net) {
            sta::NetPinIterator* fanout_pin_iter = nwk->pinIterator(cur_net);
            while (fanout_pin_iter->hasNext()) {
              const sta::Pin* fanout_pin = fanout_pin_iter->next();
              if (fanout_pin != cur_pin) {
                sta::Vertex* vertex = graph->vertex(nwk->vertexId(fanout_pin));
                if (isPrimary(nwk, vertex) || isRegInput(nwk, vertex)) {
                  if ((strstr(nwk->pathName(fanout_pin), "CK"))
                      || (strstr(nwk->pathName(fanout_pin), "RN"))
                      || (strstr(nwk->pathName(fanout_pin), "QN"))
                      || (strstr(nwk->pathName(fanout_pin), "SN"))
                      || (strstr(nwk->pathName(fanout_pin), "Q")))
                    continue;
                  else {
                    if (roots_.find(const_cast<sta::Pin*>(fanout_pin))
                        == roots_.end())
                      roots_[const_cast<sta::Pin*>(fanout_pin)] = roots_.size();
                  }
                } else {
                  if (nwk->direction(fanout_pin)
                      == sta::PortDirection::input()) {
                    walkForwardsToTimingEndPointsR(
                        graph,
                        nwk,
                        const_cast<sta::Pin*>(fanout_pin),
                        depth + 1);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

}  // namespace rmp
