/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, James Cherry, Parallax Software, Inc.
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

namespace antenna_checker {
class AntennaChecker;
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
  FastRoute::FastRouteKernel* getFastRoute() { return fastRoute_; }
  antenna_checker::AntennaChecker *getAntennaChecker(){ return antennaChecker_; }
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
  antenna_checker::AntennaChecker *antennaChecker_;
#ifdef BUILD_OPENPHYSYN
  psn::Psn *psn_;
#endif
  replace::Replace *replace_;
  pdnsim::PDNSim *pdnsim_; 

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
odb::Point
closestPtInRect(odb::Rect rect,
		int x,
		int y);

} // namespace
