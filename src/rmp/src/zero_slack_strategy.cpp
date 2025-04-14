// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "zero_slack_strategy.h"

#include <vector>

#include "abc_library_factory.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "delay_optimization_strategy.h"
#include "logic_cut.h"
#include "logic_extractor.h"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/PortDirection.hh"
#include "sta/Search.hh"
#include "sta/Units.hh"
#include "sta/VerilogWriter.hh"
#include "unique_name.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rmp {

std::vector<sta::Vertex*> GetNegativeEndpoints(sta::dbSta* sta)
{
  std::vector<sta::Vertex*> result;

  sta::dbNetwork* network = sta->getDbNetwork();
  for (sta::Vertex* vertex : *sta->endpoints()) {
    sta::PortDirection* direction = network->direction(vertex->pin());
    if (!direction->isInput()) {
      continue;
    }

    const sta::Slack slack = sta->vertexSlack(vertex, sta::MinMax::max());

    if (slack > 0.0) {
      continue;
    }
    result.push_back(vertex);
  }

  return result;
}

void ZeroSlackStrategy::OptimizeDesign(sta::dbSta* sta, utl::Logger* logger)
{
  sta->ensureGraph();
  sta->ensureLevelized();
  sta->searchPreamble();
  sta->ensureClkNetwork();

  sta::dbNetwork* network = sta->getDbNetwork();

  AbcLibraryFactory factory(logger);
  factory.AddDbSta(sta);
  AbcLibrary abc_library = factory.Build();

  std::vector<sta::Vertex*> candidate_vertices = GetNegativeEndpoints(sta);

  // Disable incremental timing.
  sta->graphDelayCalc()->delaysInvalid();
  sta->search()->arrivalsInvalid();
  sta->search()->endpointsInvalid();

  rmp::UniqueName unique_name;
  for (sta::Vertex* negative_endpoint : candidate_vertices) {
    LogicExtractorFactory logic_extractor(sta, logger);
    logic_extractor.AppendEndpoint(negative_endpoint);
    LogicCut cut = logic_extractor.BuildLogicCut(abc_library);

    utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> mapped_abc_network
        = cut.BuildMappedAbcNetwork(abc_library, network, logger);

    DelayOptimizationStrategy strategy(sta);
    utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> remapped
        = strategy.Optimize(mapped_abc_network.get(), abc_library, logger);

    cut.InsertMappedAbcNetwork(
        remapped.get(), abc_library, network, unique_name, logger);
  }
}
}  // namespace rmp
