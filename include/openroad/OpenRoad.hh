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

namespace odb {
class dbDatabase;
}

namespace sta {
class dbSta;
class dbNetwork;
class Resizer;
}

namespace ord {

class dbVerilogNetwork;

// Only pointers to components so the header has no dependents.
class OpenRoad
{
public:
  OpenRoad();
  ~OpenRoad();
  // Singleton accessor used by tcl command interpreter.
  static OpenRoad *openRoad() { return openroad_; }
  void init(Tcl_Interp *interp,
	    const char *prog_arg);

  odb::dbDatabase *getDb() { return db_; }
  sta::dbSta *getSta() { return sta_; }
  sta::dbNetwork *getDbNetwork();
  sta::Resizer *getResizer() { return resizer_; }

  void readLef(const char *filename,
	       const char *lib_name,
	       bool make_tech,
	       bool make_library);

  void readDef(const char *filename);
  void writeDef(const char *filename);

  void readVerilog(const char *filename);
  // Write a flat verilog netlist for the database.
  void writeVerilog(const char *filename,
		    bool sort);
  void linkDesign(const char *top_cell_name);

  void readDb(const char *filename);
  void writeDb(const char *filename);

private:
  odb::dbDatabase *db_;
  ord::dbVerilogNetwork *verilog_network_;
  sta::dbSta *sta_;
  sta::Resizer *resizer_;

  // Singleton used by tcl command interpreter.
  static OpenRoad *openroad_;
};

} // namespace

#endif
