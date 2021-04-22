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

#include "dbUtil.h"

#include "db.h"
#include "dbShape.h"
#include "dbTechLayerRule.h"
#include "dbWireCodec.h"
#include "utl/Logger.h"

namespace odb {

dbCreateNetUtil::dbCreateNetUtil() : _ruleNameHint(0)
{
  _milosFormat = false;
  _skipPowerNets = true;
  _currentNet = NULL;
  _mapArray = NULL;
  _ecoCnt = 0;
}
dbCreateNetUtil::~dbCreateNetUtil()
{
  if (_mapArray != NULL)
    free(_mapArray);
}
void dbCreateNetUtil::allocMapArray(uint n)
{
  _mapArray = (dbNet**) realloc(NULL, n * sizeof(dbNet*));
  _mapCnt = n;
  for (uint ii = 0; ii < n; ii++)
    _mapArray[ii] = NULL;
}
void dbCreateNetUtil::setCurrentNet(dbNet* net)
{
  _currentNet = net;
}
dbNet* dbCreateNetUtil::getCurrentNet()
{
  return _currentNet;
}

void dbCreateNetUtil::checkAndSet(uint id)
{
  if (id >= _mapCnt) {  // re-aLLOC

    uint n = _mapCnt * 2;
    while (id >= n)
      n *= 2;

    _mapArray = (dbNet**) realloc(_mapArray, n * sizeof(dbNet*));
    if (_mapArray == NULL) {
      error(0, "Cannot allocate %lu MBytes for mapArray\n", n * sizeof(dbNet*));
    } else {
      for (uint ii = _mapCnt; ii < n; ii++)
        _mapArray[ii] = NULL;

      _mapCnt = n;
    }
  }
  _currentNet = _mapArray[id];
}

dbInst* dbCreateNetUtil::createInst(dbInst* inst0)
{
  char instName[64];
  sprintf(instName, "N%d", inst0->getId());

  dbInst* inst = dbInst::create(_block, inst0->getMaster(), instName);
  if (inst == NULL)
    return NULL;

  inst->setOrient(inst0->getOrient());
  int x, y;
  inst0->getOrigin(x, y);
  inst->setOrigin(x, y);
  inst->setPlacementStatus(inst0->getPlacementStatus());

  return inst;
}

dbBlock* dbCreateNetUtil::createBlock(dbBlock* blk,
                                      bool /* unused: copyViaTable */)
{
  char blk_name[32];
  sprintf(blk_name, "%s__%d", "eco", blk->getId());
  _block = blk->findChild(blk_name);
  if (_block != NULL) {
    warning(0,
            "there is already an ECO block present!, will continue updating\n");
    return _block;
  }
  _block = dbBlock::create(blk, blk_name, '/');
  assert(_block);
  _block->setBusDelimeters('[', ']');
  _block->setBusDelimeters('[', ']');
  _block->setDefUnits(blk->getDefUnits());

  //	if (copyViaTable)
  //		dbBlock::copyViaTable( _block, blk );

  return _block;
}
dbInst* dbCreateNetUtil::createInst(dbInst* inst0,
                                    bool createInstance,
                                    bool destroyInstance)
{
  dbInst* ii = _block->findInst(inst0->getConstName());
  if (ii != NULL) {  // TO_OPTIMIZE
    if (createInstance && ii->getEcoDestroy()) {
      dbInst::destroy(ii);
      return inst0;
    }
    if (destroyInstance && ii->getEcoCreate()) {
      dbInst::destroy(ii);
      return inst0;
    }
    return ii;
  } else {
    dbInst* inst
        = dbInst::create(_block, inst0->getMaster(), inst0->getConstName());
    inst->setOrient(inst0->getOrient());
    int x, y;
    inst0->getOrigin(x, y);
    inst->setOrigin(x, y);
    inst->setPlacementStatus(inst0->getPlacementStatus());

    if (createInstance)
      inst->setEcoCreate(true);
    else if (destroyInstance)
      inst->setEcoDestroy(true);
    else
      inst->setEcoModify(true);

    return inst;
  }
}
dbNet* dbCreateNetUtil::updateNet(dbNet* nn, bool create, bool destroy)
{
  dbNet* ecoNet = _block->findNet(nn->getConstName());
  if (ecoNet == NULL)
    return createNet(nn, create, destroy);

  if (ecoNet->isMark_1ed()) {  // original new net
    if (destroy)
      dbNet::destroy(ecoNet);
  } else if (ecoNet->isMarked()) {  // original deleted net
    if (create) {
      dbNet::destroy(ecoNet);
      // ecoNet->setMark(false);
    }
  } else {
    if (create) {
      ecoNet->setMark_1(true);
      ecoNet->setDisconnected(false);
    } else if (destroy) {
      ecoNet->setMark(true);
      ecoNet->setDisconnected(false);
    }
  }
  return ecoNet;
}
dbInst* dbCreateNetUtil::updateInst(dbInst* inst0,
                                    bool createInstance,
                                    bool destroyInstance)
{
  /*
          if (strcmp(inst0->getConstName(), "MILOS_SETUP_26415_2")==0) {
                  int x1,y1;
                  inst0->getOrigin(x1,y1);
                  std::string orient1 = inst0->getOrient().getString();

              notice(0, "----> updateInst: %s Orientation %s Location %d %d\n",
                  inst0->getConstName(), orient1.c_str(), x1, y1);
      }
  */

  dbInst* ecoInst = _block->findInst(inst0->getConstName());
  if (ecoInst == NULL)
    return createInst(inst0, createInstance, destroyInstance);

  if (ecoInst->getEcoCreate()) {  // original new
    if (destroyInstance)
      dbInst::destroy(ecoInst);

    return inst0;
  } else if (ecoInst->getEcoDestroy()) {
    if (createInstance)
      dbInst::destroy(ecoInst);

    return inst0;
  } else {
    if (createInstance) {
      ecoInst->setEcoCreate(true);
      ecoInst->setEcoDestroy(false);
      ecoInst->setEcoModify(false);
    } else if (destroyInstance) {
      ecoInst->setEcoDestroy(true);
      ecoInst->setEcoCreate(false);
      ecoInst->setEcoModify(false);
    }
    return ecoInst;
  }
}
/*
    public proc milos_createPin { id name } {
    public proc milos_removePin { id } {
    public proc milos_connectNet { id net } {
    public proc milos_disconnectNet { id net } {
    public proc milos_removeNet { name } {
*/
bool dbCreateNetUtil::printEcoInstVerbose(FILE* fp,
                                          dbInst* inst,
                                          const char* header)
{
  if (_verbose)
    return false;

  dbBox* bb = inst->getBBox();

  int x1, y1;
  inst->getOrigin(x1, y1);

  int x2, y2;
  inst->getLocation(x2, y2);

  std::string orient1 = inst->getOrient().getString();

  fprintf(fp,
          "%s %d %s Orient %s Loc %d %d Origin %d %d  W %d H %d  Coords %d %d  "
          "%d %d\n",
          header,
          inst->getId(),
          inst->getMaster()->getConstName(),
          orient1.c_str(),
          x2,
          y2,
          x1,
          y1,
          bb->getDX(),
          bb->getDY(),
          bb->xMin(),
          bb->yMin(),
          bb->xMax(),
          bb->yMax());

  return true;
}

uint dbCreateNetUtil::printEcoInst(dbInst* ecoInst, dbBlock* srcBlock, FILE* fp)
{
  dbInst* origInst = srcBlock->findInst(ecoInst->getConstName());
  if (ecoInst->getEcoDestroy()) {
    if (origInst == NULL) {
      if (_milosFormat)
        fprintf(fp, "milos_removeCell %s\n", ecoInst->getConstName());
      else {
        fprintf(fp, "Milos Delete Instance %s\n", ecoInst->getConstName());
        // annotateSlack(fp, ecoInst);
      }
      return 1;
    }
    return 0;
  }
  if (ecoInst->getEcoCreate()) {
    dbMaster* master = origInst->getMaster();

    if (_milosFormat) {
      fprintf(fp,
              "milos_createCell %s %s\n",
              origInst->getConstName(),
              master->getConstName());
    } else {
      fprintf(fp,
              "Milos Create Instance %s Master %s Lib %s\n",
              origInst->getConstName(),
              master->getConstName(),
              master->getLib()->getConstName());

      // annotateSlack(fp, ecoInst);
    }
    dbBox* origBox = origInst->getBBox();

    int x1, y1;
    origInst->getOrigin(x1, y1);
    if (_useLocation)
      origInst->getLocation(x1, y1);

    std::string orient1 = origInst->getOrient().getString();

    if (_milosFormat) {
      fprintf(fp,
              "milos_moveCell %s Orientation %s Location %d %d    W %d H %d\n",
              origInst->getConstName(),
              orient1.c_str(),
              x1,
              y1,
              origBox->getDX(),
              origBox->getDY());
    } else {
      fprintf(fp,
              "Milos Place Instance %s Orientation %s Location %d %d\n",
              origInst->getConstName(),
              orient1.c_str(),
              x1,
              y1);
    }
    printEcoInstVerbose(fp, origInst, "#V_new ");
    printEcoInstVerbose(fp, ecoInst, "#V_old ");
    return 1;
  }
  uint success = 0;

  if (ecoInst->getMaster() != origInst->getMaster()) {
    if (_milosFormat) {
      fprintf(fp,
              "milos_swapCell %s %s\n",
              ecoInst->getConstName(),
              origInst->getMaster()->getConstName());
    } else {
      fprintf(fp,
              "Milos <SwapMaster> Instance %s Master %s\n",
              ecoInst->getConstName(),
              origInst->getMaster()->getConstName());
      // annotateSlack(fp, ecoInst);
    }
    success = 1;
  }
  dbBox* origBox = origInst->getBBox();
  dbBox* b0 = origInst->getBBox();
  Rect r0;
  b0->getBox(r0);

  dbBox* b1 = ecoInst->getBBox();
  Rect r1;
  b1->getBox(r1);

  // worst case : a box was moved to its original space
  // if (r0!=r1) { // different bboxes
  int x0, y0;
  ecoInst->getOrigin(x0, y0);
  std::string orient = ecoInst->getOrient().getString();
  int x1, y1;
  origInst->getOrigin(x1, y1);
  if (_useLocation)
    origInst->getLocation(x1, y1);

  std::string orient1 = origInst->getOrient().getString();

  if (_milosFormat) {
    fprintf(fp,
            "milos_moveCell %s Orientation %s Location %d %d    W %d H %d\n",
            origInst->getConstName(),
            orient1.c_str(),
            x1,
            y1,
            origBox->getDX(),
            origBox->getDY());
  } else {
    fprintf(fp,
            "Milos Place Instance %s Orientation %s Location %d %d\n",
            origInst->getConstName(),
            orient1.c_str(),
            x1,
            y1);
  }
  printEcoInstVerbose(fp, origInst, "#V_new ");
  printEcoInstVerbose(fp, ecoInst, "#V_old ");

  success = 1;
  //}
  return success;
}
uint dbCreateNetUtil::printEcoTerms(dbNet* net,
                                    const char* termConnect,
                                    FILE* fp)
{
  uint cnt = 0;

  dbSet<dbITerm> iterms = net->getITerms();
  dbSet<dbITerm>::iterator iitr;

  for (iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
    dbITerm* term = *iitr;

    const char* instName = term->getInst()->getConstName();

    if (_milosFormat) {
      fprintf(fp,
              "%s %s %s Net %s\n",
              termConnect,
              instName,
              term->getMTerm()->getName().c_str(),
              net->getConstName());
    } else {
      fprintf(fp,
              "Milos %s Instance %s Terminal %s Net %s\n",
              termConnect,
              instName,
              term->getMTerm()->getName().c_str(),
              net->getConstName());
    }

    cnt++;
  }
  return cnt;
}
uint dbCreateNetUtil::printModifiedNetECO(dbNet* net,
                                          const char* termConnect,
                                          const char* termDisconnect,
                                          FILE* fp)
{
  uint cnt = 0;
  dbSet<dbITerm> iterms = net->getITerms();
  dbSet<dbITerm>::iterator iitr;

  for (iitr = iterms.begin(); iitr != iterms.end(); ++iitr) {
    dbITerm* term = *iitr;

    dbInst* inst = term->getInst();
    const char* instName = inst->getConstName();

    if (this->_milosFormat) {
      if ((termConnect != NULL) && inst->getEcoCreate()) {
        fprintf(fp,
                "%s %s %s Net %s\n",
                termConnect,
                instName,
                term->getMTerm()->getName().c_str(),
                net->getConstName());
        cnt++;
      } else if ((termDisconnect != NULL) && inst->getEcoDestroy()) {
        fprintf(fp,
                "%s %s %s Net %s\n",
                termDisconnect,
                instName,
                term->getMTerm()->getName().c_str(),
                net->getConstName());
        cnt++;
      }
    } else {
      if ((termConnect != NULL) && inst->getEcoCreate()) {
        fprintf(fp,
                "Milos %s Instance %s Terminal %s Net %s\n",
                termConnect,
                instName,
                term->getMTerm()->getName().c_str(),
                net->getConstName());
        cnt++;
      } else if ((termDisconnect != NULL) && inst->getEcoDestroy()) {
        fprintf(fp,
                "Milos %s Instance %s Terminal %s Net %s\n",
                termDisconnect,
                instName,
                term->getMTerm()->getName().c_str(),
                net->getConstName());
        cnt++;
      }
    }
  }
  return cnt;
}
uint dbCreateNetUtil::printEcoNet(dbNet* ecoNet, dbBlock* srcBlock, FILE* fp)
{
  if (ecoNet->isMark_1ed()) {  // create net
    dbNet* origNet = srcBlock->findNet(ecoNet->getConstName());
    if (origNet != NULL) {  // new net
      if (_milosFormat)
        fprintf(fp, "milos_createNet %s\n", origNet->getConstName());
      else {
        fprintf(fp, "Milos Create Net %s\n", origNet->getConstName());
        // annotateSlack(fp, ecoNet);
      }
    } else
      return 0;
  } else if (ecoNet->isMarked()) {  // DestroyNet
    if (_milosFormat)
      fprintf(fp, "milos_removeNet %s\n", ecoNet->getConstName());
    else {
      fprintf(fp, "Milos Delete Net %s\n", ecoNet->getConstName());
      // annotateSlack(fp, ecoNet);
    }
  } else
    return 0;

  return 1;
}
uint dbCreateNetUtil::printNewInsts(dbBlock* ecoBlock,
                                    dbBlock* srcBlock,
                                    FILE* fp)
{
  uint cnt = 0;
  dbSet<dbInst> insts = ecoBlock->getInsts();
  dbSet<dbInst>::iterator iitr;
  for (iitr = insts.begin(); iitr != insts.end(); ++iitr) {
    dbInst* inst = *iitr;
    if (inst->getEcoCreate()) {
      cnt += printEcoInst(inst, srcBlock, fp);
    }
  }
  return cnt;
}
uint dbCreateNetUtil::printDeletedInsts(dbBlock* ecoBlock,
                                        dbBlock* srcBlock,
                                        FILE* fp)
{
  uint cnt = 0;
  dbSet<dbInst> insts = ecoBlock->getInsts();
  dbSet<dbInst>::iterator iitr;
  for (iitr = insts.begin(); iitr != insts.end(); ++iitr) {
    dbInst* inst = *iitr;
    if (inst->getEcoDestroy()) {
      cnt += printEcoInst(inst, srcBlock, fp);
    }
  }
  return cnt;
}
uint dbCreateNetUtil::printModifiedInsts(dbBlock* ecoBlock,
                                         dbBlock* srcBlock,
                                         FILE* fp)
{
  uint cnt = 0;
  dbSet<dbInst> insts = ecoBlock->getInsts();
  dbSet<dbInst>::iterator iitr;
  for (iitr = insts.begin(); iitr != insts.end(); ++iitr) {
    dbInst* inst = *iitr;
    // if (strcmp(inst->getConstName(), "B_19")==0)
    //	notice(0, "printECO inst: %s\n", inst->getConstName());
    if (inst->getEcoModify())
      cnt += printEcoInst(inst, srcBlock, fp);
  }
  return cnt;
}
uint dbCreateNetUtil::printDeletedNets(dbBlock* ecoBlock,
                                       dbBlock* srcBlock,
                                       FILE* fp,
                                       int itermCnt)
{
  const char* disconnectIterm = "DisconnectITerm";
  if (_milosFormat) {
    disconnectIterm = "milos_disconnectNet";
  }

  uint cnt = 0;
  dbSet<dbNet> nets = ecoBlock->getNets();
  dbSet<dbNet>::iterator nitr;
  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    dbNet* net = *nitr;

    if (net->isMarked()) {
      if (itermCnt < 0)
        cnt += printEcoTerms(net, disconnectIterm, fp);
      else
        cnt += printEcoNet(net, srcBlock, fp);
    }
  }
  return cnt;
}
uint dbCreateNetUtil::printDisconnectedTerms(dbBlock* ecoBlock,
                                             dbBlock* /* unused: srcBlock */,
                                             FILE* fp)
{
  uint cnt = 0;
  dbSet<dbNet> nets = ecoBlock->getNets();
  dbSet<dbNet>::iterator nitr;
  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    dbNet* net = *nitr;

    if (net->isMarked())
      cnt += printEcoTerms(net, "DisconnectITerm", fp);
  }
  return cnt;
}
uint dbCreateNetUtil::printConnectedTerms(dbBlock* ecoBlock,
                                          dbBlock* srcBlock,
                                          FILE* fp)
{
  uint cnt = 0;
  dbSet<dbNet> nets = ecoBlock->getNets();
  dbSet<dbNet>::iterator nitr;
  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    dbNet* net = *nitr;

    if (net->isMark_1ed()) {
      dbNet* origNet = srcBlock->findNet(net->getConstName());
      if (origNet != NULL)
        cnt += printEcoTerms(origNet, "ConnectITerm", fp);
    }
  }
  return cnt;
}
uint dbCreateNetUtil::printNewNets(dbBlock* ecoBlock,
                                   dbBlock* srcBlock,
                                   FILE* fp,
                                   int itermCnt)
{
  const char* connectIterm = "ConnectITerm";
  if (_milosFormat) {
    connectIterm = "milos_connectNet";
  }

  uint cnt = 0;
  dbSet<dbNet> nets = ecoBlock->getNets();
  dbSet<dbNet>::iterator nitr;
  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    dbNet* net = *nitr;

    if (net->isMark_1ed()) {
      if (itermCnt < 0) {
        cnt += printEcoNet(net, srcBlock, fp);
        continue;
      }
      dbNet* origNet = srcBlock->findNet(net->getConstName());
      if (origNet != NULL)
        cnt += printEcoTerms(origNet, connectIterm, fp);
    }
  }
  return cnt;
}
uint dbCreateNetUtil::printModifiedNets(dbBlock* ecoBlock,
                                        bool connectTerm,
                                        dbBlock* /* unused: srcBlock */,
                                        FILE* fp)
{
  const char* connectIterm = "ConnectITerm";
  const char* disconnectIterm = "DisconnectITerm";
  if (_milosFormat) {
    connectIterm = "milos_connectNet";
    disconnectIterm = "milos_disconnectNet";
  }

  uint cnt = 0;
  dbSet<dbNet> nets = ecoBlock->getNets();
  dbSet<dbNet>::iterator nitr;
  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    dbNet* net = *nitr;

    if (_skipPowerNets) {
      dbSigType ty = net->getSigType();
      if ((ty == dbSigType::POWER) || (ty == dbSigType::GROUND))
        continue;
    }

    if (!net->isDisconnected())
      continue;

    if (connectTerm)
      cnt += printModifiedNetECO(net, connectIterm, NULL, fp);
    else
      cnt += printModifiedNetECO(net, NULL, disconnectIterm, fp);
  }
  return cnt;
}
dbNet* dbCreateNetUtil::createNet(dbNet* nn, bool create, bool destroy)
{
  dbNet* net = dbNet::create(_block, nn->getConstName());

  dbSigType ty = nn->getSigType();
  if ((ty == dbSigType::POWER) && (ty == dbSigType::GROUND))
    net->setSpecial();

  net->setSigType(ty);

  if (create)
    net->setMark_1(true);
  else if (destroy)
    net->setMark(true);
  else
    net->setDisconnected(true);

  return net;
}
dbITerm* dbCreateNetUtil::updateITerm(dbITerm* iterm,
                                      bool /* unused: disconnect */)
{
  //	if ( !disconnect)
  //		return iterm;

  dbITerm* t = NULL;
  dbInst* ecoInst = _block->findInst(iterm->getInst()->getConstName());
  if (ecoInst != NULL) {
    t = ecoInst->findITerm(iterm->getMTerm()->getConstName());
    if ((t != NULL) && (t->getNet() != NULL))
      return t;
  }
  if (ecoInst == NULL)
    ecoInst = updateInst(iterm->getInst(), false, false);

  dbNet* ecoNet = updateNet(iterm->getNet(), false, false);

  dbITerm* ecoIterm = dbITerm::connect(ecoInst, ecoNet, iterm->getMTerm());

  return ecoIterm;
}
void dbCreateNetUtil::writeEco(dbBlock* ecoBlock,
                               dbBlock* srcBlock,
                               const char* fileName,
                               bool debug)
{
  char buff_name[1024];
  if (debug) {
    _milosFormat = false;
    sprintf(buff_name, "%s.debug.eco", fileName);
  } else {
    sprintf(buff_name, "%s.eco", fileName);
    _milosFormat = true;
  }

  FILE* fp = fopen(buff_name, "w");

  if (fp == NULL) {
    warning(0, "Cannot open file %s for writting\n", fileName);
    return;
  }
  /*
     uint n= printDeletedInsts(ecoBlock, srcBlock, fp);
     if (n>0)
         notice(0, "%d Deleted Instances\n", n);

     n= printNewInsts(ecoBlock, srcBlock, fp);
     if (n>0)
         notice(0, "%d New Instances\n", n);

 */
  fprintf(fp, "#Modified Instances\n");
  uint n = printModifiedInsts(ecoBlock, srcBlock, fp);
  if (n > 0)
    notice(0, "%d Modified Instances\n", n);

  fprintf(fp, "#Disconnected ITerms from Existing Nets\n");
  n = printModifiedNets(ecoBlock, false, srcBlock, fp);
  if (n > 0)
    notice(0, "%d Disconnected ITerms from Existing Nets\n", n);

  fprintf(fp, "#New Instances\n");
  n = printNewInsts(ecoBlock, srcBlock, fp);
  if (n > 0)
    notice(0, "%d New Instances\n", n);

  fprintf(fp, "#Connected ITerms from Existing Nets\n");
  n = printModifiedNets(ecoBlock, true, srcBlock, fp);
  if (n > 0)
    notice(0, "%d Connected ITerms from Existing Nets\n", n);

  fprintf(fp, "# Disconnected ITerms from Deleted Nets\n");
  uint deletedNetTermCnt = printDeletedNets(ecoBlock, srcBlock, fp, -1);
  fprintf(fp, "# Deleted Nets\n");
  n = printDeletedNets(ecoBlock, srcBlock, fp, 0);
  if (n > 0)
    notice(0, "%d Deleted Nets\n", n);
  if (deletedNetTermCnt > 0)
    notice(0, "%d Disconnected ITerms\n", deletedNetTermCnt);

  fprintf(fp, "#New Nets\n");
  n = printNewNets(ecoBlock, srcBlock, fp, -1);
  fprintf(fp, "# Connected ITerms from New Nets\n");
  uint newNetTermCnt = printNewNets(ecoBlock, srcBlock, fp, 0);
  if (n > 0)
    notice(0, "%d New Nets\n", n);
  if (newNetTermCnt > 0)
    notice(0, "%d Connected ITerms\n", newNetTermCnt);

  fprintf(fp, "#Deleted Instances\n");
  n = printDeletedInsts(ecoBlock, srcBlock, fp);
  if (n > 0)
    notice(0, "%d Deleted Instances\n", n);

  fclose(fp);
}
void dbCreateNetUtil::writeDetailedEco(dbBlock* ecoBlock,
                                       dbBlock* /* unused: srcBlock */,
                                       const char* fileName,
                                       bool /* unused: debug */)
{
  char buff_name[1024];
  _milosFormat = true;

  sprintf(buff_name, "%s.trace.eco", fileName);
  FILE* fp = fopen(buff_name, "w");

  if (fp == NULL) {
    warning(0, "Cannot open file %s for writting\n", fileName);
    return;
  }

  dbSet<dbInst> insts = ecoBlock->getInsts();
  dbSet<dbNet> nets = ecoBlock->getNets();

  uint maxInstEco = 0;
  dbSet<dbInst>::iterator iitr;
  for (iitr = insts.begin(); iitr != insts.end(); ++iitr) {
    dbInst* inst = *iitr;

    dbIntProperty* p = dbIntProperty::find(inst, "ECO");
    if (p == NULL)
      continue;
    uint n = p->getValue();
    if (maxInstEco < n)
      maxInstEco = n;
  }
  uint maxNetEco = 0;
  dbSet<dbNet>::iterator nitr;
  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    dbNet* net = *nitr;

    dbIntProperty* p = dbIntProperty::find(net, "ECO");
    if (p == NULL)
      continue;
    uint n = p->getValue();
    if (maxNetEco < n)
      maxNetEco = n;
  }

  uint maxSize = maxNetEco > maxInstEco ? maxNetEco : maxInstEco;
  Ath__array1D<dbObject*> transactionArray(2040);

  uint ii = 0;
  for (ii = 0; ii <= maxSize; ii++)
    transactionArray.set(ii, NULL);

  for (iitr = insts.begin(); iitr != insts.end(); ++iitr) {
    dbInst* inst = *iitr;

    dbIntProperty* p = dbIntProperty::find(inst, "ECO");
    if (p == NULL)
      continue;
    uint n = p->getValue();

    transactionArray.set(n, inst);
  }
  for (nitr = nets.begin(); nitr != nets.end(); ++nitr) {
    dbNet* net = *nitr;

    dbIntProperty* p = dbIntProperty::find(net, "ECO");
    if (p == NULL)
      continue;
    uint n = p->getValue();
    transactionArray.set(n, net);
  }
}
void dbCreateNetUtil::setBlock(dbBlock* block, bool skipInit)
{
  _block = block;
  if (skipInit)
    return;

  _tech = block->getDb()->getTech();
  _ruleNameHint = 0;
  _routingLayers.clear();
  _rules.clear();

  int layerCount = _tech->getRoutingLayerCount();
  _rules.resize(layerCount + 1);
  _routingLayers.resize(layerCount + 1);

  dbSet<dbTechLayer> layers = _tech->getLayers();
  dbSet<dbTechLayer>::iterator itr;

  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    dbTechLayer* layer = *itr;
    int rlevel = layer->getRoutingLevel();

    if (rlevel > 0)
      _routingLayers[rlevel] = layer;
  }

  // Build mapping table to rule widths
  dbSet<dbTechNonDefaultRule> nd_rules = _tech->getNonDefaultRules();
  dbSet<dbTechNonDefaultRule>::iterator nditr;
  // dbTechNonDefaultRule  *wdth_rule = NULL;

  for (nditr = nd_rules.begin(); nditr != nd_rules.end(); ++nditr) {
    dbTechNonDefaultRule* nd_rule = *nditr;
    std::vector<dbTechLayerRule*> layer_rules;
    nd_rule->getLayerRules(layer_rules);
    std::vector<dbTechLayerRule*>::iterator lritr;

    for (lritr = layer_rules.begin(); lritr != layer_rules.end(); ++lritr) {
      dbTechLayerRule* rule = *lritr;

      int rlevel = rule->getLayer()->getRoutingLevel();

      if (rlevel > 0) {
        dbTechLayerRule*& r = _rules[rlevel][rule->getWidth()];

        if (r == NULL)  // Don't overide any existing rule.
          r = rule;
      }
    }
  }

  _vias.clear();
  _vias.resize(layerCount + 1, layerCount + 1);

  dbSet<dbTechVia> vias = _tech->getVias();
  dbSet<dbTechVia>::iterator vitr;

  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    dbTechVia* via = *vitr;
    dbTechLayer* bot = via->getBottomLayer();
    dbTechLayer* top = via->getTopLayer();

    int topR = top->getRoutingLevel();
    int botR = bot->getRoutingLevel();

    if (topR == 0 || botR == 0)
      continue;

    _vias(botR, topR).push_back(via);
  }
}

dbTechLayerRule* dbCreateNetUtil::getRule(int routingLayer, int width)
{
  dbTechLayerRule*& rule = _rules[routingLayer][width];

  if (rule != NULL)
    return rule;

  // Create a non-default-rule for this width
  dbTechNonDefaultRule* nd_rule;
  char rule_name[64];

  while (_ruleNameHint >= 0) {
    snprintf(rule_name, 64, "ADS_ND_%d", _ruleNameHint++);
    nd_rule = dbTechNonDefaultRule::create(_tech, rule_name);

    if (nd_rule)
      break;
  }

  if (nd_rule == NULL)
    return NULL;
  rule->getImpl()->getLogger()->info(utl::ODB,
                                     273,
                                     "Create ND RULE {} for layer/width {},{}",
                                     rule_name,
                                     routingLayer,
                                     width);

  int i;
  for (i = 1; i <= _tech->getRoutingLayerCount(); i++) {
    dbTechLayer* layer = _routingLayers[i];

    if (layer != NULL) {
      dbTechLayerRule* lr = dbTechLayerRule::create(nd_rule, layer);
      lr->setWidth(width);
      lr->setSpacing(layer->getSpacing());

      dbTechLayerRule*& r = _rules[i][width];
      if (r == NULL)
        r = lr;
    }
  }

  // dbTechVia  *curly_via;
  dbSet<dbTechVia> all_vias = _tech->getVias();
  dbSet<dbTechVia>::iterator viter;
  std::string nd_via_name("");
  for (viter = all_vias.begin(); viter != all_vias.end(); ++viter) {
    if (((*viter)->getNonDefaultRule() == NULL) && ((*viter)->isDefault())) {
      nd_via_name = std::string(rule_name) + std::string("_")
                    + std::string((*viter)->getName().c_str());
      // curly_via = dbTechVia::clone(nd_rule, (*viter), nd_via_name.c_str());
    }
  }

  assert(rule != NULL);
  return rule;
}

dbTechVia* dbCreateNetUtil::getVia(int l1, int l2, Rect& bbox)
{
  int bot, top;

  if (l1 < l2) {
    bot = l1;
    top = l2;
  } else {
    bot = l2;
    top = l1;
  }

  uint dx = bbox.dx();
  uint dy = bbox.dy();

  dbTechVia* def = NULL;
  std::vector<dbTechVia*>& vias = _vias(bot, top);
  std::vector<dbTechVia*>::iterator itr;

  for (itr = vias.begin(); itr != vias.end(); ++itr) {
    dbTechVia* via = *itr;

    if (via->isDefault())
      def = via;

    dbBox* bbox = via->getBBox();
    uint vdx = bbox->getDX();
    uint vdy = bbox->getDY();

    if (vdx == dx && vdy == dy)  // This is a guess!
      return via;                // There's no way to determine
                                 // the via from the placed bbox.
    // There could be multiple vias with the same width and
    // height
  }

  return def;
}

dbNet* dbCreateNetUtil::createNetSingleWire(const char* netName,
                                            int x1,
                                            int y1,
                                            int x2,
                                            int y2,
                                            int routingLayer,
                                            dbTechLayerDir dir,
                                            bool skipBterms)
{
  if (dir == dbTechLayerDir::NONE)
    return createNetSingleWire(
        netName, x1, y1, x2, y2, routingLayer, dir, skipBterms);

  if ((netName == NULL) || (routingLayer < 1)
      || (routingLayer > _tech->getRoutingLayerCount())) {
    if (netName == NULL)
      warning(0, "Cannot create wire, because net name is NULL\n");
    else
      warning(0,
              "Cannot create wire, because routing layer (%d) is invalid\n",
              routingLayer);

    return NULL;
  }

  Rect r(x1, y1, x2, y2);
  int width;
  Point p0, p1;

  if (dir == dbTechLayerDir::VERTICAL) {
    uint dx = r.dx();

    // This is dangerous!
    if (dx & 1) {
      r = Rect(x1, y1, x2 + 1, y2);
      dx = r.dx();
    }

    width = (int) dx;
    int dw = dx / 2;
    p0.x() = r.xMin() + dw;
    p0.y() = r.yMin();
    p1.x() = r.xMax() - dw;
    p1.y() = r.yMax();
  } else {
    uint dy = r.dy();

    // This is dangerous!
    if (dy & 1) {
      r = Rect(x1, y1, x2, y2 + 1);
      dy = r.dy();
    }

    width = (int) dy;
    int dw = dy / 2;
    p0.y() = r.xMin();
    p0.x() = r.yMin() + dw;
    p1.y() = r.xMax();
    p1.x() = r.yMax() - dw;
  }

  dbTechLayer* layer = _routingLayers[routingLayer];
  int minWidth = layer->getWidth();

  if (width < (int) minWidth) {
    std::string ln = layer->getName();
    warning(0,
            "Cannot create net %s, because wire width (%d) is lessthan "
            "minWidth (%d) on layer %s\n",
            netName,
            width,
            minWidth,
            ln.c_str());
    return NULL;
  }

  dbNet* net = dbNet::create(_block, netName);

  if (net == NULL)
    return NULL;

  net->setSigType(dbSigType::SIGNAL);

  std::pair<dbBTerm*, dbBTerm*> blutrms;

  if (!skipBterms) {
    blutrms = createTerms4SingleNet(
        net, r.xMin(), r.yMin(), r.xMax(), r.yMax(), layer);

    if ((blutrms.first == NULL) || (blutrms.second == NULL)) {
      dbNet::destroy(net);
      warning(0,
              "Cannot create net %s, because failed to create bterms\n",
              netName);
      return NULL;
    }
  }

  dbTechLayerRule* rule = NULL;
  if ((int) layer->getWidth() != width)
    rule = getRule(routingLayer, width);

  dbWireEncoder encoder;
  encoder.begin(dbWire::create(net));

  if (rule == NULL)
    encoder.newPath(layer, dbWireType::ROUTED);
  else
    encoder.newPath(layer, dbWireType::ROUTED, rule);

  encoder.addPoint(p0.x(), p0.y(), 0);

  if (!skipBterms)
    encoder.addBTerm(blutrms.first);

  encoder.addPoint(p1.x(), p1.y(), 0);

  if (!skipBterms)
    encoder.addBTerm(blutrms.second);

  encoder.end();

  return net;
}
dbSBox* dbCreateNetUtil::createSpecialWire(dbNet* mainNet,
                                           Rect& r,
                                           dbTechLayer* layer,
                                           uint /* unused: sboxId */)
{
  dbSWire* swire = NULL;
  if (mainNet == NULL)
    swire = _currentNet->getFirstSWire();
  else
    swire = mainNet->getFirstSWire();

  return dbSBox::create(swire,
                        layer,
                        r.xMin(),
                        r.yMin(),
                        r.xMax(),
                        r.yMax(),
                        dbWireShapeType::NONE);

  // MIGHT NOT care abour sboxId!!
}
dbNet* dbCreateNetUtil::createSpecialNet(dbNet* origNet, const char* name)
{
  dbNet* net = _currentNet;

  char netName[128];

  if (_currentNet == NULL) {
    if (name == NULL)
      sprintf(netName, "S%d", origNet->getId());
    else
      sprintf(netName, "%s", name);

    net = _block->findNet(netName);
  }
  if (net == NULL) {
    net = dbNet::create(_block, netName);
    if ((origNet != NULL)
        && ((origNet->getSigType() == dbSigType::POWER)
            || (origNet->getSigType() == dbSigType::GROUND)))
      net->setSigType(origNet->getSigType());
    else
      net->setSigType(dbSigType::POWER);

    dbSWire::create(net, dbWireType::NONE, NULL);

    if ((_mapArray != NULL) && (name == NULL))
      _mapArray[origNet->getId()] = net;
    _currentNet = net;
  }
  return net;
}

dbNet* dbCreateNetUtil::createSpecialNetSingleWire(Rect& r,
                                                   dbTechLayer* layer,
                                                   dbNet* origNet,
                                                   uint /* unused: sboxId */)
{
  char netName[128];
  sprintf(netName, "S%d", origNet->getId());

  dbSWire* swire = NULL;
  dbNet* net = _block->findNet(netName);
  if (net == NULL) {
    net = dbNet::create(_block, netName);
    if ((origNet->getSigType() == dbSigType::POWER)
        || (origNet->getSigType() == dbSigType::GROUND))
      net->setSigType(origNet->getSigType());
    else
      net->setSigType(dbSigType::POWER);

    swire = dbSWire::create(net, dbWireType::NONE, NULL);
  } else {
    swire = net->getFirstSWire();
  }
  dbSBox::create(swire,
                 layer,
                 r.xMin(),
                 r.yMin(),
                 r.xMax(),
                 r.yMax(),
                 dbWireShapeType::NONE);

  return net;

  // MIGHT NOT care abour sboxId!!
}
uint dbCreateNetUtil::getFirstShape(dbNet* net, dbShape& s)
{
  dbWirePath path;
  dbWirePathShape pshape;

  dbWirePathItr pitr;
  dbWire* wire = net->getWire();

  uint status = 0;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    pitr.getNextShape(pshape);
    s = pshape.shape;
    status = pshape.junction_id;
    break;
  }
  return status;
}
bool dbCreateNetUtil::setFirstShapeProperty(dbNet* net, uint prop)
{
  if (net == NULL)
    return false;

  dbShape s;
  uint jid = getFirstShape(net, s);
  net->getWire()->setProperty(jid, prop);

  return true;
}
dbNet* dbCreateNetUtil::copyNet(dbNet* net,
                                bool copyVias,
                                char* name,
                                bool removeITermsBTerms)
{
  dbNet* newNet = NULL;

  if (name != NULL) {
    newNet = dbNet::create(_block, name);
  } else {
    char netName[128];
    sprintf(netName, "N%d", net->getId());
    newNet = dbNet::create(_block, netName, true);
  }
  dbWire* wire = dbWire::create(newNet);

  dbWire::copy(wire, net->getWire(), removeITermsBTerms, copyVias);

  return newNet;
}

dbNet* dbCreateNetUtil::createNetSingleWire(Rect& r,
                                            uint level,
                                            uint netId,
                                            uint shapeId)
{
  // bool skipBterms= false;
  char netName[128];

  if (_currentNet == NULL) {
    sprintf(netName, "N%d", netId);

    dbShape s;
    dbNet* newNet = createNetSingleWire(netName,
                                        r.xMin(),
                                        r.yMin(),
                                        r.xMax(),
                                        r.yMax(),
                                        level,
                                        true /*skipBterms*/,
                                        true);

    if (shapeId > 0)
      setFirstShapeProperty(newNet, shapeId);

    _currentNet = newNet;
    if (_mapArray != NULL)
      _mapArray[netId] = _currentNet;
    return newNet;
  } else {
    sprintf(netName, "N%d_%d", netId, shapeId);
    dbNet* newNet = createNetSingleWire(netName,
                                        r.xMin(),
                                        r.yMin(),
                                        r.xMax(),
                                        r.yMax(),
                                        level,
                                        true /*skipBterms*/,
                                        true);

    if (newNet != NULL) {
      if (shapeId > 0)
        setFirstShapeProperty(newNet, shapeId);

      _currentNet->getWire()->append(newNet->getWire(), true);
      dbNet::destroy(newNet);
    }
    return newNet;
  }
}

dbNet* dbCreateNetUtil::createNetSingleWire(const char* netName,
                                            int x1,
                                            int y1,
                                            int x2,
                                            int y2,
                                            int routingLayer,
                                            bool skipBterms,
                                            bool skipExistsNet)
{
  if ((netName == NULL) || (routingLayer < 1)
      || (routingLayer > _tech->getRoutingLayerCount())) {
    if (netName == NULL)
      warning(0, "Cannot create wire, because net name is NULL\n");
    else
      warning(0,
              "Cannot create wire, because routing layer (%d) is invalid\n",
              routingLayer);

    return NULL;
  }

  dbTechLayer* layer = _routingLayers[routingLayer];
  Rect r(x1, y1, x2, y2);
  uint dx = r.dx();
  uint dy = r.dy();

  // This is dangerous!
  if ((dx & 1) && (dy & 1)) {
    r = Rect(x1, y1, x2, y2 + 1);
    dx = r.dx();
    dy = r.dy();
  }

  uint width;
  Point p0, p1;
  uint minWidth = layer->getWidth();
  bool make_vertical = false;

  if (((dx & 1) == 0) && ((dy & 1) == 1))  // dx == even & dy == odd
  {
    make_vertical = true;
  } else if (((dx & 1) == 1) && ((dy & 1) == 0))  // dx == odd & dy == even
  {
    make_vertical = false;
  } else if ((dx < minWidth) && (dy < minWidth)) {
    if (dx > dy)
      make_vertical = true;
    else if (dx < dy)
      make_vertical = false;
    else
      make_vertical = true;
  } else if (dx == minWidth) {
    make_vertical = true;
  } else if (dy == minWidth) {
    make_vertical = false;
  } else if (dx < minWidth) {
    make_vertical = false;
  } else if (dy < minWidth) {
    make_vertical = true;
  } else if (dx < dy) {
    make_vertical = true;
  } else if (dx > dy) {
    make_vertical = false;
  } else  // square (make vertical)
  {
    make_vertical = true;
  }

  if (make_vertical) {
    width = dx;
    int dw = width / 2;
    p0.x() = r.xMin() + dw;
    p0.y() = r.yMin();
    p1.x() = r.xMax() - dw;
    p1.y() = r.yMax();
  } else {
    width = dy;
    int dw = width / 2;
    p0.x() = r.xMin();
    p0.y() = r.yMin() + dw;
    p1.x() = r.xMax();
    p1.y() = r.yMax() - dw;
  }

  dbNet* net = dbNet::create(_block, netName, skipExistsNet);

  if (net == NULL) {
    warning(0, "Cannot create net %s, duplicate net\n", netName);
    return NULL;
  }

  net->setSigType(dbSigType::SIGNAL);

  std::pair<dbBTerm*, dbBTerm*> blutrms;

  if (!skipBterms) {
    blutrms = createTerms4SingleNet(
        net, r.xMin(), r.yMin(), r.xMax(), r.yMax(), layer);

    if ((blutrms.first == NULL) || (blutrms.second == NULL)) {
      dbNet::destroy(net);
      warning(0,
              "Cannot create net %s, because failed to create bterms\n",
              netName);
      return NULL;
    }
  }

  dbTechLayerRule* rule = NULL;
  if (layer->getWidth() != width)
    rule = getRule(routingLayer, width);

  dbWireEncoder encoder;
  encoder.begin(dbWire::create(net));

  if (rule == NULL)
    encoder.newPath(layer, dbWireType::ROUTED);
  else
    encoder.newPath(layer, dbWireType::ROUTED, rule);

  encoder.addPoint(p0.x(), p0.y(), 0);

  if (!skipBterms)
    encoder.addBTerm(blutrms.first);

  encoder.addPoint(p1.x(), p1.y(), 0);

  if (!skipBterms)
    encoder.addBTerm(blutrms.second);

  encoder.end();

  return net;
}

std::pair<dbBTerm*, dbBTerm*> dbCreateNetUtil::createTerms4SingleNet(
    dbNet* net,
    int x1,
    int y1,
    int x2,
    int y2,
    dbTechLayer* inly)
{
  std::pair<dbBTerm*, dbBTerm*> retpr;
  retpr.first = NULL;
  retpr.second = NULL;

  std::string term_str(net->getName());
  term_str = term_str + "_BL";
  dbBTerm* blterm = dbBTerm::create(net, term_str.c_str());

  uint dx = x2 - x1;
  uint dy = y2 - y1;
  uint fwidth = dx < dy ? dx : dy;
  uint hwidth = fwidth / 2;
  if (!blterm)
    return retpr;

  term_str = net->getName();
  term_str = term_str + "_BU";
  dbBTerm* buterm = dbBTerm::create(net, term_str.c_str());

  if (!buterm) {
    dbBTerm::destroy(blterm);
    return retpr;
  }

  // TWG: Added bpins
  dbBPin* blpin = dbBPin::create(blterm);
  dbBPin* bupin = dbBPin::create(buterm);

  if (dx == fwidth) {
    int x = x1 + hwidth;
    dbBox::create(
        blpin, inly, -hwidth + x, -hwidth + y1, hwidth + x, hwidth + y1);
    dbBox::create(
        bupin, inly, -hwidth + x, -hwidth + y2, hwidth + x, hwidth + y2);
  } else {
    int y = y1 + hwidth;
    dbBox::create(
        blpin, inly, -hwidth + x1, -hwidth + y, hwidth + x1, hwidth + y);
    dbBox::create(
        bupin, inly, -hwidth + x2, -hwidth + y, hwidth + x2, hwidth + y);
  }

  blterm->setSigType(dbSigType::SIGNAL);
  buterm->setSigType(dbSigType::SIGNAL);
  blterm->setIoType(dbIoType::INPUT);
  buterm->setIoType(dbIoType::OUTPUT);
  blpin->setPlacementStatus(dbPlacementStatus::PLACED);
  bupin->setPlacementStatus(dbPlacementStatus::PLACED);

  retpr.first = blterm;
  retpr.second = buterm;
  return retpr;
}

dbNet* dbCreateNetUtil::createNetSingleVia(const char* netName,
                                           int x1,
                                           int y1,
                                           int x2,
                                           int y2,
                                           int lay1,
                                           int lay2)
{
  if (netName == NULL) {
    warning(0, "Cannot create wire, because net name is NULL\n");
    return NULL;
  }

  dbNet* net = dbNet::create(_block, netName);

  if (net == NULL) {
    warning(0, "Cannot create net %s, duplicate net\n", netName);
    return NULL;
  }

  net->setSigType(dbSigType::SIGNAL);

  if (!createSingleVia(net, x1, y1, x2, y2, lay1, lay2)) {
    dbNet::destroy(net);
    return NULL;
  }

  return net;
}

dbBox* dbCreateNetUtil::createTechVia(int x1,
                                      int y1,
                                      int x2,
                                      int y2,
                                      int lay1,
                                      int lay2)
{
  if ((lay1 < 1) || (lay1 > _tech->getRoutingLayerCount())) {
    warning(
        0, "Cannot create wire, because routing layer (%d) is invalid\n", lay1);
    return NULL;
  }

  if ((lay2 < 1) || (lay2 > _tech->getRoutingLayerCount())) {
    warning(
        0, "Cannot create wire, because routing layer (%d) is invalid\n", lay2);
    return NULL;
  }

  Rect r(x1, y1, x2, y2);
  dbTechVia* via = getVia(lay1, lay2, r);
  dbBox* b = via->getBBox();
  // int xmin = b->xMin();
  // int ymin = b->yMin();
  // int x = r.xMin() - xmin;
  // int y = r.yMin() - ymin;

  return b;
}
bool dbCreateNetUtil::createSingleVia(dbNet* net,
                                      int x1,
                                      int y1,
                                      int x2,
                                      int y2,
                                      int lay1,
                                      int lay2)
{
  if ((lay1 < 1) || (lay1 > _tech->getRoutingLayerCount())) {
    warning(
        0, "Cannot create wire, because routing layer (%d) is invalid\n", lay1);
    return false;
  }

  if ((lay2 < 1) || (lay2 > _tech->getRoutingLayerCount())) {
    warning(
        0, "Cannot create wire, because routing layer (%d) is invalid\n", lay2);
    return false;
  }

  Rect r(x1, y1, x2, y2);
  dbTechVia* via = getVia(lay1, lay2, r);
  dbBox* b = via->getBBox();
  int xmin = b->xMin();
  int ymin = b->yMin();
  int x = r.xMin() - xmin;
  int y = r.yMin() - ymin;

  dbWire* wire = net->getWire();
  dbWireEncoder encoder;

  if (wire == NULL) {
    wire = dbWire::create(net);
    encoder.append(wire);
  } else {
    encoder.begin(wire);
  }

  encoder.newPath(via->getBottomLayer(), dbWireType::ROUTED);
  encoder.addPoint(x, y);
  encoder.addTechVia(via);
  encoder.end();
  return true;
}

}  // namespace odb
