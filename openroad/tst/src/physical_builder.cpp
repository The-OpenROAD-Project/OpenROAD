// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "tst/physical_builder.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"

namespace tst {

namespace {

std::string pointText(const odb::Point& point)
{
  return "(" + std::to_string(point.x()) + ", " + std::to_string(point.y())
         + ")";
}

}  // namespace

odb::dbTech* makeTech(odb::dbDatabase* db,
                      const char* name,
                      const int dbu_per_micron)
{
  db->setDbuPerMicron(dbu_per_micron);
  odb::dbTech* tech = odb::dbTech::create(db, name);
  tech->setLefUnits(dbu_per_micron);
  return tech;
}

odb::dbLib* makeLib(odb::dbDatabase* db, odb::dbTech* tech, const char* name)
{
  odb::dbLib* lib = odb::dbLib::create(db, name, tech, '/');
  lib->setLefUnits(tech->getLefUnits());
  return lib;
}

odb::dbBlock* makeBlock(odb::dbDatabase* db,
                        odb::dbTech* tech,
                        const odb::Rect& die_area,
                        const char* name)
{
  odb::dbChip* chip = odb::dbChip::create(db, tech);
  odb::dbBlock* block = odb::dbBlock::create(chip, name);
  block->setDefUnits(tech->getDbUnitsPerMicron());
  block->setDieArea(die_area);
  return block;
}

odb::dbTechLayer* addRoutingLayer(odb::dbTech* tech,
                                  const char* name,
                                  const LayerSpec& spec)
{
  odb::dbTechLayer* layer
      = odb::dbTechLayer::create(tech, name, odb::dbTechLayerType::ROUTING);
  if (layer == nullptr) {
    throw std::runtime_error(std::string("Layer already exists: ") + name);
  }
  layer->setDirection(spec.direction);
  layer->setWidth(spec.width);
  layer->setPitch(spec.pitch);
  if (spec.thickness) {
    layer->setThickness(*spec.thickness);
  }
  return layer;
}

odb::dbTechLayer* addCutLayer(odb::dbTech* tech, const char* name)
{
  odb::dbTechLayer* layer
      = odb::dbTechLayer::create(tech, name, odb::dbTechLayerType::CUT);
  if (layer == nullptr) {
    throw std::runtime_error(std::string("Layer already exists: ") + name);
  }
  return layer;
}

odb::dbTechVia* addVia(odb::dbTech* tech,
                       const char* name,
                       odb::dbTechLayer* bottom,
                       odb::dbTechLayer* cut,
                       odb::dbTechLayer* top,
                       const int cut_size,
                       const int enclosure)
{
  odb::dbTechVia* via = odb::dbTechVia::create(tech, name);
  if (via == nullptr) {
    throw std::runtime_error(std::string("Via already exists: ") + name);
  }
  const int half_cut = cut_size / 2;
  const int half_enc = half_cut + enclosure;
  odb::dbBox::create(via, bottom, -half_enc, -half_enc, half_enc, half_enc);
  odb::dbBox::create(via, cut, -half_cut, -half_cut, half_cut, half_cut);
  odb::dbBox::create(via, top, -half_enc, -half_enc, half_enc, half_enc);
  return via;
}

LayerStack makeLayerStack(odb::dbTech* tech,
                          const int num_metals,
                          const LayerSpec& spec)
{
  LayerStack stack;
  LayerSpec layer_spec = spec;
  for (int level = 1; level <= num_metals; ++level) {
    if (level > 1) {
      const std::string cut_name = "V" + std::to_string(level - 1);
      stack.cuts.push_back(addCutLayer(tech, cut_name.c_str()));
    }
    const std::string metal_name = "M" + std::to_string(level);
    stack.metals.push_back(
        addRoutingLayer(tech, metal_name.c_str(), layer_spec));
    layer_spec.direction
        = layer_spec.direction == odb::dbTechLayerDir::HORIZONTAL
              ? odb::dbTechLayerDir::VERTICAL
              : odb::dbTechLayerDir::HORIZONTAL;
  }
  for (int level = 1; level < num_metals; ++level) {
    const std::string via_name
        = "VIA" + std::to_string(level) + std::to_string(level + 1);
    stack.vias.push_back(addVia(tech,
                                via_name.c_str(),
                                stack.metal(level),
                                stack.cut(level),
                                stack.metal(level + 1)));
  }
  return stack;
}

odb::dbTechLayerAntennaRule* applyAntennaRule(odb::dbTechLayer* layer,
                                              const AntennaRuleSpec& spec)
{
  odb::dbTechLayerAntennaRule* rule = layer->hasDefaultAntennaRule()
                                          ? layer->getDefaultAntennaRule()
                                          : layer->createDefaultAntennaRule();

  // odb asserts that a table is empty before setting it.
  auto check_unset = [layer](const std::optional<Pwl>& spec_table,
                             const odb::dbTechLayerAntennaRule::pwl_pair& table,
                             const char* field) {
    if (spec_table && !table.indices.empty()) {
      throw std::invalid_argument(std::string("Antenna rule ") + field
                                  + " is already set on layer "
                                  + layer->getName());
    }
  };
  check_unset(spec.diff_par, rule->getDiffPAR(), "diff_par");
  check_unset(spec.diff_car, rule->getDiffCAR(), "diff_car");
  check_unset(spec.diff_psr, rule->getDiffPSR(), "diff_psr");
  check_unset(spec.diff_csr, rule->getDiffCSR(), "diff_csr");
  check_unset(spec.gate_plus_diff_pwl,
              rule->getGatePlusDiffPWL(),
              "gate_plus_diff_pwl");
  check_unset(
      spec.area_diff_reduce, rule->getAreaDiffReduce(), "area_diff_reduce");

  if (spec.par) {
    rule->setPAR(*spec.par);
  }
  if (spec.car) {
    rule->setCAR(*spec.car);
  }
  if (spec.psr) {
    rule->setPSR(*spec.psr);
  }
  if (spec.csr) {
    rule->setCSR(*spec.csr);
  }

  if (spec.diff_par) {
    rule->setDiffPAR(spec.diff_par->indices, spec.diff_par->ratios);
  }
  if (spec.diff_car) {
    rule->setDiffCAR(spec.diff_car->indices, spec.diff_car->ratios);
  }
  if (spec.diff_psr) {
    rule->setDiffPSR(spec.diff_psr->indices, spec.diff_psr->ratios);
  }
  if (spec.diff_csr) {
    rule->setDiffCSR(spec.diff_csr->indices, spec.diff_csr->ratios);
  }

  if (spec.area_factor) {
    rule->setAreaFactor(*spec.area_factor, spec.area_factor_diffuse_only);
  }
  if (spec.side_area_factor) {
    rule->setSideAreaFactor(*spec.side_area_factor,
                            spec.side_area_factor_diffuse_only);
  }

  if (spec.area_minus_diff) {
    rule->setAreaMinusDiffFactor(*spec.area_minus_diff);
  }
  if (spec.gate_plus_diff) {
    rule->setGatePlusDiffFactor(*spec.gate_plus_diff);
  }
  if (spec.gate_plus_diff_pwl) {
    rule->setGatePlusDiffPWL(spec.gate_plus_diff_pwl->indices,
                             spec.gate_plus_diff_pwl->ratios);
  }
  if (spec.area_diff_reduce) {
    rule->setAreaDiffReduce(spec.area_diff_reduce->indices,
                            spec.area_diff_reduce->ratios);
  }
  if (spec.cum_routing_plus_cut) {
    rule->setAntennaCumRoutingPlusCut();
  }
  return rule;
}

odb::dbMaster* makeMaster(odb::dbLib* lib,
                          const char* name,
                          const int width,
                          const int height,
                          const std::vector<PinSpec>& pins,
                          const odb::dbMasterType type)
{
  odb::dbMaster* master = odb::dbMaster::create(lib, name);
  if (master == nullptr) {
    throw std::runtime_error(std::string("Master already exists: ") + name);
  }
  master->setType(type);
  master->setWidth(width);
  master->setHeight(height);

  for (const PinSpec& pin : pins) {
    odb::dbMTerm* mterm
        = odb::dbMTerm::create(master, pin.name, pin.io_type, pin.sig_type);
    if (mterm == nullptr) {
      throw std::runtime_error(std::string("Duplicate pin: ") + pin.name);
    }
    odb::dbMPin* mpin = odb::dbMPin::create(mterm);
    if (pin.layer != nullptr) {
      odb::dbBox::create(mpin,
                         pin.layer,
                         pin.rect.xMin(),
                         pin.rect.yMin(),
                         pin.rect.xMax(),
                         pin.rect.yMax());
    }
    if (pin.gate_area) {
      mterm->createDefaultAntennaModel()->addGateAreaEntry(*pin.gate_area);
    }
    if (pin.diff_area) {
      mterm->addDiffAreaEntry(*pin.diff_area);
    }
  }

  master->setFrozen();
  return master;
}

RoutePath::RoutePath(odb::dbNet* net) : net_(net)
{
  if (odb::dbWire* old_wire = net->getWire()) {
    odb::dbWire::destroy(old_wire);
  }
  wire_ = odb::dbWire::create(net);
  encoder_.begin(wire_);
}

RoutePath::~RoutePath()
{
  if (!built_) {
    build();
  }
}

RoutePath& RoutePath::on(odb::dbTechLayer* layer, const odb::Point& start)
{
  layer_ = layer;
  point_ = start;
  encoder_.newPath(layer, odb::dbWireType::ROUTED);
  encoder_.addPoint(start.x(), start.y());
  return *this;
}

RoutePath& RoutePath::to(const odb::Point& point)
{
  if (layer_ == nullptr) {
    throw std::logic_error("RoutePath::to called before on");
  }
  if (point.x() != point_.x() && point.y() != point_.y()) {
    throw std::invalid_argument("RoutePath::to: diagonal step from "
                                + pointText(point_) + " to "
                                + pointText(point));
  }
  point_ = point;
  encoder_.addPoint(point.x(), point.y());
  return *this;
}

RoutePath& RoutePath::via(odb::dbTechVia* via)
{
  if (layer_ == nullptr) {
    throw std::logic_error("RoutePath::via called before on");
  }
  if (layer_ != via->getBottomLayer() && layer_ != via->getTopLayer()) {
    throw std::invalid_argument("RoutePath::via: via " + via->getName()
                                + " does not connect to layer "
                                + layer_->getName());
  }
  encoder_.addTechVia(via);
  layer_ = (layer_ == via->getBottomLayer()) ? via->getTopLayer()
                                             : via->getBottomLayer();
  return *this;
}

odb::dbWire* RoutePath::build()
{
  if (!built_) {
    encoder_.end();
    net_->setWireType(odb::dbWireType::ROUTED);
    built_ = true;
  }
  return wire_;
}

odb::dbGuide* addGuide(odb::dbNet* net,
                       odb::dbTechLayer* layer,
                       const odb::Rect& rect,
                       odb::dbTechLayer* via_layer)
{
  return odb::dbGuide::create(net,
                              layer,
                              via_layer != nullptr ? via_layer : layer,
                              rect,
                              /*is_congested=*/false);
}

}  // namespace tst
