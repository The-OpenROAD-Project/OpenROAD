// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <sys/resource.h>
#include <unistd.h>

#include <algorithm>
#include <vector>

#include "rcx/extModelGen.h"
#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "rcx/ext_options.h"
#include "util.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;
using namespace odb;

class extModelGen;

uint extMain::getPeakMemory(const char* msg, int n)
{
  if (_dbgOption == 0)
    return 0;

  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    // Convert from KB to Mbytes
    int peak_rss = usage.ru_maxrss / 1024;

    if (n < 0)
      fprintf(stdout, "\t\t\t\tPeak Memory: %s    %6d MBytes\n", msg, peak_rss);
    else
      fprintf(
          stdout, "\t\t\t\tPeak Memory: %s %d %6d MBytes\n", msg, n, peak_rss);
    return peak_rss;
  }
  return 0;
}

// Main function that drives extraction flow
void extMain::makeBlockRCsegs_v2(const char* netNames, const char* extRules)
{
  // Model file is required if not option _lefRC is used.
  // _lefRC option requires Resistance and Capacitance values per Layer and
  // Technology to be taken from LEF file.
  if (!_lefRC && !modelExists(extRules))
    return;

  // Selected user set for net names to be extracted
  std::vector<dbNet*> inets;
  markNetsToExtract_v2(netNames, inets);

  initSomeValues_v2();
  if (_couplingFlag > 1 && !_lefRC) {
    getPeakMemory("Before LoadModels: ");

    // Associate User defined Process Corners and DensityModels in Model file
    if (!SetCornersAndReadModels_v2(extRules))
      return;

    getPeakMemory("End LoadModels: ");

    // Create Capacitance table per layer with min/max values
    // based on the model file given min nad max context scenarios
    calcMinMaxRC();

    // Create Capacitance and Resistance table per layer with min/max values
    // based on the model file required for RC Network Generation
    getResCapTable();

  } else if (_lefRC) {
    // Add a single process corner to drive the flow
    addRCCorner("LEF_RC", 0);
    _extDbCnt = _processCornerTable->getCnt();
    _block->setCornerCount(_cornerCnt, _extDbCnt, nullptr);

    // Create tables with RC values per layer from LEF file
    getResCapTable_lefRC_v2();
  }

  // Create RC network for every net: Resistor Nodes and Resistor Segments,
  // Following the order of the wires
  // NOTE: dependent on odb::orderWires
  if (!makeRCNetwork_v2())
    return;

  getPeakMemory("End RC_Network: ");

  if (_lefRC) {
    // update dbNet object flags
    update_wireAltered_v2(inets);
    return;
  }
  if (_couplingFlag > 1) {
    // Print out stats
    infoBeforeCouplingExt();

    Rect maxRect = _block->getDieArea();
    couplingFlow_v2(maxRect, _couplingFlag, nullptr);
    // Print out stats on db Ojects created during extraction
    couplingExtEnd_v2();
  }
  // Print out stats on db Ojects created during extraction
  // _modelTable->resetCnt(0);
  getPeakMemory("End Extraction: ");
}
void extMain::infoBeforeCouplingExt()
{
  logger_->info(RCX,
                503,
                "Start Coupling Cap extraction {} ...",
                getBlock()->getName().c_str());

  logger_->info(RCX,
                504,
                "Coupling capacitance less than {:.4f} fF will be grounded.",
                _coupleThreshold);
}
bool extMain::markNetsToExtract_v2(const char* netNames,
                                   std::vector<dbNet*>& inets)
{
  _allNet = !findSomeNet(_block, netNames, inets, logger_);
  for (uint j = 0; j < inets.size(); j++) {
    dbNet* net = inets[j];
    net->setMark(true);
  }
  // DELETE ?
  if (!_allNet) {
    _ccMinX = MAX_INT;
    _ccMinY = MAX_INT;
    _ccMaxX = -MAX_INT;
    _ccMaxY = -MAX_INT;
  }
  return _allNet;
}
bool extMain::makeRCNetwork_v2()
{
  logger_->info(RCX,
                501,
                "RC segment generation {} (max_merge_res {:.1f}) ...",
                getBlock()->getName().c_str(),
                _mergeResBound);

  setupMapping();  // iterm, bterm, junction CapNode mapping

  uint cnt = 0;
  for (dbNet* net : _block->getNets()) {
    if (net->getSigType().isSupply())
      continue;

    if (!_allNet && !net->isMarked())
      continue;

    _connectedBTerm.clear();
    _connectedITerm.clear();

    cnt += makeNetRCsegs_v2(net);

    for (dbBTerm* bterm : _connectedBTerm) {
      bterm->setMark(0);
    }
    for (dbITerm* iterm : _connectedITerm) {
      iterm->setMark(0);
    }
  }
  logger_->info(RCX, 502, "Final {} rc segments", cnt);
  return true;
}
bool extMain::couplingExtEnd_v2()
{
  _extracted = true;
  updatePrevControl();
  int numOfNet;
  int numOfRSeg;
  int numOfCapNode;
  int numOfCCSeg;
  _block->getExtCount(numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg);
  if (numOfRSeg) {
    logger_->info(RCX,
                  450,
                  "Extract {} nets, {} rsegs, {} caps, {} ccs",
                  numOfNet - 2,
                  numOfRSeg,
                  numOfCapNode,
                  numOfCCSeg);
    return true;
  } else {
    logger_->warn(RCX, 510, "Nothing is extracted out of {} nets!", numOfNet);
    return false;
  }
}
void extMain::update_wireAltered_v2(std::vector<dbNet*>& inets)
{
  if (_allNet) {
    for (dbNet* net : _block->getNets()) {
      if (net->getSigType().isSupply()) {
        continue;
      }
      net->setWireAltered(false);
    }
  } else {
    for (dbNet* net : inets) {
      net->setMark(false);
      net->setWireAltered(false);
    }
  }
}

void extMain::setExtractionOptions_v2(ExtractOptions options)
{
  skip_via_wires(
      options
          .skip_via_wires);  // Skip Coupling caps for Vias -- affects run time
  skip_via_wires(true);      // for debugging purposes can skip via resistance
  _lef_res = options.lef_res;  // Don't use wire resistance from Model file; but
                               // from LEF file
  _lefRC = options.lef_rc;  // Wire Resistance/Capcitance and Via Resistance are
                            // taken from LEF file; model file not required

  _wire_extracted_progress_count = options._wire_extracted_progress_count;
  _version = options._version;
  _metal_flag_22 = 0;
  // fprintf(stdout, "RC Flow Version %5.3f enabled\n", _version);
  // notice(0, "RC Flow Version %5.3f enabled\n", _version);
  if (abs(options._version - 2.2) < 0.001) {
    _metal_flag_22 = 2;
    fprintf(stdout,
            "Version %5.3f enabled new RC calc flow for lower 2 metals\n",
            options._version);
  }
  if (abs(options._version - 2.3) < 0.001) {
    _metal_flag_22 = 3;
    fprintf(stdout,
            "Version %5.3f enabled new RC calc flow for lower 2 metals\n",
            options._version);
  }
  _v2 = options._v2;
  if (options._v2 >= 2.2)
    _v2 = true;
  _dbgOption = options._dbg;
  _overCell = _v2 && options.over_cell;  // Use inside cell context for coupling
                                         // cap extraction

  // TODO _cornerCnt?
  _diagFlow = true;
  _couplingFlag = options.cc_model;
  _coupleThreshold = options.coupling_threshold;  //
  _usingMetalPlanes = true;
  _ccUp = options.cc_up;  // Context up
  _ccContextDepth = options.context_depth;
  _mergeViaRes = !options.no_merge_via_res;
  _mergeResBound = options.max_res;
  _extRun++;
}

void extMain::initSomeValues_v2()
{
  _useDbSdb = false;
  _foreign = false;  // extract after read_spef
  _totCCcnt = 0;
  _totSmallCCcnt = 0;
  _totBigCCcnt = 0;
  _totSegCnt = 0;
  _totSignalSegCnt = 0;
}

// Associate User defined Process Corners and DensityModels in Model file
bool extMain::SetCornersAndReadModels_v2(const char* rulesFileName)
{
  _modelMap.resetCnt(0);
  _metRCTable.resetCnt(0);

  if (rulesFileName != nullptr) {  // read rules

    extRCModel* m = createCornerMap(rulesFileName);

    if (!ReadModels_v2(rulesFileName, m, 0, nullptr))
      return false;
  }
  _currentModel = getRCmodel(0);
  for (uint ii = 0; (_couplingFlag > 0) && ii < _modelMap.getCnt(); ii++) {
    uint jj = _modelMap.get(ii);
    _metRCTable.add(_currentModel->getMetRCTable(jj));
  }
  _extDbCnt = _processCornerTable->getCnt();
  _block->setCornerCount(_cornerCnt, _extDbCnt, nullptr);
  return true;
}
double extMain::getDbFactor_v2()
{
  int dbunit = _block->getDbUnitsPerMicron();
  double dbFactor = 1;
  if (dbunit > 1000) {
    dbFactor = dbunit * 0.001;
  }
  return dbFactor;
}
// Wrapper function to drive reading of the model file
bool extMain::ReadModels_v2(const char* rulesFileName,
                            extRCModel* m,
                            uint extDbCnt,
                            uint* cornerTable)
{
  logger_->info(
      RCX, 441, "Reading extraction model file {} ...", rulesFileName);

  FILE* rules_file = fopen(rulesFileName, "r");
  if (rules_file == nullptr) {
    logger_->error(
        RCX, 469, "Can't open extraction model file {}", rulesFileName);
  }
  fclose(rules_file);

  m->_v2_flow = _v2;

  double dbFactor = getDbFactor_v2();

  if (!(m->readRules_v2(
          (char*) rulesFileName, false, true, true, true, true, dbFactor))) {
    delete m;
    return false;
  }
  // If RCX reads wrong extRules file format
  if (getRCmodel(0)->getModelCnt() == 0) {
    logger_->error(RCX,
                   488,
                   "No RC model read from the extraction model! "
                   "Ensure the right extRules file is used!");
    return false;
  }

  return true;
}

uint extMain::resetMapNodes_v2(dbWire* wire)
{
  uint cnt = 0;
  dbWirePath path;
  dbWirePathShape pshape;

  dbWirePathItr pitr;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    resetMapping(path.bterm, path.iterm, path.junction_id);

    while (pitr.getNextShape(pshape)) {
      resetMapping(pshape.bterm, pshape.iterm, pshape.junction_id);
      cnt++;
    }
  }
  return cnt;
}
uint extMain::makeNetRCsegs_v2(dbNet* net, bool skipStartWarning)
{
  net->setRCgraph(true);

  dbWire* wire = net->getWire();
  if (wire == nullptr) {
    if (_reportNetNoWire) {
      logger_->info(RCX, 511, "Net {} has no wires.", net->getName().c_str());
    }
    _netNoWireCnt++;
    return 0;
  }
  const uint rcCnt1 = resetMapNodes_v2(wire);
  if (rcCnt1 <= 0)
    return 0;

  _netGndcCalibFactor = net->getGndcCalibFactor();
  _netGndcCalibration = _netGndcCalibFactor == 1.0 ? false : true;

  initJunctionIdMaps(net);

  dbWirePathItr pitr;
  uint srcId;
  dbWirePathShape ppshape;
  uint rcCnt = 0;
  bool netHeadMarked = false;
  dbWirePath path;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    uint netId = net->getId();
    if (netId == _debug_net_id) {
      debugPrint(logger_,
                 RCX,
                 "rcseg",
                 1,
                 "RCSEG:R makeNetRCsegs:  path.junction_id {}",
                 path.junction_id);
    }
    if (!path.iterm && !path.bterm && !path.is_branch && path.is_short) {
      // if (! (path.iterm || path.bterm || path.is_branch) && path.is_short) {
      uint junct_id = getShortSrcJid(path.junction_id);
      srcId = getCapNodeId_v2(net, junct_id, true);
    } else {
      uint junct_id = getShortSrcJid(path.junction_id);
      srcId = getCapNodeId_v2(net, path, junct_id, path.is_branch);
    }
    if (!netHeadMarked) {
      netHeadMarked = true;
      make1stRSeg(net, path, srcId, skipStartWarning);
    }
    Point prevPoint = path.point;
    Point sprevPoint = prevPoint;
    resetSumRCtable();  // dkf start the path for merging resistors
    dbWirePathShape pshape;
    while (pitr.getNextShape(pshape)) {
      dbShape s = pshape.shape;

      if (netId == _debug_net_id) {
        debugPrint(logger_,
                   RCX,
                   "rcseg",
                   1,
                   "RCSEG:R makeNetRCsegs: {} {}",
                   pshape.junction_id,
                   s.isVia() ? "VIA" : "WIRE");
      }

      getShapeRC_v2(net, s, sprevPoint, pshape);

      sprevPoint = pshape.point;

      // ------------------------------------------------------------------
      // No merging except via merge if _mergeViaRes
      // --------------------------------------------------------------------
      if (_mergeResBound == 0.0) {  // No merging except via merge is
        if (!s.isVia()) {
          _rsegJid.push_back(pshape.junction_id);
        }
        addToSumRCtable();

        // ------------------------------------------------------------
        // Have to create node because reached iterm/bterm or branch
        // ------------------------------------------------------------

        bool isStopNode = pshape.bterm != nullptr || pshape.iterm != nullptr
                          || _nodeTable->geti(pshape.junction_id) < 0;

        // if ( (_mergeViaRes && s.isVia()) || isStopNode )
        if (!_mergeViaRes || !s.isVia() || isStopNode) {
          dbRSeg* rc
              = addRSeg_v2(net, srcId, prevPoint, path, pshape, path.is_branch);

          if (s.isVia() && rc != nullptr) {
            createShapeProperty(net, pshape.junction_id, rc->getId());
          }
          resetSumRCtable();
          rcCnt++;
        } else {
          ppshape = pshape;
        }
        continue;
      }
      //----------------------------------------------------
      // Beggining of  the next merging, create rc
      //----------------------------------------------------
      if (_tmpResTable[0] >= _mergeResBound && _tmpSumResTable[0] == 0.0) {
        if (!s.isVia()) {
          _rsegJid.push_back(pshape.junction_id);
        }
        addRSeg_v2(net,
                   srcId,
                   prevPoint,
                   path,
                   pshape,
                   path.is_branch,
                   _tmpResTable,
                   _tmpCapTable);
        rcCnt++;
        continue;
      }
      //----------------------------------------------------
      // Beggining of  the next merging, create rc
      //----------------------------------------------------
      if ((_tmpResTable[0] + _tmpSumResTable[0]) >= _mergeResBound) {
        addRSeg_v2(net, srcId, prevPoint, path, ppshape, path.is_branch);
        rcCnt++;
        if (!s.isVia()) {
          _rsegJid.push_back(pshape.junction_id);
        }
        copyToSumRCtable();
      } else {
        if (!s.isVia()) {
          _rsegJid.push_back(pshape.junction_id);
        }
        addToSumRCtable();
      }
      bool isStopNode = pshape.bterm != nullptr || pshape.iterm != nullptr
                        || _nodeTable->geti(pshape.junction_id) < 0;

      if (isStopNode || (_tmpSumResTable[0] >= _mergeResBound)) {
        addRSeg_v2(net, srcId, prevPoint, path, pshape, path.is_branch);
        rcCnt++;
        resetSumRCtable();
      } else {
        ppshape = pshape;
      }
    }
    if (_sumUpdated) {
      addRSeg_v2(net, srcId, prevPoint, path, ppshape, path.is_branch);
      rcCnt++;
    }
  }
  net->getRSegs().reverse();

  return rcCnt;
}
uint extMain::getCapNodeId_v2(dbNet* net,
                              dbWirePath& path,
                              const uint junct_id,
                              bool branch)
{
  uint srcId = 0;
  if (path.bterm != nullptr)
    srcId = getCapNodeId_v2(path.bterm, junct_id);
  else if (path.iterm != nullptr)
    srcId = getCapNodeId_v2(path.iterm, junct_id);
  else
    srcId = getCapNodeId_v2(net, junct_id, branch);
  return srcId;
}
uint extMain::getCapNodeId_v2(dbNet* net,
                              const dbWirePathShape& pshape,
                              int junct_id,
                              bool branch)
{
  uint srcId = 0;
  if (pshape.bterm != nullptr)
    srcId = getCapNodeId_v2(pshape.bterm, junct_id);
  else if (pshape.iterm != nullptr)
    srcId = getCapNodeId_v2(pshape.iterm, junct_id);
  else
    srcId = getCapNodeId_v2(net, junct_id, branch);
  return srcId;
}
uint extMain::getCapNodeId_v2(dbITerm* iterm, const uint junction)
{
  uint capId = _itermTable->geti(iterm->getId());
  if (capId > 0)  // CapNode exists
    return capId;

  dbCapNode* cap = dbCapNode::create(iterm->getNet(), 0, _foreign);

  cap->setNode(iterm->getId());
  cap->setITermFlag();

  capId = cap->getId();

  _itermTable->set(iterm->getId(), capId);
  int tcapId = _nodeTable->geti(junction) == -1 ? -capId : capId;
  _nodeTable->set(junction, tcapId);  // allow get capId using junction

  return capId;
}
uint extMain::getCapNodeId_v2(dbBTerm* bterm, const uint junction)
{
  uint id = bterm->getId();
  uint capId = _btermTable->geti(id);
  if (capId > 0)
    return capId;

  dbCapNode* cap = dbCapNode::create(bterm->getNet(), 0, _foreign);

  cap->setNode(bterm->getId());
  cap->setBTermFlag();

  capId = cap->getId();

  _btermTable->set(id, capId);
  const int tcapId = _nodeTable->geti(junction) == -1 ? -capId : capId;
  _nodeTable->set(junction, tcapId);  // allow get capId using junction

  return capId;
}
uint extMain::getCapNodeId_v2(dbNet* net, const int junction, const bool branch)
{
  int capId = _nodeTable->geti(junction);

  if (capId != 0 && capId != -1) {
    capId = abs(capId);
    dbCapNode* cap = dbCapNode::getCapNode(_block, capId);
    if (branch) {
      cap->setBranchFlag();
    }
    if (cap->getNet()->getId() == _debug_net_id)
      print_debug(branch, junction, capId, "OLD");

    return capId;
  }

  dbCapNode* cap = dbCapNode::create(net, 0, _foreign);
  cap->setInternalFlag();
  cap->setNode(junction);

  if (capId == -1) {
    if (branch) {
      cap->setBranchFlag();
    }
  }
  if (cap->getNet()->getId() == _debug_net_id)
    print_debug(branch, junction, capId, "NEW");

  uint ncapId = cap->getId();
  int tcapId = capId == 0 ? ncapId : -ncapId;
  _nodeTable->set(junction, tcapId);
  return ncapId;
}
void extMain::print_debug(const bool branch,
                          const uint junction,
                          uint capId,
                          const char* old_new)
{
  if (branch) {
    debugPrint(logger_,
               RCX,
               "rcseg",
               1,
               "RCSEG:C {} BRANCH {}  capNode {}",
               old_new,
               junction,
               capId);
  } else {
    debugPrint(logger_,
               RCX,
               "rcseg",
               1,
               "RCSEG:C {} INTERNAL {}  capNode {}",
               old_new,
               junction,
               capId);
  }
}

dbRSeg* extMain::addRSeg_v2(dbNet* net,
                            uint& srcId,
                            Point& prevPoint,
                            const dbWirePath& path,
                            const dbWirePathShape& pshape,
                            const bool isBranch,
                            const double* restbl,
                            const double* captbl)
{
  if (restbl == nullptr) {
    restbl = _tmpSumResTable;
    captbl = _tmpSumCapTable;
  }

  if (!path.bterm && isTermPathEnded(pshape.bterm, pshape.iterm)) {
    _rsegJid.clear();
    return nullptr;
  }
  const uint dstId = getCapNodeId_v2(net, pshape, pshape.junction_id, isBranch);

  // const uint dstId= getCapNodeId( net, pshape.bterm, pshape.iterm,
  // pshape.junction_id, isBranch);

  if (dstId == srcId) {
    loopWarning(net, pshape);
    return nullptr;
  }

  if (net->getId() == _debug_net_id)
    print_shape(pshape.shape, srcId, dstId);

  uint length;
  const uint pathDir = computePathDir(prevPoint, pshape.point, &length);

  Point pt;
  if (pshape.junction_id) {
    pt = net->getWire()->getCoord(pshape.junction_id);
  }
  dbRSeg* rc = dbRSeg::create(net, pt.x(), pt.y(), pathDir, true);

  const uint jidl = _rsegJid.size();
  const uint rsid = rc->getId();
  for (uint jj = 0; jj < jidl; jj++) {
    net->getWire()->setProperty(_rsegJid[jj], rsid);
  }
  _rsegJid.clear();

  rc->setSourceNode(srcId);
  rc->setTargetNode(dstId);

  if (srcId > 0)
    dbCapNode::getCapNode(_block, srcId)->incrChildrenCnt();
  if (dstId > 0)
    dbCapNode::getCapNode(_block, dstId)->incrChildrenCnt();

  setResAndCap_v2(rc, restbl, captbl);

  if (net->getId() == _debug_net_id) {
    debugPrint(logger_,
               RCX,
               "rcseg",
               1,
               "RCSEG:R shapeId= {}  rseg= {}  ({} {}) {:g}",
               pshape.junction_id,
               rsid,
               srcId,
               dstId,
               rc->getCapacitance(0));
  }

  srcId = dstId;
  prevPoint = pshape.point;
  return rc;
}
void extMain::setResAndCap_v2(dbRSeg* rc,
                              const double* restbl,
                              const double* captbl)
{
  for (uint ii = 0; ii < _extDbCnt; ii++) {
    const int pcdbIdx = getProcessCornerDbIndex(ii);
    double res = _resModify ? restbl[ii] * _resFactor : restbl[ii];
    rc->setResistance(res, pcdbIdx);
    double cap = _gndcModify ? captbl[ii] * _gndcFactor : captbl[ii];
    cap = _netGndcCalibration ? cap * _netGndcCalibFactor : cap;
    if (_lefRC)
      rc->setCapacitance(cap, pcdbIdx);  // _lefRC

    int sci, scdbIdx;
    getScaledCornerDbIndex(ii, sci, scdbIdx);
    if (sci == -1) {
      continue;
    }
    getScaledRC(sci, res, cap);
    rc->setResistance(res, scdbIdx);
    rc->setCapacitance(cap, scdbIdx);
  }
}
void extMain::loopWarning(dbNet* net, const dbWirePathShape& pshape)
{
  std::string tname;
  if (pshape.bterm) {
    tname += fmt::format(", on bterm {}", pshape.bterm->getConstName());
  } else if (pshape.iterm) {
    tname += fmt::format(", on iterm {}/{}",
                         pshape.iterm->getInst()->getConstName(),
                         pshape.iterm->getMTerm()->getConstName());
  }
  logger_->warn(RCX,
                117,
                "Net {} {} has a loop at x={} y={} {}.",
                net->getId(),
                net->getConstName(),
                pshape.point.getX(),
                pshape.point.getY(),
                tname);
}
void extMain::initJunctionIdMaps(dbNet* net)
{
  _rsegJid.clear();
  _shortSrcJid.clear();
  _shortTgtJid.clear();

  uint netId = net->getId();
  if (netId == _debug_net_id) {
    debugPrint(
        logger_, RCX, "rcseg", 1, "RCSEG:R makeNetRCsegs: BEGIN NET {}", netId);
  }
  uint srcJid;
  dbWirePathItr pitr;
  if (!(_mergeResBound != 0.0 || _mergeViaRes))
    return;
  dbWire* wire = net->getWire();

  dbWirePath path;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    if (!path.bterm && !path.iterm && path.is_branch && path.junction_id) {
      _nodeTable->set(path.junction_id, -1);
    }

    if (path.is_short) {
      _nodeTable->set(path.short_junction, -1);
      srcJid = path.short_junction;
      for (uint tt = 0; tt < _shortTgtJid.size(); tt++) {
        if (_shortTgtJid[tt] == srcJid) {
          srcJid = _shortSrcJid[tt];  // forward short
          break;
        }
      }
      _shortSrcJid.push_back(srcJid);
      _shortTgtJid.push_back(path.junction_id);
    }
    dbWirePathShape pshape;
    while (pitr.getNextShape(pshape)) {
      ;
    }
  }
}

double extMain::getViaRes_v2(dbNet* net, dbTechVia* tvia)
{
  // OPTIMIZE: map for all technology vias

  const uint width = tvia->getBottomLayer()->getWidth();
  uint level = tvia->getBottomLayer()->getRoutingLevel();

  double res
      = tvia->getResistance();  // LEF -- via section OR set at previous call
  if (res == 0)
    res = getViaResistance(tvia);  // CUT layer res
  if (res <= 0.0)
    res = getResistance(level, width, width, 0);  // Bottom Layer Resistance

  if (_lef_res || _lefRC) {
    if (res > 0)
      tvia->setResistance(res);  // set for next call af another net

    for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
      _tmpResTable[ii] = res;
    }
    if (_lefRC) {  // estimate Capacitance
      _tmpResTable[0] = res;

      double areaCap;
      double fringeCap = getFringe(level, width, 0, areaCap);
      _tmpCapTable[0] = width * 2 * fringeCap;
      _tmpCapTable[0] += 2 * areaCap * width * width;
    }

  } else {  // form model file
    bool viaModelFound = false;
    const char* viaName = tvia->getConstName();
    for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
      extMetRCTable* rcTable = _metRCTable.get(ii);
      extViaModel* viaModel = rcTable->getViaModel((char*) viaName);
      if (viaModel != nullptr) {
        _tmpResTable[ii] = viaModel->_res;
        viaModelFound = true;
      }
    }
    if (!viaModelFound) {
      for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
        _tmpResTable[ii] = res;
      }
    }
  }
  return res;
}
double extMain::getDbViaRes_v2(dbNet* net, const dbShape& s)
{
  dbVia* bvia = s.getVia();
  double res = getViaResistance_b(bvia, net);

  const uint width = bvia->getBottomLayer()->getWidth();
  uint level = bvia->getBottomLayer()->getRoutingLevel();

  if (res <= 0.0) {
    res = getResistance(level, width, width, 0);
  }
  if (level > 0) {
    if (_lefRC || _lef_res) {
      _tmpResTable[0] = res;
      if (_lef_res) {
        dbVia* bvia = s.getVia();
        uint width = bvia->getBottomLayer()->getWidth();
        double areaCap;
        _tmpCapTable[0] = width * 2 * getFringe(level, width, 0, areaCap);
        _tmpCapTable[0] += 2 * areaCap * width * width;
      }
    } else {
      getViaCapacitance(s, net);
      for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
        _tmpResTable[ii] = res;
      }
    }
  }
  return res;
}
double extMain::getMetalRes_v2(dbNet* net,
                               const dbShape& s,
                               const dbWirePathShape& pshape)
{
  const uint level = s.getTechLayer()->getRoutingLevel();
  uint width = std::min(pshape.shape.getDX(), pshape.shape.getDY());
  uint len = std::max(pshape.shape.getDX(), pshape.shape.getDY());

  double res = getLefResistance(level, width, len, 0);

  if (_lefRC || _lef_res) {
    res = _resistanceTable[0][level];
    res *= len;
    _tmpResTable[0] = res;
    if (_lefRC) {
      // const float cap_pf_per_micron = width *
      // s.getTechLayer()->getCapacitance() + 2 *
      // s.getTechLayer()->getEdgeCapacitance();
      float cap_fF_per_nm = _capacitanceTable[0][level];
      float cap = cap_fF_per_nm * len;
      _tmpCapTable[0] = cap;
    }
  } else {
    for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
      double r = getResistance(level, width, len, ii);
      _tmpResTable[ii] = r;
      double areaCap;
      getFringe(level, width, ii, areaCap);

      _tmpCapTable[ii] = 0;
    }
  }
  return res;
}
void extMain::getShapeRC_v2(dbNet* net,
                            const dbShape& s,
                            Point& prevPoint,
                            const dbWirePathShape& pshape)
{
  if (s.isVia()) {
    dbTechVia* tvia = s.getTechVia();
    if (tvia != nullptr) {
      getViaRes_v2(net, tvia);
    } else {
      dbVia* bvia = s.getVia();
      if (bvia != nullptr)
        getDbViaRes_v2(net, s);
    }
  } else {
    getMetalRes_v2(net, s, pshape);

    if (!_allNet && _couplingFlag > 0) {
      _ccMinX = std::min(s.xMin(), _ccMinX);
      _ccMinY = std::min(s.yMin(), _ccMinY);
      _ccMaxX = std::max(s.xMax(), _ccMaxX);
      _ccMaxY = std::max(s.yMax(), _ccMaxY);
    }
  }
  prevPoint = pshape.point;
}
extRCModel* extMain::createCornerMap(const char* rulesFileName)
{
  extRCModel* m = new extRCModel("MINTYPMAX", logger_);
  _modelTable->add(m);

  // uint cornerTable[10];
  uint extCornerDbCnt = 0;

  _minModelIndex = 0;
  _maxModelIndex = 0;
  _typModelIndex = 0;

  if (_processCornerTable != nullptr) {
    // User define process corners using <ext define_process_corner>
    for (uint ii = 0; ii < _processCornerTable->getCnt(); ii++) {
      extCorner* s = _processCornerTable->get(ii);
      //    cornerTable[extCornerDbCnt++] = s->_model;
      _modelMap.add(s->_model);
    }
  } else {
    // No user defined process corners:
    // All Model file process corners will  be extracted
    double version = 0.0;
    std::list<std::string> corner_list
        = extModelGen::GetCornerNames(rulesFileName, version, logger_);
    std::list<std::string>::iterator it;
    for (it = corner_list.begin(); it != corner_list.end(); ++it) {
      std::string str = *it;
      addRCCorner(str.c_str(), extCornerDbCnt, 0);
      _modelMap.add(extCornerDbCnt);
      extCornerDbCnt++;
    }
  }
  return m;
}
uint extMain::getResCapTable_lefRC_v2()
{
  uint cnt = 0;
  for (dbTechLayer* layer : _tech->getLayers()) {
    if (layer->getRoutingLevel() == 0) {
      continue;
    }
    const uint n = layer->getRoutingLevel();
    const uint w = layer->getWidth();  // nm
    _minWidthTable[n] = w;

    uint sp = layer->getSpacing();  // nm
    _minDistTable[n] = sp;
    if (sp == 0) {
      sp = layer->getPitch() - layer->getWidth();
      _minDistTable[n] = sp;
    }
    float cap2 = layer->getCapacitance();
    float edge_cap = layer->getEdgeCapacitance();
    float w_microns = _block->dbuToMicrons(layer->getWidth());
    float len_microns_1 = _block->dbuToMicrons(1);

    float cap_fF_per_nm_1
        = w_microns * len_microns_1 * cap2 * 1000 + 2 * edge_cap;

    _capacitanceTable[0][n] = cap_fF_per_nm_1;
    _minCapTable[n][0] = cap_fF_per_nm_1;

    const double res = layer->getResistance();  // OHMS per square
    _resistanceTable[0][n] = res / w;

    debugPrint(logger_,
               RCX,
               "extrules",
               1,
               "EXT_RES_LEF: R Layer= {} met= {}  lef_res= {:g}",
               layer->getConstName(),
               n,
               res);
  }
  cnt++;
  return cnt;
}

}  // namespace rcx
