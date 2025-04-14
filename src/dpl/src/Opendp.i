// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

%{

#include "ord/OpenRoad.hh"
#include "Graphics.h"
#include "DplObserver.h"
#include "dpl/Opendp.h"
#include "utl/Logger.h"



namespace dpl {

using std::vector;

// Swig vector type in does not seem to work at all.
// (see odb/src/swig/common/polgon.i)
// Copied from opensta/tcl/StaTcl.i
template <class TYPE>
vector<TYPE> *
tclListSeq(Tcl_Obj *const source,
	   swig_type_info *swig_type,
	   Tcl_Interp *interp)
{
  int argc;
  Tcl_Obj **argv;

  if (Tcl_ListObjGetElements(interp, source, &argc, &argv) == TCL_OK
      && argc > 0) {
    vector<TYPE> *seq = new vector<TYPE>;
    for (int i = 0; i < argc; i++) {
      void *obj;
      // Ignore returned TCL_ERROR because can't get swig_type_info.
      SWIG_ConvertPtr(argv[i], &obj, swig_type, false);
      seq->push_back(reinterpret_cast<TYPE>(obj));
    }
    return seq;
  }
  else
    return nullptr;
}
  
dpl::dbMasterSeq *
tclListSeqdbMaster(Tcl_Obj *const source,
                   Tcl_Interp *interp)
{
  return tclListSeq<odb::dbMaster*>(source, SWIGTYPE_p_odb__dbMaster, interp);
}

}

%}

%include "../../Exception.i"

%typemap(in) dpl::dbMasterSeq * {
  $1 = dpl::tclListSeqdbMaster($input, interp);
}

%inline %{

namespace dpl {

void
detailed_placement_cmd(int max_displacment_x,
                       int max_displacment_y,
                       const char* report_file_name){
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->detailedPlacement(max_displacment_x, max_displacment_y, std::string(report_file_name));
}

void
report_legalization_stats()
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->reportLegalizationStats();
}

void
check_placement_cmd(bool verbose, const char* report_file_name)
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->checkPlacement(verbose, std::string(report_file_name));
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
filler_placement_cmd(dpl::dbMasterSeq *filler_masters,
                     const char* prefix,
                     bool verbose)
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->fillerPlacement(filler_masters, prefix, verbose);
}

void
remove_fillers_cmd()
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->removeFillers();
}

void
optimize_mirroring_cmd()
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->optimizeMirroring();
}

void
set_debug_cmd(float min_displacement,
              const odb::dbInst* debug_instance)
{
  dpl::Opendp* opendp = ord::OpenRoad::openRoad()->getOpendp();
  if (dpl::Graphics::guiActive()) {
      std::unique_ptr<DplObserver> graphics = std::make_unique<dpl::Graphics>(
          opendp, min_displacement, debug_instance);
      opendp->setDebug(graphics);
  }
}

} // namespace

%} // inline
