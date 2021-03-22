///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include <dbExtControl.h>
#include <math.h>

#include <algorithm>

#include "OpenRCX/extRCap.h"
#include "OpenRCX/extSpef.h"
#include "parse.h"
#include "utl/Logger.h"

//#ifdef _WIN32
#define ATH__fprintf fprintf
#define ATH__fopen fopen
#define ATH__fclose fclose
//#endif

namespace rcx {

using utl::RCX;

uint extSpef::writeHierInstNameMap()
{
  odb::dbSet<odb::dbInst> insts = _block->getInsts();
  odb::dbSet<odb::dbInst>::iterator itr;

  uint instCnt = 0;
  for (itr = insts.begin(); itr != insts.end(); ++itr) {
    odb::dbInst* inst = *itr;
    odb::dbBlock* child = inst->getChild();

    if (child == NULL)
      continue;

    odb::dbIntProperty::create(child, "_instSpefMapBase", _baseNameMap);
    int baseMapId = extSpef::getIntProperty(child, "_instSpefMapBase");
    debugPrint(logger_,
               RCX,
               "hierspef",
               1,
               "[HEXT:S]"
               "\ninstMapBase {} : {}  {}",
               baseMapId,
               child->getConstName(),
               inst->getConstName());

    uint mapId = 0;
    odb::dbSet<odb::dbInst> binsts = child->getInsts();
    odb::dbSet<odb::dbInst>::iterator bitr;
    for (bitr = binsts.begin(); bitr != binsts.end(); ++bitr) {
      odb::dbInst* ii = *bitr;

      mapId = _baseNameMap + ii->getId();

      char* nname = (char*) ii->getConstName();
      char* nname1 = tinkerSpefName(nname);
      ATH__fprintf(_outFP, "*%d %s/%s\n", mapId, inst->getConstName(), nname1);
      debugPrint(logger_,
                 RCX,
                 "hierspef",
                 1,
                 "[HEXT:S]",
                 "\t{} {}/{}",
                 mapId,
                 inst->getConstName(),
                 nname1);

      instCnt++;
    }
    _baseNameMap = mapId;
    debugPrint(logger_,
               RCX,
               "hierspef",
               1,
               "[HEXT:S]"
               "END_MAP {} inames : {}\n",
               instCnt,
               child->getConstName());
  }
  return instCnt;
}
int extSpef::getIntProperty(odb::dbBlock* block, const char* name)
{
  odb::dbProperty* p = odb::dbProperty::find(block, name);

  if (p == NULL)
    return 0;

  odb::dbIntProperty* ip = (odb::dbIntProperty*) p;
  return ip->getValue();
}

uint extSpef::writeHierNetNameMap()
{
  odb::dbSet<odb::dbInst> insts = _block->getInsts();
  odb::dbSet<odb::dbInst>::iterator itr;

  uint netCnt = 0;
  for (itr = insts.begin(); itr != insts.end(); ++itr) {
    odb::dbInst* inst = *itr;
    odb::dbBlock* child = inst->getChild();

    if (child == NULL)
      continue;

    odb::dbIntProperty::create(child, "_netSpefMapBase", _baseNameMap);
    int baseMapId = extSpef::getIntProperty(child, "_netSpefMapBase");

    debugPrint(logger_,
               RCX,
               "hierspef",
               1,
               "[HEXT:S]"
               "\nnetMapBase {} : {}  {}",
               baseMapId,
               child->getConstName(),
               inst->getConstName());

    uint mapId = 0;
    odb::dbSet<odb::dbNet> nets = child->getNets();
    odb::dbSet<odb::dbNet>::iterator bitr;
    for (bitr = nets.begin(); bitr != nets.end(); ++bitr) {
      odb::dbNet* ii = *bitr;

      if (ii->getBTermCount() > 0)
        continue;

      mapId = _baseNameMap + ii->getId();

      char* nname = (char*) ii->getConstName();
      char* nname1 = tinkerSpefName(nname);
      ATH__fprintf(_outFP, "*%d %s/%s\n", mapId, inst->getConstName(), nname1);
      debugPrint(logger_,
                 RCX,
                 "hierspef",
                 1,
                 "[HEXT:S]",
                 "\t{} {}/{}",
                 mapId,
                 inst->getConstName(),
                 nname1);
      netCnt++;
    }
    _baseNameMap = mapId;
    debugPrint(logger_,
               RCX,
               "hierspef",
               1,
               "[HEXT:S]"
               "\tNET_MAP_END ({} internal nets) : {}\n",
               netCnt,
               child->getConstName());
  }
  // OPTIMIZE this ; have to do due to 1st "null" rseg;
  // if not, appending rsegs creates a havoc!

  odb::dbSet<odb::dbNet> topnets = _block->getNets();
  odb::dbSet<odb::dbNet>::iterator bitr;
  for (bitr = topnets.begin(); bitr != topnets.end(); ++bitr) {
    odb::dbNet* ii = *bitr;

    // extMain::printRSegs(ii);
    odb::dbSet<odb::dbRSeg> rSet = ii->getRSegs();
    rSet.reverse();
    // extMain::printRSegs(ii);
  }
  return netCnt;
}
uint extMain::markCCsegs(odb::dbBlock* blk, bool flag)
{
  odb::dbSet<odb::dbCCSeg> ccs = blk->getCCSegs();
  odb::dbSet<odb::dbCCSeg>::iterator ccitr;
  for (ccitr = ccs.begin(); ccitr != ccs.end(); ++ccitr) {
    odb::dbCCSeg* cc = *ccitr;
    cc->setMark(flag);
  }
  return ccs.size();
}

uint extMain::addRCtoTop(odb::dbBlock* blk, bool write_spef)
{
  logger_->info(RCX,
                232,
                "Merging Parasitics for Block {} : {} Into parent {}",
                blk->getConstName(),
                blk->getParentInst()->getConstName(),
                _block->getConstName());

  // blk->getParent()->getConstName()

  int netBaseMapId = extSpef::getIntProperty(blk, "_netSpefMapBase");
  int instBaseMapId = extSpef::getIntProperty(blk, "_instSpefMapBase");
  _spef->setHierBaseNameMap(instBaseMapId, netBaseMapId);

  debugPrint(logger_,
             RCX,
             "hierspef",
             1,
             "[HEXT:F]",
             "\n\t_netSpefMapBase= {} _instSpefMapBase= {}",
             netBaseMapId,
             instBaseMapId);

  odb::dbSet<odb::dbCapNode> allcapnodes = blk->getCapNodes();
  uint blockCapSize = allcapnodes.size();

  uint* capNodeMap = new uint[2 * blockCapSize + 1];

  bool foreign = false;
  uint spefCnt = 0;
  uint flatCnt = 0;
  uint gCnt = 0;
  uint rCnt = 0;
  uint ccCnt = 0;
  double resBound = 0.0;
  uint dbg = 0;

  odb::dbNet* topDummyNodeNet = _block->findNet("dummy_sub_block_cap_nodes");
  if (topDummyNodeNet == NULL)
    topDummyNodeNet = odb::dbNet::create(_block, "dummy_sub_block_cap_nodes");

  odb::dbSet<odb::dbNet> nets = blk->getNets();
  odb::dbSet<odb::dbNet>::iterator bitr;
  for (bitr = nets.begin(); bitr != nets.end(); ++bitr) {
    odb::dbNet* net = *bitr;

    if (!net->setIOflag())
      continue;

    odb::dbBTerm* bterm = net->get1stBTerm();
    odb::dbITerm* iterm = bterm->getITerm();
    odb::dbNet* parentNet = iterm->getNet();
    if (parentNet == NULL) {
      logger_->warn(RCX,
                    233,
                    "Null parent[{}] net : {}",
                    net->getBTermCount(),
                    net->getConstName());
      continue;
    }

    gCnt += createCapNodes(net, parentNet, capNodeMap, instBaseMapId);
    rCnt += createRSegs(net, parentNet, capNodeMap);
    flatCnt++;
  }
  markCCsegs(blk, false);
  for (bitr = nets.begin(); bitr != nets.end(); ++bitr) {
    odb::dbNet* net = *bitr;
    if (!net->isIO())
      continue;

    odb::dbBTerm* bterm = net->get1stBTerm();
    odb::dbITerm* iterm = bterm->getITerm();
    odb::dbNet* parentNet = iterm->getNet();
    if (parentNet == NULL) {
      logger_->warn(RCX,
                    233,
                    "Null parent[{}] net : {}",
                    net->getBTermCount(),
                    net->getConstName());
      continue;
    }

    ccCnt += createCCsegs(
        net, parentNet, topDummyNodeNet, capNodeMap, instBaseMapId);
  }
  markCCsegs(blk, false);

  _spef->setBlock(blk);
  for (bitr = nets.begin(); bitr != nets.end(); ++bitr) {
    odb::dbNet* net = *bitr;
    if (net->isIO())
      continue;

    // extMain::printRSegs(net);
    if (write_spef)
      spefCnt += _spef->writeHierNet(net, resBound, dbg);
  }
  _spef->setBlock(_block);
  logger_->info(RCX,
                235,
                "{} internal {} IO nets {} gCaps {} rSegs {} ccCaps : {}",
                spefCnt,
                flatCnt,
                gCnt,
                rCnt,
                ccCnt,
                blk->getConstName());

  delete[] capNodeMap;
  return flatCnt + spefCnt;
}
uint extMain::adjustParentNode(odb::dbNet* net,
                               odb::dbITerm* from_child_iterm,
                               uint node_num)
{
  odb::dbSet<odb::dbCapNode> capNodes = net->getCapNodes();
  odb::dbSet<odb::dbCapNode>::iterator cap_node_itr = capNodes.begin();
  for (; cap_node_itr != capNodes.end(); ++cap_node_itr) {
    odb::dbCapNode* node = *cap_node_itr;
    if (!node->isITerm())
      continue;
    odb::dbITerm* iterm = node->getITerm();
    if (iterm != from_child_iterm)
      continue;

    node->resetITermFlag();
    node->setInternalFlag();

    node->setNode(node_num);

    return node->getId();
  }
  return 0;
}

bool extMain::createParentCapNode(odb::dbCapNode* node,
                                  odb::dbNet* parentNet,
                                  uint nodeNum,
                                  uint* capNodeMap,
                                  uint baseNum)
{
  bool foreign = false;
  char buf[1024];

  odb::dbCapNode* cap = NULL;
  if (!node->isBTerm()) {  //
    cap = odb::dbCapNode::create(parentNet, nodeNum, foreign);
    capNodeMap[node->getId()] = cap->getId();
  }
  if (node->isInternal()) {  //
    cap->setInternalFlag();

    debugPrint(logger_,
               RCX,
               "hierspef",
               1,
               "[HEXT:C]"
               "\t\tG intrn {} --> {}",
               node->getId(),
               capNodeMap[node->getId()]);
  } else if (node->isITerm()) {  //
    odb::dbITerm* iterm = node->getITerm();
    odb::dbInst* inst = iterm->getInst();
    uint instId = inst->getId();
    sprintf(buf,
            "*%d%s%s",
            baseNum + instId,
            _spef->getDelimeter(),
            iterm->getMTerm()->getConstName());

    odb::dbStringProperty::create(cap, "_inode", buf);
    cap->setNode(baseNum + instId);  // so there is some value
    cap->setNameFlag();

    debugPrint(logger_,
               RCX,
               "hierspef",
               1,
               "[HEXT:C]"
               "\t\tG ITerm {} --> {} : {}",
               node->getId(),
               capNodeMap[node->getId()],
               buf);
  } else if (node->isBTerm()) {  //

    odb::dbBTerm* bterm = node->getBTerm();
    odb::dbITerm* iterm = bterm->getITerm();
    uint parentId = adjustParentNode(parentNet, iterm, nodeNum);
    capNodeMap[node->getId()] = parentId;

    debugPrint(logger_,
               RCX,
               "hierspef",
               1,
               "[HEXT:C]"
               "\t\tG BTerm {} --> {} : {}",
               node->getId(),
               parentId,
               nodeNum);
  }
  return true;
}
void extMain::createTop1stRseg(odb::dbNet* net, odb::dbNet* parentNet)
{
  if (parentNet->getWire() != NULL)
    return;

  odb::dbBTerm* bterm = net->get1stBTerm();
  odb::dbITerm* iterm = bterm->getITerm();

  odb::dbCapNode* cap
      = odb::dbCapNode::create(parentNet, 0, /*_foreign*/ false);
  cap->setNode(iterm->getId());
  cap->setITermFlag();
  odb::dbRSeg* rc = odb::dbRSeg::create(parentNet, 0, 0, 0, true);
  rc->setTargetNode(cap->getId());
}
uint extMain::createCapNodes(odb::dbNet* net,
                             odb::dbNet* parentNet,
                             uint* capNodeMap,
                             uint baseNum)
{
  createTop1stRseg(net, parentNet);

  // have to improve for performance
  uint maxCap = parentNet->maxInternalCapNum() + 1;
  debugPrint(logger_,
             RCX,
             "hierspef",
             1,
             "[HEXT:C]"
             "\n\tCapNodes: maxCap={} : {} {}",
             maxCap,
             net->getConstName(),
             parentNet->getConstName());

  uint gCnt = 0;
  bool foreign = false;
  char buf[1024];
  odb::dbSet<odb::dbCapNode> capNodes = net->getCapNodes();
  odb::dbSet<odb::dbCapNode>::iterator cap_node_itr = capNodes.begin();
  for (; cap_node_itr != capNodes.end(); ++cap_node_itr) {
    odb::dbCapNode* node = *cap_node_itr;

    uint nodeNum = maxCap++;

    gCnt += createParentCapNode(node, parentNet, nodeNum, capNodeMap, baseNum);
  }
  return gCnt;
}
uint extMain::printRSegs(odb::dbNet* net, Logger* logger)
{
  logger->info(RCX, 236, "\t\t\tprintRSegs: {}", net->getConstName());
  odb::dbSet<odb::dbRSeg> rsegs = net->getRSegs();
  uint rsize = rsegs.size();

  uint rCnt = 0;
  odb::dbSet<odb::dbRSeg>::iterator rseg_itr = rsegs.begin();
  for (; rseg_itr != rsegs.end(); ++rseg_itr) {
    odb::dbRSeg* rseg = *rseg_itr;

    logger->info(RCX,
                 234,
                 "\t\t\t\t\trsegId: {} -- {} {}",
                 rseg->getId(),
                 rseg->getSourceNode(),
                 rseg->getTargetNode());
    rCnt++;
  }
  return rCnt;
}
uint extMain::createRSegs(odb::dbNet* net,
                          odb::dbNet* parentNet,
                          uint* capNodeMap)
{
  debugPrint(logger_,
             RCX,
             "hierspef",
             1,
             "[HEXT:R]"
             "\n\tRSegs: {} {}",
             net->getConstName(),
             parentNet->getConstName());

  // extMain::printRSegs(parentNet);

  odb::dbSet<odb::dbRSeg> rsegs = net->getRSegs();
  uint rsize = rsegs.size();

  uint rCnt = 0;
  odb::dbSet<odb::dbRSeg>::iterator rseg_itr = rsegs.begin();
  for (; rseg_itr != rsegs.end(); ++rseg_itr) {
    odb::dbRSeg* rseg = *rseg_itr;

    int x, y;
    rseg->getCoords(x, y);
    uint pathDir = rseg->pathLowToHigh() ? 0 : 1;
    odb::dbRSeg* rc = odb::dbRSeg::create(parentNet, x, y, pathDir, true);

    uint tgtId = rseg->getTargetNode();
    uint srcId = rseg->getSourceNode();

    rc->setSourceNode(capNodeMap[srcId]);
    rc->setTargetNode(capNodeMap[tgtId]);

    for (uint corner = 0; corner < _block->getCornerCount(); corner++) {
      double res = rseg->getResistance(corner);
      double cap = rseg->getCapacitance(corner);

      rc->setResistance(res, corner);
      rc->setCapacitance(cap, corner);
      debugPrint(logger_,
                 RCX,
                 "hierspef",
                 1,
                 "[HEXT:R]"
                 "\t\tsrc:{}->{} - tgt:{}->{} - {}  {}",
                 srcId,
                 capNodeMap[srcId],
                 tgtId,
                 capNodeMap[tgtId],
                 res,
                 cap);
    }
    rCnt++;
  }
  // extMain::printRSegs(parentNet);
  /*
  if (rCnt>0) {
          odb::dbSet<odb::dbRSeg> rSet= parentNet->getRSegs();
          //rSet.reverse();
  }
  */
  return rCnt;
}
void extMain::adjustChildNode(odb::dbCapNode* childNode,
                              odb::dbNet* parentNet,
                              uint* capNodeMap)
{
  char buf[1024];
  uint parentId = capNodeMap[childNode->getId()];
  odb::dbCapNode* parentNode = odb::dbCapNode::getCapNode(_block, parentId);

  if (childNode->isInternal() || childNode->isBTerm()) {  //
    childNode->resetInternalFlag();
    childNode->resetBTermFlag();

    // spefParentNameId == parent netId (just lucky!!)
    sprintf(buf,
            "*%d%s%d",
            parentNet->getId(),
            _spef->getDelimeter(),
            parentNode->getNode());

    odb::dbStringProperty::create(childNode, "_inode", buf);

    debugPrint(logger_,
               RCX,
               "hierspef",
               1,
               "[HEXT:CC]"
               "\t\tCC intNode {} --> {} : {}",
               childNode->getId(),
               parentId,
               buf);
  } else if (childNode->isITerm()) {  //
    childNode->resetITermFlag();

    odb::dbStringProperty* p
        = odb::dbStringProperty::find(parentNode, "_inode");
    odb::dbStringProperty::create(childNode, "_inode", p->getValue().c_str());

    debugPrint(logger_,
               RCX,
               "hierspef",
               1,
               "[HEXT:CC]"
               "\t\tCC ITermNode {} --> {} : {}",
               childNode->getId(),
               parentId,
               p->getValue().c_str());
  }
  childNode->setNameFlag();
}

uint extMain::createCCsegs(odb::dbNet* net,
                           odb::dbNet* parentNet,
                           odb::dbNet* topDummyNet,
                           uint* capNodeMap,
                           uint baseNum)
{
  // IO nets

  // have to improve for performance
  uint maxCap = parentNet->maxInternalCapNum() + 1;

  debugPrint(logger_,
             RCX,
             "hierspef",
             1,
             "[HEXT:CC]"
             "\tCCsegs: maxCap[{}] {} {}",
             maxCap,
             net->getConstName(),
             parentNet->getConstName());

  odb::dbBlock* pblock = parentNet->getBlock();
  uint ccCnt = 0;

  odb::dbSet<odb::dbCapNode> nodeSet = net->getCapNodes();
  odb::dbSet<odb::dbCapNode>::iterator rc_itr;
  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    odb::dbCapNode* srcCapNode = *rc_itr;

    odb::dbSet<odb::dbCCSeg> ccsegs = srcCapNode->getCCSegs();
    odb::dbSet<odb::dbCCSeg>::iterator ccseg_itr = ccsegs.begin();
    for (; ccseg_itr != ccsegs.end(); ++ccseg_itr) {
      odb::dbCCSeg* cc = *ccseg_itr;

      if (cc->isMarked())
        continue;

      odb::dbCapNode* dstCapNode = cc->getTargetCapNode();
      if (cc->getSourceCapNode() != srcCapNode) {
        dstCapNode = cc->getSourceCapNode();
      }
      if (!dstCapNode->getNet()->isIO()) {
        uint nodeNum = maxCap++;
        createParentCapNode(
            dstCapNode, topDummyNet, nodeNum, capNodeMap, baseNum);
        adjustChildNode(srcCapNode, parentNet, capNodeMap);
      }
      uint tId = dstCapNode->getId();
      uint sId = srcCapNode->getId();
      uint tgtId = capNodeMap[tId];
      uint srcId = capNodeMap[sId];

      odb::dbCapNode* map_tgt = odb::dbCapNode::getCapNode(pblock, tgtId);
      odb::dbCapNode* map_src = odb::dbCapNode::getCapNode(pblock, srcId);

      odb::dbCCSeg* ccap = odb::dbCCSeg::create(map_src, map_tgt, false);

      for (uint corner = 0; corner < _block->getCornerCount(); corner++) {
        double cap = cc->getCapacitance(corner);
        ccap->setCapacitance(cap, corner);

        debugPrint(logger_,
                   RCX,
                   "hierspef",
                   1,
                   "[HEXT:C]"
                   "\t\tCC src:{}->{} tgt:{}->{} CC {:g}\n",
                   sId,
                   srcId,
                   tId,
                   tgtId,
                   cap);
      }
      cc->setMark(true);
    }
  }
  return maxCap;
}
/*
uint extMain::adjustCCsegs(odb::dbNet *net, uint baseNum)
{
        //Internal nets

        // have to improve for performance
        uint maxCap= parentNet->maxInternalCapNum()+1;

        debugPrint(logger_, RCX, "hierspef", 1,
               "[HEXT:F]"
                "\tCCsegs: maxCap[{}] {} {}\n", maxCap,
                net->getConstName(), parentNet->getConstName());

        odb::dbBlock *pblock= parentNet->getBlock();
        uint ccCnt= 0;

        odb::dbSet<odb::dbCapNode> nodeSet = net->getCapNodes();
        odb::dbSet<odb::dbCapNode>::iterator rc_itr;
        for( rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr ) {
                odb::dbCapNode *srcCapNode = *rc_itr;

                odb::dbSet<odb::dbCCSeg> ccsegs= srcCapNode->getCCSegs();
                odb::dbSet<odb::dbCCSeg>::iterator ccseg_itr= ccsegs.begin();
                for( ; ccseg_itr != ccsegs.end(); ++ccseg_itr ) {
                        odb::dbCCSeg *cc= *ccseg_itr;

                        if (cc->isMarked())
                                continue;

                                adjustChildNode(srcCapNode, parentNet,
capNodeMap); cc->setMark(true);
                }
        }
        return maxCap;
}
*/
void extSpef::writeDnetHier(uint mapId, double* totCap)
{
  if (_writeNameMap)
    ATH__fprintf(_outFP, "\n*D_NET *%d ", mapId);
  // else
  // ATH__fprintf(_outFP, "\n*D_NET %s ", tinkerSpefName((char
  // *)_d_net->getConstName()));
  writeRCvalue(totCap, _cap_unit);
  ATH__fprintf(_outFP, "\n");
}

bool extSpef::writeHierNet(odb::dbNet* net, double resBound, uint dbg)
{
  _block = net->getBlock();
  _d_net = net;
  uint netId = net->getId();
  _cornerBlock = net->getBlock();

  _cornersPerBlock = _cornerCnt;

  // if (_cornerBlock && _cornerBlock!=_block)
  // net= odb::dbNet::getNet(_cornerBlock, netId);

  uint minNode;
  uint capNodeCnt = getMinCapNode(net, &minNode);
  if (capNodeCnt == 0) {
    logger_->warn(RCX, 237, "No cap nodes net: {}", net->getConstName());
    return false;
  }
  odb::dbSet<odb::dbRSeg> rcSet = net->getRSegs();

  _cCnt = 1;
  // must set at calling _symmetricCCcaps= true; //TODO non symmetric case??
  // must set at calling _preserveCapValues= false // true only for reading in
  // SPEF

  double totCap[5];
  resetCap(totCap);
  if (_symmetricCCcaps)
    addCouplingCaps(net, totCap);

  _firstCapNode = minNode - 1;

  reinitCapTable(_nodeCapTable, capNodeCnt + 2);

  if (_singleP)
    computeCapsAdd2Target(rcSet, totCap);
  else
    computeCaps(rcSet, totCap);

  debugPrint(logger_,
             RCX,
             "hierspef",
             1,
             "[HEXT:S]"
             "\tDNET *{}-{} {:g} {}\n",
             _childBlockNetBaseMap + netId,
             netId,
             totCap[0],
             net->getConstName());

  // netId= _childBlockNetBaseMap+netId;
  writeDnetHier(_childBlockNetBaseMap + netId, totCap);

  if (_wConn) {
    writeKeyword("*CONN");
    writePorts(net);
    writeITerms(net);
  }
  if (_writingNodeCoords == C_STARRC)
    writeNodeCoords(netId, rcSet);

  if (_wCap || _wOnlyCCcap)
    writeKeyword("*CAP");

  if (_wCap && !_wOnlyCCcap) {
    writeCapPorts(net);
    writeCapITerms(net);
    writeNodeCaps(net, _childBlockNetBaseMap + netId);
  }

  if (_wCap || _wOnlyCCcap)
    writeSrcCouplingCaps(net);

  if (_symmetricCCcaps && (_wCap || _wOnlyCCcap))
    writeTgtCouplingCaps(net);

  if (_wRes) {
    writeRes(netId, rcSet);
  }
  writeKeyword("*END");

  odb::dbSet<odb::dbCapNode> nodeSet = net->getCapNodes();
  odb::dbSet<odb::dbCapNode>::iterator rc_itr;
  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    odb::dbCapNode* node = *rc_itr;
    node->setSortIndex(0);
  }
  return true;
}
void extSpef::setHierBaseNameMap(uint instBase, uint netBase)
{
  _childBlockNetBaseMap = netBase;
  _childBlockInstBaseMap = instBase;
}

}  // namespace rcx
