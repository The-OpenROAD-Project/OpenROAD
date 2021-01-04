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

#include "db.h"
#include "dbBlock.h"
#include "dbNet.h"
#include "dbShape.h"
#include "dbTable.h"
#include "dbWire.h"
#include "dbWireCodec.h"
#include "dbWireOpcode.h"

namespace odb {

#define DB_WIRE_DECODE_INVALID_OPCODE 0

//////////////////////////////////////////////////////////////////////////////////
//
// dbWireShapeItr
//
//////////////////////////////////////////////////////////////////////////////////
dbWireShapeItr::dbWireShapeItr()
{
  _wire  = NULL;
  _block = NULL;
  _tech  = NULL;
}

dbWireShapeItr::~dbWireShapeItr()
{
}

inline unsigned char dbWireShapeItr::nextOp(int& value)
{
  ZASSERT(_idx < (int) _wire->length());
  value = _wire->_data[_idx];
  return _wire->_opcodes[_idx++];
}

inline unsigned char dbWireShapeItr::peekOp()
{
  ZASSERT(_idx < (int) _wire->length());
  return _wire->_opcodes[_idx];
}

void dbWireShapeItr::begin(dbWire* wire)
{
  _wire         = (_dbWire*) wire;
  _block        = wire->getBlock();
  _tech         = _block->getDb()->getTech();
  _idx          = 0;
  _prev_x       = 0;
  _prev_y       = 0;
  _prev_ext     = 0;
  _has_prev_ext = false;
  _layer        = NULL;
  _via          = NULL;
  _dw           = 0;
  _point_cnt    = 0;
  _has_width    = false;
}

bool dbWireShapeItr::next(dbShape& shape)
{
  ZASSERT(_wire);
  int operand;

nextOpCode:
  _shape_id = _idx;

  if (_idx == (int) _wire->_opcodes.size())
    return false;

  unsigned char opcode = nextOp(operand);

  switch (opcode & WOP_OPCODE_MASK) {
    case WOP_PATH:
    case WOP_SHORT:
    case WOP_VWIRE: {
      _layer     = dbTechLayer::getTechLayer(_tech, operand);
      _point_cnt = 0;
      _dw        = _layer->getWidth() >> 1;
      goto nextOpCode;
    }

    case WOP_JUNCTION: {
      WirePoint pnt;
      getPrevPoint(
          _tech, _block, _wire->_opcodes, _wire->_data, operand, true, pnt);
      _layer        = pnt._layer;
      _prev_x       = pnt._x;
      _prev_y       = pnt._y;
      _prev_ext     = 0;
      _has_prev_ext = false;
      _point_cnt    = 0;
      _dw           = _layer->getWidth() >> 1;
      goto nextOpCode;
    }

    case WOP_RULE: {
      if (opcode & WOP_BLOCK_RULE) {
        dbTechLayerRule* rule
            = dbTechLayerRule::getTechLayerRule(_block, operand);
        _dw = rule->getWidth() >> 1;
      } else {
        dbTechLayerRule* rule
            = dbTechLayerRule::getTechLayerRule(_tech, operand);
        _dw = rule->getWidth() >> 1;
      }

      _has_width = true;
      goto nextOpCode;
    }

    case WOP_X: {
      int cur_x = operand;
      int cur_y;

      if (_point_cnt == 0)
        opcode = nextOp(cur_y);
      else
        cur_y = _prev_y;

      int  cur_ext;
      bool has_cur_ext;

      if (opcode & WOP_EXTENSION) {
        nextOp(cur_ext);
        has_cur_ext = true;
      } else {
        cur_ext     = 0;
        has_cur_ext = false;
      }

      if (_point_cnt++ == 0) {
        _prev_x       = cur_x;
        _prev_y       = cur_y;
        _prev_ext     = cur_ext;
        _has_prev_ext = has_cur_ext;
        goto nextOpCode;
      }

      shape.setSegment(_prev_x,
                       _prev_y,
                       _prev_ext,
                       _has_prev_ext,
                       cur_x,
                       cur_y,
                       cur_ext,
                       has_cur_ext,
                       _dw,
                       _layer);
      _prev_x       = cur_x;
      _prev_y       = cur_y;
      _prev_ext     = cur_ext;
      _has_prev_ext = has_cur_ext;
      return true;
    }

    case WOP_Y: {
      ZASSERT(_point_cnt != 0);
      _point_cnt++;
      int  cur_y = operand;
      int  cur_x = _prev_x;
      int  cur_ext;
      bool has_cur_ext;

      if (opcode & WOP_EXTENSION) {
        nextOp(cur_ext);
        has_cur_ext = true;
      } else {
        cur_ext     = 0;
        has_cur_ext = false;
      }

      shape.setSegment(_prev_x,
                       _prev_y,
                       _prev_ext,
                       _has_prev_ext,
                       cur_x,
                       cur_y,
                       cur_ext,
                       has_cur_ext,
                       _dw,
                       _layer);
      _prev_x       = cur_x;
      _prev_y       = cur_y;
      _prev_ext     = cur_ext;
      _has_prev_ext = has_cur_ext;
      return true;
    }

    case WOP_COLINEAR: {
      _point_cnt++;

      // A colinear-point with an extension begins a new path-segment
      if (opcode & WOP_EXTENSION) {
        _prev_ext     = operand;
        _has_prev_ext = true;
        goto nextOpCode;
      }

      // A colinear-point following an extension cancels the ext
      if (_has_prev_ext) {
        _prev_ext     = 0;
        _has_prev_ext = false;
        goto nextOpCode;
      }

      if (_point_cnt > 1) {
        shape.setSegment(_prev_x,
                         _prev_y,
                         _prev_ext,
                         _has_prev_ext,
                         _prev_x,
                         _prev_y,
                         0,
                         false,
                         _dw,
                         _layer);
        _has_prev_ext = false;
        return true;
      }

      _has_prev_ext = false;
      goto nextOpCode;
    }

    case WOP_VIA: {
      dbVia* via = dbVia::getVia(_block, operand);

      if (opcode & WOP_VIA_EXIT_TOP)
        _layer = via->getTopLayer();
      else
        _layer = via->getBottomLayer();

      if (_has_width == false)
        _dw = _layer->getWidth() >> 1;

      _prev_ext     = 0;
      _has_prev_ext = false;

      dbBox* box = via->getBBox();

      if (box == NULL)
        goto nextOpCode;

      Rect b;
      box->getBox(b);
      int  xmin = b.xMin() + _prev_x;
      int  ymin = b.yMin() + _prev_y;
      int  xmax = b.xMax() + _prev_x;
      int  ymax = b.yMax() + _prev_y;
      Rect r(xmin, ymin, xmax, ymax);
      shape.setVia(via, r);
      return true;
    }

    case WOP_TECH_VIA: {
      dbTechVia* via = dbTechVia::getTechVia(_tech, operand);

      if (opcode & WOP_VIA_EXIT_TOP)
        _layer = via->getTopLayer();
      else
        _layer = via->getBottomLayer();

      if (_has_width == false)
        _dw = _layer->getWidth() >> 1;

      _prev_ext     = 0;
      _has_prev_ext = false;

      dbBox* box = via->getBBox();

      if (box == NULL)
        goto nextOpCode;

      Rect b;
      box->getBox(b);
      int  xmin = b.xMin() + _prev_x;
      int  ymin = b.yMin() + _prev_y;
      int  xmax = b.xMax() + _prev_x;
      int  ymax = b.yMax() + _prev_y;
      Rect r(xmin, ymin, xmax, ymax);
      shape.setVia(via, r);
      return true;
    }

    case WOP_RECT: {
      int deltaX1 = operand;
      int deltaY1;
      int deltaX2;
      int deltaY2;
      nextOp(deltaY1);
      nextOp(deltaX2);
      nextOp(deltaY2);
      shape.setSegmentFromRect(_prev_x + deltaX1,
                               _prev_y + deltaY1,
                               _prev_x + deltaX2,
                               _prev_y + deltaY2,
                               _layer);
      return true;
    }

    case WOP_ITERM:
    case WOP_BTERM:
    case WOP_OPERAND:
    case WOP_PROPERTY:
    case WOP_NOP:
      goto nextOpCode;

    default:
      ZASSERT(DB_WIRE_DECODE_INVALID_OPCODE);
      goto nextOpCode;
  }

  return false;
}

int dbWireShapeItr::getShapeId()
{
  return _shape_id;
}

}  // namespace odb
