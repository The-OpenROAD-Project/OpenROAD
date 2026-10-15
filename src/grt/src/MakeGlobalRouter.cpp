// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "grt/MakeGlobalRouter.h"

#include <boost/json.hpp>
#include <memory>
#include <string>
#include <utility>

#include "AbstractRoutingCongestionDataSource.h"
#include "FastRoute.h"
#include "grt/GlobalRouter.h"
#include "heatMap.h"
#include "heatMapRudy.h"
#include "tcl.h"
#include "utl/OptionsRegistry.h"
#include "utl/decode.h"
#include "web/heatMap.h"

extern "C" {
extern int Grt_Init(Tcl_Interp* interp);
}

namespace grt {

// Tcl files encoded into strings.
extern const char* grt_tcl_inits[];

// Adapts a web::HeatMapSourceHandle to the gui-free abstract interface
// consumed by GlobalRouter. Defined in this TU so :grt does not see gui.
class HeatMapHandleAdapter : public AbstractRoutingCongestionDataSource
{
 public:
  explicit HeatMapHandleAdapter(web::HeatMapSourceHandle handle)
      : handle_(std::move(handle))
  {
  }

  void invalidate() override
  {
    if (handle_) {
      handle_->invalidateInstances();
    }
  }

 private:
  web::HeatMapSourceHandle handle_;
};

void initGui(grt::GlobalRouter* grt, odb::dbDatabase* db, utl::Logger* logger)
{
  grt->initGui(std::make_unique<HeatMapHandleAdapter>(
                   grt::registerRoutingCongestionHeatMapSource(logger, db)),
               std::make_unique<HeatMapHandleAdapter>(
                   grt::registerRudyHeatMapSource(logger, grt, db)));
}

void initTcl(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Grt_Init(tcl_interp);
  // The older switch names of global_route, as aliases of the switches
  // generated from GlobalRouteOptions.  Registered first: the Tcl file
  // builds its help from the schema.
  utl::OptionsRegistry::get().add(
      "global_route",
      GlobalRouteOptions::schema(),
      {
          // CUGR's rip-up and reroute loop saturates around iteration 5,
          // so -use_cugr lowers the default unless iterations were given.
          {"-use_cugr",
           {},
           true,
           "",
           [](boost::json::object& o, const std::string&) {
             o["use_cugr"] = true;
             if (!o.contains("congestion_iterations")) {
               o["congestion_iterations"] = 5;
             }
           }},
          {"-infinite_cap", {"infinite_capacity"}, true, "true", {}},
      });
  utl::evalTclInit(tcl_interp, grt::grt_tcl_inits);
}

}  // namespace grt
