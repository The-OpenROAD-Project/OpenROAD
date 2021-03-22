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
#ifdef EXT_SI
#include "../tmg/tmg_db.h"
#endif
#include <math.h>

#include <algorithm>

#include "dbExtControl.h"
#include "dbSearch.h"
#include "OpenRCX/exttree.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

#ifdef EXT_SI
void initExtSi(dbNet* victim, bool is_min, bool is_rise, tmg_db* tm);
#endif

void extRcTree::free_exttree(extTnode* driver)
{
  int i;
  for (i = 0; i < (int) (driver->_childCnt); i++) {
    free_exttree((driver->_child)[i]);
  }
  if (driver->_childCnt)
    delete[] driver->_child;
  delete driver;
}

extRcTree::extRcTree(odb::dbBlock* blk, Logger* logger)
{
  logger_ = logger;
  _block = blk;
  _cornerCnt = blk->getCornerCount();
  _net = NULL;

  _rcPool = new AthPool<extRCnode>(false, 4096);

  _junctionNodeTable = new Ath__array1D<extRCnode*>(1024);
  _nodeTable = new Ath__array1D<extRCnode*>(1024);
  _indexTable = new Ath__array1D<int>(1024);
  _itermIndexTable = new Ath__array1D<int>(1024);
  _btermIndexTable = new Ath__array1D<int>(1024);
  _map = new Ath__array1D<int>(128000);
  _debug = false;
  _printChildN = true;

  _tnodeTable = NULL;
}
extRcTree::~extRcTree()
{
  delete _rcPool;

  delete _junctionNodeTable;
  delete _nodeTable;
  delete _indexTable;
  delete _itermIndexTable;
  delete _btermIndexTable;
  delete _map;

  if (_tnodeTable != NULL)
    delete[] _tnodeTable;
}
void extRCnode::reset(uint cornerCnt)
{
  uint ii;
  for (ii = 0; ii < cornerCnt; ii++) {
    _gndcap[ii] = 0;
    _cap[ii] = 0;
    _res[ii] = 0;
  }
  _x = 0;
  _y = 0;
  _firstChild = 0;
  _termMap = 0;
  _netId = 0;
  _capndId = 0;
  _splitCnt = 0;
  _junctionId = 0;
}
extTnode::extTnode(extRCnode* m, uint cornerCnt)
{
  uint ii;
  for (ii = 0; ii < cornerCnt; ii++) {
    _gndcap[ii] = m->_gndcap[ii];
    _cap[ii] = m->_cap[ii];
    _res[ii] = m->_res[ii];
  }
  _x = m->_x;
  _y = m->_y;
  _termMap = m->_termMap;
  _netId = m->_netId;
  _capndId = m->_capndId;
  _splitCnt = 1;
  _junctionId = m->_junctionId;
  _child = NULL;
}
void extTnode::printTnodes(char* type, uint cornerCnt)
{
  FILE* fp;
  char fn[40];
  if (type) {
    sprintf(fn, "%s_net%d_tnode", type, _netId);
    fp = fopen(fn, "w");
  } else
    fp = stdout;
  fprintf(fp, "extTnodes of Net %d:\n", _netId);
  extTnode* stackV[4096];
  extTnode* parentNode[4096];
  int pN = 0;
  stackV[0] = this;
  int sequence[4096];
  int seqn = 1;
  sequence[0] = seqn++;
  int stackN = 1;
  int k;
  int csq;
  uint ii;
  extTnode* node;
  fprintf(fp,
          "NetId %d Node graph from extTnodes --------------------\n\n",
          _netId);
  while (stackN) {
    node = stackV[stackN - 1];
    if (pN > 0 && node == parentNode[pN - 1]) {
      pN--;
      stackN--;
      continue;
    }
    csq = sequence[stackN - 1];
    fprintf(fp, "node %5d : ", csq);
    fprintf(fp,
            "\tX=%d Y=%d net=%d capnd=%d splitCnt=%d\n",
            node->_x,
            node->_y,
            node->_netId,
            node->_capndId,
            node->_splitCnt);
    fprintf(fp,
            "\t\ttermMap= %d, junctionId= %d,",
            node->_termMap,
            node->_junctionId);
    if (node->_childCnt) {
      parentNode[pN++] = node;
      fprintf(fp, " children:");
      for (k = 0; k < (int) node->_childCnt; k++) {
        fprintf(fp, " %d", seqn);
        sequence[stackN] = seqn++;
        stackV[stackN++] = node->_child[k];
      }
    }
    fprintf(fp, "\n");
    for (ii = 0; ii < cornerCnt; ii++)
      fprintf(fp,
              "\tR_%d= %g  totalC_%d= %g gndC_%d= %g ccC_%d= %g \n",
              ii,
              node->_res[ii],
              ii,
              node->_cap[ii],
              ii,
              node->_gndcap[ii],
              ii,
              node->_cap[ii] - node->_gndcap[ii]);
    fprintf(fp, "\n");
    if (node->_childCnt == 0)
      stackN--;
  }
  fprintf(fp, "NetId %d has %d nodes\n\n", _netId, seqn - 1);
  if (type)
    fclose(fp);
}
void extRcTree::getCoords(odb::dbNet* net,
                          uint shapeId,
                          int* x1,
                          int* y1,
                          int* x2,
                          int* y2)
{
  odb::dbShape s;
  odb::dbWire* w = net->getWire();
  w->getShape(shapeId, s);
  *x1 = s.xMin();
  *y1 = s.yMin();
  *x2 = s.xMax();
  *y2 = s.yMax();
}
uint extRcTree::netLocalCn(uint capNodeNum)
{
  uint bot = 0;
  uint top = _netCapNodeNum.size() - 1;
  uint idx;
  while (1) {
    idx = (top + bot) / 2;
    if (_netCapNodeNum[idx] == capNodeNum)
      return _netCapNodeMapBack[idx];
    if (idx == bot) {
      assert(_netCapNodeNum[top] == capNodeNum);
      return _netCapNodeMapBack[top];
    }
    if (_netCapNodeNum[idx] > capNodeNum)
      top = idx - 1;
    else
      bot = idx + 1;
  }
}
class compareCnNum
{
 public:
  bool operator()(uint num1, uint num2) { return (num1 < num2 ? true : false); }
};

void extRcTree::initLocalCapNodeTable(odb::dbSet<odb::dbRSeg>& rSet)
{
  _cncpy.clear();
  _netCapNodeNum.clear();
  odb::dbSet<odb::dbRSeg>::iterator rc_itr;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    odb::dbRSeg* rc = *rc_itr;
    uint srcId = rc->getSourceNode();
    uint tgtId = rc->getTargetNode();
    odb::dbCapNode* srcNode = odb::dbCapNode::getCapNode(_cornerBlock, srcId);
    odb::dbCapNode* tgtNode = odb::dbCapNode::getCapNode(_cornerBlock, tgtId);

    /*
    if ( isDangling(srcNode) || isDangling(tgtNode)) {
            fprintf(stdout, "shape= %d %d (%d-%d) ---> %d (%d-%d)\n",
    rc->getShapeId(), srcId, srcNode->getChildrenCnt(), srcNode->isTreeNode(),
                    tgtId, tgtNode->getChildrenCnt(), tgtNode->isTreeNode());
    }
    */
    if (!srcNode->isSelect()) {
      _netCapNodeNum.push_back(srcId);
      _cncpy.push_back(srcId);
    }
    srcNode->setSelect(true);
    if (!tgtNode->isSelect()) {
      _netCapNodeNum.push_back(tgtId);
      _cncpy.push_back(tgtId);
    }
    tgtNode->setSelect(true);
  }
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    odb::dbRSeg* rc = *rc_itr;
    uint srcId = rc->getSourceNode();
    uint tgtId = rc->getTargetNode();
    odb::dbCapNode* srcNode = odb::dbCapNode::getCapNode(_cornerBlock, srcId);
    odb::dbCapNode* tgtNode = odb::dbCapNode::getCapNode(_cornerBlock, tgtId);
    srcNode->setSelect(false);
    tgtNode->setSelect(false);
  }
  std::sort(_netCapNodeNum.begin(), _netCapNodeNum.end(), compareCnNum());
  _netCapNodeMapBack.clear();
  uint jj, k1, k2;
  for (jj = 0; jj < _netCapNodeNum.size(); jj++) {
    if (_cncpy[jj] != _netCapNodeNum[jj])
      break;
    _netCapNodeMapBack.push_back(jj);
  }
  if (jj == _netCapNodeNum.size())
    return;
  for (k1 = jj; k1 < _netCapNodeNum.size(); k1++) {
    for (k2 = jj; k2 < _netCapNodeNum.size(); k2++) {
      if (_netCapNodeNum[k1] == _cncpy[k2]) {
        _netCapNodeMapBack.push_back(k2);
        break;
      }
    }
  }
}

extRCnode* extRcTree::init(odb::dbRSeg* zrc,
                           odb::dbRSeg* rc,
                           odb::dbSet<odb::dbRSeg>& rSet,
                           bool recycleFlag,
                           uint* id)
{
  uint resSize = rSet.size();
  initLocalCapNodeTable(rSet);
  odb::dbCapNode* capNode
      = odb::dbCapNode::getCapNode(_cornerBlock, rc->getSourceNode());

  uint startingNodeId = netLocalCn(rc->getSourceNode());

  _btermId = 0;
  _itermId = 0;
  if (capNode->isBTerm())
    _btermId = capNode->getNode();
  else if (capNode->isITerm())
    _itermId = capNode->getNode();

  for (uint ii = 0; ii < resSize + 1; ii++) {
    _junctionNodeTable->set(ii, NULL);
  }

  _firstBTermIndex = 0;
  _firstITermIndex = 0;
  _itermCnt = 0;
  _btermCnt = 0;
  if (recycleFlag) {
    for (uint ii = 0; ii < _nodeTable->getCnt(); ii++)
      _rcPool->free(_nodeTable->get(ii));
    _nodeTable->resetCnt();

    _indexTable->resetCnt();
    _itermIndexTable->resetCnt();
    _btermIndexTable->resetCnt();
    _btermIndexTable->add(0);
    _itermIndexTable->add(0);
  }
  _firstBTermIndex = _btermIndexTable->getCnt();
  _firstITermIndex = _itermIndexTable->getCnt();

  // uint childCnt= capNode->getChildrenCnt()+1;
  capNode->getChildrenCnt();

  allocNode(0, id);

  _driverNodeId = _nodeTable->getCnt();

  extRCnode* node = makeFirstNode(zrc, rc, capNode, startingNodeId);

  return node;
}
uint extRcTree::getDriverITermId()
{
  return _itermId;
}
uint extRcTree::getDriverBTermId()
{
  return _btermId;
}
uint extRcTree::makeChildren(extRCnode* node, uint childrenCnt)
{
  node->_firstChild = _indexTable->add(0);
  for (uint ii = 1; ii < childrenCnt + 1; ii++)
    _indexTable->add(0);

  return node->_firstChild;
}
extRCnode* extRcTree::allocNode(uint childrenCnt,
                                uint* id,
                                bool allocateChildren)
{
  extRCnode* node = _rcPool->alloc();
  assert(node);
  node->reset(_cornerCnt);

  // if (childrenCnt>0)
  //	*id= _nodeTable->add(node);
  // else
  //	*id= _nodeTable->add(NULL);

  *id = _nodeTable->add(node);

  if (allocateChildren)
    makeChildren(node, childrenCnt);

  return node;
}
uint extRcTree::addChild(extRCnode* node, uint child)
{
  for (uint ii = node->_firstChild;; ii++) {
    if (_indexTable->get(ii) == 0) {
      _indexTable->set(ii, child);
      return ii;
    }
  }
  return 0;
}
uint extRcTree::getChildrenCnt(uint firstChild)
{
  uint cnt = 0;
  for (uint jj = firstChild;; jj++) {
    if (_indexTable->get(jj) == 0)
      return cnt;
    cnt++;
  }
  return cnt;
}
extTnode* extRcTree::makeTnode(uint nodeId, uint& n)
{
  extTnode* tnode = NULL;
  extRCnode* node = _nodeTable->get(nodeId);

  uint mapId = _map->geti(nodeId);
  if (mapId > 0) {
    tnode = _tnodeTable[mapId];
  } else {
    //_map[nodeId]= n;
    _map->set(nodeId, n);
    tnode = new extTnode(node, _cornerCnt);
    _tnodeTable[n++] = tnode;

    tnode->_childCnt = getChildrenCnt(node->_firstChild);
    if (tnode->_childCnt)
      tnode->_child = new extTnode*[tnode->_childCnt];

    for (uint ii = 0; ii < tnode->_childCnt; ii++)
      tnode->_child[ii] = NULL;
  }

  return tnode;
}
uint extRcTree::makeGraph(uint netId)
{
  for (uint jj = 0; jj < _nodeTable->getCnt() + 1; jj++) {
    //_map[jj]= 0;
    _map->set(jj, 0);
  }
  if (_tnodeTable != NULL)
    delete[] _tnodeTable;

  _tnodeTable = new extTnode*[_nodeTable->getCnt() + 1];

  ////double totCap[10]= 0.0;
  uint n = 0;
  for (uint ii = 1; ii < _nodeTable->getCnt(); ii++) {
    extTnode* tnode = makeTnode(ii, n);
    ////for (uint jj = 0; jj < _cornerCnt; jj++)
    ////totCap[jj] += tnode->_cap[jj];

    uint k = 0;
    for (uint jj = _nodeTable->get(ii)->_firstChild;; jj++) {
      uint nodeId = _indexTable->get(jj);

      if (nodeId == 0)
        break;

      makeTnode(nodeId, n);

      tnode->_child[k++] = _tnodeTable[_map->geti(nodeId)];
    }
  }
  if (_debug) {
    //		double dbTotCap= odb::dbNet::getNet(_block,
    // netId)->getTotalCapacitance();
    // logger_->info(RCX, 0, "Total Cap= {}
    // (tree) vs. {} Rsegs", totCap, dbTotCap);
  }
  return n;
}
void extRcTree::printTree(FILE* fp, uint netId, uint tnodeCnt)
{
  fprintf(fp, "\n\nnetId %d  has %d nodes\n", netId, tnodeCnt);

  for (uint ii = 1; ii < tnodeCnt; ii++) {
    fprintf(fp, "node %5d : ", ii);

    extTnode* node = _tnodeTable[ii];
    for (uint jj = 0; jj < _cornerCnt; jj++)
      fprintf(fp, "\t\t\t\t\tR= %5g  C= %5g\n", node->_res[jj], node->_cap[jj]);
    fprintf(fp, "\t\t\t\t\tX= %d Y= %d\n", node->_x, node->_y);
  }
}
uint extRcTree::printTree(FILE* fp, uint netId, const char* msg)
{
  fprintf(fp, "\n%s ------------------------------\n", msg);
  fprintf(fp, "\nnetId %d  has %d nodes\n", netId, _nodeTable->getCnt());

  for (uint ii = 1; ii < _nodeTable->getCnt(); ii++) {
    fprintf(fp, "node %5d : ", ii);

    extRCnode* node = _nodeTable->get(ii);
    for (uint jj = node->_firstChild;; jj++) {
      if (_indexTable->get(jj) == 0)
        break;
      fprintf(fp, "\t%d", _indexTable->get(jj));
    }
    // fprintf(fp, "\n");
    for (uint jj = 0; jj < _cornerCnt; jj++)
      fprintf(fp, "\t\t\t\t\tR= %g  C= %g\n", node->_res[jj], node->_cap[jj]);
    fprintf(fp, "\t\t\t\t\tX= %d Y= %d\n", node->_x, node->_y);
  }
  return _nodeTable->getCnt();
}

int extRcTree::dfs(uint i, int* vis, odb::dbNet* net, uint l)
{
  if (vis[i] != -1) {
    logger_->warn(RCX,
                  160,
                  "Node {} in net {} {} has two parents {}, and {}",
                  i,
                  net->getId(),
                  net->getConstName(),
                  vis[i],
                  l);
    return 0;
  }
  vis[i] = l;
  extRCnode* n = _nodeTable->get(i);
  if (!n)
    return 1;
  for (uint j = n->_firstChild;; j++) {
    uint k = _indexTable->get(j);
    if (k == 0)
      break;
    if (!dfs(k, vis, net, i))
      return 0;
  }
  return 1;
}

bool extRcTree::isTree(odb::dbNet* net)
{
  int* visited = (int*) malloc(_nodeTable->getCnt() * sizeof(int));
  uint j = 0;
  for (j = 0; j < _nodeTable->getCnt(); j++)
    visited[j] = -1;
  j = 0;
  for (uint ii = 1; ii < _nodeTable->getCnt(); ii++) {
    if (visited[ii] != -1)
      continue;
    j++;
    if (j > 1) {
      logger_->warn(RCX,
                    161,
                    "Routing corresponding to net {} {} is not connected",
                    net->getId(),
                    net->getConstName());
      return false;
    }
    if (!dfs(ii, visited, net, ii))
      return false;
  }
  free(visited);
  return true;
}
void extRcTree::duplicateJunction(extRCnode* node, uint cnt)
{
  uint jnodeId = 0;
  extRCnode* jnode = allocNode(cnt, &jnodeId, false);  // children

  jnode->_firstChild = node->_firstChild;
  for (uint jj = 0; jj < _cornerCnt; jj++) {
    jnode->_res[jj] = 0.0;
    jnode->_cap[jj] = 0.0;
    jnode->_gndcap[jj] = 0.0;
  }
  jnode->_x = node->_x;
  jnode->_y = node->_y;
  // jnode->_termMap = node->_termMap;
  jnode->_netId = node->_netId;
  jnode->_capndId = node->_capndId;
  jnode->_splitCnt = node->_splitCnt;
  jnode->_junctionId = node->_junctionId;

  makeChildren(node, 1);
  addChild(node, jnodeId);
}
uint extRcTree::getChildrenCnt(extRCnode* node)
{
  uint cnt = 0;
  for (uint jj = node->_firstChild;; jj++) {
    if (_indexTable->get(jj) == 0)
      break;
    cnt++;
  }
  return cnt;
}
uint extRcTree::insertZeroJunctions()
{
  uint nodeCnt = _nodeTable->getCnt();
  if (nodeCnt <= 0)
    return 0;

  extRCnode* driver_node = _nodeTable->get(_driverNodeId);
  duplicateJunction(driver_node, getChildrenCnt(driver_node));

  for (uint ii = _driverNodeId + 1; ii < nodeCnt; ii++) {
    extRCnode* node = _nodeTable->get(ii);
    uint cnt = getChildrenCnt(node);
    if (cnt < 2)
      continue;

    duplicateJunction(node, cnt);
  }
  return _nodeTable->getCnt();
}
extTnode* extRcTree::makeTree(uint netId,
                              double max_cap,
                              uint test,
                              bool resetFlag,
                              bool addDummyJunctions,
                              uint& tnodeCnt,
                              double mcf,
                              char* printTag,
                              bool for_buffering)
{
  odb::dbNet* net = odb::dbNet::getNet(_block, netId);
  _debug = false;
  if (netId == 166)
    _debug = true;
  extTnode* node = makeTree(net,
                            max_cap,
                            test,
                            addDummyJunctions,
                            true,
                            tnodeCnt,
                            mcf,
                            for_buffering);
  if (printTag && strlen(printTag)) {
    if (node)
      node->printTnodes(printTag, _cornerCnt);
    else
      logger_->warn(RCX, 162, "Failed to make rcTree for net {}.", netId);
  }
  return node;
}
// void extRcTree::setDriverXY()
//{
//	int X=0;
//	int Y=0;
//	if (_btermId>0) {
//		odb::dbBTerm *bterm= odb::dbBTerm::getBTerm(_block, _btermId);
//		bterm->getFirstPinLocation(X, Y); // TWG: added bpins
//	}
//	else if (_itermId>0) {
//		odb::dbITerm *iterm= odb::dbITerm::getITerm(_block, _itermId);
//		iterm->getAvgXY(&X, &Y);
//	}
//	extRCnode *driver_node= _nodeTable->get(_driverNodeId);
//	driver_node->_x= X;
//	driver_node->_y= Y;
//        if((X == 0) && (Y == 0))
//          logger_->info(RCX, 0, "The driver is at 0, 0");
//}

extRCnode* extRcTree::makeFirstNode(odb::dbRSeg* zrc,
                                    odb::dbRSeg* rc,
                                    odb::dbCapNode* capNode,
                                    uint index)
{
  uint nodeId = 0;

  // uint childCnt= capNode->getChildrenCnt()+1;
  uint childCnt = capNode->getChildrenCnt();

  extRCnode* node = allocNode(childCnt, &nodeId);

  int X = 0;
  int Y = 0;
  node->_junctionId = 0;
  if (capNode->isITerm()) {
    _itermIndexTable->set(nodeId, capNode->getNode());
    node->_termMap = capNode->getNode();
    if (_net->getWire())
      node->_junctionId = _net->getWire()->getTermJid(node->_termMap);
    //		odb::dbITerm *iterm= odb::dbITerm::getITerm(_block, _itermId);
    //		if (!iterm->getAvgXY(&X, &Y)) {
    //			logger_->warn(RCX, 0, "Can not locate iterm {}/{}",
    // iterm->getInst()->getConstName(), iterm->getMTerm()->getConstName());
    //			return NULL;
    //		}
  } else if (capNode->isBTerm()) {
    _btermIndexTable->set(nodeId, capNode->getNode());
    node->_termMap = -capNode->getNode();
    if (_net->getWire())
      node->_junctionId = _net->getWire()->getTermJid(node->_termMap);
    //		odb::dbBTerm *bterm= odb::dbBTerm::getBTerm(_block, _btermId);
    //		if (!bterm->getFirstPinLocation(X, Y)) { // TWG: added bpins
    //			logger_->warn(RCX, 0, "Can not locate bterm {}",
    // bterm->getConstName()); 			return NULL;
    //		}
  } else {
    node->_junctionId = capNode->getNode();
    if (node->_junctionId == 0)
      node->_junctionId = rc->getShapeId();
    //		_net->getWire()->getCoord(node->_junctionId, X, Y);
  }
  node->_netId = _net->getId();
  node->_capndId = capNode->getId();
  node->_splitCnt = 1;
  zrc->getCoords(X, Y);
  node->_x = X;
  node->_y = Y;
  if ((X == 0) & (Y == 0))
    logger_->info(RCX, 163, "The node is at 0, 0");
  _junctionNodeTable->set(index, node);

  return node;
}
bool extRcTree::getCoords(odb::dbNet* net,
                          uint shapeId,
                          int* ll,
                          int* ur,
                          uint& length)
{
  odb::dbShape s;
  odb::dbWire* w = net->getWire();
  w->getShape(shapeId, s);

  ll[0] = s.xMin();
  ll[1] = s.yMin();
  ur[0] = s.xMax();
  ur[1] = s.yMax();

  bool horizontal = true;
  length = ur[0] - ll[0];
  if ((int) length < ur[1] - ll[1]) {
    length = ur[1] - ll[1];
    horizontal = false;
  }
  return horizontal;
}

/*
                        if (prevNode!=NULL) {
                                // node->_cap= cap; //TODO: add % if cut in the
   middle of wire
                                // node->_res= res;

                                addChild(prevNode, nodeId);
                                if (tgtNodeFlag)
                                        prevNode= NULL;
                                else
                                        prevNode= node;
                        }
                        else { // tgtNodeFlag || cap>cap_max
                                extRCnode *junctionNode=
   _junctionNodeTable->geti(netLocalCn(firstRC->getSourceNode())); if
   (junctionNode!=NULL) { addChild(junctionNode, nodeId);
                                }
                                prevNode= node;
                                if (tgtNodeFlag)
                                        prevNode= NULL;
                        }
                        if (tgtNodeFlag) {
                                uint index= netLocalCn(lastRC->getTargetNode());
                                _junctionNodeTable->set(index, node);
                                firstRC= NULL;
                        }
*/
extRCnode* extRcTree::makeNode(uint startingNodeId,
                               uint endingNodeId,
                               odb::dbCapNode* tgtNode,
                               bool fractionFlag,
                               int x,
                               int y,
                               double* res,
                               double* gndcap,
                               double* totalcap,
                               extRCnode* prevNode,
                               FILE* dbgFP)
{
  uint nodeId = 0;
  // uint childCnt= tgtNode->getChildrenCnt()+1;
  uint childCnt = tgtNode->getChildrenCnt();
  extRCnode* node = allocNode(childCnt, &nodeId);

  if (prevNode != NULL) {  // Only if cut wire
    addChild(prevNode, nodeId);
  } else {  // tgtNodeFlag || cap>cap_max
    extRCnode* junctionNode = _junctionNodeTable->geti(startingNodeId);
    if (junctionNode != NULL) {
      addChild(junctionNode, nodeId);
    }
  }
  prevNode = NULL;
  if (fractionFlag)
    prevNode = node;
  //	else if (! ((tgtNode->getChildrenCnt()==0)&&(!tgtNode->isTreeNode()))
  //		prevNode= node;

  if (!fractionFlag) {
    _junctionNodeTable->set(endingNodeId, node);

    node->_junctionId = 0;
    if (tgtNode->isITerm()) {
      _itermIndexTable->set(nodeId, tgtNode->getNode());
      node->_termMap = tgtNode->getNode();
      if (_net->getWire())
        node->_junctionId = _net->getWire()->getTermJid(node->_termMap);
    } else if (tgtNode->isBTerm()) {
      _btermIndexTable->set(nodeId, tgtNode->getNode());
      node->_termMap = -tgtNode->getNode();
      if (_net->getWire())
        node->_junctionId = _net->getWire()->getTermJid(node->_termMap);
    } else {
      node->_junctionId = tgtNode->getNode();
    }
  }
  // else TODO- when cutting a RCseg to satisfy max_cap requirements

  node->_netId = _net->getId();
  node->_capndId = tgtNode->getId();
  node->_splitCnt = 1;
  node->_x = x;
  node->_y = y;
  uint ii;
  for (ii = 0; ii < _cornerCnt; ii++) {
    node->_gndcap[ii] = 1e-3 * gndcap[ii];
    node->_cap[ii] = 1e-3 * totalcap[ii];
    node->_res[ii] = res[ii];
  }

  if (dbgFP != NULL) {
    printTest2(dbgFP, tgtNode, node, nodeId);
    for (ii = 0; ii < _cornerCnt; ii++)
      fprintf(dbgFP,
              "\t\t\ttotalC_%d= %g  gndc_%d= %g ccc_%d= %g R_%d=% g\n",
              ii,
              node->_cap[ii],
              ii,
              node->_gndcap[ii],
              ii,
              node->_cap[ii] - node->_gndcap[ii],
              ii,
              node->_res[ii]);
    fprintf(dbgFP, "\t\t\t(%d %d)\n", node->_x, node->_y);
  }

  return prevNode;
}
uint extRcTree::checkAndInit(odb::dbNet* net,
                             bool resetFlag,
                             odb::dbSet<odb::dbRSeg>& rSet)
{
  odb::dbRSeg* rc = NULL;
  odb::dbSet<odb::dbRSeg>::iterator rc_itr;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    rc = *rc_itr;
    break;
  }
  if (rc == NULL) {  // not extracted yet or "empty" net

    logger_->warn(
        RCX,
        164,
        "Net id {} has no RC segments.\tEither an empty net or has not "
        "been extracted yet!",
        net->getId());
    return 0;
  }
  odb::dbRSeg* zrc = net->getZeroRSeg();
  uint firstNodeId;
  if (!init(zrc, rc, rSet, resetFlag, &firstNodeId))
    return 0;
  return 1;
}

// extRCnode* extRcTree::makeTree2(odb::dbNet *net, double max_cap, uint test,
// bool resetFlag, bool addDummyJunctions, double mcf)
//{
//	odb::dbExtControl *extc = _block->getExtControl();
//	_foreign = extc->_foreign;
//	//if (_foreign == true && !net->anchoredRSeg())
//	//{
//	//	notice (0, "Extraction data is from read_spef. Can't make
// exttree.\n");
//	//	return NULL;
//	//}
//	if (!(max_cap>0.0))
//		max_cap= 10.0;
//
//	odb::dbSet<odb::dbRSeg> rSet= net->getRSegs();
//
//	uint rc1= checkAndInit(net, resetFlag, rSet);
//	if (rc1==0)
//		return NULL;
//
//	odb::dbRSeg* firstRC= NULL;
////	uint nodeId=0;
//	extRCnode *prevNode= NULL;
//
//	double cap= 0.0;
//	double res= 0.0;
////	uint cnt= 1;
//	odb::dbSet<odb::dbRSeg>::iterator rc_itr;
//	for( rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr )
//	{
//		odb::dbRSeg* rc = *rc_itr;
//		if ( firstRC==NULL) { // start of RCpath
//			firstRC= rc;
//			cap= 0.0; // rc->getCapacitance();
//			res= 0.0; // rc->getResistance();
//		}
//		uint startingNodeId= netLocalCn(firstRC->getSourceNode());
//		uint endingNodeId= netLocalCn(rc->getTargetNode());
//
////		uint tgtId= rc->getTargetNode();
//		odb::dbCapNode *tgtNode= odb::dbCapNode::getCapNode(_block,
// rc->getTargetNode());
//
//		double miller_mult= 1.0;
//		double wireCap= rc->getCapacitance(0, miller_mult);
//		double wireRes= rc->getResistance();
//
//		int ll[2];
//		int ur[2];
//		uint wireLength;
//		bool horizontal= getCoords(net, rc->getShapeId(), ll, ur,
// wireLength);
//
//		double deltaCap= max_cap - (cap+wireCap);
//
//		if (!tgtNode->isTreeNode() && (deltaCap>0.0)) { // not junction
// and haven't reached max_cap yet 			cap += wireCap;
// res += wireRes; 			continue;
//		}
//		if (tgtNode->isTreeNode() && (deltaCap>=0.0)) { // junction
//			prevNode= makeNode(startingNodeId, endingNodeId,
// tgtNode, false, ur[0], ur[1], res+wireRes, cap+wireCap, prevNode);
// firstRC= NULL; 			continue;
//		}
//		if (deltaCap==0.0) { // very rare
//			prevNode= makeNode(startingNodeId, endingNodeId,
// tgtNode, true, ur[0], ur[1], res+wireRes, cap+wireCap, prevNode);
// firstRC= NULL;
//		}
//		if (deltaCap<0.0) {
//			double capFraction= (max_cap-cap)/wireCap;
//			int x2= ll[0];
//			int y2= ll[1];
//			if (horizontal)
//				x2 += Ath__double2int(capFraction*wireLength);
//			else
//				y2 += Ath__double2int(capFraction*wireLength);
//
//			double res1= capFraction*wireRes;
//			double cap1= capFraction*wireCap;
//
//			prevNode= makeNode(startingNodeId, endingNodeId,
// tgtNode, false, x2, y2, res+res1, max_cap, prevNode);
//
//			double remainderCap= wireCap - cap1; //delta is negative
//			deltaCap= max_cap - remainderCap;
//			while (deltaCap<0.0) { // ??? == 0.0 and junction ???
//				remainderCap -= max_cap; //delta is negative
//				capFraction= max_cap/wireCap;
//
//				if (horizontal)
//					x2 +=
// Ath__double2int(capFraction*wireLength); 				else
// y2 += Ath__double2int(capFraction*wireLength);
//
//				prevNode= makeNode(startingNodeId, endingNodeId,
// tgtNode, false, x2, y2, capFraction*wireRes, max_cap, prevNode);
//
//				deltaCap= max_cap - remainderCap;
//			}
//			cap= remainderCap;
//			res= wireRes * (remainderCap/wireCap);
//			if (tgtNode->isTreeNode() && (deltaCap>0.0)) {
//				prevNode= makeNode(startingNodeId, endingNodeId,
// tgtNode, false, ur[0], ur[1], res , cap, prevNode);
// firstRC= NULL; 				continue;
//			}
//		}
//	}
//
//	_itermCnt= _itermIndexTable->getCnt() - _firstITermIndex;
//	_btermCnt= _btermIndexTable->getCnt() - _firstBTermIndex;
//
//	int X=0;
//	int Y=0;
//	if (_btermId>0) {
//		odb::dbBTerm *bterm= odb::dbBTerm::getBTerm(_block, _btermId);
//		if (!bterm->getFirstPinLocation(X, Y)) { // TWG: added bpins
//			logger_->warn(RCX, 0, "Can not locate bterm {}",
// bterm->getConstName()); 			return NULL;
//		}
//	}
//	else if (_itermId>0) {
//		odb::dbITerm *iterm= odb::dbITerm::getITerm(_block, _itermId);
//		if (!iterm->getAvgXY(&X, &Y)) {
//			logger_->warn(RCX, 0, "Can not locate iterm {}/{}",
// iterm->getInst()->getConstName(), iterm->getMTerm()->getConstName());
// return NULL;
//		}
//	}
//
//	extRCnode *driver_node= _nodeTable->get(_driverNodeId);
//
//	if((X == 0) && (Y == 0))
//		logger_->info(RCX, 0, "The drnode is at 0,0");
//	driver_node->_x= X;
//	driver_node->_y= Y;
//
//	uint netId= net->getId();
//	if (_debug)
//		printTree(stdout, netId, "");
//
//	if ( !isTree(net) )
//		return NULL;
//
//	if (addDummyJunctions) {
//		insertZeroJunctions();
//		if (_debug) {
//			printTree(stdout, netId, "");
//			if ( !isTree(net) )
//				return NULL;
//		}
//	}
//	if (test>0) {
//		printTree(stdout, netId, "");
//		_debug= true;
//	}
//
//	_tnodeCnt= makeGraph(net->getId());
//
//	/*if (_debug)
//		printTree(stdout, net->getId(), _tnodeCnt);
//*/
//	_debug= false;
//
//	if (_indexTable->getCnt()<=0)
//		return NULL;
//	else
//		return driver_node;
//}
void extRcTree::printTest2(FILE* dbgFP,
                           odb::dbCapNode* tgtNode,
                           extRCnode* node,
                           uint nodeId)
{
  if (tgtNode->isITerm()) {
    fprintf(dbgFP, "\t\t\t---> (%d) I_TERM= %d ", nodeId, node->_termMap);
  } else if (tgtNode->isBTerm()) {
    fprintf(dbgFP, "\t\t\t---> (%d) B_TERM= %d", nodeId, node->_termMap);
  } else {
    if (isDangling(tgtNode))
      fprintf(
          dbgFP, "\t\t\t---> (%d)  DANGLING= %d", nodeId, node->_junctionId);
    else
      fprintf(
          dbgFP, "\t\t\t---> (%d)  JUNCTION= %d", nodeId, node->_junctionId);
  }
  fprintf(dbgFP, "\n");
}

bool extRcTree::isDangling(odb::dbCapNode* node)
{
  return node->getChildrenCnt() <= 1 && !node->isTreeNode();
}
odb::dbRSeg* extRcTree::getFirstRC(odb::dbSet<odb::dbRSeg>& rSet)
{
  if (_net->getTermCount() < 2) {
    logger_->warn(RCX,
                  165,
                  "Net id {} has {} terms. can't make rcTree.",
                  _net->getId(),
                  _net->getTermCount());
    return NULL;
  }
  odb::dbRSeg* rc = NULL;
  odb::dbSet<odb::dbRSeg>::iterator rc_itr;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    rc = *rc_itr;
    break;
  }

  if (rc == NULL) {  // not extracted yet or "empty" net
    logger_->warn(
        RCX,
        164,
        "Net id {} has no RC segments.\tEither an empty net or has not "
        "been extracted yet!",
        _net->getId());
    return NULL;
  }
  return rc;
}
FILE* extRcTree::openFile(odb::dbNet* net,
                          const char* postfix,
                          const char* permissions)
{
  FILE* dbgFP = NULL;
  char name[64];
  sprintf(name, "%d.%s", net->getId(), postfix);
  dbgFP = fopen(name, permissions);
  if (dbgFP == NULL) {
    logger_->info(RCX, 27, "Cannot open file {}", name);
    return NULL;
  }
  return dbgFP;
}

extRCnode* extRcTree::makeTree(odb::dbNet* net,
                               double max_cap,
                               uint test,
                               bool resetFlag,
                               bool addDummyJunctions,
                               double mcf,
                               odb::dbBlockSearch* blk,
                               bool for_buffering,
                               uint extCorner,
                               bool is_rise,
                               bool is_min)
{
  odb::dbExtControl* extc = _block->getExtControl();
  _foreign = extc->_foreign;
  if (_foreign) {
    if (for_buffering && !extc->_rsegCoord) {
      logger_->warn(
          RCX,
          168,
          "Extraction data is from read_spef without coordinates. Can't "
          "make exttree.");
      return NULL;
    }
  }
  _extCorner = extCorner;
  _blockCornerIndex = _extCorner;
  _net = net;
  _cornerNet = net;
  _cornerBlock = _block->getExtCornerBlock(extCorner);
  _cornerBlock->preExttreeMergeRC(max_cap, 0 /*corner*/);
  if (_cornerBlock != _block) {
    _blockCornerIndex = 0;
    _cornerNet = odb::dbNet::getNet(_cornerBlock, net->getId());
  }

  odb::dbSet<odb::dbRSeg> rSet = _cornerNet->getRSegs();
  if (rSet.begin() == rSet.end()) {
    logger_->warn(RCX,
                  166,
                  "Net {}, {} has no extraction data",
                  net->getId(),
                  net->getConstName());
    return NULL;
  }

  FILE* flowFP = NULL;
  FILE* nodeFP = NULL;
  if (test > 1) {
    flowFP = openFile(net, "flow.dbg", "w");
    if (flowFP == NULL)
      return NULL;
    nodeFP = openFile(net, "node.dbg", "w");
    if (nodeFP == NULL)
      return NULL;
  }
  if (!(max_cap > 0.0))
    max_cap = 10.0;

  odb::dbRSeg* rc = getFirstRC(rSet);
  if (rc == NULL)
    return NULL;
  odb::dbRSeg* zrc = _cornerNet->getZeroRSeg();

  //	odb::dbCapNode *chkNode= odb::dbCapNode::getCapNode(_block,
  // rc->getTargetNode()); if (chkNode->isForeign())
  //        return NULL;

  //	uint size= rSet.size();
  uint firstNodeId;
  if (!init(zrc, rc, rSet, resetFlag, &firstNodeId))
    return NULL;

  extRCnode* prevNode = NULL;

  odb::dbRSeg* firstRC = NULL;
  //	uint nodeId=0;

  double gndcap[ADS_MAX_CORNER];
  double totalcap[ADS_MAX_CORNER];
  double res[ADS_MAX_CORNER];
  uint cnt = 1;
  bool firstFlag = false;

#ifdef EXT_SI
  tmg_db* tm = tmg_db::getTmgDb();
  if (mcf < 0)
    initExtSi(_cornerNet, is_min, is_rise, tm);
#endif

  zrc->getGndTotalCap(&gndcap[0], &totalcap[0], mcf);
  odb::dbSet<odb::dbRSeg>::iterator rc_itr;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    odb::dbRSeg* rc = *rc_itr;
    if (firstRC == NULL) {  // start of RCpath
      firstRC = rc;
      firstFlag = true;
    }
    uint startingNodeId = netLocalCn(firstRC->getSourceNode());
    uint endingNodeId = netLocalCn(rc->getTargetNode());

    uint srcId = rc->getSourceNode();
    uint tgtId = rc->getTargetNode();
    //		odb::dbCapNode *srcNode= odb::dbCapNode::getCapNode(_block,
    // rc->getSourceNode());
    odb::dbCapNode* tgtNode
        = odb::dbCapNode::getCapNode(_cornerBlock, rc->getTargetNode());

    //		bool srcNodeFlag= srcNode->isTreeNode();
    bool tgtNodeFlag = tgtNode->isTreeNode();

    if (cnt == 1) {
      rc->addGndTotalCap(&gndcap[0], &totalcap[0], mcf);
      rc->getAllRes(&res[0]);
    } else if (firstFlag) {
      rc->getGndTotalCap(&gndcap[0], &totalcap[0], mcf);
      rc->getAllRes(&res[0]);
    } else {
      rc->addGndTotalCap(&gndcap[0], &totalcap[0], mcf);
      rc->addAllRes(&res[0]);
    }

    cnt++;
    uint shapeId;
    if (!_foreign)
      shapeId = rc->getShapeId();

    if (flowFP != NULL)
      fprintf(flowFP, "shape %d (rc=%d)\n", shapeId, rc->getId());

    if (srcId == tgtId
        || !(tgtNodeFlag || totalcap[_blockCornerIndex] > max_cap
             || isDangling(tgtNode) || (!(_foreign) && shapeId == 0))) {
      if (_cornerBlock->getExtControl()->_exttreePreMerg)
        logger_->warn(RCX, 167, "Shouldn't merge rc again after pre-merge");
      firstFlag = false;
      continue;
    }
    if (isDangling(tgtNode)) {
      if (flowFP != NULL)
        fprintf(flowFP, "\t\t\t---> DANGLING ignored\n");
      firstRC = NULL;
      continue;
    }
    // int x1=0, y1=0, x2=0, y2=0;
    // if (_foreign)
    //	rc->getCoords(x2, y2);
    // else if (shapeId>0)
    //	//getCoords(net, shapeId, &x1, &y1, &x2, &y2);
    //	net->getWire()->getCoord((int)shapeId, x2, y2);
    // else if (tgtNodeFlag)
    //	return NULL;
    int x2, y2;
    rc->getCoords(x2, y2);

    bool fractionFlag = false;
    prevNode = makeNode(startingNodeId,
                        endingNodeId,
                        tgtNode,
                        fractionFlag,
                        x2,
                        y2,
                        &res[_blockCornerIndex],
                        &gndcap[_blockCornerIndex],
                        &totalcap[_blockCornerIndex],
                        prevNode,
                        flowFP);
    /*
    if (shapeId>0)
            frpintf(stdout, "Warning ....."
    */

    firstRC = NULL;
    /*
//		uint nodeId=0;
//		//uint childCnt= tgtNode->getChildrenCnt()+1;
//		uint childCnt= tgtNode->getChildrenCnt();
//		extRCnode *node= allocNode(childCnt, &nodeId);
//
//		 if (tgtNode->isITerm()) {
//		 _itermIndexTable->set(nodeId, tgtNode->getNode());
//		 node->_termMap= tgtNode->getNode();
//
//		  if (_debug && blk==NULL)
//		  logger_->info(RCX, 0, "\t-->node: {}  Iterm= {}", nodeId,
node->_termMap);
//		  if (test>1)
//				fprintf(dbgFP, "\t\t---> (%d) I_TERM= %d ",
nodeId, node->_termMap);
//		}
//		else if (tgtNode->isBTerm()) {
//			_btermIndexTable->set(nodeId, tgtNode->getNode());
//			node->_termMap= -tgtNode->getNode();
//			if (_debug && blk==NULL)
//				logger_->info(RCX, 0, "\t-->node: {}  B_TERM=
{}", nodeId, node->_termMap);
//			if (test>1)
//				fprintf(dbgFP, "\t\t---> (%d) B_TERM= %d",
nodeId, node->_termMap);
//		}
//		else {
//			node->_junctionId = tgtNode->getNode();
//			if (_debug && blk==NULL)
//				logger_->info(RCX, 0, "\t-->node: {} junction=
{}", nodeId, node->_junctionId);
//			if (test>1) {
//				if (isDangling(tgtNode))
//					fprintf(dbgFP, "\t\t---> (%d) DANGLING=
%d", nodeId, node->_junctionId);
//				else
//					fprintf(dbgFP, "\t\t---> (%d) JUNCTION=
%d", nodeId, node->_junctionId);
//			}
//		}
//		if (_debug && blk==NULL) {
//			logger_->info(RCX, 0, "\ttshape {} --> {} :  has {}
children", shapeId, nodeId, childCnt);
//		}
//		node->_netId= net->getId();
//        if((x2 == 0) & (y2 == 0))
//			if (blk==NULL)
//				logger_->info(RCX, 0, "The gnode is at 0, 0");
//			node->_x= x2;
//			node->_y= y2;
//			node->_cap= 1e-3*cap;
//			node->_res= res;
//
//			if (_debug && blk==NULL) {
//				logger_->info(RCX, 0, "\tR= {}  C= {}  -- x2= {}
y2= {} -- x1={} x2={}",
//					node->_res, node->_cap, x2, y2, x1, y1);
//			}
//			if (test>1) {
//				fprintf(dbgFP, " fanout %d R= %g  C= %g\n",
childCnt, node->_res, node->_cap);
//				fprintf(dbgFP, "\t\t\tx2= %d  y2= %d -- x1=%d
x2=%d\n", x2, y2, x1, y1);
//			}
//			if (prevNode!=NULL) {
//				// node->_cap= cap; //TODO: add % if cut in the
middle of wire
//				// node->_res= res;
//
//				addChild(prevNode, nodeId);
//				if (tgtNodeFlag)
//					prevNode= NULL;
//				else
//					prevNode= node;
//			}
//			else { // tgtNodeFlag || cap>cap_max
//				extRCnode *junctionNode=
_junctionNodeTable->geti(netLocalCn(firstRC->getSourceNode()));
//				if (junctionNode!=NULL) {
//					addChild(junctionNode, nodeId);
//				}
//				prevNode= node;
//				if (tgtNodeFlag)
//					prevNode= NULL;
//			}
//			if (tgtNodeFlag) {
//				uint index= netLocalCn(lastRC->getTargetNode());
//				_junctionNodeTable->set(index, node);
//				firstRC= NULL;
//			}
//			cap= 0.0;
//			res= 0.0;
//			cnt= 0;
//			*/
  }
  _itermCnt = _itermIndexTable->getCnt() - _firstITermIndex;
  _btermCnt = _btermIndexTable->getCnt() - _firstBTermIndex;

  uint netId = net->getId();

  int X = 0;
  int Y = 0;
  zrc->getCoords(X, Y);
  //	if (_btermId>0) {
  //		odb::dbBTerm *bterm= odb::dbBTerm::getBTerm(_block, _btermId);
  //		if (!bterm->getFirstPinLocation(X, Y)) { // TWG: added bpins
  //			logger_->warn(RCX, 0, "Can not locate bterm {}",
  // bterm->getConstName()); 			return NULL;
  //		}
  //	}
  //	else if (_itermId>0) {
  //		odb::dbITerm *iterm= odb::dbITerm::getITerm(_block, _itermId);
  //		if (!iterm->getAvgXY(&X, &Y)) {
  //			logger_->warn(RCX, 0, "Can not locate iterm {}/{}",
  // iterm->getInst()->getConstName(), iterm->getMTerm()->getConstName());
  // return NULL;
  //		}
  //	}
  extRCnode* driver_node = _nodeTable->get(_driverNodeId);
  if (!driver_node) {
    logger_->warn(RCX, 169, "The driver_node of the tree is NULL");
    return NULL;
  }
  if ((X == 0) && (Y == 0))
    if (blk == NULL)
      logger_->info(RCX, 170, "The driver_node is at 0,0");
  driver_node->_x = X;
  driver_node->_y = Y;

  if (test > 1)
    printTree(nodeFP, netId, "Node graph after RC traversal");

  if (!isTree(net))
    return NULL;

  if (addDummyJunctions) {
    insertZeroJunctions();
    if (nodeFP != NULL)
      printTree(nodeFP, netId, "Node graph after dummy Junctions");

    if (!isTree(net))
      return NULL;
  }
  _tnodeCnt = makeGraph(net->getId());

  if (test > 1) {
    /*
    if (graphFP!=NULL) {
            printTree(graphFP, net->getId(), _tnodeCnt);
            fclose(graphFP);
    }
    */
    if (flowFP != NULL)
      fclose(flowFP);
    if (nodeFP != NULL)
      fclose(nodeFP);
  }

  _debug = false;
  if (_block->getPtFile())
    printTree(_block->getPtFile(), net->getId(), _tnodeCnt);

  if (_indexTable->getCnt() <= 0)
    return NULL;
  else
    return driver_node;
}
extTnode* extRcTree::makeTree(odb::dbNet* net,
                              double max_cap,
                              uint test,
                              bool resetFlag,
                              bool addDummyJunctions,
                              uint& tnodeCnt,
                              double mcf,
                              bool for_buffering,
                              uint extCorner,
                              bool is_rise,
                              bool is_min)
{
  // if (net->getWire() == NULL)
  // 	return NULL;
  if (net->isRCDisconnected())
    return NULL;
  if (!_block->isBufferAltered()) {
    // Not checking isWireAltered here
    // so that we can allow fix_slew to size and then buffer same net
    // For normal buffering, caller should check net->isWireAltered()
    // before calling makeTree.
    if (net->isDisconnected())
      return NULL;
  }
  if (makeTree(net,
               max_cap,
               test,
               resetFlag,
               addDummyJunctions,
               mcf,
               NULL,
               for_buffering,
               extCorner,
               is_rise,
               is_min)
      == NULL)
    return NULL;

  tnodeCnt = _tnodeCnt;
  return _tnodeTable[0];
}

// DIMITRIS 8/28/07 void free_exttree(extTnode *driver);

void extRcTree::makeTree(double max_cap, uint test, bool for_buffering)
{
  odb::dbSet<odb::dbNet> nets = _block->getNets();
  odb::dbSet<odb::dbNet>::iterator net_itr;

  //	uint cnt= 0;
  extTnode* tnode = NULL;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    odb::dbNet* net = *net_itr;

    odb::dbSigType type = net->getSigType();
    if ((type == odb::dbSigType::POWER) || (type == odb::dbSigType::GROUND))
      continue;

    // extRCnode* node= makeTree(net->getId(), max_cap, 0, true);
    uint cnt;
    tnode = makeTree(net->getId(),
                     max_cap,
                     test,
                     true,
                     true,
                     cnt,
                     1 /*mcf*/,
                     NULL /*printTag*/,
                     for_buffering);
    if (tnode)
      extRcTree::free_exttree(tnode);
    tnode = NULL;
    cnt++;
  }
}

}  // namespace rcx
