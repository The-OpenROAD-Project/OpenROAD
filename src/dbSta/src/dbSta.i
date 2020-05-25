%module dbsta

%{

#include "opendb/db.h"
#include "db_sta/dbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "openroad/OpenRoad.hh"

namespace ord {
// Defined in OpenRoad.i
OpenRoad *getOpenRoad();
}

using sta::Instance;

%}

%include "../../src/Exception.i"
// OpenSTA swig files
%include "tcl/StaTcl.i"
%include "tcl/NetworkEdit.i"
%include "sdf/Sdf.i"
%include "dcalc/DelayCalc.i"
%include "parasitics/Parasitics.i"

%inline %{

sta::Sta *
make_block_sta(odb::dbBlock *block)
{
  return sta::makeBlockSta(block);
}

// For debugging because I can't get a dbNet vector thru swig.
void
report_clk_nets()
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  std::set<dbNet*> clk_nets;
  sta->findClkNets(clk_nets);
  for (dbNet *net : clk_nets)
    printf("%s\n", net->getConstName());
}

odb::dbInst *
sta_to_db_inst(Instance *inst)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  return db_network->staToDb(inst);
}

odb::dbMaster *
sta_to_db_master(LibertyCell *cell)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  return db_network->staToDb(cell);
}

%} // inline
