// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

%{
#include "rmp/Restructure.h"
#include "rmp/blif.h"
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
using sta::Corner;
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

void resynth_cmd(Corner* corner) {
  getRestructure()->resynth(corner);
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
Blif* create_blif(const char* hicell, const char* hiport, const char* locell, const char* loport, const int call_id=1){
  return new rmp::Blif(getOpenRoad()->getLogger(), getOpenRoad()->getSta(), locell, loport, hicell, hiport, call_id);
}

void blif_add_instance(Blif* blif_, const char* inst_){
  blif_->addReplaceableInstance(getOpenRoad()->getDb()->getChip()->getBlock()->findInst(inst_));
}

void blif_dump(Blif* blif_, const char* file_name){
  blif_->writeBlif(file_name);
}

int blif_read(Blif* blif_, const char* file_name){
  return blif_->readBlif(file_name, getOpenRoad()->getDb()->getChip()->getBlock());
}

%}
