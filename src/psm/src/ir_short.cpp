// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ir_short.h"

#include <cstdint>
#include <string>

#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace psm {

IRShort::IRShort(odb::dbTechLayer* layer, const odb::Polygon& shape)
    : layer_(layer), shape_(shape)
{
}

void IRShort::report(odb::dbNet* net, utl::Logger* logger, double dbu) const
{
  const odb::Rect rect = shape_.getEnclosingRect();

  logger->warn(utl::PSM,
               43,
               "Shorted shape on net {} to {} at ({:4.3f}um, {:4.3f}um) - "
               "({:4.3f}um, {:4.3f}um), layer: {}.",
               net->getName(),
               describe(),
               rect.xMin() / dbu,
               rect.yMin() / dbu,
               rect.xMax() / dbu,
               rect.yMax() / dbu,
               layer_->getName());
}

void IRShort::commit(odb::dbNet* net, odb::dbMarkerCategory* category) const
{
  odb::dbMarker* marker = odb::dbMarker::create(category);
  if (marker == nullptr) {
    return;
  }

  marker->addSource(net);
  addSources(marker);
  marker->setTechLayer(layer_);
  if (shape_.isRect()) {
    marker->addShape(shape_.getEnclosingRect());
  } else {
    marker->addShape(shape_);
  }
}

////////////////////

IRShortBPin::IRShortBPin(odb::dbTechLayer* layer,
                         const odb::Polygon& shape,
                         odb::dbBPin* bpin)
    : IRShort(layer, shape), bpin_(bpin)
{
}

std::string IRShortBPin::describe() const
{
  std::string netname = bpin_->getBTerm()->getNet()
                            ? bpin_->getBTerm()->getNet()->getName()
                            : "unconnected";
  return "terminal " + bpin_->getBTerm()->getName() + " (net " + netname + ")";
}

void IRShortBPin::addSources(odb::dbMarker* marker) const
{
  // dbMarker is unable to name a dbBPin, so use the terminal it belongs to
  odb::dbBTerm* bterm = bpin_->getBTerm();
  marker->addSource(bterm);
  marker->addSource(bterm->getNet());
}

////////////////////

IRShortInst::IRShortInst(odb::dbTechLayer* layer,
                         const odb::Polygon& shape,
                         odb::dbInst* inst)
    : IRShort(layer, shape), inst_(inst)
{
}

std::string IRShortInst::describe() const
{
  return "instance " + inst_->getName();
}

void IRShortInst::addSources(odb::dbMarker* marker) const
{
  marker->addSource(inst_);
}

////////////////////

IRShortITerm::IRShortITerm(odb::dbTechLayer* layer,
                           const odb::Polygon& shape,
                           odb::dbITerm* iterm)
    : IRShort(layer, shape), iterm_(iterm)
{
}

std::string IRShortITerm::describe() const
{
  std::string netname
      = iterm_->getNet() ? iterm_->getNet()->getName() : "unconnected";
  return "terminal " + iterm_->getName() + " (net " + netname + ")";
}

void IRShortITerm::addSources(odb::dbMarker* marker) const
{
  marker->addSource(iterm_);
  marker->addSource(iterm_->getNet());
}

////////////////////

IRShortObstruction::IRShortObstruction(odb::dbTechLayer* layer,
                                       const odb::Polygon& shape,
                                       odb::dbObstruction* obstruction)
    : IRShort(layer, shape), obstruction_(obstruction)
{
}

std::string IRShortObstruction::describe() const
{
  odb::dbInst* inst = obstruction_->getInstance();
  if (inst != nullptr) {
    return "obstruction of " + inst->getName();
  }

  return "obstruction";
}

void IRShortObstruction::addSources(odb::dbMarker* marker) const
{
  marker->addSource(obstruction_);
  marker->addSource(obstruction_->getInstance());
}

////////////////////

IRShortFill::IRShortFill(const odb::Polygon& shape, odb::dbFill* fill)
    : IRShort(fill->getTechLayer(), shape), fill_(fill)
{
}

std::string IRShortFill::describe() const
{
  // the mask is the only property of the fill that helps identify it, since
  // the marker is unable to point to it
  const uint32_t mask = fill_->maskNumber();
  if (mask == 0) {
    return "fill";
  }

  return fmt::format("fill with mask {}", mask);
}

void IRShortFill::addSources(odb::dbMarker* marker) const
{
  // marker->addSource(fill_);
}

////////////////////

IRShortNet::IRShortNet(odb::dbTechLayer* layer,
                       const odb::Polygon& shape,
                       odb::dbNet* net)
    : IRShort(layer, shape), net_(net)
{
}

std::string IRShortNet::describe() const
{
  return "net " + net_->getName();
}

void IRShortNet::addSources(odb::dbMarker* marker) const
{
  marker->addSource(net_);
}

}  // namespace psm
