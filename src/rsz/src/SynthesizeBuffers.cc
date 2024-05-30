#include "SynthesizeBuffers.hh"

#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/DcalcAnalysisPt.hh"
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

bool SynthesizeBuffers::collectLoads(Net *net, Pin *drvr_pin,
                                     std::vector<Pin *> &loads)
{
  NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (pin == drvr_pin)
      continue;
    if (network_->isTopLevelPort(pin))
      continue;
    if (network_->direction(pin) != PortDirection::input()) {
      printf("skipping a non-input pin: instance %s port %s\n",
             network_->name(network_->instance(pin)),
             network_->name(network_->port(pin))); // TODO
      continue;
    }
    if (!network_->libertyPort(pin)) {
      printf("skipping a non-liberty pin: instance %s port %s\n",
             network_->name(network_->instance(pin)),
             network_->name(network_->port(pin))); // TODO
      continue;
    }
    // TODO: is stripping constantness here safe?
    loads.push_back(const_cast<Pin *>(pin));
  }
  delete pin_iter;
  return true;
}

float SynthesizeBuffers::requiredTime(Pin *a)
{
  Vertex *load_vertex = graph_->pinLoadVertex(a);
  return sta_->vertexRequired(load_vertex, MinMax::max());
}

float bufferCin(const LibertyCell *cell)
{
  LibertyPort *a, *y;
  cell->bufferPorts(a, y);
  return a->capacitance();
}

void SynthesizeBuffers::synthesizeBuffers(Net *net, Pin* drvr_pin,
                                          int max_fanout,
                                          float gain)
{
  std::vector<LibertyCell *> buffer_sizes;
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

  float cin;
  std::vector<Pin *> loads;
  if (getCin(drvr_pin, cin) && collectLoads(net, drvr_pin, loads)) {
    std::sort(loads.begin(), loads.end(), [&](Pin *a, Pin *b) {
      return requiredTime(a) > requiredTime(b);
    });

    float load_sum = loadCapacitance(loads);

    while (loads.size() > max_fanout || load_sum > cin * gain) {
      float load_max = bufferCin(buffer_sizes.back()) * gain;
      float load_acc = 0;
      auto it = loads.begin();
      for (; it != loads.end(); it++) {
        if (it - loads.begin() == max_fanout) {
          break;
        }
        float leaf_load = network_->libertyPort(*it)->capacitance();
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
        // we are getting dimishing returns on inserting the buffer, stop
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

      for (auto it = loads.begin(); it != group_end; it++) {
        LibertyPort* load_port = network_->libertyPort(*it);
        Instance* load_inst = network_->instance(*it);
        sta_->disconnectPin(*it);
        sta_->connectPin(load_inst, load_port, new_net);
      }

      loads.erase(loads.begin(), group_end);
      Pin *new_pin = network_->findPin(inst, size_in);
      loads.insert(std::upper_bound(loads.begin(), loads.end(), new_pin,
                                    [&](Pin *a, Pin *b) {
        return requiredTime(a) > requiredTime(b);
      }), new_pin);

      // update load_sum for the next round
      load_sum = loadCapacitance(loads);
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

  std::vector<Pin *> work_queue;

  for (int i = resizer_->level_drvr_vertices_.size() - 1; i >= 0; i--) {
    Vertex* drvr = resizer_->level_drvr_vertices_[i];
    Pin* drvr_pin = drvr->pin();
    Net* net = network_->isTopLevelPort(drvr_pin)
                  ? network_->net(network_->term(drvr_pin))
                  : network_->net(drvr_pin);
    dbNet* net_db = db_network_->staToDb(net);
    if (net != nullptr && !resizer_->dontTouch(net)
        && !net_db->isConnectedByAbutment() && !sta_->isClock(drvr_pin)
        && !drvr->isConstant() && !resizer_->isTristateDriver(drvr_pin)) {
      std::vector<Pin *> loads;
      float cin;
      if (collectLoads(net, drvr->pin(), loads) && getCin(drvr_pin, cin)) {
        if (loads.size() > max_fanout || loadCapacitance(loads) > cin * gain) {
          work_queue.push_back(drvr_pin);
        }
      }
    }
  }

  debugPrint(logger_, RSZ, "synthesize_buffers", 3,
             "{} pins in work queue", work_queue.size());

  for (auto drvr_pin : work_queue) {
    sta_->setAnnotatedSlew(graph_->pinDrvrVertex(drvr_pin), sta_->cmdCorner(),
                           MinMaxAll::all(), RiseFallBoth::riseFall(), slew);
  }
  sta_->delaysInvalid();

  for (auto drvr_pin : work_queue) {
    Net* net = network_->isTopLevelPort(drvr_pin)
                   ? network_->net(network_->term(drvr_pin))
                   : network_->net(drvr_pin);
    synthesizeBuffers(net, drvr_pin, max_fanout, gain);
    clearSlewAnnotation(graph_->pinDrvrVertex(drvr_pin));
    //sta_->delaysInvalid();
  }

  sta_->removeDelaySlewAnnotations();
  sta_->delaysInvalid();
}

}  // namespace rsz
