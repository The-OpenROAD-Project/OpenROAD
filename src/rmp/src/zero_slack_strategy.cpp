// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "zero_slack_strategy.h"

#include <vector>

#include "base/abc/abc.h"
#include "cut/abc_library_factory.h"
#include "cut/logic_cut.h"
#include "cut/logic_extractor.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "delay_optimization_strategy.h"
#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Search.hh"
#include "utils.h"
#include "utl/Logger.h"
#include "utl/deleter.h"
#include "utl/unique_name.h"

namespace rmp {

void ZeroSlackStrategy::OptimizeDesign(sta::dbSta* sta,
                                       utl::UniqueName& name_generator,
                                       rsz::Resizer* resizer,
                                       utl::Logger* logger)
{
  sta->ensureGraph();
  sta->ensureLevelized();
  sta->searchPreamble();
  sta->ensureClkNetwork();

  sta::dbNetwork* network = sta->getDbNetwork();

  std::vector<sta::Vertex*> candidate_vertices
      = GetEndpoints(sta, resizer, 0.0);

  if (candidate_vertices.empty()) {
    logger->info(utl::RMP,
                 50,
                 "All candidate endpoints have positive slack, nothing to do.");
    return;
  }

  cut::AbcLibraryFactory factory(logger);
  factory.AddDbSta(sta);
  factory.AddResizer(resizer);
  factory.SetCorner(corner_);
  cut::AbcLibrary abc_library = factory.Build();

  // Disable incremental timing.
  sta->graphDelayCalc()->delaysInvalid();
  sta->search()->arrivalsInvalid();
  sta->search()->endpointsInvalid();

  cut::LogicExtractorFactory logic_extractor(sta, logger);
  for (sta::Vertex* negative_endpoint : candidate_vertices) {
    logic_extractor.AppendEndpoint(negative_endpoint);
  }

  cut::LogicCut cut = logic_extractor.BuildLogicCut(abc_library);

  if (cut.IsEmpty()) {
    logger->warn(
        utl::RMP, 1032, "Logic cut is empty after extraction, nothing to do.");
    return;
  }

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> mapped_abc_network
      = cut.BuildMappedAbcNetwork(abc_library, network, logger);

  DelayOptimizationStrategy strategy;
  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> remapped
      = strategy.Optimize(mapped_abc_network.get(), abc_library, logger);

  cut.InsertMappedAbcNetwork(
      remapped.get(), abc_library, network, name_generator, logger);
}
}  // namespace rmp
