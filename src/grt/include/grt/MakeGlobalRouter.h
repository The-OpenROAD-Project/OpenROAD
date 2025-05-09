// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace odb {
class dbDatabase;
}

namespace sta {
class dbSta;
}

namespace rsz {
class Resizer;
}

namespace ant {
class AntennaChecker;
}

namespace dpl {
class Opendp;
}

namespace stt {
class SteinerTreeBuilder;
}

namespace utl {
class Logger;
}

namespace grt {

class GlobalRouter;

grt::GlobalRouter* makeGlobalRouter();

void initGlobalRouter(grt::GlobalRouter* grt,
                      odb::dbDatabase* db,
                      sta::dbSta* sta,
                      rsz::Resizer* resizer,
                      ant::AntennaChecker* antenna_checker,
                      dpl::Opendp* dpl,
                      stt::SteinerTreeBuilder* stt_builder,
                      utl::Logger* logger,
                      Tcl_Interp* tcl_interp);

void deleteGlobalRouter(grt::GlobalRouter* global_router);

}  // namespace grt
