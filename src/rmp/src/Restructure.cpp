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

#include <RestructureCallBack.h>
#include <fcntl.h>
#include <omp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <tcl.h>
#include <time.h>
#include <unistd.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

#include "RestructureJobDescription.h"
#include "base/abc/abc.h"
#include "base/main/abcapis.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dst/Distributed.h"
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
using namespace abc;

BOOST_CLASS_EXPORT(rmp::RestructureJobDescription)

namespace rmp {

void Restructure::init(utl::Logger* logger,
                       sta::dbSta* open_sta,
                       odb::dbDatabase* db,
                       dst::Distributed* dist,
                       rsz::Resizer* resizer)
{
  logger_ = logger;
  db_ = db;
  dist_ = dist;
  dist_->addCallBack(new RestructureCallBack(this, dist_, logger_));
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
                      char* abc_logfile,
                      const char* post_abc_script)
{
  reset();
  block_ = db_->getChip()->getBlock();
  if (!block_)
    return;

  logfile_ = abc_logfile;
  post_abc_script_ = post_abc_script;
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

void Restructure::setDistributed(const std::string& host, unsigned short port)
{
  dist_host_ = host;
  dist_port_ = port;
}

void Restructure::getBlob(unsigned max_depth)
{
  open_sta_->ensureGraph();
  open_sta_->ensureLevelized();
  open_sta_->searchPreamble();

  sta::PinSet ends;

  getEndPoints(ends, is_area_mode_, max_depth);
  if (ends.size()) {
    sta::PinSet boundary_points = !is_area_mode_
                                      ? resizer_->findFanins(&ends)
                                      : resizer_->findFaninFanouts(&ends);
    // fanin_fanouts.insert(ends.begin(), ends.end()); // Add seq cells
    logger_->report("Found {} pins in extracted logic.",
                    boundary_points.size());
    for (sta::Pin* pin : boundary_points) {
      odb::dbITerm* term = nullptr;
      odb::dbBTerm* port = nullptr;
      open_sta_->getDbNetwork()->staToDb(pin, term, port);
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

  Blif blif_(logger_, open_sta_, locell_, loport_, hicell_, hiport_);
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
    modes = {Mode::DELAY_5};
  }
  auto dbFile = fmt::format("{}rmp.odb", work_dir_name_);
  auto sdcFile = fmt::format("{}rmp.sdc", work_dir_name_);
  ord::OpenRoad::openRoad()->writeDb(dbFile.c_str());
  open_sta_->writeSdc(sdcFile.c_str(), false, true, 4, false, false);
  files_to_remove.push_back(dbFile);
  files_to_remove.push_back(sdcFile);
  if (dist_host_.empty()) {
    dist_host_ = "127.0.0.1";
    dist_port_ = 1110;             // load balancer port
    open_sta_->setThreadCount(0);  // needed for forking
    size_t num_workers = std::min(ord::OpenRoad::openRoad()->getThreadCount(),
                                  (int) modes.size() * MAX_ITERATIONS);
    for (int i = 0; i < num_workers + 1; i++) {
      pid_t c_pid = fork();
      if (c_pid == -1) {
        logger_->error(RMP, 17, "Forking Error");
      } else if (c_pid <= 0) {
        open_sta_->setThreadCount(1);
        if (i == 0) {
          for (int i = 1; i < num_workers + 1; i++) {
            dist_->addWorkerAddress(dist_host_.c_str(), 1110 + i);
          }
          dist_->runLoadBalancer(dist_host_.c_str(), 1110, "");
        } else
          dist_->runWorker(dist_host_.c_str(), 1110 + i, false);
      } else {
        child_proc.push_back(c_pid);
      }
    }
    open_sta_->setThreadCount(ord::OpenRoad::openRoad()->getThreadCount());
  }

  std::string best_blif;
  int best_inst_count = std::numeric_limits<int>::max();
  float best_delay_gain = -1 * std::numeric_limits<float>::max();

  debugPrint(
      logger_, RMP, "remap", 1, "Running ABC with {} modes.", modes.size());
  omp_set_num_threads(modes.size() * MAX_ITERATIONS);
#pragma omp parallel for collapse(2) schedule(dynamic)
  for (size_t curr_mode_idx = 0; curr_mode_idx < modes.size();
       curr_mode_idx++) {
    for (ushort iterations = 1; iterations <= MAX_ITERATIONS; iterations++) {
      int level_gain = 0;
      float delay = std::numeric_limits<float>::max();
      int num_instances = 0;
      std::string blif_path = input_blif_file_name_;
      {
        auto uDesc = std::make_unique<RestructureJobDescription>();
        uDesc->setLoCellPort(locell_, loport_);
        uDesc->setHiCellPort(hicell_, hiport_);
        uDesc->setMode(modes[curr_mode_idx]);
        uDesc->setWorkDirName(work_dir_name_);
        uDesc->setPostABCScript(post_abc_script_);
        uDesc->setBlifPath(input_blif_file_name_);
        uDesc->setIterations(iterations);
        uDesc->setLibFiles(lib_file_names_);
        uDesc->setODBPath(fmt::format("{}rmp.odb", work_dir_name_));
        uDesc->setSDCPath(fmt::format("{}rmp.sdc", work_dir_name_));
        std::vector<std::string> ids;
        for (auto inst : path_insts_) {
          ids.push_back(inst->getName());
        }
        uDesc->setReplaceableInstsIds(ids);
        dst::JobMessage msg(dst::JobMessage::RESTRUCTURE), result;
        msg.setJobDescription(std::move(uDesc));
        dist_->sendJob(msg, dist_host_.c_str(), dist_port_, result);
        RestructureJobDescription* resultDesc
            = static_cast<RestructureJobDescription*>(
                result.getJobDescription());
        num_instances = resultDesc->getNumInstances();
        delay = resultDesc->getDelay();
        level_gain = resultDesc->getLevelGain();
        blif_path = resultDesc->getBlifPath();
      }
#pragma omp critical
      {
        logger_->report(
            "Optimized to {} instances in iteration {} with max path depth "
            "decrease of {}, delay of {}.",
            num_instances,
            curr_mode_idx,
            level_gain,
            delay);
        files_to_remove.emplace_back(blif_path);
        if (is_area_mode_) {
          if (num_instances < best_inst_count) {
            best_inst_count = num_instances;
            best_blif = blif_path;
          }
        } else {
          if (delay > best_delay_gain) {
            best_delay_gain = delay;
            best_blif = blif_path;
          }
        }
      }
    }
  }
  // TODO: replace by sending an exit signal to the workers through dst
  for (auto pid : child_proc) {
    kill(pid, SIGKILL);
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
  sta::VertexSet* end_points = sta_state->endpoints();
  std::size_t path_found = end_points->size();
  logger_->report("Number of paths for restructure are {}", path_found);
  // sta::Slack worst_slack = 0;
  // sta::Vertex* worst_vertix;
  for (auto& end_point : *end_points) {
    if (!is_area_mode_) {
      sta::PathRef path_ref
          = open_sta_->vertexWorstSlackPath(end_point, sta::MinMax::max());
      sta::Path* path = path_ref.path();
      if (path == nullptr || path->slack(open_sta_) >= 0)
        continue;
      // auto path_slack = path->slack(open_sta_);
      sta::PathExpanded expanded(path, open_sta_);
      // Members in expanded include gate output and net so divide by 2
      if (expanded.size() / 2 > max_depth) {
        ends.insert(end_point->pin());
        // if (path_slack < worst_slack)
        // {
        //   worst_slack = path_slack;
        //   worst_vertix = end_point;
        // }
        // Use only one end point to limit blob size for timing
        // break;
      }
    } else {
      ends.insert(end_point->pin());
    }
  }
  // ends.clear();
  // ends.insert(worst_vertix->pin());

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

bool Restructure::writeAbcScript(std::string file_name,
                                 Mode mode,
                                 const ushort iterations)
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
  script << "print_stats" << std::endl;
  writeOptCommands(script, mode, iterations);

  script << "write_blif " << output_blif_file_name_ << std::endl;
  script << "print_stats" << std::endl;

  if (logger_->debugCheck(RMP, "remap", 1))
    script << "write_verilog " << output_blif_file_name_ + std::string(".v")
           << std::endl;

  script.close();

  return true;
}

void Restructure::writeOptCommands(std::ofstream& script,
                                   Mode mode,
                                   const ushort iterations)
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

  if (mode == Mode::AREA_3)
    script << "choice2" << std::endl;  // << "scleanup" << std::endl;
  else
    script << "resyn2" << std::endl;  // << "scleanup" << std::endl;

  switch (mode) {
    case Mode::DELAY_1: {
      for (ushort i = 0; i < iterations; i++)
        script << "map -D 0.01 -A 0.9 -B 0.2 -M 0 -p" << std::endl;
      script << "buffer -p -c" << std::endl;
      break;
    }
    case Mode::DELAY_2: {
      for (ushort i = 0; i < iterations; i++) {
        script << "choice" << std::endl;
        script << "map -D 0.01 -A 0.9 -B 0.2 -M 0 -p" << std::endl;
        script << "choice" << std::endl;
        script << "map -D 0.01" << std::endl;
      }
      script << "buffer -p -c" << std::endl << "topo" << std::endl;
      break;
    }
    case Mode::DELAY_3: {
      for (ushort i = 0; i < iterations; i++) {
        script << "choice2" << std::endl;
        script << "map -D 0.01 -A 0.9 -B 0.2 -M 0 -p" << std::endl;
        script << "choice2" << std::endl;
        script << "map -D 0.01" << std::endl;
      }
      script << "buffer -p -c" << std::endl << "topo" << std::endl;
      break;
    }
    case Mode::DELAY_4: {
      for (ushort i = 0; i < iterations; i++) {
        script << "choice2" << std::endl;
        script << "amap -F 20 -A 20 -C 5000 -Q 0.1 -m" << std::endl;
        script << "choice2" << std::endl;
        script << "map -D 0.01 -A 0.9 -B 0.2 -M 0 -p" << std::endl;
      }
      script << "buffer -p -c" << std::endl;
      break;
    }
    case Mode::DELAY_5: {
      for (ushort i = 0; i < iterations; i++)
        script << "&get; &st; &if -g -K 6; &dch; &nf; &put" << std::endl;
      break;
    }
    case Mode::DELAY_6: {
      script << "&get;" << std::endl;
      for (ushort i = 0; i < 1; i++)
        script << "&st; &if -g -K 6 -C 8" << std::endl;
      for (ushort i = 0; i < 4; i++)
        script << "&st; &dch; map" << std::endl;
      break;
    }
    case Mode::DELAY_7: {
      script << "&get;" << std::endl;
      for (ushort i = 0; i < 2; i++)
        script << "&st; &if -g -K 6 -C 8" << std::endl;
      for (ushort i = 0; i < 6; i++)
        script << "&st; &dch; map" << std::endl;
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

void Restructure::setTieHiPort(const std::string& cell, const std::string& port)
{
  hicell_ = cell;
  hiport_ = port;
}

void Restructure::setTieLoPort(sta::LibertyPort* tieLoPort)
{
  if (tieLoPort) {
    locell_ = tieLoPort->libertyCell()->name();
    loport_ = tieLoPort->name();
  }
}

void Restructure::setTieLoPort(const std::string& cell, const std::string& port)
{
  locell_ = cell;
  loport_ = port;
}

void Restructure::addLibFile(const std::string& lib_file)
{
  lib_file_names_.emplace_back(lib_file);
}

void Restructure::runABCJob(const Mode mode,
                            const ushort iterations,
                            int& num_instances,
                            int& level_gain,
                            float& delay,
                            std::string& blif_path)
{
  delay = -1 * std::numeric_limits<float>::max();
  num_instances = std::numeric_limits<int>::max();
  level_gain = std::numeric_limits<int>::min();
  const std::string abc_script_file = fmt::format(
      "{}{}_{}_ord_abc_script.tcl", work_dir_name_, mode, iterations);
  if (logfile_ == "")
    logfile_ = work_dir_name_ + "abc.log";
  input_blif_file_name_ = blif_path;
  blif_path = output_blif_file_name_
      = work_dir_name_ + std::to_string((int) mode) + "_crit_path_out.blif";
  std::vector<std::string> files_to_remove;
  Blif blif_(logger_, open_sta_, locell_, loport_, hicell_, hiport_);
  blif_.setReplaceableInstances(path_insts_);
  if (writeAbcScript(abc_script_file, mode, iterations)) {
    // call linked abc
    files_to_remove.emplace_back(abc_script_file);
    Abc_Start();
    Abc_Frame_t* abc_frame = Abc_FrameGetGlobalFrame();
    const std::string command = "source " + abc_script_file;

    fflush(stdout);
    int stdout_fd = dup(STDOUT_FILENO);
    const std::string abc_log_name
        = fmt::format("{}abc_{}_{}.log", work_dir_name_, mode, iterations);
    std::ofstream{abc_log_name.c_str()};
    files_to_remove.emplace_back(abc_log_name);
    int redir_fd = open(abc_log_name.c_str(), O_WRONLY);
    dup2(redir_fd, STDOUT_FILENO);
    close(redir_fd);
    auto pid = Cmd_CommandExecute(abc_frame, command.c_str());
    fflush(stdout);
    dup2(stdout_fd, STDOUT_FILENO);
    close(stdout_fd);

    if (pid) {
      logger_->warn(RMP, 13, "Error executing ABC command {}.", command);
      // for (const auto& file_to_remove : files_to_remove) {
      //   std::remove(file_to_remove.c_str());
      // }
      return;
    }
    Abc_Stop();

    readAbcLog(abc_log_name, level_gain, delay);
    blif_.inspectBlif(output_blif_file_name_.c_str(), num_instances);
  }
  blif_.readBlif(output_blif_file_name_.c_str(), block_);
  postABC(0);

  for (const auto& file_to_remove : files_to_remove) {
    std::remove(file_to_remove.c_str());
  }
  if (!post_abc_script_.empty())
    Tcl_EvalFile(ord::OpenRoad::openRoad()->tclInterp(),
                 post_abc_script_.c_str());
  delay = open_sta_->worstSlack(sta::MinMax::max());

  logger_->report("Worst Slack {}", delay);
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
        prev_token = token;
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
}  // namespace rmp
