#include "SynthesizeBuffers.hh"

#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Units.hh"
#include "sta/PathVertex.hh"
#include "utl/Logger.h"

namespace rsz {

using sta::LibertyCellPortIterator;
using sta::NetConnectedPinIterator;
using sta::PortDirection;
using sta::Port;
using utl::RSZ;
using sta::MinMaxAll;
using sta::RiseFallBoth;
using sta::DcalcAPIndex;
using sta::VertexPathIterator;
using sta::VertexIterator;
using sta::Graph;
using sta::Network;
using sta::stringLess;

SynthesizeBuffers::SynthesizeBuffers(Resizer* resizer) : resizer_(resizer)
{
}

SynthesizeBuffers::~SynthesizeBuffers() = default;

void SynthesizeBuffers::init()
{
  logger_ = resizer_->logger_;
  sta_ = resizer_->sta_;
  db_network_ = resizer_->db_network_;
  copyState(sta_);
}

// TODO: we should compute this up front per each technology cell
bool SynthesizeBuffers::getCin(Pin* drvr_pin, float& cin)
{
  Instance* inst = network_->instance(drvr_pin);
  LibertyCell* cell = network_->libertyCell(inst);

  if (!network_->isTopLevelPort(drvr_pin) && cell != nullptr
      && resizer_->isLogicStdCell(inst)) {
    LibertyCellPortIterator port_iter(cell);
    int nports = 0;
    while (port_iter.hasNext()) {
      const LibertyPort* port = port_iter.next();
      if (port->direction() == PortDirection::input()) {
        cin += port->capacitance();
        nports++;
      }
    }
    if (!nports) {
      return false;
    }
    cin /= nports;
    return true;
  }
  return false;
}

float SynthesizeBuffers::loadCapacitance(std::vector<Pin *> &loads)
{
  float sum = 0;
  for (Pin* load : loads) {
    const LibertyPort* port = network_->libertyPort(load);
    sum += port->capacitance();
  }
  return sum;
}

float SynthesizeBuffers::requiredTime(Pin *a)
{
  Vertex *load_vertex = graph_->pinLoadVertex(a);
  float t = sta_->vertexRequired(load_vertex, MinMax::max());
  //printf("required %s %5.2f\n", network_->name(a), t * 10.0e9);
  return t;
}

float bufferCin(const LibertyCell *cell)
{
  LibertyPort *a, *y;
  cell->bufferPorts(a, y);
  return a->capacitance();
}

struct EnqueuedPin {
  Pin *pin;
  Required required;
  int level;

  static EnqueuedPin create(Graph *graph, const StaState *sta, Pin *pin, int level=0)
  {
    VertexPathIterator path_iter(graph->pinLoadVertex(pin), nullptr,
                                 nullptr, MinMax::max(), sta);
    Required required = MinMax::min()->initValue();
    while (path_iter.hasNext()) {
      Required path_required = path_iter.next()->required(sta);
      if (delayGreater(path_required, required, MinMax::min(), sta)) {
        required = path_required;
      }
    }
    return EnqueuedPin{pin, required, level};
  }

  std::tuple<Required, int> sort_label() const { return std::make_tuple(required, -level); }
};

class PinRequiredHigher
{
private:
  const Network *network_;

public:
  PinRequiredHigher(const Network *network) : network_(network) {};

  bool operator()(const EnqueuedPin &a, const EnqueuedPin &b) const {
    if (a.sort_label() > b.sort_label())
      return true;
    else if (a.sort_label() < b.sort_label())
      return false;
    else
      return stringLess(network_->pathName(a.pin),
                        network_->pathName(b.pin));
  }
};

template <typename F>
void visitLoads(const Network *network, Net *net, Pin *drvr_pin, F&& f) {
  NetConnectedPinIterator* pin_iter = \
    network->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (pin == drvr_pin)
      continue;
    if (network->isTopLevelPort(pin))
      continue;
    if (network->direction(pin) != PortDirection::input()) {
      printf("skipping a non-input pin: instance %s port %s\n",
             network->name(network->instance(pin)),
             network->name(network->port(pin))); // TODO
      continue;
    }
    if (!network->libertyPort(pin)) {
      printf("skipping a non-liberty pin: instance %s port %s\n",
             network->name(network->instance(pin)),
             network->name(network->port(pin))); // TODO
      continue;
    }
    // TODO: is stripping constantness here safe?
    f(const_cast<Pin *>(pin));
  }
  delete pin_iter;
}

void SynthesizeBuffers::synthesizeBuffers(Net *net, Pin* drvr_pin,
                                          int max_fanout,
                                          float gain)
{
  float cin;
  std::vector<EnqueuedPin> loads;
  if (getCin(drvr_pin, cin)) {
    float load_sum = 0;
    visitLoads(network_, net, drvr_pin, [&](Pin *pin) {
      LibertyPort* port = network_->libertyPort(pin);
      load_sum += port->capacitance();
      loads.push_back(EnqueuedPin::create(graph_, this, pin));
    });

    std::sort(loads.begin(), loads.end(), PinRequiredHigher(network_));

    while (loads.size() > max_fanout || load_sum > cin * gain) {
      float load_max = bufferCin(buffer_sizes.back()) * gain;
      float load_acc = 0;
      auto it = loads.begin();
      for (; it != loads.end(); it++) {
        if (it - loads.begin() == max_fanout) {
          break;
        }
        float leaf_load = network_->libertyPort(it->pin)->capacitance();
        if (load_acc + leaf_load > load_max
            // always include at least one load
            && it != loads.begin()) {
          break;
        }
        load_acc += leaf_load;
      }
      auto group_end = it;

      auto size = buffer_sizes.begin();
      for (; size != buffer_sizes.end() - 1; size++) {
        if (bufferCin(*size) > load_acc / gain) {
          break;
        }
      }

      if (bufferCin(*size) >= 0.9f * load_acc) {
        // we are getting dimishing returns on inserting a buffer, stop
        // the algorithm here (we might have been called with a low gain value)
        break;
      }

      Net *new_net = resizer_->makeUniqueNet();
      dbNet* net_db = db_network_->staToDb(net);
      dbNet* new_net_db = db_network_->staToDb(new_net);
      new_net_db->setSigType(net_db->getSigType());

      string buffer_name = resizer_->makeUniqueInstName("synth_buffer");
      const Point drvr_loc = db_network_->location(drvr_pin);
      Instance *inst = resizer_->makeBuffer(*size, buffer_name.c_str(),
                                            // TODO: non-top module handling?
                                            db_network_->topInstance(),
                                            drvr_loc);
      LibertyPort *size_in, *size_out;
      (*size)->bufferPorts(size_in, size_out);
      sta_->connectPin(inst, size_in, net);
      sta_->connectPin(inst, size_out, new_net);

      int max_level = 0;
      for (auto it = loads.begin(); it != group_end; it++) {
        LibertyPort* load_port = network_->libertyPort(it->pin);
        Instance* load_inst = network_->instance(it->pin);
        load_sum -= load_port->capacitance();
        if (it->level > max_level) {
          max_level = it->level;
        }
        sta_->disconnectPin(it->pin);
        sta_->connectPin(load_inst, load_port, new_net);
      }

      Pin *new_input_pin = network_->findPin(inst, size_in);
      Pin *new_output_pin = network_->findPin(inst, size_in);

      graph_delay_calc_->findDelays(graph_->pinDrvrVertex(new_output_pin));
      arrival_visitor->visit(graph_->pinLoadVertex(new_input_pin));
      arrival_visitor->visit(graph_->pinLoadVertex(new_output_pin));
      req_visitor->visit(graph_->pinLoadVertex(new_output_pin));
      req_visitor->visit(graph_->pinLoadVertex(new_input_pin));

      loads.erase(loads.begin(), group_end);
      auto new_pin = EnqueuedPin::create(graph_, this,
                      new_input_pin, max_level + 1);
      loads.insert(std::upper_bound(loads.begin(), loads.end(), new_pin,
                                    PinRequiredHigher(network_)), new_pin);

      // update load_sum for the next round
      load_sum += size_in->capacitance();
    }
  }
}

void SynthesizeBuffers::clearSlewAnnotation(Vertex *vertex)
{
  for (MinMax *minmax : MinMaxAll::all()->range()) {
    const DcalcAnalysisPt *dcalc_ap = sta_->cmdCorner()->findDcalcAnalysisPt(minmax);
    DcalcAPIndex ap_index = dcalc_ap->index();
    for (RiseFall *rf : RiseFallBoth::riseFall()->range()) {
      vertex->setSlewAnnotated(false, rf, ap_index);
    }
  }
  graph_delay_calc_->delayInvalid(vertex);
}

void SynthesizeBuffers::synthesizeBuffers(int max_fanout,
                                          float gain,
                                          float slew)
{
  init();
  resizer_->resizePreamble();

  buffer_sizes.clear();
  {
    LibertyCellSeq* equiv_cells = sta_->equivCells(resizer_->buffer_lowest_drive_);
    assert(equiv_cells);
    for (auto buffer : *equiv_cells) {
      if (!buffer->dontUse() && !resizer_->dontUse(buffer))
        buffer_sizes.push_back(buffer);
    }
    std::sort(buffer_sizes.begin(), buffer_sizes.end(),
              [=](LibertyCell *a, LibertyCell *b) {
      return bufferCin(a) < bufferCin(b);
    });
  }
  assert(buffer_sizes.size() > 1);

  std::vector<Vertex *> vertices;
  VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    Vertex* vertex = vertex_iter.next();
    vertices.emplace_back(vertex);
  }
  std::sort(vertices.begin(), vertices.end(), VertexLevelLess(network_));

  std::vector<bool> work_queue;
  work_queue.resize(vertices.size());
  int nenqueued = 0;

  for (int i = vertices.size() - 1; i >= 0; i--) {
    Vertex* vertex = vertices[i];
    if (vertex->isDriver(network_)) {
      Pin* drvr_pin = vertex->pin();
      Net* net = network_->isTopLevelPort(drvr_pin)
                    ? network_->net(network_->term(drvr_pin))
                    : network_->net(drvr_pin);
      dbNet* net_db = db_network_->staToDb(net);
      if (net != nullptr && !resizer_->dontTouch(net)
          && !net_db->isConnectedByAbutment() && !sta_->isClock(drvr_pin)
          && !vertex->isConstant() && !resizer_->isTristateDriver(drvr_pin)) {
        float cin;
        if (getCin(drvr_pin, cin)) {
          int nloads = 0;
          float load_sum = 0;
          visitLoads(network_, net, drvr_pin, [&](Pin *load_pin) {
            const LibertyPort* port = network_->libertyPort(load_pin);
            load_sum += port->capacitance();
            nloads++;
          });

          if (nloads > max_fanout || load_sum > cin * gain) {
            work_queue[i] = true;
            nenqueued++;
            sta_->setAnnotatedSlew(vertex, sta_->cmdCorner(),
                                   MinMaxAll::all(), RiseFallBoth::riseFall(), slew);
          }
        }
      }
    }
  }

  debugPrint(logger_, RSZ, "synthesize_buffers", 3,
             "{} pins in work queue", nenqueued);

  // start with valid arrivals, requireds, and levels
  // also start with delays that have taken the annotated slew into account
  sta_->findRequireds();

  arrival_visitor = new ArrivalVisitor(this);
  req_visitor = new RequiredVisitor(this);

  for (int i = vertices.size() - 1; i >= 0; i--) {
    Vertex* vertex = vertices[i];

    if (work_queue[i]) {
      Pin* drvr_pin = vertex->pin();
      Net* net = network_->isTopLevelPort(drvr_pin)
                    ? network_->net(network_->term(drvr_pin))
                    : network_->net(drvr_pin);
      synthesizeBuffers(net, drvr_pin, max_fanout, gain);
    }

    // as we go and repair the nets also backpropagate the required times
    req_visitor->visit(vertex);
  }

  delete req_visitor;
  delete arrival_visitor;

  sta_->removeDelaySlewAnnotations();
}

}  // namespace rsz
