// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "grt/MakeGlobalRouter.h"

#include <memory>
#include <utility>

#include "AbstractRoutingCongestionDataSource.h"
#include "FastRoute.h"
#include "grt/GlobalRouter.h"
#include "gui/heatMap.h"
#include "heatMap.h"
#include "heatMapRudy.h"
#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Grt_Init(Tcl_Interp* interp);
}

namespace grt {

// Tcl files encoded into strings.
extern const char* grt_tcl_inits[];

// Adapts a gui::HeatMapSourceHandle to the gui-free abstract interface
// consumed by GlobalRouter. Defined in this TU so :grt does not see gui.
class HeatMapHandleAdapter : public AbstractRoutingCongestionDataSource
{
 public:
  explicit HeatMapHandleAdapter(gui::HeatMapSourceHandle handle)
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
  gui::HeatMapSourceHandle handle_;
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
  utl::evalTclInit(tcl_interp, grt::grt_tcl_inits);
}

}  // namespace grt
