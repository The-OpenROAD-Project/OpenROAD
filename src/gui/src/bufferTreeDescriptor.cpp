// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "bufferTreeDescriptor.h"

#include <algorithm>
#include <any>
#include <functional>
#include <set>
#include <string>

#include "colorGenerator.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "sta/Liberty.hh"

namespace gui {

sta::dbSta* BufferTree::sta_ = nullptr;

BufferTree::BufferTree(odb::dbNet* net)
{
  populate(net);

  // Sort nets to ensure stable color order
  std::sort(
      nets_.begin(), nets_.end(), [](odb::dbNet* r, odb::dbNet* l) -> bool {
        return r->getName() < l->getName();
      });

  name_ = nets_[0]->getName();
}

void BufferTree::populate(odb::dbNet* net)
{
  if (net == nullptr) {
    return;
  }

  if (std::find(nets_.begin(), nets_.end(), net) != nets_.end()) {
    return;
  }

  nets_.push_back(net);

  for (auto* bterm : net->getBTerms()) {
    bterm_terms_.insert(bterm);
  }

  for (auto* iterm : net->getITerms()) {
    auto* inst = iterm->getInst();
    if (!BufferTree::isAggregate(inst)) {
      iterm_terms_.insert(iterm);
      continue;
    }

    insts_.insert(inst);

    for (auto* next_iterm : inst->getITerms()) {
      if (!next_iterm->getSigType().isSupply()) {
        populate(next_iterm->getNet());
      }
    }
  }
}

bool BufferTree::isAggregate(odb::dbNet* net)
{
  if (net == nullptr) {
    return false;
  }

  for (auto* iterm : net->getITerms()) {
    if (BufferTree::isAggregate(iterm->getInst())) {
      return true;
    }
  }

  return false;
}

bool BufferTree::isAggregate(odb::dbInst* inst)
{
  if (inst == nullptr) {
    return false;
  }

  auto* network = sta_->getDbNetwork();
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

BufferTreeDescriptor::BufferTreeDescriptor(
    odb::dbDatabase* db,
    sta::dbSta* sta,
    const std::set<odb::dbNet*>& focus_nets,
    const std::set<odb::dbNet*>& guide_nets,
    const std::set<odb::dbNet*>& tracks_nets)
    : db_(db),
      net_descriptor_(Gui::get()->getDescriptor<odb::dbNet*>()),
      focus_nets_(focus_nets),
      guide_nets_(guide_nets),
      tracks_nets_(tracks_nets)
{
  BufferTree::setSTA(sta);
}

std::string BufferTreeDescriptor::getName(const std::any& object) const
{
  const BufferTree* bnet = std::any_cast<BufferTree>(&object);
  return bnet->getName();
}

std::string BufferTreeDescriptor::getTypeName() const
{
  return "Buffer Tree";
}

bool BufferTreeDescriptor::getBBox(const std::any& object,
                                   odb::Rect& bbox) const
{
  const BufferTree* bnet = std::any_cast<BufferTree>(&object);
  bbox.mergeInit();
  for (auto* net : bnet->getNets()) {
    odb::Rect box;
    if (net_descriptor_->getBBox(net, box)) {
      bbox.merge(box);
    }
  }
  return true;
}

void BufferTreeDescriptor::highlight(const std::any& object,
                                     Painter& painter) const
{
  const BufferTree* bnet = std::any_cast<BufferTree>(&object);

  ColorGenerator generator;
  painter.saveState();
  for (auto* net : bnet->getNets()) {
    painter.setPenAndBrush(generator.getColor(), true);
    net_descriptor_->highlight(net, painter);
  }
  painter.restoreState();
}

Descriptor::Properties BufferTreeDescriptor::getProperties(
    const std::any& object) const
{
  const BufferTree* bnet = std::any_cast<BufferTree>(&object);
  Properties props;

  auto gui = Gui::get();

  SelectionSet nets;
  for (auto* net : bnet->getNets()) {
    nets.insert(gui->makeSelected(net));
  }
  props.push_back({"Nets", nets});

  SelectionSet insts;
  for (auto* inst : bnet->getInsts()) {
    insts.insert(gui->makeSelected(inst));
  }
  props.push_back({"Instances", insts});

  SelectionSet terminals;
  for (auto* iterm : bnet->getITerms()) {
    terminals.insert(gui->makeSelected(iterm));
  }
  for (auto* bterm : bnet->getBTerms()) {
    terminals.insert(gui->makeSelected(bterm));
  }
  props.push_back({"Terminals", terminals});

  return props;
}

Selected BufferTreeDescriptor::makeSelected(const std::any& object) const
{
  if (auto* bnet = std::any_cast<BufferTree>(&object)) {
    return Selected(*bnet, this);
  }
  return Selected();
}

bool BufferTreeDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  const BufferTree* l_bnet = std::any_cast<BufferTree>(&l);
  const BufferTree* r_bnet = std::any_cast<BufferTree>(&r);
  return l_bnet->getName() < r_bnet->getName();
}

void BufferTreeDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* net : block->getNets()) {
    if (BufferTree::isAggregate(net)) {
      func({BufferTree(net), this});
    }
  }
}

Descriptor::Actions BufferTreeDescriptor::getActions(
    const std::any& object) const
{
  BufferTree bnet = *std::any_cast<BufferTree>(&object);

  auto* gui = Gui::get();
  Descriptor::Actions actions;
  bool is_focus = true;
  for (auto* net : bnet.getNets()) {
    is_focus &= focus_nets_.contains(net);
  }
  if (!is_focus) {
    actions.push_back(Descriptor::Action{"Focus", [this, gui, &bnet]() {
                                           for (auto* net : bnet.getNets()) {
                                             gui->addFocusNet(net);
                                           }
                                           return makeSelected(bnet);
                                         }});
  } else {
    actions.push_back(Descriptor::Action{"De-focus", [this, gui, &bnet]() {
                                           for (auto* net : bnet.getNets()) {
                                             gui->removeFocusNet(net);
                                           }
                                           return makeSelected(bnet);
                                         }});
  }
  bool has_guides = false;
  for (auto* net : bnet.getNets()) {
    has_guides |= !net->getGuides().empty();
  }
  if (has_guides) {
    actions.push_back(Descriptor::Action{"Route Guides", [this, gui, &bnet]() {
                                           bool guides_on = false;
                                           for (auto* net : bnet.getNets()) {
                                             guides_on
                                                 |= guide_nets_.contains(net);
                                           }
                                           for (auto* net : bnet.getNets()) {
                                             if (!guides_on) {
                                               gui->addRouteGuides(net);
                                             } else {
                                               gui->removeRouteGuides(net);
                                             }
                                           }
                                           return makeSelected(bnet);
                                         }});
  }
  bool has_net_tracks = false;
  for (auto* net : bnet.getNets()) {
    has_net_tracks |= !net->getTracks().empty();
  }
  if (has_net_tracks) {
    actions.push_back(Descriptor::Action{"Tracks", [this, gui, &bnet]() {
                                           bool tracks_on = false;
                                           for (auto* net : bnet.getNets()) {
                                             tracks_on
                                                 |= tracks_nets_.contains(net);
                                           }
                                           for (auto* net : bnet.getNets()) {
                                             if (!tracks_on) {
                                               gui->addNetTracks(net);
                                             } else {
                                               gui->removeNetTracks(net);
                                             }
                                           }
                                           return makeSelected(bnet);
                                         }});
  }
  return actions;
}

}  // namespace gui
