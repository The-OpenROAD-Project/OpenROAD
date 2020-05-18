// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <string>
#include <set>
#include "Version.hh"

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
}

namespace sta {
class dbSta;
class dbNetwork;
class Resizer;
}

namespace ioPlacer {
class IOPlacementKernel;
}

namespace TritonCTS {
class TritonCTSKernel;
}

namespace FastRoute {
class FastRouteKernel;
}

namespace tapcell {
    class Tapcell;
}

namespace opendp {
class Opendp;
}

namespace MacroPlace {
class TritonMacroPlace;
}

namespace replace {
class Replace;
}

namespace OpenRCX {
class Ext;
}

namespace psn {
class Psn;
}

namespace pdnsim {
class PDNSim;
}

namespace PartClusManager {
class PartClusManagerKernel;
}

namespace ord {

using std::string;

class dbVerilogNetwork;

// Only pointers to components so the header has no dependents.
class OpenRoad
{
  OpenRoad();
  ~OpenRoad();
public:
  // Singleton accessor used by tcl command interpreter.
  static OpenRoad *openRoad();
  void init(Tcl_Interp *tcl_interp);

  Tcl_Interp *tclInterp() { return tcl_interp_; }
  odb::dbDatabase *getDb() { return db_; }
  sta::dbSta *getSta() { return sta_; }
  sta::dbNetwork *getDbNetwork();
  sta::Resizer *getResizer() { return resizer_; }
  TritonCTS::TritonCTSKernel *getTritonCts() { return tritonCts_; } 
  dbVerilogNetwork *getVerilogNetwork() { return verilog_network_; }
  opendp::Opendp *getOpendp() { return opendp_; }
  tapcell::Tapcell *getTapcell() { return tapcell_; }
  MacroPlace::TritonMacroPlace *getTritonMp() { return tritonMp_; }
  OpenRCX::Ext *getOpenRCX() { return extractor_; }
  replace::Replace* getReplace() { return replace_; }
  pdnsim::PDNSim* getPDNSim() { return pdnsim_; }
  PartClusManager::PartClusManagerKernel *getPartClusManager() { return partClusManager_; }
  FastRoute::FastRouteKernel* getFastRoute() { return fastRoute_; }
  // Return the bounding box of the db rows.
  odb::Rect getCore();
  // Return true if the command units have been initialized.
  bool unitsInitialized();
      
  void readLef(const char *filename,
	       const char *lib_name,
	       bool make_tech,
	       bool make_library);

  void readDef(const char *filename,
               bool order_wires,
               bool continue_on_errors);
  void writeDef(const char *filename,
		// major.minor (avoid including defout.h)
		string version);

  void readVerilog(const char *filename);
  // Write a flat verilog netlist for the database.
  void writeVerilog(const char *filename,
		    bool sort);
  void linkDesign(const char *top_cell_name);

  void readDb(const char *filename);
  void writeDb(const char *filename);

  // Observer interface
  class Observer
  {
  public:
    virtual ~Observer();

    // Either pointer could be null
    virtual void postReadLef(odb::dbTech* tech, odb::dbLib* library) = 0;
    virtual void postReadDef(odb::dbBlock* block) = 0;
    virtual void postReadDb(odb::dbDatabase* db) = 0;

  private:
    OpenRoad* owner_ = nullptr;
    friend class OpenRoad;
  };

  void addObserver(Observer *observer);
  void removeObserver(Observer *observer);

private:
  Tcl_Interp *tcl_interp_;
  odb::dbDatabase *db_;
  dbVerilogNetwork *verilog_network_;
  sta::dbSta *sta_;
  sta::Resizer *resizer_;
  ioPlacer::IOPlacementKernel *ioPlacer_;
  opendp::Opendp *opendp_;
  MacroPlace::TritonMacroPlace *tritonMp_;
  FastRoute::FastRouteKernel *fastRoute_;
  TritonCTS::TritonCTSKernel *tritonCts_;
  tapcell::Tapcell *tapcell_;
  OpenRCX::Ext *extractor_;
#ifdef BUILD_OPENPHYSYN
  psn::Psn *psn_;
#endif
  replace::Replace *replace_;
  pdnsim::PDNSim *pdnsim_; 
  PartClusManager::PartClusManagerKernel *partClusManager_;      

  std::set<Observer *> observers_;

  // Singleton used by tcl command interpreter.
  static OpenRoad *openroad_;
};

// Return the bounding box of the db rows.
odb::Rect
getCore(odb::dbBlock *block);

// Return the point inside rect that is closest to pt.
odb::Point
closestPtInRect(odb::Rect rect,
		odb::Point pt);

} // namespace
