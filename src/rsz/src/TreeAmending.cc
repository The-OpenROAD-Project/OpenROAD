#include "TreeAmending.hh"
#include "BufferedNet.hh"

#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Units.hh"
#include "sta/PathVertex.hh"
#include "sta/Fuzzy.hh"
#include "utl/Logger.h"

#include "RepairSetup.hh"
#include "SteinerTree.hh"

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
using sta::fuzzyLessEqual;

TreeAmending::TreeAmending(Resizer* resizer) : resizer_(resizer)
{
}

TreeAmending::~TreeAmending() = default;

void TreeAmending::init()
{
  logger_ = resizer_->logger_;
  sta_ = resizer_->sta_;
  db_network_ = resizer_->db_network_;
  copyState(sta_);
}

void dumpBranch(std::ostream &f, BufferedNetPtr net, float scale, int level=0)
{
  switch (net->type()) {
  case BufferedNetType::buffer:
    dumpBranch(f, net->ref(), scale, level);
    break;
  case BufferedNetType::wire:
    {
      Point a = net->location(), b = net->ref()->location();
      f << std::string(level, ' ')
          << "<line x1=\"" << (float) a.x() * scale << "\" y1=\"" << (float) a.y() * scale << "\" x2=\""
          << (float) b.x() * scale << "\" y2=\"" << (float) b.y() * scale
          << "\" style=\"stroke: "
          << (net->ref()->type() == BufferedNetType::load ? "gray" : "black") << "; stroke-width: 2;\"/>\n";
      dumpBranch(f, net->ref(), scale, level);
    }
    break;
  case BufferedNetType::junction:
    dumpBranch(f, net->ref(), scale, level+1);
    dumpBranch(f, net->ref2(), scale, level+1);
    break;
  case BufferedNetType::load:
    break;
  }
}

void dumpSvg(std::string fname, BufferedNetPtr net)
{
  std::ofstream f(fname, std::ios::out);
  f << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"" \
        << 0 << " " << 0 << " " << 1000 << " " << 1000 << "\">\n";
  dumpBranch(f, net, 0.001);
  f << "</svg>\n";
  f.close();
}

template <typename F>
void visitBnetLoads(BufferedNetPtr net, F&& f) {
  switch (net->type()) {
    case BufferedNetType::load:
      f(net);
      break;
    case BufferedNetType::wire:
      visitBnetLoads(net->ref(), f);
      break;
    case BufferedNetType::junction:
      visitBnetLoads(net->ref(), f);
      visitBnetLoads(net->ref2(), f);
      break;
    case BufferedNetType::buffer:
      // so far a no-op
      return;
  }
}

void setUpstreams(BufferedNetPtr net)
{
  switch (net->type()) {
    case BufferedNetType::load:
      break;
    case BufferedNetType::wire:
    case BufferedNetType::buffer:
      net->ref()->setUpstream(net.get());
      setUpstreams(net->ref());
      return;
    case BufferedNetType::junction:
      net->ref()->setUpstream(net.get());
      setUpstreams(net->ref());
      net->ref2()->setUpstream(net.get());
      setUpstreams(net->ref2());
      break;
  }
}

void updateCandidate(StaState *sta_, BufferedNet *net)
{
  if (!net->pairing_candidate_ || net->pairing_candidate_->place_ != net) {
    BufferedNetPtr left = net->ref();
    while (left->type() == BufferedNetType::wire)
      left = left->ref();
    BufferedNetPtr right = net->ref2();
    while (right->type() == BufferedNetType::wire)
      right = right->ref();

    left = left->pairing_candidate_;
    right = right->pairing_candidate_;

    BufferedNetPtr chosen = right;
    if (left && (!chosen || left->required(sta_) > chosen->required(sta_)))
      chosen = left;

    for (auto other : net->pending_candidates_) {
      if (!chosen || other->required(sta_) > chosen->required(sta_))
        chosen = other;
    }

    net->pairing_candidate_ = chosen;
  }
}

void updateCandidatesAll(StaState *sta_, BufferedNet *net)
{
  switch (net->type()) {
    case BufferedNetType::buffer:
    case BufferedNetType::load:
      break;
    case BufferedNetType::wire:
      updateCandidatesAll(sta_, net->ref().get());
      return;
    case BufferedNetType::junction:
      updateCandidatesAll(sta_, net->ref().get());
      updateCandidatesAll(sta_, net->ref2().get());
      updateCandidate(sta_, net);
      break;
  }
}

void junctionEndpoints(BufferedNetPtr &j, BufferedNetPtr &left, BufferedNetPtr &right)
{
  left = j->ref(); right = j->ref2();
  while (left->type() == BufferedNetType::wire)
    left = left->ref();
  while (right->type() == BufferedNetType::wire)
    right = right->ref();
}

void sort_pair(int &a, int &b)
{
  if (a > b)
    std::swap(a, b);
}

BufferedNetPtr TreeAmending::refineJunctionPlacement(BufferedNetPtr net)
{
  BufferedNetPtr endpoint = net;
  while (endpoint->type() == BufferedNetType::wire)
    endpoint = endpoint->ref();

  switch (endpoint->type()) {
    case BufferedNetType::junction:
      {
        BufferedNetPtr left, right;

        left = refineJunctionPlacement(endpoint->ref());
        right = refineJunctionPlacement(endpoint->ref2());

        while (left->type() == BufferedNetType::wire)
          left = left->ref();
        while (right->type() == BufferedNetType::wire)
          right = right->ref();

        int x1, x2, x3, y1, y2, y3;
        x1 = left->location().x();
        x2 = right->location().x();
        x3 = net->location().x();
        y1 = left->location().y();
        y2 = right->location().y();
        y3 = net->location().y();

        sort_pair(x1, x2);
        sort_pair(x2, x3);
        sort_pair(x1, x2);
        sort_pair(y1, y2);
        sort_pair(y2, y3);
        sort_pair(y1, y2);

        auto loc = Point(x2, y2);

        if (loc != left->location())
          left = std::make_shared<BufferedNet>(
            BufferedNetType::wire, loc, BufferedNet::null_layer,
            left, corner_, resizer_);
        if (loc != right->location())
          right = std::make_shared<BufferedNet>(
            BufferedNetType::wire, loc, BufferedNet::null_layer,
            right, corner_, resizer_);

        auto junc = std::make_shared<BufferedNet>(
            BufferedNetType::junction,
            loc,
            left,
            right,
            resizer_);

        if (loc != net->location())
          return std::make_shared<BufferedNet>(
            BufferedNetType::wire, net->location(), BufferedNet::null_layer,
            junc, corner_, resizer_);
        else
          return junc;
      }
    case BufferedNetType::load:
      return net;
    default:
      logger_->critical(RSZ, 306, "unhandled BufferedNet type {}", (int) endpoint->type());
  }
}

float TreeAmending::estimatedWireCap(BufferedNet *p1, BufferedNet *p2)
{
  double dx
    = resizer_->dbuToMeters(std::abs(p1->location().x() - p2->location().x()));
  double dy
    = resizer_->dbuToMeters(std::abs(p1->location().y() - p2->location().y())); 

  return dx * resizer_->wireSignalHCapacitance(corner_)
            + dy * resizer_->wireSignalVCapacitance(corner_);
}

BufferedNetPtr TreeAmending::build(BufferedNet *root, float cutoff, float eps, float min_cap)
{
  assert(root->pairing_candidate_ && fuzzyLessEqual(cutoff, root->pairing_candidate_->required(sta_)));

  BufferedNetPtr seed = root->pairing_candidate_;
  BufferedNet *place = seed->place_;

  place->pairing_candidate_ = NULL;

  // walk upwards and pair until we hit root
  while (true) {
    assert(place);

    if (place->type() == BufferedNetType::junction) {
      updateCandidate(sta_, place);

      BufferedNet *estimate_pt = (place->upstream()) ? place->upstream() : place;
      float cap_estimated = seed->cap() + estimatedWireCap(estimate_pt, seed.get());

      // find the required cutoff
      float seed_cutoff = std::max(seed->required(sta_)
              - ((cap_estimated < min_cap) ? 0 : (std::logf(cap_estimated / min_cap) * eps)),
              cutoff);

      BufferedNetPtr pair_seed = place->pairing_candidate_;

      if (pair_seed && fuzzyLessEqual(seed_cutoff, pair_seed->required(sta_))) {
        BufferedNetPtr pair = build(place, seed_cutoff, eps, min_cap);

        float combined_cap = pair->cap() + estimatedWireCap(place, pair.get())
            + (estimatedWireCap(place, seed.get()) + seed->cap())
               * std::expf((pair->required(sta_) - seed->required(sta_)) / eps);
        float combined_delay = pair->requiredDelay();

        // create a junction
        auto new_seed = std::make_shared<BufferedNet>(
          BufferedNetType::junction,
          place->location(),
          std::make_shared<BufferedNet>(
            BufferedNetType::wire, place->location(), BufferedNet::null_layer, seed,
            corner_, resizer_),
          std::make_shared<BufferedNet>(
            BufferedNetType::wire, place->location(), BufferedNet::null_layer, pair,
            corner_, resizer_),
          resizer_);
        new_seed->setRequiredPath(pair->requiredPath());
        new_seed->setRequiredDelay(combined_delay);
        new_seed->setCapacitance(combined_cap);
        new_seed->place_ = place;
        seed = new_seed;
      } else {
        if (place == root)
          break;

        place = place->upstream();
      }
    } else {
      if (place == root)
        break;

      place = place->upstream();
    }
  }

  return seed;
}

BufferedNetPtr TreeAmending::amendedTree(const Pin *drvr_pin, const Corner *corner)
{
  corner_ = corner;

  BufferedNetPtr bnet = resizer_->makeBufferedNetSteiner(drvr_pin, corner_);

  if (!bnet) {
    logger_->report("steiner bad {}", network_->name(drvr_pin));
    return bnet;
  }

  std::list<BufferedNetPtr> loads;
  visitBnetLoads(bnet, [&](BufferedNetPtr load){
    const Pin *load_pin = load->loadPin();
    Vertex *vertex = graph_->pinLoadVertex(load_pin);
    PathRef req_path = sta_->vertexWorstSlackPath(vertex, MinMax::max());
    const DcalcAnalysisPt* dcalc_ap = req_path.isNull()
                                            ? resizer_->tgt_slew_dcalc_ap_
                                            : req_path.dcalcAnalysisPt(sta_);
    load->setCapacitance(resizer_->pinCapacitance(load_pin, dcalc_ap));
    load->setRequiredPath(req_path);
    load->pairing_candidate_ = load;
    load->place_ = load.get();
    loads.push_back(load);
  });

  loads.sort([&](BufferedNetPtr a, BufferedNetPtr b) {
    return a->required(sta_) > b->required(sta_);
  });

  if (loads.size() <= 2)
    return bnet;

  // values for sky130hd
  float eps = 0.072e-9;
  float min_cap = 0.002e-12;

  bool fixup_req = false;
  float mark, req, weight = 0.0f;

  for (auto load : loads) {
    if (weight != 0.0f) {
      weight *= std::expf((load->required(sta_) - req) / eps);
    }

    if (weight < min_cap) {
      fixup_req = true;
      mark = load->required(sta_);
    }

    weight += load->cap();
    req = load->required(sta_);
  }

  float req_cutoff;
  if (fixup_req) {
    float cap_sum = 0;

    for (auto load : loads) {
      if (cap_sum * std::expf((mark - load->required(sta_)) / eps) > min_cap) {
        req_cutoff = mark + std::logf(cap_sum / min_cap) * eps;
        break;
      }
      cap_sum += load->cap();
    }
  }

  // make sure the req_cutoff is no higher than the 25th percentile
  // of required times on the loads
  for (auto load : loads) {
    if (m++ == loads.size() / 4 && (!fixup_req || load->required(sta_) < req_cutoff)) {
      req_cutoff = load->required(sta_);
    }
  }

  for (auto load : loads) {
    if (load->required(sta_) > req_cutoff) {
      load->setRequiredDelay(load->required(sta_) - req_cutoff);
    }
  }

  setUpstreams(bnet);
  updateCandidatesAll(sta_, bnet.get());

  BufferedNet *top_junction = bnet.get();
  while (top_junction->type() == BufferedNetType::wire)
    top_junction = top_junction->ref().get();

  BufferedNetPtr amended = build(top_junction, -sta::INF, eps, min_cap);

  amended = std::make_shared<BufferedNet>(
              BufferedNetType::wire, bnet->location(), BufferedNet::null_layer, amended,
              corner_, resizer_);

  // do two rounds of refinement
  amended = refineJunctionPlacement(amended);
  amended = refineJunctionPlacement(amended);

  // clear the overridden delays due to `req_cutoff`
  for (auto load : loads)
    load->setRequiredDelay(0);

  return amended;
}

}  // namespace rsz
