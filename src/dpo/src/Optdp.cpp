///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
// All rights reserved.
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

#include "dpo/Optdp.h"

#include <odb/db.h>

#include <boost/format.hpp>
#include <cfloat>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>

#include "dpl/Opendp.h"
#include "odb/util.h"
#include "ord/OpenRoad.hh"  // closestPtInRect
#include "utl/Logger.h"

// My stuff.
#include "architecture.h"
#include "detailed.h"
#include "detailed_manager.h"
#include "legalize_shift.h"
#include "network.h"
#include "orientation.h"
#include "router.h"
#include "symmetry.h"

namespace dpo {

using utl::DPO;

using odb::dbBlock;
using odb::dbBlockage;
using odb::dbBox;
using odb::dbBTerm;
using odb::dbInst;
using odb::dbITerm;
using odb::dbMaster;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbOrientType;
using odb::dbRegion;
using odb::dbRow;
using odb::dbSBox;
using odb::dbSet;
using odb::dbSigType;
using odb::dbSite;
using odb::dbSWire;
using odb::dbTechLayer;
using odb::dbWireType;
using odb::Rect;

////////////////////////////////////////////////////////////////
void Optdp::init(odb::dbDatabase* db, utl::Logger* logger, dpl::Opendp* opendp)
{
  db_ = db;
  logger_ = logger;
  opendp_ = opendp;
}

////////////////////////////////////////////////////////////////
void Optdp::improvePlacement(const int seed,
                             const int max_displacement_x,
                             const int max_displacement_y,
                             const bool disallow_one_site_gaps)
{
  logger_->report("Detailed placement improvement.");

  odb::WireLengthEvaluator eval(db_->getChip()->getBlock());
  const int64_t hpwlBefore = eval.hpwl();

  if (hpwlBefore == 0) {
    logger_->report("Skipping detailed improvement since hpwl is zero.");
    return;
  }

  // Get needed information from DB.
  import();

  // A manager to track cells.
  dpo::DetailedMgr mgr(arch_, network_, routeinfo_);
  mgr.setLogger(logger_);
  // Various settings.
  mgr.setSeed(seed);
  mgr.setMaxDisplacement(max_displacement_x, max_displacement_y);
  mgr.setDisallowOneSiteGaps(disallow_one_site_gaps);

  // Legalization.  Doesn't particularly do much.  It only
  // populates the data structures required for detailed
  // improvement.  If it errors or prints a warning when
  // given a legal placement, that likely means there is
  // a bug in my code somewhere.
  dpo::ShiftLegalizer lg;
  lg.legalize(mgr);

  // Detailed improvement.  Runs through a number of different
  // optimizations aimed at wirelength improvement.  The last
  // call to the random improver can be set to consider things
  // like density, displacement, etc. in addition to wirelength.
  // Everything done through a script string.

  dpo::DetailedParams dtParams;
  dtParams.script_ = "";
  // Maximum independent set matching.
  dtParams.script_ += "mis -p 10 -t 0.005;";
  // Global swaps.
  dtParams.script_ += "gs -p 10 -t 0.005;";
  // Vertical swaps.
  dtParams.script_ += "vs -p 10 -t 0.005;";
  // Small reordering.
  dtParams.script_ += "ro -p 10 -t 0.005;";
  // Random moves and swaps with hpwl as a cost function.  Use
  // random moves and hpwl objective right now.
  dtParams.script_ += "default -p 5 -f 20 -gen rng -obj hpwl -cost (hpwl);";

  if (disallow_one_site_gaps) {
    dtParams.script_ += "disallow_one_site_gaps;";
  }

  // Run the script.
  dpo::Detailed dt(dtParams);
  dt.improve(mgr);

  // Write solution back.
  updateDbInstLocations();

  // Get final hpwl.
  const int64_t hpwlAfter = eval.hpwl();

  // Cleanup.
  delete network_;
  delete arch_;
  delete routeinfo_;

  const double dbu_micron = db_->getTech()->getDbUnitsPerMicron();

  // Statistics.
  logger_->report("Detailed Improvement Results");
  logger_->report("------------------------------------------");
  logger_->report("Original HPWL         {:10.1f} u", hpwlBefore / dbu_micron);
  logger_->report("Final HPWL            {:10.1f} u", hpwlAfter / dbu_micron);
  const double hpwl_delta = (hpwlAfter - hpwlBefore) / (double) hpwlBefore;
  logger_->report("Delta HPWL            {:10.1f} %", hpwl_delta * 100);
  logger_->report("");
}

////////////////////////////////////////////////////////////////
void Optdp::import()
{
  logger_->report("Importing netlist into detailed improver.");

  network_ = new Network;
  arch_ = new Architecture;
  routeinfo_ = new RoutingParams;

  // createLayerMap(); // Does nothing right now.
  // createNdrMap(); // Does nothing right now.
  setupMasterPowers();   // Call prior to network and architecture creation.
  createNetwork();       // Create network; _MUST_ do before architecture.
  createArchitecture();  // Create architecture.
  // createRouteInformation(); // Does nothing right now.
  initPadding();  // Need to do after network creation.
  // setUpNdrRules(); // Does nothing right now.
  setUpPlacementRegions();  // Regions.
}

////////////////////////////////////////////////////////////////
void Optdp::updateDbInstLocations()
{
  for (dbInst* inst : db_->getChip()->getBlock()->getInsts()) {
    if (!inst->getMaster()->isCoreAutoPlaceable() || inst->isFixed()) {
      continue;
    }

    const auto it_n = instMap_.find(inst);
    if (it_n != instMap_.end()) {
      const Node* nd = it_n->second;

      const int y = nd->getBottom();
      const int x = nd->getLeft();

      dbOrientType orient = dbOrientType::R0;
      switch (nd->getCurrOrient()) {
        case Orientation_N:
          orient = dbOrientType::R0;
          break;
        case Orientation_FN:
          orient = dbOrientType::MY;
          break;
        case Orientation_FS:
          orient = dbOrientType::MX;
          break;
        case Orientation_S:
          orient = dbOrientType::R180;
          break;
        default:
          // ?
          break;
      }
      if (inst->getOrient() != orient) {
        inst->setOrient(orient);
      }
      int inst_x, inst_y;
      inst->getLocation(inst_x, inst_y);
      if (x != inst_x || y != inst_y) {
        inst->setLocation(x, y);
      }
    }
  }
}

////////////////////////////////////////////////////////////////
void Optdp::initPadding()
{
  // Grab information from OpenDP.

  // Need to turn off spacing tables and turn on padding.
  arch_->setUseSpacingTable(false);
  arch_->setUsePadding(true);
  arch_->init_edge_type();

  // Create and edge type for each amount of padding.  This
  // can be done by querying OpenDP.
  odb::dbSite* site = nullptr;
  for (auto* row : db_->getChip()->getBlock()->getRows()) {
    if (row->getSite()->getClass() != odb::dbSiteClass::PAD) {
      site = row->getSite();
      break;
    }
  }
  if (site == nullptr) {
    return;
  }
  const int siteWidth = site->getWidth();

  for (dbInst* inst : db_->getChip()->getBlock()->getInsts()) {
    const auto it_n = instMap_.find(inst);
    if (it_n != instMap_.end()) {
      Node* ndi = it_n->second;
      const int leftPadding = opendp_->padLeft(inst);
      const int rightPadding = opendp_->padRight(inst);
      arch_->addCellPadding(
          ndi, leftPadding * siteWidth, rightPadding * siteWidth);
    }
  }
}

////////////////////////////////////////////////////////////////
void Optdp::createLayerMap()
{
  // Relates to pin blockages, etc. Not used rignt now.
  ;
}
////////////////////////////////////////////////////////////////
void Optdp::createNdrMap()
{
  // Relates to pin blockages, etc. Not used rignt now.
  ;
}
////////////////////////////////////////////////////////////////
void Optdp::createRouteInformation()
{
  // Relates to pin blockages, etc. Not used rignt now.
  ;
}
////////////////////////////////////////////////////////////////
void Optdp::setUpNdrRules()
{
  // Relates to pin blockages, etc. Not used rignt now.
  ;
}
////////////////////////////////////////////////////////////////
void Optdp::setupMasterPowers()
{
  // Need to try and figure out which voltages are on the
  // top and bottom of the masters/insts in order to set
  // stuff up for proper row alignment of multi-height
  // cells.  What I do it look at the individual ports
  // (MTerms) and see which ones correspond to POWER and
  // GROUND and then figure out which one is on top and
  // which one is on bottom.  I also record the layers
  // and use that information later when setting up the
  // row powers.
  dbBlock* block = db_->getChip()->getBlock();
  std::vector<dbMaster*> masters;
  block->getMasters(masters);

  for (dbMaster* master : masters) {
    int maxPwr = std::numeric_limits<int>::min();
    int minPwr = std::numeric_limits<int>::max();
    int maxGnd = std::numeric_limits<int>::min();
    int minGnd = std::numeric_limits<int>::max();

    bool isVdd = false;
    bool isGnd = false;
    for (dbMTerm* mterm : master->getMTerms()) {
      if (mterm->getSigType() == dbSigType::POWER) {
        isVdd = true;
        for (dbMPin* mpin : mterm->getMPins()) {
          // Geometry or box?
          const int y = mpin->getBBox().yCenter();
          minPwr = std::min(minPwr, y);
          maxPwr = std::max(maxPwr, y);

          for (dbBox* mbox : mpin->getGeometry()) {
            dbTechLayer* layer = mbox->getTechLayer();
            pwrLayers_.insert(layer);
          }
        }
      } else if (mterm->getSigType() == dbSigType::GROUND) {
        isGnd = true;
        for (dbMPin* mpin : mterm->getMPins()) {
          // Geometry or box?
          const int y = mpin->getBBox().yCenter();
          minGnd = std::min(minGnd, y);
          maxGnd = std::max(maxGnd, y);

          for (dbBox* mbox : mpin->getGeometry()) {
            dbTechLayer* layer = mbox->getTechLayer();
            gndLayers_.insert(layer);
          }
        }
      }
    }
    int topPwr = Architecture::Row::Power_UNK;
    int botPwr = Architecture::Row::Power_UNK;
    if (isVdd && isGnd) {
      topPwr = (maxPwr > maxGnd) ? Architecture::Row::Power_VDD
                                 : Architecture::Row::Power_VSS;
      botPwr = (minPwr < minGnd) ? Architecture::Row::Power_VDD
                                 : Architecture::Row::Power_VSS;
    }

    masterPwrs_[master] = {topPwr, botPwr};
  }
}

////////////////////////////////////////////////////////////////
void Optdp::createNetwork()
{
  dbBlock* block = db_->getChip()->getBlock();

  pwrLayers_.clear();
  gndLayers_.clear();

  // I allocate things statically, so I need to do some counting.

  auto block_insts = block->getInsts();
  std::vector<dbInst*> insts(block_insts.begin(), block_insts.end());
  std::stable_sort(insts.begin(), insts.end(), [](dbInst* a, dbInst* b) {
    return a->getName() < b->getName();
  });

  int nNodes = 0;
  for (dbInst* inst : insts) {
    // Skip instances which are not placeable.
    if (!inst->getMaster()->isCoreAutoPlaceable()) {
      continue;
    }
    ++nNodes;
  }

  dbSet<dbNet> nets = block->getNets();
  int nEdges = 0;
  int nPins = 0;
  for (dbNet* net : nets) {
    // Skip supply nets.
    if (net->getSigType().isSupply()) {
      continue;
    }
    ++nEdges;
    // Only count pins in insts if they considered
    // placeable since these are the only insts
    // that will be in our network.
    for (dbITerm* iTerm : net->getITerms()) {
      if (iTerm->getInst()->getMaster()->isCoreAutoPlaceable()) {
        ++nPins;
      }
    }
    // Count pins on terminals.
    nPins += net->getBTerms().size();
  }

  dbSet<dbBTerm> bterms = block->getBTerms();
  int nTerminals = 0;
  for (dbBTerm* bterm : bterms) {
    // Skip supply nets.
    dbNet* net = bterm->getNet();
    if (!net || net->getSigType().isSupply()) {
      continue;
    }
    ++nTerminals;
  }

  int nBlockages = 0;
  for (dbBlockage* blockage : block->getBlockages()) {
    if (!blockage->isSoft()) {
      network_->createAndAddBlockage(blockage->getBBox()->getBox());
      ++nBlockages;
    }
  }

  logger_->info(DPO,
                100,
                "Creating network with {:d} cells, {:d} terminals, "
                "{:d} edges, {:d} pins, and {:d} blockages.",
                nNodes,
                nTerminals,
                nEdges,
                nPins,
                nBlockages);

  // Create and allocate the nodes.  I require nodes for
  // placeable instances as well as terminals.
  for (int i = 0; i < nNodes + nTerminals; i++) {
    network_->createAndAddNode();
  }
  for (int i = 0; i < nEdges; i++) {
    network_->createAndAddEdge();
  }

  // Return instances to a north orientation.  This makes
  // importing easier as I think it ensures all the pins,
  // etc. will be where I expect them to be.
  for (dbInst* inst : insts) {
    if (!inst->getMaster()->isCoreAutoPlaceable() || inst->isFixed()) {
      continue;
    }
    inst->setLocationOrient(dbOrientType::R0);  // Preserve lower-left.
  }

  // Populate nodes.
  int n = 0;
  for (dbInst* inst : insts) {
    if (!inst->getMaster()->isCoreAutoPlaceable()) {
      continue;
    }

    Node* ndi = network_->getNode(n);
    instMap_[inst] = ndi;

    // Name of inst.
    network_->setNodeName(n, inst->getName().c_str());

    // Fill in data.
    ndi->setType(Node::CELL);
    ndi->setId(n);
    ndi->setFixed(inst->isFixed() ? Node::FIXED_XY : Node::NOT_FIXED);

    // Determine allowed orientations.  Current orientation
    // is N, since we reset everything to this orientation.
    unsigned orientations = Orientation_N;
    if (inst->getMaster()->getSymmetryX()
        && inst->getMaster()->getSymmetryY()) {
      orientations |= Orientation_FN;
      orientations |= Orientation_FS;
      orientations |= Orientation_S;
    } else if (inst->getMaster()->getSymmetryX()) {
      orientations |= Orientation_FS;
    } else if (inst->getMaster()->getSymmetryY()) {
      orientations |= Orientation_FN;
    }
    // else...  Account for R90?
    ndi->setAvailOrient(orientations);
    ndi->setCurrOrient(Orientation_N);
    ndi->setHeight(inst->getMaster()->getHeight());
    ndi->setWidth(inst->getMaster()->getWidth());

    ndi->setOrigLeft(inst->getBBox()->xMin());
    ndi->setOrigBottom(inst->getBBox()->yMin());
    ndi->setLeft(inst->getBBox()->xMin());
    ndi->setBottom(inst->getBBox()->yMin());

    // Won't use edge types.
    ndi->setRightEdgeType(EDGETYPE_DEFAULT);
    ndi->setLeftEdgeType(EDGETYPE_DEFAULT);

    // Set the top and bottom power.
    auto it_m = masterPwrs_.find(inst->getMaster());
    if (masterPwrs_.end() == it_m) {
      ndi->setBottomPower(Architecture::Row::Power_UNK);
      ndi->setTopPower(Architecture::Row::Power_UNK);
    } else {
      ndi->setBottomPower(it_m->second.second);
      ndi->setTopPower(it_m->second.first);
    }

    // Regions setup later!
    ndi->setRegionId(0);

    ++n;  // Next node.
  }
  for (dbBTerm* bterm : bterms) {
    dbNet* net = bterm->getNet();
    if (!net || net->getSigType().isSupply()) {
      continue;
    }
    Node* ndi = network_->getNode(n);
    termMap_[bterm] = ndi;

    // Name of terminal.
    network_->setNodeName(n, bterm->getName().c_str());

    // Fill in data.
    ndi->setId(n);
    ndi->setType(Node::TERMINAL);
    ndi->setFixed(Node::FIXED_XY);
    ndi->setAvailOrient(Orientation_N);
    ndi->setCurrOrient(Orientation_N);

    int ww = (bterm->getBBox().xMax() - bterm->getBBox().xMin());
    int hh = (bterm->getBBox().yMax() - bterm->getBBox().yMax());

    ndi->setHeight(hh);
    ndi->setWidth(ww);

    ndi->setOrigLeft(bterm->getBBox().xMin());
    ndi->setOrigBottom(bterm->getBBox().yMin());
    ndi->setLeft(bterm->getBBox().xMin());
    ndi->setBottom(bterm->getBBox().yMin());

    // Not relevant for terminal.
    ndi->setRightEdgeType(EDGETYPE_DEFAULT);
    ndi->setLeftEdgeType(EDGETYPE_DEFAULT);

    // Not relevant for terminal.
    ndi->setBottomPower(Architecture::Row::Power_UNK);
    ndi->setTopPower(Architecture::Row::Power_UNK);

    // Not relevant for terminal.
    ndi->setRegionId(0);  // Set in another routine.

    ++n;  // Next node.
  }
  if (n != nNodes + nTerminals) {
    logger_->error(DPO,
                   101,
                   "Unexpected total node count.  Expected {:d}, but got {:d}",
                   (nNodes + nTerminals),
                   n);
  }

  // Populate edges and pins.
  int e = 0;
  int p = 0;
  for (dbNet* net : nets) {
    // Skip supply nets.
    if (net->getSigType().isSupply()) {
      continue;
    }

    Edge* edi = network_->getEdge(e);
    edi->setId(e);
    netMap_[net] = edi;

    // Name of edge.
    network_->setEdgeName(e, net->getName().c_str());

    for (dbITerm* iTerm : net->getITerms()) {
      if (!iTerm->getInst()->getMaster()->isCoreAutoPlaceable()) {
        continue;
      }

      auto it_n = instMap_.find(iTerm->getInst());
      if (instMap_.end() != it_n) {
        n = it_n->second->getId();  // The node id.

        if (network_->getNode(n)->getId() != n
            || network_->getEdge(e)->getId() != e) {
          logger_->error(
              DPO, 102, "Improper node indexing while connecting pins.");
        }

        Pin* ptr = network_->createAndAddPin(network_->getNode(n),
                                             network_->getEdge(e));

        // Pin offset.
        dbMTerm* mTerm = iTerm->getMTerm();
        dbMaster* master = mTerm->getMaster();
        // Due to old bookshelf, my offsets are from the
        // center of the cell whereas in DEF, it's from
        // the bottom corner.
        double ww = (mTerm->getBBox().xMax() - mTerm->getBBox().xMin());
        double hh = (mTerm->getBBox().yMax() - mTerm->getBBox().yMax());
        double xx = (mTerm->getBBox().xMax() + mTerm->getBBox().xMin()) * 0.5;
        double yy = (mTerm->getBBox().yMax() + mTerm->getBBox().yMax()) * 0.5;
        double dx = xx - ((double) master->getWidth() / 2.);
        double dy = yy - ((double) master->getHeight() / 2.);

        ptr->setOffsetX(dx);
        ptr->setOffsetY(dy);
        ptr->setPinHeight(hh);
        ptr->setPinWidth(ww);
        ptr->setPinLayer(0);  // Set to zero since not currently used.

        ++p;  // next pin.
      } else {
        logger_->error(
            DPO,
            103,
            "Could not find node for instance while connecting pins.");
      }
    }
    for (dbBTerm* bTerm : net->getBTerms()) {
      auto it_p = termMap_.find(bTerm);
      if (termMap_.end() != it_p) {
        n = it_p->second->getId();  // The node id.

        if (network_->getNode(n)->getId() != n
            || network_->getEdge(e)->getId() != e) {
          logger_->error(
              DPO, 104, "Improper terminal indexing while connecting pins.");
        }

        Pin* ptr = network_->createAndAddPin(network_->getNode(n),
                                             network_->getEdge(e));

        // These don't need an offset.
        ptr->setOffsetX(0.0);
        ptr->setOffsetY(0.0);
        ptr->setPinHeight(0.0);
        ptr->setPinWidth(0.0);
        ptr->setPinLayer(0);  // Set to zero since not currently used.

        ++p;  // next pin.
      } else {
        logger_->error(
            DPO,
            105,
            "Could not find node for terminal while connecting pins.");
      }
    }

    ++e;  // next edge.
  }
  if (e != nEdges) {
    logger_->error(DPO,
                   106,
                   "Unexpected total edge count.  Expected {:d}, but got {:d}",
                   nEdges,
                   e);
  }
  if (p != nPins) {
    logger_->error(DPO,
                   107,
                   "Unexpected total pin count.  Expected {:d}, but got {:d}",
                   nPins,
                   p);
  }

  logger_->info(DPO,
                109,
                "Network stats: inst {}, edges {}, pins {}",
                network_->getNumNodes(),
                network_->getNumEdges(),
                network_->getNumPins());
}
////////////////////////////////////////////////////////////////
void Optdp::createArchitecture()
{
  dbBlock* block = db_->getChip()->getBlock();

  odb::Rect dieRect = block->getDieArea();

  auto min_row_height = std::numeric_limits<int>::max();
  for (dbRow* row : block->getRows()) {
    min_row_height = std::min(min_row_height, row->getSite()->getHeight());
  }

  std::map<int, std::unordered_set<std::string>> skip_list;

  for (dbRow* row : block->getRows()) {
    if (row->getSite()->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    if (row->getDirection() != odb::dbRowDir::HORIZONTAL) {
      // error.
      continue;
    }
    dbSite* site = row->getSite();
    if (site->getHeight() > min_row_height) {
      skip_list[site->getHeight()].insert(site->getName());
      continue;
    }
    odb::Point origin = row->getOrigin();

    Architecture::Row* archRow = arch_->createAndAddRow();

    archRow->setSubRowOrigin(origin.x());
    archRow->setBottom(origin.y());
    archRow->setSiteSpacing(row->getSpacing());
    archRow->setNumSites(row->getSiteCount());
    archRow->setSiteWidth(site->getWidth());
    archRow->setHeight(site->getHeight());

    // Set defaults.  Top and bottom power is set below.
    archRow->setBottomPower(Architecture::Row::Power_UNK);
    archRow->setTopPower(Architecture::Row::Power_UNK);

    // Symmetry.  From the site.
    unsigned symmetry = 0x00000000;
    if (site->getSymmetryX()) {
      symmetry |= dpo::Symmetry_X;
    }
    if (site->getSymmetryY()) {
      symmetry |= dpo::Symmetry_Y;
    }
    if (site->getSymmetryR90()) {
      symmetry |= dpo::Symmetry_ROT90;
    }
    archRow->setSymmetry(symmetry);

    // Orientation.  From the row.
    unsigned orient = dbToDpoOrient(row->getOrient());
    archRow->setOrient(orient);
  }
  for (const auto& skip : skip_list) {
    std::string skip_string = "[";
    int i = 0;
    for (const auto& skipped_site : skip.second) {
      skip_string += skipped_site + ",]"[i == skip.second.size() - 1];
      ++i;
    }
    logger_->warn(DPO,
                  108,
                  "Skipping all the rows with sites {} as their height is {} "
                  "and the single-height is {}.",
                  skip_string,
                  skip.first,
                  min_row_height);
  }
  // Get surrounding box.
  {
    int xmin = std::numeric_limits<int>::max();
    int xmax = std::numeric_limits<int>::lowest();
    int ymin = std::numeric_limits<int>::max();
    int ymax = std::numeric_limits<int>::lowest();
    for (int r = 0; r < arch_->getNumRows(); r++) {
      Architecture::Row* row = arch_->getRow(r);

      xmin = std::min(xmin, row->getLeft());
      xmax = std::max(xmax, row->getRight());
      ymin = std::min(ymin, row->getBottom());
      ymax = std::max(ymax, row->getTop());
    }
    if (xmin != dieRect.xMin() || xmax != dieRect.xMax()) {
      xmin = dieRect.xMin();
      xmax = dieRect.xMax();
    }
    arch_->setMinX(xmin);
    arch_->setMaxX(xmax);
    arch_->setMinY(ymin);
    arch_->setMaxY(ymax);
  }

  for (int r = 0; r < arch_->getNumRows(); r++) {
    int numSites = arch_->getRow(r)->getNumSites();
    int originX = arch_->getRow(r)->getLeft();
    int siteSpacing = arch_->getRow(r)->getSiteSpacing();
    int siteWidth = arch_->getRow(r)->getSiteWidth();

    if (originX < arch_->getMinX()) {
      originX = (int) std::ceil(arch_->getMinX());
      if (arch_->getRow(r)->getLeft() != originX) {
        arch_->getRow(r)->setSubRowOrigin(originX);
      }
    }
    if (originX + numSites * siteSpacing + siteWidth > arch_->getMaxX()) {
      numSites = (arch_->getMaxX() - siteWidth - originX) / siteSpacing;
      if (arch_->getRow(r)->getNumSites() != numSites) {
        arch_->getRow(r)->setNumSites(numSites);
      }
    }
  }

  // Need the power running across the bottom and top of each
  // row.  I think the way to do this is to look for power
  // and ground nets and then look at the special wires.
  // Not sure, though, of the best way to pick those that
  // actually touch the cells (i.e., which layer?).
  for (dbNet* net : block->getNets()) {
    if (!net->isSpecial()) {
      continue;
    }
    if (!(net->getSigType() == dbSigType::POWER
          || net->getSigType() == dbSigType::GROUND)) {
      continue;
    }
    int pwr = (net->getSigType() == dbSigType::POWER)
                  ? Architecture::Row::Power_VDD
                  : Architecture::Row::Power_VSS;
    for (dbSWire* swire : net->getSWires()) {
      if (swire->getWireType() != dbWireType::ROUTED) {
        continue;
      }

      for (dbSBox* sbox : swire->getWires()) {
        if (sbox->getDirection() != dbSBox::HORIZONTAL) {
          continue;
        }
        if (sbox->isVia()) {
          continue;
        }
        dbTechLayer* layer = sbox->getTechLayer();
        if (pwr == Architecture::Row::Power_VDD) {
          if (pwrLayers_.end() == pwrLayers_.find(layer)) {
            continue;
          }
        } else if (pwr == Architecture::Row::Power_VSS) {
          if (gndLayers_.end() == gndLayers_.find(layer)) {
            continue;
          }
        }

        Rect rect = sbox->getBox();
        for (size_t r = 0; r < arch_->getNumRows(); r++) {
          int yb = arch_->getRow(r)->getBottom();
          int yt = arch_->getRow(r)->getTop();

          if (yb >= rect.yMin() && yb <= rect.yMax()) {
            arch_->getRow(r)->setBottomPower(pwr);
          }
          if (yt >= rect.yMin() && yt <= rect.yMax()) {
            arch_->getRow(r)->setTopPower(pwr);
          }
        }
      }
    }
  }
  arch_->postProcess(network_);
}
////////////////////////////////////////////////////////////////
void Optdp::setUpPlacementRegions()
{
  int xmin = arch_->getMinX();
  int xmax = arch_->getMaxX();
  int ymin = arch_->getMinY();
  int ymax = arch_->getMaxY();

  dbBlock* block = db_->getChip()->getBlock();

  std::unordered_map<odb::dbInst*, Node*>::iterator it_n;
  Architecture::Region* rptr = nullptr;
  int count = 0;
  Rectangle_i tempRect;

  // Default region.
  rptr = arch_->createAndAddRegion();
  rptr->setId(count);
  ++count;

  tempRect.set_xmin(xmin);
  tempRect.set_xmax(xmax);
  tempRect.set_ymin(ymin);
  tempRect.set_ymax(ymax);
  rptr->addRect(tempRect);

  rptr->setMinX(xmin);
  rptr->setMaxX(xmax);
  rptr->setMinY(ymin);
  rptr->setMaxY(ymax);

  auto db_groups = block->getGroups();
  for (auto db_group : db_groups) {
    dbRegion* parent = db_group->getRegion();
    if (parent) {
      rptr = arch_->createAndAddRegion();
      rptr->setId(count);
      ++count;
      auto boundaries = parent->getBoundaries();
      for (dbBox* boundary : boundaries) {
        Rect box = boundary->getBox();

        xmin = std::max(arch_->getMinX(), box.xMin());
        xmax = std::min(arch_->getMaxX(), box.xMax());
        ymin = std::max(arch_->getMinY(), box.yMin());
        ymax = std::min(arch_->getMaxY(), box.yMax());

        tempRect.set_xmin(xmin);
        tempRect.set_xmax(xmax);
        tempRect.set_ymin(ymin);
        tempRect.set_ymax(ymax);
        rptr->addRect(tempRect);

        rptr->setMinX(std::min(xmin, rptr->getMinX()));
        rptr->setMaxX(std::max(xmax, rptr->getMaxX()));
        rptr->setMinY(std::min(ymin, rptr->getMinY()));
        rptr->setMaxY(std::max(ymax, rptr->getMaxY()));
      }

      // The instances within this region.
      for (auto db_inst : db_group->getInsts()) {
        it_n = instMap_.find(db_inst);
        if (instMap_.end() != it_n) {
          Node* nd = it_n->second;
          if (nd->getRegionId() == 0) {
            nd->setRegionId(rptr->getId());
          }
        }
      }
    }
  }
  logger_->info(DPO, 110, "Number of regions is {:d}", arch_->getNumRegions());
}
////////////////////////////////////////////////////////////////
unsigned Optdp::dbToDpoOrient(const dbOrientType& dbOrient)
{
  unsigned orient = dpo::Orientation_N;
  switch (dbOrient) {
    case dbOrientType::R0:
      orient = dpo::Orientation_N;
      break;
    case dbOrientType::MY:
      orient = dpo::Orientation_FN;
      break;
    case dbOrientType::MX:
      orient = dpo::Orientation_FS;
      break;
    case dbOrientType::R180:
      orient = dpo::Orientation_S;
      break;
    case dbOrientType::R90:
      orient = dpo::Orientation_E;
      break;
    case dbOrientType::MXR90:
      orient = dpo::Orientation_FE;
      break;
    case dbOrientType::R270:
      orient = dpo::Orientation_W;
      break;
    case dbOrientType::MYR90:
      orient = dpo::Orientation_FW;
      break;
    default:
      break;
  }
  return orient;
}

}  // namespace dpo
