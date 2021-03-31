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

#include "dbWire.h"

#include <algorithm>

#include "db.h"
#include "dbBlock.h"
#include "dbBlockCallBackObj.h"
#include "dbNet.h"
#include "dbRtTree.h"
#include "dbShape.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayerRule.h"
#include "dbVia.h"
#include "dbWireOpcode.h"
#include "utl/Logger.h"
namespace odb {

template class dbTable<_dbWire>;
static void set_symmetric_diff(dbDiff& diff,
                               std::vector<dbShape*>& lhs,
                               std::vector<dbShape*>& rhs);
static void out(dbDiff& diff, char side, dbShape* s);

bool _dbWire::operator==(const _dbWire& rhs) const
{
  if (_flags._is_global != rhs._flags._is_global)
    return false;

  if (_data != rhs._data)
    return false;

  if (_opcodes != rhs._opcodes)
    return false;

  if (_net != rhs._net)
    return false;

  return true;
}

void _dbWire::differences(dbDiff& diff,
                          const char* field,
                          const _dbWire& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._is_global);
  DIFF_FIELD_NO_DEEP(_net);

  if (!diff.deepDiff()) {
    DIFF_VECTOR(_data);
    DIFF_VECTOR(_opcodes);
  } else {
    if ((_data != rhs._data) || (_opcodes != rhs._opcodes)) {
      dbWireShapeItr itr;
      dbShape s;

      std::vector<dbShape*> lhs_vec;
      for (itr.begin((dbWire*) this); itr.next(s);)
        lhs_vec.push_back(new dbShape(s));

      std::vector<dbShape*> rhs_vec;
      for (itr.begin((dbWire*) &rhs); itr.next(s);)
        rhs_vec.push_back(new dbShape(s));

      set_symmetric_diff(diff, lhs_vec, rhs_vec);

      std::vector<dbShape*>::iterator sitr;
      for (sitr = lhs_vec.begin(); sitr != lhs_vec.begin(); ++sitr)
        delete *sitr;

      for (sitr = rhs_vec.begin(); sitr != rhs_vec.begin(); ++sitr)
        delete *sitr;
    }
  }

  DIFF_END
}

void _dbWire::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._is_global);
  DIFF_OUT_FIELD_NO_DEEP(_net);

  if (!diff.deepDiff()) {
    DIFF_OUT_VECTOR(_data);
    DIFF_OUT_VECTOR(_opcodes);
  } else {
    dbWireShapeItr itr;
    dbShape s;
    for (itr.begin((dbWire*) this); itr.next(s);) {
      odb::out(diff, side, &s);
    }
  }

  DIFF_END
}

dbOStream& operator<<(dbOStream& stream, const _dbWire& wire)
{
  uint* bit_field = (uint*) &wire._flags;
  stream << *bit_field;
  stream << wire._data;
  stream << wire._opcodes;
  stream << wire._net;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbWire& wire)
{
  uint* bit_field = (uint*) &wire._flags;
  stream >> *bit_field;
  stream >> wire._data;
  stream >> wire._opcodes;
  stream >> wire._net;
  return stream;
}

class dbDiffShapeCmp
{
 public:
  int operator()(dbShape* t1, dbShape* t2) { return *t1 < *t2; }
};

void set_symmetric_diff(dbDiff& diff,
                        std::vector<dbShape*>& lhs,
                        std::vector<dbShape*>& rhs)
{
  std::sort(lhs.begin(), lhs.end(), dbDiffShapeCmp());
  std::sort(rhs.begin(), rhs.end(), dbDiffShapeCmp());

  std::vector<dbShape*>::iterator end;
  std::vector<dbShape*> symmetric_diff;

  symmetric_diff.resize(lhs.size() + rhs.size());

  end = std::set_symmetric_difference(lhs.begin(),
                                      lhs.end(),
                                      rhs.begin(),
                                      rhs.end(),
                                      symmetric_diff.begin(),
                                      dbDiffShapeCmp());

  std::vector<dbShape*>::iterator i1 = lhs.begin();
  std::vector<dbShape*>::iterator i2 = rhs.begin();
  std::vector<dbShape*>::iterator sd = symmetric_diff.begin();

  while ((i1 != lhs.end()) && (i2 != rhs.end())) {
    dbShape* o1 = *i1;
    dbShape* o2 = *i2;

    if (o1 == *sd) {
      out(diff, dbDiff::LEFT, o1);
      ++i1;
      ++sd;
    } else if (o2 == *sd) {
      out(diff, dbDiff::RIGHT, o2);
      ++i2;
      ++sd;
    } else  // equal keys
    {
      ++i1;
      ++i2;
    }
  }

  for (; i1 != lhs.end(); ++i1) {
    dbShape* o1 = *i1;
    out(diff, dbDiff::LEFT, o1);
  }

  for (; i2 != rhs.end(); ++i2) {
    dbShape* o2 = *i2;
    out(diff, dbDiff::RIGHT, o2);
  }
}

void out(dbDiff& diff, char side, dbShape* s)
{
  switch (s->getType()) {
    case dbShape::VIA: {
      dbVia* v = s->getVia();
      std::string n = v->getName();
      int x, y;
      s->getViaXY(x, y);
      diff.report("%c VIA %s (%d %d)\n", side, n.c_str(), x, y);
      break;
    }

    case dbShape::TECH_VIA: {
      dbTechVia* v = s->getTechVia();
      std::string n = v->getName();
      int x, y;
      s->getViaXY(x, y);
      diff.report("%c VIA %s (%d %d)\n", side, n.c_str(), x, y);
      break;
    }

    case dbShape::SEGMENT: {
      dbTechLayer* l = s->getTechLayer();
      std::string n = l->getName();
      diff.report("%c BOX %s (%d %d) (%d %d)\n",
                  side,
                  n.c_str(),
                  s->xMin(),
                  s->yMin(),
                  s->xMax(),
                  s->yMax());
      break;
    }
    default:
      break;  // Wall
  }
}

//
// DB wire methods here
//

dbBlock* dbWire::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

dbNet* dbWire::getNet()
{
  _dbWire* wire = (_dbWire*) this;

  if (wire->_net == 0)
    return NULL;

  _dbBlock* block = (_dbBlock*) wire->getOwner();
  return (dbNet*) block->_net_tbl->getPtr(wire->_net);
}

bool dbWire::isGlobalWire()
{
  _dbWire* wire = (_dbWire*) this;
  return wire->_flags._is_global == 1;
}

uint dbWire::equal(dbWire* target)
{
  _dbWire* src = (_dbWire*) this;
  _dbWire* tgt = (_dbWire*) target;
  // if (src->_flags != tgt->_flags)
  //    return false;
  uint wsize = src->_data.size();
  if (wsize != tgt->_data.size())
    return 10;
  uint pjunction = 0;
  for (uint idx = 0; idx < wsize; idx++) {
    unsigned char src_op = src->_opcodes[idx] & WOP_OPCODE_MASK;
    unsigned char tgt_op = tgt->_opcodes[idx] & WOP_OPCODE_MASK;

    if (src_op != tgt_op)
      return (1 + pjunction);

    if (src_op == WOP_ITERM || src_op == WOP_BTERM)
      continue;

    if (src->_data[idx] != tgt->_data[idx])
      return (2 + pjunction);

    if (src_op == WOP_COLINEAR)
      continue;

    if (src_op == WOP_JUNCTION)
      pjunction = 10;
    else
      pjunction = 0;
  }
  return 0;
}

/************************************************ dimitris_fix LOOK_AGAIN

void dbWire::match(dbWire* target)
{
    _dbWire * src = (_dbWire *) this;
    _dbWire * tgt = (_dbWire *) target;
    uint pjunction = 0;
    if (src->_data.size() != tgt->_data.size())
        pjunction = 0;  // bp neq
    uint wsize = src->_data.size()<tgt->_data.size() ? src->_data.size() :
tgt->_data.size(); for (uint idx = 0; idx < wsize; idx++)
    {
        unsigned char src_op = src->_opcodes[idx]&WOP_OPCODE_MASK;
        unsigned char tgt_op = tgt->_opcodes[idx]&WOP_OPCODE_MASK;

        if (src_op != tgt_op )
            continue;  // bp neq
        if (src_op == WOP_ITERM || src_op  == WOP_BTERM || src_op ==
WOP_BTERM_MAP ) continue; if (src->_data[idx] != tgt->_data[idx]) continue;  //
bp neq if (src_op == WOP_COLINEAR) continue; if (src_op == WOP_JUNCTION)
            pjunction = 10;
        else
            pjunction = 0;
    }
    return;
}
****************************************************************************************/

//

void dbWire::addOneSeg(unsigned char op,
                       int value,
                       uint jj,
                       int* did,
                       dbRSeg** new_rsegs)
{
  _dbWire* wire = (_dbWire*) this;
  did[jj] = wire->length();
  if (new_rsegs && new_rsegs[jj])
    new_rsegs[jj]->updateShapeId(did[jj]);
  else
    wire = (_dbWire*) this;  // zzzz bp
  wire->_data.push_back(value);
  wire->_opcodes.push_back(op);
}

void dbWire::addOneSeg(unsigned char op, int value)
{
  _dbWire* wire = (_dbWire*) this;
  wire->_data.push_back(value);
  wire->_opcodes.push_back(op);
}

uint dbWire::getTermJid(int termid)
{
  _dbWire* wire = (_dbWire*) this;
  int topcd = WOP_ITERM;
  int ttid = termid;
  if (termid < 0) {
    topcd = WOP_BTERM;
    ttid = -termid;
  }
  uint wlen = wire->length();
  uint jj;
  for (jj = 0; jj < wlen; jj++) {
    if ((wire->_opcodes[jj] & WOP_OPCODE_MASK) == topcd) {
      if (wire->_data[jj] == ttid)
        break;
      else
        continue;
    }
  }
  if (jj == wlen)
    return 0;
  jj--;
  if ((wire->_opcodes[jj] & WOP_OPCODE_MASK) == WOP_PROPERTY)
    jj--;
  return jj;
}

void dbWire::donateWireSeg(dbWire* w1, dbRSeg** new_rsegs)
{
  _dbWire* wire = (_dbWire*) this;
  //_dbWire * wire1 = (_dbWire *) w1;
  uint wlen = wire->length();
  int* destid = (int*) calloc(wlen, sizeof(int));
  // uint jj, did;
  uint jj;
  int data;
  unsigned char opcode;
  int opcd;
  // int xx, yy;
  // dbTechLayer *layer;
  // int njid;
  for (jj = 0; jj < wlen; jj++) {
    opcode = wire->_opcodes[jj];
    opcd = opcode & WOP_OPCODE_MASK;
    data = wire->_data[jj];
    if (opcd == WOP_ITERM || opcd == WOP_BTERM || opcd == WOP_NOP)
      continue;
    if (opcd == WOP_JUNCTION) {
      data = destid[data];
    }
    if ((opcd == WOP_SHORT) || (opcd == WOP_VWIRE)) {
      jj++;  // discard the WOP_OPERAND (jid)
      w1->addOneSeg(WOP_PATH | (opcode & WOP_WIRE_TYPE_MASK),
                    data,
                    jj,
                    destid,
                    new_rsegs);
      continue;
    }
    w1->addOneSeg(opcode, data, jj, destid, new_rsegs);
  }
}

void dbWire::shuffleWireSeg(dbNet** newNets, dbRSeg** new_rsegs)
{
  _dbWire* wire = (_dbWire*) this;
  uint wlen = wire->length();
  int* destid = (int*) calloc(wlen, sizeof(int));
  // dbTechLayer *llayer;
  // dimitri_fix
  dbTechLayer* llayer = NULL;
  dbTechLayer** jlayer = (dbTechLayer**) calloc(wlen, sizeof(dbTechLayer*));
  // int xx, yy;
  // dimitri_fix
  int xx = 0;
  int yy = 0;

  int* jxx = (int*) calloc(wlen, sizeof(int));
  int* jyy = (int*) calloc(wlen, sizeof(int));
  // int wwtype;
  // dimitri_fix
  int wwtype = 0;
  int* jwtype = (int*) calloc(wlen, sizeof(int));
  // int rrule;
  // dimitri_fix
  int rrule = 0;
  int* jrule = (int*) calloc(wlen, sizeof(int));
  int data;
  unsigned char opcode;
  int opcd;
  // uint jj, j1, j2, did;
  uint jj, j1, j2;

  // int njid;
  dbNet* leadNewNet = newNets[0];
  dbWire* twire = leadNewNet->getWire();
  if (twire == NULL)
    twire = dbWire::create(leadNewNet);
  dbWire* rwire = NULL;
  dbWire* fwire = NULL;
  bool newWire = false;

  if (twire == this) {
    newWire = true;
    rwire = dbWire::create(getBlock());
  }
  for (jj = 0; jj < wlen; jj++) {
    opcode = wire->_opcodes[jj];
    opcd = opcode & WOP_OPCODE_MASK;
    if (opcd == WOP_ITERM || opcd == WOP_BTERM || opcd == WOP_NOP)
      continue;
    data = wire->_data[jj];
    switch (opcd) {
      case WOP_PATH:
      case WOP_SHORT:
      case WOP_VWIRE: {
        rrule = 0;
        llayer = dbTechLayer::getTechLayer(getDb()->getTech(), data);
        // wwtype = dbWireType((dbWireType::value)opcode & WOP_WIRE_TYPE_MASK);
        wwtype = opcode & WOP_WIRE_TYPE_MASK;
        break;
      }
      case WOP_VIA: {
        dbVia* via = dbVia::getVia(getBlock(), data);

        if (opcode & WOP_VIA_EXIT_TOP)
          llayer = via->getTopLayer();
        else
          llayer = via->getBottomLayer();
        break;
      }
      case WOP_TECH_VIA: {
        dbTechVia* via = dbTechVia::getTechVia(getDb()->getTech(), data);

        if (opcode & WOP_VIA_EXIT_TOP)
          llayer = via->getTopLayer();
        else
          llayer = via->getBottomLayer();
        break;
      }
      case WOP_X: {
        xx = data;
        break;
      }
      case WOP_Y: {
        yy = data;
        break;
      }
      case WOP_RULE: {
        rrule = data;
        break;
      }
      case WOP_JUNCTION: {
        j1 = data;
        xx = jxx[j1];
        yy = jyy[j1];
        wwtype = jwtype[j1];
        llayer = jlayer[j1];
        rrule = jrule[j1];
        data = destid[j1];
        j2 = j1;
        while (newNets[j2] == NULL)
          j2--;
        newNets[j1] = newNets[j2];
        newNets[jj - 1] = newNets[j2];  // not jj; avoid inducing WOP_PATH
        twire = newNets[j2]->getWire();
        break;
      }
    }
    if ((opcd == WOP_SHORT) || (opcd == WOP_VWIRE)) {
      j1 = wire->_data[++jj];
      j2 = 0;
      if (newNets[jj + 2])
        j2 = jj + 2;
      else if (newNets[jj + 3])
        j2 = jj + 3;
      if (j2) {
        newNets[jj] = newNets[j2];
        newNets[j2] = NULL;
      } else {
        j2 = j1;
        while (newNets[j2] == NULL)
          j2--;
        newNets[j1] = newNets[j2];
        newNets[jj] = newNets[j2];
      }
      twire = newNets[jj]->getWire();
      fwire = twire == this ? rwire : twire;
      fwire->addOneSeg(WOP_PATH | wwtype, data, jj, destid, new_rsegs);
      continue;
    } else if (opcd == WOP_PATH
               && (newNets[jj + 2] != NULL || newNets[jj + 3] != NULL)) {
      j2 = newNets[jj + 2] != NULL ? jj + 2 : jj + 3;
      newNets[jj] = newNets[j2];
      newNets[j2] = NULL;
      twire = newNets[jj]->getWire();
      if (twire == NULL)
        twire = dbWire::create(newNets[jj]);
      fwire = twire == this ? rwire : twire;
      fwire->addOneSeg(opcode, data, jj, destid, new_rsegs);
      continue;
    }
    if (jj != 0 && newNets[jj] != NULL) {
      fwire = twire == this ? rwire : twire;
      fwire->addOneSeg(opcode, data, jj, destid, new_rsegs);
      j1 = jj + 1;
      // bool extension = false;
      if ((wire->_opcodes[j1] & WOP_OPCODE_MASK) == WOP_OPERAND) {
        // extension = true;
        fwire->addOneSeg(wire->_opcodes[j1], wire->_data[j1]);
        j1++;
      }
      if ((wire->_opcodes[j1] & WOP_OPCODE_MASK) == WOP_PROPERTY) {
        fwire->addOneSeg(wire->_opcodes[j1], wire->_data[j1]);
        j1++;
      }
      // new PATH for twire
      twire = newNets[jj]->getWire();
      if (twire == NULL)
        twire = dbWire::create(newNets[jj]);
      fwire = twire == this ? rwire : twire;
      fwire->addOneSeg(WOP_PATH | wwtype, llayer->getImpl()->getOID());
      bool extension = false;
      if ((wire->_opcodes[jj + 1] & WOP_OPCODE_MASK) == WOP_OPERAND)
        extension = true;
      if (rrule == 0) {
        fwire->addOneSeg(WOP_X | WOP_DEFAULT_WIDTH, xx);
        if (extension)
          fwire->addOneSeg(WOP_Y | WOP_EXTENSION | WOP_DEFAULT_WIDTH,
                           yy,
                           jj,
                           destid,
                           new_rsegs);
        else
          fwire->addOneSeg(
              WOP_Y | WOP_DEFAULT_WIDTH, yy, jj, destid, new_rsegs);
      } else {
        fwire->addOneSeg(WOP_RULE, rrule);
        fwire->addOneSeg(WOP_X, xx);
        if (extension)
          fwire->addOneSeg(WOP_Y | WOP_EXTENSION, yy, jj, destid, new_rsegs);
        else
          fwire->addOneSeg(WOP_Y, yy, jj, destid, new_rsegs);
      }
      jxx[jj] = xx;
      jyy[jj] = yy;
      jlayer[jj] = llayer;
      jwtype[jj] = wwtype;
      jrule[jj] = rrule;
      // jj = j1 - 1;
      continue;
    }
    fwire = twire == this ? rwire : twire;
    fwire->addOneSeg(opcode, data, jj, destid, new_rsegs);
    jxx[jj] = xx;
    jyy[jj] = yy;
    jlayer[jj] = llayer;
    jwtype[jj] = wwtype;
    jrule[jj] = rrule;
  }

  if (newWire)
    rwire->attach(leadNewNet);
}

bool dbWire::getBBox(Rect& bbox)
{
  bbox.reset(INT_MAX, INT_MAX, INT_MIN, INT_MIN);
  dbWireShapeItr itr;
  dbShape s;
  Rect r;
  uint cnt = 0;

  for (itr.begin(this); itr.next(s);) {
    s.getBox(r);
    bbox.merge(r);
    ++cnt;
  }

  return cnt > 0;
}

#define DB_WIRE_SHAPE_INVALID_SHAPE_ID 0

void dbWire::getShape(int shape_id, dbShape& shape)
{
  _dbWire* wire = (_dbWire*) this;
  ZASSERT((0 <= shape_id) && (shape_id < (int) wire->length()));
  unsigned char opcode = wire->_opcodes[shape_id];

  switch (opcode & WOP_OPCODE_MASK) {
    case WOP_X:
    case WOP_Y:
    case WOP_COLINEAR: {
      getSegment(shape_id, shape);
      return;
    }

    case WOP_VIA: {
      dbBlock* block = (dbBlock*) wire->getOwner();
      dbTech* tech = getDb()->getTech();
      int operand = wire->_data[shape_id];
      dbVia* via = dbVia::getVia(block, operand);
      dbBox* box = via->getBBox();

      if (box == NULL)
        return;

      WirePoint pnt;
      // dimitri_fix
      pnt._x = 0;
      pnt._y = 0;
      getPrevPoint(
          tech, block, wire->_opcodes, wire->_data, shape_id, false, pnt);
      Rect b;
      box->getBox(b);
      int xmin = b.xMin() + pnt._x;
      int ymin = b.yMin() + pnt._y;
      int xmax = b.xMax() + pnt._x;
      int ymax = b.yMax() + pnt._y;
      Rect r(xmin, ymin, xmax, ymax);
      shape.setVia(via, r);
      return;
    }

    case WOP_TECH_VIA: {
      dbBlock* block = (dbBlock*) wire->getOwner();
      dbTech* tech = getDb()->getTech();
      int operand = wire->_data[shape_id];
      dbTechVia* via = dbTechVia::getTechVia(tech, operand);
      dbBox* box = via->getBBox();

      if (box == NULL)
        return;

      // dimitri_fix LOOK_AGAIN WirePoint pnt;
      WirePoint pnt;
      pnt._x = 0;
      pnt._y = 0;
      getPrevPoint(
          tech, block, wire->_opcodes, wire->_data, shape_id, false, pnt);
      Rect b;
      box->getBox(b);
      int xmin = b.xMin() + pnt._x;
      int ymin = b.yMin() + pnt._y;
      int xmax = b.xMax() + pnt._x;
      int ymax = b.yMax() + pnt._y;
      Rect r(xmin, ymin, xmax, ymax);
      shape.setVia(via, r);
      return;
    }

    default:
      ZASSERT(DB_WIRE_SHAPE_INVALID_SHAPE_ID);
  }
}

void dbWire::getCoord(int jid, int& x, int& y)
{
  _dbWire* wire = (_dbWire*) this;
  ZASSERT((0 <= jid) && (jid < (int) wire->length()));
  dbBlock* block = (dbBlock*) wire->getOwner();
  dbTech* tech = getDb()->getTech();
  // dimitri_fix LOOK_AGAIN WirePoint pnt;
  WirePoint pnt;
  pnt._x = 0;
  pnt._y = 0;
  getPrevPoint(tech, block, wire->_opcodes, wire->_data, jid, false, pnt);
  x = pnt._x;
  y = pnt._y;
}

bool dbWire::getProperty(int jid, int& prpty)
{
  _dbWire* wire = (_dbWire*) this;
  int wlen = (int) wire->length();
  ZASSERT(0 <= jid && jid < wlen);
  unsigned char op = wire->_opcodes[jid] & WOP_OPCODE_MASK;
  if (op == WOP_COLINEAR || op == WOP_RECT) {
    prpty = 0;
    return true;
  }
  ZASSERT(op == WOP_X || op == WOP_Y);
  ZASSERT(jid + 1 < wlen);
  if ((wire->_opcodes[jid + 1] & WOP_OPCODE_MASK) == WOP_PROPERTY) {
    prpty = wire->_data[jid + 1];
    return true;
  }
  ZASSERT(jid + 2 < wlen);
  if ((wire->_opcodes[jid + 2] & WOP_OPCODE_MASK) == WOP_PROPERTY) {
    prpty = wire->_data[jid + 2];
    return true;
  }
  return false;
}

bool dbWire::setProperty(int jid, int prpty)
{
  _dbWire* wire = (_dbWire*) this;
  int wlen = (int) wire->length();
  ZASSERT(0 <= jid && jid < wlen);
  if ((wire->_opcodes[jid] & WOP_OPCODE_MASK) == WOP_COLINEAR)
    return true;
  ZASSERT((wire->_opcodes[jid] & WOP_OPCODE_MASK) == WOP_X
          || (wire->_opcodes[jid] & WOP_OPCODE_MASK) == WOP_Y);
  ZASSERT(jid + 1 < wlen);
  if ((wire->_opcodes[jid + 1] & WOP_OPCODE_MASK) == WOP_PROPERTY) {
    wire->_data[jid + 1] = prpty;
    return true;
  }
  ZASSERT(jid + 2 < wlen);
  if ((wire->_opcodes[jid + 2] & WOP_OPCODE_MASK) == WOP_PROPERTY) {
    wire->_data[jid + 2] = prpty;
    return true;
  }
  return false;
}

int dbWire::getData(int idx)
{
  _dbWire* wire = (_dbWire*) this;
  ZASSERT((0 <= idx) && (idx < (int) wire->length()));
  return (wire->_data[idx]);
}

unsigned char dbWire::getOpcode(int idx)
{
  _dbWire* wire = (_dbWire*) this;
  ZASSERT((0 <= idx) && (idx < (int) wire->length()));
  return (wire->_opcodes[idx]);
}

void dbWire::printWire()
{
  _dbWire* wire = (_dbWire*) this;
  int tid = (int) wire->length() - 1;
  printWire(NULL, 0, tid);
}

void dbWire::printWire(FILE* fp, int fid, int tid)
{
  if (!fp)
    fp = stdout;
  _dbWire* wire = (_dbWire*) this;
  if (fid == 0 && tid == 0)
    tid = (int) wire->length() - 1;
  ZASSERT((0 <= fid) && (fid < (int) wire->length()));
  ZASSERT((0 <= tid) && (tid < (int) wire->length()));
  int jj, opcode;
  dbTechLayer* llayer;
  int data;
  for (jj = fid; jj <= tid; jj++) {
    opcode = wire->_opcodes[jj] & WOP_OPCODE_MASK;
    data = wire->_data[jj];
    fprintf(fp, "  %d    %d    ", jj, opcode);
    switch (opcode) {
      case WOP_PATH: {
        fprintf(fp, "WOP_PATH :");
        break;
      }
      case WOP_SHORT: {
        fprintf(fp, "WOP_SHORT :");
        break;
      }
      case WOP_VWIRE: {
        fprintf(fp, "WOP_VWIRE :");
        break;
      }
      case WOP_JUNCTION: {
        fprintf(fp, "WOP_JUNCTION :");
        break;
      }
      case WOP_RULE: {
        fprintf(fp, "WOP_RULE :");
        break;
      }
      case WOP_X: {
        fprintf(fp, "WOP_X :");
        break;
      }
      case WOP_Y: {
        fprintf(fp, "WOP_Y :");
        break;
      }
      case WOP_COLINEAR: {
        fprintf(fp, "WOP_COLINEAR :");
        break;
      }
      case WOP_VIA: {
        fprintf(fp, "WOP_VIA :");
        break;
      }
      case WOP_TECH_VIA: {
        fprintf(fp, "WOP_TECH_VIA :");
        break;
      }
      case WOP_ITERM: {
        fprintf(fp, "WOP_ITERM :");
        break;
      }
      case WOP_BTERM: {
        fprintf(fp, "WOP_BTERM :");
        break;
      }
      case WOP_RECT: {
        fprintf(fp, "WOP_RECT :");
        break;
      }
      case WOP_OPERAND: {
        fprintf(fp, "WOP_OPERAND :");
        break;
      }
      case WOP_PROPERTY: {
        fprintf(fp, "WOP_PROPERTY :");
        break;
      }
      case WOP_NOP: {
        fprintf(fp, "WOP_NOP :");
        break;
      }
    }
    fprintf(fp, "    %d", data);
    switch (opcode) {
      case WOP_PATH:
      case WOP_SHORT:
      case WOP_VWIRE: {
        llayer = dbTechLayer::getTechLayer(getDb()->getTech(), data);
        fprintf(fp,
                " ( %d %s )",
                llayer->getRoutingLevel(),
                llayer->getConstName());
        break;
      }
      case WOP_VIA: {
        dbVia* via = dbVia::getVia(getBlock(), data);
        fprintf(fp,
                " ( %d %d %s )",
                via->getTopLayer()->getRoutingLevel(),
                via->getBottomLayer()->getRoutingLevel(),
                via->getConstName());
        break;
      }
      case WOP_TECH_VIA: {
        dbTechVia* via = dbTechVia::getTechVia(getDb()->getTech(), data);
        fprintf(fp,
                " ( %d %d %s )",
                via->getTopLayer()->getRoutingLevel(),
                via->getBottomLayer()->getRoutingLevel(),
                via->getConstName());
        break;
      }
      case WOP_ITERM: {
        dbITerm* iterm = dbITerm::getITerm(getBlock(), data);
        fprintf(fp,
                " I%d/%s",
                iterm->getInst()->getId(),
                iterm->getMTerm()->getName().c_str());
        break;
      }
      case WOP_BTERM: {
        dbBTerm* bterm = dbBTerm::getBTerm(getBlock(), data);
        fprintf(fp, " %s", bterm->getName().c_str());
        break;
      }
    }
    fprintf(fp, "\n");
  }
}

uint64 dbWire::getLength()
{
  dbWireShapeItr shapes;
  dbShape s;
  int tplen;
  uint64 rtlen = 0;
  for (shapes.begin(this); shapes.next(s);) {
    if (!s.isVia()) {
      tplen = s.getDX() - s.getDY();
      if (tplen < 0)
        tplen = -tplen;
      rtlen += tplen;
    }
  }

  return rtlen;
}

uint dbWire::length()
{
  _dbWire* wire = (_dbWire*) this;
  return wire->length();
}

uint dbWire::count()
{
  uint jj;
  int opcode;
  uint cnt = 0;
  _dbWire* wire = (_dbWire*) this;
  for (jj = 0; jj < wire->length(); jj++) {
    opcode = wire->_opcodes[jj] & WOP_OPCODE_MASK;
    if (opcode == WOP_X || opcode == WOP_Y)
      cnt++;
  }
  return cnt - 2;
}

//
// getSegment: This code implements a state machine to reverse engineer the
// segment from the encoding
//
enum MachineInput
{
  XInput = 0,
  YInput = 1,
  CInput = 2,
};

enum Coord
{
  XCoord = 0,
  YCoord = 1,
  None = 2
};

static int nextState[13][3] = {
    /*            X   Y   C */
    /*  0  */ {1, 2, 3},
    /*  1  */ {4, 5, 6},
    /*  2  */ {7, 8, 9},
    /*  3  */ {10, 11, 12},
    /*  4  */ {4, 13, 4},
    /*  5  */ {13, 5, 5},
    /*  6  */ {4, 5, 6},
    /*  7  */ {7, 13, 7},
    /*  8  */ {13, 8, 8},
    /*  9  */ {7, 8, 9},
    /*  10 */ {10, 13, 10},
    /*  11 */ {13, 11, 11},
    /*  12 */ {10, 11, 12}};

static Coord curCoord[13][3] = {
    /*            X        Y        C */
    /*  0  */ {None, None, None},
    /*  1  */ {XCoord, YCoord, None},
    /*  2  */ {XCoord, YCoord, None},
    /*  3  */ {XCoord, YCoord, None},
    /*  4  */ {None, YCoord, None},
    /*  5  */ {XCoord, None, None},
    /*  6  */ {XCoord, YCoord, None},
    /*  7  */ {None, YCoord, None},
    /*  8  */ {XCoord, None, None},
    /*  9  */ {XCoord, YCoord, None},
    /*  10 */ {None, YCoord, None},
    /*  11 */ {XCoord, None, None},
    /*  12 */ {XCoord, YCoord, None}};

static Coord prevCoord[13][3] = {
    /*            X        Y        C */
    /*  0  */ {XCoord, YCoord, None},
    /*  1  */ {None, YCoord, None},
    /*  2  */ {XCoord, None, None},
    /*  3  */ {XCoord, YCoord, None},
    /*  4  */ {None, YCoord, None},
    /*  5  */ {None, None, None},
    /*  6  */ {None, YCoord, None},
    /*  7  */ {None, None, None},
    /*  8  */ {XCoord, None, None},
    /*  9  */ {XCoord, None, None},
    /*  10 */ {None, YCoord, None},
    /*  11 */ {XCoord, None, None},
    /*  12 */ {XCoord, YCoord, None}};

void dbWire::getSegment(int shape_id, dbShape& shape)
{
  _dbWire* wire = (_dbWire*) this;
  dbTechLayer* layer = NULL;

  int width = 0;
  bool found_width = false;
  bool default_width = false;

  int idx = shape_id;
  int state = 0;
  unsigned char opcode;
  int input;

  int cur[3];
  int prev[3];
  int cur_ext = 0;
  int prev_ext = 0;
  bool has_prev_ext = false;
  bool has_cur_ext = false;
  bool ignore_ext = false;

decode_loop : {
  ZASSERT(idx >= 0);
  opcode = wire->_opcodes[idx];

  switch (opcode & WOP_OPCODE_MASK) {
    case WOP_JUNCTION:
      idx = wire->_data[idx];
      ignore_ext = true;
      goto decode_loop;

    case WOP_RULE:
      if (found_width == false) {
        found_width = true;

        if (opcode & WOP_BLOCK_RULE) {
          dbBlock* block = (dbBlock*) wire->getOwner();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(block, wire->_data[idx]);
          width = rule->getWidth();
        } else {
          dbTech* tech = getDb()->getTech();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(tech, wire->_data[idx]);
          width = rule->getWidth();
        }
      }

      break;

    case WOP_X:
      input = XInput;
      goto state_machine_update;

    case WOP_Y:
      input = YInput;
      goto state_machine_update;

    case WOP_COLINEAR:
      input = CInput;
      goto state_machine_update;

    case WOP_VIA:
      if (layer == NULL) {
        dbBlock* block = (dbBlock*) wire->getOwner();
        dbVia* via = dbVia::getVia(block, wire->_data[idx]);

        if (opcode & WOP_VIA_EXIT_TOP)
          layer = via->getTopLayer();
        else
          layer = via->getBottomLayer();
      }
      ignore_ext = true;
      break;

    case WOP_TECH_VIA:
      if (layer == NULL) {
        dbTech* tech = getDb()->getTech();
        dbTechVia* via = dbTechVia::getTechVia(tech, wire->_data[idx]);

        if (opcode & WOP_VIA_EXIT_TOP)
          layer = via->getTopLayer();
        else
          layer = via->getBottomLayer();
      }
      ignore_ext = true;
      break;
  }

  --idx;
  goto decode_loop;

state_machine_update : {
  if (state == 0) {
    if (opcode & WOP_DEFAULT_WIDTH) {
      found_width = true;
      default_width = true;
    }

    if (opcode & WOP_EXTENSION) {
      prev_ext = wire->_data[idx + 1];
      has_prev_ext = true;
    }
  } else if (state <= 3) {
    if ((opcode & WOP_EXTENSION) && !ignore_ext) {
      cur_ext = wire->_data[idx + 1];
      has_cur_ext = true;
    }
  }

  int value = wire->_data[idx];
  cur[curCoord[state][input]] = value;
  prev[prevCoord[state][input]] = value;
  state = nextState[state][input];
  --idx;

  if (state < 13)
    goto decode_loop;
}
}

  while ((layer == NULL) || (found_width == false)) {
    ZASSERT(idx >= 0);
    opcode = wire->_opcodes[idx];

    switch (opcode & WOP_OPCODE_MASK) {
      case WOP_PATH:
      case WOP_SHORT:
      case WOP_VWIRE: {
        if (layer == NULL) {
          dbTech* tech = getDb()->getTech();
          layer = dbTechLayer::getTechLayer(tech, wire->_data[idx]);
        }

        --idx;
        break;
      }

      case WOP_JUNCTION: {
        idx = wire->_data[idx];
        break;
      }

      case WOP_RULE: {
        found_width = true;
        if (opcode & WOP_BLOCK_RULE) {
          dbBlock* block = (dbBlock*) wire->getOwner();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(block, wire->_data[idx]);
          width = rule->getWidth();
        } else {
          dbTech* tech = getDb()->getTech();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(tech, wire->_data[idx]);
          width = rule->getWidth();
        }
        --idx;
        break;
      }

      case WOP_VIA: {
        if (layer == NULL) {
          dbBlock* block = (dbBlock*) wire->getOwner();
          dbVia* via = dbVia::getVia(block, wire->_data[idx]);

          if (opcode & WOP_VIA_EXIT_TOP)
            layer = via->getTopLayer();
          else
            layer = via->getBottomLayer();
        }
        --idx;
        break;
      }

      case WOP_TECH_VIA: {
        if (layer == NULL) {
          dbTech* tech = getDb()->getTech();
          dbTechVia* via = dbTechVia::getTechVia(tech, wire->_data[idx]);

          if (opcode & WOP_VIA_EXIT_TOP)
            layer = via->getTopLayer();
          else
            layer = via->getBottomLayer();
        }
        --idx;
        break;
      }

      default:
        --idx;
        break;
    }
  }

  int dw;

  if (default_width)
    dw = layer->getWidth() >> 1;
  else
    dw = width >> 1;

  shape.setSegment(prev[0],
                   prev[1],
                   prev_ext,
                   has_prev_ext,
                   cur[0],
                   cur[1],
                   cur_ext,
                   has_cur_ext,
                   dw,
                   layer);
}

//
// This version is optimized for the extraction code, where the layer is already
// known
//
void dbWire::getSegment(int shape_id, dbTechLayer* layer, dbShape& shape)
{
  _dbWire* wire = (_dbWire*) this;
  assert(layer);

  int width = 0;
  bool found_width = false;
  bool default_width = false;

  int idx = shape_id;
  int state = 0;
  unsigned char opcode;
  int input;

  int cur[3];
  int prev[3];
  int cur_ext = 0;
  int prev_ext = 0;
  bool has_prev_ext = false;
  bool has_cur_ext = false;
  bool ignore_ext = false;

decode_loop : {
  ZASSERT(idx >= 0);
  opcode = wire->_opcodes[idx];

  switch (opcode & WOP_OPCODE_MASK) {
    case WOP_JUNCTION:
      idx = wire->_data[idx];
      ignore_ext = true;
      goto decode_loop;

    case WOP_RULE:
      if (found_width == false) {
        found_width = true;

        if (opcode & WOP_BLOCK_RULE) {
          dbBlock* block = (dbBlock*) wire->getOwner();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(block, wire->_data[idx]);
          width = rule->getWidth();
        } else {
          dbTech* tech = getDb()->getTech();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(tech, wire->_data[idx]);
          width = rule->getWidth();
        }
      }

      break;

    case WOP_X:
      input = XInput;
      goto state_machine_update;

    case WOP_Y:
      input = YInput;
      goto state_machine_update;

    case WOP_COLINEAR:
      input = CInput;
      goto state_machine_update;
  }

  --idx;
  goto decode_loop;

state_machine_update : {
  if (state == 0) {
    if (opcode & WOP_DEFAULT_WIDTH) {
      found_width = true;
      default_width = true;
    }

    if (opcode & WOP_EXTENSION) {
      prev_ext = wire->_data[idx + 1];
      has_prev_ext = true;
    }
  } else if (state <= 3) {
    if ((opcode & WOP_EXTENSION) && !ignore_ext) {
      cur_ext = wire->_data[idx + 1];
      has_cur_ext = true;
    }
  }

  int value = wire->_data[idx];
  cur[curCoord[state][input]] = value;
  prev[prevCoord[state][input]] = value;
  state = nextState[state][input];
  --idx;

  if (state < 13)
    goto decode_loop;
}
}

  while (found_width == false) {
    ZASSERT(idx >= 0);
    opcode = wire->_opcodes[idx];

    switch (opcode & WOP_OPCODE_MASK) {
      case WOP_JUNCTION: {
        idx = wire->_data[idx];
        break;
      }

      case WOP_RULE: {
        found_width = true;
        if (opcode & WOP_BLOCK_RULE) {
          dbBlock* block = (dbBlock*) wire->getOwner();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(block, wire->_data[idx]);
          width = rule->getWidth();
        } else {
          dbTech* tech = getDb()->getTech();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(tech, wire->_data[idx]);
          width = rule->getWidth();
        }
        --idx;
        break;
      }
      default:
        --idx;
        break;
    }
  }

  int dw;

  if (default_width)
    dw = layer->getWidth() >> 1;
  else
    dw = width >> 1;

  shape.setSegment(prev[0],
                   prev[1],
                   prev_ext,
                   has_prev_ext,
                   cur[0],
                   cur[1],
                   cur_ext,
                   has_cur_ext,
                   dw,
                   layer);
}

inline unsigned char getPrevOpcode(_dbWire* wire, int& idx)
{
  --idx;

prevOpCode:
  assert(idx >= 0);
  unsigned char opcode = wire->_opcodes[idx];

  switch (opcode & WOP_OPCODE_MASK) {
    case WOP_PATH:
    case WOP_SHORT:
    case WOP_VWIRE:
    case WOP_JUNCTION:
      return opcode;

    case WOP_RULE:
      --idx;
      goto prevOpCode;

    case WOP_X:
    case WOP_Y:
    case WOP_COLINEAR:
    case WOP_VIA:
    case WOP_TECH_VIA:
      return opcode;

    default:
      --idx;
      goto prevOpCode;
  }
}

inline bool createVia(_dbWire* wire, int idx, dbShape& shape)
{
  dbBlock* block = (dbBlock*) wire->getOwner();
  dbTech* tech = wire->getDb()->getTech();
  int operand = wire->_data[idx];
  dbVia* via = dbVia::getVia(block, operand);
  dbBox* box = via->getBBox();

  if (box == NULL)
    return false;

  WirePoint pnt;
  // dimitri_fix
  pnt._x = 0;
  pnt._y = 0;
  getPrevPoint(tech, block, wire->_opcodes, wire->_data, idx, false, pnt);
  Rect b;
  box->getBox(b);
  int xmin = b.xMin() + pnt._x;
  int ymin = b.yMin() + pnt._y;
  int xmax = b.xMax() + pnt._x;
  int ymax = b.yMax() + pnt._y;
  Rect r(xmin, ymin, xmax, ymax);
  shape.setVia(via, r);
  return true;
}

inline bool createTechVia(_dbWire* wire, int idx, dbShape& shape)
{
  dbBlock* block = (dbBlock*) wire->getOwner();
  dbTech* tech = wire->getDb()->getTech();
  int operand = wire->_data[idx];
  dbTechVia* via = dbTechVia::getTechVia(tech, operand);
  dbBox* box = via->getBBox();

  if (box == NULL)
    return false;

  WirePoint pnt;
  // dimitri_fix
  pnt._x = 0;
  pnt._y = 0;
  getPrevPoint(tech, block, wire->_opcodes, wire->_data, idx, false, pnt);
  Rect b;
  box->getBox(b);
  int xmin = b.xMin() + pnt._x;
  int ymin = b.yMin() + pnt._y;
  int xmax = b.xMax() + pnt._x;
  int ymax = b.yMax() + pnt._y;
  Rect r(xmin, ymin, xmax, ymax);
  shape.setVia(via, r);
  return true;
}

// This function gets the previous via of this shape_id,
// if one exists.
bool dbWire::getPrevVia(int idx, dbShape& shape)
{
  _dbWire* wire = (_dbWire*) this;
  ZASSERT((0 < idx) && (idx < (int) wire->length()));

  unsigned char opcode;
  opcode = getPrevOpcode(wire, idx);

  switch (opcode & WOP_OPCODE_MASK) {
    case WOP_COLINEAR: {
      // special case: colinear point with ext starts a new segment
      //   idx-3   idx-2   idx-1      idx
      // ( X1 Y1 ) ( V ) ( X1 Y1 E) ( X1 Y2 )
      if (opcode & WOP_EXTENSION) {
        opcode = getPrevOpcode(wire, idx);

        switch (opcode & WOP_OPCODE_MASK) {
          case WOP_TECH_VIA:
            return createTechVia(wire, idx, shape);

          case WOP_VIA:
            return createVia(wire, idx, shape);

          default:
            break;
        }
      }

      break;
    }

    case WOP_TECH_VIA:
      return createTechVia(wire, idx, shape);

    case WOP_VIA:
      return createVia(wire, idx, shape);

    default:
      break;
  }

  return false;
}

//
// This function gets the next via of this shape_id,
// if one exists.
bool dbWire::getNextVia(int idx, dbShape& shape)
{
  _dbWire* wire = (_dbWire*) this;
  ZASSERT((0 < idx) && (idx < (int) wire->length()));
  ++idx;

nextOpCode:
  if ((uint) idx == wire->length())
    return false;

  unsigned char opcode = wire->_opcodes[idx];

  switch (opcode & WOP_OPCODE_MASK) {
    case WOP_PATH:
    case WOP_SHORT:
    case WOP_VWIRE:
    case WOP_JUNCTION:
      return false;

    case WOP_RULE:
      ++idx;
      goto nextOpCode;

    case WOP_X:
    case WOP_Y:
    case WOP_COLINEAR:
      return false;

    case WOP_TECH_VIA:
      return createTechVia(wire, idx, shape);

    case WOP_VIA:
      return createVia(wire, idx, shape);

    default:
      ++idx;
      goto nextOpCode;
  }
  return false;
}

bool dbWire::getViaBoxes(int via_shape_id, std::vector<dbShape>& shapes)
{
  dbShape via;

  getShape(via_shape_id, via);

  if (!via.isVia())
    return false;

  dbShape::getViaBoxes(via, shapes);
  return true;
}

void dbWire::append(dbWire* src_, bool singleSegmentWire)
{
  _dbWire* dst = (_dbWire*) this;
  _dbWire* src = (_dbWire*) src_;
  _dbBlock* src_block = (_dbBlock*) src->getOwner();
  _dbBlock* dst_block = (_dbBlock*) dst->getOwner();

  assert(dst->getDatabase() == src->getDatabase());

  // we can't move bterms or iterms of another block
  if (src_block != dst_block && !singleSegmentWire) {
    int i;
    int n = src->_opcodes.size();

    for (i = 0; i < n; ++i) {
      unsigned char opcode = src->_opcodes[i] & WOP_OPCODE_MASK;

      if (opcode == WOP_ITERM || opcode == WOP_BTERM)
        return;
    }
  }
  for (auto callback : ((_dbBlock*) getBlock())->_callbacks)
    callback->inDbWirePreAppend(src_, this);
  uint sz = dst->_opcodes.size();
  dst->_opcodes.insert(
      dst->_opcodes.end(), src->_opcodes.begin(), src->_opcodes.end());
  dst->_data.insert(dst->_data.end(), src->_data.begin(), src->_data.end());

  // fix up the dbVia's if needed...
  if (src_block != dst_block && !singleSegmentWire) {
    int i;
    int n = dst->_opcodes.size();

    for (i = sz; i < n; ++i) {
      unsigned char opcode = dst->_opcodes[i] & WOP_OPCODE_MASK;

      if (opcode == WOP_VIA) {
        uint vid = dst->_data[i];
        _dbVia* src_via = src_block->_via_tbl->getPtr(vid);
        dbVia* dst_via = ((dbBlock*) dst_block)->findVia(src_via->_name);

        // duplicate src-via in dst-block if needed
        if (dst_via == NULL)
          dst_via = dbVia::copy((dbBlock*) dst_block, (dbVia*) src_via);

        dst->_data[i] = dst_via->getImpl()->getOID();
      }
    }
  }

  // Fix up the junction-ids
  int i;
  int n = dst->_opcodes.size();

  for (i = sz; i < n; ++i) {
    unsigned char opcode = dst->_opcodes[i] & WOP_OPCODE_MASK;
    if ((opcode == WOP_SHORT) || (opcode == WOP_JUNCTION)
        || (opcode == WOP_VWIRE))
      dst->_data[i] += sz;
  }
  for (auto callback : ((_dbBlock*) getBlock())->_callbacks)
    callback->inDbWirePostAppend(src_, this);
}

void dbWire::attach(dbNet* net_)
{
  _dbWire* wire = (_dbWire*) this;
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) getBlock();
  assert(wire->_flags._is_global == 0);
  if (wire->_net == net->getOID() && net->_wire == wire->getOID())
    return;
  for (auto callback : block->_callbacks)
    callback->inDbWirePreAttach(this, net_);

  // dbWire * prev = net_->getWire();

  if (net->_wire != 0)
    dbWire::destroy(net_->getWire());

  if (wire->_net != 0)
    detach();

  wire->_net = net->getOID();
  net->_wire = wire->getOID();
  for (auto callback : block->_callbacks)
    callback->inDbWirePostAttach(this);
}

void dbWire::detach()
{
  _dbWire* wire = (_dbWire*) this;
  _dbBlock* block = (_dbBlock*) getBlock();
  assert(wire->_flags._is_global == 0);
  if (wire->_net == 0)
    return;
  for (auto callback : block->_callbacks)
    callback->inDbWirePreDetach(this);

  _dbNet* net = (_dbNet*) getNet();
  net->_wire = 0;
  wire->_net = 0;
  for (auto callback : block->_callbacks)
    callback->inDbWirePostDetach(this, (dbNet*) net);
}

void dbWire::copy(dbWire* dst_,
                  dbWire* src_,
                  bool removeITermsBTerms,
                  bool copyVias)
{
  _dbWire* dst = (_dbWire*) dst_;
  _dbWire* src = (_dbWire*) src_;

  assert(dst->getDatabase() == src->getDatabase());
  _dbBlock* block = (_dbBlock*) dst_->getBlock();
  for (auto callback : block->_callbacks)
    callback->inDbWirePreCopy(src_, dst_);
  uint n = src->_opcodes.size();

  // Free the old memory
  dst->_data.~dbVector<int>();
  new (&dst->_data) dbVector<int>();
  dst->_data.reserve(n);
  dst->_data = src->_data;

  // Free the old memory
  dst->_opcodes.~dbVector<unsigned char>();
  new (&dst->_opcodes) dbVector<unsigned char>();
  dst->_opcodes.reserve(n);
  dst->_opcodes = src->_opcodes;

  if (removeITermsBTerms) {
    uint i;

    for (i = 0; i < n; ++i) {
      unsigned char opcode = dst->_opcodes[i] & WOP_OPCODE_MASK;

      if (opcode == WOP_ITERM || opcode == WOP_BTERM) {
        dst->_opcodes[i] = WOP_NOP;
        dst->_data[i] = 0;
      }
    }
  }

  if (copyVias) {
    _dbBlock* src_block = (_dbBlock*) src->getOwner();
    _dbBlock* dst_block = (_dbBlock*) dst->getOwner();

    if (src_block != dst_block) {
      uint i;

      for (i = 0; i < n; ++i) {
        unsigned char opcode = dst->_opcodes[i] & WOP_OPCODE_MASK;

        if (opcode == WOP_VIA) {
          uint vid = dst->_data[i];
          _dbVia* src_via = src_block->_via_tbl->getPtr(vid);
          dbVia* dst_via = ((dbBlock*) dst_block)->findVia(src_via->_name);

          // duplicate src-via in dst-block if needed
          if (dst_via == NULL)
            dst_via = dbVia::copy((dbBlock*) dst_block, (dbVia*) src_via);

          dst->_data[i] = dst_via->getImpl()->getOID();
        }
      }
    }
  }
  for (auto callback : block->_callbacks)
    callback->inDbWirePostCopy(src_, dst_);
}

void dbWire::copy(dbWire* dst,
                  dbWire* src,
                  const Rect& bbox,
                  bool removeITermsBTerms,
                  bool copyVias)
{
  _dbBlock* block = (_dbBlock*) dst->getBlock();
  for (auto callback : block->_callbacks)
    callback->inDbWirePreCopy(src, dst);
  dbRtTree tree;
  tree.decode(src, !removeITermsBTerms);

  Rect r;
  dbRtTree::edge_iterator itr;

  for (itr = tree.begin_edges(); itr != tree.end_edges();) {
    dbRtEdge* edge = *itr;
    edge->getBBox(r);

    if (!bbox.intersects(r))
      ++itr;
    else
      itr = tree.deleteEdge(itr, true);
  }

  if (copyVias) {
    dbBlock* src_block = (dbBlock*) src->getImpl()->getOwner();
    dbBlock* dst_block = (dbBlock*) dst->getImpl()->getOwner();

    if (src_block != dst_block) {
      for (itr = tree.begin_edges(); itr != tree.end_edges();) {
        dbRtEdge* edge = *itr;

        if (edge->getType() == dbRtEdge::VIA) {
          dbVia* src_via = ((dbRtVia*) edge)->getVia();
          std::string name = src_via->getName();
          dbVia* dst_via = dst_block->findVia(name.c_str());

          // duplicate src-via in dst-block if needed
          if (dst_via == NULL)
            dst_via = dbVia::copy(dst_block, src_via);

          ((dbRtVia*) edge)->setVia(dst_via);
        }
      }
    }
  }

  tree.encode(dst, !removeITermsBTerms);
  for (auto callback : block->_callbacks)
    callback->inDbWirePostCopy(src, dst);
}

dbWire* dbWire::create(dbNet* net_, bool global_wire)
{
  _dbNet* net = (_dbNet*) net_;

  if (global_wire) {
    if (net->_global_wire != 0)
      return NULL;
  } else {
    if (net->_wire != 0)
      return NULL;
  }

  _dbBlock* block = (_dbBlock*) net->getOwner();
  _dbWire* wire = block->_wire_tbl->create();
  wire->_net = net->getOID();

  if (global_wire) {
    net->_global_wire = wire->getOID();
    wire->_flags._is_global = 1;
  } else
    net->_wire = wire->getOID();

  net->_flags._wire_ordered = 0;
  net->_flags._disconnected = 0;
  for (auto callback : block->_callbacks)
    callback->inDbWireCreate((dbWire*) wire);
  return (dbWire*) wire;
}

dbWire* dbWire::create(dbBlock* block_, bool /* unused: global_wire */)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbWire* wire = block->_wire_tbl->create();
  for (auto callback : block->_callbacks)
    callback->inDbWireCreate((dbWire*) wire);
  return (dbWire*) wire;
}

dbWire* dbWire::getWire(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbWire*) block->_wire_tbl->getPtr(dbid_);
}

void dbWire::destroy(dbWire* wire_)
{
  _dbWire* wire = (_dbWire*) wire_;
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  _dbNet* net = (_dbNet*) wire_->getNet();
  for (auto callback : block->_callbacks)
    callback->inDbWireDestroy(wire_);
  Rect bbox;

  if (wire_->getBBox(bbox))
    block->remove_rect(bbox);
  if (net) {
    if (wire->_flags._is_global)
      net->_global_wire = 0;
    else {
      net->_wire = 0;
      net->_flags._wire_ordered = 0;
      net->_flags._wire_altered = 1;
    }
  } else
    wire_->getImpl()->getLogger()->warn(utl::ODB, 62, "This wire has no net");

  dbProperty::destroyProperties(wire);
  block->_wire_tbl->destroy(wire);
}

}  // namespace odb
