// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbWire.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <vector>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTechLayerRule.h"
#include "dbVia.h"
#include "dbWireOpcode.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbWire>;

bool _dbWire::operator==(const _dbWire& rhs) const
{
  if (flags_.is_global != rhs.flags_.is_global) {
    return false;
  }

  if (data_ != rhs.data_) {
    return false;
  }

  if (opcodes_ != rhs.opcodes_) {
    return false;
  }

  if (net_ != rhs.net_) {
    return false;
  }

  return true;
}

dbOStream& operator<<(dbOStream& stream, const _dbWire& wire)
{
  uint32_t* bit_field = (uint32_t*) &wire.flags_;
  stream << *bit_field;
  stream << wire.data_;
  stream << wire.opcodes_;
  stream << wire.net_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbWire& wire)
{
  uint32_t* bit_field = (uint32_t*) &wire.flags_;
  stream >> *bit_field;
  stream >> wire.data_;
  stream >> wire.opcodes_;
  stream >> wire.net_;
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

  if (wire->net_ == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) wire->getOwner();
  return (dbNet*) block->net_tbl_->getPtr(wire->net_);
}

bool dbWire::isGlobalWire()
{
  _dbWire* wire = (_dbWire*) this;
  return wire->flags_.is_global == 1;
}

void dbWire::addOneSeg(unsigned char op,
                       int value,
                       uint32_t jj,
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
  wire->data_.push_back(value);
  wire->opcodes_.push_back(op);
}

void dbWire::addOneSeg(unsigned char op, int value)
{
  _dbWire* wire = (_dbWire*) this;
  wire->data_.push_back(value);
  wire->opcodes_.push_back(op);
}

uint32_t dbWire::getTermJid(const int termid) const
{
  _dbWire* wire = (_dbWire*) this;
  int topcd = kIterm;
  int ttid = termid;
  if (termid < 0) {
    topcd = kBterm;
    ttid = -termid;
  }
  const uint32_t wlen = wire->length();
  uint32_t jj;
  for (jj = 0; jj < wlen; jj++) {
    if ((wire->opcodes_[jj] & WOP_OPCODE_MASK) == topcd) {
      if (wire->data_[jj] == ttid) {
        break;
      }
    }
  }
  if (jj == wlen) {
    return 0;
  }
  jj--;
  if ((wire->opcodes_[jj] & WOP_OPCODE_MASK) == kProperty) {
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
  assert((0 <= shape_id) && (shape_id < (int) wire->length()));
  unsigned char opcode = wire->opcodes_[shape_id];

  switch (opcode & WOP_OPCODE_MASK) {
    case kX:
    case kY:
    case kColinear: {
      getSegment(shape_id, shape);
      return;
    }

    case kVia: {
      dbBlock* block = (dbBlock*) wire->getOwner();
      dbTech* tech = getDb()->getTech();
      int operand = wire->data_[shape_id];
      dbVia* via = dbVia::getVia(block, operand);
      dbBox* box = via->getBBox();

      if (box == nullptr) {
        return;
      }

      WirePoint pnt;
      getPrevPoint(
          tech, block, wire->opcodes_, wire->data_, shape_id, false, pnt);
      Rect b = box->getBox();
      int xmin = b.xMin() + pnt.x;
      int ymin = b.yMin() + pnt.y;
      int xmax = b.xMax() + pnt.x;
      int ymax = b.yMax() + pnt.y;
      Rect r(xmin, ymin, xmax, ymax);
      shape.setVia(via, r);
      return;
    }

    case kTechVia: {
      dbBlock* block = (dbBlock*) wire->getOwner();
      dbTech* tech = getDb()->getTech();
      int operand = wire->data_[shape_id];
      dbTechVia* via = dbTechVia::getTechVia(tech, operand);
      dbBox* box = via->getBBox();

      if (box == nullptr) {
        return;
      }

      // dimitri_fix LOOK_AGAIN WirePoint pnt;
      WirePoint pnt;
      pnt.x = 0;
      pnt.y = 0;
      getPrevPoint(
          tech, block, wire->opcodes_, wire->data_, shape_id, false, pnt);
      Rect b = box->getBox();
      int xmin = b.xMin() + pnt.x;
      int ymin = b.yMin() + pnt.y;
      int xmax = b.xMax() + pnt.x;
      int ymax = b.yMax() + pnt.y;
      Rect r(xmin, ymin, xmax, ymax);
      shape.setVia(via, r);
      return;
    }

    default:
      assert(DB_WIRE_SHAPE_INVALID_SHAPE_ID);
  }
}

Point dbWire::getCoord(int jid)
{
  _dbWire* wire = (_dbWire*) this;
  assert((0 <= jid) && (jid < (int) wire->length()));
  dbBlock* block = (dbBlock*) wire->getOwner();
  dbTech* tech = getDb()->getTech();
  WirePoint pnt;
  getPrevPoint(tech, block, wire->opcodes_, wire->data_, jid, false, pnt);

  return {pnt.x, pnt.y};
}

bool dbWire::getProperty(int jid, int& prpty)
{
  _dbWire* wire = (_dbWire*) this;
  [[maybe_unused]] const int wlen = (int) wire->length();
  assert(0 <= jid && jid < wlen);
  unsigned char op = wire->opcodes_[jid] & WOP_OPCODE_MASK;
  if (op == kColinear || op == kRect) {
    prpty = 0;
    return true;
  }
  assert(op == kX || op == kY);
  assert(jid + 1 < wlen);
  if ((wire->opcodes_[jid + 1] & WOP_OPCODE_MASK) == kProperty) {
    prpty = wire->data_[jid + 1];
    return true;
  }
  assert(jid + 2 < wlen);
  if ((wire->opcodes_[jid + 2] & WOP_OPCODE_MASK) == kProperty) {
    prpty = wire->data_[jid + 2];
    return true;
  }
  return false;
}

bool dbWire::setProperty(int jid, int prpty)
{
  _dbWire* wire = (_dbWire*) this;
  [[maybe_unused]] const int wlen = (int) wire->length();
  assert(0 <= jid && jid < wlen);
  if ((wire->opcodes_[jid] & WOP_OPCODE_MASK) == kColinear) {
    return true;
  }
  assert((wire->opcodes_[jid] & WOP_OPCODE_MASK) == kX
         || (wire->opcodes_[jid] & WOP_OPCODE_MASK) == kY);
  assert(jid + 1 < wlen);
  if ((wire->opcodes_[jid + 1] & WOP_OPCODE_MASK) == kProperty) {
    wire->data_[jid + 1] = prpty;
    return true;
  }
  assert(jid + 2 < wlen);
  if ((wire->opcodes_[jid + 2] & WOP_OPCODE_MASK) == kProperty) {
    wire->data_[jid + 2] = prpty;
    return true;
  }
  return false;
}

int dbWire::getData(int idx)
{
  _dbWire* wire = (_dbWire*) this;
  assert((0 <= idx) && (idx < (int) wire->length()));
  return (wire->data_[idx]);
}

unsigned char dbWire::getOpcode(int idx)
{
  _dbWire* wire = (_dbWire*) this;
  assert((0 <= idx) && (idx < (int) wire->length()));
  return (wire->opcodes_[idx]);
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

uint32_t dbWire::length()
{
  _dbWire* wire = (_dbWire*) this;
  return wire->length();
}

uint32_t dbWire::count()
{
  uint32_t jj;
  int opcode;
  uint32_t cnt = 0;
  _dbWire* wire = (_dbWire*) this;
  for (jj = 0; jj < wire->length(); jj++) {
    opcode = wire->opcodes_[jj] & WOP_OPCODE_MASK;
    if (opcode == kX || opcode == kY) {
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
  assert(idx >= 0);
  opcode = wire->opcodes_[idx];

  const WireOp wire_op = static_cast<WireOp>(opcode & WOP_OPCODE_MASK);
  switch (wire_op) {
    case kJunction:
      idx = wire->data_[idx];
      ignore_ext = true;
      goto decode_loop;

    case kRule:
      if (found_width == false) {
        found_width = true;

        if (opcode & WOP_BLOCK_RULE) {
          dbBlock* block = (dbBlock*) wire->getOwner();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(block, wire->data_[idx]);
          width = rule->getWidth();
        } else {
          dbTech* tech = getDb()->getTech();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(tech, wire->data_[idx]);
          width = rule->getWidth();
        }
      }

      break;

    case kX:
      input = XInput;
      goto state_machine_update;

    case kY:
      input = YInput;
      goto state_machine_update;

    case kColinear:
      input = CInput;
      goto state_machine_update;

    case kVia:
      if (layer == nullptr) {
        dbBlock* block = (dbBlock*) wire->getOwner();
        dbVia* via = dbVia::getVia(block, wire->data_[idx]);

        if (opcode & WOP_VIA_EXIT_TOP) {
          layer = via->getTopLayer();
        } else {
          layer = via->getBottomLayer();
        }
      }
      ignore_ext = true;
      break;

    case kTechVia:
      if (layer == nullptr) {
        dbTech* tech = getDb()->getTech();
        dbTechVia* via = dbTechVia::getTechVia(tech, wire->data_[idx]);

        if (opcode & WOP_VIA_EXIT_TOP) {
          layer = via->getTopLayer();
        } else {
          layer = via->getBottomLayer();
        }
      }
      ignore_ext = true;
      break;
    case kPath:
    case kShort:
    case kIterm:
    case kBterm:
    case kOperand:
    case kProperty:
    case kVwire:
    case kRect:
    case kNop:
    case kColor:
    case kViaColor: {
      utl::Logger* logger = getImpl()->getLogger();
      logger->error(
          utl::ODB, 1115, "Unexpected {} in dbWire::getSegment", wire_op);
      break;
    }
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
      prev_ext = wire->data_[idx + 1];
      has_prev_ext = true;
    }
  } else if (state <= 3) {
    if ((opcode & WOP_EXTENSION) && !ignore_ext) {
      cur_ext = wire->data_[idx + 1];
      has_cur_ext = true;
    }
  }

  int value = wire->data_[idx];
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
    assert(idx >= 0);
    opcode = wire->opcodes_[idx];

    switch (opcode & WOP_OPCODE_MASK) {
      case kPath:
      case kShort:
      case kVwire: {
        if (layer == nullptr) {
          dbTech* tech = getDb()->getTech();
          layer = dbTechLayer::getTechLayer(tech, wire->data_[idx]);
        }

        --idx;
        break;
      }

      case kJunction: {
        idx = wire->data_[idx];
        break;
      }

      case kRule: {
        found_width = true;
        if (opcode & WOP_BLOCK_RULE) {
          dbBlock* block = (dbBlock*) wire->getOwner();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(block, wire->data_[idx]);
          width = rule->getWidth();
        } else {
          dbTech* tech = getDb()->getTech();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(tech, wire->data_[idx]);
          width = rule->getWidth();
        }
        --idx;
        break;
      }

      case kVia: {
        if (layer == nullptr) {
          dbBlock* block = (dbBlock*) wire->getOwner();
          dbVia* via = dbVia::getVia(block, wire->data_[idx]);

          if (opcode & WOP_VIA_EXIT_TOP) {
            layer = via->getTopLayer();
          } else {
            layer = via->getBottomLayer();
          }
        }
        --idx;
        break;
      }

      case kTechVia: {
        if (layer == nullptr) {
          dbTech* tech = getDb()->getTech();
          dbTechVia* via = dbTechVia::getTechVia(tech, wire->data_[idx]);

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
  assert(idx >= 0);
  opcode = wire->opcodes_[idx];

  const WireOp wire_op = static_cast<WireOp>(opcode & WOP_OPCODE_MASK);
  switch (wire_op) {
    case kJunction:
      idx = wire->data_[idx];
      ignore_ext = true;
      goto decode_loop;

    case kRule:
      if (found_width == false) {
        found_width = true;

        if (opcode & WOP_BLOCK_RULE) {
          dbBlock* block = (dbBlock*) wire->getOwner();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(block, wire->data_[idx]);
          width = rule->getWidth();
        } else {
          dbTech* tech = getDb()->getTech();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(tech, wire->data_[idx]);
          width = rule->getWidth();
        }
      }

      break;

    case kX:
      input = XInput;
      goto state_machine_update;

    case kY:
      input = YInput;
      goto state_machine_update;

    case kColinear:
      input = CInput;
      goto state_machine_update;

    case kPath:
    case kShort:
    case kVia:
    case kTechVia:
    case kIterm:
    case kBterm:
    case kOperand:
    case kProperty:
    case kVwire:
    case kRect:
    case kNop:
    case kColor:
    case kViaColor: {
      utl::Logger* logger = getImpl()->getLogger();
      logger->error(
          utl::ODB, 1114, "Unexpected {} in dbWire::getSegment", wire_op);
      break;
    }
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
      prev_ext = wire->data_[idx + 1];
      has_prev_ext = true;
    }
  } else if (state <= 3) {
    if ((opcode & WOP_EXTENSION) && !ignore_ext) {
      cur_ext = wire->data_[idx + 1];
      has_cur_ext = true;
    }
  }

  int value = wire->data_[idx];
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
    assert(idx >= 0);
    opcode = wire->opcodes_[idx];

    switch (opcode & WOP_OPCODE_MASK) {
      case kJunction: {
        idx = wire->data_[idx];
        break;
      }

      case kRule: {
        found_width = true;
        if (opcode & WOP_BLOCK_RULE) {
          dbBlock* block = (dbBlock*) wire->getOwner();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(block, wire->data_[idx]);
          width = rule->getWidth();
        } else {
          dbTech* tech = getDb()->getTech();
          dbTechLayerRule* rule
              = dbTechLayerRule::getTechLayerRule(tech, wire->data_[idx]);
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

static unsigned char getPrevOpcode(_dbWire* wire, int& idx)
{
  --idx;

prevOpCode:
  assert(idx >= 0);
  unsigned char opcode = wire->opcodes_[idx];

  switch (opcode & WOP_OPCODE_MASK) {
    case kPath:
    case kShort:
    case kVwire:
    case kJunction:
      return opcode;

    case kRule:
      --idx;
      goto prevOpCode;

    case kX:
    case kY:
    case kColinear:
    case kVia:
    case kTechVia:
      return opcode;

    default:
      --idx;
      goto prevOpCode;
  }
}

static bool createVia(_dbWire* wire, int idx, dbShape& shape)
{
  dbBlock* block = (dbBlock*) wire->getOwner();
  dbTech* tech = wire->getDb()->getTech();
  int operand = wire->data_[idx];
  dbVia* via = dbVia::getVia(block, operand);
  dbBox* box = via->getBBox();

  if (box == nullptr) {
    return false;
  }

  WirePoint pnt;
  // dimitri_fix
  pnt.x = 0;
  pnt.y = 0;
  getPrevPoint(tech, block, wire->opcodes_, wire->data_, idx, false, pnt);
  Rect b = box->getBox();
  int xmin = b.xMin() + pnt.x;
  int ymin = b.yMin() + pnt.y;
  int xmax = b.xMax() + pnt.x;
  int ymax = b.yMax() + pnt.y;
  Rect r(xmin, ymin, xmax, ymax);
  shape.setVia(via, r);
  return true;
}

static bool createTechVia(_dbWire* wire, int idx, dbShape& shape)
{
  dbBlock* block = (dbBlock*) wire->getOwner();
  dbTech* tech = wire->getDb()->getTech();
  int operand = wire->data_[idx];
  dbTechVia* via = dbTechVia::getTechVia(tech, operand);
  dbBox* box = via->getBBox();

  if (box == nullptr) {
    return false;
  }

  WirePoint pnt;
  // dimitri_fix
  pnt.x = 0;
  pnt.y = 0;
  getPrevPoint(tech, block, wire->opcodes_, wire->data_, idx, false, pnt);
  Rect b = box->getBox();
  int xmin = b.xMin() + pnt.x;
  int ymin = b.yMin() + pnt.y;
  int xmax = b.xMax() + pnt.x;
  int ymax = b.yMax() + pnt.y;
  Rect r(xmin, ymin, xmax, ymax);
  shape.setVia(via, r);
  return true;
}

// This function gets the previous via of this shape_id,
// if one exists.
bool dbWire::getPrevVia(int idx, dbShape& shape)
{
  _dbWire* wire = (_dbWire*) this;
  assert((0 < idx) && (idx < (int) wire->length()));

  unsigned char opcode;
  opcode = getPrevOpcode(wire, idx);

  switch (opcode & WOP_OPCODE_MASK) {
    case kColinear: {
      // special case: colinear point with ext starts a new segment
      //   idx-3   idx-2   idx-1      idx
      // ( X1 Y1 ) ( V ) ( X1 Y1 E) ( X1 Y2 )
      if (opcode & WOP_EXTENSION) {
        opcode = getPrevOpcode(wire, idx);

        switch (opcode & WOP_OPCODE_MASK) {
          case kTechVia:
            return createTechVia(wire, idx, shape);

          case kVia:
            return createVia(wire, idx, shape);

          default:
            break;
        }
      }

      break;
    }

    case kTechVia:
      return createTechVia(wire, idx, shape);

    case kVia:
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
  assert((0 < idx) && (idx < (int) wire->length()));
  ++idx;

nextOpCode:
  if ((uint32_t) idx == wire->length()) {
    return false;
  }

  unsigned char opcode = wire->opcodes_[idx];

  switch (opcode & WOP_OPCODE_MASK) {
    case kPath:
    case kShort:
    case kVwire:
    case kJunction:
      return false;

    case kRule:
      ++idx;
      goto nextOpCode;

    case kX:
    case kY:
    case kColinear:
      return false;

    case kTechVia:
      return createTechVia(wire, idx, shape);

    case kVia:
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
    int n = src->opcodes_.size();

    for (i = 0; i < n; ++i) {
      unsigned char opcode = src->opcodes_[i] & WOP_OPCODE_MASK;

      if (opcode == kIterm || opcode == kBterm) {
        return;
      }
    }
  }
  for (auto callback : ((_dbBlock*) getBlock())->callbacks_) {
    callback->inDbWirePreAppend(src_, this);
  }
  uint32_t sz = dst->opcodes_.size();
  dst->opcodes_.insert(
      dst->opcodes_.end(), src->opcodes_.begin(), src->opcodes_.end());
  dst->data_.insert(dst->data_.end(), src->data_.begin(), src->data_.end());

  // fix up the dbVia's if needed...
  if (src_block != dst_block && !singleSegmentWire) {
    int i;
    int n = dst->opcodes_.size();

    for (i = sz; i < n; ++i) {
      unsigned char opcode = dst->opcodes_[i] & WOP_OPCODE_MASK;

      if (opcode == kVia) {
        uint32_t vid = dst->data_[i];
        _dbVia* src_via = src_block->via_tbl_->getPtr(vid);
        dbVia* dst_via = ((dbBlock*) dst_block)->findVia(src_via->name_);

        // duplicate src-via in dst-block if needed
        if (dst_via == nullptr) {
          dst_via = dbVia::copy((dbBlock*) dst_block, (dbVia*) src_via);
        }

        dst->data_[i] = dst_via->getImpl()->getOID();
      }
    }
  }

  // Fix up the junction-ids
  int i;
  int n = dst->opcodes_.size();

  for (i = sz; i < n; ++i) {
    unsigned char opcode = dst->opcodes_[i] & WOP_OPCODE_MASK;
    if ((opcode == kShort) || (opcode == kJunction) || (opcode == kVwire)) {
      dst->data_[i] += sz;
    }
  }
  for (auto callback : ((_dbBlock*) getBlock())->callbacks_) {
    callback->inDbWirePostAppend(src_, this);
  }
}

void dbWire::attach(dbNet* net_)
{
  _dbWire* wire = (_dbWire*) this;
  _dbNet* net = (_dbNet*) net_;
  _dbBlock* block = (_dbBlock*) getBlock();
  assert(wire->flags_.is_global == 0);
  if (wire->net_ == net->getOID() && net->wire_ == wire->getOID()) {
    return;
  }
  for (auto callback : block->callbacks_) {
    callback->inDbWirePreAttach(this, net_);
  }

  // dbWire * prev = net_->getWire();

  if (net->wire_ != 0) {
    dbWire::destroy(net_->getWire());
  }

  if (wire->net_ != 0) {
    detach();
  }

  wire->net_ = net->getOID();
  net->wire_ = wire->getOID();
  for (auto callback : block->callbacks_) {
    callback->inDbWirePostAttach(this);
  }
}

void dbWire::detach()
{
  _dbWire* wire = (_dbWire*) this;
  _dbBlock* block = (_dbBlock*) getBlock();
  assert(wire->flags_.is_global == 0);
  if (wire->net_ == 0) {
    return;
  }
  for (auto callback : block->callbacks_) {
    callback->inDbWirePreDetach(this);
  }

  _dbNet* net = (_dbNet*) getNet();
  net->wire_ = 0;
  wire->net_ = 0;
  for (auto callback : block->callbacks_) {
    callback->inDbWirePostDetach(this, (dbNet*) net);
  }
}

dbWire* dbWire::create(dbNet* net_, bool global_wire)
{
  _dbNet* net = (_dbNet*) net_;

  if (global_wire) {
    if (net->global_wire_ != 0) {
      return nullptr;
    }
  } else {
    if (net->wire_ != 0) {
      return nullptr;
    }
  }

  _dbBlock* block = (_dbBlock*) net->getOwner();
  _dbWire* wire = block->wire_tbl_->create();
  wire->net_ = net->getOID();

  if (global_wire) {
    net->global_wire_ = wire->getOID();
    wire->flags_.is_global = 1;
  } else {
    net->wire_ = wire->getOID();
  }

  net->flags_.wire_ordered = 0;
  net->flags_.disconnected = 0;
  for (auto callback : block->callbacks_) {
    callback->inDbWireCreate((dbWire*) wire);
  }
  return (dbWire*) wire;
}

dbWire* dbWire::create(dbBlock* block_, bool /* unused: global_wire */)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbWire* wire = block->wire_tbl_->create();
  for (auto callback : block->callbacks_) {
    callback->inDbWireCreate((dbWire*) wire);
  }
  return (dbWire*) wire;
}

dbWire* dbWire::getWire(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbWire*) block->wire_tbl_->getPtr(dbid_);
}

void dbWire::destroy(dbWire* wire_)
{
  _dbWire* wire = (_dbWire*) wire_;
  _dbBlock* block = (_dbBlock*) wire->getOwner();
  _dbNet* net = (_dbNet*) wire_->getNet();
  for (auto callback : block->callbacks_) {
    callback->inDbWireDestroy(wire_);
  }
  const auto opt_bbox = wire_->getBBox();

  if (opt_bbox) {
    block->remove_rect(opt_bbox.value());
  }
  if (net) {
    if (wire->flags_.is_global) {
      net->global_wire_ = 0;
    } else {
      net->wire_ = 0;
      net->flags_.wire_ordered = 0;
      net->flags_.wire_altered = 1;
    }
  } else {
    wire_->getImpl()->getLogger()->warn(utl::ODB, 62, "This wire has no net");
  }

  dbProperty::destroyProperties(wire);
  block->wire_tbl_->destroy(wire);
}

void _dbWire::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["data"].add(data_);
  info.children["opcodes"].add(opcodes_);
}

}  // namespace odb
