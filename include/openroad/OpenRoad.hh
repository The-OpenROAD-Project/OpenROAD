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

#ifndef OPENROAD_H
#define OPENROAD_H

#include <string>

extern "C" {
struct Tcl_Interp;
}

namespace odb {
class dbDatabase;
}

namespace sta {
class dbSta;
class dbNetwork;
class Resizer;
}

namespace pdngen {
class PdnGen;
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

namespace psn {
class Psn;
}

namespace MacroPlace {
class TritonMacroPlace;
}

namespace OpenRCX {
class Ext;
}

namespace ord {

using std::string;

class dbVerilogNetwork;

// Only pointers to components so the header has no dependents.
class OpenRoad
{
public:
  OpenRoad();
  ~OpenRoad();
  // Singleton accessor used by tcl command interpreter.
  static OpenRoad *openRoad() { return openroad_; }
  void init(Tcl_Interp *tcl_interp);

  Tcl_Interp *tclInterp() { return tcl_interp_; }
  pdngen::PdnGen *getPdnGen(){ return pdngen_; }
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

  void readLef(const char *filename,
	       const char *lib_name,
	       bool make_tech,
	       bool make_library);

  void readDef(const char *filename,
               bool order_wires);
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

private:
  Tcl_Interp *tcl_interp_;
  odb::dbDatabase *db_;
  dbVerilogNetwork *verilog_network_;
  sta::dbSta *sta_;
  sta::Resizer *resizer_;
  ioPlacer::IOPlacementKernel *ioPlacer_;
  opendp::Opendp *opendp_;
  MacroPlace::TritonMacroPlace *tritonMp_;
  pdngen::PdnGen *pdngen_;
  FastRoute::FastRouteKernel *fastRoute_;
  TritonCTS::TritonCTSKernel *tritonCts_;
  tapcell::Tapcell *tapcell_;
  OpenRCX::Ext *extractor_;

  // Singleton used by tcl command interpreter.
  static OpenRoad *openroad_;
};

} // namespace

#endif
