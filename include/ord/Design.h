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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace odb {
class dbBlock;
class dbMaster;
class dbMTerm;
class dbNet;
class dbInst;
class dbITerm;
}  // namespace odb

namespace ifp {
class InitFloorplan;
}

namespace utl {
class Logger;
}

namespace ant {
class AntennaChecker;
}

namespace grt {
class GlobalRouter;
}

namespace gpl {
class Replace;
}

namespace dpl {
class Opendp;
}

namespace mpl {
class MacroPlacer;
}

namespace ppl {
class IOPlacer;
}

namespace tap {
class Tapcell;
}

namespace cts {
class TritonCTS;
}

namespace triton_route {
class TritonRoute;
}

namespace dpo {
class Optdp;
}

namespace fin {
class Finale;
}

namespace par {
class PartitionMgr;
}

namespace rcx {
class Ext;
}

namespace rmp {
class Restructure;
}

namespace stt {
class SteinerTreeBuilder;
}

namespace psm {
class PDNSim;
}

namespace pdn {
class PdnGen;
}

namespace pad {
class ICeWall;
}

namespace sta {
class dbSta;
class LibertyCell;
}  // namespace sta

namespace ord {

class Tech;

class Design
{
 public:
  explicit Design(Tech* tech);
  void readVerilog(const std::string& file_name);
  void readDef(const std::string& file_name,
               bool continue_on_errors = false,
               bool floorplan_init = false,
               bool incremental = false,
               bool child = false);
  void link(const std::string& design_name);

  void readDb(const std::string& file_name);
  void writeDb(const std::string& file_name);
  void writeDef(const std::string& file_name);

  odb::dbBlock* getBlock();
  utl::Logger* getLogger();

  int micronToDBU(double coord);

  // This is intended as a temporary back door to tcl from Python
  const std::string evalTclString(const std::string& cmd);

  Tech* getTech();

  bool isSequential(odb::dbMaster* master);
  bool isBuffer(odb::dbMaster* master);
  bool isInverter(odb::dbMaster* master);
  bool isInSupply(odb::dbITerm* pin);
  std::string getITermName(odb::dbITerm* pin);
  bool isInClock(odb::dbInst* inst);
  std::uint64_t getNetRoutedLength(odb::dbNet* net);

  // Services
  ifp::InitFloorplan* getFloorplan();
  ant::AntennaChecker* getAntennaChecker();
  grt::GlobalRouter* getGlobalRouter();
  gpl::Replace* getReplace();
  dpl::Opendp* getOpendp();
  mpl::MacroPlacer* getMacroPlacer();
  ppl::IOPlacer* getIOPlacer();
  tap::Tapcell* getTapcell();
  cts::TritonCTS* getTritonCts();
  triton_route::TritonRoute* getTritonRoute();
  dpo::Optdp* getOptdp();
  fin::Finale* getFinale();
  par::PartitionMgr* getPartitionMgr();
  rcx::Ext* getOpenRCX();
  rmp::Restructure* getRestructure();
  stt::SteinerTreeBuilder* getSteinerTreeBuilder();
  psm::PDNSim* getPDNSim();
  pdn::PdnGen* getPdnGen();
  pad::ICeWall* getICeWall();

 private:
  sta::dbSta* getSta();
  sta::LibertyCell* getLibertyCell(odb::dbMaster* master);

  Tech* tech_;
};

}  // namespace ord
