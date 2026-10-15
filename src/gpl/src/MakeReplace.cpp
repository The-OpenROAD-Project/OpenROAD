// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "gpl/MakeReplace.h"

#include <boost/json.hpp>
#include <string>

#include "gpl/Replace.h"
#include "graphicsImpl.h"
#include "graphicsNone.h"
#include "tcl.h"
#include "utl/Logger.h"
#include "utl/OptionsRegistry.h"
#include "utl/decode.h"
#include "web/core.h"

extern "C" {
extern int Gpl_Init(Tcl_Interp* interp);
}

namespace gpl {

extern const char* gpl_tcl_inits[];

// The older switch names of global_placement, kept as aliases of the
// switches generated from PlaceOptions.
static utl::ArgAliases globalPlacementAliases()
{
  using boost::json::object;
  return {
      {"-timing_driven", {"timing.enabled"}, true, "true", {}},
      {"-timing_driven_repair_timing",
       {"timing.repair_timing"},
       true,
       "true",
       {}},
      {"-routability_driven", {"routability.enabled"}, true, "true", {}},
      {"-virtual_cts", {"virtual_cts.enabled"}, true, "true", {}},
      {"-routability_use_grt", {"routability.use_rudy"}, true, "false", {}},
      {"-disable_revert_if_diverge",
       {"nesterov.revert_if_diverge"},
       true,
       "false",
       {}},
      {"-disable_pin_density_adjust",
       {"pin_density_adjust"},
       true,
       "false",
       {}},
      {"-enable_routing_congestion", {"routing_congestion"}, true, "true", {}},
      {"-force_center_initial_place",
       {"initial.force_center"},
       true,
       "true",
       {}},
      {"-skip_initial_place", {"initial.max_iter"}, true, "0", {}},
      {"-initial_place_max_iter", {"initial.max_iter"}, false, "", {}},
      {"-initial_place_max_fanout", {"initial.max_fanout"}, false, "", {}},
      {"-random_seed", {"initial.perturbation_seed"}, false, "", {}},
      {"-perturb_dist", {"initial.perturbation_dist"}, false, "", {}},
      {"-overflow", {"nesterov.target_overflow"}, false, "", {}},
      {"-min_phi_coef", {"nesterov.min_phi_coef"}, false, "", {}},
      {"-max_phi_coef", {"nesterov.max_phi_coef"}, false, "", {}},
      {"-init_density_penalty",
       {"nesterov.init_density_penalty"},
       false,
       "",
       {}},
      {"-init_wirelength_coef",
       {"nesterov.init_wirelength_coef"},
       false,
       "",
       {}},
      {"-reference_hpwl", {"nesterov.reference_hpwl"}, false, "", {}},
      {"-routability_check_overflow",
       {"routability.check_overflow"},
       false,
       "",
       {}},
      {"-routability_snapshot_overflow",
       {"routability.snapshot_overflow"},
       false,
       "",
       {}},
      {"-routability_max_density", {"routability.max_density"}, false, "", {}},
      {"-routability_target_rc_metric",
       {"routability.target_rc_metric"},
       false,
       "",
       {}},
      {"-routability_inflation_ratio_coef",
       {"routability.inflation_ratio_coef"},
       false,
       "",
       {}},
      {"-routability_max_inflation_ratio",
       {"routability.max_inflation_ratio"},
       false,
       "",
       {}},
      {"-routability_min_congestion_for_inflation",
       {"routability.min_congestion_for_inflation"},
       false,
       "",
       {}},
      {"-routability_max_inflation_total",
       {"routability.max_inflation_total"},
       false,
       "",
       {}},
      {"-routability_net_weight_max",
       {"routability.net_weight_max"},
       false,
       "",
       {}},
      {"-routability_congested_nets_percentage",
       {"routability.congested_nets_percentage"},
       false,
       "",
       {}},
      {"-routability_rc_coefficients",
       {"routability.rc_coefficients"},
       false,
       "",
       {}},
      {"-timing_driven_net_reweight_overflow",
       {"timing.reweight_overflows"},
       false,
       "",
       {}},
      {"-timing_driven_net_weight_max",
       {"timing.net_weight_max"},
       false,
       "",
       {}},
      {"-timing_driven_nets_percentage",
       {"timing.nets_percentage"},
       false,
       "",
       {}},
      {"-timing_driven_repair_tns_end_percent",
       {"timing.repair_tns_end_percent"},
       false,
       "",
       {}},
      {"-keep_resize_below_overflow",
       {"timing.keep_resize_below_overflow"},
       false,
       "",
       {}},
      {"-virtual_cts_max_skew_fraction",
       {"virtual_cts.max_skew_fraction"},
       false,
       "",
       {}},
      {"-bin_grid_count",
       {"bin_grid_count_x", "bin_grid_count_y"},
       false,
       "",
       {}},
      // -density uniform is the uniform_density flag.
      {"-density",
       {},
       false,
       "",
       [](object& o, const std::string& value) {
         if (value == "uniform") {
           o["uniform_density"] = true;
         } else {
           o["density"] = value;
         }
       }},
      // PlaceOptions::skipIo(): last, so that it overrides the modes.
      {"-skip_io",
       {},
       true,
       "",
       [](object& o, const std::string&) {
         o["skip_io"] = true;
         o["initial.max_iter"] = 0;
         o["timing.enabled"] = false;
         o["routability.enabled"] = false;
       }},
  };
}

void initReplace(Tcl_Interp* tcl_interp)
{
  // Registered first: the Tcl file builds its help from the schema.
  utl::OptionsRegistry::get().add(
      "global_placement", PlaceOptions::schema(), globalPlacementAliases());
  Gpl_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, gpl::gpl_tcl_inits);
}

void initReplaceGraphics(Replace* replace, utl::Logger* log)
{
  if (web::Gui::get() == nullptr) {
    replace->setGraphicsInterface(gpl::GraphicsNone());
  } else {
    replace->setGraphicsInterface(gpl::GraphicsImpl(log));
  }
}

}  // namespace gpl
