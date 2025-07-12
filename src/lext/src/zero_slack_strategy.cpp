// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "lext/zero_slack_strategy.h"

#include <vector>

#include "lext/abc_library_factory.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "lext/delay_optimization_strategy.h"
#include "lext/logic_cut.h"
#include "lext/logic_extractor.h"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/PortDirection.hh"
#include "sta/Search.hh"
#include "sta/StaMain.hh"
#include "sta/Units.hh"
#include "sta/VerilogWriter.hh"
#include "lext/unique_name.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace lext {

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

void ZeroSlackStrategy::OptimizeDesign(sta::dbSta* sta,
                                       UniqueName& name_generator,
                                       utl::Logger* logger)
{
  sta->ensureGraph();
  sta->ensureLevelized();
  sta->searchPreamble();
  sta->ensureClkNetwork();

  sta::dbNetwork* network = sta->getDbNetwork();

  std::vector<sta::Vertex*> candidate_vertices = GetNegativeEndpoints(sta);

  if (candidate_vertices.empty()) {
    logger->info(
        utl::LEXT, 5, "All endpoints have positive slack, nothing to do.");
    return;
  }

  AbcLibraryFactory factory(logger);
  factory.AddDbSta(sta);
  factory.SetCorner(corner_);
  AbcLibrary abc_library = factory.Build();

  // Disable incremental timing.
  sta->graphDelayCalc()->delaysInvalid();
  sta->search()->arrivalsInvalid();
  sta->search()->endpointsInvalid();

  LogicExtractorFactory logic_extractor(sta, logger);
  for (sta::Vertex* negative_endpoint : candidate_vertices) {
    logic_extractor.AppendEndpoint(negative_endpoint);
  }

  LogicCut cut = logic_extractor.BuildLogicCut(abc_library);

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> mapped_abc_network
      = cut.BuildMappedAbcNetwork(abc_library, network, logger);

  DelayOptimizationStrategy strategy;
  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> remapped
      = strategy.Optimize(mapped_abc_network.get(), abc_library, logger);

  cut.InsertMappedAbcNetwork(
      remapped.get(), abc_library, network, name_generator, logger);
}
}  // namespace lext
