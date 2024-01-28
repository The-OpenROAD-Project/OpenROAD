/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
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

#include <set>
#include <string>
#include <vector>

#include "OpenRoadObserver.hh"

extern "C" {
struct Tcl_Interp;
}

namespace odb {
class dbDatabase;
class dbBlock;
class dbTech;
class dbLib;
class Point;
class Rect;
}  // namespace odb

namespace sta {
class dbSta;
class dbNetwork;
}  // namespace sta

namespace rsz {
class Resizer;
}

namespace ppl {
class IOPlacer;
}

namespace rmp {
class Restructure;
}

namespace cts {
class TritonCTS;
}

namespace grt {
class GlobalRouter;
}

namespace tap {
class Tapcell;
}

namespace dpl {
class Opendp;
}

namespace dpo {
class Optdp;
}

namespace fin {
class Finale;
}

namespace mpl {
class MacroPlacer;
}

namespace mpl2 {
class MacroPlacer2;
}

namespace gpl {
class Replace;
}

namespace rcx {
class Ext;
}

namespace triton_route {
class TritonRoute;
}

namespace psm {
class PDNSim;
}

namespace ant {
class AntennaChecker;
}

namespace par {
class PartitionMgr;
}

namespace pdn {
class PdnGen;
}

namespace pad {
class ICeWall;
}

namespace utl {
class Logger;
}

namespace dst {
class Distributed;
}
namespace stt {
class SteinerTreeBuilder;
}

namespace dft {
class Dft;
}

namespace ord {

using std::string;

class dbVerilogNetwork;

// Only pointers to components so the header has no dependents.
class OpenRoad
{
 public:
  // Singleton accessor.
  // This accessor should ONLY be used for tcl commands.
  // Tools should use their initialization functions to get the
  // OpenRoad object and/or any other tools they need to reference.
  static OpenRoad* openRoad();
  void init(Tcl_Interp* tcl_interp);

  Tcl_Interp* tclInterp() { return tcl_interp_; }
  utl::Logger* getLogger() { return logger_; }
  odb::dbDatabase* getDb() { return db_; }
  sta::dbSta* getSta() { return sta_; }
  sta::dbNetwork* getDbNetwork();
  rsz::Resizer* getResizer() { return resizer_; }
  rmp::Restructure* getRestructure() { return restructure_; }
  cts::TritonCTS* getTritonCts() { return tritonCts_; }
  dbVerilogNetwork* getVerilogNetwork() { return verilog_network_; }
  dpl::Opendp* getOpendp() { return opendp_; }
  dpo::Optdp* getOptdp() { return optdp_; }
  fin::Finale* getFinale() { return finale_; }
  tap::Tapcell* getTapcell() { return tapcell_; }
  mpl::MacroPlacer* getMacroPlacer() { return macro_placer_; }
  mpl2::MacroPlacer2* getMacroPlacer2() { return macro_placer2_; }
  rcx::Ext* getOpenRCX() { return extractor_; }
  triton_route::TritonRoute* getTritonRoute() { return detailed_router_; }
  gpl::Replace* getReplace() { return replace_; }
  psm::PDNSim* getPDNSim() { return pdnsim_; }
  grt::GlobalRouter* getGlobalRouter() { return global_router_; }
  par::PartitionMgr* getPartitionMgr() { return partitionMgr_; }
  ant::AntennaChecker* getAntennaChecker() { return antenna_checker_; }
  ppl::IOPlacer* getIOPlacer() { return ioPlacer_; }
  pdn::PdnGen* getPdnGen() { return pdngen_; }
  pad::ICeWall* getICeWall() { return icewall_; }
  dst::Distributed* getDistributed() { return distributer_; }
  stt::SteinerTreeBuilder* getSteinerTreeBuilder() { return stt_builder_; }
  dft::Dft* getDft() { return dft_; }

  // Return the bounding box of the db rows.
  odb::Rect getCore();
  // Return true if the command units have been initialized.
  bool unitsInitialized();

  void readLef(const char* filename,
               const char* lib_name,
               const char* tech_name,
               bool make_tech,
               bool make_library);

  void readDef(const char* filename,
               odb::dbTech* tech,
               bool continue_on_errors,
               bool floorplan_init,
               bool incremental,
               bool child);

  void writeLef(const char* filename);

  void writeAbstractLef(const char* filename,
                        int bloat_factor,
                        bool bloat_occupied_layers);

  void writeDef(const char* filename,
                // major.minor (avoid including defout.h)
                const string& version);

  void writeCdl(const char* outFilename,
                const std::vector<const char*>& mastersFilenames,
                bool includeFillers);

  void readVerilog(const char* filename);
  void linkDesign(const char* design_name);

  // Used if a design is created programmatically rather than loaded
  // to notify the tools (eg dbSta, gui).
  void designCreated();

  void readDb(const char* filename);
  void writeDb(const char* filename);

  void diffDbs(const char* filename1, const char* filename2, const char* diffs);

  void setThreadCount(int threads, bool printInfo = true);
  void setThreadCount(const char* threads, bool printInfo = true);
  int getThreadCount();

  void addObserver(OpenRoadObserver* observer);
  void removeObserver(OpenRoadObserver* observer);

  static const char* getVersion();
  static const char* getGitDescribe();

 protected:
  ~OpenRoad();

 private:
  OpenRoad();

  Tcl_Interp* tcl_interp_ = nullptr;
  utl::Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  dbVerilogNetwork* verilog_network_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  ppl::IOPlacer* ioPlacer_ = nullptr;
  dpl::Opendp* opendp_ = nullptr;
  dpo::Optdp* optdp_ = nullptr;
  fin::Finale* finale_ = nullptr;
  mpl::MacroPlacer* macro_placer_ = nullptr;
  mpl2::MacroPlacer2* macro_placer2_ = nullptr;
  grt::GlobalRouter* global_router_ = nullptr;
  rmp::Restructure* restructure_ = nullptr;
  cts::TritonCTS* tritonCts_ = nullptr;
  tap::Tapcell* tapcell_ = nullptr;
  rcx::Ext* extractor_ = nullptr;
  triton_route::TritonRoute* detailed_router_ = nullptr;
  ant::AntennaChecker* antenna_checker_ = nullptr;
  gpl::Replace* replace_ = nullptr;
  psm::PDNSim* pdnsim_ = nullptr;
  par::PartitionMgr* partitionMgr_ = nullptr;
  pdn::PdnGen* pdngen_ = nullptr;
  pad::ICeWall* icewall_ = nullptr;
  dst::Distributed* distributer_ = nullptr;
  stt::SteinerTreeBuilder* stt_builder_ = nullptr;
  dft::Dft* dft_ = nullptr;

  std::set<OpenRoadObserver*> observers_;

  int threads_ = 1;
};

int tclAppInit(Tcl_Interp* interp);

}  // namespace ord
