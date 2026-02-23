// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "cut/logic_extractor.h"

#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cut/abc_library_factory.h"
#include "cut/logic_cut.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Bfs.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/SearchPred.hh"
#include "sta/TimingRole.hh"
#include "utl/Logger.h"

namespace cut {

bool SearchPredCombAbcSupport::searchThru(sta::Edge* edge,
                                          const sta::Mode* mode) const
{
  const sta::TimingRole* role = edge->role();
  if (role->genericRole() == sta::TimingRole::regClkToQ()
      || role->genericRole() == sta::TimingRole::latchDtoQ()) {
    return false;
  }

  sta::Vertex* to_vertex = edge->from(graph_);
  sta::Network* network = sta_->network();
  sta::Instance* to_instance = network->instance(to_vertex->pin());
  if (to_instance == nullptr) {
    return false;
  }
  sta::LibertyCell* cell = network->libertyCell(to_instance);
  if (cell == nullptr) {
    return false;
  }

  if (!abc_library_->IsSupportedCell(cell->name())) {
    return false;
  }

  return sta::SearchPred1::searchThru(edge, mode);
}

LogicExtractorFactory& LogicExtractorFactory::AppendEndpoint(
    sta::Vertex* vertex)
{
  endpoints_.push_back(vertex);
  return *this;
}

std::vector<sta::Vertex*> LogicExtractorFactory::GetCutVertices(
    AbcLibrary& abc_network)
{
  cut::SearchPredCombAbcSupport pred(
      open_sta_, &abc_network, open_sta_->graph());
  sta::BfsBkwdIterator iter(sta::BfsIndex::other, &pred, open_sta_);
  for (const auto& end_point : endpoints_) {
    iter.enqueue(end_point);
  }

  std::vector<sta::Vertex*> cut_vertices;
  while (iter.hasNext()) {
    sta::Vertex* vertex = iter.next();
    iter.enqueueAdjacentVertices(vertex);
    cut_vertices.push_back(vertex);
  }
  return cut_vertices;
}

std::vector<sta::Pin*> LogicExtractorFactory::GetPrimaryInputs(
    std::vector<sta::Vertex*>& cut_vertices)
{
  sta::dbNetwork* network = open_sta_->getDbNetwork();
  std::unordered_set<const sta::Pin*> pins;
  for (sta::Vertex* vertex : cut_vertices) {
    pins.insert(vertex->pin());
  }

  std::unordered_set<sta::Vertex*> endpoint_vertex;
  for (sta::Vertex* vertex : endpoints_) {
    endpoint_vertex.insert(vertex);
  }

  std::vector<sta::Pin*> primary_inputs;
  for (sta::Vertex* vertex : cut_vertices) {
    // If the pins in the cutset are primary outputs don't call it
    // a primary input
    if (endpoint_vertex.find(vertex) != endpoint_vertex.end()) {
      continue;
    }

    bool is_primary_input = true;
    sta::PinSet* pin_set = network->drivers(vertex->pin());
    if (pin_set->empty()) {
      continue;
    }

    for (const sta::Pin* pin : *pin_set) {
      // If a driver pin of the pin under consideration is not in the cut
      // vertices, then it is a primary input.
      if (pins.find(pin) != pins.end()) {
        is_primary_input = false;
        break;
      }
    }

    if (is_primary_input) {
      primary_inputs.push_back(vertex->pin());
    }
  }

  return primary_inputs;
}

// Returns the vertex of a constant 1 or 0 cell if the driver of the input
// pin is a constant 0 or 1 cell. Otherwise return nullptr.
sta::Vertex* GetConstantVertexIfExists(sta::dbNetwork* network,
                                       sta::Vertex* input_vertex,
                                       AbcLibrary& abc_library,
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
  if (!abc_library.IsConstCell(liberty_cell->name())) {
    return nullptr;
  }
  sta::Vertex* constant_vertex = graph->vertex(network->vertexId(constant_pin));

  return constant_vertex;
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
std::vector<sta::Vertex*> LogicExtractorFactory::AddMissingVertices(
    std::vector<sta::Vertex*>& cut_vertices,
    AbcLibrary& abc_library)
{
  std::vector<sta::Vertex*> result(cut_vertices.begin(), cut_vertices.end());
  sta::dbNetwork* network = open_sta_->getDbNetwork();
  std::unordered_set<sta::Vertex*> endpoint_set(endpoints_.begin(),
                                                endpoints_.end());
  std::unordered_set<sta::Vertex*> cut_vertex_set(cut_vertices.begin(),
                                                  cut_vertices.end());

  for (sta::Vertex* vertex : cut_vertices) {
    // Skip over DFFs and other primary outs. We don't actually
    // want to add vertices from these cells. We really just
    // want the ones in the fan-in set who are filtered by STA
    // for one reason or another.
    if (endpoint_set.find(vertex) != endpoint_set.end()) {
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
      if (cut_vertex_set.find(vertex) != cut_vertex_set.end()) {
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
      if (!open_sta_->isConstant(pin, open_sta_->cmdMode())) {
        continue;
      }

      sta::Vertex* constant_vertex
          = GetConstantVertexIfExists(network, vertex, abc_library, logger_);
      if (!constant_vertex) {
        continue;
      }

      result.push_back(constant_vertex);
      cut_vertex_set.insert(constant_vertex);
    }
  }
  return result;
}

std::vector<sta::Pin*> LogicExtractorFactory::GetPrimaryOutputs(
    std::vector<sta::Vertex*>& cut_vertices)
{
  sta::dbNetwork* network = open_sta_->getDbNetwork();

  std::unordered_set<const sta::Pin*> cut_set_vertices;
  for (sta::Vertex* vertex : cut_vertices) {
    cut_set_vertices.insert(vertex->pin());
  }

  std::vector<sta::Pin*> primary_outputs;
  primary_outputs.reserve(endpoints_.size());
  // Append any and all endpoints to the primary outputs.
  for (sta::Vertex* vertex : endpoints_) {
    primary_outputs.push_back(vertex->pin());
  }

  for (sta::Vertex* vertex : cut_vertices) {
    sta::Pin* pin = vertex->pin();
    sta::PortDirection* direction = network->direction(pin);
    if (!direction->isOutput()) {
      continue;
    }
    auto pin_iterator = std::unique_ptr<sta::PinConnectedPinIterator>(
        network->connectedPinIterator(pin));
    while (pin_iterator->hasNext()) {
      const sta::Pin* connected_pin = pin_iterator->next();
      // Pin is not in our cutset, and therefore is a primary output.
      if (cut_set_vertices.find(connected_pin) == cut_set_vertices.end()) {
        sta::VertexId vertex_id = network->vertexId(connected_pin);
        sta::Vertex* vertex = network->graph()->vertex(vertex_id);
        primary_outputs.push_back(vertex->pin());
      }
    }
  }

  return primary_outputs;
}

sta::InstanceSet LogicExtractorFactory::GetCutInstances(
    std::vector<sta::Vertex*>& cut_vertices)
{
  // Loop through all the verticies in the cut set, and then turn their pins
  // into instances. Verticies are pretty much pins which means for any given
  // instance we should see two pins an input and output. Example:
  //    Cut Vertex: output_flop/D(DFF_X1)
  //    Cut Vertex: _403_/ZN(AND2_X1)
  //    Cut Vertex: _403_/A1(AND2_X1)
  //    Cut Vertex: _403_/A2(AND2_X1)
  //    Primary Input: _403_/A1
  //    Primary Input: _403_/A2
  sta::dbNetwork* network = open_sta_->getDbNetwork();
  sta::InstanceSet cut_instances(network);
  for (sta::Vertex* vertex : cut_vertices) {
    sta::Instance* instance = network->instance(vertex->pin());
    if (instance) {
      cut_instances.insert(instance);
    }
  }

  // We don't want DFFs in the instance themselves since we don't actually
  // want to put those cells in ABC. Remove them.
  for (sta::Vertex* vertex : endpoints_) {
    sta::Instance* endpoint_instance = network->instance(vertex->pin());
    cut_instances.erase(endpoint_instance);
  }

  return cut_instances;
}

std::vector<sta::Pin*> LogicExtractorFactory::FilterUndrivenOutputs(
    std::vector<sta::Pin*>& primary_outputs,
    sta::InstanceSet& cut_instances)
{
  sta::dbNetwork* network = open_sta_->getDbNetwork();
  sta::PinSet filtered_pin_set(network);

  for (sta::Pin* pin : primary_outputs) {
    auto pin_iterator = std::unique_ptr<sta::PinConnectedPinIterator>(
        network->connectedPinIterator(pin));
    while (pin_iterator->hasNext()) {
      const sta::Pin* connected_pin = pin_iterator->next();
      sta::Instance* connected_instance = network->instance(connected_pin);

      if (!network->direction(connected_pin)->isOutput()) {
        continue;
      }

      if (!connected_instance) {
        continue;
      }
      // Output pin is driven by something in the cutset. Keep it.
      if (cut_instances.find(connected_instance) != cut_instances.end()) {
        filtered_pin_set.insert(pin);
      }
    }
  }

  std::vector<sta::Pin*> filtered_output_pins;
  filtered_output_pins.reserve(filtered_pin_set.size());
  for (const sta::Pin* pin : filtered_pin_set) {
    filtered_output_pins.push_back(const_cast<sta::Pin*>(pin));
  }

  return filtered_output_pins;
}

std::vector<sta::Net*> LogicExtractorFactory::ConvertIoPinsToNets(
    std::vector<sta::Pin*>& primary_io_pins)
{
  sta::dbNetwork* network = open_sta_->getDbNetwork();

  std::unordered_set<sta::Net*> primary_input_nets;
  primary_input_nets.reserve(primary_io_pins.size());
  for (sta::Pin* pin : primary_io_pins) {
    sta::Net* net = nullptr;
    // check if this pin is a terminal
    sta::Term* term = network->term(pin);
    if (term) {
      net = network->net(term);
    } else {
      net = network->net(pin);
    }

    if (!net) {
      logger_->error(utl::CUT,
                     48,
                     "primary input pin {} connected to null net",
                     network->name(pin));
    }

    primary_input_nets.insert(net);
  }

  std::vector<sta::Net*> result;
  result.insert(
      result.end(), primary_input_nets.begin(), primary_input_nets.end());
  return result;
}

LogicCut LogicExtractorFactory::BuildLogicCut(AbcLibrary& abc_network)
{
  open_sta_->ensureGraph();
  open_sta_->ensureLevelized();

  std::vector<sta::Vertex*> cut_vertices = GetCutVertices(abc_network);
  // Dealing with constant cells 1/0 and disabled timing paths.
  cut_vertices = AddMissingVertices(cut_vertices, abc_network);

  std::vector<sta::Pin*> primary_inputs = GetPrimaryInputs(cut_vertices);
  std::vector<sta::Pin*> primary_outputs = GetPrimaryOutputs(cut_vertices);
  sta::InstanceSet cut_instances = GetCutInstances(cut_vertices);

  // Remove primary outputs who are undriven. This can happen when a flop feeds
  // into another flop where the logic cone is essentially just a wire. Just
  // remove them.
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

}  // namespace cut
