// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <type_traits>
#include <set>
#include <vector>

#include "cut/abc_library_factory.h"
#include "cut/logic_cut.h"
#include "db_sta/dbSta.hh"
#include "mockturtle/utils/tech_library.hpp"
#include "sta/Bfs.hh"
#include "sta/Graph.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/SearchPred.hh"
#include "utl/Logger.h"

namespace cut {

template <typename T>
struct is_mockturtle_library : std::false_type
{
};

template <unsigned NInputs, mockturtle::classification_type Config>
struct is_mockturtle_library<mockturtle::tech_library<NInputs, Config>>
    : std::true_type
{
};

template <typename T>
inline constexpr bool is_mockturtle_library_v
    = is_mockturtle_library<std::remove_cvref_t<T>>::value;

template <typename T>
concept Library = std::same_as<std::remove_cvref_t<T>, AbcLibrary>
                  || is_mockturtle_library_v<T>;

// This class is an implementation of an sta::SearchPred that constrains
// the edges BfsBwkIterators are allowed to traverse. In this case this
// particular class will not allow the iterator to walk through DFFs, latches
// or any cell that is not supported by ABC. This allows our logic extractor
// to only extract cells that we could reasonably put in ABC.
class SearchPredNonReg2LibrarySupport : public sta::SearchPredNonReg2
{
 public:
  // supported_liberty_cells is a set of cells that are supported by library.
  SearchPredNonReg2LibrarySupport(sta::dbSta* open_sta,
                                  const std::set<std::string>& supported_cells,
                                  sta::Graph* graph)
      : sta::SearchPredNonReg2(open_sta),
        graph_(graph),
        supported_(supported_cells)
  {
  }

  bool searchThru(sta::Edge* edge) override;

 private:
  sta::Graph* graph_;
  const std::set<std::string>& supported_;
};

class LogicExtractorFactory
{
 public:
  LogicExtractorFactory(sta::dbSta* open_sta, utl::Logger* logger)
      : open_sta_(open_sta), logger_(logger)
  {
  }
  LogicExtractorFactory& AppendEndpoint(sta::Vertex* vertex);
  template <Library T>
  LogicCut BuildLogicCut(T& library);

 private:
  // Process vertices from BFS STA output to find the primary inputs.
  std::vector<sta::Pin*> GetPrimaryInputs(
      std::vector<sta::Vertex*>& cut_vertices);
  std::vector<sta::Pin*> GetPrimaryOutputs(
      std::vector<sta::Vertex*>& cut_vertices);
  std::vector<sta::Vertex*> GetCutVertices(
      const std::set<std::string>& supported_cells);
  sta::InstanceSet GetCutInstances(
      std::vector<sta::Vertex*>& cut_vertices,
      const std::set<std::string>& supported_cells);
  std::vector<sta::Pin*> FilterUndrivenOutputs(
      std::vector<sta::Pin*>& primary_outputs,
      sta::InstanceSet& cut_instances);
  std::vector<sta::Net*> ConvertIoPinsToNets(
      std::vector<sta::Pin*>& primary_io_pins);
  template <Library T>
  std::vector<sta::Vertex*> AddMissingVertices(
      std::vector<sta::Vertex*>& cut_vertices,
      T& library);

  std::vector<sta::Vertex*> endpoints_;
  sta::dbSta* open_sta_;
  utl::Logger* logger_;
};

namespace {

// Returns the vertex of a constant 1 or 0 cell if the driver of the input
// pin is a constant 0 or 1 cell. Otherwise return nullptr.
template <Library T>
sta::Vertex* GetConstantVertexIfExists(sta::dbNetwork* network,
                                       sta::Vertex* input_vertex,
                                       T& library,
                                       utl::Logger* logger)
{
  sta::Graph* graph = network->graph();
  // If it's constant add the driving pin. If we have multiple drivers
  // bad things are happening.
  sta::PinSet* constant_driver = network->drivers(input_vertex->pin());
  if (constant_driver->size() != 1) {
    logger->error(
        utl::CUT,
        47,
        "constant vertex: {} should have exactly one constant driver. "
        "Has {}, please report this internal error.",
        input_vertex->name(network),
        constant_driver->size());
  }

  const sta::Pin* constant_pin = *constant_driver->begin();
  sta::Instance* instance = network->instance(constant_pin);
  if (!instance) {
    return nullptr;
  }
  // Okay if the cell we're looking for is actually a const cell add it.
  sta::LibertyCell* liberty_cell = network->libertyCell(instance);

  if constexpr (is_mockturtle_library_v<T>) {
    auto const& gates = library.get_gates();

    auto gate = std::find_if(
        gates.begin(), gates.end(), [&](mockturtle::gate const& g) {
          return g.name == liberty_cell->name();
        });

    if (gate == gates.end()) {
      return nullptr;
    }

    auto const& f = gate->function;

    if (!(kitty::is_const0(f) || kitty::is_const0(~f))) {
      return nullptr;
    }
  } else {
    if (!library.IsConstCell(liberty_cell->name())) {
      return nullptr;
    }
  }

  sta::Vertex* constant_vertex = graph->vertex(network->vertexId(constant_pin));

  return constant_vertex;
}

}  // anonymous namespace

template <Library T>
LogicCut LogicExtractorFactory::BuildLogicCut(T& library)
{
  open_sta_->ensureGraph();
  open_sta_->ensureLevelized();

  std::set<std::string> supported_cells;
  if constexpr (is_mockturtle_library_v<T>) {
    // supported.reserve(library.get_gates().size());
    for (auto const& g : library.get_gates()) {
      supported_cells.insert(g.name);
    }
  } else {
    supported_cells = library.SupportedCells();
  }

  std::vector<sta::Vertex*> cut_vertices = GetCutVertices(supported_cells);
  // Dealing with constant cells 1/0 and disabled timing paths.
  cut_vertices = AddMissingVertices(cut_vertices, library);

  std::vector<sta::Pin*> primary_inputs = GetPrimaryInputs(cut_vertices);
  std::vector<sta::Pin*> primary_outputs = GetPrimaryOutputs(cut_vertices);
  sta::InstanceSet cut_instances
      = GetCutInstances(cut_vertices, supported_cells);

  // Remove primary outputs who are undriven. This can happen when a flop
  // feeds into another flop where the logic cone is essentially just a wire.
  // Just remove them.
  std::vector<sta::Pin*> filtered_primary_outputs
      = FilterUndrivenOutputs(primary_outputs, cut_instances);

  std::vector<sta::Net*> primary_input_nets
      = ConvertIoPinsToNets(primary_inputs);
  std::vector<sta::Net*> primary_output_nets
      = ConvertIoPinsToNets(filtered_primary_outputs);

  return LogicCut(std::move(primary_input_nets),
                  std::move(primary_output_nets),
                  std::move(cut_instances));
}

// Annoyingly STA does not include input pins that are filtered out in the
// search. for example constant inputs, or disabled timing paths. We need to
// loop through all the instances attached to the vertices to make sure we
// grabbed all the input pins.
//
// Some vertices can be const even if they're not constant cells themselves
// for example an AND gate with one of its inputs set to 0. The output will
// be marked as constant, and thus not included.
//
// If OpenSTA offers a search predicate that allows constant cells to be
// included remove the while loop below.
template <Library T>
std::vector<sta::Vertex*> LogicExtractorFactory::AddMissingVertices(
    std::vector<sta::Vertex*>& cut_vertices,
    T& library)
{
  std::vector<sta::Vertex*> result(cut_vertices.begin(), cut_vertices.end());
  sta::dbNetwork* network = open_sta_->getDbNetwork();
  std::set<sta::Vertex*> endpoint_set(endpoints_.begin(),
                                                endpoints_.end());
  std::set<sta::Vertex*> cut_vertex_set(cut_vertices.begin(),
                                                  cut_vertices.end());

  for (sta::Vertex* vertex : cut_vertices) {
    // Skip over DFFs and other primary outs. We don't actually
    // want to add vertices from these cells. We really just
    // want the ones in the fan-in set who are filtered by STA
    // for one reason or another.
    if (endpoint_set.contains(vertex)) {
      continue;
    }

    sta::Instance* instance = network->instance(vertex->pin());
    std::unique_ptr<sta::InstancePinIterator> iter(
        network->pinIterator(instance));
    sta::Graph* graph = network->graph();
    while (iter->hasNext()) {
      sta::Pin* pin = iter->next();
      sta::Vertex* vertex = graph->vertex(network->vertexId(pin));
      // Don't want any duplicate entries
      if (cut_vertex_set.contains(vertex)) {
        continue;
      }
      // Don't want any non-inputs.
      sta::PortDirection* direction = network->direction(pin);
      if (!direction->isInput()) {
        continue;
      }
      // Found a vertex who should be in the cut set, but isn't;
      // add it. The output of this instance is not constant( or is disabled for
      // some reason), but one of its inputs is. We need to add this pin, and
      // possibly its driver.
      result.push_back(vertex);
      cut_vertex_set.insert(vertex);

      // Figure out if we should add the driver.
      if (!vertex->isConstant()) {
        continue;
      }

      sta::Vertex* constant_vertex
          = GetConstantVertexIfExists(network, vertex, library, logger_);
      if (!constant_vertex) {
        continue;
      }

      result.push_back(constant_vertex);
      cut_vertex_set.insert(constant_vertex);
    }
  }
  return result;
}

}  // namespace cut
