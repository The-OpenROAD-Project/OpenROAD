// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

%include <std_string.i>
%include <std_vector.i>
%include <stdint.i>

%{

#include "odb/db.h"
#include "ord/Tech.h"
#include "ord/Design.h"
#include "ord/Timing.h"
#include "ifp/InitFloorplan.hh"

using odb::dbDatabase;
using odb::dbBlock;
using odb::dbTech;

// Defined by OpenRoad.i inlines
const char *
openroad_version();

const char *
openroad_git_describe();

bool
openroad_gpu_compiled();

bool 
openroad_python_compiled();

bool
openroad_gui_compiled();

odb::dbDatabase *
get_db();

odb::dbTech *
get_db_tech();

bool
db_has_tech();

odb::dbBlock *
get_db_block();

%}

%template(Corners) std::vector<sta::Scene*>;
%template(MTerms) std::vector<odb::dbMTerm*>;
%template(Masters) std::vector<odb::dbMaster*>;

%include "Exception-py.i"
%include "ord/Tech.h"
%include "ord/Design.h"
%include "ord/Timing.h"

#ifdef BAZEL
%include "src/gpl/src/replace-py.i"
%include "src/ifp/src/InitFloorplan-py.i"
%include "src/ant/src/AntennaChecker-py.i"
%include "src/cts/src/TritonCTS-py.i"
%include "src/dpl/src/Opendp-py.i"
%include "src/drt/src/TritonRoute-py.i"
%include "src/exa/src/example-py.i"
%include "src/fin/src/finale-py.i"
%include "src/grt/src/GlobalRouter-py.i"
%include "src/par/src/partitionmgr-py.i"
%include "src/pdn/src/PdnGen-py.i"
%include "src/ppl/src/IOPlacer-py.i"
%include "src/psm/src/pdnsim-py.i"
%include "src/rcx/src/ext-py.i"
%include "src/stt/src/SteinerTreeBuilder-py.i"
%include "src/tap/src/tapcell-py.i"
%import "src/odb/src/swig/common/odb.i"
%import "src/utl/src/Logger-py.i"
#endif

%newobject Design::getFloorplan();

const char *
openroad_version();

const char *
openroad_git_describe();

odb::dbDatabase *
get_db();

odb::dbTech *
get_db_tech();

bool
db_has_tech();

odb::dbBlock *
get_db_block();

%inline %{

namespace ord {
  void set_thread_count(int threads);
  int thread_count();
}

%}
