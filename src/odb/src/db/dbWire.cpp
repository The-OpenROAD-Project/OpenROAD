// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbWire.h"

#include <algorithm>
#include <optional>
#include <vector>

#include "dbBlock.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayerRule.h"
#include "dbVia.h"
#include "dbWireOpcode.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbShape.h"
#include "utl/Logger.h"
namespace odb {

template class dbTable<_dbWire>;

bool _dbWire::operator==(const _dbWire& rhs) const
{
  if (_flags._is_global != rhs._flags._is_global) {
    return false;
  }

  if (_data != rhs._data) {
    return false;
  }

  if (_opcodes != rhs._opcodes) {
    return false;
  }

  if (_net != rhs._net) {
    return false;
  }

  return true;
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

  if (wire->_net == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) wire->getOwner();
  return (dbNet*) block->_net_tbl->getPtr(wire->_net);
}

bool dbWire::isGlobalWire()
{
  _dbWire* wire = (_dbWire*) this;
  return wire->_flags._is_global == 1;
}

void dbWire::addOneSeg(unsigned char op,
                       int value,
                       uint jj,
                       int* did,
                       dbRSeg** new_rsegs)
{
  _dbWire* wire = (_dbWire*) this;
  did[jj] = wire->length();
  if (new_rsegs && new_rsegs[jj]) {
    new_rsegs[jj]->updateShapeId(did[jj]);
  } else {
    wire = (_dbWire*) this;  // zzzz bp
  }
  wire->_data.push_back(value);
  wire->_opcodes.push_back(op);
}

void dbWire::addOneSeg(unsigned char op, int value)
{
  _dbWire* wire = (_dbWire*) this;
  wire->_data.push_back(value);
  wire->_opcodes.push_back(op);
}

uint dbWire::getTermJid(const int termid) const
{
  _dbWire* wire = (_dbWire*) this;
  int topcd = WOP_ITERM;
  int ttid = termid;
  if (termid < 0) {
    topcd = WOP_BTERM;
    ttid = -termid;
  }
  const uint wlen = wire->length();
  uint jj;
  for (jj = 0; jj < wlen; jj++) {
    if ((wire->_opcodes[jj] & WOP_OPCODE_MASK) == topcd) {
      if (wire->_data[jj] == ttid) {
        break;
      }
    }
  }
  if (jj == wlen) {
    return 0;
  }
  jj--;
  if ((wire->_opcodes[jj] & WOP_OPCODE_MASK) == WOP_PROPERTY) {
    jj--;
  }
  return jj;
}

std::optional<Rect> dbWire::getBBox()
{
  Rect bbox;
  bbox.mergeInit();

  dbShape s;
  dbWireShapeItr itr;
  for (itr.begin(this); itr.next(s);) {
    bbox.merge(s.getBox());
  }

  if (!bbox.isInverted()) {
    return bbox;
  }
  return {};
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

      if (box == nullptr) {
        return;
      }

      WirePoint pnt;
      getPrevPoint(
          tech, block, wire->_opcodes, wire->_data, shape_id, false, pnt);
      Rect b = box->getBox();
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

      if (box == nullptr) {
        return;
      }

      // dimitri_fix LOOK_AGAIN WirePoint pnt;
      WirePoint pnt;
      pnt._x = 0;
      pnt._y = 0;
      getPrevPoint(
          tech, block, wire->_opcodes, wire->_data, shape_id, false, pnt);
      Rect b = box->getBox();
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

Point dbWire::getCoord(int jid)
{
  _dbWire* wire = (_dbWire*) this;
  ZASSERT((0 <= jid) && (jid < (int) wire->length()));
  dbBlock* block = (dbBlock*) wire->getOwner();
  dbTech* tech = getDb()->getTech();
  WirePoint pnt;
  getPrevPoint(tech, block, wire->_opcodes, wire->_data, jid, false, pnt);

  return {pnt._x, pnt._y};
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
  if ((wire->_opcodes[jid] & WOP_OPCODE_MASK) == WOP_COLINEAR) {
    return true;
  }
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

uint64_t dbWire::getLength()
{
  dbWireShapeItr shapes;
  dbShape s;
  uint64_t rtlen = 0;
  for (shapes.begin(this); shapes.next(s);) {
    if (!s.isVia()) {
      rtlen += s.getLength();
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
    if (opcode == WOP_X || opcode == WOP_Y) {
      cnt++;
    }
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

static const int nextState[13][3] = {
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

static const Coord curCoord[13][3] = {
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

static const Coord prevCoord[13][3] = {
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
  dbTechLayer* layer = nullptr;

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

decode_loop: {
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
      if (layer == nullptr) {
        dbBlock* block = (dbBlock*) wire->getOwner();
        dbVia* via = dbVia::getVia(block, wire->_data[idx]);

        if (opcode & WOP_VIA_EXIT_TOP) {
          layer = via->getTopLayer();
        } else {
          layer = via->getBottomLayer();
        }
      }
      ignore_ext = true;
      break;

    case WOP_TECH_VIA:
      if (layer == nullptr) {
        dbTech* tech = getDb()->getTech();
        dbTechVia* via = dbTechVia::getTechVia(tech, wire->_data[idx]);

        if (opcode & WOP_VIA_EXIT_TOP) {
          layer = via->getTopLayer();
        } else {
          layer = via->getBottomLayer();
        }
      }
      ignore_ext = true;
      break;
  }

  --idx;
  goto decode_loop;

state_machine_update: {
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

  if (state < 13) {
    goto decode_loop;
  }
}
}

  while ((layer == nullptr) || (found_width == false)) {
    ZASSERT(idx >= 0);
    opcode = wire->_opcodes[idx];

    switch (opcode & WOP_OPCODE_MASK) {
      case WOP_PATH:
      case WOP_SHORT:
      case WOP_VWIRE: {
        if (layer == nullptr) {
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
        if (layer == nullptr) {
          dbBlock* block = (dbBlock*) wire->getOwner();
          dbVia* via = dbVia::getVia(block, wire->_data[idx]);

          if (opcode & WOP_VIA_EXIT_TOP) {
            layer = via->getTopLayer();
          } else {
            layer = via->getBottomLayer();
          }
        }
        --idx;
        break;
      }

      case WOP_TECH_VIA: {
        if (layer == nullptr) {
          dbTech* tech = getDb()->getTech();
          dbTechVia* via = dbTechVia::getTechVia(tech, wire->_data[idx]);

          if (opcode & WOP_VIA_EXIT_TOP) {
            layer = via->getTopLayer();
          } else {
            layer = via->getBottomLayer();
          }
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
  int default_ext;

  if (default_width) {
    dw = layer->getWidth() >> 1;
    default_ext = dw;
    if (prev[0] != cur[0] || prev[1] != cur[1]) {
      if (prev[0] != cur[0]
          && layer->getDirection() == dbTechLayerDir::VERTICAL) {
        dw = layer->getWrongWayWidth() >> 1;
      } else if (prev[1] != cur[1]
                 && layer->getDirection() == dbTechLayerDir::HORIZONTAL) {
        dw = layer->getWrongWayWidth() >> 1;
      }
    }

  } else {
    dw = width >> 1;
    default_ext = dw;
  }

  shape.setSegment(prev[0],
                   prev[1],
                   prev_ext,
                   has_prev_ext,
                   cur[0],
                   cur[1],
                   cur_ext,
                   has_cur_ext,
                   dw,
                   default_ext,
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

decode_loop: {
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

state_machine_update: {
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

  if (state < 13) {
    goto decode_loop;
  }
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
  int default_ext;
  if (default_width) {
    dw = layer->getWidth() >> 1;
    default_ext = dw;
    if (prev[0] != cur[0] || prev[1] != cur[1]) {
      if (prev[0] != cur[0]
          && layer->getDirection() == dbTechLayerDir::VERTICAL) {
        dw = layer->getWrongWayWidth() >> 1;
      } else if (prev[1] != cur[1]
                 && layer->getDirection() == dbTechLayerDir::HORIZONTAL) {
        dw = layer->getWrongWayWidth() >> 1;
      }
    }

  } else {
    dw = width >> 1;
    default_ext = dw;
  }

  shape.setSegment(prev[0],
                   prev[1],
                   prev_ext,
                   has_prev_ext,
                   cur[0],
                   cur[1],
                   cur_ext,
                   has_cur_ext,
                   dw,
                   default_ext,
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

  if (box == nullptr) {
    return false;
  }

  WirePoint pnt;
  // dimitri_fix
  pnt._x = 0;
  pnt._y = 0;
  getPrevPoint(tech, block, wire->_opcodes, wire->_data, idx, false, pnt);
  Rect b = box->getBox();
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

  if (box == nullptr) {
    return false;
  }

  WirePoint pnt;
  // dimitri_fix
  pnt._x = 0;
  pnt._y = 0;
  getPrevPoint(tech, block, wire->_opcodes, wire->_data, idx, false, pnt);
  Rect b = box->getBox();
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
  if ((uint) idx == wire->length()) {
    return false;
  }

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

  if (!via.isVia()) {
    return false;
  }

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

      if (opcode == WOP_ITERM || opcode == WOP_BTERM) {
        return;
      }
    }
  }
  for (auto callback : ((_dbBlock*) getBlock())->_callbacks) {
    callback->inDbWirePreAppend(src_, this);
  }
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
        if (dst_via == nullptr) {
          dst_via = dbVia::copy((dbBlock*) dst_block, (dbVia*) src_via);
        }

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
        || (opcode == WOP_VWIRE)) {
      dst->_data[i] += sz;
    }
  }
  for (auto callback : ((_dbBlock*) getBlock())->_callbacks) {
    callback->inDbWirePostAppend(src_, this);
  }
}

void dbWire::attach(dbNet* net_)
{
  _dbWire* wire = (_dbWire*) this;
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) getBlock();
  assert(wire->_flags._is_global == 0);
  if (wire->_net == net->getOID() && net->_wire == wire->getOID()) {
    return;
  }
  for (auto callback : block->_callbacks) {
    callback->inDbWirePreAttach(this, net_);
  }

  // dbWire * prev = net_->getWire();

  if (net->_wire != 0) {
    dbWire::destroy(net_->getWire());
  }

  if (wire->_net != 0) {
    detach();
  }

  wire->_net = net->getOID();
  net->_wire = wire->getOID();
  for (auto callback : block->_callbacks) {
    callback->inDbWirePostAttach(this);
  }
}

void dbWire::detach()
{
  _dbWire* wire = (_dbWire*) this;
  _dbBlock* block = (_dbBlock*) getBlock();
  assert(wire->_flags._is_global == 0);
  if (wire->_net == 0) {
    return;
  }
  for (auto callback : block->_callbacks) {
    callback->inDbWirePreDetach(this);
  }

  _dbNet* net = (_dbNet*) getNet();
  net->_wire = 0;
  wire->_net = 0;
  for (auto callback : block->_callbacks) {
    callback->inDbWirePostDetach(this, (dbNet*) net);
  }
}

dbWire* dbWire::create(dbNet* net_, bool global_wire)
{
  _dbNet* net = (_dbNet*) net_;

  if (global_wire) {
    if (net->_global_wire != 0) {
      return nullptr;
    }
  } else {
    if (net->_wire != 0) {
      return nullptr;
    }
  }

  _dbBlock* block = (_dbBlock*) net->getOwner();
  _dbWire* wire = block->_wire_tbl->create();
  wire->_net = net->getOID();

  if (global_wire) {
    net->_global_wire = wire->getOID();
    wire->_flags._is_global = 1;
  } else {
    net->_wire = wire->getOID();
  }

  net->_flags._wire_ordered = 0;
  net->_flags._disconnected = 0;
  for (auto callback : block->_callbacks) {
    callback->inDbWireCreate((dbWire*) wire);
  }
  return (dbWire*) wire;
}

dbWire* dbWire::create(dbBlock* block_, bool /* unused: global_wire */)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbWire* wire = block->_wire_tbl->create();
  for (auto callback : block->_callbacks) {
    callback->inDbWireCreate((dbWire*) wire);
  }
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
  for (auto callback : block->_callbacks) {
    callback->inDbWireDestroy(wire_);
  }
  const auto opt_bbox = wire_->getBBox();

  if (opt_bbox) {
    block->remove_rect(opt_bbox.value());
  }
  if (net) {
    if (wire->_flags._is_global) {
      net->_global_wire = 0;
    } else {
      net->_wire = 0;
      net->_flags._wire_ordered = 0;
      net->_flags._wire_altered = 1;
    }
  } else {
    wire_->getImpl()->getLogger()->warn(utl::ODB, 62, "This wire has no net");
  }

  dbProperty::destroyProperties(wire);
  block->_wire_tbl->destroy(wire);
}

void _dbWire::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["data"].add(_data);
  info.children_["opcodes"].add(_opcodes);
}

}  // namespace odb
