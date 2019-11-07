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

#include "opendb/db.h"
#include "opendb/lefin.h"
#include "opendb/defin.h"
#include "opendb/defout.h"
#include "Machine.hh"
#include "Report.hh"
#include "VerilogWriter.hh"
#include "db_sta/dbSta.hh"
#include "resizer/Resizer.hh"
#include "openroad/OpenRoad.hh"
#include "dbReadVerilog.hh"

namespace ord {

OpenRoad *OpenRoad::openroad_ = nullptr;

OpenRoad::OpenRoad(dbDatabase *db) :
  db_(db),
  sta_(new sta::dbSta(db_)),
  resizer_(new sta::Resizer(sta_))
{
}

OpenRoad::~OpenRoad()
{
  delete sta_;
  delete resizer_;
  odb::dbDatabase::destroy(db_);
}

void
OpenRoad::setOpenRoad(OpenRoad *openroad)
{
  openroad_ = openroad;
}

void
OpenRoad::readLef(const char *filename,
		  const char *lib_name,
		  bool make_tech,
		  bool make_library)
{
  odb::lefin lef_reader(db_, false);
  if (make_tech && make_library)
    lef_reader.createTechAndLib(lib_name, filename);
  else if (make_tech)
    lef_reader.createTech(filename);
  else if (make_library)
    lef_reader.createLib(lib_name, filename);
}

void
OpenRoad::readDef(const char *filename)
{
  if (db_->getTech() == nullptr)
    sta_->report()->error("no technology has been read.\n");
  else {
    odb::defin def_reader(db_);
    std::vector<odb::dbLib *> search_libs;
    for (odb::dbLib *lib : db_->getLibs())
      search_libs.push_back(lib);
    def_reader.createChip(search_libs, filename);
    sta_->readDbAfter();
  }
}

void
OpenRoad::writeDef(const char *filename)
{
  odb::dbChip *chip = db_->getChip();
  if (chip) {
    odb::dbBlock *block = chip->getBlock();
    if (block) {
      odb::defout def_writer;
      def_writer.writeBlock(block, filename);
    }
  }
}

void
OpenRoad::readDb(const char *filename)
{
  FILE *stream = fopen(filename, "r");
  if (stream) {
    db_->read(stream);
    sta_->readDbAfter();
    fclose(stream);
  }
}

void
OpenRoad::writeDb(const char *filename)
{
  FILE *stream = fopen(filename, "w");
  if (stream) {
    db_->write(stream);
    fclose(stream);
  }
}

void
OpenRoad::readVerilog(const char *filename)
{
  ord::dbReadVerilog(filename,
		     sta_->report(),
		     sta_->debug());
}

void
OpenRoad::linkDesign(const char *top_cell_name)

{
  if (db_->getTech() == nullptr)
    sta_->report()->error("no technology has been read.\n");
  else {
    dbLinkDesign(top_cell_name, db_);
    sta_->readDbAfter();
  }
}

void
OpenRoad::writeVerilog(const char *filename,
		       bool sort)
{
  sta::writeVerilog(filename, sort, sta_->network());
}

////////////////////////////////////////////////////////////////

sta::dbNetwork *
OpenRoad::getDbNetwork()
{
  return sta_->getDbNetwork();
}

sta::Resizer *
OpenRoad::getResizer()
{
  return resizer_;
}

} // namespace
