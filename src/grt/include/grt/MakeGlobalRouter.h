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
class CallBackHandler;
}  // namespace utl

namespace grt {

class GlobalRouter;

grt::GlobalRouter* makeGlobalRouter();

void initGlobalRouter(grt::GlobalRouter* grt,
                      odb::dbDatabase* db,
                      sta::dbSta* sta,
                      ant::AntennaChecker* antenna_checker,
                      dpl::Opendp* dpl,
                      stt::SteinerTreeBuilder* stt_builder,
                      utl::Logger* logger,
                      utl::CallBackHandler* callback_handler,
                      Tcl_Interp* tcl_interp);

void deleteGlobalRouter(grt::GlobalRouter* global_router);

}  // namespace grt
