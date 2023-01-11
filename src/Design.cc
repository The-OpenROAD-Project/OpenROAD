/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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

#include "ord/Design.h"

#include "ant/AntennaChecker.hh"
#include "grt/GlobalRouter.h"
#include "ifp/InitFloorplan.hh"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "ord/Tech.h"
#include "utl/Logger.h"

namespace ord {

Design::Design(Tech* tech) : tech_(tech)
{
}

odb::dbBlock* Design::getBlock()
{
  auto chip = tech_->getDB()->getChip();
  return chip ? chip->getBlock() : nullptr;
}

void Design::readVerilog(const std::string& file_name)
{
  auto chip = tech_->getDB()->getChip();
  if (chip && chip->getBlock()) {
    getLogger()->error(utl::ORD, 36, "A block already exists in the db");
  }

  auto app = OpenRoad::openRoad();
  app->readVerilog(file_name.c_str());
}

void Design::readDef(const std::string& file_name,
                     bool continue_on_errors,  // = false
                     bool floorplan_init,      // = false
                     bool incremental          // = false
)
{
  auto app = OpenRoad::openRoad();
  if (floorplan_init && incremental) {
    getLogger()->error(utl::ORD,
                       101,
                       "Only one of the options -incremental and"
                       " -floorplan_init can be set at a time");
  }
  if (tech_->getDB()->getTech() == nullptr) {
    getLogger()->error(utl::ORD, 102, "No technology has been read.");
  }
  app->readDef(
      file_name.c_str(), continue_on_errors, floorplan_init, incremental);
}

void Design::link(const std::string& design_name)
{
  auto app = OpenRoad::openRoad();
  app->linkDesign(design_name.c_str());
}

void Design::readDb(const std::string& file_name)
{
  auto app = OpenRoad::openRoad();
  app->readDb(file_name.c_str());
}

void Design::writeDb(const std::string& file_name)
{
  auto app = OpenRoad::openRoad();
  app->writeDb(file_name.c_str());
}

void Design::writeDef(const std::string& file_name)
{
  auto app = OpenRoad::openRoad();
  app->writeDef(file_name.c_str(), "5.8");
}

ifp::InitFloorplan* Design::getFloorplan()
{
  auto app = OpenRoad::openRoad();
  auto block = getBlock();
  if (!block) {
    getLogger()->error(utl::ORD, 37, "No block loaded.");
  }
  return new ifp::InitFloorplan(block, app->getLogger(), app->getDbNetwork());
}

utl::Logger* Design::getLogger()
{
  auto app = OpenRoad::openRoad();
  return app->getLogger();
}

int Design::micronToDBU(double coord)
{
  int dbuPerMicron = getBlock()->getDbUnitsPerMicron();
  return round(coord * dbuPerMicron);
}

ant::AntennaChecker* Design::getAntennaChecker()
{
  auto app = OpenRoad::openRoad();
  return app->getAntennaChecker();
}

const std::string Design::evalTclString(const std::string& cmd)
{
  Tcl_Interp* tcl_interp = OpenRoad::openRoad()->tclInterp();
  Tcl_Eval(tcl_interp, cmd.c_str());
  return std::string(Tcl_GetStringResult(tcl_interp));
}

Tech* Design::getTech()
{
  return tech_;
}

grt::GlobalRouter* Design::getGlobalRouter()
{
  auto app = OpenRoad::openRoad();
  return app->getGlobalRouter();
}

gpl::Replace* Design::getReplace()
{
  auto app = OpenRoad::openRoad();
  return app->getReplace();
}

dpl::Opendp* Design::getOpendp()
{
  auto app = OpenRoad::openRoad();
  return app->getOpendp();
}

mpl::MacroPlacer* Design::getMacroPlacer()
{
  auto app = OpenRoad::openRoad();
  return app->getMacroPlacer();
}

ppl::IOPlacer* Design::getIOPlacer()
{
  auto app = OpenRoad::openRoad();
  return app->getIOPlacer();
}

tap::Tapcell* Design::getTapcell()
{
  auto app = OpenRoad::openRoad();
  return app->getTapcell();
}

cts::TritonCTS* Design::getTritonCts()
{
  auto app = OpenRoad::openRoad();
  return app->getTritonCts();
}

triton_route::TritonRoute* Design::getTritonRoute()
{
  auto app = OpenRoad::openRoad();
  return app->getTritonRoute();
}

dpo::Optdp* Design::getOptdp()
{
  auto app = OpenRoad::openRoad();
  return app->getOptdp();
}

fin::Finale* Design::getFinale()
{
  auto app = OpenRoad::openRoad();
  return app->getFinale();
}

par::PartitionMgr* Design::getPartitionMgr()
{
  auto app = OpenRoad::openRoad();
  return app->getPartitionMgr();
}

rcx::Ext* Design::getOpenRCX()
{
  auto app = OpenRoad::openRoad();
  return app->getOpenRCX();
}

rmp::Restructure* Design::getRestructure()
{
  auto app = OpenRoad::openRoad();
  return app->getRestructure();
}

stt::SteinerTreeBuilder* Design::getSteinerTreeBuilder()
{
  auto app = OpenRoad::openRoad();
  return app->getSteinerTreeBuilder();
}

psm::PDNSim* Design::getPDNSim()
{
  auto app = OpenRoad::openRoad();
  return app->getPDNSim();
}

pdn::PdnGen* Design::getPdnGen()
{
  auto app = OpenRoad::openRoad();
  return app->getPdnGen();
}

}  // namespace ord
