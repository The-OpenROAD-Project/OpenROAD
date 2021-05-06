/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
// All rights reserved.
//
// BSD 3-Clause License
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
///////////////////////////////////////////////////////////////////////////////

%{
#include "ord/OpenRoad.hh"
#include "dpl/Opendp.h"
#include "utl/Logger.h"

using dpl::StringSeq;

StringSeq *
tclListSeqString(Tcl_Obj *const source,
                 Tcl_Interp *interp)
{
  int argc;
  Tcl_Obj **argv;

  if (Tcl_ListObjGetElements(interp, source, &argc, &argv) == TCL_OK) {
    StringSeq *seq = new StringSeq;
    for (int i = 0; i < argc; i++) {
      int length;
      const char *str = Tcl_GetStringFromObj(argv[i], &length);
      seq->push_back(str);
    }
    return seq;
  }
  else
    return nullptr;
}

// Failed attempt to pass in dbMaster set.
#if 0
// copied from opensta/tcl/StaTcl.i
template <class TYPE>
std::set<TYPE> *
tclListSet(Tcl_Obj *const source,
           swig_type_info *swig_type,
           Tcl_Interp *interp)
{
  int argc;
  Tcl_Obj **argv;

  if (Tcl_ListObjGetElements(interp, source, &argc, &argv) == TCL_OK
      && argc > 0) {
    std::set<TYPE> *set = new std::set<TYPE>;
    for (int i = 0; i < argc; i++) {
      void *obj;
      // Ignore returned TCL_ERROR because can't get swig_type_info.
      SWIG_ConvertPtr(argv[i], &obj, swig_type, false);
      set->insert(reinterpret_cast<TYPE>(obj));
    }
    return set;
  }
  else
    return nullptr;
}
  
dpl::dbMasterSet *
tclListSetdbMaster(Tcl_Obj *const source,
                   Tcl_Interp *interp)
{
  return tclListSet<odb::dbMaster*>(source, SWIGTYPE_p_dbMaster, interp);
}

%typemap(in) dbMasterSet * {
  $1 = tclListSeqLibertyLibrary($input, interp);
}
#endif

%}

%typemap(in) StringSeq* {
  $1 = tclListSeqString($input, interp);
}

%include "../../Exception.i"

%inline %{

namespace dpl {

void
detailed_placement_cmd(int max_displacment)
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->detailedPlacement(max_displacment);
}

void
report_legalization_stats()
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->reportLegalizationStats();
}

bool
check_placement_cmd(bool verbose)
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  return opendp->checkPlacement(verbose);
}


void
set_padding_global(int left,
                   int right)
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->setPaddingGlobal(left, right);
}

void
set_padding_master(odb::dbMaster *master,
                   int left,
                   int right)
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->setPadding(master, left, right);
}

void
set_padding_inst(odb::dbInst *inst,
                 int left,
                 int right)
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->setPadding(inst, left, right);
}

void
filler_placement_cmd(const char* prefix, 
                     StringSeq *fillers)
{
  ord::OpenRoad *openroad = ord::OpenRoad::openRoad();
  odb::dbDatabase *db = openroad->getDb();
  dpl::Opendp *opendp = openroad->getOpendp();

  vector<dbMaster*> filler_masters;
  for (const string &master_name : *fillers) {
    dbMaster *master = nullptr;
    for (dbLib *lib : db->getLibs()) {
      master = lib->findMaster(master_name.c_str());
      if (master) {
        break;
      }
    }
    if (master)
      filler_masters.push_back(master);
    else
      openroad->getLogger()->warn(utl::DPL, 31, "filler master {} not found.",
                                  master_name);
  }

  opendp->fillerPlacement(filler_masters, prefix);
}

void
optimize_mirroring_cmd()
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->optimizeMirroring();
}

} // namespace

%} // inline
