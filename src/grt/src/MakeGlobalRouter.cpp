// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "grt/MakeGlobalRouter.h"

#include <memory>

#include "FastRoute.h"
#include "grt/GlobalRouter.h"
#include "heatMap.h"
#include "heatMapRudy.h"
#include "utl/decode.h"

extern "C" {
extern int Grt_Init(Tcl_Interp* interp);
}

namespace grt {

// Tcl files encoded into strings.
extern const char* grt_tcl_inits[];

grt::GlobalRouter* makeGlobalRouter()
{
  return new grt::GlobalRouter();
}

void deleteGlobalRouter(grt::GlobalRouter* global_router)
{
  delete global_router;
}

void initGlobalRouter(grt::GlobalRouter* grt,
                      odb::dbDatabase* db,
                      sta::dbSta* sta,
                      rsz::Resizer* resizer,
                      ant::AntennaChecker* antenna_checker,
                      dpl::Opendp* dpl,
                      stt::SteinerTreeBuilder* stt_builder,
                      utl::Logger* logger,
                      Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Grt_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, grt::grt_tcl_inits);
  grt->init(logger,
            stt_builder,
            db,
            sta,
            resizer,
            antenna_checker,
            dpl,
            std::make_unique<grt::RoutingCongestionDataSource>(logger, db),
            std::make_unique<grt::RUDYDataSource>(logger, grt, db));
}

}  // namespace grt
