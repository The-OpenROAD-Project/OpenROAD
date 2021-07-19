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

#include <fstream>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "opendb/db.h"
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

  getEndPoints(ends, opt_mode_ <= Mode::AREA_3, max_depth);
  if (ends.size()) {
    sta::PinSet fanin_fanouts = resizer_->findFaninFanouts(&ends);
    // fanin_fanouts.insert(ends.begin(), ends.end()); // Add seq cells
    logger_->report("Found {} pins in extracted logic", fanin_fanouts.size());
    for (sta::Pin* pin : fanin_fanouts) {
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
  blif_.writeBlif(input_blif_file_name_.c_str());
  debugPrint(
      logger_, RMP, "remap", 1, "Writing blif file {}", input_blif_file_name_);
  files_to_remove.emplace_back(input_blif_file_name_);

  // abc optimization
  bool area_mode = opt_mode_ <= Mode::AREA_3;
  std::string abc_script_file = work_dir_name_ + "ord_abc_script.tcl";
  std::string best_blif;
  int best_inst_count = std::numeric_limits<int>::max();
  for (int opt_mode = static_cast<int>(area_mode)
                          ? static_cast<int>(Mode::AREA_1)
                          : static_cast<int>(Mode::DELAY_1);
       opt_mode <= (static_cast<int>(area_mode)
                        ? static_cast<int>(Mode::AREA_3)
                        : static_cast<int>(Mode::DELAY_4));
       opt_mode++) {
    opt_mode_ = (Mode) opt_mode;
    output_blif_file_name_ = work_dir_name_
                             + std::string(block_->getConstName())
                             + std::to_string(opt_mode) + "_crit_path_out.blif";
    debugPrint(logger_,
               RMP,
               "remap",
               1,
               "Writing ABC script file {}",
               abc_script_file);
    if (writeAbcScript(abc_script_file)) {
      std::string abc_command = std::string("yosys-abc < ") + abc_script_file;
      if (logfile_ != "")
        abc_command = abc_command + " > " + logfile_;
      int ret = std::system(abc_command.c_str());
      if (ret) {
        logger_->warn(
            RMP,
            11,
            "ABC failed with code {}, please check messages for details.",
            ret);
        continue;
      }
    }
    int num_instances = 0;
    bool status
        = blif_.inspectBlif(output_blif_file_name_.c_str(), num_instances);
    logger_->report(
        "Optimized to {} instances in iteration {}", num_instances, opt_mode);
    if (status && num_instances < best_inst_count) {
      best_inst_count = num_instances;
      best_blif = output_blif_file_name_;
    }
    files_to_remove.emplace_back(output_blif_file_name_);
  }
  files_to_remove.emplace_back(abc_script_file);
  if (best_inst_count < std::numeric_limits<int>::max()) {
    // read back netlist
    debugPrint(logger_, RMP, "remap", 1, "Reading blif file {}", best_blif);
    blif_.readBlif(best_blif.c_str(), block_);
    debugPrint(logger_,
               utl::RMP,
               "remap",
               1,
               "Number constants after restructure {}",
               countConsts(block_));
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
  auto sta_state = open_sta_->search();
  sta::VertexSet*  end_points  = sta_state->endpoints();
  std::size_t path_found = end_points->size();
  logger_->report("Number of paths for restructure are {}", path_found);
  //TODO: Sort?
  for (auto& end_point : *end_points) {
    if (opt_mode_ >= Mode::DELAY_1) {
      sta::Path* path = open_sta_->vertexWorstSlackPath(end_point, sta::MinMax::max()).path();
      sta::PathExpanded expanded(path, open_sta_);
      // Members in expanded include gate output and net so divide by 2
      logger_->report("Found path of depth {}", expanded.size() / 2);
      if (expanded.size() / 2 > max_depth) {
        ends.insert(end_point->pin());
      }
      // Consider only 5 timing paths exceeding path depth to limit blob size for timing
      if (ends.size() > 5)
        break;
    } else {
      ends.insert(end_point->pin());
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
  if (!errors.empty() && errors[0]->size() > 1) {
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
      auto mterm = iterm->getMTerm();
      auto net = iterm->getNet();

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
            logger_->warn(RMP, 35, "Could not create instance {}", inst_name);
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

  script << "read_blif " << input_blif_file_name_ << std::endl;

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
      script << "map -p -B 0.2 -A 0.9 -M 0" << std::endl;
      break;
    }
    case Mode::DELAY_2: {
      script << choice << std::endl;
      script << "map -p -B 0.2 -A 0.9 -M 0" << std::endl;
      script << choice << std::endl;
      script << "map" << std::endl << "topo" << std::endl;
      break;
    }
    case Mode::DELAY_3: {
      script << choice2 << std::endl;
      script << "map -p -B 0.2 -A 0.9 -M 0" << std::endl;
      script << choice2 << std::endl;
      script << "map" << std::endl << "topo" << std::endl;
      break;
    }
    case Mode::DELAY_4: {
      script << choice2 << std::endl;
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000" << std::endl;
      script << choice2 << std::endl;
      script << "map -p -B 0.2 -A 0.9 -M 0" << std::endl;
      break;
    }
    case Mode::AREA_2:
    case Mode::AREA_3: {
      script << choice2 << std::endl;
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000" << std::endl;
      script << choice2 << std::endl;
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000" << std::endl;
      break;
    }
    case Mode::AREA_1:
    default: {
      script << choice2 << std::endl;
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000" << std::endl;
      break;
    }
  }

  script << "upsize {D}" << std::endl << "dnsize {D}" << std::endl;
  script << "stime -p -c" << std::endl << "print_stats -m" << std::endl;
}

void Restructure::setMode(const char* mode_name)
{
  if (!strcmp(mode_name, "delay1") || !strcmp(mode_name, "delay"))
    opt_mode_ = Mode::DELAY_1;
  else if (!strcmp(mode_name, "delay2"))
    opt_mode_ = Mode::DELAY_2;
  else if (!strcmp(mode_name, "delay3"))
    opt_mode_ = Mode::DELAY_3;
  else if (!strcmp(mode_name, "delay4"))
    opt_mode_ = Mode::DELAY_4;
  else if (!strcmp(mode_name, "area1") || !strcmp(mode_name, "area"))
    opt_mode_ = Mode::AREA_1;
  else if (!strcmp(mode_name, "area2"))
    opt_mode_ = Mode::AREA_2;
  else if (!strcmp(mode_name, "area3"))
    opt_mode_ = Mode::AREA_3;
  else {
    logger_->report("Mode {} note recognized.", mode_name);
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

}  // namespace rmp
