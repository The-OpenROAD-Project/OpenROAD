// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <unordered_set>
#include <vector>

#include "cut/abc_library_factory.h"
#include "cut/logic_cut.h"
#include "db_sta/dbSta.hh"
#include "sta/Graph.hh"
#include "sta/NetworkClass.hh"
#include "sta/SearchPred.hh"
#include "utl/Logger.h"

namespace sta {
class Mode;
}

namespace cut {

// This class is an implementation of an sta::SearchPred that constrains
// the edges BfsBwkIterators are allowed to traverse. In this case this
// particular class will not allow the iterator to walk through DFFs, latches
// or any cell that is not supported by ABC. This allows our logic extractor
// to only extract cells that we could reasonably put in ABC.
class SearchPredCombAbcSupport : public sta::SearchPred1
{
 public:
  // supported_liberty_cells is a set of cells that are supported by ABC.
  SearchPredCombAbcSupport(sta::dbSta* open_sta,
                           AbcLibrary* abc_library,
                           sta::Graph* graph)
      : sta::SearchPred1(open_sta), abc_library_(abc_library), graph_(graph)
  {
  }
  bool searchThru(sta::Edge* edge, const sta::Mode* mode) const override;

 private:
  AbcLibrary* abc_library_;
  sta::Graph* graph_;
};

class LogicExtractorFactory
{
 public:
  LogicExtractorFactory(sta::dbSta* open_sta, utl::Logger* logger)
      : open_sta_(open_sta), logger_(logger)
  {
  }
  LogicExtractorFactory& AppendEndpoint(sta::Vertex* vertex);
  LogicCut BuildLogicCut(AbcLibrary& abc_network);

 private:
  // Process vertices from BFS STA output to find the primary inputs.
  std::vector<sta::Pin*> GetPrimaryInputs(
      std::vector<sta::Vertex*>& cut_vertices);
  std::vector<sta::Pin*> GetPrimaryOutputs(
      std::vector<sta::Vertex*>& cut_vertices);
  std::vector<sta::Vertex*> GetCutVertices(AbcLibrary& abc_network);
  sta::InstanceSet GetCutInstances(std::vector<sta::Vertex*>& cut_vertices);
  std::vector<sta::Pin*> FilterUndrivenOutputs(
      std::vector<sta::Pin*>& primary_outputs,
      sta::InstanceSet& cut_instances);
  std::vector<sta::Net*> ConvertIoPinsToNets(
      std::vector<sta::Pin*>& primary_io_pins);
  std::vector<sta::Vertex*> AddMissingVertices(
      std::vector<sta::Vertex*>& cut_vertices,
      AbcLibrary& abc_library);

  std::vector<sta::Vertex*> endpoints_;
  sta::dbSta* open_sta_;
  utl::Logger* logger_;
};
}  // namespace cut
