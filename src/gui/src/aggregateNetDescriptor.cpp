/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023, The Regents of the University of California
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

#include "aggregateNetDescriptor.h"

#include "colorGenerator.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "sta/Liberty.hh"

namespace gui {

sta::dbSta* AggregateNet::sta_ = nullptr;

AggregateNet::AggregateNet(odb::dbNet* net)
{
  populate(net);

  std::sort(
      nets_.begin(), nets_.end(), [](odb::dbNet* r, odb::dbNet* l) -> bool {
        return r->getName() < l->getName();
      });

  name_ = nets_[0]->getName();
}

void AggregateNet::populate(odb::dbNet* net)
{
  if (net == nullptr) {
    return;
  }

  if (std::find(nets_.begin(), nets_.end(), net) != nets_.end()) {
    return;
  }

  nets_.push_back(net);

  for (auto* iterm : net->getITerms()) {
    auto* inst = iterm->getInst();
    if (!AggregateNet::isAggregate(inst)) {
      continue;
    }

    for (auto* next_iterm : inst->getITerms()) {
      if (!next_iterm->getSigType().isSupply()) {
        populate(next_iterm->getNet());
      }
    }
  }
}

bool AggregateNet::isAggregate(odb::dbNet* net)
{
  if (net == nullptr) {
    return false;
  }

  for (auto* iterm : net->getITerms()) {
    if (AggregateNet::isAggregate(iterm->getInst())) {
      return true;
    }
  }

  return false;
}

bool AggregateNet::isAggregate(odb::dbInst* inst)
{
  if (inst == nullptr) {
    return false;
  }

  auto* network = sta_->getDbNetwork();
  if (inst->getSourceType() != odb::dbSourceType::TIMING) {
    return false;
  }

  auto* master = inst->getMaster();
  auto* cell = network->dbToSta(master);
  if (cell == nullptr) {
    return false;
  }

  auto* lib_cell = network->libertyCell(cell);
  if (lib_cell == nullptr) {
    return false;
  }

  return lib_cell->isBuffer();
}

///////

AggregateNetDescriptor::AggregateNetDescriptor(
    odb::dbDatabase* db,
    sta::dbSta* sta,
    const std::set<odb::dbNet*>& guide_nets)
    : db_(db),
      net_descriptor_(Gui::get()->getDescriptor<odb::dbNet*>()),
      guide_nets_(guide_nets)
{
  AggregateNet::setSTA(sta);
}

std::string AggregateNetDescriptor::getName(std::any object) const
{
  AggregateNet* anet = std::any_cast<AggregateNet>(&object);
  return anet->getName();
}

std::string AggregateNetDescriptor::getTypeName() const
{
  return "Aggregate Net";
}

bool AggregateNetDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  AggregateNet* anet = std::any_cast<AggregateNet>(&object);
  bbox.mergeInit();
  for (auto* net : anet->getNets()) {
    odb::Rect box;
    if (net_descriptor_->getBBox(net, box)) {
      bbox.merge(box);
    }
  }
  return true;
}

void AggregateNetDescriptor::highlight(std::any object, Painter& painter) const
{
  AggregateNet* anet = std::any_cast<AggregateNet>(&object);

  ColorGenerator generator;
  painter.saveState();
  for (auto* net : anet->getNets()) {
    painter.setPenAndBrush(generator.getColor(), true);
    net_descriptor_->highlight(net, painter);
  }
  painter.restoreState();
}

Descriptor::Properties AggregateNetDescriptor::getProperties(
    std::any object) const
{
  AggregateNet* anet = std::any_cast<AggregateNet>(&object);
  Properties props;

  auto gui = Gui::get();

  SelectionSet nets;
  for (auto* net : anet->getNets()) {
    nets.insert(gui->makeSelected(net));
  }
  props.push_back({"Nets", nets});

  return props;
}

Selected AggregateNetDescriptor::makeSelected(std::any object) const
{
  if (auto* anet = std::any_cast<AggregateNet>(&object)) {
    return Selected(*anet, this);
  }
  return Selected();
}

bool AggregateNetDescriptor::lessThan(std::any l, std::any r) const
{
  AggregateNet* l_anet = std::any_cast<AggregateNet>(&l);
  AggregateNet* r_anet = std::any_cast<AggregateNet>(&r);
  return l_anet->getName() < r_anet->getName();
}

bool AggregateNetDescriptor::getAllObjects(SelectionSet& objects) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return false;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return false;
  }

  for (auto* net : block->getNets()) {
    if (AggregateNet::isAggregate(net)) {
      objects.insert(makeSelected(AggregateNet(net)));
    }
  }

  return true;
}

Descriptor::Actions AggregateNetDescriptor::getActions(std::any object) const
{
  AggregateNet anet = *std::any_cast<AggregateNet>(&object);

  auto* gui = Gui::get();
  Descriptor::Actions actions;
  bool has_guides = false;
  for (auto* net : anet.getNets()) {
    has_guides |= !net->getGuides().empty();
  }
  if (has_guides) {
    actions.push_back(Descriptor::Action{"Route Guides", [this, gui, anet]() {
                                           bool guides_on = false;
                                           for (auto* net : anet.getNets()) {
                                             guides_on
                                                 |= guide_nets_.count(net) != 0;
                                           }
                                           for (auto* net : anet.getNets()) {
                                             if (!guides_on) {
                                               gui->addRouteGuides(net);
                                             } else {
                                               gui->removeRouteGuides(net);
                                             }
                                           }
                                           return makeSelected(anet);
                                         }});
  }
  return actions;
}

}  // namespace gui
