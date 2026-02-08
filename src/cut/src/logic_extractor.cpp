// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "cut/logic_extractor.h"

#include <memory>
#include <set>
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
#include "utl/Logger.h"

namespace cut {

bool SearchPredNonReg2LibrarySupport::searchThru(sta::Edge* edge)
{
  sta::Vertex* v = edge->from(graph_);
  sta::Network* network = sta_->network();

  sta::Instance* inst = network->instance(v->pin());
  if (!inst) {
    return false;
  }

  sta::LibertyCell* cell = network->libertyCell(inst);
  if (!cell) {
    return false;
  }

  if (!supported_.contains(cell->name())) {
    return false;
  }

  return sta::SearchPredNonReg2::searchThru(edge);
}

LogicExtractorFactory& LogicExtractorFactory::AppendEndpoint(
    sta::Vertex* vertex)
{
  endpoints_.push_back(vertex);
  return *this;
}

std::vector<sta::Vertex*> LogicExtractorFactory::GetCutVertices(
    const std::set<std::string>& supported_cells)
{
  cut::SearchPredNonReg2LibrarySupport pred(
      open_sta_, supported_cells, open_sta_->graph());
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
  std::set<const sta::Pin*> pins;
  for (sta::Vertex* vertex : cut_vertices) {
    pins.insert(vertex->pin());
  }

  std::set<sta::Vertex*> endpoint_vertex;
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

std::vector<sta::Pin*> LogicExtractorFactory::GetPrimaryOutputs(
    std::vector<sta::Vertex*>& cut_vertices)
{
  sta::dbNetwork* network = open_sta_->getDbNetwork();

  std::set<const sta::Pin*> cut_set_vertices;
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
    std::vector<sta::Vertex*>& cut_vertices,
    const std::set<std::string>& supported_cells)
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
    sta::LibertyCell* cell = network->libertyCell(endpoint_instance);
    if (!cell || !supported_cells.contains(cell->name())) {
      cut_instances.erase(endpoint_instance);
    }
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

  std::set<sta::Net*> primary_input_nets;
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

}  // namespace cut
