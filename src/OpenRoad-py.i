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

const bool
openroad_gpu_compiled();

const bool 
openroad_python_compiled();

const bool
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

%template(Corners) std::vector<sta::Corner*>;
%template(MTerms) std::vector<odb::dbMTerm*>;
%template(Masters) std::vector<odb::dbMaster*>;

%include "Exception-py.i"
%include "ord/Tech.h"
%include "ord/Design.h"
%include "ord/Timing.h"

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
