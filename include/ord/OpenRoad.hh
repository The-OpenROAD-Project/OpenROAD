// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

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
class VerilogReader;
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

namespace fin {
class Finale;
}

namespace mpl {
class MacroPlacer;
}

namespace gpl {
class Replace;
}

namespace rcx {
class Ext;
}

namespace drt {
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
  static void setOpenRoad(OpenRoad* app, bool reinit_ok = false);
  void init(Tcl_Interp* tcl_interp,
            const char* log_filename,
            const char* metrics_filename,
            bool batch_mode);

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
  fin::Finale* getFinale() { return finale_; }
  tap::Tapcell* getTapcell() { return tapcell_; }
  mpl::MacroPlacer* getMacroPlacer() { return macro_placer_; }
  rcx::Ext* getOpenRCX() { return extractor_; }
  drt::TritonRoute* getTritonRoute() { return detailed_router_; }
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
  void linkDesign(const char* design_name,
                  bool hierarchy,
                  bool omit_filename_prop = false);
  // Used if a design is created programmatically rather than loaded
  // to notify the tools (eg dbSta, gui).
  void designCreated();

  void readDb(std::istream& stream);
  void readDb(const char* filename, bool hierarchy = false);
  void writeDb(std::ostream& stream);
  void writeDb(const char* filename);

  void setThreadCount(int threads, bool printInfo = true);
  void setThreadCount(const char* threads, bool printInfo = true);
  int getThreadCount();

  std::string getExePath() const;
  std::string getDocsPath() const;

  static const char* getVersion();
  static const char* getGitDescribe();

  static bool getGPUCompileOption();
  static bool getPythonCompileOption();
  static bool getGUICompileOption();

 protected:
  ~OpenRoad();

 private:
  OpenRoad();

  Tcl_Interp* tcl_interp_ = nullptr;
  utl::Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  dbVerilogNetwork* verilog_network_ = nullptr;
  sta::VerilogReader* verilog_reader_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  ppl::IOPlacer* ioPlacer_ = nullptr;
  dpl::Opendp* opendp_ = nullptr;
  fin::Finale* finale_ = nullptr;
  mpl::MacroPlacer* macro_placer_ = nullptr;
  grt::GlobalRouter* global_router_ = nullptr;
  rmp::Restructure* restructure_ = nullptr;
  cts::TritonCTS* tritonCts_ = nullptr;
  tap::Tapcell* tapcell_ = nullptr;
  rcx::Ext* extractor_ = nullptr;
  drt::TritonRoute* detailed_router_ = nullptr;
  ant::AntennaChecker* antenna_checker_ = nullptr;
  gpl::Replace* replace_ = nullptr;
  psm::PDNSim* pdnsim_ = nullptr;
  par::PartitionMgr* partitionMgr_ = nullptr;
  pdn::PdnGen* pdngen_ = nullptr;
  pad::ICeWall* icewall_ = nullptr;
  dst::Distributed* distributer_ = nullptr;
  stt::SteinerTreeBuilder* stt_builder_ = nullptr;
  dft::Dft* dft_ = nullptr;

  int threads_ = 1;

  static OpenRoad* app_;

  friend class Tech;
};

int tclAppInit(Tcl_Interp* interp);
int tclInit(Tcl_Interp* interp);

}  // namespace ord
