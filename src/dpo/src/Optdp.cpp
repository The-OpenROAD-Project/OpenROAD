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
#include "ord/OpenRoad.hh"  // closestPtInRect
#include "utl/Logger.h"

// My stuff.
#include "architecture.h"
#include "detailed.h"
#include "detailed_manager.h"
#include "legalize_shift.h"
#include "network.h"
#include "router.h"

namespace dpo {

using std::round;
using std::string;

using utl::DPO;

using odb::dbBlock;
using odb::dbBox;
using odb::dbBPin;
using odb::dbBTerm;
using odb::dbITerm;
using odb::dbMasterType;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbPlacementStatus;
using odb::dbRegion;
using odb::dbSBox;
using odb::dbSet;
using odb::dbSigType;
using odb::dbSWire;
using odb::dbTechLayer;
using odb::dbWireType;
using odb::Rect;

static bool swapWidthHeight(dbOrientType orient);

////////////////////////////////////////////////////////////////
Optdp::Optdp() : nw_(nullptr), arch_(nullptr), rt_(nullptr) {}

////////////////////////////////////////////////////////////////
Optdp::~Optdp() {}

////////////////////////////////////////////////////////////////
void Optdp::init(ord::OpenRoad* openroad) {
  openroad_ = openroad;
  db_ = openroad->getDb();
  logger_ = openroad->getLogger();
}

////////////////////////////////////////////////////////////////
void Optdp::improvePlacement() {
  logger_->report("Detailed placement improvement.");

  dpl::Opendp* opendp = openroad_->getOpendp();
  hpwlBefore_ = opendp->hpwl();

  import();

  // A manager to track cells.
  dpo::DetailedMgr mgr(arch_, nw_, rt_);
  mgr.setLogger(logger_);

  // Legalization.  Doesn't particularly do much.  It only
  // populates the data structures required for detailed
  // improvement.  If it errors or prints a warning when
  // given a legal placement, that likely means there is
  // a bug in my code somewhere.
  dpo::ShiftLegalizerParams lgParams;
  dpo::ShiftLegalizer lg(lgParams);
  lg.legalize(mgr);

  // Detailed improvement.  Runs through a number of different
  // optimizations aimed at wirelength improvement.  The last
  // call to the random improver can be set to consider things
  // like density, displacement, etc. in addition to wirelength.

  dpo::DetailedParams dtParams;
  dtParams.m_script = "";
  // Maximum independent set matching.
  dtParams.m_script += "mis -p 10 -t 0.005";
  dtParams.m_script += ";";
  // Global swaps - commented out right now.  caused padding violation???
  //dtParams.m_script += "gs -p 10 -t 0.005";
  //dtParams.m_script += ";";
  // Vertical swaps - commented out right now.  caused padding violation???
  //dtParams.m_script += "vs -p 10 -t 0.005";
  //dtParams.m_script += ";";
  // Small reordering.
  dtParams.m_script += "ro -p 10 -t 0.005";
  dtParams.m_script += ";";
  // Random moves and swaps with hpwl as a cost function.
  // Commented out moves generated with global and vertical swaps 
  // since they seem to be causing padding violations.
  dtParams.m_script +=
       "default -p 5 -f 20 -gen rng -obj hpwl -cost (hpwl)";
  dpo::Detailed dt(dtParams);
  dt.improve(mgr);

  // Write solution back.
  updateDbInstLocations();

  hpwlAfter_ = opendp->hpwl();

  double dbu_micron = db_->getTech()->getDbUnitsPerMicron();

  // Statistics.
  logger_->report("Detailed Improvement Results");
  logger_->report("------------------------------------------");
  logger_->report("Original HPWL         {:10.1f} u", (hpwlBefore_/dbu_micron));
  logger_->report("Final HPWL            {:10.1f} u", (hpwlAfter_/dbu_micron));
  double hpwl_delta = (hpwlBefore_ == 0.0)
    ? 0.0
    : ((double)(hpwlAfter_ - hpwlBefore_) / (double)hpwlBefore_) * 100.;
  logger_->report("Delta HPWL            {:10.1f} %", hpwl_delta);
  logger_->report("");

  // Cleanup.
  if (nw_) delete nw_;
  if (arch_) delete arch_;
  if (rt_) delete rt_;
  nw_ = 0;
  arch_ = 0;
  rt_ = 0;
}
////////////////////////////////////////////////////////////////
void Optdp::import() {
  logger_->report("Importing netlist into detailed improver.");

  nw_ = new Network;
  arch_ = new Architecture;
  rt_ = new RoutingParams;

  initEdgeTypes(); // Does nothing right now.
  initCellSpacingTable(); // Does nothing right now.
  createLayerMap(); // Does nothing right now.
  createNdrMap(); // Does nothing right now.
  setupMasterPowers();  // Call prior to network and architecture creation.
  createNetwork(); // Create network; _MUST_ do before architecture.
  createArchitecture(); // Create architecture.
  createRouteGrid(); // Bad name.  Holds routing info for placement, but not used right now.
  initPadding(); // Need to do after network creation.
  setUpNdrRules(); // Does nothing right now.
  setUpPlacementRegions(); // Regions.
}
////////////////////////////////////////////////////////////////
void Optdp::updateDbInstLocations() {
  std::unordered_map<odb::dbInst*, Node*>::iterator it_n;
  dbBlock* block = db_->getChip()->getBlock();
  dbSet<dbInst> insts = block->getInsts();
  for (dbInst* inst : insts) {
    auto type = inst->getMaster()->getType();
    if (!type.isCore() && !type.isBlock()) {
      continue;
    }
    it_n = instMap_.find(inst);
    if (instMap_.end() != it_n) {
      Node* nd = it_n->second;
      // XXX: Need to figure out orientation.
      int lx = (int)(nd->getX() - 0.5 * nd->getWidth());
      int yb = (int)(nd->getY() - 0.5 * nd->getHeight());
      inst->setLocation(lx, yb);

      double xmin = nd->getX() - 0.5 * nd->getWidth();
      double xmax = nd->getX() + 0.5 * nd->getWidth();
      double ymin = nd->getY() - 0.5 * nd->getHeight();
      double ymax = nd->getY() + 0.5 * nd->getHeight();
    }
  }
}
////////////////////////////////////////////////////////////////
void Optdp::initEdgeTypes() {
  // Do nothing.  Use padding instead.
  ;
}
////////////////////////////////////////////////////////////////
void Optdp::initCellSpacingTable() {
  // Do nothing.  Use padding instead.
  ;
}
////////////////////////////////////////////////////////////////
void Optdp::initPadding() {
  // Grab information from OpenDP.
  dpl::Opendp* opendp = openroad_->getOpendp();

  // Need to turn off spacing tables and turn on padding.
  arch_->setUseSpacingTable(false);
  arch_->setUsePadding(true);
  arch_->init_edge_type();


  // Create and edge type for each amount of padding.  This
  // can be done by querying OpenDP.
  dbSet<dbRow> rows = db_->getChip()->getBlock()->getRows();
  if (rows.empty()) {
    return;
  }
  int siteWidth = (*rows.begin())->getSite()->getWidth();
  std::unordered_map<odb::dbInst*, Node*>::iterator it_n;

  dbSet<dbInst> insts = db_->getChip()->getBlock()->getInsts();
  for (dbInst* inst : insts) {
    it_n = instMap_.find(inst);
    if (instMap_.end() != it_n) {
      Node* ndi = it_n->second;
      int leftPadding = opendp->padLeft(inst);
      int rightPadding = opendp->padRight(inst);
      arch_->addCellPadding(ndi, leftPadding * siteWidth,
                            rightPadding * siteWidth);
    }
  }
}
////////////////////////////////////////////////////////////////
void Optdp::createLayerMap() {
  // Relates to pin blockages, etc. Maybe not needed right now.
  ;
}
////////////////////////////////////////////////////////////////
void Optdp::createNdrMap() {
  // Relates to pin blockages, etc. Maybe not needed right now.
  ;
}
////////////////////////////////////////////////////////////////
void Optdp::setupMasterPowers() {
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

  for (size_t i = 0; i < masters.size(); i++) {
    dbMaster* master = masters[i];

    double maxPwr = -std::numeric_limits<double>::max();
    double minPwr = std::numeric_limits<double>::max();
    double maxGnd = -std::numeric_limits<double>::max();
    double minGnd = std::numeric_limits<double>::max();

    bool isVdd = false;
    bool isGnd = false;
    for (dbMTerm* mterm : master->getMTerms()) {
      // XXX: Do I need to look at ports, or would the surrounding
      // box be enough?
      if (mterm->getSigType() == dbSigType::POWER) {
        isVdd = true;
        for (dbMPin* mpin : mterm->getMPins()) {
          // Geometry or box?
          double y = 0.5 * (mpin->getBBox().yMin() + mpin->getBBox().yMax());
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
          double y = 0.5 * (mpin->getBBox().yMin() + mpin->getBBox().yMax());
          minGnd = std::min(minGnd, y);
          maxGnd = std::max(maxGnd, y);

          for (dbBox* mbox : mpin->getGeometry()) {
            dbTechLayer* layer = mbox->getTechLayer();
            gndLayers_.insert(layer);
          }
        }
      }
    }
    int topPwr = RowPower_UNK;
    int botPwr = RowPower_UNK;
    if (isVdd && isGnd) {
      topPwr = (maxPwr > maxGnd) ? RowPower_VDD : RowPower_VSS;
      botPwr = (minPwr < minGnd) ? RowPower_VDD : RowPower_VSS;
    }

    masterPwrs_[master] = std::make_pair(topPwr, botPwr);
  }
}

////////////////////////////////////////////////////////////////
void Optdp::createNetwork() {
  std::unordered_map<odb::dbInst*, Node*>::iterator it_n;
  std::unordered_map<odb::dbNet*, Edge*>::iterator it_e;
  std::unordered_map<odb::dbBTerm*, Node*>::iterator it_p;
  std::unordered_map<dbMaster*, std::pair<int, int> >::iterator it_m;

  dbBlock* block = db_->getChip()->getBlock();

  pwrLayers_.clear();
  gndLayers_.clear();

  // I allocate things statically, so I need to do some counting.

  dbSet<dbInst> insts = block->getInsts();
  dbSet<dbNet> nets = block->getNets();
  dbSet<dbBTerm> bterms = block->getBTerms();

  int errors = 0;

  // Number of this and that.
  int nTerminals = bterms.size();
  int nNodes = 0;
  int nEdges = 0;
  int nPins = 0;
  for (dbInst* inst : insts) {
    dbMasterType type = inst->getMaster()->getType();
    if (!type.isCore() && !type.isBlock()) {
      continue;
    }
    ++nNodes;
  }

  for (dbNet* net : nets) {
    dbSigType netType = net->getSigType();
    // Should probably skip global nets.
    ++nEdges;

    nPins += net->getITerms().size();
    nPins += net->getBTerms().size();
  }

  logger_->info(DPO, 100, "Created network with {:d} cells, {:d} terminals, "
                "{:d} edges and {:d} pins.",
                nNodes, nTerminals, nEdges, nPins);

  // Create and allocate the nodes.  I require nodes for
  // placeable instances as well as terminals.
  nw_->resizeNodes(nNodes + nTerminals);
  nw_->resizeEdges(nEdges);
  nw_->resizePins(nPins);
  nw_->m_shapes.resize(nNodes + nTerminals);
  for (size_t i = 0; i < nw_->m_shapes.size(); i++) {
    nw_->m_shapes[i] = std::vector<Node*>();
  }

  // XXX: NEED TO DO BETTER WITH ORIENTATIONS AND SYMMETRY...

  // Return instances to a north orientation.  This makes
  // importing easier.
  for (dbInst* inst : insts) {
    dbMasterType type = inst->getMaster()->getType();
    if (!type.isCore() && !type.isBlock()) {
      continue;
    }
    inst->setLocationOrient(dbOrientType::R0);
  }

  // Populate nodes.
  int n = 0;
  for (dbInst* inst : insts) {
    auto type = inst->getMaster()->getType();
    if (!type.isCore() && !type.isBlock()) {
      continue;
    }

    Node* ndi = nw_->getNode(n);
    instMap_[inst] = ndi;

    double xc = inst->getBBox()->xMin() + 0.5 * inst->getMaster()->getWidth();
    double yc = inst->getBBox()->yMin() + 0.5 * inst->getMaster()->getHeight();

    // Name of inst.
    nw_->setNodeName(n, inst->getName().c_str());

    // Fill in data.
    ndi->setType(NodeType_CELL);
    ndi->setId(n);
    ndi->setFixed(inst->isFixed() ? NodeFixed_FIXED_XY : NodeFixed_NOT_FIXED);
    ndi->setAttributes(NodeAttributes_EMPTY);

    // XXX: Once again, need to think more about orientiation.  I
    // reset everything to R0 (my Orientation_N).  Should also
    // think about R90, etc.
    unsigned orientations = Orientation_N;
    if (inst->getMaster()->getSymmetryX() &&
        inst->getMaster()->getSymmetryY()) {
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

    ndi->setOrigX(xc);
    ndi->setOrigY(yc);
    ndi->setX(xc);
    ndi->setY(yc);

    // Won't use edge types.
    ndi->setRightEdgeType(EDGETYPE_DEFAULT);
    ndi->setLeftEdgeType(EDGETYPE_DEFAULT);

    // Set the top and bottom power.
    it_m = masterPwrs_.find(inst->getMaster());
    if (masterPwrs_.end() == it_m) {
      ndi->setBottomPower(RowPower_UNK);
      ndi->setTopPower(RowPower_UNK);
    } else {
      ndi->setBottomPower(it_m->second.second);
      ndi->setTopPower(it_m->second.first);
    }

    // Regions setup later!
    ndi->setRegionId(0);

    ++n;  // Next node.
  }
  for (dbBTerm* bterm : bterms) {
    Node* ndi = nw_->getNode(n);
    termMap_[bterm] = ndi;

    // Name of terminal.
    nw_->setNodeName(n, bterm->getName().c_str());

    // Fill in data.
    ndi->setId(n);
    ndi->setType(NodeType_TERMINAL_NI);
    ndi->setFixed(NodeFixed_FIXED_XY);
    ndi->setAttributes(NodeAttributes_EMPTY);
    ndi->setAvailOrient(Orientation_N);
    ndi->setCurrOrient(Orientation_N);

    double ww = (bterm->getBBox().xMax() - bterm->getBBox().xMin());
    double hh = (bterm->getBBox().yMax() - bterm->getBBox().yMax());
    double xx = (bterm->getBBox().xMax() + bterm->getBBox().xMin()) * 0.5;
    double yy = (bterm->getBBox().yMax() + bterm->getBBox().yMax()) * 0.5;

    ndi->setHeight(hh);
    ndi->setWidth(ww);

    ndi->setOrigX(xx);
    ndi->setOrigY(yy);
    ndi->setX(xx);
    ndi->setY(yy);

    // Not relevant for terminal.
    ndi->setRightEdgeType(EDGETYPE_DEFAULT);
    ndi->setLeftEdgeType(EDGETYPE_DEFAULT);

    // Not relevant for terminal.
    ndi->setBottomPower(RowPower_UNK);
    ndi->setTopPower(RowPower_UNK);

    // Not relevant for terminal.
    ndi->setRegionId(0);  // Set in another routine.

    ++n;  // Next node.
  }
  if (n != nNodes + nTerminals) {
    ++errors;
  }

  // Populate edges and pins.
  int e = 0;
  int p = 0;
  for (dbNet* net : nets) {
    // Skip globals and pre-routes?
    // dbSigType netType = net->getSigType();

    Edge* edi = nw_->getEdge(e);
    netMap_[net] = edi;

    // Name of edge.
    nw_->setEdgeName(e, net->getName().c_str());

    edi->setId(e);

    for (dbITerm* iTerm : net->getITerms()) {
      it_n = instMap_.find(iTerm->getInst());
      if (instMap_.end() != it_n) {
        n = it_n->second->getId();  // The node id.

        Pin* ptr = &(nw_->m_pins[p]);

        ptr->setId(p);
        ptr->setEdgeId(e);
        ptr->setNodeId(n);

        // Pin offset.  Correct?
        dbMTerm* mTerm = iTerm->getMTerm();
        dbMaster* master = mTerm->getMaster();
        // Due to old bookshelf, my offsets are from the
        // center of the cell whereas in DEF, it's from
        // the bottom corner.
        double ww = (mTerm->getBBox().xMax() - mTerm->getBBox().xMin());
        double hh = (mTerm->getBBox().yMax() - mTerm->getBBox().yMax());
        double xx = (mTerm->getBBox().xMax() + mTerm->getBBox().xMin()) * 0.5;
        double yy = (mTerm->getBBox().yMax() + mTerm->getBBox().yMax()) * 0.5;
        double dx = xx - ((double)master->getWidth() / 2.);
        double dy = yy - ((double)master->getHeight() / 2.);

        ptr->setOffsetX(dx);
        ptr->setOffsetY(dy);
        ptr->setPinHeight(hh);
        ptr->setPinWidth(ww);

        // XXX: Not correct, but okay for now!
        ptr->setPinLayer(0);

        ++p;  // next pin.
      } else {
        ++errors;
      }
    }
    for (dbBTerm* bTerm : net->getBTerms()) {
      it_p = termMap_.find(bTerm);
      if (termMap_.end() != it_p) {
        n = it_p->second->getId();  // The node id.

        Pin* ptr = &(nw_->m_pins[p]);

        ptr->setId(p);
        ptr->setEdgeId(e);
        ptr->setNodeId(n);

        // These don't need an offset.
        ptr->setOffsetX(0.0);
        ptr->setOffsetY(0.0);
        ptr->setPinHeight(0.0);
        ptr->setPinWidth(0.0);

        // XXX: Not correct, but okay for now!
        ptr->setPinLayer(0);

        ++p;  // next pin.
      } else {
        ++errors;
      }
    }

    ++e;  // next edge.
  }
  if (e != nEdges) {
    ++errors;
  }
  if (p != nPins) {
    ++errors;
  }

  // Connectivity information.
  {
    nw_->m_nodePins.resize(nw_->m_pins.size());
    nw_->m_edgePins.resize(nw_->m_pins.size());
    for (size_t i = 0; i < nw_->m_pins.size(); i++) {
      nw_->m_nodePins[i] = &(nw_->m_pins[i]);
      nw_->m_edgePins[i] = &(nw_->m_pins[i]);
    }
    std::stable_sort(nw_->m_nodePins.begin(), nw_->m_nodePins.end(),
                     Network::comparePinsByNodeId());
    p = 0;
    for (n = 0; n < nw_->getNumNodes(); n++) {
      Node* nd = nw_->getNode(n);

      nd->setFirstPinIdx(p);
      while (p < nw_->m_nodePins.size() && nw_->m_nodePins[p]->getNodeId() == n)
        ++p;
      nd->setLastPinIdx(p);
    }

    std::stable_sort(nw_->m_edgePins.begin(), nw_->m_edgePins.end(),
                     Network::comparePinsByEdgeId());
    p = 0;
    for (e = 0; e < nw_->getNumEdges(); e++) {
      Edge* ed = nw_->getEdge(e);

      ed->setFirstPinIdx(p);
      while (p < nw_->m_edgePins.size() && nw_->m_edgePins[p]->getEdgeId() == e)
        ++p;
      ed->setLastPinIdx(p);
    }
  }

  if (errors != 0) {
    logger_->error(DPO, 101, "Error creating network.");
  } else {
    logger_->info(DPO, 102, "Network stats: inst {}, edges {}, pins {}",
                  nw_->getNumNodes(), nw_->getNumEdges(), nw_->m_pins.size());
  }
}
////////////////////////////////////////////////////////////////
void Optdp::createArchitecture() {
  dbBlock* block = db_->getChip()->getBlock();

  dbSet<dbRow> rows = block->getRows();

  odb::Rect coreRect;
  block->getCoreArea(coreRect);
  odb::Rect dieRect;
  block->getDieArea(dieRect);



  for (dbRow* row : rows) {
    if (row->getDirection() != odb::dbRowDir::HORIZONTAL) {
      // error.
      continue;
    }
    dbSite* site = row->getSite();
    int originX;
    int originY;
    row->getOrigin(originX, originY);

    Architecture::Row* archRow = new Architecture::Row;

    archRow->m_rowLoc = (double)originY;
    archRow->m_rowHeight = (double)site->getHeight();
    archRow->m_siteWidth = (double)site->getWidth();
    archRow->m_siteSpacing = (double)row->getSpacing();
    archRow->m_subRowOrigin = (double)originX;
    archRow->m_numSites = row->getSiteCount();

    // Is this correct?  No...
    archRow->m_powerBot = RowPower_UNK;
    archRow->m_powerTop = RowPower_UNK;
    // Is this correct?  No...
    archRow->m_siteOrient = Orientation_N;
    // Is this correct?  No...
    archRow->m_siteSymmetry = Symmetry_UNKNOWN;

    arch_->m_rows.push_back(archRow);
  }

  // Get surrounding box.
  arch_->m_xmin = std::numeric_limits<double>::max();
  arch_->m_xmax = -std::numeric_limits<double>::max();
  arch_->m_ymin = std::numeric_limits<double>::max();
  arch_->m_ymax = -std::numeric_limits<double>::max();
  for (int r = 0; r < arch_->m_rows.size(); r++) {
    Architecture::Row* row = arch_->m_rows[r];

    double lx = row->m_subRowOrigin;
    double rx = lx + row->m_numSites * row->m_siteSpacing;

    double yb = row->getY();
    double yt = yb + row->getH();

    arch_->m_xmin = std::min(arch_->m_xmin, lx);
    arch_->m_xmax = std::max(arch_->m_xmax, rx);
    arch_->m_ymin = std::min(arch_->m_ymin, yb);
    arch_->m_ymax = std::max(arch_->m_ymax, yt);
  }
  if (arch_->m_xmin != (double)dieRect.xMin() ||
      arch_->m_xmax != (double)dieRect.xMax() ||
      arch_->m_ymin != (double)dieRect.yMin() ||
      arch_->m_ymax != (double)dieRect.yMax()) {
    arch_->m_xmin = dieRect.xMin();
    arch_->m_xmax = dieRect.xMax();
  }

  for (int r = 0; r < arch_->m_rows.size(); r++) {
    int numSites = arch_->m_rows[r]->m_numSites;
    double originX = arch_->m_rows[r]->m_subRowOrigin;
    double siteSpacing = arch_->m_rows[r]->m_siteSpacing;

    double lx = originX;
    double rx = originX + numSites * siteSpacing;
    if (lx < arch_->m_xmin || rx > arch_->m_xmax) {
      if (lx < arch_->m_xmin) {
        originX = arch_->m_xmin;
      }
      rx = originX + numSites * siteSpacing;
      if (rx > arch_->m_xmax) {
        numSites = (int)((arch_->m_xmax - originX) / siteSpacing);
      }

      if (arch_->m_rows[r]->m_subRowOrigin != originX) {
        arch_->m_rows[r]->m_subRowOrigin = originX;
      }
      if (arch_->m_rows[r]->m_numSites != numSites) {
        arch_->m_rows[r]->m_numSites = numSites;
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
    if (!(net->getSigType() == dbSigType::POWER ||
          net->getSigType() == dbSigType::GROUND)) {
      continue;
    }
    int pwr =
        (net->getSigType() == dbSigType::POWER) ? RowPower_VDD : RowPower_VSS;
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
        if (pwr == RowPower_VDD) {
          if (pwrLayers_.end() == pwrLayers_.find(layer)) {
            continue;
          }
        } else if (pwr == RowPower_VSS) {
          if (gndLayers_.end() == gndLayers_.find(layer)) {
            continue;
          }
        }

        Rect rect;
        sbox->getBox(rect);
        for (size_t r = 0; r < arch_->m_rows.size(); r++) {
          double yb = arch_->m_rows[r]->getY();
          double yt = yb + arch_->m_rows[r]->getH();

          if (yb >= rect.yMin() && yb <= rect.yMax()) {
            arch_->m_rows[r]->m_powerBot = pwr;
          }
          if (yt >= rect.yMin() && yt <= rect.yMax()) {
            arch_->m_rows[r]->m_powerTop = pwr;
          }
        }
      }
    }
  }
  arch_->postProcess(nw_);
}
////////////////////////////////////////////////////////////////
void Optdp::createRouteGrid() {
  // Relates to pin blockages, etc. Maybe not needed right now.
  ;
}
////////////////////////////////////////////////////////////////
void Optdp::setUpNdrRules() {
  // Relates to pin blockages, etc. Maybe not needed right now.
  ;
}
////////////////////////////////////////////////////////////////
void Optdp::setUpPlacementRegions() {
  double xmin, xmax, ymin, ymax;
  int r = 0;
  xmin = arch_->getMinX();
  xmax = arch_->getMaxX();
  ymin = arch_->getMinY();
  ymax = arch_->getMaxY();

  dbBlock* block = db_->getChip()->getBlock();
  dbSet<dbRegion> regions = block->getRegions();

  std::unordered_map<odb::dbInst*, Node*>::iterator it_n;
  Architecture::Region* rptr = nullptr;

  // Default region.
  rptr = new Architecture::Region;
  rptr->m_rects.push_back(Rectangle(xmin, ymin, xmax, ymax));
  rptr->m_xmin = xmin;
  rptr->m_xmax = xmax;
  rptr->m_ymin = ymin;
  rptr->m_ymax = ymax;
  rptr->m_id = arch_->m_regions.size();
  arch_->m_regions.push_back(rptr);

  // Hmm.  I noticed a comment in the OpenDP interface that
  // the OpenDB represents groups as regions.  I'll follow
  // the same approach and hope it is correct.
  // DEF GROUP => dbRegion with instances, no boundary, parent->region
  // DEF REGION => dbRegion no instances, boundary, parent = null
  auto db_regions = block->getRegions();
  for (auto db_region : db_regions) {
    dbRegion* parent = db_region->getParent();
    if (parent) {
      rptr = new Architecture::Region;
      rptr->m_id = arch_->m_regions.size();
      arch_->m_regions.push_back(rptr);

      // Assuming these are the rectangles making up the region...
      auto boundaries = db_region->getParent()->getBoundaries();
      for (dbBox* boundary : boundaries) {
        Rect box;
        boundary->getBox(box);

        xmin = std::max(arch_->getMinX(), (double)box.xMin());
        xmax = std::min(arch_->getMaxX(), (double)box.xMax());
        ymin = std::max(arch_->getMinY(), (double)box.yMin());
        ymax = std::min(arch_->getMaxY(), (double)box.yMax());

        rptr->m_rects.push_back(Rectangle(xmin, ymin, xmax, ymax));
        rptr->m_xmin = std::min(xmin, rptr->m_xmin);
        rptr->m_xmax = std::max(xmax, rptr->m_xmax);
        rptr->m_ymin = std::min(ymin, rptr->m_ymin);
        rptr->m_ymax = std::max(ymax, rptr->m_ymax);
      }

      // The instances within this region.
      for (auto db_inst : db_region->getRegionInsts()) {
        it_n = instMap_.find(db_inst);
        if (instMap_.end() != it_n) {
          Node* nd = it_n->second;
          if (nd->getRegionId() == 0) {
            nd->setRegionId(rptr->m_id);
          }
        }
        // else error?
      }
    }
  }
  // Compute counts of the number of nodes in each region.
  arch_->m_numNodesInRegion.resize(arch_->m_regions.size());
  std::fill(arch_->m_numNodesInRegion.begin(), arch_->m_numNodesInRegion.end(),
            0);
  for (size_t i = 0; i < nw_->getNumNodes(); i++) {
    Node* ndi = nw_->getNode(i);

    int regId = ndi->getRegionId();
    if (regId >= arch_->m_numNodesInRegion.size()) {
      continue;  // error.
    }
    ++arch_->m_numNodesInRegion[regId];
  }

  logger_->info(DPO, 103, "Number of regions is {:d}", arch_->m_regions.size());
  for (size_t r = 0; r < arch_->m_regions.size(); r++) {
    rptr = arch_->m_regions[r];
    logger_->info(DPO, 104, "Region {:d} has {:d} instances.", r,
                  arch_->m_numNodesInRegion[r]);
  }
}

}  // namespace dpo
