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

#include <tcl.h>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "grt/GlobalRouter.h"
#include "ifp/InitFloorplan.hh"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "ord/Tech.h"
#include "utl/Logger.h"

namespace ord {

std::mutex Design::interp_mutex;

Design::Design(Tech* tech) : tech_(tech)
{
}

ord::OpenRoad* Design::getOpenRoad()
{
  return tech_->app_;
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

  getOpenRoad()->readVerilog(file_name.c_str());
}

void Design::readDef(const std::string& file_name,
                     bool continue_on_errors,  // = false
                     bool floorplan_init,      // = false
                     bool incremental,         // = false
                     bool child                // = false
)
{
  if (floorplan_init && incremental) {
    getLogger()->error(utl::ORD,
                       101,
                       "Only one of the options -incremental and"
                       " -floorplan_init can be set at a time");
  }
  if (tech_->getDB()->getTech() == nullptr) {
    getLogger()->error(utl::ORD, 102, "No technology has been read.");
  }
  getOpenRoad()->readDef(file_name.c_str(),
                         tech_->getDB()->getTech(),
                         continue_on_errors,
                         floorplan_init,
                         incremental,
                         child);
}

void Design::link(const std::string& design_name)
{
  getOpenRoad()->linkDesign(design_name.c_str(), false);
}

void Design::readDb(std::istream& stream)
{
  getOpenRoad()->readDb(stream);
}

void Design::readDb(const std::string& file_name)
{
  getOpenRoad()->readDb(file_name.c_str());
}

void Design::writeDb(std::ostream& stream)
{
  getOpenRoad()->writeDb(stream);
}

void Design::writeDb(const std::string& file_name)
{
  getOpenRoad()->writeDb(file_name.c_str());
}

void Design::writeDef(const std::string& file_name)
{
  getOpenRoad()->writeDef(file_name.c_str(), "5.8");
}

ifp::InitFloorplan Design::getFloorplan()
{
  auto block = getBlock();
  if (!block) {
    getLogger()->error(utl::ORD, 37, "No block loaded.");
  }
  return ifp::InitFloorplan(block, getLogger(), getSta()->getDbNetwork());
}

utl::Logger* Design::getLogger()
{
  return getOpenRoad()->getLogger();
}

int Design::micronToDBU(double coord)
{
  const int dbuPerMicron = getBlock()->getDbUnitsPerMicron();
  return round(coord * dbuPerMicron);
}

ant::AntennaChecker* Design::getAntennaChecker()
{
  return getOpenRoad()->getAntennaChecker();
}

std::string Design::evalTclString(const std::string& cmd)
{
  const std::lock_guard<std::mutex> lock(interp_mutex);
  auto openroad = getOpenRoad();
  ord::OpenRoad::setOpenRoad(openroad, /* reinit_ok */ true);
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  Tcl_Eval(tcl_interp, cmd.c_str());
  return std::string(Tcl_GetStringResult(tcl_interp));
}

Tech* Design::getTech()
{
  return tech_;
}

sta::dbSta* Design::getSta()
{
  return tech_->getSta();
}

sta::LibertyCell* Design::getLibertyCell(odb::dbMaster* master)
{
  sta::dbNetwork* network = getSta()->getDbNetwork();

  sta::Cell* cell = network->dbToSta(master);
  if (!cell) {
    return nullptr;
  }
  return network->libertyCell(cell);
}

bool Design::isBuffer(odb::dbMaster* master)
{
  auto lib_cell = getLibertyCell(master);
  if (!lib_cell) {
    return false;
  }
  return lib_cell->isBuffer();
}

bool Design::isInverter(odb::dbMaster* master)
{
  auto lib_cell = getLibertyCell(master);
  if (!lib_cell) {
    return false;
  }
  return lib_cell->isInverter();
}

bool Design::isSequential(odb::dbMaster* master)
{
  auto lib_cell = getLibertyCell(master);
  if (!lib_cell) {
    return false;
  }
  return lib_cell->hasSequentials();
}

bool Design::isInClock(odb::dbInst* inst)
{
  for (auto* iterm : inst->getITerms()) {
    auto* net = iterm->getNet();
    if (net != nullptr && net->getSigType() == odb::dbSigType::CLOCK) {
      return true;
    }
  }
  return false;
}

bool Design::isInClock(odb::dbITerm* iterm)
{
  auto* net = iterm->getNet();
  if (net != nullptr && net->getSigType() == odb::dbSigType::CLOCK) {
    return true;
  }
  return false;
}

std::string Design::getITermName(odb::dbITerm* pin)
{
  return pin->getName();
}

bool Design::isInSupply(odb::dbITerm* pin)
{
  return pin->getSigType().isSupply();
}

std::uint64_t Design::getNetRoutedLength(odb::dbNet* net)
{
  std::uint64_t route_length = 0;
  if (net->getSigType().isSupply()) {
    for (odb::dbSWire* swire : net->getSWires()) {
      for (odb::dbSBox* wire : swire->getWires()) {
        if (wire != nullptr && !(wire->isVia())) {
          route_length += wire->getLength();
        }
      }
    }
  } else {
    auto* wire = net->getWire();
    if (wire != nullptr) {
      route_length += wire->getLength();
    }
  }
  return route_length;
}

grt::GlobalRouter* Design::getGlobalRouter()
{
  return getOpenRoad()->getGlobalRouter();
}

gpl::Replace* Design::getReplace()
{
  return getOpenRoad()->getReplace();
}

dpl::Opendp* Design::getOpendp()
{
  return getOpenRoad()->getOpendp();
}

mpl2::MacroPlacer2* Design::getMacroPlacer2()
{
  return getOpenRoad()->getMacroPlacer2();
}

mpl::MacroPlacer* Design::getMacroPlacer()
{
  return getOpenRoad()->getMacroPlacer();
}

ppl::IOPlacer* Design::getIOPlacer()
{
  return getOpenRoad()->getIOPlacer();
}

tap::Tapcell* Design::getTapcell()
{
  return getOpenRoad()->getTapcell();
}

cts::TritonCTS* Design::getTritonCts()
{
  return getOpenRoad()->getTritonCts();
}

drt::TritonRoute* Design::getTritonRoute()
{
  return getOpenRoad()->getTritonRoute();
}

dpo::Optdp* Design::getOptdp()
{
  return getOpenRoad()->getOptdp();
}

fin::Finale* Design::getFinale()
{
  return getOpenRoad()->getFinale();
}

par::PartitionMgr* Design::getPartitionMgr()
{
  return getOpenRoad()->getPartitionMgr();
}

rcx::Ext* Design::getOpenRCX()
{
  return getOpenRoad()->getOpenRCX();
}

rmp::Restructure* Design::getRestructure()
{
  return getOpenRoad()->getRestructure();
}

stt::SteinerTreeBuilder* Design::getSteinerTreeBuilder()
{
  return getOpenRoad()->getSteinerTreeBuilder();
}

psm::PDNSim* Design::getPDNSim()
{
  return getOpenRoad()->getPDNSim();
}

pdn::PdnGen* Design::getPdnGen()
{
  return getOpenRoad()->getPdnGen();
}

pad::ICeWall* Design::getICeWall()
{
  return getOpenRoad()->getICeWall();
}

dft::Dft* Design::getDft()
{
  return getOpenRoad()->getDft();
}

odb::dbDatabase* Design::getDb()
{
  return getOpenRoad()->getDb();
}

rsz::Resizer* Design::getResizer()
{
  return getOpenRoad()->getResizer();
}

/* static */
odb::dbDatabase* Design::createDetachedDb()
{
  auto app = OpenRoad::openRoad();
  auto db = odb::dbDatabase::create();
  db->setLogger(app->getLogger());
  return db;
}

}  // namespace ord
