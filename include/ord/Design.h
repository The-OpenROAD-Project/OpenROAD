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
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace odb {
class dbBlock;
class dbDatabase;
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

namespace dft {
class Dft;
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

namespace drt {
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

namespace rsz {
class Resizer;
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

class OpenRoad;
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

  void readDb(std::istream& stream);
  void readDb(const std::string& file_name);
  void writeDb(std::ostream& stream);
  void writeDb(const std::string& file_name);
  void writeDef(const std::string& file_name);

  odb::dbBlock* getBlock();
  utl::Logger* getLogger();

  int micronToDBU(double coord);

  // This is intended as a temporary back door to tcl from Python
  std::string evalTclString(const std::string& cmd);

  Tech* getTech();

  bool isSequential(odb::dbMaster* master);
  bool isBuffer(odb::dbMaster* master);
  bool isInverter(odb::dbMaster* master);
  bool isInSupply(odb::dbITerm* pin);
  std::string getITermName(odb::dbITerm* pin);
  bool isInClock(odb::dbInst* inst);
  bool isInClock(odb::dbITerm* iterm);
  std::uint64_t getNetRoutedLength(odb::dbNet* net);

  // Services
  ant::AntennaChecker* getAntennaChecker();
  cts::TritonCTS* getTritonCts();
  dft::Dft* getDft();
  dpl::Opendp* getOpendp();
  dpo::Optdp* getOptdp();
  drt::TritonRoute* getTritonRoute();
  fin::Finale* getFinale();
  gpl::Replace* getReplace();
  grt::GlobalRouter* getGlobalRouter();
  ifp::InitFloorplan getFloorplan();
  mpl::MacroPlacer* getMacroPlacer();
  odb::dbDatabase* getDb();
  pad::ICeWall* getICeWall();
  par::PartitionMgr* getPartitionMgr();
  pdn::PdnGen* getPdnGen();
  ppl::IOPlacer* getIOPlacer();
  psm::PDNSim* getPDNSim();
  rcx::Ext* getOpenRCX();
  rmp::Restructure* getRestructure();
  rsz::Resizer* getResizer();
  stt::SteinerTreeBuilder* getSteinerTreeBuilder();
  tap::Tapcell* getTapcell();

  // Needed by standalone startup, not for general use.
  ord::OpenRoad* getOpenRoad();

  // This returns a database that is not the one associated with
  // the rest of the application.  It is usable as a standalone
  // db but should not passed to any other Design or Tech APIs.
  //
  // This is useful if you need a second database for specialized
  // use cases and is not ordinarily required.
  static odb::dbDatabase* createDetachedDb();

 private:
  sta::dbSta* getSta();
  sta::LibertyCell* getLibertyCell(odb::dbMaster* master);

  Tech* tech_;

  // Single-thread access to the interpreter in evalTclString
  static std::mutex interp_mutex;
};

}  // namespace ord
