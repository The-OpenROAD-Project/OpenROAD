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

#include "rmp/Restructure.h"
#include "rmp/blif.h"

#include "opendb/db.h"
#include "utl/Logger.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "ord/OpenRoad.hh"

#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/Sta.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/Graph.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "rsz/Resizer.hh"
#include "utl/Logger.h"

#include <fstream>

using utl::RMP;

namespace rmp {

void Restructure::init(ord::OpenRoad* openroad)
{
  openroad_ = openroad;
  logger_ = openroad->getLogger();
  makeComponents();
}

void Restructure::makeComponents()
{
  db_ = openroad_->getDb();
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
  libFileNames_.clear();
  pathInsts_.clear();
}

void Restructure::run(const char* libertyFileName, float slack_threshold, unsigned max_depth)
{
  reset();
  block_ = openroad_->getDb()->getChip()->getBlock();
  if (!block_)
    return;

  libFileNames_.emplace_back(std::string(libertyFileName));
  sta::Slack worst_slack = slack_threshold;
  openSta_ = openroad_->getSta();
  openSta_->ensureGraph();
  openSta_->ensureLevelized();
  openSta_->searchPreamble();

  getBlob(max_depth);

  runABC();

  postABC(worst_slack);
}

void Restructure::getBlob(unsigned max_depth)
{

  rsz::Resizer* resizer = openroad_->getResizer();

  sta::PinSet ends;
  
  getEndPoints(ends, optMode_ <= AREA_3_MODE, max_depth);
  if (ends.size()) {
      sta::PinSet fanin_fanouts = resizer->findFaninFanouts(&ends);
      //fanin_fanouts.insert(ends.begin(), ends.end()); // Add seq cells
      logger_->report("Got {} pins in cloud extraction", fanin_fanouts.size());
      for (sta::Pin *pin : fanin_fanouts) {
        odb::dbITerm* term = nullptr;
        odb::dbBTerm* port = nullptr;
        openSta_->getDbNetwork()->staToDb(pin, term, port);
        if (term && term->getInst()->getITerms().size() < 10)
          pathInsts_.insert(term->getInst());
      }
      logger_->report("Fount {} Insts for restructuring ...", pathInsts_.size());
  }
}

void Restructure::runABC()
{
  inputBlifFileName_ = std::string(block_->getConstName()) + "_crit_path.blif";
  std::vector<std::string> filesToRemove;

  debugPrint(logger_, utl::RMP, "remap", 1, "Constants before remap {}", countConsts(block_));

  blif blif_(openroad_, locell_, loport_, hicell_, hiport_);
  blif_.setReplaceableInstances(pathInsts_);
  blif_.writeBlif(inputBlifFileName_.c_str());
  debugPrint(logger_, RMP, "remap", 1, "Writing blif file {}", inputBlifFileName_);
  filesToRemove.emplace_back(inputBlifFileName_);

  // abc optimization
  bool areaMode = optMode_ <= AREA_3_MODE;
  std::string abcScriptFile = "ord_abc_script.tcl";
  std::string bestBlif;
  int bestInstCount = std::numeric_limits<int>::max();
  for (int optMode = areaMode ? AREA_1_MODE : DELAY_1_MODE;
            optMode <= (areaMode ? AREA_3_MODE : DELAY_4_MODE); optMode += 1) {
    optMode_ = (mode) optMode;
    outputBlifFileName_ = std::string(block_->getConstName()) + std::to_string(optMode) + "_crit_path_out.blif";
    debugPrint(logger_, RMP, "remap", 1, "Writing ABC script file {}", abcScriptFile);
    if (writeAbcScript(abcScriptFile)) {
      std::string abc_command = std::string("yosys-abc < ") + abcScriptFile;
      if (logfile_ != "")
        abc_command = abc_command + " > " + logfile_;
      std::system(abc_command.c_str());
    }
    int numInstances = 0;
    bool status = blif_.inspectBlif(outputBlifFileName_.c_str(), numInstances);
    logger_->report("Optimized to {} instance in iter {}", numInstances, optMode);
    if (status && numInstances < bestInstCount) {
      bestInstCount = numInstances;
      bestBlif = outputBlifFileName_;
    }
    filesToRemove.emplace_back(outputBlifFileName_);
  }
  filesToRemove.emplace_back(abcScriptFile);
  if (bestInstCount < std::numeric_limits<int>::max()) {
    // read back netlist
    debugPrint(logger_, RMP, "remap", 1, "Reading blif file {}", bestBlif);
    blif_.readBlif(bestBlif.c_str(), block_);
    debugPrint(logger_, utl::RMP, "remap", 1, "Number constants after restructure {}", countConsts(block_));
  }

  for (auto fileToRemove : filesToRemove) {
    if (!logger_->debugCheck(RMP, "remap", 1))
      std::remove(fileToRemove.c_str());
  }
}

void Restructure::postABC(float worst_slack)
{

  rsz::Resizer* resizer = openroad_->getResizer();
  bool areaMode = optMode_ <= AREA_3_MODE;
  if (!areaMode) {
      // Recompute timing
    resizer->estimateWireParasitics();
    openSta_->findRequireds();
    float new_slack;
    sta::Vertex *worst_vertex;
    openSta_->worstSlack(sta::MinMax::max(), new_slack, worst_vertex);
    if (!areaMode && new_slack < worst_slack) {
      // failed, revert
      
    }
    else {
      // slack improved, accept
    }
  }

  // Leave the parasitices up to date.
  resizer->estimateWireParasitics();
}
void Restructure::getEndPoints(sta::PinSet &ends, bool areaMode, unsigned max_depth)
{
  openSta_->ensureGraph();
  openSta_->searchPreamble();
  auto sta_state = openSta_->search();
  int path_count = 100000;
  float min_slack = areaMode ? -sta::INF : -sta::INF;
  float max_slack = areaMode ? sta::INF : 0;

  sta::PathEndSeq* path_ends
      = sta_state->findPathEnds(  // from, thrus, to, unconstrained
          nullptr,
          nullptr,
          nullptr,
          false,
          // corner, min_max,
          openSta_->findCorner("default"),
          sta::MinMaxAll::max(),
          // group_count, endpoint_count, unique_pins
          path_count,
          2,
          true,
          min_slack,
          max_slack,  // slack_min, slack_max,
          true,      // sort_by_slack
          nullptr,   // group_names
          // setup, hold, recovery, removal,
          true,
          true,
          false,
          false,
          // clk_gating_setup, clk_gating_hold
          false,
          false);
  bool skip_top_paths = false;
  std::size_t path_found = path_ends->size();
  if (path_found > 1000) {
    skip_top_paths = true;
  }
  logger_->report("Number of paths for restructure {}", path_found);
  int end_count = 0;
  for (auto& path_end : *path_ends) {
    if (optMode_ >= DELAY_1_MODE) {
      sta::PathExpanded expanded(path_end->path(), openSta_);
      logger_->report("Got path of depth {}", expanded.size()/2);
      if (expanded.size()/2 > max_depth) {
        ends.insert(path_end->vertex(sta_state)->pin());
        end_count++;
      }
      if (end_count > 5) // limit blob size for timing
        break;
    } else {
      ends.insert(path_end->vertex(sta_state)->pin());
    }
  }

  // unconstrained end points
  auto errors = openSta_->checkTiming(false, false, false, true, true, false, false);
  debugPrint(logger_, RMP, "remap", 1, "Size of errors = {}", errors.size());
  if (errors.size() && errors[0]->size() > 1) {
    sta::CheckError* error = errors[0];
    bool first = true;
    for (auto pinName : *error) {
      debugPrint(logger_, RMP, "remap", 1, "Unconstrained pin: {}", pinName);
      if (!first && openSta_->getDbNetwork()->findPin(pinName)) {
        ends.insert(openSta_->getDbNetwork()->findPin(pinName));
      }
    }
  }
  if (errors.size() > 1 && errors[1]->size() > 1) {
    sta::CheckError* error = errors[1];
    bool first = true;
    for (auto pinName : *error) {
      debugPrint(logger_, RMP, "remap", 1, "Unclocked pin: {}", pinName);
      if (!first && openSta_->getDbNetwork()->findPin(pinName)) {
        ends.insert(openSta_->getDbNetwork()->findPin(pinName));
      }
    }
  }
  logger_->report("Number of end points for restructure {}", ends.size());
}

int Restructure::countConsts(odb::dbBlock* topBlock)
{
  int constNets = 0;
  int constPins = 0;
  int constVertices = 0;
  for (auto blockNet : topBlock->getNets()) {
    if (blockNet->getSigType() == odb::dbSigType::GROUND
                    || blockNet->getSigType() == odb::dbSigType::POWER)
      constNets++;

#if 0 // Wait till STA exposes sim.hh
    for (auto iterm : blockNet->getITerms()) {
      if (iterm->getIoType() != odb::dbIoType::OUTPUT)
        continue;
      sta::Pin* p = openSta_->getDbNetwork()->dbToSta(iterm);
      if (p && openSta_->sim()->logicZeroOne(p)) {
        constPins++;
        //if (openSta_->graph()->pinLoadVertex(p))
        odb::dbInst* dbFF = iterm->getInst();
        sta::LibertyCell* libCell = openSta_->getDbNetwork()->libertyCell(dbFF);
        odb::dbITerm* output = dbFF->getFirstOutput();
        if (output && libCell->hasSequentials())
          constVertices++;
      }
    }
#endif
  }
  //logger_->report("Const pins = {}, Const Vertices = {}", constPins, constVertices);

  return constNets;
}

bool Restructure::writeAbcScript(std::string fileName)
{
  std::ofstream script(fileName.c_str());

  if (!script.is_open()) {
    logger_->error(RMP, 2, "Cannot open file %s for writing.", fileName);
    return false;
  }

  for (auto libName : libFileNames_) {
    std::string readLibStr = "read_lib " + libName + "\n";
    script << readLibStr;
  }
  
  script << "read_blif " << inputBlifFileName_ << std::endl;

  if (logger_->debugCheck(RMP, "remap", 1))
    script << "write_verilog " << inputBlifFileName_ + std::string(".v") << std::endl;

  writeOptCommands(script);

  script << "write_blif " << outputBlifFileName_ << std:: endl;

  if (logger_->debugCheck(RMP, "remap", 1))
    script << "write_verilog " << outputBlifFileName_ + std::string(".v") << std:: endl;

  script.close();
  
  return true;
}

void Restructure::writeOptCommands(std::ofstream &script)
{
  std::string choice = "alias choice \"fraig_store; resyn2; fraig_store; resyn2; fraig_store; fraig_restore\"";
  std::string choice2 = "alias choice2 \"fraig_store; balance; fraig_store; resyn2; fraig_store; resyn2; fraig_store; resyn2; fraig_store; fraig_restore\"";
  script << "bdd; sop" << std::endl;

  script << "alias resyn2 \"balance; rewrite; refactor; balance; rewrite; rewrite -z; balance; refactor -z; rewrite -z; balance\"" << std::endl;
  script << choice << std::endl;
  script << choice2 << std::endl;

  if (optMode_ == AREA_3_MODE)
    script << "choice2" << std::endl;// << "scleanup" << std::endl;
  else
    script << "resyn2" << std::endl;// << "scleanup" << std::endl;

  switch (optMode_) {
    case DELAY_1_MODE:
    {
      script << "map -p -B 0.2 -A 0.9 -M 0" << std::endl;
      break;
    }
    case DELAY_2_MODE:
    {
      script << choice << std::endl;
      script << "map -p -B 0.2 -A 0.9 -M 0" << std::endl;
      script << choice << std::endl;
      script << "map" << std::endl << "topo" << std::endl;
      break;
    }
    case DELAY_3_MODE:
    {
      script << choice2 << std::endl;
      script << "map -p -B 0.2 -A 0.9 -M 0" << std::endl;
      script << choice2 << std::endl;
      script << "map" << std::endl << "topo" << std::endl;
      break;

    }
    case DELAY_4_MODE:
    {
      script << choice2 << std::endl;
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000" << std::endl;
      script << choice2 << std::endl;
      script << "map -p -B 0.2 -A 0.9 -M 0" << std::endl;
      break;
    }
    case AREA_2_MODE:
    case AREA_3_MODE:
    {
      script << choice2 << std::endl;
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000" << std::endl;
      script << choice2 << std::endl;
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000" << std::endl;
      break;

    }
    case AREA_1_MODE:
    default:
    {
      script << choice2 << std::endl;
      script << "amap -m -Q 0.1 -F 20 -A 20 -C 5000" << std::endl;
      break;
    }
  }

  script << "upsize {D}" << std::endl << "dnsize {D}" << std::endl;
  script << "stime -p -c" << std::endl << "print_stats -m" << std::endl;

}

void Restructure::setMode(const char* modeName)
{
  if (!strcmp(modeName, "delay1") || !strcmp(modeName, "delay"))
    optMode_ = DELAY_1_MODE;
  else if (!strcmp(modeName, "delay2"))
    optMode_ = DELAY_2_MODE;
  else if (!strcmp(modeName, "delay3"))
    optMode_ = DELAY_3_MODE;
  else if (!strcmp(modeName, "delay4"))
    optMode_ = DELAY_4_MODE;
  else if (!strcmp(modeName, "area1") || !strcmp(modeName, "area"))
    optMode_ = AREA_1_MODE;
  else if (!strcmp(modeName, "area2"))
    optMode_ = AREA_2_MODE;
  else if (!strcmp(modeName, "area3"))
    optMode_ = AREA_3_MODE;
  else {
    logger_->report("Mode {} note recognized.", modeName);
  }
}

void Restructure::setLogfile(const char* logfile)
{
   logfile_ = logfile;
}

void Restructure::setLoCell(const char* locell)
{
   locell_ = locell;
}

void Restructure::setLoPort(const char* loport)
{
   loport_ = loport;
}

void Restructure::setHiCell(const char* hicell)
{
   hicell_ = hicell;
}

void Restructure::setHiPort(const char* hiport)
{
   hiport_ = hiport;
}

}  // namespace rmp
