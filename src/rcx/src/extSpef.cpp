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
#include "rcx/extRCap.h"
#include <dbExtControl.h>
#include <math.h>

#include <algorithm>

#include "parse.h"
#include "utl/Logger.h"

//#ifdef _WIN32
#define ATH__fprintf fprintf
#define ATH__fopen fopen
#define ATH__fclose fclose
//#endif

namespace rcx {

using utl::RCX;

class extMain;

extSpef::extSpef(odb::dbTech* tech,
                 odb::dbBlock* blk,
                 Logger* logger,
                 extMain* extmain,
                 uint btermCnt,
                 uint itermCnt)
{
  logger_ = logger;
  _ext = extmain;
  _tech = tech;
  _block = blk;
  _blockId = 0;

  if (blk != NULL)
    _blockId = blk->getId();

  _outFP = NULL;

  // strcpy(_divider, ".");
  strcpy(_divider, "/");
  strcpy(_delimiter, ":");
  strcpy(_bus_delimiter, "[]");

  strcpy(_res_unit_word, "OHM");
  strcpy(_cap_unit_word, "FF");
  strcpy(_time_unit_word, "NS");
  strcpy(_ind_unit_word, "HENRY");
  _design[0] = '\0';

  _res_unit = 1.0;
  _cap_unit = 1.0;
  _time_unit = 1;
  _ind_unit = 1;

  _nodeCoordParser = NULL;
  _nodeParser = NULL;
  _parser = NULL;
  _tmpNetSpefId = 0;
  _baseNameMap = 0;

  _extracted = false;
  _termJxy = false;

  _wRun = 0;
  _wConn = false;
  _wCap = false;
  _wOnlyCCcap = false;
  _wRes = false;
  _noCnum = false;
  _foreign = false;

  _rRun = 0;
  _stampWire = false;
  _netSdb = NULL;
  _rConn = false;
  _rCap = false;
  _rOnlyCCcap = false;
  _rRes = false;
  _inputNet = false;

  _cornerCnt = 0;  // TODO

  _idMapTable = new Ath__array1D<uint>(
      128000);  // for ABNORMAL and sparse map numbers should be a hash table
  _nameMapTable = NULL;
  _lastNameMapIndex = 0;

  _btermTable = NULL;
  _itermTable = NULL;
  _nodeTable = NULL;
  _btermCapTable = NULL;
  _itermCapTable = NULL;
  _nodeCapTable = NULL;
  _rcPool = NULL;
  _rcTrippletTable = NULL;

  _capNodeTable = NULL;
  _xCoordTable = NULL;
  _yCoordTable = NULL;
  _x1CoordTable = NULL;
  _y1CoordTable = NULL;
  _x2CoordTable = NULL;
  _y2CoordTable = NULL;
  _levelTable = NULL;
  _search = NULL;
  _idTable = NULL;

  _partial = false;

  _noBackSlash = false;
  _useIds = false;
  _preserveCapValues = false;
  _symmetricCCcaps = true;

  _testParsing = false;
  _noCapNumCollapse = false;
  _capNodeFile = NULL;

  _addRepeatedCapValue = true;

  _nodeHashTable = NULL;
  _node2nodeHashTable = NULL;
  _tmpCapId = 1;

  _gzipFlag = false;
  _stopAfterNameMap = false;
  _stopBeforeDnets = false;
  _calib = false;
  _match = false;
  _diff = false;
  _diffLogFP = NULL;
  _diffOutFP = NULL;

  _cc_break_cnt = 0;
  _cc_merge_cnt = 0;
  _cc_app_print_limit = 0;
  _ccidmap = NULL;
  _resCnt = 0;
  _gndCapCnt = 0;
  _ccCapCnt = 0;
  _statsOnly = false;
  _netExcludeSubWord = NULL;
  _netSubWord = NULL;
  _capStatsFP = NULL;
  _tmpNetName = NULL;

  _singleP = false;
  _NsLayer = true;
  _doSortRSeg = true;
  _readingNodeCoords = C_NONE;
  _writingNodeCoords = C_NONE;

  _srsegi = new Ath__array1D<uint>(1024);
  _nrseg = new Ath__array1D<odb::dbRSeg*>(1024);
  _hcnrc = new Ath__array1D<Ath__array1D<int>*>(1024);
  for (uint ii = 0; ii < 1024; ii++) {
    Ath__array1D<int>* n1d = new Ath__array1D<int>(4);
    _hcnrc->set(ii, n1d);
  }

  _readAllCorners = false;
  _in_spef_corner = -1;
  _db_ext_corner = -1;

  _active_corner_cnt = 0;

  _useBaseCornerRc = false;

  _incrPlusCcNets = false;

  _bufString = NULL;
  _msgBuf1 = (char*) malloc(sizeof(char) * 2048);
  _msgBuf2 = (char*) malloc(sizeof(char) * 2048);

  // 021810D BEGIN
  _childBlockInstBaseMap = 0;
  _childBlockNetBaseMap = 0;
  // 021810D END
}

extSpef::~extSpef()
{
  delete _idMapTable;
  if (_nodeParser)
    delete _nodeParser;
  if (_parser)
    delete _parser;
  if (_nodeTable)
    delete _nodeTable;
  if (_btermTable)
    delete _btermTable;
  if (_itermTable)
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
  if (_nameMapTable)
    delete _nameMapTable;
  if (_nodeHashTable)
    delete _nodeHashTable;
  if (_node2nodeHashTable)
    delete _node2nodeHashTable;
  if (_rcPool)
    delete _rcPool;
  if (_rcTrippletTable)
    delete _rcTrippletTable;
  if (_ccidmap)
    delete _ccidmap;
  if (_nodeCoordParser)
    delete _nodeCoordParser;
  free(_msgBuf1);
  free(_msgBuf2);
}
void extSpef::setBlock(odb::dbBlock* blk)
{
  _block = blk;
}
void extSpef::set_single_pi(bool v)
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
  if ((itermCnt == 0) && (_block != NULL)) {
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
uint extSpef::getMultiples(uint cnt, uint base)
{
  return ((cnt / base) + 1) * base;
}
void extSpef::setCalibLimit(float upperLimit, float lowerLimit)
{
  _upperCalibLimit = upperLimit;
  _lowerCalibLimit = lowerLimit;
  if (_upperCalibLimit == 0.0 && _lowerCalibLimit == 0.0)
    _match = true;
  else
    _match = false;
}

void extSpef::setUseIdsFlag(bool useIds, bool diff, bool calib)
{
  _useIds = useIds;
  _diff = diff;
  _calib = calib;
  if (diff && !calib) {
    _diffLogFP = ATH__fopen("diff_spef.log", "w");
    if (!_diffLogFP)
      logger_->error(
          RCX,
          171,
          "Can't open log file diff_spef.log for writing. Do you have "
          "permissions?");
    _diffOutFP = ATH__fopen("diff_spef.out", "w");
    if (!_diffOutFP)
      logger_->error(RCX,
                     172,
                     "Can't open output file diff_spef.out for writing. Do you "
                     "have permissions?");
  }
}
void extSpef::preserveFlag(bool v)
{
  _preserveCapValues = v;
}
void extSpef::setCornerCnt(uint n)
{
  _cornerCnt = n;
}
void extSpef::setGzipFlag(bool gzFlag)
{
  _gzipFlag = gzFlag;
}
void extSpef::resetTermTables()
{
  _btermTable->resetCnt(1);
  _itermTable->resetCnt(1);
}
void extSpef::resetCapTables(uint maxNode)
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
    delete a;
  }
}
void extSpef::reinitCapTable(Ath__array1D<double*>* table, uint n)
{
  if (n > table->getSize()) {
    uint prevCnt = table->getSize();
    table->reSize(n);  // wfs
    table->resetCnt(prevCnt);
    double* a = new double[_cornerCnt];
    table->add(a);
    uint currentCnt = table->getSize();
    for (uint ii = prevCnt + 1; ii < currentCnt; ii++) {
      double* a = new double[_cornerCnt];
      table->add(a);
    }
  }
  for (uint kk = 1; kk < n; kk++) {
    double* a = table->get(kk);

    for (uint jj = 0; jj < _cornerCnt; jj++)
      a[jj] = 0.0;
  }
}
void extSpef::setDesign(char* name)
{
  strcpy(_design, name);
}
uint extSpef::getInstMapId(uint id)
{
  // 021810D BEGIN
  if (_childBlockInstBaseMap > 0)
    return _childBlockInstBaseMap + id;
  // 021810D END

  return _baseNameMap + id;
}
void extSpef::writeNameNode(odb::dbCapNode* node)
{
  odb::dbStringProperty* p = odb::dbStringProperty::find(node, "_inode");

  if (_bufString) {
    sprintf(_msgBuf1, "%s ", p->getValue().c_str());
    strcat(_bufString, _msgBuf1);
  } else {
    ATH__fprintf(_outFP, "%s ", p->getValue().c_str());
  }
}
void extSpef::writeITermNode(uint node)
{
  char ttname[256];
  odb::dbITerm* iterm = odb::dbITerm::getITerm(_block, node);
  odb::dbInst* inst = iterm->getInst();
  if (inst->getMaster()->isMarked())
    return;

  if (_bufString) {
    if (_writeNameMap)
      sprintf(_msgBuf1,
              "*%d%s%s ",
              getInstMapId(inst->getId()),
              _delimiter,
              addEscChar(iterm->getMTerm()->getName(inst, &ttname[0]), false));
    else
      sprintf(_msgBuf1,
              "%s%s%s ",
              addEscChar(tinkerSpefName((char*) inst->getConstName()), true),
              _delimiter,
              addEscChar(iterm->getMTerm()->getName(inst, &ttname[0]), false));
    strcat(_bufString, _msgBuf1);
  } else {
    if (_writeNameMap)
      ATH__fprintf(_outFP,
                   "*%d%s%s ",
                   getInstMapId(inst->getId()),
                   _delimiter,
                   addEscChar(iterm->getMTerm()->getName(inst, &ttname[0]), false));
    else
      ATH__fprintf(_outFP,
                   "%s%s%s ",
                   addEscChar(tinkerSpefName((char*) inst->getConstName()), true),
                   _delimiter,
                   addEscChar(iterm->getMTerm()->getName(inst, &ttname[0]), false));
  }
  /*
  odb::debug("HEXT",
             "T",
             "writeITermNode= %d i%d- %d%s%s %s\n",
             node,
             inst->getId(),
             getInstMapId(inst->getId()),
             _delimiter,
             iterm->getMTerm()->getName(inst, &ttname[0]),
             inst->getConstName()); */
}
bool extSpef::writeITerm(uint node)
{
  odb::dbITerm* iterm = odb::dbITerm::getITerm(_block, node);
  if (iterm->getInst()->getMaster()->isMarked())
    return false;

  ATH__fprintf(_outFP, "*I ");
  writeITermNode(node);

  char io = iterm->getIoType().getString()[0];
  //	if (io=='I')
  //	{
  ////		ATH__fprintf(_outFP, " %c\n", io);
  //		ATH__fprintf(_outFP, " %c *D %s\n", io,
  //			iterm->getMTerm()->getMaster()->getName().c_str());
  //	}
  //	else
  //	{
  //		ATH__fprintf(_outFP, " %c *D %s\n", io,
  //			iterm->getMTerm()->getMaster()->getName().c_str());
  //	}
  ATH__fprintf(_outFP, "%c ", io);
  // double db2nm= 0.001; // TODO: make it defunits driven
  int dbunit = _block->getDbUnitsPerMicron();
  double db2nm = 1.0 / ((double) dbunit);
  int x1 = 0;
  int y1 = 0;
  int jid = 0;
  odb::dbWire* wire = _d_net->getWire();
  if (wire)
    jid = wire->getTermJid(iterm->getId());
  if (_writingNodeCoords == C_STARRC) {
    if (_termJxy && jid) {
      wire->getCoord(jid, x1, y1);
    } else
      iterm->getAvgXY(&x1, &y1);
    ATH__fprintf(_outFP, "*C %f %f ", db2nm * x1, db2nm * y1);
  }
  ATH__fprintf(
               _outFP, "*D %s\n", addEscChar(iterm->getMTerm()->getMaster()->getName().c_str(), false));
  return true;
}
bool extSpef::writeBTerm(uint node)
{
  odb::dbBTerm* bterm = odb::dbBTerm::getBTerm(_block, node);
  if (_bufString) {
    if (_useIds)
      sprintf(_msgBuf1, "B%d ", bterm->getId());
    else
      sprintf(_msgBuf1, "%s ", addEscChar(bterm->getName().c_str(), false));
    strcat(_bufString, _msgBuf1);
  } else {
    if (_useIds)
      ATH__fprintf(_outFP, "B%d ", bterm->getId());
    else
      ATH__fprintf(_outFP, "%s ", addEscChar(bterm->getName().c_str(), false));
  }

  return true;
}
bool extSpef::writeNode(uint netId, uint node)
{
  // 022210D odb::dbNet *tnet = odb::dbNet::getNet(_block, netId);
  odb::dbNet* tnet = _d_net;
  if (_bufString) {
    if (_writeNameMap)
      sprintf(_msgBuf1, "*%d%s%d ", netId, _delimiter, node);
    else
      sprintf(_msgBuf1,
              "%s%s%d ",
              addEscChar(tinkerSpefName((char*) tnet->getConstName()), false),
              _delimiter,
              node);
    strcat(_bufString, _msgBuf1);
  } else {
    if (_writeNameMap)
      ATH__fprintf(_outFP, "*%d%s%d ", netId, _delimiter, node);
    else
      ATH__fprintf(_outFP,
                   "%s%s%d ",
                   addEscChar(tinkerSpefName((char*) tnet->getConstName()), false),
                   _delimiter,
                   node);
  }
  return true;
}
bool extSpef::writeCapNode(uint capNodeId, uint netId)
{
  odb::dbCapNode* capNode = odb::dbCapNode::getCapNode(_block, capNodeId);
  return writeCapNode(capNode, netId);
}
bool extSpef::writeCapNode(odb::dbCapNode* capNode, uint netId)
{
  if (netId == 0)
    netId = capNode->getNet()->getId();

  if (capNode->isITerm()) {
    writeITermNode(capNode->getNode());
  } else if (capNode->isBTerm()) {
    writeBTerm(capNode->getNode());
  } else if (capNode->isInternal()) {
    if (_childBlockNetBaseMap > 0)
      netId += _childBlockNetBaseMap;

    writeNode(netId, capNode->getNode());
  } else if (capNode->isName()) {
    writeNameNode(capNode);
  }

  return true;
}
uint extSpef::writeCapITerm(uint node, uint capIndex)
{
  if ((odb::dbITerm::getITerm(_block, node))
          ->getInst()
          ->getMaster()
          ->isMarked())
    return 0;
  writeCNodeNumber();

  writeITermNode(node);
  writeRCvalue(_nodeCapTable->geti(capIndex), _cap_unit);

  ATH__fprintf(_outFP, "\n");

  return 1;
}
uint extSpef::writeCapName(odb::dbCapNode* capNode, uint capIndex)
{
  writeCNodeNumber();

  writeNameNode(capNode);
  writeRCvalue(_nodeCapTable->geti(capIndex), _cap_unit);

  ATH__fprintf(_outFP, "\n");

  return 1;
}
uint extSpef::writeCapPort(uint node, uint capIndex)
{
  writeCNodeNumber();

  writeBTerm(node);

  writeRCvalue(_nodeCapTable->geti(capIndex), _cap_unit);
  ATH__fprintf(_outFP, "\n");

  return 1;
}
uint extSpef::writePort(uint node)
{
  odb::dbBTerm* bterm = odb::dbBTerm::getBTerm(_block, node);
  if (_useIds) {
    ATH__fprintf(
        _outFP, "*P B%d %c", bterm->getId(), bterm->getIoType().getString()[0]);
  } else {
    ATH__fprintf(_outFP,
                 "*P %s %c",
                 addEscChar(bterm->getName().c_str(), false),
                 bterm->getIoType().getString()[0]);
  }
  if (_writingNodeCoords != C_STARRC) {
    ATH__fprintf(_outFP, "\n");
    return 1;
  }
  // double db2nm= 0.001; // TODO: make it defunits driven
  int dbunit = _block->getDbUnitsPerMicron();
  double db2nm = 1.0 / ((double) dbunit);
  int x1 = 0;
  int y1 = 0;
  int jid = 0;
  odb::dbWire* wire = _d_net->getWire();
  if (wire)
    jid = wire->getTermJid(-bterm->getId());
  if (_termJxy && jid) {
    wire->getCoord(jid, x1, y1);
  } else
    bterm->getFirstPinLocation(x1, y1);
  ATH__fprintf(_outFP, " *C %f %f\n", db2nm * x1, db2nm * y1);
  return 1;
}
void extSpef::writeSingleRC(double val, bool delimeter)
{
  if (delimeter)
    ATH__fprintf(_outFP, "%s%g", _delimiter, val * _cap_unit);
  else
    ATH__fprintf(_outFP, "%g", val * _cap_unit);
}
void extSpef::writeRCvalue(double* totCap, double units)
{
  ATH__fprintf(_outFP, "%g", totCap[_active_corner_number[0]] * units);
  for (int ii = 1; ii < _active_corner_cnt; ii++)
    ATH__fprintf(
        _outFP, "%s%g", _delimiter, totCap[_active_corner_number[ii]] * units);
}
void extSpef::writeDnet(uint netId, double* totCap)
{
  netId = getNetMapId(netId);

  if (_writeNameMap)
    ATH__fprintf(_outFP, "\n*D_NET *%d ", netId);
  else
    ATH__fprintf(
                 _outFP, "\n*D_NET %s ", addEscChar(tinkerSpefName((char*) _d_net->getConstName()), false));
  
  writeRCvalue(totCap, _cap_unit);
  ATH__fprintf(_outFP, "\n");
}
void extSpef::writeKeyword(const char* keyword)
{
  ATH__fprintf(_outFP, "%s\n", keyword);
}
void extSpef::addCap(double* cap, double* totCap, uint n)
{
  for (uint ii = 0; ii < n; ii++)
    totCap[ii] += cap[ii];
}
void extSpef::incrementCounter(double* cap, uint n)
{
  cap[n] += 1.0;
}
void extSpef::setCap(double* cap, uint n, double* totCap, uint startIndex)
{
  for (uint jj = 0; jj < startIndex; jj++)
    totCap[jj] = 0.0;

  for (uint ii = 0; ii < n; ii++)
    totCap[startIndex++] = cap[ii];
}
void extSpef::resetCap(double* cap)
{
  resetCap(cap, _cornerCnt);
}

void extSpef::resetCap(double* cap, uint cnt)
{
  for (uint ii = 0; ii < cnt; ii++)
    cap[ii] = 0.0;
}
void extSpef::copyCap(double* totCap, double* cap, uint n)
{
  if (n == 0)
    n = _cornerCnt;

  for (uint ii = 0; ii < n; ii++)
    totCap[ii] = cap[ii];
}
void extSpef::adjustCap(double* totCap, double* cap, uint n)
{
  if (n == 0)
    n = _cornerCnt;

  for (uint ii = 0; ii < n; ii++)
    totCap[ii] += cap[ii];
}
void extSpef::addHalfCap(double* totCap, double* cap, uint n)
{
  if (n == 0)
    n = _cornerCnt;

  for (uint ii = 0; ii < n; ii++)
    totCap[ii] += cap[ii] / 2;
}
uint extSpef::getMappedCapNode(uint nodeId)
{
  return nodeId - _firstCapNode;
}

uint extSpef::computeCaps(odb::dbSet<odb::dbRSeg>& rcSet, double* totCap)
{
  odb::dbSet<odb::dbRSeg>::iterator rc_itr;

  uint cnt = 0;
  //	uint min= 200000000;
  //	uint max= 0;

  double cap[10];  // TODO: allow more
  for (rc_itr = rcSet.begin(); rc_itr != rcSet.end(); ++rc_itr) {
    odb::dbRSeg* rc = *rc_itr;

    rc->getCapTable(cap);
    addCap(cap, totCap, this->_cornerCnt);

    uint trgNodeId = rc->getTargetNode();
    /*
    odb::dbCapNode *capNode= odb::dbCapNode::getCapNode(_cornerBlock,
    trgNodeId); capNode->getCapTable(cap);
    */
    uint trgMappedNode
        = odb::dbCapNode::getCapNode(_cornerBlock, trgNodeId)->getSortIndex();
    addHalfCap(_nodeCapTable->geti(trgMappedNode), cap);

    uint srcNodeId = rc->getSourceNode();
    uint srcMappedNode
        = odb::dbCapNode::getCapNode(_cornerBlock, srcNodeId)->getSortIndex();
    addHalfCap(_nodeCapTable->geti(srcMappedNode), cap);

    cnt++;
  }

  return cnt;
}
uint extSpef::computeCapsAdd2Target(odb::dbSet<odb::dbRSeg>& rcSet,
                                    double* totCap)
{
  odb::dbSet<odb::dbRSeg>::iterator rc_itr;

  uint cnt = 0;
  //`	uint min= 200000000;
  //	uint max= 0;

  double cap[10];  // TODO: allow more
  for (rc_itr = rcSet.begin(); rc_itr != rcSet.end(); ++rc_itr) {
    odb::dbRSeg* rc = *rc_itr;

    rc->getCapTable(cap);
    addCap(cap, totCap, this->_cornerCnt);

    uint trgNodeId = rc->getTargetNode();
    uint trgMappedNode
        = odb::dbCapNode::getCapNode(_block, trgNodeId)->getSortIndex();
    adjustCap(_nodeCapTable->geti(trgMappedNode), cap);

    cnt++;
  }
  return cnt;
}
void extSpef::getCaps(odb::dbNet* net, double* totCap)
{
  odb::dbSet<odb::dbCapNode> nodeSet = net->getCapNodes();

  odb::dbSet<odb::dbCapNode>::iterator rc_itr;

  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    odb::dbCapNode* node = *rc_itr;

    double cap[16];
    for (uint ii = 0; ii < _cornersPerBlock; ii++)
      cap[ii] = node->getCapacitance(ii);

    addCap(cap, totCap, _cornersPerBlock);
  }
}
void extSpef::addCouplingCaps(odb::dbNet* net, double* totCap)
{
  double cap[16];
  for (uint ii = 0; ii < _cornersPerBlock; ii++)
    cap[ii] = net->getTotalCouplingCap(ii);

  addCap(cap, totCap, _cornersPerBlock);
}
void extSpef::addCouplingCaps(odb::dbSet<odb::dbCCSeg>& capSet, double* totCap)
{
  odb::dbSet<odb::dbCCSeg>::iterator cap_itr;
  for (cap_itr = capSet.begin(); cap_itr != capSet.end(); ++cap_itr) {
    odb::dbCCSeg* cc = *cap_itr;

    double cap[16];
    for (uint ii = 0; ii < _cornerCnt; ii++)
      cap[ii] = cc->getCapacitance(ii);

    addCap(cap, totCap, _cornerCnt);
  }
}
uint extSpef::getMinCapNode(odb::dbNet* net, uint* minNode)
{
  odb::dbSet<odb::dbCapNode> nodeSet = net->getCapNodes();
  // nodeSet.reverse();

  odb::dbSet<odb::dbCapNode>::iterator rc_itr;

  uint cnt = 0;
  uint min = 0;
  uint max = 0;
  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    odb::dbCapNode* node = *rc_itr;
    cnt++;
    node->setSortIndex(cnt);

    min = min == 0 ? node->getId() : MIN(min, node->getId());
    max = MAX(max, node->getId());
  }
  if (cnt == 0) {
    *minNode = 0;
    return 0;
  } else {
    *minNode = min;
    return cnt + 1;
  }
}
void extSpef::writeCNodeNumber()
{
  if (_noCnum)
    return;
  ATH__fprintf(_outFP, "%d ", _cCnt++);
}
// void extSpef::writeNodeCaps(uint netId, uint minNode, uint maxNode)
//{
//	for (uint ii= minNode; ii<=maxNode; ii++)
//	{
//		writeCNodeNumber();
//		writeNode(netId, ii);
//		writeRCvalue(_nodeCapTable->get(ii), _cap_unit);
//		ATH__fprintf(_outFP, "\n");
//	}
//}
void extSpef::writeNodeCap(uint netId, uint capIndex, uint ii)
{
  writeCNodeNumber();
  writeNode(netId, ii);
  writeRCvalue(_nodeCapTable->geti(capIndex), _cap_unit);
  ATH__fprintf(_outFP, "\n");
}
uint extSpef::writePorts(odb::dbNet* net)
{
  uint cnt = 0;
  odb::dbSet<odb::dbCapNode> capSet = net->getCapNodes();
  odb::dbSet<odb::dbCapNode>::iterator cap_itr;
  for (cap_itr = capSet.begin(); cap_itr != capSet.end(); ++cap_itr) {
    odb::dbCapNode* capNode = *cap_itr;

    if (!capNode->isBTerm())
      continue;

    writePort(capNode->getNode());
    cnt++;
  }
  return cnt;
}
uint extSpef::writeInternalCaps(odb::dbNet* net,
                                odb::dbSet<odb::dbCapNode>& capSet)
{
  odb::dbSet<odb::dbCapNode>::iterator cap_itr;
  for (cap_itr = capSet.begin(); cap_itr != capSet.end(); ++cap_itr) {
    odb::dbCapNode* capNode = *cap_itr;

    if (!capNode->isInternal())
      continue;

    writeCNodeNumber();
    writeNode(net->getId(), capNode->getNode());

    writeSingleRC(capNode->getCapacitance(_active_corner_number[0]), false);
    for (int ii = 1; ii < _active_corner_cnt; ii++)
      writeSingleRC(capNode->getCapacitance(_active_corner_number[ii]), true);

    ATH__fprintf(_outFP, "\n");
  }
  return _cCnt;
}
uint extSpef::writeCapPortsAndIterms(odb::dbSet<odb::dbCapNode>& capSet,
                                     bool bterms)
{
  odb::dbSet<odb::dbCapNode>::iterator cap_itr;
  for (cap_itr = capSet.begin(); cap_itr != capSet.end(); ++cap_itr) {
    odb::dbCapNode* capNode = *cap_itr;

    if (capNode->isInternal())
      continue;

    if (capNode->isBTerm()) {
      if (!bterms)
        continue;
      writeCNodeNumber();
      writeBTerm(capNode->getNode());
    } else if (capNode->isITerm()) {
      if (bterms)
        continue;
      if (capNode->getITerm(_block)->getInst()->getMaster()->isMarked())
        continue;
      writeCNodeNumber();
      writeITermNode(capNode->getNode());
    } else
      continue;

    writeSingleRC(capNode->getCapacitance(_active_corner_number[0]), false);
    for (int ii = 1; ii < _active_corner_cnt; ii++)
      writeSingleRC(capNode->getCapacitance(_active_corner_number[ii]), true);

    ATH__fprintf(_outFP, "\n");
  }
  return _cCnt;
}
uint extSpef::writeCapPorts(odb::dbNet* net)
{
  odb::dbSet<odb::dbCapNode> capSet = net->getCapNodes();
  odb::dbSet<odb::dbCapNode>::iterator cap_itr;
  for (cap_itr = capSet.begin(); cap_itr != capSet.end(); ++cap_itr) {
    odb::dbCapNode* capNode = *cap_itr;

    if (!capNode->isBTerm())
      continue;

    writeCapPort(capNode->getNode(), capNode->getSortIndex());
  }
  return _cCnt;
}
uint extSpef::writeITerms(odb::dbNet* net)
{
  uint cnt = 0;
  odb::dbSet<odb::dbCapNode> capSet = net->getCapNodes();

  odb::dbSet<odb::dbCapNode>::iterator cap_itr;

  for (cap_itr = capSet.begin(); cap_itr != capSet.end(); ++cap_itr) {
    odb::dbCapNode* capNode = *cap_itr;

    if (capNode->isITerm())
      writeITerm(capNode->getNode());
    else if (capNode->isName()) {
      ATH__fprintf(_outFP, "*I ");
      writeNameNode(capNode);
      ATH__fprintf(_outFP, "\n");
    }
    cnt++;
  }
  return cnt;
}
uint extSpef::writeCapITerms(odb::dbNet* net)
{
  odb::dbSet<odb::dbCapNode> capSet = net->getCapNodes();
  odb::dbSet<odb::dbCapNode>::iterator cap_itr;
  for (cap_itr = capSet.begin(); cap_itr != capSet.end(); ++cap_itr) {
    odb::dbCapNode* capNode = *cap_itr;

    if (capNode->isITerm())
      writeCapITerm(capNode->getNode(), capNode->getSortIndex());
    else if (capNode->isName())  // coming from lower level
      writeCapName(capNode, capNode->getSortIndex());
  }
  return _cCnt;
}
uint extSpef::writeNodeCaps(odb::dbNet* net, uint netId)
{
  if (netId == 0)
    netId = net->getId();

  odb::dbSet<odb::dbCapNode> capSet = net->getCapNodes();
  odb::dbSet<odb::dbCapNode>::iterator cap_itr;
  for (cap_itr = capSet.begin(); cap_itr != capSet.end(); ++cap_itr) {
    odb::dbCapNode* capNode = *cap_itr;

    if (!capNode->isInternal())
      continue;

    /* 021810D BEGIN
    if (capNode->isITerm())
            continue;
    if (capNode->isBTerm())
            continue;
     021810D END */

    // uint capId= capNode->getId();
    uint capNodeId = capNode->getSortIndex();
    writeNodeCap(netId, capNodeId, capNode->getNode());
  }
  return _cCnt;
}
class compareCC
{
 public:
  bool operator()(odb::dbCCSeg* cc1, odb::dbCCSeg* cc2)
  {
    odb::dbBlock* block = cc1->getSourceNet()->getBlock();
    uint cn1, cn2;
    uint net1, net2;
    uint id1, id2;
    odb::dbCapNode *cp1, *cp2;
    int rc;
    /*
                    cn1 = cc1->getSourceNode();
                    cn2 = cc2->getSourceNode();
                    cp1 = odb::dbCapNode::getCapNode(block, cn1);
                    cp2 = odb::dbCapNode::getCapNode(block, cn2);
    */
    cp1 = cc1->getSourceCapNode();
    cp2 = cc2->getSourceCapNode();
    cn1 = cp1->getId();
    cn2 = cp2->getId();

    net1 = cp1->getNet()->getId();
    id1 = cp1->getNode();
    net2 = cp2->getNet()->getId();
    id2 = cp2->getNode();
    if (cp1->isBTerm() && cp2->isBTerm()) {
      rc = strcmp(odb::dbBTerm::getBTerm(block, id1)->getName().c_str(),
                  odb::dbBTerm::getBTerm(block, id2)->getName().c_str());
      if (rc != 0)
        return (rc < 0 ? true : false);
    }
    if (cp1->isBTerm() && !cp2->isBTerm())
      return true;
    if (!cp1->isBTerm() && cp2->isBTerm())
      return false;
    if (cp1->isITerm() && cp2->isITerm()) {
      if (id1 != id2)
        return (id1 < id2 ? true : false);
    }
    if (cp1->isITerm() && !cp2->isITerm())
      return true;
    if (!cp1->isITerm() && cp2->isITerm())
      return false;
    if (net1 != net2)
      return (net1 < net2 ? true : false);
    if (id1 != id2)
      return (id1 < id2 ? true : false);
    /*
                    cp1 = odb::dbCapNode::getCapNode(block, cn1);
                    cp2 = odb::dbCapNode::getCapNode(block, cn2);
                    cn1 = cc1->getTargetNode();
                    cn2 = cc2->getTargetNode();
    */
    cp1 = cc1->getTargetCapNode();
    cp2 = cc2->getTargetCapNode();
    cn1 = cp1->getId();
    cn2 = cp2->getId();

    net1 = cp1->getNet()->getId();
    id1 = cp1->getNode();
    net2 = cp2->getNet()->getId();
    id2 = cp2->getNode();
    if (cp1->isBTerm() && cp2->isBTerm()) {
      rc = strcmp(odb::dbBTerm::getBTerm(block, id1)->getName().c_str(),
                  odb::dbBTerm::getBTerm(block, id2)->getName().c_str());
      if (rc != 0)
        return (rc < 0 ? true : false);
    }
    if (cp1->isBTerm() && !cp2->isBTerm())
      return true;
    if (!cp1->isBTerm() && cp2->isBTerm())
      return false;
    if (cp1->isITerm() && cp2->isITerm()) {
      if (id1 != id2)
        return (id1 < id2 ? true : false);
    }
    if (cp1->isITerm() && !cp2->isITerm())
      return true;
    if (!cp1->isITerm() && cp2->isITerm())
      return false;
    if (net1 != net2)
      return (net1 < net2 ? true : false);
    return (id1 < id2 ? true : false);
  }
};
uint extSpef::writeCouplingCapsNoSort(odb::dbSet<odb::dbCCSeg>& capSet,
                                      uint netId)
{
  std::vector<odb::dbCCSeg*> vec_cc;
  odb::dbSet<odb::dbCCSeg>::iterator cap_itr;
  odb::dbCCSeg* cc = NULL;
  for (cap_itr = capSet.begin(); cap_itr != capSet.end(); ++cap_itr) {
    cc = *cap_itr;

    writeCNodeNumber();

    writeCapNode(cc->getSourceCapNode()->getId(), netId);
    writeCapNode(cc->getTargetCapNode()->getId(), netId);

    ATH__fprintf(
        _outFP, "%g", cc->getCapacitance(_active_corner_number[0]) * _cap_unit);
    for (int ii = 1; ii < _active_corner_cnt; ii++)
      ATH__fprintf(_outFP,
                   "%s%g",
                   _delimiter,
                   cc->getCapacitance(_active_corner_number[ii]) * _cap_unit);

    ATH__fprintf(_outFP, "\n");
  }
  return _cCnt;
}
uint extSpef::writeCouplingCaps(odb::dbSet<odb::dbCCSeg>& capSet, uint netId)
{
  if (_preserveCapValues)
    return writeCouplingCapsNoSort(capSet, netId);

  std::vector<odb::dbCCSeg*> vec_cc;
  odb::dbSet<odb::dbCCSeg>::iterator cap_itr;
  odb::dbCCSeg* cc = NULL;
  for (cap_itr = capSet.begin(); cap_itr != capSet.end(); ++cap_itr) {
    cc = *cap_itr;
    vec_cc.push_back(cc);
  }
  std::sort(vec_cc.begin(), vec_cc.end(), compareCC());
  uint j;
  for (j = 0; j < vec_cc.size(); j++) {
    cc = vec_cc[j];

    writeCNodeNumber();

    writeCapNode(cc->getSourceCapNode()->getId(), netId);
    writeCapNode(cc->getTargetCapNode()->getId(), netId);

    ATH__fprintf(
        _outFP, "%g", cc->getCapacitance(_active_corner_number[0]) * _cap_unit);
    for (int ii = 1; ii < _active_corner_cnt; ii++)
      ATH__fprintf(_outFP,
                   "%s%g",
                   _delimiter,
                   cc->getCapacitance(_active_corner_number[ii]) * _cap_unit);

    ATH__fprintf(_outFP, "\n");
  }
  return _cCnt;
}
uint extSpef::writeTgtCouplingCaps(odb::dbNet* net, uint netId)
{
  std::vector<odb::dbCCSeg*> vec_cc;
  net->getTgtCCSegs(vec_cc);

  if (!_preserveCapValues)
    std::sort(vec_cc.begin(), vec_cc.end(), compareCC());

  return writeCouplingCaps(vec_cc, netId);
}
uint extSpef::writeSrcCouplingCaps(odb::dbNet* net, uint netId)
{
  std::vector<odb::dbCCSeg*> vec_cc;
  net->getSrcCCSegs(vec_cc);

  if (!_preserveCapValues)
    std::sort(vec_cc.begin(), vec_cc.end(), compareCC());

  return writeCouplingCaps(vec_cc, netId);
}
uint extSpef::writeCouplingCaps(std::vector<odb::dbCCSeg*>& vec_cc, uint netId)
{
  uint j;
  char msg1[2048];
  for (j = 0; j < vec_cc.size(); j++) {
    odb::dbCCSeg* cc = vec_cc[j];

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
      logger_->warn(RCX,
                    173,
                    "{} on the same net {} {}. Discard this CC.",
                    _bufString,
                    scnode->getNet()->getId(),
                    scnode->getNet()->getConstName());
      _bufString = NULL;
      continue;
    }
    writeCapNode(cc->getSourceCapNode(), netId);
    writeCapNode(cc->getTargetCapNode(), netId);

    ATH__fprintf(
        _outFP, "%g", cc->getCapacitance(_active_corner_number[0]) * _cap_unit);
    for (int ii = 1; ii < _active_corner_cnt; ii++)
      ATH__fprintf(_outFP,
                   "%s%g",
                   _delimiter,
                   cc->getCapacitance(_active_corner_number[ii]) * _cap_unit);

    ATH__fprintf(_outFP, "\n");
  }
  return _cCnt;
}
void extSpef::getAnchorCoords(odb::dbNet* net,
                              uint shapeId,
                              int* x1,
                              int* y1,
                              int* x2,
                              int* y2,
                              odb::dbTechLayer** layer)
{
  odb::dbShape s;
  odb::dbWire* w = net->getWire();
  w->getShape(shapeId, s);
  *x1 = s.xMin();
  *y1 = s.yMin();
  *x2 = s.xMax();
  *y2 = s.yMax();
  odb::dbTechVia* tv = s.getTechVia();
  odb::dbVia* via = s.getVia();
  if (tv)
    *layer = tv->getBottomLayer();
  else if (via)
    *layer = via->getBottomLayer();
  else
    *layer = s.getTechLayer();
}
uint extSpef::writeNodeCoords(uint netId, odb::dbSet<odb::dbRSeg>& rSet)
{
  // double db2nm= 0.001; // TODO: make it defunits driven
  int dbunit = _block->getDbUnitsPerMicron();
  double db2nm = 1.0 / ((double) dbunit);
  // rSet.reverse();

  //*N *1:2 *C 3.06500 125.815
  //*N *1:3 *C 3.03000 120.555
  //*N *2:4 *C 3.07000 120.190
  //*N *2:5 *C 3.07000 120.190

  odb::dbSet<odb::dbRSeg>::iterator rc_itr;

  //	odb::dbNet *net= odb::dbNet::getNet(_block, netId);

  uint cnt = 1;

  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    odb::dbRSeg* rc = *rc_itr;

    uint shapeId = rc->getShapeId();
    if (!_foreign && shapeId == 0)
      continue;

    odb::dbCapNode* capNode
        = odb::dbCapNode::getCapNode(_block, rc->getTargetNode());

    if (capNode->isITerm() || capNode->isBTerm())
      continue;

    // int x1, y1, x2, y2;
    // odb::dbTechLayer *layer;
    // getAnchorCoords(net, shapeId, &x1, &y1, &x2, &y2, &layer);
    //
    // ATH__fprintf(_outFP, "*N ");
    // writeCapNode(rc->getTargetNode());

    // if (_NsLayer)
    //	ATH__fprintf(_outFP, "*C %f %f %s\n", db2nm*(x1+x2)/2, db2nm*(y1+y2)/2,
    // layer->getConstName()); else 	ATH__fprintf(_outFP, "*C %f %f\n",
    // db2nm*(x1+x2)/2, db2nm*(y1+y2)/2);
    ATH__fprintf(_outFP, "*N ");
    writeCapNode(rc->getTargetNode(), netId);
    int x1, y1;

    // if (_foreign)
    //	rc->getCoords(x1, y1);
    // else
    //	net->getWire() -> getCoord( shapeId, x1, y1);
    rc->getCoords(x1, y1);

    ATH__fprintf(_outFP, "*C %f %f\n", db2nm * x1, db2nm * y1);
  }
  return cnt;
}
bool extSpef::isCapNodeExcluded(odb::dbCapNode* node)
{
  if (node == NULL || node->getITerm(_block) == NULL)
    return false;
  if (node->getITerm(_block)->getInst()->getMaster()->isMarked())
    return true;
  return false;
}

uint extSpef::writeRes(uint netId, odb::dbSet<odb::dbRSeg>& rSet)
{
  // rSet.reverse();

  odb::dbSet<odb::dbRSeg>::iterator rc_itr;

  uint cnt = 1;

  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    if (cnt == 1)
      writeKeyword("*RES");
    odb::dbRSeg* rc = *rc_itr;

    if (isCapNodeExcluded(rc->getSourceCapNode()))
      continue;
    if (isCapNodeExcluded(rc->getTargetCapNode()))
      continue;

    ATH__fprintf(_outFP, "%d ", cnt++);
    writeCapNode(rc->getSourceNode(), netId);
    writeCapNode(rc->getTargetNode(), netId);

    ATH__fprintf(
        _outFP, "%g", rc->getResistance(_active_corner_number[0]) * _res_unit);
    for (int ii = 1; ii < _active_corner_cnt; ii++)
      ATH__fprintf(_outFP,
                   "%s%g",
                   _delimiter,
                   rc->getResistance(_active_corner_number[ii]) * _res_unit);

    ATH__fprintf(_outFP, " \n");
  }
  return cnt;
}
void getjxy(odb::dbNet* net, uint jid, Logger* logger)
{
  int jx, jy;
  net->getWire()->getCoord((int) jid, jx, jy);
  logger->info(RCX, 174, "{}:{} {} {}", net->getId(), jid, jx, jy);
}
uint extSpef::writeNet(odb::dbNet* net, double resBound, uint debug)
{
  _d_net = net;
  uint netId = net->getId();

  if (_cornerBlock && _cornerBlock != _block)
    net = odb::dbNet::getNet(_cornerBlock, netId);

  uint minNode;
  uint capNodeCnt = getMinCapNode(net, &minNode);
  // if (capNodeCnt==0)
  //	return 1;
  if (capNodeCnt) {
    odb::dbSet<odb::dbRSeg> rcSet = net->getRSegs();
    //	resetTermTables();

    _cCnt = 1;

    double totCap[5];
    resetCap(totCap);

    /* dimitris_change_
    odb::dbSet<odb::dbCCSeg> srcCCcaps= net->getSrcCCSegs();
    addCouplingCaps(srcCCcaps, totCap);
    odb::dbSet<odb::dbCCSeg> tgtCCcaps= net->getTgtCCSegs();
    if (_symmetricCCcaps)
            addCouplingCaps(tgtCCcaps, totCap);
    */

    if (_symmetricCCcaps)
      addCouplingCaps(net, totCap);
    else
      logger_->warn(
          RCX, 175, "dimitris_change NEED TO IMPLEMENT NON SYMMTRIC CASE");

    //	odb::dbSet<odb::dbCapNode> capSet= net->getCapNodes();

    if (_preserveCapValues) {
      getCaps(net, totCap);
      writeDnet(netId, totCap);

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
        odb::dbSet<odb::dbCapNode> capSet = net->getCapNodes();

        writeCapPortsAndIterms(capSet, true);
        writeCapPortsAndIterms(capSet, false);
        writeInternalCaps(net, capSet);
      }
    } else {
      _firstCapNode = minNode - 1;

      reinitCapTable(_nodeCapTable, capNodeCnt + 2);

      if (_singleP)
        computeCapsAdd2Target(rcSet, totCap);
      else
        computeCaps(rcSet, totCap);

      writeDnet(netId, totCap);
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
        writeNodeCaps(net, netId);
      }
    }
    if (_wCap || _wOnlyCCcap)
      // writeCouplingCaps(srcCCcaps);
      writeSrcCouplingCaps(net);

    if (_symmetricCCcaps && (_wCap || _wOnlyCCcap))
      // writeCouplingCaps(tgtCCcaps);
      writeTgtCouplingCaps(net);

    if (_wRes) {
      // writeKeyword("*RES");
      writeRes(netId, rcSet);
    }
    writeKeyword("*END");
  }
  odb::dbSet<odb::dbCapNode> nodeSet = net->getCapNodes();
  odb::dbSet<odb::dbCapNode>::iterator rc_itr;
  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    odb::dbCapNode* node = *rc_itr;
    node->setSortIndex(0);
  }
  return 1;
}
bool extSpef::setInSpef(char* filename, bool onlyOpen)
{
  if (filename == NULL) {
    _inFP = stdin;
    return true;
  }

  strcpy(_inFile, filename);

  if (!onlyOpen) {
    _nodeParser = new Ath__parser();
    _parser = new Ath__parser();
  }
  _parser->openFile(filename);

  return true;
}
bool extSpef::setOutSpef(char* filename)
{
  if (filename == NULL) {
#ifdef _WIN32
    _outFP = stdout;
#endif
    return true;
  }

  strcpy(_outFile, filename);

#ifndef _WIN32
  if (_gzipFlag) {
    char cmd[2048];
    sprintf(cmd, "gzip -1 > %s.gz", filename);
    _outFP = popen(cmd, "w");
    /*
                    char cmd[2048];
                    sprintf(cmd, "%s.gz", filename);
                    _outFP= ATH__fopen(cmd, "w");
    */
  } else
#endif
    _outFP = ATH__fopen(filename, "w");

  if (_outFP == NULL) {
    fprintf(stderr, "Cannot open file %s with permissions \"w\"", filename);
    return false;
  }
  return true;
}
bool extSpef::closeOutFile()
{
  if (_outFP == NULL)
    return false;

#ifndef _WIN32
  if (_gzipFlag)
    ATH__fclose(_outFP);
  // pclose(_outFP);
  else
#endif
    ATH__fclose(_outFP);

  return true;
}
uint extSpef::writeBlockPorts()
{
  if (_partial && !_btermFound)
    return 0;
  writeKeyword("\n*PORTS");

  uint cnt = 0;
  odb::dbSet<odb::dbBTerm> bterms = _block->getBTerms();
  odb::dbSet<odb::dbBTerm>::iterator itr;
  for (itr = bterms.begin(); itr != bterms.end(); ++itr) {
    odb::dbBTerm* bterm = *itr;
    if (bterm->getSigType() == odb::dbSigType::POWER)
      continue;
    if (bterm->getSigType() == odb::dbSigType::GROUND)
      continue;
    if (_partial && !bterm->isSetMark())
      continue;
    bterm->setMark(0);

    uint id = bterm->getId();

    if (_useIds) {
      ATH__fprintf(_outFP, "B%d %c\n", id, bterm->getIoType().getString()[0]);
    } else {
      ATH__fprintf(_outFP,
                   "%s %c\n",
                   addEscChar(bterm->getName().c_str(), false),
                   bterm->getIoType().getString()[0]);
    }
  }
  return cnt;
}
uint extSpef::getNetMapId(uint netId)
{
  _baseNameMap = MAX(_baseNameMap, netId);

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
char* extSpef::addEscChar(const char* iname, bool esc_bus_brkts) {
  uint ii = 0;
  uint jj = 0;
  while (iname[ii] != '\0') {
    char ch = iname[ii];
    if (!std::isalnum(ch)
        && ch != '_'
        && ch != '\\'
        && (esc_bus_brkts || (ch != '[' && ch != ']'))
        // Check if there is an escape char before
        // the non-alphanumeric character
        && (ii == 0 || iname[ii-1] != '\\'))
      _mMapName[jj++] = '\\';
    _mMapName[jj++] = iname[ii++];
  }
  
  _mMapName[jj] = '\0';
  return _mMapName;
}
char* extSpef::tinkerSpefName(char* iname)
{
  if (!_noBackSlash)
    return iname;
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

uint extSpef::writeNetMap(odb::dbSet<odb::dbNet>& nets)
{
  uint cnt = 0;
  char *nname, *nname1;
  odb::dbSet<odb::dbNet>::iterator net_itr;
  _btermFound = false;
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    odb::dbNet* net = *net_itr;

    odb::dbSigType type = net->getSigType();
    if ((type == odb::dbSigType::POWER) || (type == odb::dbSigType::GROUND))
      continue;
    if (!_partial || !net->isMarked())
      continue;
    net->setMark_1(true);
    odb::dbSet<odb::dbITerm> iterms = net->getITerms();
    odb::dbSet<odb::dbITerm>::iterator iterm_itr;
    for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr)
      (*iterm_itr)->getInst()->setUserFlag1();
    odb::dbSet<odb::dbBTerm> bterms = net->getBTerms();
    odb::dbSet<odb::dbBTerm>::iterator bterm_itr;
    for (bterm_itr = bterms.begin(); bterm_itr != bterms.end(); ++bterm_itr) {
      _btermFound = true;
      (*bterm_itr)->setMark(1);
    }
    odb::dbSet<odb::dbCapNode> capNodes = net->getCapNodes();
    odb::dbSet<odb::dbCapNode>::iterator citr;
    for (citr = capNodes.begin(); citr != capNodes.end(); ++citr) {
      odb::dbCapNode* capn = *citr;
      odb::dbSet<odb::dbCCSeg> ccSegs = capn->getCCSegs();
      odb::dbSet<odb::dbCCSeg>::iterator ccitr;
      for (ccitr = ccSegs.begin(); ccitr != ccSegs.end();) {
        odb::dbCCSeg* cc = *ccitr;
        ++ccitr;
        odb::dbCapNode* tcap = cc->getSourceCapNode();
        if (tcap == capn)
          tcap = cc->getTargetCapNode();
        if (tcap->isITerm())
          tcap->getITerm(_block)->getInst()->setUserFlag1();
        else if (tcap->isBTerm())
          tcap->getBTerm(_block)->setMark(1);
        else
          tcap->getNet()->setMark_1(true);
      }
    }
  }
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    odb::dbNet* net = *net_itr;

    odb::dbSigType type = net->getSigType();
    if ((type == odb::dbSigType::POWER) || (type == odb::dbSigType::GROUND))
      continue;
    if (_partial && !net->isMark_1ed())
      continue;
    net->setMark_1(false);
    uint netMapId = getNetMapId(net->getId());

    if (_useIds)
      ATH__fprintf(_outFP, "*%d N%d\n", netMapId, netMapId);
    else {
      nname = (char*) net->getConstName();
      nname1 = tinkerSpefName(nname);
      nname1 = addEscChar(nname1, false);
      ATH__fprintf(_outFP, "*%d %s\n", netMapId, nname1);
    }

    cnt++;
  }
  return cnt;
}
uint extSpef::writeInstMap()
{
  uint cnt = 0;
  char *nname, *nname1;
  uint instMapId = 0;

  odb::dbSet<odb::dbInst> insts = _block->getInsts();
  odb::dbSet<odb::dbInst>::iterator inst_itr;

  for (inst_itr = insts.begin(); inst_itr != insts.end(); ++inst_itr) {
    odb::dbInst* inst = *inst_itr;

    // 021610D BEGIN for flat block won't make any difference!!!
    if (inst->getChild() != NULL)
      continue;
    // 021610D END

    if (inst->getMaster()->getMTermCount() <= 0)
      continue;
    if (inst->getMaster()->isMarked()) {
      logger_->info(RCX,
                    176,
                    "Skip instance {} for cell {} is excluded",
                    inst->getConstName(),
                    inst->getMaster()->getConstName());
      odb::verbose(0,
                   "Skip instance %s for cell %s is excluded\n",
                   inst->getConstName(),
                   inst->getMaster()->getConstName());
      continue;
    }
    if (_partial && !inst->getUserFlag1())
      continue;
    inst->clearUserFlag1();

    instMapId = getInstMapId(inst->getId());

    if (_useIds)
      ATH__fprintf(_outFP, "*%d I%d\n", instMapId, inst->getId());
    else {
      nname = (char*) inst->getConstName();
      nname1 = tinkerSpefName(nname);
      nname1 = addEscChar(nname1, true);
      ATH__fprintf(_outFP, "*%d %s\n", instMapId, nname1);
    }

    cnt++;
  }
  return instMapId;
}
uint extSpef::stopWrite()
{
  // TODO	for (uint j=0;j<tnets.size();j++)
  // TODO		tnets[j]->setMark(false);
  // logger_->info(RCX, 0, "{} nets finished", cnt);

  closeOutFile();
  return 0;
}
uint extSpef::writeBlock(char* nodeCoord,
                         const char* excludeCell,
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
                         bool flatten,
                         bool parallel)
{
  uint rtc = writeBlock(nodeCoord,
                        excludeCell,
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
                        flatten,
                        parallel);
  return rtc;
}

uint extSpef::writeBlock(char* nodeCoord,
                         const char* excludeCell,
                         const char* capUnit,
                         const char* resUnit,
                         bool stopAfterNameMap,
                         std::vector<odb::dbNet*> tnets,
                         bool wClock,
                         bool wConn,
                         bool wCap,
                         bool wOnlyCCcap,
                         bool wRes,
                         bool noCnum,
                         bool stopBeforeDnets,
                         bool noBackSlash,
                         bool flatten,
                         bool parallel)
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
  if (nodeCoord && strcmp(nodeCoord, "m") == 0) {
    _writingNodeCoords = C_MAGMA;  // Magma
    logger_->warn(RCX, 177, "\" -N m \" is not implemented.");
    return 0;
  } else if (nodeCoord && strcmp(nodeCoord, "s") == 0)
    _writingNodeCoords = C_STARRC;  // SynopsysStarRC
  else if (nodeCoord && strcmp(nodeCoord, "S") == 0) {
    _writingNodeCoords = C_STARRC;
    _NsLayer = false;
  } else if (nodeCoord && nodeCoord[0] != '\0') {
    logger_->warn(RCX, 178, "\" -N {} \" is unknown.", nodeCoord);
    return 0;
  }
  if (!wConn && !wCap && !wOnlyCCcap && !wRes)
    _wConn = _wCap = _wRes = true;

  _partial = tnets.size() ? true : false;
  uint j;
  for (j = 0; j < tnets.size(); j++)
    tnets[j]->setMark(true);

  odb::dbMaster* master;
  std::vector<odb::dbMaster*> excmaster;
  char* mname;
  int ii;
  if (excludeCell && excludeCell[0] != '\0') {
    Ath__parser* parser = new Ath__parser();
    parser->mkWords(excludeCell);
    for (ii = 0; ii < parser->getWordCnt(); ii++) {
      mname = parser->get(ii);
      master = _block->getDb()->findMaster(mname);
      if (!master) {
        logger_->warn(RCX, 179, "Can't find master {}", mname);
        continue;
      }
      excmaster.push_back(master);
      master->setMark(1);
    }
  }
  if (excmaster.size())
    logger_->info(
        RCX, 180, "{} cells are excluded from write_spef", excmaster.size());
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

    if (!_preserveCapValues)
      setupMappingForWrite();

    odb::dbSet<odb::dbNet> nets = _block->getNets();

    writeHeaderInfo(0);

    if (_writeNameMap) {
      writeKeyword("\n*NAME_MAP");
      writeNetMap(nets);
      uint lastNetId = _baseNameMap;
      uint lastId = writeInstMap();
      if (parallel && flatten) {
        _baseNameMap = lastId;

        writeHierNetNameMap();
        writeHierInstNameMap();

        // 022510D Ready for TopNets after spef merging
        _baseNameMap = lastNetId;  // Ready for TopNets after spef merging
                                   // 022510D END
      }
    }
    // Mattias, Nov 07 - avoid writing *PORTS for incremental SPEF
    else {
      _btermFound = false;
    }
    // END Mattias, Nov 07
  }

  if (stopAfterNameMap) {
    _stopAfterNameMap = true;
    for (ii = 0; ii < (int) excmaster.size(); ii++)
      ((odb::dbMaster*) excmaster[ii])->setMark(0);
    return 0;
  }
  uint repChunk = 100000;

  if (stopBeforeDnets) {
    _stopBeforeDnets = true;
    writeBlockPorts();
    for (ii = 0; ii < (int) excmaster.size(); ii++)
      ((odb::dbMaster*) excmaster[ii])->setMark(0);
    return 0;
  }
  if (!_stopBeforeDnets)
    writeBlockPorts();

  _cornerBlock = NULL;

  if (_independentExtCorners) {
    if (_active_corner_cnt != 1) {
      logger_->warn(RCX,
                    181,
                    "Can only write one corner at a time when write_spef for "
                    "independent_spef_corner.");
      return 0;
    }
    _cornersPerBlock = 1;
  } else
    _cornersPerBlock = _cornerCnt;

  if (_independentExtCorners && _active_corner_number[0]) {
    _cornerBlock = _useBaseCornerRc
                       ? _block
                       : _block->findExtCornerBlock(_active_corner_number[0]);
    if (_cornerBlock == NULL) {
      logger_->warn(RCX,
                    182,
                    "Can't find extBlock for corner {}.",
                    _active_corner_number[0]);
      return 0;
    }
    _active_corner_number[0] = 0;
  } else
    _cornerBlock = _block;

  odb::dbSet<odb::dbNet> nets = _block->getNets();
  odb::dbSet<odb::dbNet>::iterator net_itr;

  uint cnt = 0;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    odb::dbNet* net = *net_itr;

    if (tnets.size() && !net->isMarked()) {
      if (!_incrPlusCcNets || net->getCcCount() == 0)
        continue;
    }
    odb::dbSigType type = net->getSigType();
    if ((type == odb::dbSigType::POWER) || (type == odb::dbSigType::GROUND))
      continue;
    if (_wOnlyClock && type != odb::dbSigType::CLOCK)
      continue;

    cnt += writeNet(net, 0.0, 0);

    if (cnt % repChunk == 0)
      logger_->info(RCX, 47, "{} nets finished", cnt);
  }
  for (ii = 0; ii < (int) excmaster.size(); ii++)
    ((odb::dbMaster*) excmaster[ii])->setMark(0);
  for (j = 0; j < tnets.size(); j++)
    tnets[j]->setMark(false);
  logger_->info(RCX, 47, "{} nets finished", cnt);

  closeOutFile();

  return cnt;
}
uint extSpef::write_spef_nets(bool flatten, bool parallel)
{
  _childBlockNetBaseMap = 0;
  _childBlockInstBaseMap = 0;
  _cornerBlock = _block;
  _cornersPerBlock = _cornerCnt;

  logger_->info(RCX,
                0,
                "extSpef::write_spef_nets: _cornersPerBlock= {}",
                _cornersPerBlock);
  uint repChunk = 50000;
  odb::dbSet<odb::dbNet> nets = _block->getNets();
  odb::dbSet<odb::dbNet>::iterator net_itr;

  uint cnt = 0;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    odb::dbNet* net = *net_itr;

    /*
                    if (tnets.size() && !net->isMarked()) {
                            if (!_incrPlusCcNets || net->getCcCount() == 0)
                                    continue;
                    }
    */
    odb::dbSigType type = net->getSigType();
    if ((type == odb::dbSigType::POWER) || (type == odb::dbSigType::GROUND))
      continue;
    if (_wOnlyClock && type != odb::dbSigType::CLOCK)
      continue;

    odb::dbSet<odb::dbRSeg> rSet = net->getRSegs();
    rSet.reverse();
    // extMain::printRSegs(net);
    cnt += writeNet(net, 0.0, 0);

    if (cnt % repChunk == 0)
      logger_->info(RCX, 47, "{} nets finished", cnt);
  }
  /*
          for (ii = 0; ii < (int)excmaster.size(); ii++)
                  ((odb::dbMaster*)excmaster[ii])->setMark(0);
          for (j=0;j<tnets.size();j++)
                  tnets[j]->setMark(false);
  */
  logger_->info(RCX, 47, "{} nets finished", cnt);

  // closeOutFile();

  return cnt;
}
uint extSpef::getMappedBTermId(uint spefId)
{
  //	uint btermId= 0;
  odb::dbBTerm* bterm = NULL;
  if (_testParsing || _statsOnly)
    return 0;
  uint id = 0;
  if (_useIds) {
    id = getNameMapId(spefId);
    bterm = odb::dbBTerm::getBTerm(_block, id);
  } else {
    char* name = _nameMapTable->geti(spefId);
    bterm = _block->findBTerm(name);
  }
  return bterm->getId();
}
bool extSpef::writeHeaderInfo(uint debug)
{
  /*
          if (debug>0)
                  _outFP= stdout;
  */

  ATH__fprintf(_outFP, "*SPEF \"ieee 1481-1999\"\n");
  ATH__fprintf(_outFP, "*DESIGN \"%s\"\n", _design);

  ATH__fprintf(_outFP, "*DATE \"11:11:11 Fri 11 11, 1111\"\n");

  ATH__fprintf(_outFP, "*VENDOR \"OpenRCX\"\n");
  ATH__fprintf(_outFP, "*PROGRAM \"Parallel Extraction\"\n");
  ATH__fprintf(_outFP, "*VERSION \"1.0\"\n");
  ATH__fprintf(_outFP, "*DESIGN_FLOW \"NAME_SCOPE LOCAL\" \"PIN_CAP NONE\"\n");
  ATH__fprintf(_outFP, "*DIVIDER %s\n", _divider);
  ATH__fprintf(_outFP, "*DELIMITER %s\n", _delimiter);
  ATH__fprintf(_outFP, "*BUS_DELIMITER %s\n", _bus_delimiter);
  ATH__fprintf(_outFP, "*T_UNIT %d %s\n", _time_unit, _time_unit_word);
  ATH__fprintf(_outFP, "*C_UNIT %d %s\n", 1, _cap_unit_word);
  ATH__fprintf(_outFP, "*R_UNIT %d %s\n", 1, _res_unit_word);
  ATH__fprintf(_outFP, "*L_UNIT %d %s\n", _ind_unit, _ind_unit_word);

  return true;
}

}  // namespace rcx
