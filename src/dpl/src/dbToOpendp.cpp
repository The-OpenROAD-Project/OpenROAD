// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cfloat>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "PlacementDRC.h"
#include "dpl/Opendp.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/architecture.h"
#include "infrastructure/network.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/util.h"
#include "util/symmetry.h"
#include "utl/Logger.h"

namespace dpl {

using std::string;
using std::vector;

using odb::dbBox;
using odb::dbMaster;
using odb::dbOrientType;
using odb::dbRegion;
using odb::Rect;

void Opendp::importDb()
{
  block_ = db_->getChip()->getBlock();
  core_ = block_->getCoreArea();
  grid_->setCore(core_);
  have_fillers_ = false;
  disallow_one_site_gaps_ = !odb::hasOneSiteMaster(db_);
  debugPrint(logger_,
             utl::DPL,
             "one_site_gap",
             1,
             "one_site_gaps disallowed: {}",
             disallow_one_site_gaps_ ? "true" : "false");
  importClear();
  grid_->examineRows(block_);
  initPlacementDRC();
  createNetwork();
  createArchitecture();
  setUpPlacementGroups();
}

void Opendp::importClear()
{
  deleteGrid();
  have_multi_row_cells_ = false;
  network_->clear();
  arch_->clear();
}

void Opendp::initPlacementDRC()
{
  drc_engine_ = std::make_unique<PlacementDRC>(
      grid_.get(), db_->getTech(), padding_.get(), !odb::hasOneSiteMaster(db_));
}

static bool swapWidthHeight(const dbOrientType& orient)
{
  switch (orient.getValue()) {
    case dbOrientType::R90:
    case dbOrientType::MXR90:
    case dbOrientType::R270:
    case dbOrientType::MYR90:
      return true;
    case dbOrientType::R0:
    case dbOrientType::R180:
    case dbOrientType::MY:
    case dbOrientType::MX:
      return false;
  }
  // gcc warning
  return false;
}

Rect Opendp::getBbox(odb::dbInst* inst)
{
  dbMaster* master = inst->getMaster();

  int loc_x, loc_y;
  inst->getLocation(loc_x, loc_y);
  // Shift by core lower left.
  loc_x -= core_.xMin();
  loc_y -= core_.yMin();

  int width = master->getWidth();
  int height = master->getHeight();
  if (swapWidthHeight(inst->getOrient())) {
    std::swap(width, height);
  }

  return Rect(loc_x, loc_y, loc_x + width, loc_y + height);
}
void Opendp::createNetwork()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  network_->setCore(core_);
  ///////////////////////////////////
  auto min_row_height = std::numeric_limits<int>::max();
  for (odb::dbRow* row : db_->getChip()->getBlock()->getRows()) {
    min_row_height = std::min(min_row_height, row->getSite()->getHeight());
  }
  ///////////////////////////////////
  using odb::dbInst;
  auto block_insts = block->getInsts();
  std::vector<dbInst*> insts(block_insts.begin(), block_insts.end());
  std::ranges::stable_sort(
      insts, [](dbInst* a, dbInst* b) { return a->getName() < b->getName(); });

  for (dbInst* inst : insts) {
    // Skip instances which are not placeable.
    if (!inst->getMaster()->isCoreAutoPlaceable()) {
      continue;
    }
    network_->addMaster(inst->getMaster(), grid_.get(), drc_engine_.get());
    network_->addNode(inst);
    if (inst->getMaster()->isCore()
        && network_->getMaster(inst->getMaster())->isMultiRow()) {
      have_multi_row_cells_ = true;
    }
    if (isFiller(inst)) {
      have_fillers_ = true;
    }
  }
  for (odb::dbBTerm* bterm : block->getBTerms()) {
    // Skip supply nets.
    odb::dbNet* net = bterm->getNet();
    if (!net || net->getSigType().isSupply()) {
      continue;
    }
    if (bterm->getBBox().isInverted()) {
      logger_->error(
          utl::DPL, 386, "BTerm {} has no shapes.", bterm->getName());
    }

    network_->addNode(bterm);
  }

  auto nets = block->getNets();
  for (odb::dbNet* net : nets) {
    // Skip supply nets.
    if (net->getSigType().isSupply()) {
      continue;
    }
    network_->addEdge(net);
  }

  for (odb::dbBlockage* blockage : block->getBlockages()) {
    if (!blockage->isSoft()) {
      auto box = blockage->getBBox()->getBox();
      box.moveDelta(-core_.xMin(), -core_.yMin());
      network_->createAndAddBlockage(box);
    }
  }
}
////////////////////////////////////////////////////////////////
void Opendp::createArchitecture()
{
  odb::dbBlock* block = db_->getChip()->getBlock();

  auto min_row_height = std::numeric_limits<int>::max();
  for (odb::dbRow* row : block->getRows()) {
    min_row_height = std::min(min_row_height, row->getSite()->getHeight());
  }

  std::map<int, std::unordered_set<std::string>> skip_list;

  for (odb::dbRow* row : block->getRows()) {
    if (row->getSite()->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    if (row->getDirection() != odb::dbRowDir::HORIZONTAL) {
      // error.
      continue;
    }
    odb::dbSite* site = row->getSite();
    if (site->getHeight() > min_row_height) {
      skip_list[site->getHeight()].insert(site->getName());
      continue;
    }
    odb::Point origin = row->getOrigin();

    Architecture::Row* archRow = arch_->createAndAddRow();

    archRow->setSubRowOrigin(DbuX{origin.x() - core_.xMin()});
    archRow->setBottom(DbuY{origin.y() - core_.yMin()});
    archRow->setSiteSpacing(DbuX{row->getSpacing()});
    archRow->setNumSites(row->getSiteCount());
    archRow->setSiteWidth(DbuX{site->getWidth()});
    archRow->setHeight(DbuY{site->getHeight()});

    // Set defaults.  Top and bottom power is set below.
    archRow->setBottomPower(Architecture::Row::Power_UNK);
    archRow->setTopPower(Architecture::Row::Power_UNK);

    // Symmetry.  From the site.
    unsigned symmetry = 0x00000000;
    if (site->getSymmetryX()) {
      symmetry |= Symmetry_X;
    }
    if (site->getSymmetryY()) {
      symmetry |= Symmetry_Y;
    }
    if (site->getSymmetryR90()) {
      symmetry |= Symmetry_ROT90;
    }
    archRow->setSymmetry(symmetry);

    // Orientation.  From the row.
    archRow->setOrient(row->getOrient());
  }
  // Get surrounding box.
  {
    DbuX xmin = std::numeric_limits<DbuX>::max();
    DbuX xmax = std::numeric_limits<DbuX>::lowest();
    DbuY ymin = std::numeric_limits<DbuY>::max();
    DbuY ymax = std::numeric_limits<DbuY>::lowest();
    for (int r = 0; r < arch_->getNumRows(); r++) {
      Architecture::Row* row = arch_->getRow(r);

      xmin = std::min(xmin, row->getLeft());
      xmax = std::max(xmax, row->getRight());
      ymin = std::min(ymin, row->getBottom());
      ymax = std::max(ymax, row->getTop());
    }
    arch_->setMinX(xmin);
    arch_->setMaxX(xmax);
    arch_->setMinY(ymin);
    arch_->setMaxY(ymax);
  }

  for (int r = 0; r < arch_->getNumRows(); r++) {
    int numSites = arch_->getRow(r)->getNumSites();
    DbuX originX = arch_->getRow(r)->getLeft();
    DbuX siteSpacing = arch_->getRow(r)->getSiteSpacing();
    DbuX siteWidth = arch_->getRow(r)->getSiteWidth();
    const DbuX endGap = siteWidth - siteSpacing;
    if (originX < arch_->getMinX()) {
      originX = arch_->getMinX();
      if (arch_->getRow(r)->getLeft() != originX) {
        arch_->getRow(r)->setSubRowOrigin(originX);
      }
    }
    if (originX + numSites * siteSpacing + endGap > arch_->getMaxX()) {
      numSites = ((arch_->getMaxX() - endGap - originX) / siteSpacing).v;
      if (arch_->getRow(r)->getNumSites() != numSites) {
        arch_->getRow(r)->setNumSites(numSites);
      }
    }
  }
  // PADDING
  arch_->setUsePadding(padding_ != nullptr);
  arch_->setPadding(padding_.get());
  arch_->setSiteWidth(grid_->getSiteWidth());

  arch_->postProcess(network_.get());
}

void Opendp::setUpPlacementGroups()
{
  regions_rtree_.clear();
  odb::dbBlock* block = db_->getChip()->getBlock();
  int count = 0;
  auto db_groups = block->getGroups();
  for (auto db_group : db_groups) {
    dbRegion* region = db_group->getRegion();
    if (region) {
      Group* rptr = arch_->createAndAddRegion();
      rptr->setId(count++);
      Rect bbox;
      bbox.mergeInit();
      for (dbBox* boundary : region->getBoundaries()) {
        Rect box = boundary->getBox();
        box = box.intersect(core_);
        box.moveDelta(-core_.xMin(), -core_.yMin());

        bgBox bgbox(
            bgPoint(box.xMin(), box.yMin()),
            bgPoint(
                box.xMax() - 1,
                box.yMax() - 1));  /// the -1 is to prevent imaginary overlaps
                                   /// where a region ends and another starts
        regions_rtree_.insert(bgbox);
        rptr->addRect(box);
        bbox.merge(box);
      }
      rptr->setBoundary(bbox);

      // The instances within this region.
      for (auto db_inst : db_group->getInsts()) {
        Node* nd = network_->getNode(db_inst);
        if (nd != nullptr) {
          if (true) {
            nd->setGroupId(rptr->getId());
            nd->setGroup(rptr);
            rptr->addCell(nd);
          }
        }
      }
    }
  }
}
void Opendp::adjustNodesOrient()
{
  for (auto& node : network_->getNodes()) {
    if (node->getType() != Node::CELL) {
      continue;
    }
    auto inst = node->getDbInst();
    node->adjustCurrOrient(inst->getOrient());
  }
}
}  // namespace dpl
