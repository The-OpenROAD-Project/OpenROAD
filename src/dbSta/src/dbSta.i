%module dbsta

%{

#include "opendb/db.h"
#include "db_sta/dbSta.hh"

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

%} // inline
