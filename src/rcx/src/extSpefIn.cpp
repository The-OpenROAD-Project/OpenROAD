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

#include "name.h"
#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "utl/Logger.h"
#include "wire.h"

namespace rcx {

using odb::dbBTerm;
using odb::dbCapNode;
using odb::dbCCSeg;
using odb::dbInst;
using odb::dbITerm;
using odb::dbNet;
using odb::dbRSeg;
using odb::dbSet;
using odb::dbShape;
using odb::dbSigType;
using odb::dbWire;
using odb::dbWirePath;
using odb::dbWirePathItr;
using odb::dbWirePathShape;
using utl::RCX;

dbInst* extSpef::getDbInst(const uint id)
{
  uint ii = 0;
  const char hierD = _block->getHierarchyDelimeter();
  const char* instName = _spefName;
  const char* iName;
  if (!_mMap && _divider[0] != hierD) {
    while (instName[ii] != '\0') {
      if (instName[ii] == _divider[0]) {
        _nDvdName[ii] = hierD;
      } else {
        _nDvdName[ii] = instName[ii];
      }
      ii++;
    }
    _nDvdName[ii] = '\0';
    iName = &_nDvdName[0];
  } else {
    iName = instName;
  }
  dbInst* inst = _block->findInst(iName);
  if (inst || !_mMap) {
    if (!inst && _notFoundInst->getDataId(iName, 1, 0) == 0) {
      _unmatchedSpefInst++;
      _notFoundInst->addNewName(iName, 1);
      logger_->warn(RCX, 452, "Spef instance {} not found in db.", instName);
    }
    return inst;
  }

  // bad orig:  "cnfgctrl|C148_2/C3_8_C5_3_C2"
  // ok map1 :  ""cnfgctrl/C148_2\/C3_8_C5_3_C2"
  //
  ii = 0;
  uint jj = 0;
  while (instName[ii] != '\0')  // m_map type 1
  {
    if (instName[ii] == '|') {
      _mMapName[jj] = '/';
    } else if (instName[ii] == '/' || instName[ii] == '['
               || instName[ii] == ']') {
      sprintf(&_mMapName[jj], "\\");
      jj++;
      _mMapName[jj] = instName[ii];
    } else {
      _mMapName[jj] = instName[ii];
    }
    ii++;
    jj++;
  }
  _mMapName[jj] = '\0';
  inst = _block->findInst(_mMapName);
  if (inst) {
    return inst;
  }

  // bad orig:  "cnfgctrl/instPktProcess/globalRegister/r_glbstate0/q_reg[0]"
  // bad map1:
  // "cnfgctrl\/instPktProcess\/globalRegister\/r_glbstate0\/q_reg\[0\]" ok
  // map2:  "cnfgctrl/instPktProcess/globalRegister/r_glbstate0/q_reg\[0\]"
  //
  ii = jj = 0;
  while (instName[ii] != '\0')  // m_map type 2
  {
    if (instName[ii] == '[' || instName[ii] == ']') {
      sprintf(&_mMapName[jj], "\\");
      jj++;
      _mMapName[jj] = instName[ii];
    } else {
      _mMapName[jj] = instName[ii];
    }
    ii++;
    jj++;
  }
  _mMapName[jj] = '\0';
  inst = _block->findInst(_mMapName);
  if (inst) {
    return inst;
  }
  ii = jj = 0;
  while (instName[ii] != '\0')  // m_map type 2
  {
    if (instName[ii] == '\\' && instName[ii + 1] == '[') {
      _mMapName[jj] = '[';
      ii++;
    } else if (instName[ii] == '\\' && instName[ii + 1] == ']') {
      _mMapName[jj] = ']';
      ii++;
    } else {
      _mMapName[jj] = instName[ii];
    }
    ii++;
    jj++;
  }
  _mMapName[jj] = '\0';
  inst = _block->findInst(_mMapName);
  if (!inst && _notFoundInst->getDataId(iName, 1, 0) == 0) {
    _unmatchedSpefInst++;
    _notFoundInst->addNewName(iName, 1);
    logger_->warn(RCX, 258, "Spef instance {} not found in db.", instName);
  }
  return inst;
}

uint extSpef::getITermId(const uint id, const char* name)
{
  if (_block == nullptr) {
    return id;
  }

  if (_testParsing) {
    return id;
  }

  dbInst* inst = getDbInst(id);
  if (!inst) {
    return 0;
  }
  inst->setUserFlag1();
  dbITerm* iterm = inst->findITerm(name);
  return iterm->getId();
}

uint extSpef::getBTermId(const char* name)
{
  if (_block == nullptr) {
    return 1;
  }

  dbBTerm* bterm = _block->findBTerm(name);
  if (!bterm) {
    logger_->error(RCX, 259, "Can't find bterm {} in db.", name);
  }
  return bterm->getId();
}

uint extSpef::getBTermId(const uint id)
{
  dbBTerm* bterm = dbBTerm::getBTerm(_block, id);
  return bterm->getId();
}

uint extSpef::getNodeCap(dbSet<dbRSeg>& rcSet,
                         const uint capNodeId,
                         double* totCap)
{
  resetCap(totCap);

  uint cnt = 0;
  double cap[10];
  for (dbRSeg* rc : rcSet) {
    if (!((rc->getTargetNode() == capNodeId)
          || (rc->getSourceNode() == capNodeId))) {
      continue;
    }

    rc->getCapTable(cap);

    addCap(cap, totCap, this->_cornerCnt);

    cnt++;
  }
  return cnt;
}

double extSpef::printDiff(dbNet* net,
                          const double dbCap,
                          const double refCap,
                          const char* ctype,
                          const int ii,
                          const int id)
{
  if (_calib) {
    return 0.0;
  }

  double diffCap = 100.0;
  if (refCap < 0.0) {
    fprintf(_diffLogFP, "Invalid ref %s for net ", ctype);
    net->printNetName(_diffLogFP);
    _diffLogCnt++;

    return 100.0;
  }
  if (refCap > 0.0) {
    diffCap *= ((dbCap - refCap) / refCap);
  } else if (dbCap == 0.0) {  // both dbCap and refCap are 0.0
    diffCap = 0.0;
  }

  if ((diffCap <= _upperThres) && (diffCap >= _lowerThres)) {
    return diffCap;
  }

  constexpr bool ext_stats = true;
  if (ext_stats) {
    const uint corner = ii;
    int wlen;
    uint via_cnt;
    double min_cap, max_cap, min_res, max_res, via_res;

    const uint wireCnt = _ext->getExtStats(net,
                                           corner,
                                           wlen,
                                           min_cap,
                                           max_cap,
                                           min_res,
                                           max_res,
                                           via_res,
                                           via_cnt);

    double boundsPercent;
    double boundsPercentRef;
    if (strcmp(ctype, "netRes") == 0) {
      const char* comp_db = comp_bounds(
          dbCap, min_res + via_res, max_res + via_res, boundsPercent);
      const char* comp_ref = comp_bounds(
          refCap, min_res + via_res, max_res + via_res, boundsPercentRef);

      fprintf(_diffOutFP,
              "%4.1f  %10.4f D %s %4.1f ref %10.4f R %s %4.1f bounds: %10.4f "
              "%10.4f VR %g  V %d  L %d  WC %d  %s corner %d %s ",
              diffCap,
              dbCap,
              comp_db,
              boundsPercent,
              refCap,
              comp_ref,
              boundsPercentRef,
              min_res,
              max_res,
              via_res,
              via_cnt,
              wlen,
              wireCnt,
              ctype,
              ii,
              _ext->_tmpLenStats.c_str());
    } else {
      const char* comp_db = comp_bounds(dbCap, min_cap, max_cap, boundsPercent);
      const char* comp_ref
          = comp_bounds(refCap, min_cap, max_cap, boundsPercentRef);

      fprintf(_diffOutFP,
              "%4.1f  %10.4f D %s %4.1f ref %10.4f R %s %4.1f bounds: %10.4f "
              "%10.4f  L %8d  WC %3d  V %3d %s corner %d %s ",
              diffCap,
              dbCap,
              comp_db,
              boundsPercent,
              refCap,
              comp_ref,
              boundsPercentRef,
              min_cap,
              max_cap,
              wlen,
              wireCnt,
              via_cnt,
              ctype,
              ii,
              _ext->_tmpLenStats.c_str());
    }

  } else {
    fprintf(_diffOutFP,
            "%4.2f - %s %g ref %g corner %d ",
            diffCap,
            ctype,
            dbCap,
            refCap,
            ii);
  }

  if (id > 0) {
    fprintf(_diffOutFP, "capId %d ", id);
  }

  net->printNetName(_diffOutFP, true);

  return diffCap;
}

double extSpef::percentDiff(const double dbCap, const double refCap)
{
  double percent = 100.0;
  if (refCap > 0.0) {
    percent *= ((dbCap - refCap) / refCap);
  }
  return percent;
}

const char* extSpef::comp_bounds(const double val,
                                 const double min,
                                 const double max,
                                 double& percent)
{
  percent = percentDiff(val, max);
  double percent_low = percentDiff(val, min);
  const char* comp_db = "OK";
  if (val < min && percent_low < -0.1) {
    comp_db = "LO";
    percent = percent_low;
  } else if (val > max && percent > 0.1) {
    comp_db = "HI";
    percent = percentDiff(val, max);
  } else {
    // percent = 0;
  }
  return comp_db;
}

double extSpef::printDiffCC(dbNet* net1,
                            dbNet* net2,
                            const uint node1,
                            const uint node2,
                            const double dbCap,
                            const double refCap,
                            const char* ctype,
                            const int ii)
{
  if (_calib) {
    return 0.0;
  }

  if (!(refCap > 0.0)) {
    fprintf(
        _diffLogFP, "Invalid ref %s for nodeIds %d %d ", ctype, node1, node2);
    net1->printNetName(_diffLogFP, true, false);
    net2->printNetName(_diffLogFP);

    return 100.0;
  }

  double diffCap = 100.0;
  if (refCap > 0.0) {
    diffCap *= ((dbCap - refCap) / refCap);
  }

  if ((diffCap <= _upperThres) && (diffCap >= _lowerThres)) {
    return diffCap;
  }

  fprintf(_diffOutFP,
          "%4.1f - %s %g ref %g corner %d capNodes %d %d ",
          diffCap,
          ctype,
          dbCap,
          refCap,
          ii,
          node1,
          node2);

  net1->printNetName(_diffOutFP, true, false);
  net2->printNetName(_diffOutFP);

  return diffCap;
}

uint extSpef::getCapIdFromCapTable(const char* nodeWord)
{
  if (_cc_app_print_limit) {
    int nn;
    const uint id = _nodeHashTable->getDataId(nodeWord, 1, 0, &nn);
    if (id) {
      _ccidmap->set(id, nn);
    }
    return id;
  }
  return _nodeHashTable->getDataId(nodeWord, 1, 0);
}

void extSpef::addNewCapIdOnCapTable(const char* nodeWord, const uint capId)
{
  _nodeHashTable->addNewName(nodeWord, capId);
}

void extSpef::checkCCterm()
{
  dbNet* tnet1 = nullptr;
  dbNet* tnet2 = nullptr;
  if (_cciterm1 || _ccbterm1) {
    tnet1 = _cciterm1 ? _cciterm1->getNet() : _ccbterm1->getNet();
  }
  if (_cciterm2 || _ccbterm2) {
    tnet2 = _cciterm2 ? _cciterm2->getNet() : _ccbterm2->getNet();
  }
  if (!tnet1 || !tnet2 || tnet1 == _d_net || tnet2 == _d_net) {
    return;
  }
  char tn1[256];
  char tn2[256];
  if (_cciterm1) {
    sprintf(tn1,
            "Iterm %s/%s",
            _cciterm1->getInst()->getConstName(),
            _cciterm1->getMTerm()->getConstName());
  } else {
    sprintf(tn1, "Bterm %s", _ccbterm1->getConstName());
  }
  if (_cciterm2) {
    sprintf(tn2,
            "Iterm %s/%s",
            _cciterm2->getInst()->getConstName(),
            _cciterm2->getMTerm()->getConstName());
  } else {
    sprintf(tn2, "Bterm %s", _ccbterm2->getConstName());
  }
  logger_->error(
      RCX,
      260,
      "{} and {} are connected to a coupling cap of net {} {} in spef, but "
      "connected to net {} {} and net {} {} respectively in db.",
      tn1,
      tn2,
      _d_net->getId(),
      _d_net->getConstName(),
      tnet1->getId(),
      tnet1->getConstName(),
      tnet2->getId(),
      tnet2->getConstName());
}

uint extSpef::getCapNodeId(const char* nodeWord,
                           const char* capWord,
                           uint* netId)
{
  dbCapNode* cap = nullptr;
  uint capId = 0;
  const uint cccap = *netId;
  if (cccap == 1) {
    _cciterm1 = nullptr;
    _cciterm2 = nullptr;
    _ccbterm1 = nullptr;
    _ccbterm2 = nullptr;
  }

  if (_testParsing) {
    capId = getCapIdFromCapTable(nodeWord);
    if (capId == 0) {
      capId = _tmpCapId++;
      addNewCapIdOnCapTable(nodeWord, capId);
    }
  }
  const uint tokenCnt = _nodeParser->mkWords(nodeWord);

  dbNet* net = nullptr;
  dbNet* cornerNet = nullptr;
  if (tokenCnt == 2)  // iterm or internal node
  {
    uint id1;
    if (_maxMapId) {
      id1 = _nodeParser->getInt(0, 1);
      _spefName = _nameMapTable->geti(id1);
    } else {
      id1 = 0;
      _spefName = _nodeParser->get(0);
    }

    if (_nodeParser->isDigit(1, 0)) {  // internal node
      net = getDbNet(netId, id1);
      if (_cornerBlock != _block) {
        cornerNet = dbNet::getNet(_cornerBlock, net->getId());
      } else {
        cornerNet = net;
      }

      uint nodeId = _nodeParser->getInt(1, 0);

      if (!_testParsing && !_diff) {
        capId = getCapIdFromCapTable(nodeWord);

        if (capId > 0) {
          cap = dbCapNode::getCapNode(_cornerBlock, capId);
        } else {
          cap = dbCapNode::create(cornerNet, 0, true);  // "foreign" mode
          capId = cap->getId();
          addNewCapIdOnCapTable(nodeWord, capId);
          cap->setNode(nodeId);
          cap->setInternalFlag();
        }
      }
    } else {  // iterm
      char* termName = _nodeParser->get(1);
      uint termId = getITermId(id1, termName);
      if (!termId) {
        return 0;
      }

      dbITerm* iterm = nullptr;
      if (_block != nullptr) {
        iterm = dbITerm::getITerm(_block, termId);
        net = iterm->getNet();
        if (net != nullptr) {
          *netId = net->getId();
        }
        if (_cornerBlock != _block) {
          cornerNet = dbNet::getNet(_cornerBlock, net->getId());
        } else {
          cornerNet = net;
        }
        if (!cccap && *netId != _d_net->getId()) {
          logger_->error(
              RCX,
              262,
              "Iterm {}/{} is connected to net {} {} in spef, but connected "
              "to net {} {} in db.",
              iterm->getInst()->getConstName(),
              iterm->getMTerm()->getConstName(),
              _d_net->getId(),
              _d_net->getConstName(),
              net->getId(),
              net->getConstName());
        }
        if (cccap == 1) {
          _cciterm1 = iterm;
        }
        if (cccap == 2) {
          _cciterm2 = iterm;
        }
      }
      if (!_testParsing) {
        capId = iterm->getExtId();
        if ((capId == 0) && !_diff) {
          cap = dbCapNode::create(cornerNet, 0, true);  // "foreign" mode
          capId = cap->getId();
          iterm->setExtId(capId);
          cap->setNode(termId);
          cap->setITermFlag();
        } else if (capId == 0) {
          cap = nullptr;
          logger_->warn(RCX, 76, "Cap Node {} not extracted", nodeWord);
        } else {
          cap = dbCapNode::getCapNode(_cornerBlock, capId);
          if (_ccidmap) {
            _ccidmap->set(capId, id1);
          }
        }
      }
    }
  } else {  // Port
    uint btermId = 0;
    if (_nodeParser->get(0)[0] == '*') {  // mapped port
      const uint id = _nodeParser->getInt(0, 1);
      btermId = getMappedBTermId(id);
    } else {
      btermId = getBTermId(_nodeParser->get(0));
    }
    if (!_testParsing) {
      dbBTerm* bterm = dbBTerm::getBTerm(_block, btermId);
      net = bterm->getNet();
      if (_cornerBlock != _block) {
        cornerNet = dbNet::getNet(_cornerBlock, net->getId());
      } else {
        cornerNet = net;
      }
      *netId = net->getId();
      if (!cccap && *netId != _d_net->getId()) {
        logger_->error(
            RCX,
            263,
            "Bterm {} is connected to net {} {} in spef, but connected to "
            "net {} {} in db.",
            bterm->getConstName(),
            _d_net->getId(),
            _d_net->getConstName(),
            net->getId(),
            net->getConstName());
      }
      if (cccap == 1) {
        _ccbterm1 = bterm;
      }
      if (cccap == 2) {
        _ccbterm2 = bterm;
      }

      capId = bterm->getExtId();
      if ((capId == 0) && !_diff) {
        cap = dbCapNode::create(cornerNet, 0, true);  // "foreign" mode
        capId = cap->getId();
        bterm->setExtId(capId);
        cap->setBTermFlag();
        cap->setNode(btermId);
      } else if (capId == 0) {
        cap = nullptr;
        logger_->warn(RCX, 261, "Cap Node {} not extracted", nodeWord);
      } else {
        cap = dbCapNode::getCapNode(_cornerBlock, capId);

        if (_ccidmap) {
          _ccidmap->set(capId, btermId);
        }
      }
    }
  }
  if (cccap == 2) {
    checkCCterm();
  }

  if (_testParsing) {
    return capId;
  }

  if (capWord != nullptr && _inputNet && _rCap) {
    const uint capCnt = _nodeParser->mkWords(capWord);
    if (_diff) {
      diffGndCap(cornerNet, capCnt, capId);
    } else {
      if (_readAllCorners) {
        for (uint ii = 0; ii < capCnt; ii++) {
          double capVal = _cap_unit * _nodeParser->getDouble(ii);
          if (_addRepeatedCapValue) {
            cap->addCapacitance(capVal, ii);
          } else {
            cap->setCapacitance(capVal, ii);
          }
        }
      } else {
        double capVal = _cap_unit * _nodeParser->getDouble(_in_spef_corner);
        if (_addRepeatedCapValue) {
          cap->addCapacitance(capVal, _db_ext_corner);
        } else {
          cap->setCapacitance(capVal, _db_ext_corner);
        }
      }
    }
  }

  if (_calib && capId == 0) {
    capId = 1;  //  such that readDNet won't return 0 and call endNet
  }
  return capId;
}

void extSpef::resetExtIds(uint rit)
{
  for (dbBTerm* bterm : _block->getBTerms()) {
    bterm->setExtId(0);
  }
  if (!rit) {
    return;
  }

  for (dbITerm* iterm : _block->getITerms()) {
    iterm->setExtId(0);
  }
}

void extSpef::setExtIds()
{
  if (_testParsing || _statsOnly) {
    return;
  }

  for (dbNet* net : _cornerBlock->getNets()) {
    net->setTermExtIds(1);
  }
}

void extSpef::setSpefFlag(const bool v)
{
  if (_testParsing || _statsOnly) {
    return;
  }

  for (dbNet* net : _cornerBlock->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }
    net->setSpef(v);
  }
}

dbNet* extSpef::getDbNet(uint* id, const uint spefId)
{
  *id = 0;
  if (_testParsing || _statsOnly) {
    return nullptr;
  }

  const char hierD = _block->getHierarchyDelimeter();
  const char* netName = _spefName;
  const char* nName;
  if (!_mMap && _divider[0] != hierD) {
    uint ii = 0;
    while (netName[ii] != '\0') {
      if (netName[ii] == _divider[0]) {
        _nDvdName[ii] = hierD;
      } else {
        _nDvdName[ii] = netName[ii];
      }
      ii++;
    }
    _nDvdName[ii] = '\0';
    nName = &_nDvdName[0];
  } else {
    nName = netName;
  }
  dbNet* net;
  net = _block->findNet(nName);
  if (!_mMap || net) {
    if (!net) {
      logger_->warn(RCX, 77, "Spef net {} not found in db.", netName);
      _unmatchedSpefNet++;
    } else {
      *id = net->getId();
    }
    return net;
  }

  // bad orig:  "cnfgctrl|CClockPh1"
  // ok map11:  "cnfgctrl/CClockPh1"
  //
  uint ii = 0;
  uint jj = 0;
  uint lcnt = 0;
  while (netName[ii] != '\0')  // m_map type 1 v 1
  {
    if (netName[ii] == '|') {
      lcnt++;
      _mMapName[jj] = '/';
    } else if (lcnt
               && (netName[ii] == '/' || netName[ii] == '['
                   || netName[ii] == ']')) {
      sprintf(&_mMapName[jj], "\\");
      jj++;
      _mMapName[jj] = netName[ii];
    } else {
      _mMapName[jj] = netName[ii];
    }
    ii++;
    jj++;
  }
  _mMapName[jj] = '\0';
  net = _block->findNet(_mMapName);
  if (net) {
    *id = net->getId();
    return net;
  }

  // when runcnt==0, add \ before / after detecting |
  // when runcnt==1, add \ before / anyhow
  //
  int runcnt = 0;
  while (runcnt <= 1) {
    // bad orig:  "cnfgctrl|seamShiftOut1P[55]"
    // bad map11: "cnfgctrl/seamShiftOut1P\[55\]"
    // ok map12:  "cnfgctrl/seamShiftOut1P[55]"
    //
    ii = 0;
    jj = 0;
    lcnt = runcnt;
    while (netName[ii] != '\0')  // m_map type 1 v 2
    {
      if (netName[ii] == '|') {
        lcnt++;
        _mMapName[jj] = '/';
      }
      // else if (lcnt && (netName[ii] == '/' || netName[ii] == '[' ||
      // netName[ii] == ']'))
      else if (lcnt && netName[ii] == '/') {
        sprintf(&_mMapName[jj], "\\");
        jj++;
        _mMapName[jj] = netName[ii];
      } else {
        _mMapName[jj] = netName[ii];
      }
      ii++;
      jj++;
    }
    _mMapName[jj] = '\0';
    net = _block->findNet(_mMapName);
    if (net) {
      *id = net->getId();
      return net;
    }

    // bad orig:  "cnfgctrl/pwin_3[31]"
    // bad map11:  "cnfgctrl/pwin_3[31]"
    // bad map12:  "cnfgctrl/pwin_3[31]"
    // ok  map2:  "cnfgctrl/pwin_3\[31\]"
    //
    ii = jj = 0;
    while (netName[ii] != '\0')  // m_map type 2
    {
      if (netName[ii] == '[' || netName[ii] == ']') {
        sprintf(&_mMapName[jj], "\\");
        jj++;
        _mMapName[jj] = netName[ii];
      } else {
        _mMapName[jj] = netName[ii];
      }
      ii++;
      jj++;
    }
    _mMapName[jj] = '\0';
    net = _block->findNet(_mMapName);
    if (net) {
      *id = net->getId();
      return net;
    }
    // bad orig:  "cnfgctrl/pwin_3[31]"
    // ok  map3:  "cnfgctrl\/pwin_3\[31\]"
    //
    ii = jj = 0;
    while (netName[ii] != '\0')  // m_map type 3
    {
      if (netName[ii] == '/' || netName[ii] == '[' || netName[ii] == ']') {
        sprintf(&_mMapName[jj], "\\");
        jj++;
        _mMapName[jj] = netName[ii];
      } else {
        _mMapName[jj] = netName[ii];
      }
      ii++;
      jj++;
    }
    _mMapName[jj] = '\0';
    net = _block->findNet(_mMapName);
    if (net) {
      *id = net->getId();
      return net;
    }
    runcnt++;
  }
  if (net) {
    return net;
  }
  ii = jj = 0;
  while (netName[ii] != '\0')  // m_map type 2
  {
    if (netName[ii] == '\\' && netName[ii + 1] == '[') {
      _mMapName[jj] = '[';
      ii++;
    } else if (netName[ii] == '\\' && netName[ii + 1] == ']') {
      _mMapName[jj] = ']';
      ii++;
    } else {
      _mMapName[jj] = netName[ii];
    }
    ii++;
    jj++;
  }
  _mMapName[jj] = '\0';
  net = _block->findNet(_mMapName);
  if (!net) {
    logger_->warn(RCX, 264, "Spef net {} not found in db.", netName);
    _unmatchedSpefNet++;
    return net;
  }
  *id = net->getId();
  return net;
}

bool extSpef::isNetExcluded()
{
  if (!((_netSubWord == nullptr)
        || (strstr(_tmpNetName, _netSubWord) != nullptr))) {
    return true;
  }
  if ((_netExcludeSubWord != nullptr)
      && (strstr(_tmpNetName, _netExcludeSubWord) != nullptr)) {
    return true;
  }

  return false;
}

uint extSpef::diffNetCap(dbNet* net)
{
  if (isNetExcluded()) {
    return 0;
  }

  const uint capCnt = _nodeParser->mkWords(_parser->get(2));

  if (_readAllCorners) {
    for (uint ii = 0; ii < capCnt; ii++) {
      const double dbCap = net->getTotalCapacitance(ii, true);
      const double refCap = _cap_unit * _nodeParser->getDouble(ii);

      printDiff(net, dbCap, refCap, "netCap", ii);
    }
  } else {
    printDiff(net,
              net->getTotalCapacitance(_db_ext_corner, true),
              _cap_unit * _nodeParser->getDouble(_in_spef_corner),
              "netCap",
              _db_ext_corner);
  }

  return capCnt;
}

bool extSpef::computeFactor(const double db, const double ref, float& factor)
{
  factor = 1.0;

  if (db > 0.0) {
    factor = ref / db;
  }

  return factor != 1.0;
}

uint extSpef::matchNetRes(dbNet* net)
{
  if (_readAllCorners) {
    for (uint ii = 0; ii < _cornerCnt; ii++) {
      float factor;
      if (computeFactor(
              net->getTotalResistance(ii), _netResTable[ii], factor)) {
        net->adjustNetRes(factor, ii);
      }
    }
  } else {
    float factor;
    if (computeFactor(net->getTotalResistance(_db_ext_corner),
                      _netResTable[_in_spef_corner],
                      factor)) {
      net->adjustNetRes(factor, _db_ext_corner);
    }
  }
  return _cornerCnt;
}

uint extSpef::diffNetRes(dbNet* net)
{
  if (_readAllCorners) {
    for (uint ii = 0; ii < _cornerCnt; ii++) {
      const double dbRes = net->getTotalResistance(ii);
      const double refRes = _netResTable[ii];

      printDiff(net, dbRes, refRes, "netRes", ii);
    }
  } else {
    printDiff(net,
              net->getTotalResistance(_db_ext_corner),
              _netResTable[_in_spef_corner],
              "netRes",
              _db_ext_corner);
  }
  return _cornerCnt;
}

bool extSpef::matchNetGndCap(dbNet* net,
                             const uint dbCorner,
                             const double dbCap,
                             const double refCap)
{
  float factor = 1.0;

  if (dbCap > 0.0) {
    factor = refCap / dbCap;
  }

  if (factor != 1.0) {
    net->adjustNetGndCap(dbCorner, factor);
    return true;
  }
  return false;
}

bool extSpef::calibrateNetGndCap(dbNet* net,
                                 const uint dbCorner,
                                 const double dbCap,
                                 const double refCap)
{
  float factor = 1.0;

  if (dbCap > 0.0) {
    factor = refCap / dbCap;
  }

  if (factor != 1.0) {
    net->adjustNetGndCap(dbCorner, factor);
    return true;
  }
  return false;
}

uint extSpef::diffNetGndCap(dbNet* net)
{
  if (_match) {
    if (_readAllCorners) {
      for (uint ii = 0; ii < _cornerCnt; ii++) {
        matchNetGndCap(
            net, ii, net->getTotalCapacitance(ii), _netGndCapTable[ii]);
      }
    } else {
      matchNetGndCap(net,
                     _db_ext_corner,
                     net->getTotalCapacitance(_db_ext_corner),
                     _netGndCapTable[_in_spef_corner]);
    }
    return 0;
  }
  if (_calib) {
    if (_readAllCorners) {
      for (uint ii = 0; ii < _cornerCnt; ii++) {
        calibrateNetGndCap(
            net, ii, net->getTotalCapacitance(ii), _netGndCapTable[ii]);
      }
    } else {
      calibrateNetGndCap(net,
                         _db_ext_corner,
                         net->getTotalCapacitance(_db_ext_corner),
                         _netGndCapTable[_in_spef_corner]);
    }
    return 0;
  }
  if (isNetExcluded()) {
    return 0;
  }

  if (_readAllCorners) {
    for (uint ii = 0; ii < _cornerCnt; ii++) {
      const double dbCap = net->getTotalCapacitance(ii);
      const double refCap = _netGndCapTable[ii];

      printDiff(net, dbCap, refCap, "netGndCap", ii);
    }
  } else {
    printDiff(net,
              net->getTotalCapacitance(_db_ext_corner),
              _netGndCapTable[_in_spef_corner],
              "netGndCap",
              _db_ext_corner);
  }
  return _cornerCnt;
}

void extSpef::collectDbCCap(dbNet* net)
{
  std::vector<dbCCSeg*> vec_cc;
  net->getSrcCCSegs(vec_cc);
  for (dbCCSeg* cc : vec_cc) {
    dbNet* otherNet = cc->getTargetCapNode()->getNet();
    if (!otherNet->isMark_1ed()) {
      logger_->info(
          RCX,
          265,
          "There is cc cap between net {} and net {} in db, but not in "
          "reference spef file",
          net->getId(),
          otherNet->getId());
      continue;
    }
    float ccap = 0.0;
    if (_readAllCorners) {
      for (uint ii = 0; ii < _cornerCnt; ii++) {
        ccap += cc->getCapacitance(ii);
      }
    } else {
      ccap = cc->getCapacitance(_db_ext_corner);
    }

    otherNet->addDbCc(ccap);
  }
}

void extSpef::matchCcValue(dbNet* net)
{
  std::vector<dbCCSeg*> vec_cc;
  net->getSrcCCSegs(vec_cc);
  for (dbCCSeg* cc : vec_cc) {
    dbNet* otherNet = cc->getTargetCapNode()->getNet();
    if (!otherNet->isMark_1ed()) {
      continue;
    }
    const float ratio = otherNet->getCcMatchRatio();
    if (ratio == 0 || ratio == 1.0) {
      continue;
    }

    if (_readAllCorners) {
      cc->adjustCapacitance(ratio);
    } else {
      cc->adjustCapacitance(ratio, _db_ext_corner);
    }
  }
}

uint extSpef::matchNetCcap(dbNet* net)
{
  if (_netV1.empty()) {
    return 0;
  }
  collectDbCCap(net);
  for (dbNet* otherNet : _netV1) {
    const float dbCC = otherNet->getDbCc();
    if (dbCC == 0.0) {
      logger_->info(
          RCX,
          266,
          "There is cc cap between net {} and net {} in reference spef "
          "file, but not in db",
          net->getId(),
          otherNet->getId());
      continue;
    }
    otherNet->setCcMatchRatio(otherNet->getRefCc() / dbCC);
  }
  matchCcValue(net);
  for (dbNet* otherNet : _netV1) {
    otherNet->setMark_1(false);
    otherNet->setCcCalibFactor(1.0);
    otherNet->setGndcCalibFactor(1.0);
  }
  return 0;
}

uint extSpef::diffNetCcap(dbNet* net)
{
  if (_calib) {
    float factor = 1.0;

    if (_readAllCorners) {
      if (net->getTotalCouplingCap(0) != 0.0 && _netCCapTable[0] != 0.0) {
        factor = _netCCapTable[0] / net->getTotalCouplingCap(0);
      }
    } else {
      computeFactor(net->getTotalCouplingCap(_db_ext_corner),
                    _netCCapTable[_in_spef_corner],
                    factor);
    }
    factor = factor > _upperCalibLimit ? _upperCalibLimit : factor;
    factor = factor < _lowerCalibLimit ? _lowerCalibLimit : factor;

    net->setCcCalibFactor(factor);
  }
  if (_readAllCorners) {
    for (uint ii = 0; ii < _cornerCnt; ii++) {
      const double dbCap = net->getTotalCouplingCap(ii);
      const double refCap = _netCCapTable[ii];

      printDiff(net, dbCap, refCap, "netCcap", ii);
    }
  } else {
    printDiff(net,
              net->getTotalCouplingCap(_db_ext_corner),
              _netCCapTable[_in_spef_corner],
              "netCcap",
              _db_ext_corner);
  }
  return _cornerCnt;
}

uint extSpef::collectRefCCap(dbNet* srcNet, dbNet* tgtNet, const uint capCnt)
{
  float refCap = 0.0;
  if (_readAllCorners) {
    for (uint i = 0; i < capCnt; i++) {
      refCap += _cap_unit * _nodeParser->getDouble(i);
    }
  } else {
    refCap += _cap_unit * _nodeParser->getDouble(_in_spef_corner);
  }
  dbNet* otherNet;
  if (_d_corner_net == srcNet) {
    otherNet = tgtNet;
  } else {
    otherNet = srcNet;
  }
  if (_d_corner_net->getId() > otherNet->getId()) {
    return 0;
  }
  if (!otherNet->isMark_1ed()) {
    otherNet->setMark_1(true);
    _netV1.push_back(otherNet);
    otherNet->setRefCc(0.0);
    otherNet->setDbCc(0.0);
  }
  const float tot = otherNet->getRefCc() + refCap;
  otherNet->setRefCc(tot);
  return 0;
}

uint extSpef::diffCCap(dbNet* srcNet,
                       const uint srcId,
                       dbNet* tgtNet,
                       const uint dstId,
                       const uint capCnt)
{
  for (uint i = 0; i < capCnt; i++) {
    double refCap = _cap_unit * _nodeParser->getDouble(i);
    _netCCapTable[i] += refCap;
  }
  if (_calib) {
    return capCnt;
  }
  if ((srcId == 0) || (dstId == 0)) {
    return capCnt;
  }

  dbCCSeg* ccap = dbCCSeg::findCC(dbCapNode::getCapNode(_cornerBlock, srcId),
                                  dbCapNode::getCapNode(_cornerBlock, dstId));
  if (ccap == nullptr) {
    fprintf(_diffLogFP,
            "cannot find in DB, CC: %s %s %s\n",
            _parser->get(1),
            _parser->get(2),
            _parser->get(3));

    return 0;
  }
  if (isNetExcluded()) {
    return 0;
  }

  if (_readAllCorners) {
    for (uint ii = 0; ii < capCnt; ii++) {
      const double refCap = _cap_unit * _nodeParser->getDouble(ii);
      const double dbCap = ccap->getCapacitance(ii);

      printDiffCC(srcNet, tgtNet, srcId, dstId, dbCap, refCap, "ccCap", ii);
    }
  } else {
    printDiffCC(srcNet,
                tgtNet,
                srcId,
                dstId,
                ccap->getCapacitance(_db_ext_corner),
                _cap_unit * _nodeParser->getDouble(_in_spef_corner),
                "ccCap",
                _db_ext_corner);
  }
  return capCnt;
}

uint extSpef::diffGndCap(dbNet* net, const uint capCnt, const uint capId)
{
  if (isNetExcluded()) {
    return 0;
  }

  for (uint ii = 0; ii < capCnt; ii++) {
    double refCap = _cap_unit * _nodeParser->getDouble(ii);
    _netGndCapTable[ii] += refCap;
  }
  return 0;  // TODO - have to get cap from Rseg!

  if (capId <= 0) {
    return 0;
  }

  dbCapNode* cap = dbCapNode::getCapNode(_cornerBlock, capId);
  if (_readAllCorners) {
    for (uint ii = 0; ii < capCnt; ii++) {
      double dbCap = cap->getCapacitance(ii);

      if ((dbCap == 0.0) && (_netGndCapTable[ii] == 0.0)) {
        continue;
      }

      printDiff(net, dbCap, _netGndCapTable[ii], "gndCapNode", ii, capId);
    }
  } else {
    if (!((cap->getCapacitance(_db_ext_corner) == 0.0)
          && (_netGndCapTable[_in_spef_corner] == 0.0))) {
      printDiff(net,
                cap->getCapacitance(_db_ext_corner),
                _netGndCapTable[_in_spef_corner],
                "gndCap",
                _db_ext_corner,
                capId);
    }
  }
  return capCnt;
}

bool extSpef::getFirstShape(dbNet* net, dbShape& s)
{
  dbWirePath path;
  dbWirePathShape pshape;

  dbWirePathItr pitr;
  dbWire* wire = net->getWire();

  bool status = false;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    pitr.getNextShape(pshape);
    s = pshape.shape;
    status = true;
    break;
  }
  return status;
}

uint extSpef::getNetLW(dbNet* net, uint& w)
{
  dbShape s;
  if (!getFirstShape(net, s)) {
    logger_->info(RCX, 267, "{} has no shapes!", net->getName().c_str());
  }
  w = s.yMax() - s.yMin();
  const uint len = s.xMax() - s.xMin();

  if (w > len) {
    const uint l = w;
    w = len;
    return l;
  }
  return len;
}

bool extSpef::mkCapStats(dbNet* net)
{
  if (_capStatsFP == nullptr) {
    return false;
  }

  _nodeParser->mkWords(_tmpNetName, "/WS[");
  const double w = _nodeParser->getDouble(4);
  const double s = _nodeParser->getDouble(5);
  const char* mword = _nodeParser->get(3);
  const uint wcnt = _nodeParser->mkWords(mword, "ou");

  const uint m = _nodeParser->getInt(0, 1);
  const uint u = _nodeParser->getInt(1, 1);
  uint o = 0;
  if (wcnt == 3) {
    o = _nodeParser->getInt(2, 1);
    fprintf(_capStatsFP, "Metal %d OVER %d UNDER %d ", m, u, o);
  } else if (strstr(_tmpNetName, "uM") != nullptr) {
    o = _nodeParser->getInt(1, 1);
    fprintf(_capStatsFP, "Metal %d UNDER Metal %d ", m, o);
  } else {
    fprintf(_capStatsFP, "Metal %d OVER Metal %d ", m, u);
  }
  uint nW;
  const double len = 1.0 * (getNetLW(net, nW) - nW / 2);
  const double gndCap = _netGndCapTable[0] / 2;
  const double ccCap = _netCCapTable[0] / 2;
  const double cc1 = ccCap / len;
  const double gnd = gndCap / len;
  const double res = _netResTable[0] / 2;
  const double res1 = res / len;
  fprintf(_capStatsFP, "WIDTH %g SPACING %g %e %e %e\n", w, s, cc1, gnd, res1);

  return true;
}

uint extSpef::endNet(dbNet* net, const uint resCnt)
{
  if (_match
      && ((_netSubWord == nullptr)
          || (strstr(_tmpNetName, _netSubWord) != nullptr))) {
    if ((_netExcludeSubWord != nullptr)
        && (strstr(_tmpNetName, _netExcludeSubWord) != nullptr)) {
      return 0;
    }

    matchNetCcap(net);
    matchNetRes(net);
    return 0;
  }
  if (_diff
      && ((_netSubWord == nullptr)
          || (strstr(_tmpNetName, _netSubWord) != nullptr))) {
    if ((_netExcludeSubWord != nullptr)
        && (strstr(_tmpNetName, _netExcludeSubWord) != nullptr)) {
      return 0;
    }

    if (_calib) {
      matchNetRes(net);
    }
    mkCapStats(net);
    diffNetCcap(net);
    diffNetRes(net);
  }

  if (!_noCapNumCollapse) {
    net->collapseInternalCapNum(_capNodeFile);
  }

  if (!_testParsing && !_statsOnly) {
    net->setSpef(true);
  }

  if (!_keep_loaded_corner && !_statsOnly && !_testParsing && !_extracted) {
    net->getRSegs().reverse();
  }

  return resCnt;
}

uint extSpef::sortRSegs()
{
  uint srcCapn;
  uint tgtCapn;
  dbCapNode* drvCapNode = nullptr;
  _nrseg->resetCnt();
  uint capidx = 1;
  for (dbRSeg* rc : _d_corner_net->getRSegs()) {
    _nrseg->add(rc);

    srcCapn = rc->getSourceNode();
    dbCapNode* srcCapNode = dbCapNode::getCapNode(_cornerBlock, srcCapn);
    if (srcCapNode->isSourceTerm(_block)) {
      drvCapNode = srcCapNode;
    }
    if (srcCapNode->getSortIndex() == 0) {
      srcCapNode->setSortIndex(capidx++);
    }

    tgtCapn = rc->getTargetNode();
    dbCapNode* tgtCapNode = dbCapNode::getCapNode(_cornerBlock, tgtCapn);
    if (tgtCapNode->isSourceTerm(_block)) {
      drvCapNode = tgtCapNode;
    }
    if (tgtCapNode->getSortIndex() == 0) {
      tgtCapNode->setSortIndex(capidx++);
    }
  }
  if (drvCapNode == nullptr) {
    for (dbRSeg* rc : _d_corner_net->getRSegs()) {
      srcCapn = rc->getSourceNode();
      dbCapNode* srcCapNode = dbCapNode::getCapNode(_cornerBlock, srcCapn);
      if (srcCapNode->isInoutTerm(_block)) {
        drvCapNode = srcCapNode;
        break;
      }
      tgtCapn = rc->getTargetNode();
      dbCapNode* tgtCapNode = dbCapNode::getCapNode(_cornerBlock, tgtCapn);
      if (tgtCapNode->isInoutTerm(_block)) {
        drvCapNode = tgtCapNode;
        break;
      }
    }
    _d_corner_net->setRCDisconnected(true);
    return 0;
  }
  int ndx, ndy;
  bool zcfound = false;
  if (_readingNodeCoords == C_ON) {
    int ncidx = findNodeIndexFromNodeCoords(drvCapNode->getId());
    if (ncidx >= 0) {
      ndx = lround(_nodeCoordFactor * _xCoordTable->get(ncidx));
      ndy = lround(_nodeCoordFactor * _yCoordTable->get(ncidx));
      zcfound = true;
    }
  }
  if (!zcfound) {
    zcfound = drvCapNode->getTermCoords(ndx, ndy, _block);
  }
  if (!zcfound) {
    logger_->warn(RCX,
                  269,
                  "Cannot find coords of driver capNode {} of net {} {}",
                  tgtCapn,
                  _d_net->getId(),
                  _d_net->getConstName());
    _d_corner_net->setRCDisconnected(true);
    return 0;
  }
  _d_corner_net->getZeroRSeg()->setCoords(ndx, ndy);
  _d_corner_net->getZeroRSeg()->setTargetNode(drvCapNode->getId());
  const int cnn = _nrseg->getCnt() + 1;
  if (cnn > (int) _hcnrc->getSize()) {
    const int ocsz = _hcnrc->getSize();
    int ncsz = (cnn / 1024 + 1) * 1024;
    _hcnrc->resize(ncsz);
    ncsz = _hcnrc->getSize();
    for (int ii = ocsz; ii < ncsz; ii++) {
      Ath__array1D<int>* n1d = new Ath__array1D<int>(4);
      _hcnrc->set(ii, n1d);
    }
  }
  for (int ii = 0; ii < cnn; ii++) {
    _hcnrc->geti(ii)->resetCnt();
  }
  _rsegCnt = _nrseg->getCnt();
  for (int ii = 0; ii < (int) _rsegCnt; ii++) {
    dbRSeg* rc = _nrseg->get(ii);
    _hcnrc
        ->geti(dbCapNode::getCapNode(_cornerBlock, rc->getSourceNode())
                   ->getSortIndex()
               - 1)
        ->add(ii + 1);  // to use negetive index as visited
    _hcnrc
        ->geti(dbCapNode::getCapNode(_cornerBlock, rc->getTargetNode())
                   ->getSortIndex()
               - 1)
        ->add(ii + 1);
  }
  const int drvCapii = drvCapNode->getSortIndex() - 1;
  if (_hcnrc->geti(drvCapii)->getCnt() < 1) {
    logger_->warn(RCX,
                  270,
                  "Driving node of net {} {} is not connected to a rseg.",
                  _d_net->getId(),
                  _d_net->getConstName());
    _d_corner_net->setRCDisconnected(true);
    return 0;
  }
  Ath__array1D<int>* tcnrc;
  int hcnii;
  int cntTcn;
  int scns[2000];
  scns[0] = drvCapii;
  int ski = 1;
  int rnn, nridx, tridx;
  int pridx = 0;
  _srsegi->resetCnt();
  while (ski) {
    hcnii = scns[ski - 1];
    tcnrc = _hcnrc->geti(hcnii);
    cntTcn = tcnrc->getCnt();
    nridx = 0;
    for (int ii = 0; ii < cntTcn; ii++) {
      rnn = tcnrc->get(ii);
      if (rnn < 0) {
        continue;
      }
      if (rnn == pridx) {
        tcnrc->set(ii, -rnn);
        if (nridx) {
          break;
        }
        continue;
      }
      if (nridx == 0) {
        tcnrc->set(ii, -rnn);
        nridx = rnn;
        continue;
      }
    }
    pridx = nridx;
    if (nridx == 0) {
      ski--;
      continue;
    }
    tridx = nridx - 1;
    _srsegi->add(tridx);
    dbRSeg* rc = _nrseg->get(tridx);
    tgtCapn = rc->getTargetNode();
    srcCapn = rc->getSourceNode();
    int srcCapidx
        = dbCapNode::getCapNode(_cornerBlock, srcCapn)->getSortIndex() - 1;
    int tgtCapidx
        = dbCapNode::getCapNode(_cornerBlock, tgtCapn)->getSortIndex() - 1;
    if (srcCapidx == hcnii) {
      scns[ski++] = tgtCapidx;
    } else if (tgtCapidx == hcnii) {
      rc->setSourceNode(tgtCapn);
      rc->setTargetNode(srcCapn);
      tgtCapn = srcCapn;
      scns[ski++] = srcCapidx;
    } else {
      logger_->warn(RCX,
                    374,
                    "Inconsistency in RC of net {} {}.",
                    _d_net->getId(),
                    _d_net->getConstName());
      _d_corner_net->setRCDisconnected(true);
      return 0;
    }
    if (_readingNodeCoords == C_ON) {
      int ncidx = findNodeIndexFromNodeCoords(tgtCapn);
      if (ncidx >= 0) {
        const int x1 = lround(_nodeCoordFactor * _xCoordTable->get(ncidx));
        const int y1 = lround(_nodeCoordFactor * _yCoordTable->get(ncidx));
        rc->setCoords(x1, y1);
      } else {
        logger_->warn(
            RCX,
            271,
            "Cannot find node coords for targetCapNodeId {} of net {} {}",
            tgtCapn,
            _d_net->getId(),
            _d_net->getConstName());
      }
    }
  }
  if (_srsegi->getCnt() != _rsegCnt) {
    if (_diffLogFP) {
      fprintf(_diffLogFP,
              "RC of net %d %s is disconnected!\n",
              _d_net->getId(),
              _d_net->getConstName());
    } else {
      logger_->warn(RCX,
                    272,
                    "RC of net {} {} is disconnected!",
                    _d_net->getId(),
                    _d_net->getConstName());
    }
    _d_corner_net->setRCDisconnected(true);
    return 0;
  }
  _d_corner_net->setRCDisconnected(
      false);  // before read_spef, check_lib might have been done which called
               // makeRcModel and set almost all nets to be rc_disconnected
  for (int ii = 0; ii < (int) _rsegCnt; ii++) {
    dbRSeg* rc = _nrseg->get(ii);
    dbCapNode* capn = dbCapNode::getCapNode(_cornerBlock, rc->getSourceNode());
    uint capidx = capn->getSortIndex() - 1;
    capn->setChildrenCnt(_hcnrc->geti(capidx)->getCnt());
    if (_hcnrc->geti(capidx)->getCnt() > 2) {
      capn->setBranchFlag();
    }
    capn = dbCapNode::getCapNode(_cornerBlock, rc->getTargetNode());
    capidx = capn->getSortIndex() - 1;
    capn->setChildrenCnt(_hcnrc->geti(capidx)->getCnt());
    if (_hcnrc->geti(capidx)->getCnt() > 2) {
      capn->setBranchFlag();
    }
  }
  dbRSeg* trseg;
  dbRSeg* prseg = nullptr;
  dbRSeg* rseg1 = nullptr;
  dbRSeg* rseg2 = nullptr;
  dbRSeg* rsegb = nullptr;
  uint loopcnt = 0;
  uint hh;
  for (int ii = 0; ii < (int) _rsegCnt; ii++) {
    trseg = _nrseg->get(_srsegi->get(ii));
    dbCapNode* tcapn = trseg->getTargetCapNode();
    if (tcapn->isSelect()) {
      int jj;
      for (jj = ii; jj >= 0; jj--) {
        if (tcapn == _nrseg->get(_srsegi->get(jj))->getSourceCapNode()) {
          break;
        }
      }
      if (jj < 0) {
        logger_->error(RCX,
                       273,
                       "Failed to identify loop in net {} {}",
                       _d_net->getId(),
                       (char*) _d_net->getConstName());
      }
      hh = ii - jj + 1;
      logger_->warn(RCX,
                    274,
                    "{} capNodes loop in net {} {}",
                    hh,
                    _d_net->getId(),
                    (char*) _d_net->getConstName());
      for (; jj <= ii; jj++) {
        tcapn = _nrseg->get(_srsegi->get(jj))->getSourceCapNode();
        logger_->warn(RCX, 275, "    id={}", tcapn->getId());
        for (uint kk = 0; kk < _cornerCnt; kk++) {
          logger_->warn(RCX, 276, " cap-{}={}", kk, tcapn->getCapacitance(kk));
        }
      }
      if (loopcnt == 0) {
        if (jj == ii - 2)  // simple triangle, prepare to break it
        {
          rseg1 = _nrseg->get(_srsegi->get(ii - 2));
          rsegb = _nrseg->get(_srsegi->get(ii - 1));
          rseg2 = trseg;
        }
      }
      loopcnt++;
    }
    tcapn->setSelect(true);
    if (prseg) {
      prseg->setNext(trseg->getId());
    } else {
      _d_corner_net->getZeroRSeg()->setNext(trseg->getId());
      trseg->getSourceCapNode()->setSelect(true);
    }
    trseg->setNext(0);
    prseg = trseg;
  }
  for (int ii = 0; ii < (int) _rsegCnt; ii++) {
    trseg = _nrseg->get(_srsegi->get(ii));
    trseg->getSourceCapNode()->setSelect(false);
    trseg->getTargetCapNode()->setSelect(false);
  }
  double dres, o1res, o2res, d1res;
  uint srcnoden, tgtnoden;
  if (loopcnt == 0) {
    return 1;
  }
  if (_fixloop == 1 && loopcnt == 1 && rseg1)  // break one simple loop
  {
    _breakLoopNet++;
    if (_breakLoopNet <= 50) {
      logger_->warn(RCX,
                    406,
                    "Break one simple loop of {}-rsegs net {} {}",
                    _rsegCnt,
                    _d_net->getId(),
                    _d_net->getConstName());
    } else {
      logger_->warn(RCX,
                    277,
                    "Break one simple loop of {}-rsegs net {} {}",
                    _rsegCnt,
                    _d_net->getId(),
                    _d_net->getConstName());
    }
    for (int ii = 0; ii < (int) _cornerCnt; ii++) {
      dres = rsegb->getResistance(ii);
      o1res = rseg1->getResistance(ii);
      o2res = rseg2->getResistance(ii);
      d1res = dres * (o1res / (o1res + o2res));
      rseg1->setResistance(o1res + d1res, ii);
      rseg2->setResistance(o2res + dres - d1res, ii);
    }
    srcnoden = rseg2->getSourceNode();
    tgtnoden = rseg2->getTargetNode();
    rseg2->setSourceNode(tgtnoden);
    rseg2->setTargetNode(srcnoden);
    dbRSeg::destroy(rsegb);
    return 1;
  }
  _loopNet++;
  _d_corner_net->setRCDisconnected(true);
  if (loopcnt == 1 && rseg1 == nullptr) {
    _bigLoop++;
    if (_bigLoop <= 50) {
      logger_->warn(RCX,
                    278,
                    "{}-rsegs net {} {} has a {}-rsegs loop",
                    _rsegCnt,
                    _d_net->getId(),
                    _d_net->getConstName(),
                    hh);
    } else {
      logger_->warn(RCX,
                    407,
                    "{}-rsegs net {} {} has a {}-rsegs loop",
                    _rsegCnt,
                    _d_net->getId(),
                    _d_net->getConstName(),
                    hh);
    }
  } else {
    _multipleLoop++;
    if (_multipleLoop <= 50) {
      logger_->warn(RCX,
                    405,
                    "{}-rsegs net {} {} has {} loops",
                    _rsegCnt,
                    _d_net->getId(),
                    _d_net->getConstName(),
                    loopcnt);
    } else {
      logger_->warn(RCX,
                    279,
                    "{}-rsegs net {} {} has {} loops",
                    _rsegCnt,
                    _d_net->getId(),
                    _d_net->getConstName(),
                    loopcnt);
    }
  }
  return 0;
}

uint extSpef::readDNet(const uint debug)
{
  uint resCnt = 0;
  uint netId = 0;
  _netV1.clear();

  dbNet* srcNet = nullptr;
  dbNet* tgtNet;

  while (strcmp("*D_NET", _parser->get(0)) != 0) {
    if (_parser->parseNextLine() <= 0) {
      return 0;
    }
  }
  if (_capNodeFile) {
    _parser->printWords(_capNodeFile);
  }
  if (_maxMapId) {
    _tmpNetSpefId = _parser->getInt(1, 1);
    _spefName = _nameMapTable->geti(_tmpNetSpefId);
  } else {
    _tmpNetSpefId = 0;
    _spefName = _parser->get(1);
  }

  _d_net = getDbNet(&netId, _tmpNetSpefId);
  if (!_d_net) {
    return 0;
  }
  if (_cornerBlock == _block) {
    _d_corner_net = _d_net;
  } else {
    _d_corner_net = dbNet::getNet(_cornerBlock, _d_net->getId());
  }
  _tmpNetName = _nameMapTable->geti(_tmpNetSpefId);

  _inputNet = (_tnetCnt == 0 || _d_net->isMarked()) ? true : false;

  if (_diff) {
    diffNetCap(_d_corner_net);
    resetCap(_netGndCapTable, 10);
    resetCap(_netCCapTable, 10);
    resetCap(_netResTable, 10);
  }
  dbRSeg* zrseg = _d_corner_net->getZeroRSeg();
  if (!_diff && !_keep_loaded_corner && zrseg) {
    logger_->warn(RCX,
                  280,
                  "Net {} {} has rseg before reading spef",
                  _d_net->getId(),
                  _d_net->getConstName());
    return 0;
  }
  if (!_diff && !_keep_loaded_corner) {
    zrseg = dbRSeg::create(_d_corner_net,
                           0 /*x*/,
                           0 /*y*/,
                           0,
                           false);  // create zrseg, "foreign" mode
  }
  bool fstRSegDone = false;
  if (_readingNodeCoords != C_NONE) {
    resetNodeCoordTables();
  }

  bool s_coordFlag = false;  // parse res locations for SPEF specs
  uint srcCapNodeId;
  uint dstCapNodeId;
  uint cpos;

  while (_parser->parseNextLine() > 0) {
    if (strcmp("*CONN", _parser->get(0)) == 0) {
      while (_parser->parseNextLine() > 0) {
        if (_readingNodeCoords == C_ON && !_diff) {
          cpos = 0;
          if (strcmp("*N", _parser->get(0)) == 0) {
            cpos = 2;
          } else if (strcmp("*I", _parser->get(0)) == 0
                     || strcmp("*P", _parser->get(0)) == 0) {
            cpos = 3;
          }
          if (cpos != 0) {
            s_coordFlag = readNodeCoords(cpos);
            if (cpos != 2
                || s_coordFlag) {  // /fs/ae/agusta/siart_tss/mme_run/myn.spef.gz
                                   // has *N with (0,0) , but no coords
                                   // for *I and *P
              continue;
            }
          }
        }
        if (strcmp("*END", _parser->get(0)) == 0) {
          return 0;
        }
        if (strcmp("*RES", _parser->get(0)) == 0) {
          break;
        }
        if (strcmp("*CAP", _parser->get(0)) == 0) {
          break;
        }
        if (_readingNodeCoords == C_ON) {
          if (_readingNodeCoordsInput == C_ON) {
            logger_->warn(RCX,
                          281,
                          "\"-N s\" in read_spef command, but no coordinates "
                          "in spef file.");
          }
          _readingNodeCoords = _readingNodeCoordsInput;
        }

        // skip
      }
    }
    if (strcmp("*CAP", _parser->get(0)) == 0) {
      while (_parser->parseNextLine() > 0) {
        if (strcmp("*RES", _parser->get(0)) == 0) {
          break;
        }
        if (strcmp("*END", _parser->get(0)) == 0) {
          return endNet(_d_corner_net, resCnt);
        }

        uint wCnt = _parser->getWordCnt();

        if (!(wCnt >= 3))  // should be 3 or 4!!
        {
          _parser->syntaxError("Unexpected number of tokens");
          continue;
        }
        if (wCnt == 3 && _rCap)  // Grounding cap
        {
          _gndCapCnt++;
          if (_statsOnly) {
            continue;
          }

          netId = 0;
          uint cnid = getCapNodeId(_parser->get(1), _parser->get(2), &netId);
          if (!cnid) {
            continue;
          }
          // return 0;
        } else if (wCnt == 4)  // Coupling cap
        {
          _ccCapCnt++;
          if (_statsOnly) {
            continue;
          }

          netId = 1;
          uint srcId = getCapNodeId(_parser->get(1), nullptr, &netId);
          if (!srcId) {
            continue;
          }
          if (!_testParsing) {
            srcNet = dbNet::getNet(_block, netId);
          }

          netId = 2;
          uint dstId = getCapNodeId(_parser->get(2), nullptr, &netId);
          if (!dstId) {
            continue;
          }

          if (_testParsing) {
            continue;
          }

          tgtNet = dbNet::getNet(_block, netId);

          uint capCnt = _nodeParser->mkWords(_parser->get(3));

          if (_match) {
            collectRefCCap(srcNet, tgtNet, capCnt);
            continue;
          }
          if (_diff) {
            diffCCap(srcNet, srcId, tgtNet, dstId, capCnt);
            continue;
          }
          if (!_inputNet || (!_rCap && !_rOnlyCCcap)) {
            continue;
          }

          dbCapNode* srcCapNode = dbCapNode::getCapNode(_cornerBlock, srcId);
          dbCapNode* tgtCapNode = dbCapNode::getCapNode(_cornerBlock, dstId);
          if (srcId == dstId) {
            logger_->warn(
                RCX,
                282,
                "Source capnode {} is the same as target capnode {}. Add "
                "the cc capacitance to ground.",
                _parser->get(1),
                _parser->get(2));
            if (_readAllCorners) {
              for (uint ii = 0; ii < capCnt; ii++) {
                srcCapNode->addCapacitance(
                    _cap_unit * _nodeParser->getDouble(ii), ii);
              }
            } else {
              srcCapNode->addCapacitance(
                  _cap_unit * _nodeParser->getDouble(_in_spef_corner),
                  _db_ext_corner);
            }
            continue;
          }
          dbCCSeg* ccap = dbCCSeg::create(srcCapNode, tgtCapNode, true);
          if (_readAllCorners) {
            for (uint ii = 0; ii < capCnt; ii++) {
              ccap->setCapacitance(_cap_unit * _nodeParser->getDouble(ii), ii);
            }
          } else {
            ccap->setCapacitance(
                _cap_unit * _nodeParser->getDouble(_in_spef_corner),
                _db_ext_corner);
          }
        }
      }
    }
    if (!(_testParsing || _statsOnly) && _rRun == 1 && !_extracted) {
      _d_corner_net->getCapNodes().reverse();
    }
    if ((!_testParsing) && (!_statsOnly) && _rRun == 1) {
      _d_corner_net->reverseCCSegs();
    }

    if (strcmp("*RES", _parser->get(0)) == 0) {
      if (_diff) {
        diffNetGndCap(_d_corner_net);
      }

      if (!_testParsing && !_statsOnly) {
        _d_corner_net->setSpef(true);
      }

      while (_parser->parseNextLine() > 0) {
        if (strcmp("*END", _parser->get(0)) == 0) {
          return endNet(_d_corner_net, resCnt);
        }

        _resCnt++;
        if (_statsOnly) {
          continue;
        }

        if (_diff) {
          uint resCnt = _nodeParser->mkWords(_parser->get(3));
          for (uint ii = 0; ii < resCnt; ii++) {
            double res = _res_unit * _nodeParser->getDouble(ii);
            _netResTable[ii] += res;
          }
          continue;
        }
        if (!_testParsing && _rRes && _inputNet) {
          netId = 0;
          srcCapNodeId = getCapNodeId(_parser->get(1), nullptr, &netId);
          if (!srcCapNodeId) {
            return 0;
          }
          netId = 0;
          dstCapNodeId = getCapNodeId(_parser->get(2), nullptr, &netId);
          if (!dstCapNodeId) {
            return 0;
          }

          if (fstRSegDone == false) {
            fstRSegDone = true;
            zrseg->setTargetNode(srcCapNodeId);
            if (_readingNodeCoords == C_NONE) {
              int ttx, tty;
              dbCapNode::getCapNode(_cornerBlock, srcCapNodeId)
                  ->getTermCoords(ttx, tty);
              zrseg->setCoords(ttx, tty);
            }
          }
          dbRSeg* rseg;
          if (_keep_loaded_corner) {
            rseg = _d_net->findRSeg(srcCapNodeId, dstCapNodeId);
          } else {
            rseg = dbRSeg::create(_d_corner_net,
                                  0 /*x*/,
                                  0 /*y*/,
                                  0,
                                  false);  //"foreign" mode
          }

          uint resCnt = _nodeParser->mkWords(_parser->get(3));
          if (_readAllCorners) {
            for (uint ii = 0; ii < resCnt; ii++) {
              rseg->setResistance(_res_unit * _nodeParser->getDouble(ii), ii);
            }
          } else {
            rseg->setResistance(
                _res_unit * _nodeParser->getDouble(_in_spef_corner),
                _db_ext_corner);
          }

          rseg->setSourceNode(srcCapNodeId);
          rseg->setTargetNode(dstCapNodeId);
        }

        resCnt++;
      }
    }
  }
  return 0;  // should not get here!!!
}

void extSpef::setupMapping(uint itermCnt)
{
  if (_btermTable) {
    return;
  }
  uint btermCnt = 0;
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
  _nodeTable = new Ath__array1D<uint>(16000);
}

void extSpef::resetNameTable(const uint n)
{
  _nameMapTable = new Ath__array1D<const char*>(128000);
  _nameMapTable->resize(n);
  _lastNameMapIndex = 0;
}

const char* extSpef::makeName(const char* name)
{
  uint len = strlen(name);
  char* a = new char[len + 1];
  strcpy(a, name);
  return a;
}

void extSpef::createName(const uint n, const char* name)
{
  const char* newName = makeName(name);
  _nameMapTable->set(n, newName);
  _lastNameMapIndex = n;
}

void extSpef::addNetNodeHash(dbNet* net)
{
  const uint netId = net->getId();
  for (dbCapNode* capNode : net->getCapNodes()) {
    const uint capId = capNode->getId();
    const uint nodeNum = capNode->getNode();
    if (capNode->isBTerm()) {
      dbBTerm* bterm = dbBTerm::getBTerm(_block, nodeNum);
      bterm->setExtId(capId);
      continue;
    }
    if (capNode->isITerm()) {
      dbITerm* iterm = dbITerm::getITerm(_block, nodeNum);
      iterm->setExtId(capId);
      continue;
    }
    char nodeWord[100];
    sprintf(nodeWord, "*%d%s%d", netId, _delimiter, nodeNum);
    addNewCapIdOnCapTable(nodeWord, capId);
  }
}

void extSpef::buildNodeHashTable()
{
  for (dbNet* net : _cornerBlock->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }
    addNetNodeHash(net);
  }
}

void extSpef::reinit()
{
  delete _nodeHashTable;
  _nodeHashTable = nullptr;
  delete _notFoundInst;
  _notFoundInst = nullptr;
  _rRun = 0;
}

uint extSpef::readBlockIncr(const uint debug)
{
  _noNameMap = _noPorts = false;
  if (!(readHeaderInfo(0, true))) {
    _parser->syntaxError("Header Section");
    return 0;
  }
  if (!_noNameMap && !(readNameMap(0, true))) {
    _parser->syntaxError("NameMap Section");
    return 0;
  }
  if (!_noPorts && !(readPorts())) {
    _parser->syntaxError("NameMap Section");
    return 0;
  }
  if (_readingNodeCoords != C_NONE) {
    initNodeCoordTables(128000);  // to be used for processing  *N lines
  }

  _diffLogCnt = 0;

  uint cnt = 0;
  _unmatchedSpefNet = 0;
  _unmatchedSpefInst = 0;
  _loopNet = 0;
  _bigLoop = 0;
  _multipleLoop = 0;
  _breakLoopNet = 0;
  const bool sortingRSeg
      = !_keep_loaded_corner && (_doSortRSeg || _readingNodeCoords != C_NONE);
  do {
    cnt++;
    readDNet(debug);
    if (sortingRSeg) {
      sortRSegs();
      for (dbCapNode* node : _d_corner_net->getCapNodes()) {
        node->setSortIndex(0);
      }
    }
    if (cnt % 100000 == 0) {
      logger_->info(RCX, 283, "Have read {} nets", cnt);
    }

  } while (_parser->parseNextLine() > 0);
  if (sortingRSeg) {
    _cornerBlock->preExttreeMergeRC(0.0, 0);
  }
  if (_loopNet) {
    logger_->warn(RCX, 284, "There are {} nets with looped spef rc", _loopNet);
  }
  if (_breakLoopNet) {
    logger_->warn(RCX, 78, "Break simple loop of {} nets", _breakLoopNet);
  }
  setSpefFlag(false);

  deleteNodeCoordTables();

  if (_diff && (_diffLogFP != nullptr)) {
    fclose(_diffLogFP);
    fclose(_diffOutFP);
  }
  logger_->info(
      RCX,
      444,
      "Read {} D_NET nets, {} resistors, {} gnd caps {} coupling caps",
      _resCnt,
      _gndCapCnt,
      _ccCapCnt,
      cnt);

  return cnt;
}

uint extSpef::readBlock(const uint debug,
                        const std::vector<dbNet*>& tnets,
                        const bool force,
                        const bool rConn,
                        const char* nodeCoord,
                        const bool rCap,
                        const bool rOnlyCCcap,
                        const bool rRes,
                        const float cc_thres,
                        const float length_unit,
                        const bool extracted,
                        const bool keepLoadedCorner,
                        const bool stampWire,
                        const uint testParsing,
                        const int app_print_limit,
                        const bool m_map,
                        const int corner,
                        const double lo,
                        const double up,
                        const char* excludeNetSubWord,
                        const char* netSubWord,
                        const char* capStatsFile,
                        const char* dbCornerName,
                        const char* calibrateBaseCorner,
                        int spefCorner,
                        const int fixLoop,
                        bool& rsegCoord)
{
  _stampWire = stampWire;
  _rConn = rConn;
  _rCap = rCap;
  _rOnlyCCcap = rOnlyCCcap;
  _rRes = rRes;
  _tnetCnt = tnets.size();
  _extracted = extracted;
  _keep_loaded_corner = keepLoadedCorner;

  _mMap = m_map;
  _dbCorner = corner;
  _lowerThres = lo;
  _upperThres = up;

  _capStatsFP = nullptr;
  _netSubWord = nullptr;
  _netExcludeSubWord = nullptr;
  _readingNodeCoords = C_NONE;
  if (force) {
    _rRun = 1;
  }
  _lengthUnit = length_unit;
  _fixloop = fixLoop;
  const int dbunit = _block->getDbUnitsPerMicron();
  _nodeCoordFactor = (double) dbunit * _lengthUnit;
  if (nodeCoord && strcmp(nodeCoord, "s") == 0) {
    _readingNodeCoords = C_ON;
  } else if (nodeCoord && strcmp(nodeCoord, "s01") == 0) {
    _readingNodeCoords = C_ON;
    _nodeCoordFactor = 0.1;
  } else if (nodeCoord && strcmp(nodeCoord, "S") == 0) {
    _readingNodeCoords = C_ON;
  } else if (nodeCoord) {
    logger_->info(RCX, 178, "\" -N {} \" is unknown.", nodeCoord);
    return 0;
  }
  _readingNodeCoordsInput = _readingNodeCoords;
  if (_readingNodeCoords != C_NONE && _nodeCoordParser == nullptr) {
    _nodeCoordParser = new Ath__parser(logger_);
  }
  if (capStatsFile != nullptr) {
    _capStatsFP = fopen(capStatsFile, "w");
  }
  if (excludeNetSubWord != nullptr) {
    _netExcludeSubWord = new char[1024];
    strcpy(_netExcludeSubWord, excludeNetSubWord);
  }
  if (netSubWord != nullptr) {
    _netSubWord = new char[1024];
    strcpy(_netSubWord, netSubWord);
  }
  _cc_thres = 100000000;
  _cc_thres_flag = false;
  if (cc_thres >= 0.0) {
    _cc_thres_flag = true;
    _cc_thres = cc_thres;
    _cc_break_cnt = 0;
  }
  _cc_merge_cnt = 0;
  if (!rConn && !rCap && !rOnlyCCcap && !rRes) {
    _rConn = _rCap = _rRes = true;
  }
  for (uint j = 0; j < _tnetCnt; j++) {
    tnets[j]->setMark(true);
  }

  if (testParsing == 1) {
    _testParsing = true;
  }
  if (testParsing == 2) {
    _statsOnly = true;
  }

  if (!_testParsing && !_statsOnly) {
    int cornerCnt = 0;

    _maxMapId = readMaxMapId(&cornerCnt);
    if (cornerCnt == 0) {
      logger_->info(RCX, 286, "Number of corners in SPEF file = 0.");
      return 0;
    }
    if ((spefCorner < 0) && (cornerCnt == 1)) {
      spefCorner = 0;
    }

    _db_calibbase_corner = -1;
    if (calibrateBaseCorner != nullptr) {
      int n = _block->getExtCornerIndex(calibrateBaseCorner);
      if (n < 0) {
        logger_->info(
            RCX, 404, "Cannot find corner name {} in DB", calibrateBaseCorner);
        return 0;
      }
      _db_calibbase_corner = n;
    }
    if (dbCornerName != nullptr) {
      const int n = _block->getExtCornerIndex(dbCornerName);
      if (n < 0) {
        logger_->info(
            RCX, 287, "Cannot find corner name {} in DB", dbCornerName);
        return 0;
      }
      _db_ext_corner = n;
    } else if (corner >= 0) {
      if (corner >= (int) _cornerCnt) {
        logger_->info(RCX,
                      288,
                      "Ext corner {} out of range; There are only {} defined "
                      "process corners.",
                      corner,
                      _cornerCnt);
        return 0;
      }
      _db_ext_corner = corner;
    } else if ((corner < 0) && (_cornerCnt == 1)) {
      _db_ext_corner = 0;
    }

    if (spefCorner == -1 && _cornerCnt && cornerCnt != (int) _cornerCnt) {
      logger_->info(
          RCX,
          289,
          "Mismatch on the numbers of corners: Spef file has {} corners vs. "
          "Process corner table has {} corners.(Use -spef_corner option).",
          cornerCnt,
          _cornerCnt);
      return 0;
    }
    if (spefCorner > cornerCnt - 1) {
      logger_->info(
          RCX,
          290,
          "Spef corner {} out of range; There are only {} corners in Spef file",
          spefCorner,
          cornerCnt);
      return 0;
    }
    _readAllCorners = false;
    if (!_extracted) {
      const uint prevCornerCnt = _cornerCnt;
      if (spefCorner < 0) {  // assumption: read all
        if (prevCornerCnt == 0) {
          setCornerCnt(cornerCnt);
        }
        _readAllCorners = true;
      } else {
        // Assumption: can only read one corner on DB; cannot read another after
        // the first
        _in_spef_corner = spefCorner;
        if (_db_ext_corner == -1) {
          _db_ext_corner = 0;
        }
        if (prevCornerCnt == 0) {
          setCornerCnt(1);
        }
      }
      if (!prevCornerCnt) {
        extMain::addDummyCorners(_block, _cornerCnt, logger_);
      }
    } else {  // one corner at a time!
      if ((spefCorner < 0) && (_db_ext_corner < 0)) {
        // no spef_corner and db_corner_name specified, try all corners
        _readAllCorners = true;
      } else if (_db_ext_corner >= 0) {  // target specific db corner
        _in_spef_corner = spefCorner;
        if (spefCorner <= 0) {  // assuming 0 spef corner index
          _in_spef_corner = 0;
        }
      } else if (_db_ext_corner < 0) {  // target specific db corner
        logger_->info(RCX, 291, "Have to specify option _db_corner_name");
        return 0;
      }
    }
  }

  _cornerBlock = _block;

  if (_db_calibbase_corner >= 0) {
    _block->copyExtDb(_db_calibbase_corner,
                      _db_ext_corner,
                      _cornerCnt,
                      1.0 /*resFactor*/,
                      1.0 /*ccFactor*/,
                      1.0 /*gndcFactor*/);
    setUseIdsFlag(true /*diff*/, true /*calib*/);
    setCalibLimit(101.0 /*upper_limit*/, 0.009 /*lower_limit*/);
    _keep_loaded_corner = true;
  }

  if (!_testParsing && !_statsOnly && _rRun == 1) {
    if (!_diff && (!extracted || !_keep_loaded_corner)) {
      if (extracted) {
        resetExtIds(1);
      }
      _block->setCornerCount(_cornerCnt);
      _extracted = false;
    }
    setupMapping();

    _idMapTable->resize(_maxMapId + 1);
    resetNameTable(_maxMapId + 1);
  }
  if (_nodeHashTable == nullptr) {
    _nodeHashTable = new NameTable(8000000);
  }
  if (_notFoundInst == nullptr) {
    _notFoundInst = new NameTable(800);
  }
  if (_rRun == 1 && _extracted && _rOnlyCCcap) {
    buildNodeHashTable();
  }

  _cc_app_print_limit = app_print_limit;
  if (_cc_app_print_limit) {
    _ccidmap = new Ath__array1D<int>(8000000);
  }
  uint cnt = 0;
  bool rc;
  _noNameMap = _noPorts = false;
  if (!(rc = readHeaderInfo(0))) {
    _parser->syntaxError("Header Section");
  } else if (!_noNameMap && !(rc = readNameMap(0))) {
    _parser->syntaxError("NameMap Section");
  } else if (!_noPorts && !(rc = readPorts())) {
    _parser->syntaxError("Ports Section");
  } else {
    _nodeParser->resetSeparator(_delimiter);

    if (_rRun == 1) {
      setSpefFlag(false);
    }

    if (_readingNodeCoords != C_NONE) {
      initNodeCoordTables(128000);  // to be used for processing  *N lines
      _nodeCoordParser->resetSeparator("[,=");
    }

    setExtIds();

    _unmatchedSpefNet = 0;
    _unmatchedSpefInst = 0;
    _loopNet = 0;
    _bigLoop = 0;
    _multipleLoop = 0;
    _breakLoopNet = 0;
    bool doSortingRSeg = false;
    do {
      cnt++;
      readDNet(debug);

      if (cnt % 100000 == 0) {
        logger_->info(
            RCX,
            445,
            "Have read {} D_NET nets, {} resistors, {} gnd caps {} coupling "
            "caps",
            cnt,
            _resCnt,
            _gndCapCnt,
            _ccCapCnt);
      }
      const bool sortingRSeg = _d_net && !_keep_loaded_corner
                               && (_doSortRSeg || _readingNodeCoords != C_NONE);
      doSortingRSeg |= sortingRSeg;
      if (sortingRSeg) {
        sortRSegs();
        for (dbCapNode* node : _d_corner_net->getCapNodes()) {
          node->setSortIndex(0);
        }
      }
    } while (_parser->parseNextLine() > 0);
    if (doSortingRSeg) {
      _cornerBlock->preExttreeMergeRC(0.0, 0);
    }
    if (_stampWire) {
      _block->getExtControl()->_wireStamped = true;
    }
    if (_loopNet) {
      logger_->warn(RCX, 292, "{} nets with looped spef rc", _loopNet);
    }
    if (_breakLoopNet) {
      logger_->warn(RCX, 285, "Break simple loop of {} nets", _breakLoopNet);
    }

    if (!(_testParsing || _statsOnly)) {
      resetExtIds(0);
    }
  }
  deleteNodeCoordTables();

  if (_diff && (_diffLogFP != nullptr)) {
    fclose(_diffLogFP);
    fclose(_diffOutFP);
  }

  if (_capStatsFP != nullptr) {
    fclose(_capStatsFP);
  }

  for (uint j = 0; j < _tnetCnt; j++) {
    tnets[j]->setMark(false);
  }
  if (!rc) {
    return 0;
  }

  if (_calib && !_match) {
    for (dbNet* net : _block->getNets()) {
      if (net->getSigType().isSupply()) {
        continue;
      }
      net->calibrateCouplingCap(_db_ext_corner);
    }
  }

  logger_->info(
      RCX,
      463,
      "Have read {} D_NET nets, {} resistors, {} gnd caps {} coupling caps",
      cnt,
      _resCnt,
      _gndCapCnt,
      _ccCapCnt);

  if (_cc_merge_cnt) {
    logger_->info(RCX, 60, "     merged {} coupling caps", _cc_merge_cnt);
  }
  if (_cc_thres_flag) {
    logger_->info(RCX,
                  464,
                  "Broke {} coupling caps of {} fF or smaller",
                  _cc_break_cnt,
                  _cc_thres);
  }
  uint unmatchedDbNet = 0;
  uint unmatchedDbInst = 0;
  if (!_moreToRead) {
    for (dbNet* net : _block->getNets()) {
      if (net->getSigType().isSupply()) {
        continue;
      }
      dbNet* cornerNet = dbNet::getNet(_cornerBlock, net->getId());
      if (!cornerNet->isSpef()) {
        unmatchedDbNet++;
        if (unmatchedDbNet < 20) {
          logger_->warn(RCX,
                        448,
                        "Db net {} {} not read from spef file!",
                        net->getId(),
                        (char*) net->getConstName());
        }
      }
    }
    for (dbInst* inst : _block->getInsts()) {
      if (!inst->getUserFlag1()) {
        unmatchedDbInst++;
        if (unmatchedDbInst < 20) {
          logger_->warn(RCX,
                        447,
                        "Db inst {} {} not read from spef file!",
                        inst->getId(),
                        (char*) inst->getConstName());
        }
      }
      inst->clearUserFlag1();
    }
  }

  if (unmatchedDbNet) {
    logger_->warn(RCX, 48, "{} db nets not read from spef.", unmatchedDbNet);
  }
  if (unmatchedDbInst) {
    logger_->warn(RCX, 49, "{} db insts not read from spef.", unmatchedDbInst);
  }
  if (_unmatchedSpefNet) {
    logger_->warn(RCX, 50, "{} spef nets not found in db.", _unmatchedSpefNet);
  }
  if (_unmatchedSpefInst) {
    logger_->warn(
        RCX, 44, "{} spef insts not found in db.", _unmatchedSpefInst);
  }
  if (_unmatchedSpefNet || _unmatchedSpefInst) {
    logger_->error(RCX, 52, "Unmatched spef and db!");
  }

  rsegCoord = _readingNodeCoords == C_NONE ? false : true;
  return cnt;
}

void extSpef::printCapNode(const uint capNodeId)
{
  dbCapNode* capNode = dbCapNode::getCapNode(_cornerBlock, capNodeId);
  const int tid = _ccidmap->geti(capNodeId);
  const uint tnode = capNode->getNode();
  if (capNode->isITerm()) {
    dbITerm* iterm = dbITerm::getITerm(_block, tnode);
    logger_->info(RCX,
                  293,
                  "*{}{}{}",
                  tid,
                  _delimiter,
                  iterm->getMTerm()->getName().c_str());
  } else if (capNode->isBTerm()) {
    logger_->info(
        RCX, 449, "{} ", dbBTerm::getBTerm(_block, tid)->getName().c_str());
  } else {
    logger_->info(RCX, 451, "{}", _nodeHashTable->getName(tid));
  }
}

void extSpef::printAppearance(const int app, const int appc)
{
  const int pcnt
      = appc < (int) _cc_app_print_limit ? appc : _cc_app_print_limit;
  logger_->info(
      RCX, 294, "    First {} cc that appear {} times", pcnt, app + 1);
  int cnt = 0;
  for (dbCCSeg* cc : _cornerBlock->getCCSegs()) {
    if (cc->getInfileCnt() != app) {
      continue;
    }
    printCapNode(cc->getSourceCapNode()->getId());
    printCapNode(cc->getTargetCapNode()->getId());
    cnt++;
    if (cnt >= pcnt) {
      break;
    }
  }
}

void extSpef::printAppearance(const int* appcnt, const int tapp)
{
  if (_cc_app_print_limit == 0) {
    return;
  }
  for (int jj = 0; jj < tapp; jj++) {
    if (jj == 1 || appcnt[jj] == 0) {
      continue;
    }
    printAppearance(jj, appcnt[jj]);
  }
}

bool extSpef::readPorts()
{
  while (_parser->parseNextLine() > 0) {
    if (strcmp("*D_NET", _parser->get(0)) == 0) {
      return true;
    }
  }
  return false;
}

uint extSpef::readMaxMapId(int* cornerCnt)
{
  _nodeParser->resetSeparator(_delimiter);

  uint maxId = 0;
  bool dnetFound = false;
  while (_parser->parseNextLine() > 0) {
    if (strcmp("*NAME_MAP", _parser->get(0)) != 0
        && strcmp("*D_NET", _parser->get(0))
               != 0) {  // skip to *NAME_MAP or *D_NET
      continue;
    }

    dnetFound = strcmp("*D_NET", _parser->get(0)) == 0;
    while (dnetFound || _parser->parseNextLine() > 0) {
      if (strcmp("*PORTS", _parser->get(0)) == 0
          || strcmp("*D_NET", _parser->get(0)) == 0)  // all nets
      {
        dnetFound = strcmp("*D_NET", _parser->get(0)) == 0;
        while (dnetFound || _parser->parseNextLine() > 0) {
          if (strcmp("*D_NET", _parser->get(0)) != 0) {
            continue;
          }

          if (cornerCnt != nullptr) {
            *cornerCnt = _nodeParser->mkWords(_parser->get(2));
          }

          _parser->resetLineNum(0);
          _parser->openFile();

          return maxId;
        }
      }

      if (strcmp("*DEFINE", _parser->get(0)) == 0) {
        continue;
      }

      if (_parser->isDigit(0, 1)) {
        const uint id = _parser->getInt(0, 1);
        maxId = std::max(maxId, id);
      }
    }
  }
  return maxId;
}

void extSpef::addNameMapId(const uint ii, const uint id)
{
  _idMapTable->set(ii, id);
}

uint extSpef::getNameMapId(const uint ii)
{
  return _idMapTable->geti(ii);  // TODO: have to find a better name for geti
}

bool extSpef::readNameMap(const uint debug, const bool skip)
{
  while (_parser->parseNextLine() > 0) {
    if (strcmp("*PORTS", _parser->get(0)) == 0) {
      return true;
    }
    if (strcmp("*D_NET", _parser->get(0)) == 0) {
      logger_->warn(RCX, 295, "There is no *PORTS section");
      _noPorts = true;
      return true;
    }
    if (skip) {
      continue;
    }

    if (strcmp("*DEFINE", _parser->get(0)) == 0) {
      continue;
    }

    if (_testParsing || _statsOnly) {
      _parser->getInt(0, 1);
    } else if (_rRun == 1) {
      uint id = _parser->getInt(0, 1);
      createName(id, _parser->get(1));
    }
  }
  return false;
}

bool extSpef::readHeaderInfo(const uint debug, const bool skipFlag)
{
  while (_parser->parseNextLine() > 0) {
    if (_parser->isKeyword(0, "*NAME_MAP")) {
      return true;
    }
    if (_parser->isKeyword(0, "*PORTS")) {
      _noNameMap = true;
      logger_->warn(RCX, 296, "There is no *NAME_MAP section");
      return true;
    }
    if (_parser->isKeyword(0, "*D_NET")) {
      _noNameMap = true;
      logger_->warn(RCX, 297, "There is no *NAME_MAP section");
      _noPorts = true;
      logger_->warn(RCX, 298, "There is no *PORTS section");
      return true;
    }
    if (skipFlag) {
      continue;
    }

    if (_parser->isKeyword(0, "*DESIGN")) {
      _parser->mkWords(_parser->get(1), "\"");
      strcpy(_design, _parser->get(0));
    } else if (_parser->isKeyword(0, "*DIVIDER")) {
      strcpy(_divider, _parser->get(1));
    } else if (_parser->isKeyword(0, "*DELIMITER")) {
      strcpy(_delimiter, _parser->get(1));
    } else if (_parser->isKeyword(0, "*BUS_DELIMITER")) {
      strcpy(_bus_delimiter, _parser->get(1));
      if (_parser->getWordCnt() > 2) {
        strcat(_bus_delimiter, _parser->get(2));
      }
    } else if (_parser->isKeyword(0, "*DESIGN_FLOW")) {
    } else if (_parser->isKeyword(0, "*T_UNIT")) {
    } else if (_parser->isKeyword(0, "*R_UNIT")) {
      strcpy(_res_unit_word, _parser->get(2));

      _res_unit = 1.0;
      if (strcmp("MOHM", _res_unit_word) == 0) {
        _res_unit = 0.001 * _parser->getInt(1);
      }
      if (strcmp("KOHM", _res_unit_word) == 0) {
        _res_unit = 1000.0 * _parser->getInt(1);
      }

    } else if (_parser->isKeyword(0, "*C_UNIT")) {
      strcpy(_cap_unit_word, _parser->get(2));

      _cap_unit = 1.0;
      if (strcmp("PF", _cap_unit_word) == 0) {
        _cap_unit = 1000.0 * _parser->getInt(1);
      } else if (strcmp("FF", _cap_unit_word) == 0) {
        _cap_unit = 1.0 * _parser->getInt(1);
      }
    } else if (_parser->isKeyword(0, "*L_UNIT")) {
      continue;
    }
  }
  return false;
}

}  // namespace rcx
