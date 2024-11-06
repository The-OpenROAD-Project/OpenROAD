#include "logic_extractor.h"

#include <memory>
#include <unordered_set>
#include <vector>

#include "abc_library_factory.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "logic_cut.h"
#include "sta/Bfs.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/SearchPred.hh"

namespace rmp {

bool SearchPredNonReg2AbcSupport::searchThru(sta::Edge* edge)
{
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

  return sta::SearchPredNonReg2::searchThru(edge);
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
  rmp::SearchPredNonReg2AbcSupport pred(
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
    for (const sta::Pin* pin : *network->drivers(vertex->pin())) {
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

std::unordered_set<sta::Instance*> LogicExtractorFactory::GetCutInstances(
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
  std::unordered_set<sta::Instance*> cut_instances;
  sta::dbNetwork* network = open_sta_->getDbNetwork();
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
    std::unordered_set<sta::Instance*>& cut_instances)
{
  sta::dbNetwork* network = open_sta_->getDbNetwork();
  sta::PinSet filtered_pin_set(network);

  for (sta::Pin* pin : primary_outputs) {
    sta::PinSet* pin_iterator = network->drivers(pin);
    for (const sta::Pin* connected_pin : *pin_iterator) {
      sta::Instance* connected_instance = network->instance(connected_pin);
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
    sta::Net* net = network->net(pin);
    if (!net) {
      logger_->error(utl::RMP,
                     1023,
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

void LogicExtractorFactory::RemovePrimaryOutputInstances(
    std::unordered_set<sta::Instance*>& cut_instances,
    std::vector<sta::Pin*>& primary_output_pins)
{
  sta::dbNetwork* network = open_sta_->getDbNetwork();
  for (sta::Pin* pin : primary_output_pins) {
    sta::Instance* instance = network->instance(pin);
    cut_instances.erase(instance);
  }
}

LogicCut LogicExtractorFactory::BuildLogicCut(AbcLibrary& abc_network)
{
  open_sta_->ensureGraph();
  open_sta_->ensureLevelized();

  std::vector<sta::Vertex*> cut_vertices = GetCutVertices(abc_network);
  std::vector<sta::Pin*> primary_inputs = GetPrimaryInputs(cut_vertices);
  std::vector<sta::Pin*> primary_outputs = GetPrimaryOutputs(cut_vertices);
  std::unordered_set<sta::Instance*> cut_instances
      = GetCutInstances(cut_vertices);

  // Remove primary outputs who are undriven. This can happen when a flop feeds
  // into another flop where the logic cone is essentially just a wire. Just
  // remove them.
  std::vector<sta::Pin*> filtered_primary_outputs
      = FilterUndrivenOutputs(primary_outputs, cut_instances);

  std::vector<sta::Net*> primary_input_nets
      = ConvertIoPinsToNets(primary_inputs);
  std::vector<sta::Net*> primary_output_nets
      = ConvertIoPinsToNets(filtered_primary_outputs);

  // Modifies cut_instances in-place
  RemovePrimaryOutputInstances(cut_instances, primary_outputs);

  return LogicCut(primary_input_nets, primary_output_nets, cut_instances);
}

}  // namespace rmp
