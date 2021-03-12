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

#include "dbWireCodec.h"

#include <ctype.h>

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayerRule.h"
#include "dbWire.h"
#include "dbWireOpcode.h"
#include "utility/Logger.h"

namespace odb {

//////////////////////////////////////////////////////////////////////////////////
//
// dbWireEncoder
//
//////////////////////////////////////////////////////////////////////////////////

#define DB_WIRE_ENCODER_INVALID_VIA_LAYER 0

inline unsigned char dbWireEncoder::getOp(int idx)
{
  return _opcodes[idx];
}

inline void dbWireEncoder::updateOp(int idx, unsigned char op)
{
  _opcodes[idx] = op;
}

inline void dbWireEncoder::addOp(unsigned char op, int value)
{
  _data.push_back(value);
  _opcodes.push_back(op);
  ++_idx;
}

inline unsigned char dbWireEncoder::getWireType(dbWireType type)
{
  unsigned char wire_type = 0;

  switch (type.getValue()) {
    case dbWireType::NONE:
      wire_type = WOP_NONE;
      break;

    case dbWireType::COVER:
      wire_type = WOP_COVER;
      break;

    case dbWireType::FIXED:
      wire_type = WOP_FIXED;
      break;

    case dbWireType::ROUTED:
      wire_type = WOP_ROUTED;
      break;

    case dbWireType::NOSHIELD:
      wire_type = WOP_NOSHIELD;
      break;

    default:
      _wire_type = WOP_NONE;
      break;
  }

  return wire_type;
}

inline void dbWireEncoder::initPath(dbTechLayer* layer, dbWireType type)
{
  initPath(layer, getWireType(type));
}

inline void dbWireEncoder::initPath(dbTechLayer* layer,
                                    dbWireType type,
                                    dbTechLayerRule* rule)
{
  initPath(layer, getWireType(type), rule);
}

void dbWireEncoder::initPath(dbTechLayer* layer, unsigned char wire_type)
{
  _wire_type = wire_type;
  _point_cnt = 0;
  _via_cnt = 0;
  _layer = layer;
  _non_default_rule = 0;
  _rule_opcode = 0;
  _prev_extended_colinear_pnt = false;
}

void dbWireEncoder::initPath(dbTechLayer* layer,
                             unsigned char wire_type,
                             dbTechLayerRule* rule)
{
  _wire_type = wire_type;
  _point_cnt = 0;
  _via_cnt = 0;
  _layer = layer;
  _non_default_rule = rule->getImpl()->getOID();

  if (rule->isBlockRule())
    _rule_opcode = WOP_RULE | WOP_BLOCK_RULE;
  else
    _rule_opcode = WOP_RULE;

  _prev_extended_colinear_pnt = false;
  _x = 0;
  _y = 0;
}

dbWireEncoder::dbWireEncoder()
{
  _wire = NULL;
}

dbWireEncoder::~dbWireEncoder()
{
}

void dbWireEncoder::begin(dbWire* wire)
{
  clear();
  _wire = (_dbWire*) wire;
  _block = wire->getBlock();
  _tech = _block->getDb()->getTech();
}

void dbWireEncoder::clear()
{
  _wire = NULL;
  _block = NULL;
  _tech = NULL;
  _data.clear();
  _opcodes.clear();
  _layer = NULL;
  _idx = 0;
  _x = 0;
  _y = 0;
  _non_default_rule = 0;
  _rule_opcode = 0;
  _point_cnt = 0;
  _via_cnt = 0;
  _prev_extended_colinear_pnt = false;
}

void dbWireEncoder::append(dbWire* wire)
{
  _wire = (_dbWire*) wire;
  _block = wire->getBlock();
  _tech = _block->getDb()->getTech();
  _data = _wire->_data;
  _opcodes = _wire->_opcodes;
  _layer = NULL;
  _idx = _data.size();
  _x = 0;
  _y = 0;
  _non_default_rule = 0;
  _rule_opcode = 0;
  _point_cnt = 0;
  _via_cnt = 0;
  _prev_extended_colinear_pnt = false;
}

#define DB_WIRE_ENCODER_NON_ORTHOGANAL_SEGMENT 0

int dbWireEncoder::addPoint(int x, int y, uint property)
{
  int jct_id = _idx;

  if (_non_default_rule == 0) {
    if (_point_cnt == 0) {
      addOp(WOP_X | WOP_DEFAULT_WIDTH, x);
      addOp(WOP_Y | WOP_DEFAULT_WIDTH, y);
      _x = x;
      _y = y;
      jct_id++;
      _point_cnt++;
    } else if ((_x == x) && (_y == y)) {
      addOp(WOP_COLINEAR | WOP_DEFAULT_WIDTH, 0);
    } else if (_y == y) {
      addOp(WOP_X | WOP_DEFAULT_WIDTH, x);
      _x = x;
      _point_cnt++;
    } else if (_x == x) {
      addOp(WOP_Y | WOP_DEFAULT_WIDTH, y);
      _y = y;
      _point_cnt++;
    } else {
      ZASSERT(DB_WIRE_ENCODER_NON_ORTHOGANAL_SEGMENT);
    }
  } else {
    if (_point_cnt == 0) {
      addOp(WOP_X, x);
      addOp(WOP_Y, y);
      _x = x;
      _y = y;
      _point_cnt++;
      jct_id++;
    } else if ((_x == x) && (_y == y)) {
      addOp(WOP_COLINEAR, 0);
    } else if (_y == y) {
      addOp(WOP_X, x);
      _x = x;
      _point_cnt++;
    } else if (_x == x) {
      addOp(WOP_Y, y);
      _y = y;
      _point_cnt++;
    } else {
      ZASSERT(DB_WIRE_ENCODER_NON_ORTHOGANAL_SEGMENT);
    }

    if (_point_cnt
        && ((_point_cnt & (WOP_NON_DEFAULT_WIDTH_POINT_CNT - 1)) == 0))
      addOp(_rule_opcode, _non_default_rule);
  }
  if (_point_cnt != 1)
    addOp(WOP_PROPERTY, property);

  _prev_extended_colinear_pnt = false;
  return jct_id;
}

#define DB_WIRE_ENCODER_COLINEAR_EXT_RULE_1() \
  ((_point_cnt > 1) || ((_point_cnt > 0) && (_via_cnt > 0)))
#define DB_WIRE_ENCODER_COLINEAR_EXT_RULE_2() \
  (_prev_extended_colinear_pnt == false)

int dbWireEncoder::addPoint(int x, int y, int ext, uint property)
{
  int jct_id = _idx;

  if (_non_default_rule == 0) {
    if (_point_cnt == 0) {
      addOp(WOP_X | WOP_DEFAULT_WIDTH, x);
      addOp(WOP_Y | WOP_EXTENSION | WOP_DEFAULT_WIDTH, y);
      addOp(WOP_OPERAND, ext);
      _x = x;
      _y = y;
      _point_cnt++;
      jct_id++;
      _prev_extended_colinear_pnt = false;
    } else if ((_x == x) && (_y == y)) {
      ZASSERT(DB_WIRE_ENCODER_COLINEAR_EXT_RULE_1());
      ZASSERT(DB_WIRE_ENCODER_COLINEAR_EXT_RULE_2());
      addOp(WOP_COLINEAR | WOP_EXTENSION | WOP_DEFAULT_WIDTH, ext);
      _prev_extended_colinear_pnt = true;
    } else if (_y == y) {
      addOp(WOP_X | WOP_EXTENSION | WOP_DEFAULT_WIDTH, x);
      addOp(WOP_OPERAND, ext);
      _x = x;
      _point_cnt++;
      _prev_extended_colinear_pnt = false;
    } else if (_x == x) {
      addOp(WOP_Y | WOP_EXTENSION | WOP_DEFAULT_WIDTH, y);
      addOp(WOP_OPERAND, ext);
      _y = y;
      _point_cnt++;
      _prev_extended_colinear_pnt = false;
    } else {
      ZASSERT(DB_WIRE_ENCODER_NON_ORTHOGANAL_SEGMENT);
    }
  } else {
    if (_point_cnt == 0) {
      addOp(WOP_X, x);
      addOp(WOP_Y | WOP_EXTENSION, y);
      addOp(WOP_OPERAND, ext);
      _x = x;
      _y = y;
      _point_cnt++;
      jct_id++;
      _prev_extended_colinear_pnt = false;
    } else if ((_x == x) && (_y == y)) {
      ZASSERT(DB_WIRE_ENCODER_COLINEAR_EXT_RULE_1());
      ZASSERT(DB_WIRE_ENCODER_COLINEAR_EXT_RULE_2());
      addOp(WOP_COLINEAR | WOP_EXTENSION, ext);
      _prev_extended_colinear_pnt = true;
    } else if (_y == y) {
      addOp(WOP_X | WOP_EXTENSION, x);
      addOp(WOP_OPERAND, ext);
      _x = x;
      _point_cnt++;
      _prev_extended_colinear_pnt = false;
    } else if (_x == x) {
      addOp(WOP_Y | WOP_EXTENSION, y);
      addOp(WOP_OPERAND, ext);
      _y = y;
      _point_cnt++;
      _prev_extended_colinear_pnt = false;
    } else {
      ZASSERT(DB_WIRE_ENCODER_NON_ORTHOGANAL_SEGMENT);
    }

    if (_point_cnt
        && ((_point_cnt & (WOP_NON_DEFAULT_WIDTH_POINT_CNT - 1)) == 0))
      addOp(_rule_opcode, _non_default_rule);
  }
  if (_point_cnt != 1)
    addOp(WOP_PROPERTY, property);

  return jct_id;
}

int dbWireEncoder::addVia(dbVia* via)
{
  int jct_id = _idx;
  ZASSERT(_point_cnt != 0);
  dbTechLayer* top = via->getTopLayer();
  dbTechLayer* bot = via->getBottomLayer();

  if (top == _layer) {
    _layer = bot;
    addOp(WOP_VIA, via->getImpl()->getOID());
  } else if (bot == _layer) {
    _layer = top;
    addOp(WOP_VIA | WOP_VIA_EXIT_TOP, via->getImpl()->getOID());
  } else {
    ZASSERT(DB_WIRE_ENCODER_INVALID_VIA_LAYER);
    addOp(WOP_VIA, 0);
  }

  _via_cnt++;
  return jct_id;
}

int dbWireEncoder::addTechVia(dbTechVia* via)
{
  int jct_id = _idx;
  ZASSERT(_point_cnt != 0);
  dbTechLayer* top = via->getTopLayer();
  dbTechLayer* bot = via->getBottomLayer();

  if (top == _layer) {
    _layer = bot;
    addOp(WOP_TECH_VIA, via->getImpl()->getOID());
  } else if (bot == _layer) {
    _layer = top;
    addOp(WOP_TECH_VIA | WOP_VIA_EXIT_TOP, via->getImpl()->getOID());
  } else {
    ZASSERT(DB_WIRE_ENCODER_INVALID_VIA_LAYER);
    addOp(WOP_TECH_VIA, 0);
  }

  _via_cnt++;
  return jct_id;
}

void dbWireEncoder::addRect(int deltaX1, int deltaY1, int deltaX2, int deltaY2)
{
  // order must match dbWireDecoder::next
  ZASSERT(_point_cnt != 0);
  addOp(WOP_RECT, deltaX1);
  addOp(WOP_OPERAND, deltaY1);
  addOp(WOP_OPERAND, deltaX2);
  addOp(WOP_OPERAND, deltaY2);
}

void dbWireEncoder::addITerm(dbITerm* iterm)
{
  ZASSERT(_point_cnt != 0);
  addOp(WOP_ITERM, iterm->getImpl()->getOID());
}

void dbWireEncoder::addBTerm(dbBTerm* bterm)
{
  ZASSERT(_point_cnt != 0);
  addOp(WOP_BTERM, bterm->getImpl()->getOID());
}

void dbWireEncoder::newPath(dbTechLayer* layer, dbWireType type)
{
  initPath(layer, type);
  addOp(WOP_PATH | _wire_type, layer->getImpl()->getOID());
}

void dbWireEncoder::newPath(dbTechLayer* layer,
                            dbWireType type,
                            dbTechLayerRule* rule)
{
  initPath(layer, type, rule);
  addOp(WOP_PATH | _wire_type, layer->getImpl()->getOID());
  addOp(_rule_opcode, _non_default_rule);
}

void dbWireEncoder::newPath(int jct_id)
{
  ZASSERT((jct_id >= 0) && (jct_id < _idx));

  WirePoint pnt;
  getPrevPoint(_tech, _block, _opcodes, _data, jct_id, true, pnt);
  initPath(pnt._layer, _wire_type);
  _x = pnt._x;
  _y = pnt._y;
  _point_cnt = 1;

  addOp(WOP_JUNCTION | _wire_type, jct_id);
  addOp(WOP_COLINEAR | WOP_DEFAULT_WIDTH, 0);
}

void dbWireEncoder::newPath(int jct_id, dbWireType type)
{
  ZASSERT((jct_id >= 0) && (jct_id < _idx));
  WirePoint pnt;
  getPrevPoint(_tech, _block, _opcodes, _data, jct_id, true, pnt);
  initPath(pnt._layer, type);
  _x = pnt._x;
  _y = pnt._y;
  _point_cnt = 1;
  addOp(WOP_JUNCTION | _wire_type, jct_id);
  addOp(WOP_COLINEAR | WOP_DEFAULT_WIDTH, 0);
}

void dbWireEncoder::newPathShort(int jct_id,
                                 dbTechLayer* layer,
                                 dbWireType type)
{
  ZASSERT((jct_id >= 0) && (jct_id < _idx));
  initPath(layer, type);
  addOp(WOP_SHORT | _wire_type, layer->getImpl()->getOID());
  addOp(WOP_OPERAND, jct_id);
}

void dbWireEncoder::newPathVirtualWire(int jct_id,
                                       dbTechLayer* layer,
                                       dbWireType type)
{
  ZASSERT((jct_id >= 0) && (jct_id < _idx));
  initPath(layer, type);
  addOp(WOP_VWIRE | _wire_type, layer->getImpl()->getOID());
  addOp(WOP_OPERAND, jct_id);
}

void dbWireEncoder::newPathExt(int jct_id, int ext)
{
  ZASSERT((jct_id >= 0) && (jct_id < _idx));
  WirePoint pnt;
  getPrevPoint(_tech, _block, _opcodes, _data, jct_id, true, pnt);
  initPath(pnt._layer, _wire_type);
  _x = pnt._x;
  _y = pnt._y;
  _point_cnt = 1;
  addOp(WOP_JUNCTION | _wire_type, jct_id);
  addOp(WOP_COLINEAR | WOP_EXTENSION | WOP_DEFAULT_WIDTH, ext);
}

void dbWireEncoder::newPathExt(int jct_id, int ext, dbWireType type)
{
  ZASSERT((jct_id >= 0) && (jct_id < _idx));
  WirePoint pnt;
  getPrevPoint(_tech, _block, _opcodes, _data, jct_id, true, pnt);
  initPath(pnt._layer, type);
  _x = pnt._x;
  _y = pnt._y;
  _point_cnt = 1;
  addOp(WOP_JUNCTION | _wire_type, jct_id);
  addOp(WOP_COLINEAR | WOP_EXTENSION | WOP_DEFAULT_WIDTH, ext);
}

void dbWireEncoder::newPath(int jct_id, dbTechLayerRule* rule)
{
  ZASSERT((jct_id >= 0) && (jct_id < _idx));
  WirePoint pnt;
  getPrevPoint(_tech, _block, _opcodes, _data, jct_id, true, pnt);
  initPath(pnt._layer, _wire_type, rule);
  _x = pnt._x;
  _y = pnt._y;
  _point_cnt = 1;
  addOp(WOP_JUNCTION | _wire_type, jct_id);
  addOp(_rule_opcode, _non_default_rule);
  addOp(WOP_COLINEAR, 0);
}

void dbWireEncoder::newPath(int jct_id, dbWireType type, dbTechLayerRule* rule)
{
  ZASSERT((jct_id >= 0) && (jct_id < _idx));
  WirePoint pnt;
  getPrevPoint(_tech, _block, _opcodes, _data, jct_id, true, pnt);
  initPath(pnt._layer, type, rule);
  _x = pnt._x;
  _y = pnt._y;
  _point_cnt = 1;
  addOp(WOP_JUNCTION | _wire_type, jct_id);
  addOp(_rule_opcode, _non_default_rule);
  addOp(WOP_COLINEAR, 0);
}

void dbWireEncoder::newPathShort(int jct_id,
                                 dbTechLayer* layer,
                                 dbWireType type,
                                 dbTechLayerRule* rule)
{
  ZASSERT((jct_id >= 0) && (jct_id < _idx));
  initPath(layer, type, rule);
  addOp(WOP_SHORT | _wire_type, layer->getImpl()->getOID());
  addOp(WOP_OPERAND, jct_id);
  addOp(_rule_opcode, _non_default_rule);
}

void dbWireEncoder::newPathVirtualWire(int jct_id,
                                       dbTechLayer* layer,
                                       dbWireType type,
                                       dbTechLayerRule* rule)
{
  ZASSERT((jct_id >= 0) && (jct_id < _idx));
  initPath(layer, type, rule);
  addOp(WOP_VWIRE | _wire_type, layer->getImpl()->getOID());
  addOp(WOP_OPERAND, jct_id);
  addOp(_rule_opcode, _non_default_rule);
}

void dbWireEncoder::newPathExt(int jct_id, int ext, dbTechLayerRule* rule)
{
  ZASSERT((jct_id >= 0) && (jct_id < _idx));
  WirePoint pnt;
  getPrevPoint(_tech, _block, _opcodes, _data, jct_id, true, pnt);
  initPath(pnt._layer, _wire_type, rule);
  _x = pnt._x;
  _y = pnt._y;
  _point_cnt = 1;
  addOp(WOP_JUNCTION | _wire_type, jct_id);
  addOp(_rule_opcode, _non_default_rule);
  addOp(WOP_COLINEAR | WOP_EXTENSION, ext);
}

void dbWireEncoder::newPathExt(int jct_id,
                               int ext,
                               dbWireType type,
                               dbTechLayerRule* rule)
{
  ZASSERT((jct_id >= 0) && (jct_id < _idx));
  WirePoint pnt;
  getPrevPoint(_tech, _block, _opcodes, _data, jct_id, true, pnt);
  initPath(pnt._layer, type, rule);
  _x = pnt._x;
  _y = pnt._y;
  _point_cnt = 1;
  addOp(WOP_JUNCTION | _wire_type, jct_id);
  addOp(_rule_opcode, _non_default_rule);
  addOp(WOP_COLINEAR | WOP_EXTENSION, ext);
}

void dbWireEncoder::end()
{
  if (_opcodes.size() == 0)
    return;

  uint n = _opcodes.size();

  // Free the old memory
  _wire->_data.~dbVector<int>();
  new (&_wire->_data) dbVector<int>();
  _wire->_data.reserve(n);
  _wire->_data = _data;

  // Free the old memory
  _wire->_opcodes.~dbVector<unsigned char>();
  new (&_wire->_opcodes) dbVector<unsigned char>();
  _wire->_opcodes.reserve(n);
  _wire->_opcodes = _opcodes;

  // Should we calculate the bbox???
  ((_dbBlock*) _block)->_flags._valid_bbox = 0;
  _point_cnt = 0;
}

//////////////////////////////////////////////////////////////////////////////////
//
// dbWireDecoder
//
//////////////////////////////////////////////////////////////////////////////////

#define DB_WIRE_DECODE_INVALID_OPCODE 0
#define DB_WIRE_DECODE_INVALID_OPCODE_SEQUENCE 0
#define DB_WIRE_DECODE_INVALID_VIA_LAYER 0

dbWireDecoder::dbWireDecoder()
{
  _wire = NULL;
  _block = NULL;
  _tech = NULL;
}

dbWireDecoder::~dbWireDecoder()
{
}

void dbWireDecoder::begin(dbWire* wire)
{
  _wire = (_dbWire*) wire;
  _block = wire->getBlock();
  _tech = _block->getDb()->getTech();
  _x = 0;
  _y = 0;
  _default_width = true;
  _layer = NULL;
  _idx = 0;
  _jct_id = -1;
  _opcode = END_DECODE;
  _wire_type = dbWireType::NONE;
  _point_cnt = 0;
  _property = 0;
  _deltaX1 = 0;
  _deltaY1 = 0;
  _deltaX2 = 0;
  _deltaY2 = 0;
}

inline unsigned char dbWireDecoder::nextOp(int& value)
{
  ZASSERT(_idx < (int) _wire->length());
  value = _wire->_data[_idx];
  return _wire->_opcodes[_idx++];
}

inline unsigned char dbWireDecoder::nextOp(uint& value)
{
  ZASSERT(_idx < (int) _wire->length());
  value = (uint) _wire->_data[_idx];
  return _wire->_opcodes[_idx++];
}

inline unsigned char dbWireDecoder::peekOp()
{
  ZASSERT(_idx < (int) _wire->length());
  return _wire->_opcodes[_idx];
}

//
// Flush the rule opcode if it exists here. The rule opcode that follows a point
// is used to speed up shape-id searches.
//
inline void dbWireDecoder::flushRule()
{
  if (_idx != (int) _wire->_opcodes.size())
    if ((_wire->_opcodes[_idx] & WOP_OPCODE_MASK) == WOP_RULE)
      ++_idx;
}

dbWireDecoder::OpCode dbWireDecoder::peek() const
{
  ZASSERT(_wire);

  int idx = _idx;

nextOpCode:

  if (idx == (int) _wire->_opcodes.size())
    return END_DECODE;

  unsigned char opcode = _wire->_opcodes[idx];

  switch (opcode & WOP_OPCODE_MASK) {
    case WOP_PATH:
      return PATH;

    case WOP_SHORT:
      return SHORT;

    case WOP_JUNCTION:
      return JUNCTION;

    case WOP_RULE:
      return RULE;

    case WOP_X:
    case WOP_Y:
    case WOP_COLINEAR:
      if (opcode & WOP_EXTENSION)
        return POINT_EXT;

      return POINT;

    case WOP_VIA:
      return VIA;

    case WOP_TECH_VIA:
      return TECH_VIA;

    case WOP_ITERM:
      return ITERM;

    case WOP_BTERM:
      return BTERM;

    case WOP_RECT:
      return RECT;

    case WOP_VWIRE:
      return VWIRE;

    default:
      ++idx;
      goto nextOpCode;
  }
}

dbWireDecoder::OpCode dbWireDecoder::next()
{
  ZASSERT(_wire);

nextOpCode:

  if (_idx == (int) _wire->_opcodes.size())
    return END_DECODE;

  _jct_id = _idx;
  unsigned char opcode = nextOp(_operand);

  switch (opcode & WOP_OPCODE_MASK) {
    case WOP_PATH: {
      _layer = dbTechLayer::getTechLayer(_tech, _operand);
      _point_cnt = 0;
      _default_width = true;

      switch (opcode & WOP_WIRE_TYPE_MASK) {
        case WOP_NONE:
          _wire_type = dbWireType::NONE;
          break;

        case WOP_COVER:
          _wire_type = dbWireType::COVER;
          break;

        case WOP_FIXED:
          _wire_type = dbWireType::FIXED;
          break;

        case WOP_ROUTED:
          _wire_type = dbWireType::ROUTED;
          break;

        case WOP_NOSHIELD:
          _wire_type = dbWireType::NOSHIELD;
          break;
      }

      return _opcode = PATH;
    }

    case WOP_SHORT: {
      _layer = dbTechLayer::getTechLayer(_tech, _operand);
      _point_cnt = 0;
      _default_width = true;

      switch (opcode & WOP_WIRE_TYPE_MASK) {
        case WOP_NONE:
          _wire_type = dbWireType::NONE;
          break;

        case WOP_COVER:
          _wire_type = dbWireType::COVER;
          break;

        case WOP_FIXED:
          _wire_type = dbWireType::FIXED;
          break;

        case WOP_ROUTED:
          _wire_type = dbWireType::ROUTED;
          break;

        case WOP_NOSHIELD:
          _wire_type = dbWireType::NOSHIELD;
          break;
      }

      // getjuction id
      nextOp(_operand2);
      return _opcode = SHORT;
    }

    case WOP_JUNCTION: {
      WirePoint pnt;
      getPrevPoint(
          _tech, _block, _wire->_opcodes, _wire->_data, _operand, true, pnt);
      _layer = pnt._layer;
      _x = pnt._x;
      _y = pnt._y;
      _point_cnt = 0;
      _default_width = true;

      switch (opcode & WOP_WIRE_TYPE_MASK) {
        case WOP_NONE:
          _wire_type = dbWireType::NONE;
          break;

        case WOP_COVER:
          _wire_type = dbWireType::COVER;
          break;

        case WOP_FIXED:
          _wire_type = dbWireType::FIXED;
          break;

        case WOP_ROUTED:
          _wire_type = dbWireType::ROUTED;
          break;

        case WOP_NOSHIELD:
          _wire_type = dbWireType::NOSHIELD;
          break;
      }

      return _opcode = JUNCTION;
    }

    case WOP_RULE:
      _default_width = false;
      _block_rule = (opcode & WOP_BLOCK_RULE);
      return _opcode = RULE;

    case WOP_X: {
      _x = _operand;

      if (_point_cnt == 0) {
        _jct_id = _idx;
        opcode = nextOp(_y);
        _point_cnt = 1;

        if (opcode & WOP_EXTENSION) {
          nextOp(_operand2);
          return _opcode = POINT_EXT;
        }

        return _opcode = POINT;
      }

      _point_cnt++;

      if (opcode & WOP_EXTENSION) {
        nextOp(_operand2);
        _opcode = POINT_EXT;
      } else
        _opcode = POINT;

      // if ( peekOp() == WOP_PROPERTY )
      //{
      //    opcode = nextOp(_property);
      //    assert(opcode == WOP_PROPERTY);
      //}
      // else
      _property = 0;

      flushRule();
      return _opcode;
    }

    case WOP_Y: {
      ZASSERT(_point_cnt != 0);
      _point_cnt++;
      _y = _operand;

      if (opcode & WOP_EXTENSION) {
        nextOp(_operand2);
        _opcode = POINT_EXT;
      } else
        _opcode = POINT;

      // if ( peekOp() == WOP_PROPERTY )
      //{
      //    opcode = nextOp(_property);
      //    assert(opcode == WOP_PROPERTY);
      //}
      // else
      _property = 0;

      flushRule();
      return _opcode;
    }

    case WOP_COLINEAR: {
      _point_cnt++;

      if (opcode & WOP_EXTENSION) {
        _operand2 = _operand;
        _opcode = POINT_EXT;
      } else
        _opcode = POINT;

      // if ( peekOp() == WOP_PROPERTY )
      //{
      //    opcode = nextOp(_property);
      //    assert(opcode == WOP_PROPERTY);
      //}
      // else
      _property = 0;

      flushRule();
      return _opcode;
    }

    case WOP_VIA: {
      dbVia* via = dbVia::getVia(_block, _operand);

      if (opcode & WOP_VIA_EXIT_TOP)
        _layer = via->getTopLayer();
      else
        _layer = via->getBottomLayer();

      return _opcode = VIA;
    }

    case WOP_TECH_VIA: {
      dbTechVia* via = dbTechVia::getTechVia(_tech, _operand);

      if (opcode & WOP_VIA_EXIT_TOP)
        _layer = via->getTopLayer();
      else
        _layer = via->getBottomLayer();

      return _opcode = TECH_VIA;
    }

    case WOP_RECT:
      // order matches dbWireEncoder::addRect
      _deltaX1 = _operand;
      nextOp(_deltaY1);
      nextOp(_deltaX2);
      nextOp(_deltaY2);
      return _opcode = RECT;

    case WOP_ITERM:
      return _opcode = ITERM;

    case WOP_BTERM:
      return _opcode = BTERM;

    case WOP_OPERAND:
      ZASSERT(DB_WIRE_DECODE_INVALID_OPCODE_SEQUENCE);
      goto nextOpCode;

    case WOP_PROPERTY:
      goto nextOpCode;

    case WOP_VWIRE: {
      _layer = dbTechLayer::getTechLayer(_tech, _operand);
      _point_cnt = 0;
      _default_width = true;

      switch (opcode & WOP_WIRE_TYPE_MASK) {
        case WOP_NONE:
          _wire_type = dbWireType::NONE;
          break;

        case WOP_COVER:
          _wire_type = dbWireType::COVER;
          break;

        case WOP_FIXED:
          _wire_type = dbWireType::FIXED;
          break;

        case WOP_ROUTED:
          _wire_type = dbWireType::ROUTED;
          break;

        case WOP_NOSHIELD:
          _wire_type = dbWireType::NOSHIELD;
          break;
      }

      // getjuction id
      nextOp(_operand2);
      return _opcode = VWIRE;
    }

    case WOP_NOP:
      goto nextOpCode;

    default:
      ZASSERT(DB_WIRE_DECODE_INVALID_OPCODE);
      goto nextOpCode;
  }
}

dbTechLayer* dbWireDecoder::getLayer() const
{
  return _layer;
}

dbTechLayerRule* dbWireDecoder::getRule() const
{
  ZASSERT(_opcode == RULE);

  if (_block_rule) {
    return dbTechLayerRule::getTechLayerRule(_block, _operand);
  } else {
    return dbTechLayerRule::getTechLayerRule(_tech, _operand);
  }
}

void dbWireDecoder::getPoint(int& x, int& y) const
{
  ZASSERT(_opcode == POINT);
  x = _x;
  y = _y;
}

void dbWireDecoder::getPoint(int& x, int& y, int& ext) const
{
  ZASSERT(_opcode == POINT_EXT);
  x = _x;
  y = _y;
  ext = _operand2;
}

uint dbWireDecoder::getProperty() const
{
  ZASSERT(((_opcode == POINT) || (_opcode == POINT_EXT)) && (_point_cnt > 1));
  return _property;
}

dbVia* dbWireDecoder::getVia() const
{
  ZASSERT(_opcode == VIA);
  dbVia* via = dbVia::getVia(_block, _operand);
  return via;
}

dbTechVia* dbWireDecoder::getTechVia() const
{
  ZASSERT(_opcode == TECH_VIA);
  dbTechVia* via = dbTechVia::getTechVia(_tech, _operand);
  return via;
}

void dbWireDecoder::getRect(int& deltaX1,
                            int& deltaY1,
                            int& deltaX2,
                            int& deltaY2) const
{
  ZASSERT(_opcode == RECT);
  deltaX1 = _deltaX1;
  deltaY1 = _deltaY1;
  deltaX2 = _deltaX2;
  deltaY2 = _deltaY2;
}

dbITerm* dbWireDecoder::getITerm() const
{
  ZASSERT(_opcode == ITERM);
  dbITerm* iterm = dbITerm::getITerm(_block, _operand);
  return iterm;
}

dbBTerm* dbWireDecoder::getBTerm() const
{
  ZASSERT(_opcode == BTERM);
  dbBTerm* bterm = dbBTerm::getBTerm(_block, _operand);
  return bterm;
}

dbWireType dbWireDecoder::getWireType() const
{
  ZASSERT((_opcode == PATH) || (_opcode == JUNCTION) || (_opcode == SHORT)
          || (_opcode == VWIRE));
  return dbWireType((dbWireType::Value) _wire_type);
}

int dbWireDecoder::getJunctionId() const
{
  return _jct_id;
}

int dbWireDecoder::getJunctionValue() const
{
  ZASSERT((_opcode == JUNCTION) || (_opcode == SHORT) || (_opcode == VWIRE));

  if (_opcode == SHORT || _opcode == VWIRE)
    return _operand2;

  return _operand;
}

void dumpDecoder4Net(dbNet* innet)
{
  if (!innet)
    return;

  const char* prfx = "dumpDecoder:";
  dbWire* wire0 = innet->getWire();
  utl::Logger* logger = innet->getImpl()->getLogger();
  if (!wire0) {
    logger->warn(
        utl::ODB, 63, "{} No wires for net {}", prfx, innet->getName());
    return;
  }

  dbWireDecoder decoder;
  dbWireDecoder::OpCode opcode;
  int x, y, ext;
  decoder.begin(wire0);
  logger->info(
      utl::ODB, 64, "{} begin decoder for net {}", prfx, innet->getName());

  dbTechLayer* layer;
  dbWireType wtype;
  dbTechLayerRule* lyr_rule = nullptr;
  while (1) {
    opcode = decoder.next();
    if (opcode == dbWireDecoder::END_DECODE) {
      logger->info(
          utl::ODB, 65, "{} End decoder for net {}", prfx, innet->getName());
      break;
    }

    switch (opcode) {
      case dbWireDecoder::PATH: {
        layer = decoder.getLayer();
        wtype = decoder.getWireType();
        if (decoder.peek() == dbWireDecoder::RULE) {
          opcode = decoder.next();
          lyr_rule = decoder.getRule();
          logger->info(utl::ODB,
                       66,
                       "{} New path: layer {} type {}  non-default rule {}",
                       prfx,
                       layer->getName(),
                       wtype.getString(),
                       lyr_rule->getNonDefaultRule()->getName());

        } else {
          logger->info(utl::ODB,
                       67,
                       "{} New path: layer {} type {}\n",
                       prfx,
                       layer->getName(),
                       wtype.getString());
        }
        break;
      }

      case dbWireDecoder::JUNCTION: {
        uint jct = decoder.getJunctionValue();
        lyr_rule = NULL;
        opcode = decoder.peek();
        if (opcode == dbWireDecoder::RULE) {
          opcode = decoder.next();
          lyr_rule = decoder.getRule();
          opcode = decoder.peek();
        }
        if (opcode == dbWireDecoder::POINT_EXT) {
          opcode = decoder.next();
          decoder.getPoint(x, y, ext);
          if (lyr_rule)
            logger->info(
                utl::ODB,
                68,
                "{} New path at junction {}, point(ext) {} {} {}, with rule "
                "{}",
                prfx,
                jct,
                x,
                y,
                ext,
                lyr_rule->getNonDefaultRule()->getName());
          else
            logger->info(utl::ODB,
                         69,
                         "{} New path at junction {}, point(ext) {} {} {}",
                         prfx,
                         jct,
                         x,
                         y,
                         ext);
        } else if (opcode == dbWireDecoder::POINT) {
          opcode = decoder.next();
          decoder.getPoint(x, y);
          if (lyr_rule)
            logger->info(
                utl::ODB,
                70,
                "{} New path at junction {}, point {} {}, with rule {}",
                prfx,
                jct,
                x,
                y,
                lyr_rule->getNonDefaultRule()->getName());
          else
            logger->info(utl::ODB,
                         71,
                         "{} New path at junction {}, point {} {}",
                         prfx,
                         jct,
                         x,
                         y);

        } else {
          logger->warn(utl::ODB,
                       72,
                       "{} opcode after junction is not point or point_ext??\n",
                       prfx);
        }
        break;
      }

      case dbWireDecoder::SHORT: {
        uint jval = decoder.getJunctionValue();
        layer = decoder.getLayer();
        wtype = decoder.getWireType();
        lyr_rule = NULL;
        opcode = decoder.peek();
        if (opcode == dbWireDecoder::RULE) {
          opcode = decoder.next();
          lyr_rule = decoder.getRule();
        }
        if (lyr_rule)
          logger->info(utl::ODB,
                       73,
                       "{} Short at junction {}, with rule {}",
                       prfx,
                       jval,
                       lyr_rule->getNonDefaultRule()->getName());
        else
          logger->info(utl::ODB, 74, "{} Short at junction {}", prfx, jval);
        break;
      }

      case dbWireDecoder::VWIRE: {
        uint jval = decoder.getJunctionValue();
        layer = decoder.getLayer();
        wtype = decoder.getWireType();
        lyr_rule = NULL;
        opcode = decoder.peek();
        if (opcode == dbWireDecoder::RULE) {
          opcode = decoder.next();
          lyr_rule = decoder.getRule();
        }
        if (lyr_rule)
          logger->info(utl::ODB,
                       75,
                       "{} Virtual wire at junction {}, with rule {}",
                       prfx,
                       jval,
                       lyr_rule->getNonDefaultRule()->getName());
        else
          logger->info(
              utl::ODB, 76, "{} Virtual wire at junction {}", prfx, jval);
        break;
      }

      case dbWireDecoder::POINT: {
        decoder.getPoint(x, y);
        logger->info(utl::ODB, 77, "{} Found point {} {}", prfx, x, y);
        break;
      }

      case dbWireDecoder::POINT_EXT: {
        decoder.getPoint(x, y, ext);
        logger->info(
            utl::ODB, 78, "{} Found point(ext){} {} {}", prfx, x, y, ext);
        break;
      }

      case dbWireDecoder::TECH_VIA: {
        logger->info(utl::ODB,
                     79,
                     "{} Found via {}",
                     prfx,
                     decoder.getTechVia()->getName());
        break;
      }

      case dbWireDecoder::VIA: {
        logger->info(utl::ODB, 80, "block via found in signal net!");
        break;
      }

      case dbWireDecoder::RECT: {
        int deltaX1;
        int deltaY1;
        int deltaX2;
        int deltaY2;
        decoder.getRect(deltaX1, deltaY1, deltaX2, deltaY2);
        break;
      }

      case dbWireDecoder::ITERM: {
        logger->info(utl::ODB, 81, "{} Found Iterm", prfx);
        break;
      }

      case dbWireDecoder::BTERM: {
        logger->info(utl::ODB, 82, "{} Found Bterm", prfx);
        break;
      }

      case dbWireDecoder::RULE: {
        if (strcmp(
                lyr_rule->getNonDefaultRule()->getName().c_str(),
                (decoder.getRule())->getNonDefaultRule()->getName().c_str())) {
          logger->error(
              utl::ODB,
              83,
              "{} GOT RULE {}, EXPECTED RULE {}",
              prfx,
              lyr_rule->getNonDefaultRule()->getName().c_str(),
              decoder.getRule()->getNonDefaultRule()->getName().c_str());
        }
        lyr_rule = decoder.getRule();
        logger->info(utl::ODB,
                     84,
                     "{} Found Rule {} in middle of path",
                     prfx,
                     lyr_rule->getNonDefaultRule()->getName().c_str());
        break;
      }

      case dbWireDecoder::END_DECODE:
        logger->info(utl::ODB,
                     85,
                     "{} End decoder for net {}",
                     prfx,
                     innet->getName().c_str());
        break;

      default: {
        logger->error(utl::ODB, 86, "{} Hit default!", prfx);
        break;
      }
    }  // switch opcode
  }    // while
}

void dumpDecoder(dbBlock* inblk, const char* net_name_or_id)
{
  const char* prfx = "dumpDecoder:";

  if (!inblk || !net_name_or_id) {
    // error(0, "%s Must specify DB Block and either net name or ID\n", prfx);
    return;
  }

  dbNet* innet = NULL;
  const char* ckdigit;
  for (ckdigit = net_name_or_id; *ckdigit; ckdigit++)
    if (!isdigit(*ckdigit))
      break;

  innet = (*ckdigit == '\0') ? dbNet::getNet(inblk, atoi(net_name_or_id))
                             : inblk->findNet(net_name_or_id);
  if (!innet) {
    inblk->getImpl()->getLogger()->error(
        utl::ODB, 0, "{} Net {} not found", prfx, net_name_or_id);
    return;
  }

  dumpDecoder4Net(innet);
}

}  // namespace odb
