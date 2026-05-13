// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#ifdef BAZEL
#include <string>
#include "cut/blif.h"

namespace utl {
class Logger;
}

namespace odb {
class dbBlock;
class dbInst;
}

namespace sta {
class dbSta;
}

using namespace cut;
using odb::dbInst;
#else
#include "rmp/Restructure.h"
#include "cut/blif.h"
#include "sta/Scene.hh"
#include "ord/OpenRoad.hh"
#include "odb/db.h"
#include "sta/Liberty.hh"
#include <string>

namespace ord {
// Defined in OpenRoad.i
rmp::Restructure *
getRestructure();

OpenRoad *
getOpenRoad();
}

using namespace rmp;
using namespace cut;
using ord::getRestructure;
using ord::getOpenRoad;
using odb::dbInst;
using sta::LibertyPort;
using sta::Scene;
#endif
%}

%include "../../Exception-py.i"

%include <typemaps.i>
%include <std_string.i>
#ifdef BAZEL
namespace utl {
class Logger;
}

namespace odb {
class dbBlock;
class dbInst;
}

namespace sta {
class dbSta;
}

namespace cut {
class Blif
{
 public:
  Blif(utl::Logger* logger,
       sta::dbSta* sta,
       const std::string& const0_cell_,
       const std::string& const0_cell_port_,
       const std::string& const1_cell_,
       const std::string& const1_cell_port_,
       int call_id_);
  void addReplaceableInstance(odb::dbInst* inst);
  bool writeBlif(const char* file_name, bool write_arrival_requireds = false);
  bool readBlif(const char* file_name, odb::dbBlock* block);
};
}  // namespace cut
#else
%include "cut/blif.h"
%include "rmp/Restructure.h"
#endif
