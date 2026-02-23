// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

%{
#include "rmp/Restructure.h"
#include "cut/blif.h"
#include "ord/OpenRoad.hh"
#include "odb/db.h"
#include "sta/Liberty.hh"

namespace ord {
// Defined in OpenRoad.i
rmp::Restructure *
getRestructure();

OpenRoad *
getOpenRoad();
}


using namespace rmp;
using ord::getRestructure;
using ord::getOpenRoad;
using odb::dbInst;
using sta::LibertyPort;
using sta::Scene;
%}

%include "../../Exception.i"
%include "tcl/StaTclTypes.i"

%inline %{


void set_tielo_port_cmd(LibertyPort* tieLoport)
{
  getRestructure()->setTieLoPort(tieLoport);
}

void set_tiehi_port_cmd(LibertyPort* tieHiport)
{
  getRestructure()->setTieHiPort(tieHiport);
}

void
set_slack_threshold(float slack_threshold)
{
  getRestructure()->setSlackThreshold(slack_threshold);
}

void
set_annealing_seed(std::mt19937::result_type annealing_seed)
{
  getRestructure()->setAnnealingSeed(annealing_seed);
}

void
set_annealing_temp(float annealing_temp)
{
  getRestructure()->setAnnealingTemp(annealing_temp);
}

void
set_annealing_iters(int annealing_iters)
{
  getRestructure()->setAnnealingIters(annealing_iters);
}

void
set_annealing_revert_after(int annealing_revert_after)
{
  getRestructure()->setAnnealingRevertAfter(annealing_revert_after);
}

void
set_annealing_initial_ops(int set_annealing_initial_ops)
{
  getRestructure()->setAnnealingInitialOps(set_annealing_initial_ops);
}

void resynth_cmd(Scene* corner) {
  getRestructure()->resynth(corner);
}

void resynth_annealing_cmd(Scene* corner) {
  getRestructure()->resynthAnnealing(corner);
}

void
restructure_cmd(char* liberty_file_name, char* target, float slack_threshold,
                int depth_threshold, char* workdir_name, char* abc_logfile)
{
  getRestructure()->setMode(target);
  getRestructure()->run(liberty_file_name, slack_threshold, depth_threshold,
                        workdir_name, abc_logfile);
}

// Locally Exposed for testing only..
cut::Blif* create_blif(const char* hicell, const char* hiport, const char* locell, const char* loport, const int call_id=1){
  return new cut::Blif(getOpenRoad()->getLogger(), getOpenRoad()->getSta(), locell, loport, hicell, hiport, call_id);
}

void blif_add_instance(cut::Blif* blif_, const char* inst_){
  blif_->addReplaceableInstance(getOpenRoad()->getDb()->getChip()->getBlock()->findInst(inst_));
}

void blif_dump(cut::Blif* blif_, const char* file_name){
  blif_->writeBlif(file_name);
}

int blif_read(cut::Blif* blif_, const char* file_name){
  return blif_->readBlif(file_name, getOpenRoad()->getDb()->getChip()->getBlock());
}

%}
