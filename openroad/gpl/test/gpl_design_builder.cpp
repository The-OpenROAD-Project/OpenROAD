// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "gpl_design_builder.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "tst/physical_builder.h"

namespace gpl {

namespace {

constexpr int kPinSize = 100;

}  // namespace

DesignBuilder::DesignBuilder(odb::dbDatabase* db, const int cell_sites)
{
  odb::dbTech* tech = tst::makeTech(db, "tech", kDbuPerMicron);
  layer_ = tst::addRoutingLayer(tech, "M1");
  odb::dbLib* lib = tst::makeLib(db, tech);
  odb::dbSite* site = odb::dbSite::create(lib, "site");
  site->setWidth(kSiteWidth);
  site->setHeight(kRowHeight);
  site->setClass(odb::dbSiteClass::CORE);

  const int width = cell_sites * kSiteWidth;
  const int mid_x = width / 2;
  const int mid_y = kRowHeight / 2;
  const int half = kPinSize / 2;
  master_ = tst::makeMaster(
      lib,
      "cell",
      width,
      kRowHeight,
      {
          {.name = "W",
           .layer = layer_,
           .rect = {0, mid_y - half, kPinSize, mid_y + half}},
          {.name = "E",
           .io_type = odb::dbIoType::OUTPUT,
           .layer = layer_,
           .rect = {width - kPinSize, mid_y - half, width, mid_y + half}},
          {.name = "S",
           .layer = layer_,
           .rect = {mid_x - half, 0, mid_x + half, kPinSize}},
          {.name = "N",
           .io_type = odb::dbIoType::OUTPUT,
           .layer = layer_,
           .rect
           = {mid_x - half, kRowHeight - kPinSize, mid_x + half, kRowHeight}},
      });
  master_->setSite(site);

  // One row of margin around the core.
  const int margin = kRowHeight;
  block_ = tst::makeBlock(
      db,
      tech,
      odb::Rect(0, 0, kCoreSize + 2 * margin, kCoreSize + 2 * margin));
  for (int row = 0; row < kRows; ++row) {
    odb::dbRow::create(block_,
                       ("row" + std::to_string(row)).c_str(),
                       site,
                       margin,
                       margin + row * kRowHeight,
                       odb::dbOrientType::R0,
                       odb::dbRowDir::HORIZONTAL,
                       kSites,
                       kSiteWidth);
  }
  block_->setCoreArea(block_->computeCoreArea());
}

odb::dbInst* DesignBuilder::addCell(const std::string& name)
{
  return odb::dbInst::create(block_, master_, name.c_str());
}

odb::dbBTerm* DesignBuilder::addIo(const std::string& name,
                                   const Edge edge,
                                   const double t)
{
  const odb::Rect die = block_->getDieArea();
  const odb::Rect core = block_->getCoreArea();
  const int x = core.xMin() + static_cast<int>(t * core.dx());
  const int y = core.yMin() + static_cast<int>(t * core.dy());
  const int half = kPinSize / 2;
  odb::Rect box;
  switch (edge) {
    case Edge::kLeft:
      box = {die.xMin(), y - half, die.xMin() + kPinSize, y + half};
      break;
    case Edge::kRight:
      box = {die.xMax() - kPinSize, y - half, die.xMax(), y + half};
      break;
    case Edge::kBottom:
      box = {x - half, die.yMin(), x + half, die.yMin() + kPinSize};
      break;
    case Edge::kTop:
      box = {x - half, die.yMax() - kPinSize, x + half, die.yMax()};
      break;
  }
  odb::dbBTerm* bterm = odb::dbBTerm::create(net(name), name.c_str());
  bterm->setSigType(odb::dbSigType::SIGNAL);
  odb::dbBPin* pin = odb::dbBPin::create(bterm);
  odb::dbBox::create(
      pin, layer_, box.xMin(), box.yMin(), box.xMax(), box.yMax());
  pin->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  return bterm;
}

void DesignBuilder::connect(odb::dbInst* inst,
                            const char* pin,
                            const std::string& net_name)
{
  iterm(inst, pin)->connect(net(net_name));
}

void DesignBuilder::connect(odb::dbInst* inst,
                            const char* pin,
                            odb::dbBTerm* io)
{
  iterm(inst, pin)->connect(io->getNet());
}

odb::dbITerm* DesignBuilder::iterm(odb::dbInst* inst, const char* pin)
{
  odb::dbITerm* iterm = inst->findITerm(pin);
  if (!iterm) {
    throw std::invalid_argument("No pin " + std::string(pin) + " on "
                                + inst->getName());
  }
  return iterm;
}

odb::dbNet* DesignBuilder::net(const std::string& name)
{
  odb::dbNet* net = block_->findNet(name.c_str());
  return net ? net : odb::dbNet::create(block_, name.c_str());
}

std::vector<odb::dbInst*> makeChain(DesignBuilder& builder, const int length)
{
  if (length < 1) {
    throw std::invalid_argument("A chain needs at least one cell");
  }
  std::vector<odb::dbInst*> cells;
  for (int i = 0; i < length; ++i) {
    cells.push_back(builder.addCell("c" + std::to_string(i)));
  }
  using Edge = DesignBuilder::Edge;
  builder.connect(cells.front(), "W", builder.addIo("in", Edge::kLeft, 0.5));
  for (int i = 0; i + 1 < length; ++i) {
    const std::string net = "n" + std::to_string(i);
    builder.connect(cells[i], "E", net);
    builder.connect(cells[i + 1], "W", net);
  }
  builder.connect(cells.back(), "E", builder.addIo("out", Edge::kRight, 0.5));
  return cells;
}

namespace {

// A cols x rows grid of cells joined to their four neighbors, with names
// and nets prefixed by prefix.
std::vector<std::vector<odb::dbInst*>> latticeCells(DesignBuilder& builder,
                                                    const int cols,
                                                    const int rows,
                                                    const std::string& prefix)
{
  std::vector<std::vector<odb::dbInst*>> cells(cols);
  for (int i = 0; i < cols; ++i) {
    for (int j = 0; j < rows; ++j) {
      cells[i].push_back(builder.addCell(prefix + "_" + std::to_string(i) + "_"
                                         + std::to_string(j)));
    }
  }
  for (int i = 0; i < cols; ++i) {
    for (int j = 0; j < rows; ++j) {
      const std::string at = std::to_string(i) + "_" + std::to_string(j);
      if (i + 1 < cols) {
        builder.connect(cells[i][j], "E", prefix + "_h_" + at);
        builder.connect(cells[i + 1][j], "W", prefix + "_h_" + at);
      }
      if (j + 1 < rows) {
        builder.connect(cells[i][j], "N", prefix + "_v_" + at);
        builder.connect(cells[i][j + 1], "S", prefix + "_v_" + at);
      }
    }
  }
  return cells;
}

}  // namespace

std::vector<std::vector<odb::dbInst*>> makeLattice(DesignBuilder& builder,
                                                   const int cols,
                                                   const int rows)
{
  std::vector<std::vector<odb::dbInst*>> cells
      = latticeCells(builder, cols, rows, "c");
  using Edge = DesignBuilder::Edge;
  for (int j = 0; j < rows; ++j) {
    const double t = (j + 0.5) / rows;
    const std::string at = std::to_string(j);
    builder.connect(cells[0][j], "W", builder.addIo("w" + at, Edge::kLeft, t));
    builder.connect(
        cells[cols - 1][j], "E", builder.addIo("e" + at, Edge::kRight, t));
  }
  for (int i = 0; i < cols; ++i) {
    const double t = (i + 0.5) / cols;
    const std::string at = std::to_string(i);
    builder.connect(
        cells[i][0], "S", builder.addIo("s" + at, Edge::kBottom, t));
    builder.connect(
        cells[i][rows - 1], "N", builder.addIo("n" + at, Edge::kTop, t));
  }
  return cells;
}

std::vector<odb::dbInst*> makeUnconnected(DesignBuilder& builder,
                                          const int count)
{
  std::vector<odb::dbInst*> cells;
  for (int i = 0; i < count; ++i) {
    cells.push_back(builder.addCell("c" + std::to_string(i)));
  }
  return cells;
}

TwoClusters makeTwoClusters(DesignBuilder& builder, const int side)
{
  using Edge = DesignBuilder::Edge;
  TwoClusters clusters;
  const std::vector<std::vector<odb::dbInst*>> left
      = latticeCells(builder, side, side, "a");
  const std::vector<std::vector<odb::dbInst*>> right
      = latticeCells(builder, side, side, "b");
  builder.connect(
      left[0][side / 2], "W", builder.addIo("a_in", Edge::kLeft, 0.5));
  builder.connect(
      right[side - 1][side / 2], "E", builder.addIo("b_in", Edge::kRight, 0.5));
  for (int i = 0; i < side; ++i) {
    clusters.left.insert(clusters.left.end(), left[i].begin(), left[i].end());
    clusters.right.insert(
        clusters.right.end(), right[i].begin(), right[i].end());
  }
  return clusters;
}

}  // namespace gpl
