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

%{
#include "rmp/Restructure.h"
#include "rmp/blif.h"
#include "ord/OpenRoad.hh"
#include "opendb/db.h"
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
%}

%include "../../Exception.i"

%inline %{


void set_logfile_cmd(const char* logfile)
{
  getRestructure()->setLogfile(logfile);
}

void set_tielo_port_cmd(LibertyPort* tieLoport)
{
  getRestructure()->setTieLoPort(tieLoport);
}

void set_tiehi_port_cmd(LibertyPort* tieHiport)
{
  getRestructure()->setTieHiPort(tieHiport);
}

void
restructure_cmd(char* liberty_file_name, char* target, float slack_threshold, int depth_threshold, char* workdir_name)
{
  getRestructure()->setMode(target);
  getRestructure()->run(liberty_file_name, slack_threshold, depth_threshold, workdir_name);
}

// Locally Exposed for testing only..
Blif* create_blif(const char* hicell, const char* hiport, const char* locell, const char* loport){
  return new rmp::Blif(getOpenRoad()->getLogger(), getOpenRoad()->getSta(), locell, loport, hicell, hiport);
}

void blif_add_instance(Blif* blif_, const char* inst_){
  blif_->addReplaceableInstance(getOpenRoad()->getDb()->getChip()->getBlock()->findInst(inst_));
}

void blif_dump(Blif* blif_, const char* file_name){
  getOpenRoad()->getSta()->ensureGraph();
  getOpenRoad()->getSta()->ensureLevelized();
  getOpenRoad()->getSta()->searchPreamble();
  blif_->writeBlif(file_name);
}

int blif_read(Blif* blif_, const char* file_name){
  return blif_->readBlif(file_name, getOpenRoad()->getDb()->getChip()->getBlock());
}

%}
