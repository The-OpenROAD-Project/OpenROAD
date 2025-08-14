// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

#include "odb/db.h"
#include "utl/Logger.h"

namespace sta {
class dbSta;
}

namespace stt {
class SteinerTreeBuilder;
}

namespace grt {
class GlobalRouter;
}

namespace utl {
class CallBackHandler;
}

namespace est {

class EstimateParasitics;

est::EstimateParasitics* makeEstimateParasitics();

void deleteEstimateParasitics(est::EstimateParasitics* estimate_parasitics);

void initEstimateParasitics(est::EstimateParasitics* estimate_parasitics,
                            Tcl_Interp* tcl_interp,
                            utl::Logger* logger,
                            utl::CallBackHandler* callback_handler,
                            odb::dbDatabase* db,
                            sta::dbSta* sta,
                            stt::SteinerTreeBuilder* stt_builder,
                            grt::GlobalRouter* global_router);

}  // namespace est
