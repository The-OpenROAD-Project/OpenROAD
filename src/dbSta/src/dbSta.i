%{

#include "odb/db.h"
#include "db_sta/dbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "ord/OpenRoad.hh"

namespace ord {
// Defined in OpenRoad.i
OpenRoad *getOpenRoad();
}

using sta::Instance;

%}

%import "odb.i"
%include "../../src/Exception.i"
// OpenSTA swig files
%include "tcl/StaTcl.i"
%include "tcl/NetworkEdit.i"
%include "sdf/Sdf.i"
%include "dcalc/DelayCalc.i"
%include "parasitics/Parasitics.i"
%include "power/Power.i"

namespace std {
  %template(dbNetVector) vector<dbNet*>;
}

%inline %{

sta::Sta *
make_block_sta(odb::dbBlock *block)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  return sta::makeBlockSta(openroad, block);
}

// For testing
void
find_logic_constants()
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->findLogicConstants();
}

void
highlight_path_cmd(PathRef *path)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->highlight(path);
}

std::vector<odb::dbNet*>
find_all_clk_nets()
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  auto clks = sta->findClkNets();
  return std::vector<odb::dbNet*>(clks.begin(), clks.end());
}

odb::dbInst *
sta_to_db_inst(Instance *inst)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  dbInst *db_inst;
  dbModInst* mod_inst;
  db_network->staToDb(inst, db_inst, mod_inst);
  if (db_inst) {
    return db_inst;
  }
  return nullptr;
}

odb::dbMTerm *
sta_to_db_mterm(LibertyPort *port)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  return db_network->staToDb(port);
}

odb::dbBTerm *
sta_to_db_port(Port *port)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  Pin *pin = db_network->findPin(db_network->topInstance(), port);
  dbITerm *iterm;
  dbBTerm *bterm;
  db_network->staToDb(pin, iterm, bterm);
  return bterm;
}

odb::dbITerm *
sta_to_db_pin(Pin *pin)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  dbITerm *iterm;
  dbBTerm *bterm;
  db_network->staToDb(pin, iterm, bterm);
  return iterm;
}

odb::dbNet *
sta_to_db_net(Net *net)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  return db_network->staToDb(net);
}

odb::dbMaster *
sta_to_db_master(LibertyCell *cell)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  return db_network->staToDb(cell);
}

void
db_network_defined()
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbDatabase *db = openroad->getDb();
  odb::dbChip *chip = db->getChip();
  odb::dbBlock *block = chip->getBlock();
  db_network->readDefAfter(block);
}

%} // inline
