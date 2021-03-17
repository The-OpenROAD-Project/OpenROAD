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

#include <stdio.h>
#include <stdlib.h>

#include "db.h"
#include "dbLogger.h"
#include "dbMap.h"
#include "dbShape.h"
#include "dbWireCodec.h"
#include "tmg_conn.h"

namespace odb {

void tmg_conn::checkConnOrdered(bool verbose)
{
  if (verbose)
    notice(0, "net %d %s\n", _net->getId(), _net->getName().c_str());
  dbITerm* drv_iterm = NULL;
  dbBTerm* drv_bterm = NULL;
  dbITerm* itermV[1024];
  dbBTerm* btermV[1024];
  int itermN = 0;
  int btermN = 0;
  int j;
  _connected = true;
  dbWire* wire = _net->getWire();
  dbWirePathItr pitr;
  dbWirePath path;
  dbWirePathShape pathShape;
  pitr.begin(wire);
  int first = 1;
  while (pitr.getNextPath(path)) {
    if (!path.is_branch) {
      if (verbose) {
        notice(0, "path %d", path.junction_id);
        if (path.iterm) {
          notice(0,
                 " iterm I%d/%s",
                 path.iterm->getInst()->getId(),
                 path.iterm->getMTerm()->getName().c_str());
        } else if (path.bterm) {
          notice(0, " bterm %d", path.bterm->getId());
        }
        notice(0, "\n");
      }
      if (!path.iterm && !path.bterm) {
        notice(0, "NO TERM\n");
        _connected = false;
      }
      if (first) {
        first = 0;
        if (path.iterm) {
          drv_iterm = path.iterm;
          itermV[itermN++] = drv_iterm;
        }
        if (path.bterm) {
          drv_bterm = path.bterm;
          btermV[btermN++] = drv_bterm;
        }
      } else {
        if (verbose && !(drv_iterm && path.iterm == drv_iterm)
            && !(drv_bterm && path.bterm == drv_bterm)) {
          notice(0, "PATH NOT FROM DRIVER\n");
          notice(0, "path.short_junction = %d\n", path.short_junction);
        }
        if (path.iterm) {
          for (j = 0; j < itermN; j++)
            if (itermV[j] == path.iterm)
              break;
          if (j == itermN) {
            notice(0, "DISC\n");
            _connected = false;
            itermV[itermN++] = path.iterm;
          }
        }
        if (path.bterm) {
          for (j = 0; j < btermN; j++)
            if (btermV[j] == path.bterm)
              break;
          if (j == btermN) {
            notice(0, "DISC\n");
            _connected = false;
            btermV[btermN++] = path.bterm;
          }
        }
      }
    } else {
      if (verbose)
        notice(0, "branch %d\n", path.junction_id);
    }
    while (pitr.getNextShape(pathShape)) {
      if (0 && verbose)
        notice(0, "shape %d", pathShape.junction_id);
      if (pathShape.iterm) {
        if (0 && verbose)
          notice(0,
                 " iterm I%d/%s",
                 pathShape.iterm->getInst()->getId(),
                 pathShape.iterm->getMTerm()->getName().c_str());
        for (j = 0; j < itermN; j++)
          if (itermV[j] == pathShape.iterm)
            break;
        if (j == itermN) {
          itermV[itermN++] = pathShape.iterm;
        }
      } else if (pathShape.bterm) {
        if (0 && verbose)
          notice(0, " bterm %d", pathShape.bterm->getId());
        for (j = 0; j < btermN; j++)
          if (btermV[j] == pathShape.bterm)
            break;
        if (j == btermN)
          btermV[btermN++] = pathShape.bterm;
      }
      if (0 && verbose) {
        dbShape s = pathShape.shape;
        if (s.getTechVia())
          notice(0, " %s", s.getTechVia()->getName().c_str());
        else if (s.getVia())
          notice(0, " %s", s.getVia()->getName().c_str());
        else if (s.getTechLayer())
          notice(0, " %s", s.getTechLayer()->getName().c_str());
        notice(0, "\n");
      }
    }
  }
  if (!_connected) {
    _net->setDisconnected(true);
    notice(
        0, "disconnected net %d %s\n", _net->getId(), _net->getName().c_str());
  }
}

static bool checkITermConnect(dbITerm* iterm, dbShape& shape)
{
  // check that some iterm shape intersects/touches shape
  Rect wrect;
  shape.getBox(wrect);
  dbTechLayer* wlyr = shape.getTechLayer();
  dbTechLayer* wlyr_top = NULL;
  if (shape.isVia()) {
    if (shape.getTechVia()) {
      wlyr = shape.getTechVia()->getBottomLayer();
      wlyr_top = shape.getTechVia()->getTopLayer();
    } else if (shape.getVia()) {
      wlyr = shape.getVia()->getBottomLayer();
      wlyr_top = shape.getVia()->getTopLayer();
    }
  }
  dbMTerm* mterm = iterm->getMTerm();
  int px, py;
  iterm->getInst()->getOrigin(px, py);
  Point origin = Point(px, py);
  dbOrientType orient = iterm->getInst()->getOrient();
  dbTransform transform(orient, origin);

  dbSet<dbMPin> mpins = mterm->getMPins();
  dbSet<dbMPin>::iterator mpin_itr;
  for (mpin_itr = mpins.begin(); mpin_itr != mpins.end(); mpin_itr++) {
    dbMPin* mpin = *mpin_itr;
    dbSet<dbBox> boxes = mpin->getGeometry();
    dbSet<dbBox>::iterator box_itr;
    for (box_itr = boxes.begin(); box_itr != boxes.end(); box_itr++) {
      dbBox* box = *box_itr;
      Rect rect;
      if (box->isVia()) {
        dbTechVia* tv = box->getTechVia();
        if (tv->getTopLayer() == wlyr
            || (!wlyr_top && tv->getBottomLayer() == wlyr)
            || tv->getBottomLayer() == wlyr_top) {
          box->getBox(rect);
          transform.apply(rect);
          if (rect.intersects(wrect)) {
            return true;
          }
        }
      } else {
        if (box->getTechLayer() == wlyr || box->getTechLayer() == wlyr_top) {
          box->getBox(rect);
          transform.apply(rect);
          if (rect.intersects(wrect)) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

static bool checkBTermConnect(dbBTerm* bterm, dbShape& shape)
{
  // check that some iterm shape intersects/touches shape
  Rect wrect;
  shape.getBox(wrect);
  dbTechLayer* wlyr = shape.getTechLayer();
  dbTechLayer* wlyrT = NULL;
  if (shape.isVia()) {
    if (shape.getTechVia()) {
      wlyr = shape.getTechVia()->getBottomLayer();
      wlyrT = shape.getTechVia()->getTopLayer();
    }
  }
  dbShape pin;
  if (!bterm->getFirstPin(pin))  // TWG: added bpins
    return false;
  if (pin.isVia()) {
    // TODO
  } else {
    if (pin.getTechLayer() == wlyr || pin.getTechLayer() == wlyrT) {
      Rect rect;
      pin.getBox(rect);
      if (rect.intersects(wrect)) {
        return true;
      }
    }
  }
  return false;
}

void tmg_conn::checkConnected(bool verbose)
{
  struct itinfo
  {
    dbITerm* iterm;
    bool found;
  };
  dbSet<dbITerm> iterms = _net->getITerms();
  dbSet<dbITerm>::iterator itt;
  itinfo itV1[32];
  itinfo* itV2 = NULL;
  itinfo* itV = itV1;
  int itN = iterms.size();
  if (itN > 32) {
    itV = (itinfo*) malloc(itN * sizeof(itinfo));
    itV2 = itV;
  }
  itinfo* x;
  for (x = itV, itt = iterms.begin(); itt != iterms.end(); ++itt, ++x) {
    x->iterm = *itt;
    x->found = false;
  }
  struct btinfo
  {
    dbBTerm* bterm;
    bool found;
  };
  dbSet<dbBTerm> bterms = _net->getBTerms();
  dbSet<dbBTerm>::iterator btt;
  btinfo btV1[32];
  btinfo* btV2 = NULL;
  btinfo* btV = btV1;
  int btN = bterms.size();
  if (btN > 32) {
    btV = (btinfo*) malloc(btN * sizeof(btinfo));
    btV2 = btV;
  }
  btinfo* y;
  for (y = btV, btt = bterms.begin(); btt != bterms.end(); ++btt, ++y) {
    y->bterm = *btt;
    y->found = false;
  }
  int j;
  dbWire* wire = _net->getWire();
  dbWirePathItr pitr;
  dbWirePath path;
  dbWirePathShape pathShape;
  pitr.begin(wire);
  bool disc = false;
  while (pitr.getNextPath(path) && !disc) {
    bool path_start = true;
    while (pitr.getNextShape(pathShape)) {
      if (path_start) {
        if (path.iterm) {
          for (j = 0; j < itN; j++)
            if (itV[j].iterm == path.iterm)
              break;
          if (j == itN) {
            disc = true;
            break;
          }
          if (checkITermConnect(path.iterm, pathShape.shape)) {
            itV[j].found = true;
          }
        }
        if (path.bterm) {
          for (j = 0; j < btN; j++)
            if (btV[j].bterm == path.bterm)
              break;
          if (j == btN) {
            disc = true;
            break;
          }
          if (checkBTermConnect(path.bterm, pathShape.shape)) {
            btV[j].found = true;
          }
        }
      }
      path_start = false;
      if (pathShape.iterm) {
        for (j = 0; j < itN; j++)
          if (itV[j].iterm == pathShape.iterm)
            break;
        if (j == itN) {
          disc = true;
          break;
        }
        if (checkITermConnect(pathShape.iterm, pathShape.shape)) {
          itV[j].found = true;
        }
      }
      if (pathShape.bterm) {
        for (j = 0; j < btN; j++)
          if (btV[j].bterm == pathShape.bterm)
            break;
        if (j == btN) {
          disc = true;
          break;
        }
        if (checkBTermConnect(pathShape.bterm, pathShape.shape)) {
          btV[j].found = true;
        }
      }
    }
  }
  for (j = 0; j < itN; j++)
    if (!itV[j].found) {
      disc = true;
    }
  for (j = 0; j < btN; j++)
    if (!btV[j].found)
      disc = true;
  if (disc) {
    _net->setDisconnected(true);
    if (verbose) {
      notice(0,
             "disconnected net %d %s\n",
             _net->getId(),
             _net->getName().c_str());
    }
  }

  if (itV2)
    free(itV2);
  if (btV2)
    free(btV2);
}

}  // namespace odb
