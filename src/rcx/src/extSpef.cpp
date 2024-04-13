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

#include "rcx/extSpef.h"

#include <algorithm>

#include "name.h"
#include "odb/dbExtControl.h"
#include "odb/parse.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

class extMain;

extSpef::extSpef(odb::dbTech* tech,
                 odb::dbBlock* blk,
                 Logger* logger,
                 const char* version,
                 extMain* extmain)
{
  logger_ = logger;
  _ext = extmain;
  _tech = tech;
  _block = blk;
  _version = version;

  if (blk != nullptr) {
    _blockId = blk->getId();
  }

  strcpy(_divider, "/");
  strcpy(_delimiter, ":");
  strcpy(_bus_delimiter, "[]");

  strcpy(_res_unit_word, "OHM");
  strcpy(_cap_unit_word, "FF");
  strcpy(_time_unit_word, "NS");
  strcpy(_ind_unit_word, "HENRY");
  _design[0] = '\0';

  // for ABNORMAL and sparse map numbers should be a hash table
  _idMapTable = new Ath__array1D<uint>(128000);

  _srsegi = new Ath__array1D<uint>(1024);
  _nrseg = new Ath__array1D<odb::dbRSeg*>(1024);
  _hcnrc = new Ath__array1D<Ath__array1D<int>*>(1024);
  for (uint ii = 0; ii < 1024; ii++) {
    Ath__array1D<int>* n1d = new Ath__array1D<int>(4);
    _hcnrc->set(ii, n1d);
  }

  _msgBuf1 = (char*) malloc(sizeof(char) * 2048);
  _msgBuf2 = (char*) malloc(sizeof(char) * 2048);
}

extSpef::~extSpef()
{
  delete _idMapTable;
  delete _nodeParser;
  delete _parser;
  delete _nodeTable;
  delete _btermTable;
  delete _itermTable;
  if (_nodeCapTable) {
    deleteTableCap(_nodeCapTable);
    delete _nodeCapTable;
  }
  if (_btermCapTable) {
    deleteTableCap(_btermCapTable);
    delete _btermCapTable;
  }
  if (_itermCapTable) {
    deleteTableCap(_itermCapTable);
    delete _itermCapTable;
  }
  delete _nameMapTable;
  delete _nodeHashTable;
  delete _ccidmap;
  delete _nodeCoordParser;
  free(_msgBuf1);
  free(_msgBuf2);
}

void extSpef::setBlock(odb::dbBlock* blk)
{
  _block = blk;
}

void extSpef::set_single_pi(const bool v)
{
  _singleP = v;
}

void extSpef::setLogger(Logger* logger)
{
  logger_ = logger;
}

char* extSpef::getDelimeter()
{
  return _delimiter;
}

void extSpef::setupMappingForWrite(uint btermCnt, uint itermCnt)
{
  if (_btermTable) {
    resetTermTables();
    if (!_nodeCapTable) {
      _nodeCapTable = new Ath__array1D<double*>(16000);
      initCapTable(_nodeCapTable);
    }
    return;
  }
  if ((itermCnt == 0) && (_block != nullptr)) {
    btermCnt = _block->getBTerms().size();
    btermCnt = getMultiples(btermCnt, 1024);

    itermCnt = _block->getITerms().size();
    itermCnt = getMultiples(itermCnt, 1024);
  } else if (itermCnt == 0) {
    btermCnt = 512;
    itermCnt = 64000;
  }

  _btermTable = new Ath__array1D<uint>(btermCnt);
  _itermTable = new Ath__array1D<uint>(itermCnt);

  _btermTable->add(0);
  _itermTable->add(0);

  _btermCapTable = new Ath__array1D<double*>(btermCnt);
  _itermCapTable = new Ath__array1D<double*>(itermCnt);
  _nodeCapTable = new Ath__array1D<double*>(16000);

  initCapTable(_btermCapTable);
  initCapTable(_itermCapTable);
  initCapTable(_nodeCapTable);
}

uint extSpef::getMultiples(const uint cnt, const uint base)
{
  return ((cnt / base) + 1) * base;
}

void extSpef::setCalibLimit(const float upperLimit, const float lowerLimit)
{
  _upperCalibLimit = upperLimit;
  _lowerCalibLimit = lowerLimit;
  _match = (_upperCalibLimit == 0.0 && _lowerCalibLimit == 0.0);
}

void extSpef::setUseIdsFlag(const bool diff, const bool calib)
{
  _diff = diff;
  _calib = calib;
  if (diff && !calib) {
    _diffLogFP = fopen("diff_spef.log", "w");
    if (_diffLogFP == nullptr) {
      logger_->error(
          RCX, 171, "Can't open log file diff_spef.log for writing!");
    }
    _diffOutFP = fopen("diff_spef.out", "w");
    if (_diffOutFP == nullptr) {
      logger_->error(
          RCX, 172, "Can't open output file diff_spef.out for writing!");
    }
  }
}

void extSpef::preserveFlag(const bool v)
{
  _preserveCapValues = v;
}

void extSpef::setCornerCnt(const uint n)
{
  _cornerCnt = n;
}

void extSpef::setGzipFlag(const bool gzFlag)
{
  _gzipFlag = gzFlag;
}

void extSpef::resetTermTables()
{
  _btermTable->resetCnt(1);
  _itermTable->resetCnt(1);
}

void extSpef::resetCapTables(const uint maxNode)
{
  reinitCapTable(_nodeCapTable, maxNode);
  reinitCapTable(_itermCapTable, _itermTable->getCnt() + 1);
  reinitCapTable(_btermCapTable, _btermTable->getCnt() + 1);
}

void extSpef::initCapTable(Ath__array1D<double*>* table)
{
  for (uint ii = 0; ii < table->getSize(); ii++) {
    double* a = new double[_cornerCnt];
    table->add(a);
  }
}

void extSpef::deleteTableCap(Ath__array1D<double*>* table)
{
  for (uint ii = 0; ii < table->getSize(); ii++) {
    double* a = table->geti(ii);
    delete[] a;
  }
}

void extSpef::reinitCapTable(Ath__array1D<double*>* table, const uint n)
{
  if (n > table->getSize()) {
    const uint prevCnt = table->getSize();
    table->resize(n);
    table->resetCnt(prevCnt);
    double* a = new double[_cornerCnt];
    table->add(a);
    const uint currentCnt = table->getSize();
    for (uint ii = prevCnt + 1; ii < currentCnt; ii++) {
      double* a = new double[_cornerCnt];
      table->add(a);
    }
  }
  for (uint kk = 1; kk < n; kk++) {
    double* a = table->get(kk);

    for (uint jj = 0; jj < _cornerCnt; jj++) {
      a[jj] = 0.0;
    }
  }
}

void extSpef::setDesign(const char* name)
{
  strcpy(_design, name);
}

uint extSpef::getInstMapId(const uint id)
{
  if (_childBlockInstBaseMap > 0) {
    return _childBlockInstBaseMap + id;
  }

  return _baseNameMap + id;
}

void extSpef::writeNameNode(odb::dbCapNode* node)
{
  odb::dbStringProperty* p = odb::dbStringProperty::find(node, "_inode");

  if (_bufString) {
    sprintf(_msgBuf1, "%s ", p->getValue().c_str());
    strcat(_bufString, _msgBuf1);
  } else {
    fprintf(_outFP, "%s ", p->getValue().c_str());
  }
}

void extSpef::writeITermNode(const uint node)
{
  odb::dbITerm* iterm = odb::dbITerm::getITerm(_block, node);
  odb::dbInst* inst = iterm->getInst();
  if (inst->getMaster()->isMarked()) {
    return;
  }

  char ttname[256];
  if (_bufString) {
    if (_writeNameMap) {
      sprintf(_msgBuf1,
              "*%d%s%s ",
              getInstMapId(inst->getId()),
              _delimiter,
              addEscChar(iterm->getMTerm()->getName(inst, &ttname[0]), false));
    } else {
      sprintf(_msgBuf1,
              "%s%s%s ",
              addEscChar(tinkerSpefName((char*) inst->getConstName()), true),
              _delimiter,
              addEscChar(iterm->getMTerm()->getName(inst, &ttname[0]), false));
    }
    strcat(_bufString, _msgBuf1);
  } else {
    if (_writeNameMap) {
      fprintf(_outFP,
              "*%d%s%s ",
              getInstMapId(inst->getId()),
              _delimiter,
              addEscChar(iterm->getMTerm()->getName(inst, &ttname[0]), false));
    } else {
      fprintf(_outFP,
              "%s%s%s ",
              addEscChar(tinkerSpefName((char*) inst->getConstName()), true),
              _delimiter,
              addEscChar(iterm->getMTerm()->getName(inst, &ttname[0]), false));
    }
  }
}

void extSpef::writeITerm(const uint node)
{
  odb::dbITerm* iterm = odb::dbITerm::getITerm(_block, node);
  if (iterm->getInst()->getMaster()->isMarked()) {
    return;
  }

  fprintf(_outFP, "*I ");
  writeITermNode(node);

  const char io = iterm->getIoType().getString()[0];
  fprintf(_outFP, "%c ", io);
  const int dbunit = _block->getDbUnitsPerMicron();
  const double db2nm = 1.0 / dbunit;
  if (_writingNodeCoords == C_ON) {
    int jid = 0;
    odb::dbWire* wire = _d_net->getWire();
    if (wire) {
      jid = wire->getTermJid(iterm->getId());
    }
    odb::Point pt;
    if (_termJxy && jid) {
      pt = wire->getCoord(jid);
    } else {
      int x1 = 0;
      int y1 = 0;
      iterm->getAvgXY(&x1, &y1);
      pt = {x1, y1};
    }
    fprintf(_outFP, "*C %f %f ", db2nm * pt.x(), db2nm * pt.y());
  }
  fprintf(_outFP,
          "*D %s\n",
          addEscChar(iterm->getMTerm()->getMaster()->getName().c_str(), false));
}

void extSpef::writeBTerm(const uint node)
{
  odb::dbBTerm* bterm = odb::dbBTerm::getBTerm(_block, node);
  if (_bufString) {
    sprintf(_msgBuf1, "%s ", addEscChar(bterm->getName().c_str(), false));
    strcat(_bufString, _msgBuf1);
  } else {
    fprintf(_outFP, "%s ", addEscChar(bterm->getName().c_str(), false));
  }
}

void extSpef::writeNode(const uint netId, const uint node)
{
  odb::dbNet* tnet = _d_net;
  if (_bufString) {
    if (_writeNameMap) {
      sprintf(_msgBuf1, "*%d%s%d ", netId, _delimiter, node);
    } else {
      sprintf(_msgBuf1,
              "%s%s%d ",
              addEscChar(tinkerSpefName((char*) tnet->getConstName()), false),
              _delimiter,
              node);
    }
    strcat(_bufString, _msgBuf1);
  } else {
    if (_writeNameMap) {
      fprintf(_outFP, "*%d%s%d ", netId, _delimiter, node);
    } else {
      fprintf(_outFP,
              "%s%s%d ",
              addEscChar(tinkerSpefName((char*) tnet->getConstName()), false),
              _delimiter,
              node);
    }
  }
}

void extSpef::writeCapNode(const uint capNodeId, const uint netId)
{
  odb::dbCapNode* capNode = odb::dbCapNode::getCapNode(_block, capNodeId);
  writeCapNode(capNode, netId);
}

void extSpef::writeCapNode(odb::dbCapNode* capNode, uint netId)
{
  if (netId == 0) {
    netId = capNode->getNet()->getId();
  }

  if (capNode->isITerm()) {
    writeITermNode(capNode->getNode());
  } else if (capNode->isBTerm()) {
    writeBTerm(capNode->getNode());
  } else if (capNode->isInternal()) {
    if (_childBlockNetBaseMap > 0) {
      netId += _childBlockNetBaseMap;
    }

    writeNode(netId, capNode->getNode());
  } else if (capNode->isName()) {
    writeNameNode(capNode);
  }
}

void extSpef::writeCapITerm(const uint node, const uint capIndex)
{
  if ((odb::dbITerm::getITerm(_block, node))
          ->getInst()
          ->getMaster()
          ->isMarked()) {
    return;
  }
  writeCNodeNumber();

  writeITermNode(node);
  writeRCvalue(_nodeCapTable->geti(capIndex), _cap_unit);

  fprintf(_outFP, "\n");
}

void extSpef::writeCapName(odb::dbCapNode* capNode, const uint capIndex)
{
  writeCNodeNumber();

  writeNameNode(capNode);
  writeRCvalue(_nodeCapTable->geti(capIndex), _cap_unit);

  fprintf(_outFP, "\n");
}

void extSpef::writeCapPort(const uint node, const uint capIndex)
{
  writeCNodeNumber();

  writeBTerm(node);

  writeRCvalue(_nodeCapTable->geti(capIndex), _cap_unit);
  fprintf(_outFP, "\n");
}

void extSpef::writePort(const uint node)
{
  odb::dbBTerm* bterm = odb::dbBTerm::getBTerm(_block, node);
  fprintf(_outFP,
          "*P %s %c",
          addEscChar(bterm->getName().c_str(), false),
          bterm->getIoType().getString()[0]);
  if (_writingNodeCoords != C_ON) {
    fprintf(_outFP, "\n");
    return;
  }
  const int dbunit = _block->getDbUnitsPerMicron();
  const double db2nm = 1.0 / dbunit;
  int jid = 0;
  odb::dbWire* wire = _d_net->getWire();
  if (wire) {
    jid = wire->getTermJid(-bterm->getId());
  }
  odb::Point pt;
  if (_termJxy && jid) {
    pt = wire->getCoord(jid);
  } else {
    int x1 = 0;
    int y1 = 0;
    bterm->getFirstPinLocation(x1, y1);
    pt = {x1, y1};
  }
  fprintf(_outFP, " *C %f %f\n", db2nm * pt.x(), db2nm * pt.y());
}

void extSpef::writeSingleRC(const double val, const bool delimeter)
{
  if (delimeter) {
    fprintf(_outFP, "%s%g", _delimiter, val * _cap_unit);
  } else {
    fprintf(_outFP, "%g", val * _cap_unit);
  }
}

void extSpef::writeRCvalue(const double* totCap, const double units)
{
  fprintf(_outFP, "%g", totCap[_active_corner_number[0]] * units);
  for (int ii = 1; ii < _active_corner_cnt; ii++) {
    fprintf(
        _outFP, "%s%g", _delimiter, totCap[_active_corner_number[ii]] * units);
  }
}

void extSpef::writeDnet(uint netId, const double* totCap)
{
  netId = getNetMapId(netId);

  if (_writeNameMap) {
    fprintf(_outFP, "\n*D_NET *%d ", netId);
  } else {
    fprintf(_outFP,
            "\n*D_NET %s ",
            addEscChar(tinkerSpefName((char*) _d_net->getConstName()), false));
  }
  writeRCvalue(totCap, _cap_unit);
  fprintf(_outFP, "\n");
}

void extSpef::writeKeyword(const char* keyword)
{
  fprintf(_outFP, "%s\n", keyword);
}

void extSpef::addCap(const double* cap, double* totCap, const uint n)
{
  for (uint ii = 0; ii < n; ii++) {
    totCap[ii] += cap[ii];
  }
}

void extSpef::incrementCounter(double* cap, const uint n)
{
  cap[n] += 1.0;
}

void extSpef::setCap(const double* cap,
                     const uint n,
                     double* totCap,
                     uint startIndex)
{
  for (uint jj = 0; jj < startIndex; jj++) {
    totCap[jj] = 0.0;
  }

  for (uint ii = 0; ii < n; ii++) {
    totCap[startIndex++] = cap[ii];
  }
}

void extSpef::resetCap(double* cap)
{
  resetCap(cap, _cornerCnt);
}

void extSpef::resetCap(double* cap, const uint cnt)
{
  for (uint ii = 0; ii < cnt; ii++) {
    cap[ii] = 0.0;
  }
}

void extSpef::copyCap(double* totCap, const double* cap, uint n)
{
  if (n == 0) {
    n = _cornerCnt;
  }

  for (uint ii = 0; ii < n; ii++) {
    totCap[ii] = cap[ii];
  }
}

void extSpef::adjustCap(double* totCap, const double* cap, uint n)
{
  if (n == 0) {
    n = _cornerCnt;
  }

  for (uint ii = 0; ii < n; ii++) {
    totCap[ii] += cap[ii];
  }
}

void extSpef::addHalfCap(double* totCap, const double* cap, uint n)
{
  if (n == 0) {
    n = _cornerCnt;
  }

  for (uint ii = 0; ii < n; ii++) {
    totCap[ii] += cap[ii] / 2;
  }
}

uint extSpef::getMappedCapNode(const uint nodeId)
{
  return nodeId - _firstCapNode;
}

void extSpef::computeCaps(odb::dbSet<odb::dbRSeg>& rcSet, double* totCap)
{
  odb::dbSet<odb::dbRSeg>::iterator rc_itr;

  double cap[ADS_MAX_CORNER];
  for (odb::dbRSeg* rc : rcSet) {
    rc->getCapTable(cap);
    addCap(cap, totCap, this->_cornerCnt);

    const uint trgNodeId = rc->getTargetNode();
    const uint trgMappedNode
        = odb::dbCapNode::getCapNode(_cornerBlock, trgNodeId)->getSortIndex();
    addHalfCap(_nodeCapTable->geti(trgMappedNode), cap);

    const uint srcNodeId = rc->getSourceNode();
    const uint srcMappedNode
        = odb::dbCapNode::getCapNode(_cornerBlock, srcNodeId)->getSortIndex();
    addHalfCap(_nodeCapTable->geti(srcMappedNode), cap);
  }
}

void extSpef::computeCapsAdd2Target(odb::dbSet<odb::dbRSeg>& rcSet,
                                    double* totCap)
{
  double cap[ADS_MAX_CORNER];
  for (odb::dbRSeg* rc : rcSet) {
    rc->getCapTable(cap);
    addCap(cap, totCap, this->_cornerCnt);

    const uint trgNodeId = rc->getTargetNode();
    const uint trgMappedNode
        = odb::dbCapNode::getCapNode(_block, trgNodeId)->getSortIndex();
    adjustCap(_nodeCapTable->geti(trgMappedNode), cap);
  }
}

void extSpef::getCaps(odb::dbNet* net, double* totCap)
{
  for (odb::dbCapNode* node : net->getCapNodes()) {
    double cap[ADS_MAX_CORNER];
    for (uint ii = 0; ii < _cornersPerBlock; ii++) {
      cap[ii] = node->getCapacitance(ii);
    }

    addCap(cap, totCap, _cornersPerBlock);
  }
}

void extSpef::addCouplingCaps(odb::dbNet* net, double* totCap)
{
  double cap[ADS_MAX_CORNER];
  for (uint ii = 0; ii < _cornersPerBlock; ii++) {
    cap[ii] = net->getTotalCouplingCap(ii);
  }

  addCap(cap, totCap, _cornersPerBlock);
}

void extSpef::addCouplingCaps(odb::dbSet<odb::dbCCSeg>& capSet, double* totCap)
{
  for (odb::dbCCSeg* cc : capSet) {
    double cap[ADS_MAX_CORNER];
    for (uint ii = 0; ii < _cornerCnt; ii++) {
      cap[ii] = cc->getCapacitance(ii);
    }

    addCap(cap, totCap, _cornerCnt);
  }
}

uint extSpef::getMinCapNode(odb::dbNet* net, uint* minNode)
{
  uint cnt = 0;
  uint min = std::numeric_limits<uint>::max();
  for (odb::dbCapNode* node : net->getCapNodes()) {
    cnt++;
    node->setSortIndex(cnt);

    min = std::min(min, node->getId());
  }
  if (cnt == 0) {
    *minNode = 0;
    return 0;
  }
  *minNode = min;
  return cnt + 1;
}

void extSpef::writeCNodeNumber()
{
  if (_noCnum) {
    return;
  }
  fprintf(_outFP, "%d ", _cCnt++);
}

void extSpef::writeNodeCap(const uint netId, const uint capIndex, const uint ii)
{
  writeCNodeNumber();
  writeNode(netId, ii);
  writeRCvalue(_nodeCapTable->geti(capIndex), _cap_unit);
  fprintf(_outFP, "\n");
}

void extSpef::writePorts(odb::dbNet* net)
{
  for (odb::dbCapNode* capNode : net->getCapNodes()) {
    if (!capNode->isBTerm()) {
      continue;
    }

    writePort(capNode->getNode());
  }
}

void extSpef::writeInternalCaps(odb::dbNet* net,
                                odb::dbSet<odb::dbCapNode>& capSet)
{
  for (odb::dbCapNode* capNode : capSet) {
    if (!capNode->isInternal()) {
      continue;
    }

    writeCNodeNumber();
    writeNode(net->getId(), capNode->getNode());

    writeSingleRC(capNode->getCapacitance(_active_corner_number[0]), false);
    for (int ii = 1; ii < _active_corner_cnt; ii++) {
      writeSingleRC(capNode->getCapacitance(_active_corner_number[ii]), true);
    }

    fprintf(_outFP, "\n");
  }
}

void extSpef::writeCapPortsAndIterms(odb::dbSet<odb::dbCapNode>& capSet,
                                     const bool bterms)
{
  for (odb::dbCapNode* capNode : capSet) {
    if (capNode->isInternal()) {
      continue;
    }

    if (capNode->isBTerm()) {
      if (!bterms) {
        continue;
      }
      writeCNodeNumber();
      writeBTerm(capNode->getNode());
    } else if (capNode->isITerm()) {
      if (bterms) {
        continue;
      }
      if (capNode->getITerm(_block)->getInst()->getMaster()->isMarked()) {
        continue;
      }
      writeCNodeNumber();
      writeITermNode(capNode->getNode());
    } else {
      continue;
    }

    writeSingleRC(capNode->getCapacitance(_active_corner_number[0]), false);
    for (int ii = 1; ii < _active_corner_cnt; ii++) {
      writeSingleRC(capNode->getCapacitance(_active_corner_number[ii]), true);
    }

    fprintf(_outFP, "\n");
  }
}

void extSpef::writeCapPorts(odb::dbNet* net)
{
  for (odb::dbCapNode* capNode : net->getCapNodes()) {
    if (!capNode->isBTerm()) {
      continue;
    }

    writeCapPort(capNode->getNode(), capNode->getSortIndex());
  }
}

void extSpef::writeITerms(odb::dbNet* net)
{
  for (odb::dbCapNode* capNode : net->getCapNodes()) {
    if (capNode->isITerm()) {
      writeITerm(capNode->getNode());
    } else if (capNode->isName()) {
      fprintf(_outFP, "*I ");
      writeNameNode(capNode);
      fprintf(_outFP, "\n");
    }
  }
}

void extSpef::writeCapITerms(odb::dbNet* net)
{
  for (odb::dbCapNode* capNode : net->getCapNodes()) {
    if (capNode->isITerm()) {
      writeCapITerm(capNode->getNode(), capNode->getSortIndex());
    } else if (capNode->isName()) {  // coming from lower level
      writeCapName(capNode, capNode->getSortIndex());
    }
  }
}

void extSpef::writeNodeCaps(odb::dbNet* net, uint netId)
{
  if (netId == 0) {
    netId = net->getId();
  }

  for (odb::dbCapNode* capNode : net->getCapNodes()) {
    if (!capNode->isInternal()) {
      continue;
    }

    const uint capNodeId = capNode->getSortIndex();
    writeNodeCap(netId, capNodeId, capNode->getNode());
  }
}

class compareCC
{
 public:
  bool operator()(odb::dbCCSeg* cc1, odb::dbCCSeg* cc2)
  {
    odb::dbBlock* block = cc1->getSourceNet()->getBlock();
    {
      odb::dbCapNode* cp1 = cc1->getSourceCapNode();
      odb::dbCapNode* cp2 = cc2->getSourceCapNode();

      const uint id1 = cp1->getNode();
      const uint id2 = cp2->getNode();
      if (cp1->isBTerm() && cp2->isBTerm()) {
        const int rc
            = strcmp(odb::dbBTerm::getBTerm(block, id1)->getName().c_str(),
                     odb::dbBTerm::getBTerm(block, id2)->getName().c_str());
        if (rc != 0) {
          return (rc < 0 ? true : false);
        }
      }
      if (cp1->isBTerm() && !cp2->isBTerm()) {
        return true;
      }
      if (!cp1->isBTerm() && cp2->isBTerm()) {
        return false;
      }
      if (cp1->isITerm() && cp2->isITerm()) {
        if (id1 != id2) {
          return (id1 < id2 ? true : false);
        }
      }
      if (cp1->isITerm() && !cp2->isITerm()) {
        return true;
      }
      if (!cp1->isITerm() && cp2->isITerm()) {
        return false;
      }
      const uint net1 = cp1->getNet()->getId();
      const uint net2 = cp2->getNet()->getId();
      if (net1 != net2) {
        return (net1 < net2 ? true : false);
      }
      if (id1 != id2) {
        return (id1 < id2 ? true : false);
      }
    }
    {
      odb::dbCapNode* cp1 = cc1->getTargetCapNode();
      odb::dbCapNode* cp2 = cc2->getTargetCapNode();

      const uint id1 = cp1->getNode();
      const uint id2 = cp2->getNode();
      if (cp1->isBTerm() && cp2->isBTerm()) {
        const int rc
            = strcmp(odb::dbBTerm::getBTerm(block, id1)->getName().c_str(),
                     odb::dbBTerm::getBTerm(block, id2)->getName().c_str());
        if (rc != 0) {
          return (rc < 0 ? true : false);
        }
      }
      if (cp1->isBTerm() && !cp2->isBTerm()) {
        return true;
      }
      if (!cp1->isBTerm() && cp2->isBTerm()) {
        return false;
      }
      if (cp1->isITerm() && cp2->isITerm()) {
        if (id1 != id2) {
          return (id1 < id2 ? true : false);
        }
      }
      if (cp1->isITerm() && !cp2->isITerm()) {
        return true;
      }
      if (!cp1->isITerm() && cp2->isITerm()) {
        return false;
      }
      const uint net1 = cp1->getNet()->getId();
      const uint net2 = cp2->getNet()->getId();
      if (net1 != net2) {
        return (net1 < net2 ? true : false);
      }
      return (id1 < id2 ? true : false);
    }
  }
};
void extSpef::writeCouplingCapsNoSort(odb::dbSet<odb::dbCCSeg>& capSet,
                                      const uint netId)
{
  for (odb::dbCCSeg* cc : capSet) {
    writeCNodeNumber();

    writeCapNode(cc->getSourceCapNode()->getId(), netId);
    writeCapNode(cc->getTargetCapNode()->getId(), netId);

    fprintf(
        _outFP, "%g", cc->getCapacitance(_active_corner_number[0]) * _cap_unit);
    for (int ii = 1; ii < _active_corner_cnt; ii++) {
      fprintf(_outFP,
              "%s%g",
              _delimiter,
              cc->getCapacitance(_active_corner_number[ii]) * _cap_unit);
    }
    fprintf(_outFP, "\n");
  }
}

void extSpef::writeCouplingCaps(odb::dbSet<odb::dbCCSeg>& capSet,
                                const uint netId)
{
  if (_preserveCapValues) {
    return writeCouplingCapsNoSort(capSet, netId);
  }

  std::vector<odb::dbCCSeg*> vec_cc(capSet.begin(), capSet.end());
  std::sort(vec_cc.begin(), vec_cc.end(), compareCC());
  for (odb::dbCCSeg* cc : vec_cc) {
    writeCNodeNumber();

    writeCapNode(cc->getSourceCapNode()->getId(), netId);
    writeCapNode(cc->getTargetCapNode()->getId(), netId);

    fprintf(
        _outFP, "%g", cc->getCapacitance(_active_corner_number[0]) * _cap_unit);
    for (int ii = 1; ii < _active_corner_cnt; ii++) {
      fprintf(_outFP,
              "%s%g",
              _delimiter,
              cc->getCapacitance(_active_corner_number[ii]) * _cap_unit);
    }
    fprintf(_outFP, "\n");
  }
}

void extSpef::writeTgtCouplingCaps(odb::dbNet* net, const uint netId)
{
  std::vector<odb::dbCCSeg*> vec_cc;
  net->getTgtCCSegs(vec_cc);

  if (!_preserveCapValues) {
    std::sort(vec_cc.begin(), vec_cc.end(), compareCC());
  }

  writeCouplingCaps(vec_cc, netId);
}

void extSpef::writeSrcCouplingCaps(odb::dbNet* net, const uint netId)
{
  std::vector<odb::dbCCSeg*> vec_cc;
  net->getSrcCCSegs(vec_cc);

  if (!_preserveCapValues) {
    std::sort(vec_cc.begin(), vec_cc.end(), compareCC());
  }

  writeCouplingCaps(vec_cc, netId);
}

void extSpef::writeCouplingCaps(const std::vector<odb::dbCCSeg*>& vec_cc,
                                const uint netId)
{
  char msg1[2048];
  for (odb::dbCCSeg* cc : vec_cc) {
    writeCNodeNumber();

    odb::dbCapNode* scnode = cc->getSourceCapNode();
    odb::dbCapNode* tcnode = cc->getTargetCapNode();
    if (scnode->getNet() == tcnode->getNet()) {
      _bufString = _msgBuf2;
      sprintf(_bufString,
              "CC of net %d %s with capacitance %g",
              _d_net->getId(),
              _d_net->getConstName(),
              cc->getCapacitance(_active_corner_number[0]) * _cap_unit);
      for (int ii = 1; ii < _active_corner_cnt; ii++) {
        sprintf(&msg1[0],
                "%s%g",
                _delimiter,
                cc->getCapacitance(_active_corner_number[ii]) * _cap_unit);
        strcat(_bufString, &msg1[0]);
      }
      strcat(_bufString, " has both capNodes ");
      writeCapNode(cc->getSourceCapNode(), netId);
      strcat(_bufString, "and ");
      writeCapNode(cc->getTargetCapNode(), netId);
      _bufString = nullptr;
      continue;
    }
    writeCapNode(cc->getSourceCapNode(), netId);
    writeCapNode(cc->getTargetCapNode(), netId);

    fprintf(
        _outFP, "%g", cc->getCapacitance(_active_corner_number[0]) * _cap_unit);
    for (int ii = 1; ii < _active_corner_cnt; ii++) {
      fprintf(_outFP,
              "%s%g",
              _delimiter,
              cc->getCapacitance(_active_corner_number[ii]) * _cap_unit);
    }

    fprintf(_outFP, "\n");
  }
}

void extSpef::writeNodeCoords(const uint netId, odb::dbSet<odb::dbRSeg>& rSet)
{
  const int dbunit = _block->getDbUnitsPerMicron();
  const double db2nm = 1.0 / ((double) dbunit);

  //*N *1:2 *C 3.06500 125.815
  //*N *1:3 *C 3.03000 120.555
  //*N *2:4 *C 3.07000 120.190
  //*N *2:5 *C 3.07000 120.190

  for (odb::dbRSeg* rc : rSet) {
    const uint shapeId = rc->getShapeId();
    if (!_foreign && shapeId == 0) {
      continue;
    }

    odb::dbCapNode* capNode
        = odb::dbCapNode::getCapNode(_block, rc->getTargetNode());

    if (capNode->isITerm() || capNode->isBTerm()) {
      continue;
    }

    fprintf(_outFP, "*N ");
    writeCapNode(rc->getTargetNode(), netId);

    int x1, y1;
    rc->getCoords(x1, y1);

    fprintf(_outFP, "*C %f %f\n", db2nm * x1, db2nm * y1);
  }
}

bool extSpef::isCapNodeExcluded(odb::dbCapNode* node)
{
  if (node == nullptr || node->getITerm(_block) == nullptr) {
    return false;
  }
  if (node->getITerm(_block)->getInst()->getMaster()->isMarked()) {
    return true;
  }
  return false;
}

void extSpef::writeRes(const uint netId, odb::dbSet<odb::dbRSeg>& rSet)
{
  uint cnt = 1;

  for (odb::dbRSeg* rc : rSet) {
    if (cnt == 1) {
      writeKeyword("*RES");
    }

    if (isCapNodeExcluded(rc->getSourceCapNode())) {
      continue;
    }
    if (isCapNodeExcluded(rc->getTargetCapNode())) {
      continue;
    }

    fprintf(_outFP, "%d ", cnt++);
    writeCapNode(rc->getSourceNode(), netId);
    writeCapNode(rc->getTargetNode(), netId);

    fprintf(
        _outFP, "%g", rc->getResistance(_active_corner_number[0]) * _res_unit);
    for (int ii = 1; ii < _active_corner_cnt; ii++) {
      fprintf(_outFP,
              "%s%g",
              _delimiter,
              rc->getResistance(_active_corner_number[ii]) * _res_unit);
    }
    fprintf(_outFP, " \n");
  }
}

void extSpef::writeNet(odb::dbNet* net, const double resBound, const uint debug)
{
  _d_net = net;
  const uint netId = net->getId();

  if (_cornerBlock && _cornerBlock != _block) {
    net = odb::dbNet::getNet(_cornerBlock, netId);
  }

  uint minNode;
  const uint capNodeCnt = getMinCapNode(net, &minNode);
  if (capNodeCnt) {
    odb::dbSet<odb::dbRSeg> rcSet = net->getRSegs();
    _cCnt = 1;

    double totCap[ADS_MAX_CORNER];
    resetCap(totCap);

    if (_symmetricCCcaps) {
      addCouplingCaps(net, totCap);
    } else {
      logger_->warn(RCX, 175, "Non-symmetric case feature is not implemented!");
    }

    if (_preserveCapValues) {
      getCaps(net, totCap);
      writeDnet(netId, totCap);

      if (_wConn) {
        writeKeyword("*CONN");
        writePorts(net);
        writeITerms(net);
      }
      if (_writingNodeCoords == C_ON) {
        writeNodeCoords(netId, rcSet);
      }

      if (_wCap || _wOnlyCCcap) {
        writeKeyword("*CAP");
      }
      if (_wCap && !_wOnlyCCcap) {
        odb::dbSet<odb::dbCapNode> capSet = net->getCapNodes();

        writeCapPortsAndIterms(capSet, true);
        writeCapPortsAndIterms(capSet, false);
        writeInternalCaps(net, capSet);
      }
    } else {
      _firstCapNode = minNode - 1;

      reinitCapTable(_nodeCapTable, capNodeCnt + 2);

      if (_singleP) {
        computeCapsAdd2Target(rcSet, totCap);
      } else {
        computeCaps(rcSet, totCap);
      }

      writeDnet(netId, totCap);
      if (_wConn) {
        writeKeyword("*CONN");
        writePorts(net);
        writeITerms(net);
      }
      if (_writingNodeCoords == C_ON) {
        writeNodeCoords(netId, rcSet);
      }

      if (_wCap || _wOnlyCCcap) {
        writeKeyword("*CAP");
      }
      if (_wCap && !_wOnlyCCcap) {
        writeCapPorts(net);
        writeCapITerms(net);
        writeNodeCaps(net, netId);
      }
    }
    if (_wCap || _wOnlyCCcap) {
      writeSrcCouplingCaps(net);
    }

    if (_symmetricCCcaps && (_wCap || _wOnlyCCcap)) {
      writeTgtCouplingCaps(net);
    }

    if (_wRes) {
      writeRes(netId, rcSet);
    }
    writeKeyword("*END");
  }
  for (odb::dbCapNode* node : net->getCapNodes()) {
    node->setSortIndex(0);
  }
}

bool extSpef::setInSpef(const char* filename, const bool onlyOpen)
{
  if (filename == nullptr) {
    _inFP = stdin;
    return true;
  }

  strcpy(_inFile, filename);

  if (!onlyOpen) {
    _nodeParser = new Ath__parser(logger_);
    _parser = new Ath__parser(logger_);
  }
  _parser->openFile(filename);

  return true;
}

bool extSpef::setOutSpef(const char* filename)
{
  if (filename == nullptr) {
    return true;
  }

  strcpy(_outFile, filename);

  if (_gzipFlag) {
    char cmd[2048];
    sprintf(cmd, "gzip -1 > %s.gz", filename);
    _outFP = popen(cmd, "w");
  } else {
    _outFP = fopen(filename, "w");
  }

  if (_outFP == nullptr) {
    fprintf(stderr, "Cannot open file %s with permissions \"w\"", filename);
    return false;
  }
  return true;
}

bool extSpef::closeOutFile()
{
  if (_outFP == nullptr) {
    return false;
  }

  if (_gzipFlag) {
    pclose(_outFP);
  } else {
    fclose(_outFP);
  }

  return true;
}

void extSpef::writeBlockPorts()
{
  if (_partial && !_btermFound) {
    return;
  }
  odb::dbSet<odb::dbBTerm> bterms = _block->getBTerms();
  if (!bterms.empty()) {
    writeKeyword("\n*PORTS");
  }

  for (odb::dbBTerm* bterm : bterms) {
    if (bterm->getSigType().isSupply()) {
      continue;
    }
    if (_partial && !bterm->isSetMark()) {
      continue;
    }
    bterm->setMark(0);

    fprintf(_outFP,
            "%s %c\n",
            addEscChar(bterm->getName().c_str(), false),
            bterm->getIoType().getString()[0]);
  }
}

uint extSpef::getNetMapId(const uint netId)
{
  _baseNameMap = std::max(_baseNameMap, netId);

  return netId;
}

// BTerm/ITerm/MTerm
//   any non-alphanum, _ SHOULD be escaped
//   bus brackets at the end of the name should NOT be escaped.
// Instance names
//   For block spef hierarchy dividers SHOULD be escaped.
//   bus brackets SHOULD be escaped.
// Net names
//   For block spef hierarchy dividers SHOULD be escaped
//   bus brackets should NOT be escaped.
// -cherry 04/30/2021
const char* extSpef::addEscChar(const char* iname, const bool esc_bus_brkts)
{
  uint ii = 0;
  uint jj = 0;
  while (iname[ii] != '\0') {
    char ch = iname[ii];
    if (!std::isalnum(ch) && ch != '_' && ch != '\\' && ch != '/'
        &&  // hier delimeters are already escaped if needed
        (esc_bus_brkts || (ch != '[' && ch != ']'))
        // Check if there is an escape char before
        // the non-alphanumeric character
        && (ii == 0 || iname[ii - 1] != '\\')) {
      _mMapName[jj++] = '\\';
    }
    _mMapName[jj++] = iname[ii++];
  }

  _mMapName[jj] = '\0';
  return _mMapName;
}

const char* extSpef::tinkerSpefName(const char* iname)
{
  if (!_noBackSlash) {
    return iname;
  }
  uint ii = 0;
  uint jj = 0;
  while (iname[ii] != '\0') {
    if (_noBackSlash && iname[ii] == '\\')  // strip off backslash
    {
      ii++;
      continue;
    }
    _mMapName[jj] = iname[ii];
    ii++;
    jj++;
  }
  _mMapName[jj] = '\0';
  return (&_mMapName[0]);
}

void extSpef::writeNetMap(odb::dbSet<odb::dbNet>& nets)
{
  _btermFound = false;
  for (odb::dbNet* net : nets) {
    if (net->getSigType().isSupply()) {
      continue;
    }
    if (!_partial || !net->isMarked()) {
      continue;
    }
    net->setMark_1(true);
    for (odb::dbITerm* iterm : net->getITerms()) {
      iterm->getInst()->setUserFlag1();
    }
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      _btermFound = true;
      bterm->setMark(1);
    }
    for (odb::dbCapNode* capn : net->getCapNodes()) {
      for (odb::dbCCSeg* cc : capn->getCCSegs()) {
        odb::dbCapNode* tcap = cc->getSourceCapNode();
        if (tcap == capn) {
          tcap = cc->getTargetCapNode();
        }
        if (tcap->isITerm()) {
          tcap->getITerm(_block)->getInst()->setUserFlag1();
        } else if (tcap->isBTerm()) {
          tcap->getBTerm(_block)->setMark(1);
        } else {
          tcap->getNet()->setMark_1(true);
        }
      }
    }
  }
  for (odb::dbNet* net : nets) {
    if (net->getSigType().isSupply()) {
      continue;
    }
    if (_partial && !net->isMark_1ed()) {
      continue;
    }
    net->setMark_1(false);
    const uint netMapId = getNetMapId(net->getId());

    const char* nname = net->getConstName();
    const char* nname1 = tinkerSpefName(nname);
    nname1 = addEscChar(nname1, false);
    fprintf(_outFP, "*%d %s\n", netMapId, nname1);
  }
}

void extSpef::writeInstMap()
{
  for (odb::dbInst* inst : _block->getInsts()) {
    // for flat block won't make any difference!!!
    if (inst->getChild() != nullptr) {
      continue;
    }

    if (inst->getMaster()->getMTermCount() <= 0) {
      continue;
    }
    if (inst->getMaster()->isMarked()) {
      logger_->info(RCX,
                    176,
                    "Skip instance {} for cell {} is excluded",
                    inst->getConstName(),
                    inst->getMaster()->getConstName());
      continue;
    }
    if (_partial && !inst->getUserFlag1()) {
      continue;
    }
    inst->clearUserFlag1();

    const uint instMapId = getInstMapId(inst->getId());

    const char* nname = inst->getConstName();
    const char* nname1 = tinkerSpefName(nname);
    nname1 = addEscChar(nname1, true);
    fprintf(_outFP, "*%d %s\n", instMapId, nname1);
  }
}

void extSpef::stopWrite()
{
  closeOutFile();
}

void extSpef::writeBlock(char* nodeCoord,
                         const char* capUnit,
                         const char* resUnit,
                         bool stopAfterNameMap,
                         std::vector<odb::dbNet*>* tnets,
                         bool wClock,
                         bool wConn,
                         bool wCap,
                         bool wOnlyCCcap,
                         bool wRes,
                         bool noCnum,
                         bool stopBeforeDnets,
                         bool noBackSlash,
                         bool parallel)
{
  return writeBlock(nodeCoord,
                    capUnit,
                    resUnit,
                    stopAfterNameMap,
                    *tnets,
                    wClock,
                    wConn,
                    wCap,
                    wOnlyCCcap,
                    wRes,
                    noCnum,
                    stopBeforeDnets,
                    noBackSlash,
                    parallel);
}

void extSpef::writeBlock(const char* nodeCoord,
                         const char* capUnit,
                         const char* resUnit,
                         const bool stopAfterNameMap,
                         const std::vector<odb::dbNet*>& tnets,
                         const bool wClock,
                         const bool wConn,
                         const bool wCap,
                         const bool wOnlyCCcap,
                         const bool wRes,
                         const bool noCnum,
                         const bool stopBeforeDnets,
                         const bool noBackSlash,
                         const bool parallel)
{
  // _block is always the original block! even when #NEW_EXTRACTION_CORNER_DB
  _wOnlyClock = wClock;
  _wConn = wConn;
  _wCap = wCap;
  _wOnlyCCcap = wOnlyCCcap;
  _wRes = wRes;
  _noCnum = noCnum;
  _noBackSlash = noBackSlash;
  _foreign = _block->getExtControl()->_foreign;

  _writingNodeCoords = C_NONE;
  if (nodeCoord && nodeCoord[0] != '\0') {
    _writingNodeCoords = C_ON;
  }
  if (!wConn && !wCap && !wOnlyCCcap && !wRes) {
    _wConn = _wCap = _wRes = true;
  }

  _partial = !tnets.empty();
  for (odb::dbNet* tnet : tnets) {
    tnet->setMark(true);
  }

  if (!_stopBeforeDnets && !_stopAfterNameMap) {
    if (strcmp("PF", capUnit) == 0) {
      _cap_unit = 0.001;
      strcpy(_cap_unit_word, capUnit);
    }
    if (strcmp("MOHM", resUnit) == 0) {
      _res_unit = 1000.0;
      strcpy(_res_unit_word, resUnit);
    } else if (strcmp("KOHM", resUnit) == 0) {
      _res_unit = 0.001;
      strcpy(_res_unit_word, resUnit);
    }
    setCornerCnt(_block->getCornerCount());

    if (!_preserveCapValues) {
      setupMappingForWrite();
    }

    writeHeaderInfo();

    if (_writeNameMap) {
      writeKeyword("\n*NAME_MAP");
      odb::dbSet<odb::dbNet> nets = _block->getNets();
      writeNetMap(nets);
      writeInstMap();
    } else {
      // avoid writing *PORTS for incremental SPEF
      _btermFound = false;
    }
  }

  if (stopAfterNameMap) {
    _stopAfterNameMap = true;
    return;
  }

  if (stopBeforeDnets) {
    _stopBeforeDnets = true;
    writeBlockPorts();
    return;
  }
  if (!_stopBeforeDnets) {
    writeBlockPorts();
  }

  _cornerBlock = nullptr;
  _cornersPerBlock = _cornerCnt;
  _cornerBlock = _block;

  uint cnt = 0;

  for (odb::dbNet* net : _block->getNets()) {
    if (!tnets.empty() && !net->isMarked()) {
      if (!_incrPlusCcNets || net->getCcCount() == 0) {
        continue;
      }
    }
    const odb::dbSigType type = net->getSigType();
    if (type.isSupply()) {
      continue;
    }
    if (_wOnlyClock && type != odb::dbSigType::CLOCK) {
      continue;
    }

    writeNet(net, 0.0, 0);
    ++cnt;

    constexpr uint repChunk = 100000;
    if (cnt % repChunk == 0) {
      logger_->info(RCX, 42, "{} nets finished", cnt);
    }
  }
  for (odb::dbNet* net : tnets) {
    net->setMark(false);
  }
  logger_->info(RCX, 443, "{} nets finished", cnt);

  closeOutFile();
}

void extSpef::write_spef_nets(const bool flatten, const bool parallel)
{
  _childBlockNetBaseMap = 0;
  _childBlockInstBaseMap = 0;
  _cornerBlock = _block;
  _cornersPerBlock = _cornerCnt;

  uint cnt = 0;

  for (odb::dbNet* net : _block->getNets()) {
    const odb::dbSigType type = net->getSigType();
    if (type.isSupply()) {
      continue;
    }
    if (_wOnlyClock && type != odb::dbSigType::CLOCK) {
      continue;
    }

    odb::dbSet<odb::dbRSeg> rSet = net->getRSegs();
    rSet.reverse();
    writeNet(net, 0.0, 0);
    ++cnt;

    constexpr uint repChunk = 50000;
    if (cnt % repChunk == 0) {
      logger_->info(RCX, 465, "{} nets finished", cnt);
    }
  }
  logger_->info(RCX, 47, "{} nets finished", cnt);
}

uint extSpef::getMappedBTermId(const uint spefId)
{
  if (_testParsing || _statsOnly) {
    return 0;
  }
  const char* name = _nameMapTable->geti(spefId);
  odb::dbBTerm* bterm = _block->findBTerm(name);
  return bterm->getId();
}

void extSpef::writeHeaderInfo()
{
  fprintf(_outFP, "*SPEF \"ieee 1481-1999\"\n");
  fprintf(_outFP, "*DESIGN \"%s\"\n", _design);

  std::time_t currentTime = std::time(nullptr);

  // Format the current time as a string
  char buffer[128];
  std::strftime(buffer,
                sizeof(buffer),
                "%H:%M:%S %A %B %d, %Y",
                std::localtime(&currentTime));

  fprintf(_outFP, "*DATE \"%s\"\n", buffer);

  fprintf(_outFP, "*VENDOR \"The OpenROAD Project\"\n");
  fprintf(_outFP, "*PROGRAM \"OpenROAD\"\n");
  fprintf(_outFP, "*VERSION \"%s\"\n", _version);
  fprintf(_outFP, "*DESIGN_FLOW \"NAME_SCOPE LOCAL\" \"PIN_CAP NONE\"\n");
  fprintf(_outFP, "*DIVIDER %s\n", _divider);
  fprintf(_outFP, "*DELIMITER %s\n", _delimiter);
  fprintf(_outFP, "*BUS_DELIMITER %s\n", _bus_delimiter);
  fprintf(_outFP, "*T_UNIT %d %s\n", _time_unit, _time_unit_word);
  fprintf(_outFP, "*C_UNIT %d %s\n", 1, _cap_unit_word);
  fprintf(_outFP, "*R_UNIT %d %s\n", 1, _res_unit_word);
  fprintf(_outFP, "*L_UNIT %d %s\n", _ind_unit, _ind_unit_word);
}

}  // namespace rcx
