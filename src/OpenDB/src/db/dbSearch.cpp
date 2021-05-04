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

#include "dbSearch.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTypes.h"

//#define NEW_TRACKS

#include "dbShape.h"
namespace odb {

dbBlockSearch::dbBlockSearch(dbBlock* blk, dbTech* tech)
{
  _block = blk;
  _tech = tech;
  _blockId = blk->getId();

  _signalNetSdb = NULL;
  _netSdb = NULL;
  _netViaSdb = NULL;
  _instSdb = NULL;
  _trackSdb = NULL;

  initMenuIds();

  _skipCutBoxes = false;
}
dbBlockSearch::~dbBlockSearch()
{
  if (_signalNetSdb != NULL)
    delete _signalNetSdb;
  if (_netSdb != NULL)
    delete _netSdb;
  if (_instSdb != NULL)
    delete _instSdb;
  if (_netViaSdb != NULL)
    delete _netViaSdb;
  if (_trackSdb != NULL)
    delete _trackSdb;
}

void dbBlockSearch::initMenuIds()
{
  _blockMenuId = 0;

  _block_bb_id = 1;  // ids are required by SDB in the absence of GUI
  _block_pin_id = 2;
  _block_obs_id = 3;
  _block_track_id = 4;

  _instMenuId = 0;

  _inst_bb_id = 5;
  _inst_pin_id = 6;
  _inst_obs_id = 7;

  _signalMenuId = 0;

  _signal_wire_id = 9;
  _signal_via_id = 10;

  _powerMenuId = 0;

  _power_wire_id = 11;
  _power_via_id = 12;
}
uint dbBlockSearch::getBbox(int* x1, int* y1, int* x2, int* y2)
{
  Rect rect;
  _block->getDieArea(rect);
  *x1 = rect.xMin();
  *y1 = rect.yMin();
  *x2 = rect.xMax();
  *y2 = rect.yMax();
  return _block->getId();
}
void dbBlockSearch::setViaCutsFlag(bool skipViaCuts)
{
  _skipCutBoxes = skipViaCuts;
}
void dbBlockSearch::makeSearchDB(bool nets, bool insts, ZContext& context)
{
  //_dbBlock * block = (_dbBlock *) this;
  if (insts) {
    if (adsNewComponent(context, ZCID(Sdb), _instSdb) != Z_OK) {
      assert(0);
    }
    makeInstSearchDb();
  }
  if (nets) {
    if (!_netSdb)
      makeNetSdb(context);
    if (!_netViaSdb)
      makeNetViaSdb(context);
#ifndef NEW_TRACKS
    if (!_trackSdb)
      makeTrackSdb(context);
#endif
  }
}
#ifndef NEW_TRACKS
void dbBlockSearch::makeTrackSdb(ZContext& context)
{
  if (adsNewComponent(context, ZCID(Sdb), _trackSdb) != Z_OK) {
    assert(0);
  }
  Rect r;
  _trackSdb->initSearchForNets(_tech, _block);
  _block->getDieArea(r);
  _trackSdb->setMaxArea(r.xMin(), r.yMin(), r.xMax(), r.yMax());
  makeTrackSearchDb();
}
#endif
void dbBlockSearch::makeNetSdb(ZContext& context)
{
  if (adsNewComponent(context, ZCID(Sdb), _netSdb) != Z_OK) {
    assert(0);
  }
  Rect r;
  _netSdb->initSearchForNets(_tech, _block);
  _block->getDieArea(r);
  _netSdb->setMaxArea(r.xMin(), r.yMin(), r.xMax(), r.yMax());
  _netSdb->addPowerNets(_block, _power_wire_id, true);
  _netSdb->addSignalNets(_block, _signal_wire_id, true);

  //	if ( adsNewComponent( context, ZCID(Sdb), _trackSdb )  != Z_OK )
  //    {
  //        assert(0);
  //    }
  //    _trackSdb->initSearchForNets(_tech, _block);
  //    _trackSdb->setMaxArea(r.xMin(), r.yMin(), r.xMax(), r.yMax());
  //
  //	makeTrackSearchDb();
}
void dbBlockSearch::makeNetViaSdb(ZContext& context)
{
  //_dbBlock * block = (_dbBlock *) this;
  if (adsNewComponent(context, ZCID(Sdb), _netViaSdb) != Z_OK) {
    assert(0);
  }
  _netViaSdb->initSearchForNets(_tech, _block);
  _netViaSdb->addPowerNets(_block, _power_via_id, false);
  //    _netViaSdb->addSignalNets(_block, _signal_via_id, false);
}

void dbBlockSearch::makeSignalNetSdb(ZContext& context)
{
  if (adsNewComponent(context, ZCID(Sdb), _signalNetSdb) != Z_OK) {
    assert(0);
  }
  Rect r;
  _signalNetSdb->initSearchForNets(_tech, _block);
  _block->getDieArea(r);
  _signalNetSdb->setMaxArea(r.xMin(), r.yMin(), r.xMax(), r.yMax());
  _signalNetSdb->addSignalNets(_block, _signal_wire_id, _signal_via_id);
}
ZPtr<ISdb> dbBlockSearch::getSignalNetSdb(ZContext& context)
{
  if (!_signalNetSdb)
    makeSignalNetSdb(context);
  return _signalNetSdb;
}
ZPtr<ISdb> dbBlockSearch::getSignalNetSdb()
{
  return _signalNetSdb;
}
void dbBlockSearch::resetSignalNetSdb()
{
  _signalNetSdb = NULL;
}
ZPtr<ISdb> dbBlockSearch::getNetSdb(ZContext& context)
{
  if (!_netSdb)
    makeNetSdb(context);
  return _netSdb;
}
ZPtr<ISdb> dbBlockSearch::getNetSdb()
{
  return _netSdb;
}
void dbBlockSearch::resetNetSdb()
{
  _netSdb = NULL;
}
uint dbBlockSearch::makeInstSearchDb()
{
  uint maxInt = 2100000000;
  uint minWidth = maxInt;
  uint minHeight = maxInt;

  dbSet<dbInst> insts = _block->getInsts();

  // dbBox *maxBox= _block->getBBox();
  Rect maxRect;
  _block->getDieArea(maxRect);

  maxRect.reset(maxInt, maxInt, -maxInt, -maxInt);
  // maxBox->getBox(maxRect);

  dbSet<dbInst>::iterator inst_itr;
  uint instCnt = 0;
  for (inst_itr = insts.begin(); inst_itr != insts.end(); ++inst_itr) {
    dbInst* inst = *inst_itr;

    if (inst->getMaster()->getType() == dbMasterType::CORE_FEEDTHRU)
      continue;
    if (inst->getMaster()->getMTermCount() <= 0)
      continue;

    dbBox* bb = inst->getBBox();

    minWidth = MIN(minWidth, bb->getDX());
    minHeight = MIN(minHeight, bb->getDY());

    Rect r;
    bb->getBox(r);
    maxRect.merge(r);
    instCnt++;
  }

  //_dbBlock * block = (_dbBlock *) this;

  if (instCnt == 0)
    return 0;

  // TODO    uint rowSize= 100 * minHeight;
  // TODO    uint colSize= 100 * minWidth;

  _instSdb->setupForBoxes(maxRect, minHeight, minWidth);

  for (inst_itr = insts.begin(); inst_itr != insts.end(); ++inst_itr) {
    dbInst* inst = *inst_itr;
    /*
if (inst->getMaster()->getMTermCount()<=0)
continue;
    */

    dbBox* ibox = inst->getBBox();

    //		_inst->addBox(x1, y1, x2, y2,
    //		level, rc->getId(), pshape.junction_id, _dbSignalId);

    _instSdb->addBox(ibox, _inst_bb_id, inst->getId());
  }

  uint start = 0;
  uint end = 0;
  // uint adjustedMarkerCnt= _instSdb->setExtrusionMarker(start, end);
  _instSdb->setExtrusionMarker(start, end);
  return instCnt;
}
#ifndef NEW_TRACKS
uint dbBlockSearch::makeTrackSearchDb()
{
  Rect maxRect;
  _block->getDieArea(maxRect);

  dbSet<dbTechLayer> layers = _tech->getLayers();
  dbSet<dbTechLayer>::iterator itr;

  uint cnt = 0;

  dbSet<dbTechLayer>::iterator litr;
  dbTechLayer* layer;
  for (litr = layers.begin(); litr != layers.end(); ++litr) {
    layer = *litr;
    if (layer->getType() != dbTechLayerType::ROUTING)
      continue;

    dbTrackGrid* g = _block->findTrackGrid(layer);
    if (g == NULL)
      continue;

    uint level = layer->getRoutingLevel();

    int lo[2] = {maxRect.xMin(), maxRect.yMin()};
    int hi[2] = {maxRect.xMax(), maxRect.yMax()};

    cnt += addTracks(g, 1, level, lo, hi);

    lo[0] = maxRect.xMin();
    lo[1] = maxRect.yMin();
    hi[0] = maxRect.xMax();
    hi[1] = maxRect.yMax();

    cnt += addTracks(g, 0, level, lo, hi);
  }
  return cnt;
}
#endif
#ifndef NEW_TRACKS
uint dbBlockSearch::addTracks(dbTrackGrid* g,
                              uint dir,
                              uint level,
                              int lo[2],
                              int hi[2])
{
  std::vector<int> trackXY(32000);

  if (dir > 0)  // horizontla
    g->getGridY(trackXY);
  else
    g->getGridX(trackXY);

  uint cnt = 0;
  for (uint ii = 0; ii < trackXY.size(); ii++) {
    int xy = trackXY[ii];
    lo[dir] = xy;
    hi[dir] = xy + 1;
    cnt++;

    Ath__searchBox bb;
    bb.set(lo[0], lo[1], hi[0], hi[1], level, -1);
    bb.setOwnerId(xy, 0);

    _trackSdb->addBox(
        lo[0], lo[1], hi[0], hi[1], level, g->getId(), ii, _block_track_id);

    cnt++;
  }
  return cnt;
}
#endif

uint dbBlockSearch::getViaLevel(dbSBox* s)
{
  dbTechVia* via = s->getTechVia();
  if (via != NULL) {
    return via->getTopLayer()->getRoutingLevel();
    /*
    dbTechLayer * getTopLayer();
    dbTechLayer * getBottomLayer();
    */
  } else {
    dbVia* via = s->getBlockVia();
    return via->getTopLayer()->getRoutingLevel();
  }
}
uint dbBlockSearch::getViaLevel(dbShape* s)
{
  dbTechVia* via = s->getTechVia();
  if (via != NULL) {
    return via->getTopLayer()->getRoutingLevel();
    /*
    dbTechLayer * getTopLayer();
    dbTechLayer * getBottomLayer();
    */
  } else {
    dbVia* via = s->getVia();
    return via->getTopLayer()->getRoutingLevel();
  }
}

uint dbBlockSearch::getBlockObs(bool ignoreFlags)
{
  if (!ignoreFlags && !_dcr->getSubMenuFlag(_blockMenuId, _block_obs_id))
    return 0;

  dbSet<dbObstruction> obstructions = _block->getObstructions();
  dbSet<dbBlockage> blockages = _block->getBlockages();

  int bcnt = obstructions.size() + blockages.size();

  if (bcnt == 0)
    return 0;

  uint cnt = 0;
  dbSet<dbObstruction>::iterator obs_itr;

  for (obs_itr = obstructions.begin(); obs_itr != obstructions.end();
       ++obs_itr) {
    dbObstruction* obs = *obs_itr;

    // dbInst * inst = obs->getInstance();
    // if ( obs->isSlotObstruction() )
    // if ( obs->isFillObstruction() )
    // if ( obs->isPushedDown() )

    dbBox* s = obs->getBBox();
    uint level = s->getTechLayer()->getRoutingLevel();
    cnt += _dcr->addBox(s->getId(),
                        _block_obs_id,
                        _blockMenuId,
                        level,
                        s->xMin(),
                        s->yMin(),
                        s->xMax(),
                        s->yMax(),
                        0);
  }

  dbSet<dbBlockage>::iterator blockage_itr;

  for (blockage_itr = blockages.begin(); blockage_itr != blockages.end();
       ++blockage_itr) {
    dbBlockage* obs = *blockage_itr;

    // dbInst * inst = obs->getInstance();
    // if ( obs->isSlotObstruction() )
    // if ( obs->isFillObstruction() )
    // if ( obs->isPushedDown() )

    dbBox* s = obs->getBBox();
    uint level = 0;
    if (s->getTechLayer() != NULL)
      level = s->getTechLayer()->getRoutingLevel();

    cnt += _dcr->addBox(s->getId(),
                        _block_obs_id,
                        _blockMenuId,
                        level,
                        s->xMin(),
                        s->yMin(),
                        s->xMax(),
                        s->yMax(),
                        0);
  }

  return cnt;
}

uint dbBlockSearch::addInstBox(dbInst* inst)
{
  dbBox* s = inst->getBBox();

  return _dcr->addBox(inst->getId(),
                      _inst_bb_id,
                      _instMenuId,
                      0,
                      s->xMin(),
                      s->yMin(),
                      s->xMax(),
                      s->yMax(),
                      0);
}
uint dbBlockSearch::addBoxes(dbBTerm* bterm)
{
  uint cnt = 0;
  dbSet<dbBPin> bpins = bterm->getBPins();
  dbSet<dbBPin>::iterator itr;

  // TWG: Added bpins
  for (itr = bpins.begin(); itr != bpins.end(); ++itr) {
    dbBPin* bpin = *itr;

    for (dbBox* box : bpin->getBoxes()) {
      cnt += _dcr->addBox(box->getId(),
                          _block_pin_id,
                          _blockMenuId,
                          box->getTechLayer()->getRoutingLevel(),
                          box->xMin(),
                          box->yMin(),
                          box->xMax(),
                          box->yMax(),
                          0);
    }
  }

  return cnt;
}
uint dbBlockSearch::addBtermBoxes(dbSet<dbBTerm>& bterms, bool ignoreFlag)
{
  if (!(ignoreFlag || _dcr->getSubMenuFlag(_blockMenuId, _block_pin_id)))
    return 0;

  uint cnt = 0;
  dbSet<dbBTerm>::iterator itr;
  for (itr = bterms.begin(); itr != bterms.end(); ++itr) {
    dbBTerm* bterm = *itr;

    cnt += addBoxes(bterm);
  }
  return cnt;
}
uint dbBlockSearch::getFirstShape(dbITerm* iterm,
                                  bool viaFlag,
                                  int& x1,
                                  int& y1,
                                  int& x2,
                                  int& y2)
{
  // dbInst *inst= iterm->getInst();
  // uint cnt= 0;

  dbITermShapeItr term_shapes;

  dbShape s;
  for (term_shapes.begin(iterm); term_shapes.next(s);) {
    if (s.isVia()) {
      if (viaFlag)
        continue;
    } else {
      x1 = s.xMin();
      y1 = s.yMin();
      x2 = s.xMax();
      y2 = s.yMax();
      return s.getTechLayer()->getRoutingLevel();
      break;
    }
  }
  return 0;
}
uint dbBlockSearch::getItermShapesWithViaShapes(dbITerm* iterm)
{
  const char* tcut = "tcut";
  const char* bcut = "bcut";

  // dbInst *inst= iterm->getInst();
  uint cnt = 0;
  uint itermId = iterm->getId();

  dbITermShapeItr term_shapes(true);

  uint shapeId = 0;

  dbShape s;
  for (term_shapes.begin(iterm); term_shapes.next(s);) {
    if (s.isViaBox()) {
      int x1 = s.xMin();
      int y1 = s.yMin();
      int x2 = s.xMax();
      int y2 = s.yMax();

      if (s.getTechLayer()->getType() != dbTechLayerType::CUT) {
        uint level = s.getTechLayer()->getRoutingLevel();

        cnt += _dcr->addBox(
            itermId, _inst_pin_id, _instMenuId, level, x1, y1, x2, y2, shapeId);
      } else {
        dbTechVia* via = s.getTechVia();
        uint topLevel = via->getTopLayer()->getRoutingLevel();
        uint botLevel = via->getBottomLayer()->getRoutingLevel();

        cnt += _dcr->addBox(itermId,
                            _inst_pin_id,
                            _instMenuId,
                            topLevel,
                            x1,
                            y1,
                            x2,
                            y2,
                            shapeId,
                            tcut);
        cnt += _dcr->addBox(itermId,
                            _inst_pin_id,
                            _instMenuId,
                            botLevel,
                            x1,
                            y1,
                            x2,
                            y2,
                            shapeId,
                            bcut);
      }
    } else {
      cnt += _dcr->addBox(iterm->getId(),
                          _inst_pin_id,
                          _instMenuId,
                          s.getTechLayer()->getRoutingLevel(),
                          s.xMin(),
                          s.yMin(),
                          s.xMax(),
                          s.yMax(),
                          0);
    }
  }
  return cnt;
}
uint dbBlockSearch::getItermShapesNoVias(dbITerm* iterm)
{
  // dbInst *inst= iterm->getInst();
  uint cnt = 0;
  // uint itermId= iterm->getId();

  dbITermShapeItr term_shapes;

  dbShape s;
  for (term_shapes.begin(iterm); term_shapes.next(s);) {
    if (s.isVia()) {
      continue;
    } else {
      cnt += _dcr->addBox(iterm->getId(),
                          _inst_pin_id,
                          _instMenuId,
                          s.getTechLayer()->getRoutingLevel(),
                          s.xMin(),
                          s.yMin(),
                          s.xMax(),
                          s.yMax(),
                          0);
    }
  }
  return cnt;
}
uint dbBlockSearch::getItermShapes(dbInst* inst, bool viaFlag)
{
  uint cnt = 0;
  dbSet<dbITerm> iterms = inst->getITerms();

  dbSet<dbITerm>::iterator iterm_itr;

  for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr) {
    dbITerm* iterm = *iterm_itr;
    if (viaFlag)
      cnt += getItermShapesWithViaShapes(iterm);
    else
      cnt += getItermShapesNoVias(iterm);
  }
  return cnt;
}

uint dbBlockSearch::getFirstInstObsShape(dbInst* inst,
                                         bool viaFlag,
                                         int& x1,
                                         int& y1,
                                         int& x2,
                                         int& y2)
{
  // uint cnt= 0;

  dbInstShapeItr obs_shapes;

  dbShape s;
  for (obs_shapes.begin(inst, dbInstShapeItr::OBSTRUCTIONS);
       obs_shapes.next(s);) {
    if (s.isVia()) {
      if (viaFlag)
        continue;
    } else {
      x1 = s.xMin();
      y1 = s.yMin();
      x2 = s.xMax();
      y2 = s.yMax();

      return s.getTechLayer()->getRoutingLevel();
    }
  }
  return 0;
}
uint dbBlockSearch::getInstObs(dbInst* inst, bool viaFlag)
{
  uint cnt = 0;

  dbInstShapeItr obs_shapes;

  dbShape s;
  for (obs_shapes.begin(inst, dbInstShapeItr::OBSTRUCTIONS);
       obs_shapes.next(s);) {
    if (s.isVia()) {
      if (viaFlag)
        continue;
    } else {
      cnt += _dcr->addBox(inst->getId(),
                          _inst_obs_id,
                          _instMenuId,
                          s.getTechLayer()->getRoutingLevel(),
                          s.xMin(),
                          s.yMin(),
                          s.xMax(),
                          s.yMax(),
                          0);
    }
  }
  return cnt;
}
uint dbBlockSearch::addInstShapes(dbInst* inst,
                                  bool vias,
                                  bool pinFlag,
                                  bool obsFlag)
{
  uint cnt = 0;
  if (pinFlag) {
    cnt += getItermShapes(inst, false);
    if (vias)
      cnt += getItermShapes(inst, true);
  }
  if (obsFlag) {
    cnt += getInstObs(inst, false);
    if (vias)
      cnt += getInstObs(inst, true);
  }
  return cnt;
}

uint dbBlockSearch::getInstBoxes(int x1,
                                 int y1,
                                 int x2,
                                 int y2,
                                 std::vector<dbInst*>& result)
{
  _instSdb->searchBoxIds(x1, y1, x2, y2);

  uint cnt = 0;
  _instSdb->startIterator();
  uint wireId = 0;
  while ((wireId = _instSdb->getNextWireId()) > 0) {
    uint dbBoxId;
    uint id2;
    uint wtype;
    _instSdb->getIds(wireId, &dbBoxId, &id2, &wtype);
    dbInst* inst = dbInst::getInst(_block, dbBoxId);

    result.push_back(inst);
    cnt++;
  }
  return cnt;
}

void dbBlockSearch::getInstBoxes(bool /* unused: ignoreFlag */)
{
  if (_instSdb == NULL)
    return;

  if (!_dcr->getSubMenuFlag(_instMenuId, _inst_bb_id))
    return;

  int x1, y1, x2, y2;
  _dcr->getBbox(&x1, &y1, &x2, &y2);

  _instSdb->searchBoxIds(x1, y1, x2, y2);
  _instSdb->makeGuiBoxes(
      _dcr, _instMenuId, _inst_bb_id, false);  // use coords from Sdb

  /* Can get box ids and use DB box coords

          while (uint boxId= zui->getNextId()) {
                  dbBox *bb= dbBox::getBox(_block, boxId);
                  zui->addBox(bb, Ath_box__bbox, 0);
          }
  */
}
void dbBlockSearch::getInstShapes(bool vias, bool pins, bool obs)
{
  if (_instSdb == NULL)
    return;

  bool obsFlag = obs || _dcr->getSubMenuFlag(_instMenuId, _inst_obs_id);
  bool pinFlag = pins || _dcr->getSubMenuFlag(_instMenuId, _inst_pin_id);

  if (!(obsFlag || pinFlag))
    return;

  uint cnt = 0;

  int x1, y1, x2, y2;
  _dcr->getBbox(&x1, &y1, &x2, &y2);

  _instSdb->searchBoxIds(x1, y1, x2, y2);

  _instSdb->startIterator();
  uint wireId = 0;
  while ((wireId = _instSdb->getNextWireId()) > 0) {
    /*
    int x1, y1, x2, y2;
    uint level, id1, id2, wtype;
    _instSdb->getBox(wireId, &x1, &y1, &x2, &y2, &level, &id1, &id2, &wtype);
    */
    uint dbBoxId;
    uint id2;
    uint wtype;
    _instSdb->getIds(wireId, &dbBoxId, &id2, &wtype);
    dbInst* inst = dbInst::getInst(_block, dbBoxId);

    /* DKF 07/05/05		dbBox *bb= dbBox::getBox(_block, dbBoxId);
                    dbInst *inst= (dbInst *) bb->getBoxOwner();
    */
    cnt += addInstShapes(inst, vias, pinFlag, obsFlag);
  }
}
/*
void dbBlockSearch::white(Ath__zui *zui, Ath__hierType hier, bool ignoreFlag)
{
        if (!ignoreFlag && !zui->getDbFlag("inst/white"))
                return;

        if (_dbInstSearch==NULL)
                return;

        Ath__searchBox bb(zui->getBbox(), 0, 0);
        zui->getIdTable()->resetCnt();
        _dbInstSearch->white(&bb, 0, 0, zui->getIdTable(), false); //single grid

        while (uint id= zui->getNextId())
        {
                Ath__wire *w= _dbInstSearch->getWirePtr(id);

                Ath__searchBox bb;
                w->getCoords(&bb);
                bb.setLevel(5);

                zui->addBox(&bb, Ath_hier__block, Ath_box__white, id);

                _dbInstSearch->releaseWire(id);
        }
}
void dbBlockSearch::getWireIds(Ath__array1D<uint> *wireIdTable,
Ath__array1D<uint> *idtable)
{
        // remove duplicate entries

        for (uint ii= 0; ii<wireIdTable->getCnt(); ii++)
        {
                uint wid= wireIdTable->get(ii);
                Ath__wire *w= _dbNetWireSearch->getWirePtr(wid);

                uint srcId= w->getSrcId();
                if (srcId>0) {
                        w= _dbNetWireSearch->getWirePtr(srcId);
                }
                if (w->_ext>0)
                        continue;

                w->_ext= 1;
                idtable->add(w->_id);
        }

        for (uint jj= 0; jj<wireIdTable->getCnt(); jj++)
        {
                Ath__wire *w= _dbNetWireSearch->getWirePtr( wireIdTable->get(jj)
);

                w->_ext= 0;
        }
}
uint dbBlockSearch::getDbBoxId(uint wid, uint wireType)
{
        Ath__wire *w= _dbNetWireSearch->getWirePtr(wid);
        if (w->_flags!=wireType)
                return 0;

        return w->getBoxId();
}
*/
int dbBlockSearch::getShapeLevel(dbSBox* s, bool wireVia)
{
  if (s->isVia()) {
    if (wireVia)  // request for wire
      return -1;

    return getViaLevel(s);
  } else {
    if (!wireVia)  // request for via
      return -1;

    return s->getTechLayer()->getRoutingLevel();
  }
}
int dbBlockSearch::getShapeLevel(dbShape* s, bool wireVia)
{
  if (s->isVia()) {
    if (wireVia)  // request for wire
      return -1;

    return getViaLevel(s);
  } else {
    if (!wireVia)  // request for via
      return -1;

    return s->getTechLayer()->getRoutingLevel();
  }
}
dbNet* dbBlockSearch::getNet(uint wireId, uint shapeId)
{
  // wireVia=true, wire
  if (shapeId > 0) {
    dbShape s;
    dbWire* w = dbWire::getWire(_block, wireId);
    w->getShape(shapeId, s);

    return w->getNet();
  } else {
    dbSBox* s = dbSBox::getSBox(_block, wireId);

    dbSWire* sw = (dbSWire*) s->getBoxOwner();
    return sw->getNet();
  }
}
uint dbBlockSearch::addSBox(uint menuId,
                            uint subMenuId,
                            bool wireVia,
                            uint wireId)
{
  dbSBox* s = dbSBox::getSBox(_block, wireId);

  int level = getShapeLevel(s, wireVia);
  if (level < 0)
    return 0;

  dbSWire* sw = (dbSWire*) s->getBoxOwner();
  dbNet* net = sw->getNet();

  return _dcr->addBox(net->getId(),
                      subMenuId,
                      menuId,
                      level,
                      s->xMin(),
                      s->yMin(),
                      s->xMax(),
                      s->yMax(),
                      0);
}
uint dbBlockSearch::addWireCoords(dbShape& s,
                                  uint menuId,
                                  uint subMenuId,
                                  uint netId,
                                  uint shapeId)
{
  int level = s.getTechLayer()->getRoutingLevel();

  int x1 = s.xMin();
  int y1 = s.yMin();
  int x2 = s.xMax();
  int y2 = s.yMax();

  return _dcr->addBox(netId, subMenuId, menuId, level, x1, y1, x2, y2, shapeId);
}
uint dbBlockSearch::addViaCoords(dbShape& viaShape,
                                 uint menuId,
                                 uint subMenuId,
                                 uint netId,
                                 uint shapeId)
{
  std::vector<dbShape> shapes;
  dbShape::getViaBoxes(viaShape, shapes);

  // dbTechVia *via= viaShape.getTechVia();

  return addViaBoxes(viaShape, menuId, subMenuId, netId, shapeId);
}

uint dbBlockSearch::addWireViaCoords(uint menuId,
                                     uint subMenuId,
                                     bool wireVia,
                                     uint wireId,
                                     uint shapeId)
{
  // wireVia=true, wire
  if (shapeId > 0) {
    dbShape s;
    dbWire* w = dbWire::getWire(_block, wireId);
    w->getShape(shapeId, s);

    dbNet* net = w->getNet();

    if (s.isVia()) {
      if (!wireVia)
        return addViaBoxes(s, menuId, subMenuId, net->getId(), shapeId);
      // return addViaCoords(s, menuId, subMenuId, net->getId(), shapeId);
      else
        return 0;
    }

    int level = getShapeLevel(&s, wireVia);

    int x1 = s.xMin();
    int y1 = s.yMin();
    int x2 = s.xMax();
    int y2 = s.yMax();
    uint flag = _dcr->addBox(
        net->getId(), subMenuId, menuId, level, x1, y1, x2, y2, shapeId);

    return flag;
  } else {
    dbSBox* s = dbSBox::getSBox(_block, wireId);

    int level = getShapeLevel(s, wireVia);
    if (level < 0)
      return 0;

    dbSWire* sw = (dbSWire*) s->getBoxOwner();
    dbNet* net = sw->getNet();

    return _dcr->addBox(net->getId(),
                        subMenuId,
                        menuId,
                        level,
                        s->xMin(),
                        s->yMin(),
                        s->xMax(),
                        s->yMax(),
                        0);
  }
}
uint dbBlockSearch::addViaBoxes(dbShape& sVia,
                                uint menuId,
                                uint subMenuId,
                                uint id,
                                uint shapeId)
{
  uint cnt = 0;

  const char* tcut = "tcut";
  const char* bcut = "bcut";

  std::vector<dbShape> shapes;
  dbShape::getViaBoxes(sVia, shapes);
  uint topLevel = 0;
  uint botLevel = getViaLevels(sVia, topLevel);

  // uint topLevel= via->getTopLayer()->getRoutingLevel();
  // uint botLevel= via->getBottomLayer()->getRoutingLevel();

  std::vector<dbShape>::iterator shape_itr;

  for (shape_itr = shapes.begin(); shape_itr != shapes.end(); ++shape_itr) {
    dbShape s = *shape_itr;

    if (s.getTechLayer()->getType() == dbTechLayerType::CUT)
      continue;

    int x1 = s.xMin();
    int y1 = s.yMin();
    int x2 = s.xMax();
    int y2 = s.yMax();

    uint level = s.getTechLayer()->getRoutingLevel();

    cnt += _dcr->addBox(id, subMenuId, menuId, level, x1, y1, x2, y2, shapeId);
  }
  notice(0, "_skipCutBoxes=%d\n", _skipCutBoxes);
  if (_skipCutBoxes)
    return cnt;
  for (shape_itr = shapes.begin(); shape_itr != shapes.end(); ++shape_itr) {
    dbShape s = *shape_itr;

    if (s.getTechLayer()->getType() != dbTechLayerType::CUT)
      continue;

    int x1 = s.xMin();
    int y1 = s.yMin();
    int x2 = s.xMax();
    int y2 = s.yMax();

    cnt += _dcr->addBox(
        id, subMenuId, menuId, topLevel, x1, y1, x2, y2, shapeId, tcut);
    cnt += _dcr->addBox(
        id, subMenuId, menuId, botLevel, x1, y1, x2, y2, shapeId, bcut);
  }
  return cnt;
}
uint dbBlockSearch::addViaBoxes(dbBox* viaBox,
                                uint menuId,
                                uint subMenuId,
                                uint netId,
                                uint shapeId)
{
  uint cnt = 0;

  const char* tcut = "tcut";
  const char* bcut = "bcut";

  uint topLevel = 0;
  uint botLevel = getViaLevels(viaBox, topLevel);

  std::vector<dbShape> shapes;
  viaBox->getViaBoxes(shapes);

  std::vector<dbShape>::iterator shape_itr;

  for (shape_itr = shapes.begin(); shape_itr != shapes.end(); ++shape_itr) {
    dbShape s = *shape_itr;

    if (s.getTechLayer()->getType() == dbTechLayerType::CUT)
      continue;

    int x1 = s.xMin();
    int y1 = s.yMin();
    int x2 = s.xMax();
    int y2 = s.yMax();

    uint level = s.getTechLayer()->getRoutingLevel();

    cnt += _dcr->addBox(
        netId, subMenuId, menuId, level, x1, y1, x2, y2, shapeId);
  }
  notice(0, "_skipCutBoxes=%d\n", _skipCutBoxes);
  if (_skipCutBoxes)
    return cnt;

  for (shape_itr = shapes.begin(); shape_itr != shapes.end(); ++shape_itr) {
    dbShape s = *shape_itr;

    if (s.getTechLayer()->getType() != dbTechLayerType::CUT)
      continue;

    int x1 = s.xMin();
    int y1 = s.yMin();
    int x2 = s.xMax();
    int y2 = s.yMax();

    cnt += _dcr->addBox(
        netId, subMenuId, menuId, topLevel, x1, y1, x2, y2, shapeId, tcut);
    cnt += _dcr->addBox(
        netId, subMenuId, menuId, botLevel, x1, y1, x2, y2, shapeId, bcut);
  }
  return cnt;
}
uint dbBlockSearch::getViaLevels(dbBox* s, uint& top)
{
  dbTechVia* via = s->getTechVia();
  if (via != NULL) {
    uint topLevel = via->getTopLayer()->getRoutingLevel();
    uint botLevel = via->getBottomLayer()->getRoutingLevel();
    top = topLevel;

    return botLevel;
  } else {
    top = 0;
    dbVia* via = s->getBlockVia();
    if (via == NULL)  // should be error????
      return 0;
    uint topLevel = via->getTopLayer()->getRoutingLevel();
    uint botLevel = via->getBottomLayer()->getRoutingLevel();
    top = topLevel;

    return botLevel;
  }
}
uint dbBlockSearch::getViaLevels(dbShape& s, uint& top)
{
  dbTechVia* via = s.getTechVia();
  if (via != NULL) {
    uint topLevel = via->getTopLayer()->getRoutingLevel();
    uint botLevel = via->getBottomLayer()->getRoutingLevel();
    top = topLevel;

    return botLevel;
  } else {
    top = 0;
    dbVia* via = s.getVia();
    if (via == NULL)  // should be error????
      return 0;
    uint topLevel = via->getTopLayer()->getRoutingLevel();
    uint botLevel = via->getBottomLayer()->getRoutingLevel();
    top = topLevel;

    return botLevel;
  }
}
uint dbBlockSearch::addViaCoordsFromWire(uint menuId,
                                         uint subMenuId,
                                         uint netId,
                                         uint shapeId)
{
  /* TODO
  uint labelCnt= 2;
  strcpy(_labelName[0], "R=");
  strcpy(_labelName[1], "C=");
  */
  uint cnt = 0;

  if (shapeId > 0) {  // signal via
    dbNet* net = dbNet::getNet(_block, netId);
    dbWire* w = net->getWire();

    dbShape viaShape1;
    if (w->getPrevVia(shapeId, viaShape1)) {
      cnt = addViaBoxes(viaShape1, menuId, subMenuId, net->getId(), shapeId);
    }
    dbShape viaShape2;
    if (w->getNextVia(shapeId, viaShape2)) {
      cnt = addViaBoxes(viaShape2, menuId, subMenuId, net->getId(), shapeId);
    }
  } else {  // power via
    uint sboxId = netId;

    dbBox* viaBox
        = (dbBox*) dbSBox::getSBox(_block, sboxId);  // netId is box id
    if (!viaBox->isVia())
      return 0;

    cnt = addViaBoxes(viaBox, menuId, subMenuId, sboxId, 0);
  }
  return cnt;
}

uint dbBlockSearch::getViasFromWires(ZPtr<ISdb> sdb,
                                     uint menuId,
                                     uint subMenuId,
                                     uint wireMenuId,
                                     dbNet* targetNet,
                                     bool excludeFlag)
{
  uint cnt = 0;
  sdb->startIterator();
  uint wid = 0;
  while ((wid = sdb->getNextWireId()) > 0) {
    uint netId, shapeId;
    uint wtype;
    sdb->getIds(wid, &netId, &shapeId, &wtype);

    if (wireMenuId != wtype)  // TO_TEST
      continue;

    dbNet* net = NULL;
    if (shapeId == 0) {
      dbSBox* b = dbSBox::getSBox(_block, netId);
      net = (dbNet*) b->getBoxOwner();
    } else {
      net = dbNet::getNet(_block, netId);
    }
    if (targetNet == NULL) {
      cnt += addViaCoordsFromWire(menuId, subMenuId, netId, shapeId);
      continue;
    }
    if (excludeFlag && (net == targetNet))
      continue;
    if (!excludeFlag && (net != targetNet))
      continue;

    cnt += addViaCoordsFromWire(menuId, subMenuId, netId, shapeId);
  }
  return cnt;
}
int dbBlockSearch::getTrackXY(int boxXY, int org, int count, int step)
{
  int startTrackNum = (boxXY - org) / step;
  if (startTrackNum > count)
    startTrackNum = count;
  if (startTrackNum <= 0)
    startTrackNum = 0;

  int xy = org + startTrackNum * step;

  return xy;
}
uint dbBlockSearch::getTracks(dbTrackGrid* g,
                              int org,
                              int count,
                              int step,
                              int* bb_ll,
                              int* bb_ur,
                              uint level,
                              uint dir)
{
  uint cnt = 0;

  int ll[2] = {bb_ll[0], bb_ll[1]};
  int ur[2] = {bb_ur[0], bb_ur[1]};

  int XY1 = getTrackXY(bb_ll[dir], org, count, step);
  int XY2 = getTrackXY(bb_ur[dir], org, count, step);

  for (int xy = XY1; xy <= XY2; xy += step) {
    ll[dir] = xy;
    ur[dir] = xy + 4;

    cnt += _dcr->addBox(g->getId(),
                        _block_track_id,
                        _blockMenuId,
                        level,
                        ll[0],
                        ll[1],
                        ur[0],
                        ur[1],
                        dir + 1);
  }
  return cnt;
}
#ifdef NEW_TRACKS
uint dbBlockSearch::getTracks(bool ignoreLayers)
{
  if (!_dcr->getSubMenuFlag(_blockMenuId, _block_track_id))
    return 0;

  Rect die;
  _block->getDieArea(die);

  int x1, y1, x2, y2;
  _dcr->getBbox(&x1, &y1, &x2, &y2);

  x1 = MAX(x1, die.xMin());
  y1 = MAX(y1, die.yMin());
  x2 = MIN(x2, die.xMax());
  y2 = MIN(y2, die.yMax());

  int bb_ll[2] = {x1, y1};
  int bb_ur[2] = {x2, y2};

  bool* excludeTable = _dcr->getExcludeLayerTable();

  dbSet<dbTrackGrid> grids = _block->getTrackGrids();
  dbSet<dbTrackGrid>::iterator itr;

  uint cnt = 0;
  for (itr = grids.begin(); itr != grids.end(); ++itr) {
    dbTrackGrid* grid = *itr;
    dbTechLayer* layer = grid->getTechLayer();
    uint level = layer->getRoutingLevel();
    if (excludeTable[level])
      continue;

    int i;
    for (i = 0; i < grid->getNumGridPatternsX(); ++i) {
      int orgX, count, step;
      grid->getGridPatternX(i, orgX, count, step);

      cnt += getTracks(grid, orgX, count, step, bb_ll, bb_ur, level, 0);
    }

    for (i = 0; i < grid->getNumGridPatternsY(); ++i) {
      int orgY, count, step;
      grid->getGridPatternY(i, orgY, count, step);

      cnt += getTracks(grid, orgY, count, step, bb_ll, bb_ur, level, 1);
    }
  }
  return cnt;
}
#else
uint dbBlockSearch::getTracks(bool /* unused: ignoreLayers */)
{
  if (_trackSdb == NULL)
    return 0;

  if (!_dcr->getSubMenuFlag(_blockMenuId, _block_track_id))
    return 0;

  int x1, y1, x2, y2;
  _dcr->getBbox(&x1, &y1, &x2, &y2);

  bool* exludeTable = _dcr->getExcludeLayerTable();
  _trackSdb->searchWireIds(x1, y1, x2, y2, false, exludeTable);

  return _trackSdb->makeGuiBoxes(
      _dcr, _blockMenuId, _block_track_id, false, 0);  // use coords from Sdb
}
#endif
uint dbBlockSearch::getPowerWireVias(ZPtr<ISdb> sdb,
                                     dbNet* targetNet,
                                     bool vias,
                                     std::vector<dbBox*>& viaTable)
{
  sdb->startIterator();
  uint wid = 0;
  while ((wid = sdb->getNextWireId()) > 0) {
    uint netId, shapeId;
    uint wtype;
    sdb->getIds(wid, &netId, &shapeId, &wtype);

    dbNet* net = NULL;
    if (shapeId > 0)
      continue;

    uint sboxId = netId;
    dbSBox* s = (dbSBox*) dbSBox::getSBox(_block, sboxId);  // netId is box id

    if (vias && !s->isVia())
      continue;
    if (!vias && s->isVia())
      continue;

    net = s->getSWire()->getNet();
    if ((targetNet != NULL) && (net != targetNet))
      continue;

    viaTable.push_back(s);
  }
  return viaTable.size();
}
uint dbBlockSearch::getPowerWires(int x1,
                                  int y1,
                                  int x2,
                                  int y2,
                                  int layer,
                                  dbNet* targetNet,
                                  std::vector<dbBox*>& viaTable)
{
  if (_netSdb == NULL)
    return 0;

  bool exludeTable[16];
  for (uint i = 0; i < 16; i++)
    exludeTable[i] = true;

  exludeTable[layer] = false;
  uint cnt = 0;

  _netSdb->searchWireIds(x1, y1, x2, y2, false, exludeTable);
  cnt = getPowerWireVias(_netSdb, targetNet, false, viaTable);
  return cnt;
}
uint dbBlockSearch::getPowerWiresAndVias(int x1,
                                         int y1,
                                         int x2,
                                         int y2,
                                         int layer,
                                         dbNet* targetNet,
                                         bool power_wires,
                                         std::vector<dbBox*>& viaTable)
{
  if ((_netSdb == NULL) || (_netViaSdb == NULL))
    return 0;

  bool exludeTable[16];
  for (uint i = 0; i < 16; i++)
    exludeTable[i] = true;

  exludeTable[layer] = false;
  uint cnt = 0;

  if (power_wires) {
    _netSdb->searchWireIds(x1, y1, x2, y2, false, exludeTable);
    cnt = getPowerWireVias(_netSdb, targetNet, false, viaTable);
  } else {
    _netViaSdb->searchWireIds(x1, y1, x2, y2, true, exludeTable);
    cnt = getPowerWireVias(_netViaSdb, targetNet, true, viaTable);
  }
  return cnt;
}
uint dbBlockSearch::getWiresAndVias_all(dbNet* targetNet, bool ignoreFlag)
{
  if (_netSdb == NULL)
    return 0;

  bool signal_wires = _dcr->getSubMenuFlag(_signalMenuId, _signal_wire_id);
  bool signal_vias = _dcr->getSubMenuFlag(_signalMenuId, _signal_via_id);
  bool power_wires = _dcr->getSubMenuFlag(_powerMenuId, _power_wire_id);
  bool power_vias = _dcr->getSubMenuFlag(_powerMenuId, _power_via_id);

  if ((!ignoreFlag)
      && (!(signal_wires || signal_vias || power_wires || power_vias)))

    return 0;

  uint excludeNetId = 0;
  if (targetNet != NULL)
    excludeNetId = targetNet->getId();

  uint cnt = 0;

  int x1, y1, x2, y2;
  _dcr->getBbox(&x1, &y1, &x2, &y2);

  bool* exludeTable = NULL;
  if (!signal_vias)  // get all layers
    exludeTable = _dcr->getExcludeLayerTable();

  // _netSdb->searchWireIds(x1, y1, x2, y2, true, exludeTable);
  _netSdb->searchWireIds(x1, y1, x2, y2, false, exludeTable);

  if (power_wires)
    _netSdb->makeGuiBoxes(_dcr,
                          _powerMenuId,
                          _power_wire_id,
                          false,
                          excludeNetId);  // use coords from Sdb

  if (power_vias) {
    _netViaSdb->searchWireIds(x1, y1, x2, y2, true, exludeTable);
    getViasFromWires(
        _netViaSdb, _powerMenuId, _power_via_id, _power_via_id, NULL, false);
    //_netViaSdb->makeGuiBoxes(_dcr, _powerMenuId, _power_via_id, false,
    // excludeNetId); // use coords from Sdb
  }

  if (signal_wires)
    _netSdb->makeGuiBoxes(_dcr,
                          _signalMenuId,
                          _signal_wire_id,
                          false,
                          excludeNetId);  // use coords from Sdb

  if (signal_vias)
    getViasFromWires(
        _netSdb, _signalMenuId, _signal_via_id, _signal_wire_id, NULL, false);
  //_netViaSdb->makeGuiBoxes(_dcr, _signalMenuId, _signal_via_id, false,
  // excludeNetId); // use coords from Sdb

  return cnt;
}
uint dbBlockSearch::getWiresClipped(dbNet* targetNet,
                                    uint halo,
                                    bool ignoreFlag)
{
  if (_netSdb == NULL)
    return 0;

  bool signal_wires = _dcr->getSubMenuFlag(_signalMenuId, _signal_wire_id);
  bool power_wires = _dcr->getSubMenuFlag(_powerMenuId, _power_wire_id);

  if ((!ignoreFlag) && (!(signal_wires || power_wires)))
    return 0;

  uint excludeNetId = 0;
  if (targetNet != NULL)
    excludeNetId = targetNet->getId();

  int x1, y1, x2, y2;
  _dcr->getBbox(&x1, &y1, &x2, &y2);
  bool* exludeTable = _dcr->getExcludeLayerTable();
  uint cnt = 0;

  dbWire* wire = targetNet->getWire();

  if (wire == NULL)
    return 0;

  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    // uint level= 0;

    // int shapeId= shapes.getShapeId();

    if (s.isVia())
      continue;
    //	level= s.getTechLayer()->getRoutingLevel();

    int sx1 = s.xMin() - halo;
    int sy1 = s.yMin() - halo;
    int sx2 = s.xMax() + halo;
    int sy2 = s.yMax() + halo;

    if (_dcr->clipBox(sx1, sy1, sx2, sy2)) {
      _netSdb->searchWireIds(sx1, sy1, sx2, sy2, true, exludeTable);

      _dcr->setSearchBox(sx1, sy1, sx2, sy2);

      if (power_wires)
        _netSdb->makeGuiBoxes(_dcr,
                              _powerMenuId,
                              _power_wire_id,
                              false,
                              excludeNetId);  // use coords from Sdb
      if (signal_wires)
        _netSdb->makeGuiBoxes(_dcr,
                              _signalMenuId,
                              _signal_wire_id,
                              false,
                              excludeNetId);  // use coords from Sdb
    }
    _dcr->setSearchBox(x1, y1, x2, y2);
  }
  return cnt;
}
/*
uint dbBlockSearch::getTileBuses(Ath__zui *zui)
{
        if (_quad==NULL)
                return 0;

        uint cnt= 0;
        if (zui->getDbFlag("tile/tnet/bus"))
                cnt += _quad->getTileBuses_1(zui);

        return cnt;
}
uint dbBlockSearch::getTilePins(Ath__zui *zui)
{
        if (_quad==NULL)
                return 0;

        uint cnt= 0;
        if (zui->getDbFlag("tile/tnet/pin"))
                cnt += _quad->getTilePins_1(zui);

        return cnt;
}
*/
uint dbBlockSearch::addArrow(int x1,
                             int y1,
                             int x2,
                             int y2,
                             int labelCnt,
                             char** label,
                             double* val)
{
  return _dcr->addArrow(true,
                        _inst_bb_id,
                        _instMenuId,
                        0,
                        labelCnt,
                        label,
                        val,
                        x1,
                        y1,
                        x2,
                        y2,
                        0);
}
uint dbBlockSearch::addArrow(dbInst* inst1,
                             dbInst* inst2,
                             int labelCnt,
                             char** label,
                             double* val)
{
  dbBox* s1 = inst1->getBBox();
  dbBox* s2 = inst2->getBBox();

  int x1 = (s1->xMin() + s1->xMax()) / 2;
  int y1 = (s1->yMin() + s1->yMax()) / 2;
  int x2 = (s2->xMin() + s2->xMax()) / 2;
  int y2 = (s2->yMin() + s2->yMax()) / 2;

  // 3 inst rarrow 1900000 400000 0 5000 color 0 label {tarrow 1}
  /*
  uint Ath__zui::addArrow(bool right, uint boxType, uint hier, int layer,
                                          int labelCnt, char **label, double
  *val, int x1, int y1,int x2, int y2, uint boxFilter)
  */

  return _dcr->addArrow(true,
                        _inst_bb_id,
                        _instMenuId,
                        0,
                        labelCnt,
                        label,
                        val,
                        x1,
                        y1,
                        x2,
                        y2,
                        0);
}
uint dbBlockSearch::addFlightLines(dbInst* inst)
{
  int labelCnt = 0;
  char** label = new char*[3];
  for (uint ii = 0; ii < 3; ii++)
    label[ii] = new char[16];

  double val[3];

  uint cnt = 0;
  std::vector<dbInst*> connectivity;
  inst->getConnectivity(connectivity);

  std::vector<dbInst*>::iterator inst_itr;

  _dcr->setInstMarker();
  for (inst_itr = connectivity.begin(); inst_itr != connectivity.end();
       ++inst_itr) {
    dbInst* inst1 = *inst_itr;

    labelCnt = 0;

    val[labelCnt] = inst->getId();
    strcpy(label[labelCnt++], "instId1=");

    val[labelCnt] = inst1->getId();
    strcpy(label[labelCnt++], "instId2=");
    cnt += addArrow(inst, inst1, labelCnt, label, val);
  }
  for (uint jj = 0; jj < 3; jj++)
    delete[] label[jj];
  delete[] label;

  return cnt;
}
void dbBlockSearch::addInstConnList(dbInst* inst, bool ignoreFlags)
{
  bool instBoxes
      = ignoreFlags || _dcr->getSubMenuFlag(_instMenuId, _inst_bb_id);
  bool termShapes
      = ignoreFlags || _dcr->getSubMenuFlag(_instMenuId, _inst_pin_id);
  bool instObs = ignoreFlags || _dcr->getSubMenuFlag(_instMenuId, _inst_obs_id);

  std::vector<dbInst*> connectivity;
  inst->getConnectivity(connectivity);

  std::vector<dbInst*>::iterator inst_itr;

  _dcr->setInstMarker();
  uint cnt = addInstBoxes(inst, instBoxes, termShapes, instObs, false);

  for (inst_itr = connectivity.begin(); inst_itr != connectivity.end();
       ++inst_itr) {
    dbInst* inst1 = *inst_itr;
    cnt += addInstBoxes(inst1, instBoxes, termShapes, instObs, false);
  }
}

bool dbBlockSearch::getWildCardName(const char* name, char* outName)
{
  uint wCnt = 0;
  uint kk = 0;
  for (uint ii = 0; name[ii] != '\0'; ii++) {
    if (name[ii] == '*') {
      wCnt++;
      continue;
    }
    outName[kk++] = name[ii];
  }
  outName[kk] = '\0';
  if (wCnt > 1)
    return false;
  else
    return true;
}

void dbBlockSearch::selectInst()
{
  bool termShapes = false;
  bool instObs = false;
  bool vias = false;

  uint instId = _dcr->getSubmenuObjId(NULL);
  dbInst* inst = NULL;
  dbSet<dbInst> insts = _block->getInsts();

  if (instId > 0) {
    if (instId <= insts.sequential())
      inst = dbInst::getInst(_block, instId);

    if (inst == NULL) {
      warning(0, "Cannot find instance in DB with id %d\n", instId);
      return;
    }
    addInstBoxes(inst, true, termShapes, instObs, vias);
  } else {
    char* inspectName = _dcr->getInspectName();
    if (strstr(inspectName, "*") == NULL) {
      inst = _block->findInst(inspectName);
      if (inst == NULL) {
        warning(0, "Cannot find instance in DB with name %s\n", inspectName);
        return;
      }
      addInstBoxes(inst, true, termShapes, instObs, vias);
    } else {
      char instSubName[1024];
      if (!getWildCardName(inspectName, instSubName))
        return;

      dbSet<dbInst>::iterator inst_itr;

      // uint instCnt= 0;
      for (inst_itr = insts.begin(); inst_itr != insts.end(); ++inst_itr) {
        dbInst* inst = *inst_itr;

        if (inst->getMaster()->getMTermCount() <= 0)
          continue;

        char instName[1024];
        strcpy(instName, inst->getName().c_str());

        if (strstr(instName, instSubName) == NULL)
          continue;

        addInstBoxes(inst, true, termShapes, instObs, vias);
      }
    }
  }
}
void dbBlockSearch::selectIterm2Net(uint itermId)
{
  dbITerm* iterm = NULL;
  dbSet<dbITerm> iterms = _block->getITerms();

  if (itermId <= iterms.sequential())
    iterm = dbITerm::getITerm(_block, itermId);
  if (iterm == NULL) {
    warning(0, "Cannot find instance term in DB with id %d\n", itermId);
    return;
  }

  getItermShapesWithViaShapes(iterm);

  dbNet* net = iterm->getNet();

  // add context markers

  dbSigType type = net->getSigType();
  if ((type == dbSigType::POWER) || (type == dbSigType::GROUND))
    addNetSBoxesOnSearch(net, false);
  else
    getNetConnectivity(net, false, 0, false, false, false);
}

uint dbBlockSearch::selectIterm()
{
  uint itermId = _dcr->getSubmenuObjId(NULL);

  uint netCnt = 0;

  // dbNet *net= NULL;
  if (itermId > 0) {
    selectIterm2Net(itermId);
    netCnt++;
  }
  /*
  else {
          char *inspectName= _dcr->getInspectName();
          if (strstr(inspectName, "*")==NULL) {
                  dbBTerm* bterm= _block->findBTerm(inspectName);

                  selectBterm2Net(bterm->getId());
                  netCnt++;
          }
          else {
                  char netSubName[1024];
                  if (!getWildCardName(inspectName, netSubName))
                          return 0;

                  dbSet<dbBTerm> bterms = _block->getBTerms();
                  dbSet<dbBTerm>::iterator bterm_itr;

                  for( bterm_itr = bterms.begin(); bterm_itr != bterms.end();
  ++bterm_itr ) { dbBTerm *bterm = *bterm_itr;

                          char btermName[1024];
                          strcpy(btermName, bterm->getName().c_str());

                          if (strstr(btermName, netSubName)==NULL)
                                  continue;

                          selectBterm2Net(bterm->getId());

                          netCnt++;
                  }
          }
  }
  */
  return netCnt;
}

void dbBlockSearch::addNetSBoxesOnSearch(dbNet* net, bool skipVias)
{
  dbSet<dbSWire> swires = net->getSWires();
  dbSet<dbSWire>::iterator itr;
  for (itr = swires.begin(); itr != swires.end(); ++itr) {
    dbSWire* swire = *itr;
    dbSet<dbSBox> wires = swire->getWires();
    dbSet<dbSBox>::iterator box_itr;

    for (box_itr = wires.begin(); box_itr != wires.end(); ++box_itr) {
      dbSBox* s = *box_itr;

      uint boxId = s->getId();

      uint level = 0;

      if (s->isVia()) {
        if (skipVias)
          continue;

        level = getViaLevel(s);
      } else
        level = s->getTechLayer()->getRoutingLevel();

      Ath__searchBox sbb;
      sbb.set(s->xMin(), s->yMin(), s->xMax(), s->yMax(), level, -1);
      sbb.setOwnerId(boxId, 0);

      // uint dir= sbb.getDir();

      //			_dbNetWireSearch->getGrid(dir,
      // level)->placeWire(&sbb);
    }
  }
}
uint dbBlockSearch::getNetSBoxes(dbNet* net, bool skipVias)
{
  uint cnt = 0;
  dbSet<dbSWire> swires = net->getSWires();
  dbSet<dbSWire>::iterator itr;
  for (itr = swires.begin(); itr != swires.end(); ++itr) {
    dbSWire* swire = *itr;
    dbSet<dbSBox> wires = swire->getWires();
    dbSet<dbSBox>::iterator box_itr;

    for (box_itr = wires.begin(); box_itr != wires.end(); ++box_itr) {
      dbSBox* s = *box_itr;

      uint sboxId = s->getId();

      uint level = 0;

      if (s->isVia()) {
        if (skipVias)
          continue;

        dbBox* viaBox
            = (dbBox*) dbSBox::getSBox(_block, sboxId);  // netId is box id
        if (!viaBox->isVia())
          return 0;

        cnt += addViaBoxes(viaBox, _powerMenuId, _power_wire_id, sboxId, 0);

      } else {
        level = s->getTechLayer()->getRoutingLevel();

        cnt += _dcr->addBox(sboxId,
                            _power_wire_id,
                            _powerMenuId,
                            level,
                            s->xMin(),
                            s->yMin(),
                            s->xMax(),
                            s->yMax(),
                            0);
      }
    }
  }
  return cnt;
}
uint dbBlockSearch::addInstBoxes(dbInst* inst,
                                 bool instBoxes,
                                 bool termShapes,
                                 bool instObs,
                                 bool vias)
{
  uint cnt = 0;
  _dcr->setInstMarker();

  if (instBoxes)
    cnt += addInstBox(inst);

  if (termShapes)
    cnt += getItermShapes(inst, vias);

  if (instObs)
    cnt += getInstObs(inst, vias);

  return cnt;
}
uint dbBlockSearch::addInstBoxes(dbNet* net, bool ignoreFlags)
{
  bool instBoxes
      = ignoreFlags || _dcr->getSubMenuFlag(_instMenuId, _inst_bb_id);
  bool termShapes
      = ignoreFlags || _dcr->getSubMenuFlag(_instMenuId, _inst_pin_id);
  bool instObs = ignoreFlags || _dcr->getSubMenuFlag(_instMenuId, _inst_obs_id);

  if (!(instBoxes || instBoxes || termShapes))
    return 0;

  dbSet<dbITerm> iterms = net->getITerms();
  dbSet<dbITerm>::iterator iterm_itr;

  uint cnt = 0;
  for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr) {
    dbITerm* iterm = *iterm_itr;

    if (termShapes)
      cnt += getItermShapesWithViaShapes(iterm);

    dbInst* inst = iterm->getInst();
    // dbBox * bb = inst->getBBox();

    if (instBoxes)
      cnt += addInstBox(inst);

    if (instObs)
      cnt += getInstObs(inst, true);
  }
  return cnt;
}

uint dbBlockSearch::addNetShapes(dbNet* net,
                                 bool viaWireFlag,
                                 uint menuId,
                                 uint subMenuId)
{
  uint cnt = 0;
  dbWire* wire = net->getWire();
  if (wire == NULL)
    return 0;

  // uint wireId= wire->getId();
  uint netId = net->getId();

  dbWireShapeItr shapes;
  dbShape s;

  for (shapes.begin(wire); shapes.next(s);) {
    if (s.isVia()) {
      if (viaWireFlag)
        continue;

      cnt += addViaCoords(s, menuId, subMenuId, netId, shapes.getShapeId());
    } else if (viaWireFlag) {
      cnt += addWireCoords(s, menuId, subMenuId, netId, shapes.getShapeId());
    }
  }

  return cnt;
}
void dbBlockSearch::getNetBbox(dbNet* net, Rect& maxRect)
{
  dbWire* wire = net->getWire();
  if (wire == NULL)
    return;

  // uint wireId= wire->getId();

  dbWireShapeItr shapes;
  dbShape s;

  for (shapes.begin(wire); shapes.next(s);) {
    Rect r;
    s.getBox(r);
    maxRect.merge(r);
  }
}
uint dbBlockSearch::getNetFromDb(dbNet* net,
                                 bool /* unused: ignoreZuiFlags */,
                                 bool ignoreBB)
{
  uint cnt = 0;

  if (ignoreBB || !_dcr->validSearchBbox()) {
    Rect maxBB;
    maxBB.reset(ath__maxInt, ath__maxInt, -ath__maxInt, -ath__maxInt);
    getNetBbox(net, maxBB);

    uint dd = 1000;
    _dcr->setSearchBox(maxBB.xMin() - dd,
                       maxBB.yMin() - dd,
                       maxBB.xMax() + dd,
                       maxBB.yMax() + dd);

    cnt += addNetShapes(net, true, _signalMenuId, _signal_wire_id);
    cnt += addNetShapes(net, false, _signalMenuId, _signal_via_id);

    _dcr->invalidateSearchBox();
  } else {
    cnt += addNetShapes(net, true, _signalMenuId, _signal_wire_id);
    cnt += addNetShapes(net, false, _signalMenuId, _signal_via_id);
  }

  return cnt;
}
/*
uint dbBlockSearch::getNetFromSearch(dbNet *net, bool ignoreZuiFlags, bool
ignoreBB)
{
        uint cnt= 0;

        int x1, y1,	x2,	y2;
        _dcr->getBbox(&x1, &y1, &x2, &y2);

        Ath__array1D<uint> wireIdTable(16000);

        if (!ignoreBB && !_dcr->validSearchBbox())
        {
                Rect maxBB;
                maxBB.reset(ath__maxInt, ath__maxInt, -ath__maxInt,
-ath__maxInt); getNetBbox(net, maxBB);

                x1= maxBB.xMin();
                y1= maxBB.yMin();
                x2= maxBB.xMax();
                y2= maxBB.yMax();

                uint dd= 1000;
                _dcr->setSearchBox(x1-dd, y1-dd, x2+dd, y2+dd);
        }
        else {
                _dcr->getBbox(&x1, &y1, &x2, &y2);
        }

        bool *exludeTable= _dcr->getExcludeLayerTable();
        _netSdb->searchWireIds(x1, y1, x2, y2, true, exludeTable);

        cnt += getWireVias(_signalMenuId, _signal_wire_id, true, net, false);
        cnt += getWireVias(_signalMenuId, _signal_via_id, false, net, false);

        return cnt;
}
*/
uint dbBlockSearch::getNetWires(dbNet* net,
                                bool contextFlag,
                                uint clipMargin,
                                bool ignoreZuiFlags,
                                bool ignoreBB)
{
  if (net == NULL)
    return 0;

  _dcr->setSignalMarker();
  uint cnt = getNetFromDb(net, ignoreZuiFlags, ignoreBB);
  _dcr->resetMarker();

  _dcr->setContextMarker();
  if (contextFlag) {
    if (clipMargin > 0)
      cnt += getWiresClipped(net, 5000, false);
    else
      // cnt += getWiresAndVias_all(net, true);
      cnt += getWiresClipped(net, 0, false);
  }

  return cnt;
}
uint dbBlockSearch::getNetConnectivity(dbNet* net,
                                       bool contextFlag,
                                       uint clipMargin,
                                       bool /* unused: ignoreLayerFlags */,
                                       bool ignoreZuiFlags,
                                       bool ignoreBB)
{
  if (net == NULL)
    return 0;

  _dcr->setSignalMarker();
  uint cnt = getNetFromDb(net, ignoreZuiFlags, ignoreBB);
  _dcr->resetMarker();

  _dcr->setInstMarker();
  cnt += addInstBoxes(net, ignoreZuiFlags);

  dbSet<dbBTerm> bterms = net->getBTerms();
  cnt += addBtermBoxes(bterms, ignoreZuiFlags);

  _dcr->setContextMarker();
  if (contextFlag) {
    if (clipMargin > 0)
      cnt += getWiresClipped(net, 5000, false);
    else
      cnt += getWiresAndVias_all(net, true);

    dbSet<dbBTerm> blk_bterms = net->getBTerms();
    cnt += addBtermBoxes(blk_bterms, ignoreZuiFlags);
  }

  return cnt;
}
uint dbBlockSearch::getConnectivityWires(dbInst* inst, bool ignoreZuiFlags)
{
  _dcr->setSignalMarker();

  dbSet<dbITerm> iterms = inst->getITerms();
  dbSet<dbITerm>::iterator iterm_itr;

  uint cnt = 0;
  for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr) {
    dbITerm* iterm = *iterm_itr;
    dbNet* net = iterm->getNet();
    if (net == NULL)
      continue;

    dbSigType type = net->getSigType();
    if ((type == dbSigType::POWER) || (type == dbSigType::GROUND))
      continue;

    cnt += getNetFromDb(net, ignoreZuiFlags, true);
  }
  return cnt;
}

void dbBlockSearch::addNetSBoxes(dbNet* net,
                                 uint /* unused: wtype */,
                                 bool skipVias)
{
  dbSet<dbSWire> swires = net->getSWires();
  dbSet<dbSWire>::iterator itr;

  for (itr = swires.begin(); itr != swires.end(); ++itr) {
    dbSWire* swire = *itr;
    dbSet<dbSBox> wires = swire->getWires();
    dbSet<dbSBox>::iterator box_itr;

    for (box_itr = wires.begin(); box_itr != wires.end(); ++box_itr) {
      dbSBox* s = *box_itr;

      // uint level = 0;

      if (s->isVia()) {
        if (skipVias)
          continue;

        // level = getViaLevel(s);
      } else {
        // level = s->getTechLayer()->getRoutingLevel();
      }
      //			_search->addBox(s->xMin(), s->yMin(), s->xMax(),
      // s->yMax(), 				level, s->getId(), 0, wtype);
    }
  }
}

/*
dbInst *dbBlockSearch::getDbInst(Ath__zui *zui)
{
        uint id= zui->getInspNameId();
        dbInst* inst= NULL;
        if (id==0)
        {
                inst= _block->findInst(zui->getInspectName());
                if (inst==NULL)
                {
                        fprintf(stdout, "Cannot find instance %s\n",
                                zui->getInspectName());
                        return NULL;
                }
                id= inst->getId();
        }
        else {
                inst= dbInst::getInst(_block, id);
        }
        return inst;
}
dbNet *dbBlockSearch::getDbNet(Ath__zui *zui)
{
        uint id= zui->getInspNameId();
        dbNet *net= NULL;
        if (id==0)
        {
                net= _block->findNet(zui->getInspectName());
                if (net==NULL)
                {
                        fprintf(stdout, "Cannot find net %s\n",
                                zui->getInspectName());
                        return NULL;
                }
                id= net->getId();
        }
        else {
                net= dbNet::getNet(_block, id);
        }
        return net;
}
dbBTerm *dbBlockSearch::getDbBTerm(Ath__zui *zui)
{
        uint id= zui->getInspNameId();
        dbBTerm *bterm= NULL;
        if (id==0)
        {
                bterm= _block->findBTerm(zui->getInspectName());
                if (bterm==NULL)
                {
                        fprintf(stdout, "Cannot find bterm %s\n",
zui->getInspectName()); return NULL;
                }
                id= bterm->getId();
        }
        else {
                bterm= dbBTerm::getBTerm(_block, id);
        }
        return bterm;
}
*/
dbNet* dbBlockSearch::getNetAndShape(dbShape& s, uint* shapeId, uint* level)
{
  uint netId = _dcr->getSubmenuObjId(shapeId);
  dbNet* net = dbNet::getNet(_block, netId);

  dbWire* w = net->getWire();
  w->getShape(*shapeId, s);

  *level = getShapeLevel(&s, true);

  return net;
}
dbRSeg* dbBlockSearch::getRSeg(dbNet* net, uint shapeId)
{
  int rsegId = 0;

  dbWire* w = net->getWire();

  dbRSeg* rseg = NULL;
  if (w->getProperty(shapeId, rsegId) && rsegId != 0)
    rseg = dbRSeg::getRSeg(_block, rsegId);

  return rseg;
}

bool dbBlockSearch::isSignalNet(dbNet* net)
{
  dbSigType type = net->getSigType();

  return ((type == dbSigType::POWER) || (type == dbSigType::GROUND)) ? false
                                                                     : true;
}
uint dbBlockSearch::selectNet()
{
  uint netId = _dcr->getSubmenuObjId(NULL);

  uint netCnt = 0;

  // bool contextFlag=false;
  // uint clipMargin= 0;
  // bool ignoreLayerFlags= true;
  bool ignoreZuiFlags = false;
  bool ignoreBB = false;

  _dcr->setSignalMarker();
  dbNet* net = NULL;
  dbSet<dbNet> nets = _block->getNets();

  if (netId > 0) {
    if (netId <= nets.sequential())
      net = dbNet::getNet(_block, netId);
    if (net == NULL) {
      warning(0, "Cannot find net in DB with id %d\n", netId);
      return 0;
    }
    if (isSignalNet(net))
      getNetFromDb(net, ignoreZuiFlags, ignoreBB);
    // getNetConnectivity(net, false, 0, true, false, true);
    // getNetConnectivity(net, contextFlag, clipMargin, ignoreLayerFlags,
    // ignoreZuiFlags, ignoreBB);
    else
      getNetSBoxes(net, false);
    netCnt++;
  } else {
    char* inspectName = _dcr->getInspectName();
    if (strstr(inspectName, "*") == NULL) {
      net = _block->findNet(inspectName);
      if (net == NULL) {
        warning(0, "Cannot find net in DB with name %s\n", inspectName);
        return 0;
      }
      if (isSignalNet(net))
        getNetFromDb(net, ignoreZuiFlags, ignoreBB);
      // getNetConnectivity(net, contextFlag, clipMargin, ignoreLayerFlags,
      // ignoreZuiFlags, ignoreBB);
      else
        getNetSBoxes(net, false);

      netCnt++;
    } else {
      char netSubName[1024];
      if (!getWildCardName(inspectName, netSubName))
        return 0;

      dbSet<dbNet>::iterator net_itr;

      for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
        dbNet* net = *net_itr;
        /*
                                        dbSigType type= net->getSigType();
                                        if (!signal &&
           !((type==POWER)||(type==GROUND))) continue;
        */

        char netName[1024];
        strcpy(netName, net->getName().c_str());

        if (strstr(netName, netSubName) == NULL)
          continue;

        if (isSignalNet(net))
          getNetFromDb(net, ignoreZuiFlags, ignoreBB);
        // getNetConnectivity(net, contextFlag, clipMargin, ignoreLayerFlags,
        // ignoreZuiFlags, ignoreBB);
        else
          getNetSBoxes(net, false);

        netCnt++;
      }
    }
  }
  _dcr->resetMarker();
  return netCnt;
}

void dbBlockSearch::selectBterm2Net(uint btermId)
{
  dbBTerm* bterm = NULL;
  dbSet<dbBTerm> bterms = _block->getBTerms();

  if (btermId <= bterms.sequential())
    bterm = dbBTerm::getBTerm(_block, btermId);
  if (bterm == NULL) {
    warning(0, "Cannot find block term in DB with id %d\n", btermId);
    return;
  }
  dbNet* net = bterm->getNet();

  dbSigType type = net->getSigType();

  if ((type == dbSigType::POWER) || (type == dbSigType::GROUND))
    addNetSBoxesOnSearch(net, false);
  else
    getNetConnectivity(net, false, 0, false, false, false);
}

uint dbBlockSearch::selectBterm()
{
  uint btermId = _dcr->getSubmenuObjId(NULL);

  uint netCnt = 0;

  // dbNet *net= NULL;
  if (btermId > 0) {
    selectBterm2Net(btermId);
    netCnt++;
  } else {
    char* inspectName = _dcr->getInspectName();
    if (strstr(inspectName, "*") == NULL) {
      dbBTerm* bterm = _block->findBTerm(inspectName);
      if (bterm == NULL) {
        warning(0, "Cannot find block term in DB with name %s\n", inspectName);
        return 0;
      }

      selectBterm2Net(bterm->getId());
      netCnt++;
    } else {
      char netSubName[1024];
      if (!getWildCardName(inspectName, netSubName))
        return 0;

      dbSet<dbBTerm> bterms = _block->getBTerms();
      dbSet<dbBTerm>::iterator bterm_itr;

      for (bterm_itr = bterms.begin(); bterm_itr != bterms.end(); ++bterm_itr) {
        dbBTerm* bterm = *bterm_itr;

        char btermName[1024];
        strcpy(btermName, bterm->getName().c_str());

        if (strstr(btermName, netSubName) == NULL)
          continue;

        selectBterm2Net(bterm->getId());

        netCnt++;
      }
    }
  }
  return netCnt;
}

}  // namespace odb
