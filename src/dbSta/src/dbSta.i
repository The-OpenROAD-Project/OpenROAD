%module dbsta

%{

#include "Machine.hh"
#include "opendb/db.h"
#include "db_sta/dbSta.hh"

%}

%include "../../src/Exception.i"
// OpenSTA swig files
%include "StaTcl.i"
%include "NetworkEdit.i"
%include "Sdf.i"
%include "DelayCalc.i"
%include "Parasitics.i"

%inline %{

sta::Sta *
make_block_sta(odb::dbBlock *block)
{
  return sta::makeBlockSta(block);
}

%} // inline
