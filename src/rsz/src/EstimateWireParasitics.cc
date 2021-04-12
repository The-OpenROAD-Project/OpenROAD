/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "rsz/Resizer.hh"

#include "utl/Logger.h"

#include "sta/Report.hh"
#include "sta/Units.hh"
#include "sta/Corner.hh"
#include "sta/Sdc.hh"
#include "sta/Parasitics.hh"
#include "sta/ArcDelayCalc.hh"

#include "rsz/SteinerTree.hh"

namespace rsz {

using utl::RSZ;

using sta::PinSet;
using sta::NetIterator;
using sta::NetConnectedPinIterator;
using sta::ReducedParasiticType;
using sta::OperatingConditions;

using odb::dbInst;
using odb::dbMasterType;

void
Resizer::ensureWireParasitics()
{
  if (have_estimated_parasitics_) {
    for (const Net *net : parasitics_invalid_)
      estimateWireParasitic(net);
    parasitics_invalid_.clear();
  }
  else
    estimateWireParasitics();
}

void
Resizer::ensureWireParasitic(const Pin *drvr_pin)
{
  const Net *net = network_->net(drvr_pin);
  if (net)
    ensureWireParasitic(drvr_pin, net);
}

void
Resizer::ensureWireParasitic(const Net *net)
{
  if (have_estimated_parasitics_) {
    PinSet *drivers = network_->drivers(net);
    if (drivers && !drivers->empty()) {
      PinSet::Iterator drvr_iter(drivers);
      Pin *drvr_pin = drvr_iter.next();
      // Sufficient to check for parasitic for one corner because
      // they are all made at the same time.
      const Corner *corner = sta_->corners()->findCorner(0);
      const ParasiticAnalysisPt *parasitic_ap = corner->findParasiticAnalysisPt(max_);
      if (parasitics_invalid_.hasKey(net)
          || parasitics_->findPiElmore(drvr_pin,RiseFall::rise(),parasitic_ap) == nullptr)
        estimateWireParasitic(net);
    }
  }
}

void
Resizer::ensureWireParasitic(const Pin *drvr_pin,
                             const Net *net)
{
  // Sufficient to check for parasitic for one corner because
  // they are all made at the same time.
  const Corner *corner = sta_->corners()->findCorner(0);
  const ParasiticAnalysisPt *parasitic_ap = corner->findParasiticAnalysisPt(max_);
  if (have_estimated_parasitics_
      && net
      && (parasitics_invalid_.hasKey(net)
          || parasitics_->findPiElmore(drvr_pin, RiseFall::rise(),
                                       parasitic_ap) == nullptr)) {
    estimateWireParasitic(net);
    parasitics_invalid_.erase(net);
  }
}

void
Resizer::estimateWireParasitics()
{
  if (wire_signal_cap_ > 0.0) {
    sta_->ensureClkNetwork();
    sta_->deleteParasitics();
    // Make separate parasitics for each corner, same for min/max.
    sta_->corners()->makeCornerParasiticAnalysisPts();

    NetIterator *net_iter = network_->netIterator(network_->topInstance());
    while (net_iter->hasNext()) {
      Net *net = net_iter->next();
      estimateWireParasitic(net);
    }
    delete net_iter;
    have_estimated_parasitics_ = true;
    parasitics_invalid_.clear();
  }
}

void
Resizer::estimateWireParasitic(const Net *net)
{
  if (!network_->isPower(net)
      && !network_->isGround(net)) {
    if (isPadNet(net))
      // When an input port drives a pad instance with huge input
      // cap the elmore delay is gigantic. Annotate with zero
      // wire capacitance to prevent wireload model parasitics from being used.
      makePadParasitic(net);
    else
      estimateWireParasiticSteiner(net);
  }
}

bool
Resizer::isPadNet(const Net *net) const
{
  const Pin *pin1, *pin2;
  net2Pins(net, pin1, pin2);
  return pin1 && pin2
    && ((network_->isTopLevelPort(pin1)
         && isPadPin(pin2))
        || (network_->isTopLevelPort(pin2)
            && isPadPin(pin1)));
}

void
Resizer::makePadParasitic(const Net *net)
{
  const Pin *pin1, *pin2;
  net2Pins(net, pin1, pin2);
  for (Corner *corner : *sta_->corners()) {
    const ParasiticAnalysisPt *parasitics_ap = corner->findParasiticAnalysisPt(max_);
    Parasitic *parasitic = sta_->makeParasiticNetwork(net, false, parasitics_ap);
    ParasiticNode *n1 = parasitics_->ensureParasiticNode(parasitic, pin1);
    ParasiticNode *n2 = parasitics_->ensureParasiticNode(parasitic, pin2);

    // Use a small resistor to keep the connectivity intact.
    parasitics_->makeResistor(nullptr, n1, n2, .001, parasitics_ap);

    ReducedParasiticType reduce_to = sta_->arcDelayCalc()->reducedParasiticType();
    const OperatingConditions *op_cond = sdc_->operatingConditions(max_);
    parasitics_->reduceTo(parasitic, net, reduce_to, op_cond,
                          corner, max_, parasitics_ap);
    parasitics_->deleteParasiticNetwork(net, parasitics_ap);
  }
}

void
Resizer::estimateWireParasiticSteiner(const Net *net)
{
  SteinerTree *tree = makeSteinerTree(net, false, db_network_, logger_);
  if (tree) {
    debugPrint(logger_, RSZ, "resizer_parasitics", 1, "estimate wire {}",
               sdc_network_->pathName(net));
    for (Corner *corner : *sta_->corners()) {
      const ParasiticAnalysisPt *parasitics_ap = corner->findParasiticAnalysisPt(max_);
      Parasitic *parasitic = sta_->makeParasiticNetwork(net, false, parasitics_ap);
      bool is_clk = sta_->isClock(net);
      int branch_count = tree->branchCount();
      for (int i = 0; i < branch_count; i++) {
        Point pt1, pt2;
        Pin *pin1, *pin2;
        SteinerPt steiner_pt1, steiner_pt2;
        int wire_length_dbu;
        tree->branch(i,
                     pt1, pin1, steiner_pt1,
                     pt2, pin2, steiner_pt2,
                     wire_length_dbu);
        ParasiticNode *n1 = findParasiticNode(tree, parasitic, net, pin1, steiner_pt1);
        ParasiticNode *n2 = findParasiticNode(tree, parasitic, net, pin2, steiner_pt2);
        if (n1 != n2) {
          if (wire_length_dbu == 0)
            // Use a small resistor to keep the connectivity intact.
            parasitics_->makeResistor(nullptr, n1, n2, 1.0e-3, parasitics_ap);
          else {
            float wire_length = dbuToMeters(wire_length_dbu);
            float wire_cap = wire_length * (is_clk ? wire_clk_cap_ : wire_signal_cap_);
            float wire_res = wire_length * (is_clk ? wire_clk_res_ : wire_signal_res_);
            // Make pi model for the wire.
            debugPrint(logger_, RSZ, "resizer_parasitics", 2,
                       " pi {} l={} c2={} rpi={} c1={} {}",
                       parasitics_->name(n1),
                       units_->distanceUnit()->asString(wire_length),
                       units_->capacitanceUnit()->asString(wire_cap / 2.0),
                       units_->resistanceUnit()->asString(wire_res),
                       units_->capacitanceUnit()->asString(wire_cap / 2.0),
                       parasitics_->name(n2));
            parasitics_->incrCap(n1, wire_cap / 2.0, parasitics_ap);
            parasitics_->makeResistor(nullptr, n1, n2, wire_res, parasitics_ap);
            parasitics_->incrCap(n2, wire_cap / 2.0, parasitics_ap);
          }
        }
      }
      ReducedParasiticType reduce_to = ReducedParasiticType::pi_elmore;
      const OperatingConditions *op_cond = sdc_->operatingConditions(max_);
      parasitics_->reduceTo(parasitic, net, reduce_to, op_cond,
                            corner, max_, parasitics_ap);
      parasitics_->deleteParasiticNetwork(net, parasitics_ap);
    }
    delete tree;
  }
}

void
Resizer::net2Pins(const Net *net,
                  const Pin *&pin1,
                  const Pin *&pin2)  const
{
  pin1 = nullptr;
  pin2 = nullptr;
  NetConnectedPinIterator *pin_iter = network_->connectedPinIterator(net);
  if (pin_iter->hasNext())
    pin1 = pin_iter->next();
  if (pin_iter->hasNext())
    pin2 = pin_iter->next();
  delete pin_iter;
}

bool
Resizer::isPadPin(const Pin *pin) const
{
  Instance *inst = network_->instance(pin);
  return inst
    && !network_->isTopInstance(inst)
    && isPad(inst);
}

bool
Resizer::isPad(const Instance *inst) const
{
  dbInst *db_inst = db_network_->staToDb(inst);
  dbMasterType type = db_inst->getMaster()->getType();
  // Use switch so if new types are added we get a compiler warning.
  switch (type) {
  case dbMasterType::CORE:
  case dbMasterType::CORE_ANTENNACELL:
  case dbMasterType::CORE_FEEDTHRU:
  case dbMasterType::CORE_TIEHIGH:
  case dbMasterType::CORE_TIELOW:
  case dbMasterType::CORE_WELLTAP:
  case dbMasterType::ENDCAP:
  case dbMasterType::ENDCAP_PRE:
  case dbMasterType::ENDCAP_POST:
  case dbMasterType::CORE_SPACER:
  case dbMasterType::BLOCK:
  case dbMasterType::BLOCK_BLACKBOX:
  case dbMasterType::BLOCK_SOFT:
  case dbMasterType::ENDCAP_TOPLEFT:
  case dbMasterType::ENDCAP_TOPRIGHT:
  case dbMasterType::ENDCAP_BOTTOMLEFT:
  case dbMasterType::ENDCAP_BOTTOMRIGHT:
  case dbMasterType::COVER:
  case dbMasterType::RING:
    return false;
  case dbMasterType::COVER_BUMP:
  case dbMasterType::PAD:
  case dbMasterType::PAD_AREAIO:
  case dbMasterType::PAD_INPUT:
  case dbMasterType::PAD_OUTPUT:
  case dbMasterType::PAD_INOUT:
  case dbMasterType::PAD_POWER:
  case dbMasterType::PAD_SPACER:
  case dbMasterType::NONE:
    return true;
  }
  // gcc warniing
  return false;
}

ParasiticNode *
Resizer::findParasiticNode(SteinerTree *tree,
                           Parasitic *parasitic,
                           const Net *net,
                           const Pin *pin,
                           SteinerPt steiner_pt)
{
  if (pin == nullptr)
    // If the steiner pt is on top of a pin, use the pin instead.
    pin = tree->steinerPtAlias(steiner_pt);
  if (pin)
    return parasitics_->ensureParasiticNode(parasitic, pin);
  else 
    return parasitics_->ensureParasiticNode(parasitic, net, steiner_pt);
}

bool
Resizer::hasTopLevelPort(const Net *net)
{
  bool has_top_level_port = false;
  NetConnectedPinIterator *pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    if (network_->isTopLevelPort(pin)) {
      has_top_level_port = true;
      break;
    }
  }
  delete pin_iter;
  return has_top_level_port;
}

void
Resizer::parasiticsInvalid(const Net *net)
{
  if (have_estimated_parasitics_) {
    debugPrint(logger_, RSZ, "resizer_parasitics", 2, "parasitics invalid {}",
               network_->pathName(net));
    parasitics_invalid_.insert(net);
  }
}

void
Resizer::parasiticsInvalid(const dbNet *net)
{
  parasiticsInvalid(db_network_->dbToSta(net));
}

} // namespace
