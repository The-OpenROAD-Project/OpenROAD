// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cassert>

#include "dbBlock.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbWire.h"
#include "dbWireOpcode.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"

namespace odb {

#define DB_WIRE_DECODE_INVALID_OPCODE 0

//////////////////////////////////////////////////////////////////////////////////
//
// dbWireShapeItr
//
//////////////////////////////////////////////////////////////////////////////////
dbWireShapeItr::dbWireShapeItr()
{
  wire_ = nullptr;
  block_ = nullptr;
  tech_ = nullptr;
}

inline unsigned char dbWireShapeItr::nextOp(int& value)
{
  assert(idx_ < (int) wire_->length());
  value = wire_->data_[idx_];
  return wire_->opcodes_[idx_++];
}

inline unsigned char dbWireShapeItr::peekOp()
{
  assert(idx_ < (int) wire_->length());
  return wire_->opcodes_[idx_];
}

void dbWireShapeItr::begin(dbWire* wire)
{
  wire_ = (_dbWire*) wire;
  block_ = wire->getBlock();
  tech_ = block_->getTech();
  idx_ = 0;
  prev_x_ = 0;
  prev_y_ = 0;
  prev_ext_ = 0;
  has_prev_ext_ = false;
  layer_ = nullptr;
  via_ = nullptr;
  dw_ = 0;
  point_cnt_ = 0;
  has_width_ = false;
}

bool dbWireShapeItr::next(dbShape& shape)
{
  int operand;

nextOpCode:
  shape_id_ = idx_;

  if (idx_ == (int) wire_->opcodes_.size()) {
    return false;
  }

  unsigned char opcode = nextOp(operand);

  switch (opcode & WOP_OPCODE_MASK) {
    case kPath:
    case kShort:
    case kVwire: {
      layer_ = dbTechLayer::getTechLayer(tech_, operand);
      point_cnt_ = 0;
      dw_ = layer_->getWidth() >> 1;
      goto nextOpCode;
    }

    case kJunction: {
      WirePoint pnt;
      getPrevPoint(
          tech_, block_, wire_->opcodes_, wire_->data_, operand, true, pnt);
      layer_ = pnt.layer;
      prev_x_ = pnt.x;
      prev_y_ = pnt.y;
      prev_ext_ = 0;
      has_prev_ext_ = false;
      point_cnt_ = 0;
      dw_ = layer_->getWidth() >> 1;
      goto nextOpCode;
    }

    case kRule: {
      if (opcode & WOP_BLOCK_RULE) {
        dbTechLayerRule* rule
            = dbTechLayerRule::getTechLayerRule(block_, operand);
        dw_ = rule->getWidth() >> 1;
      } else {
        dbTechLayerRule* rule
            = dbTechLayerRule::getTechLayerRule(tech_, operand);
        dw_ = rule->getWidth() >> 1;
      }

      has_width_ = true;
      goto nextOpCode;
    }

    case kX: {
      int cur_x = operand;
      int cur_y;

      if (point_cnt_ == 0) {
        opcode = nextOp(cur_y);
      } else {
        cur_y = prev_y_;
      }

      int cur_ext;
      bool has_cur_ext;

      if (opcode & WOP_EXTENSION) {
        nextOp(cur_ext);
        has_cur_ext = true;
      } else {
        cur_ext = 0;
        has_cur_ext = false;
      }

      if (point_cnt_++ == 0) {
        prev_x_ = cur_x;
        prev_y_ = cur_y;
        prev_ext_ = cur_ext;
        has_prev_ext_ = has_cur_ext;
        goto nextOpCode;
      }
      auto dw = dw_;
      if (!has_width_ && layer_->getDirection() == dbTechLayerDir::VERTICAL) {
        dw = layer_->getWrongWayWidth() / 2;
      }

      shape.setSegment(prev_x_,
                       prev_y_,
                       prev_ext_,
                       has_prev_ext_,
                       cur_x,
                       cur_y,
                       cur_ext,
                       has_cur_ext,
                       dw,
                       dw_,
                       layer_);
      prev_x_ = cur_x;
      prev_y_ = cur_y;
      prev_ext_ = cur_ext;
      has_prev_ext_ = has_cur_ext;
      return true;
    }

    case kY: {
      assert(point_cnt_ != 0);
      point_cnt_++;
      int cur_y = operand;
      int cur_x = prev_x_;
      int cur_ext;
      bool has_cur_ext;

      if (opcode & WOP_EXTENSION) {
        nextOp(cur_ext);
        has_cur_ext = true;
      } else {
        cur_ext = 0;
        has_cur_ext = false;
      }
      auto dw = dw_;

      if (!has_width_ && layer_->getDirection() == dbTechLayerDir::HORIZONTAL) {
        dw = layer_->getWrongWayWidth() / 2;
      }
      shape.setSegment(prev_x_,
                       prev_y_,
                       prev_ext_,
                       has_prev_ext_,
                       cur_x,
                       cur_y,
                       cur_ext,
                       has_cur_ext,
                       dw,
                       dw_,
                       layer_);
      prev_x_ = cur_x;
      prev_y_ = cur_y;
      prev_ext_ = cur_ext;
      has_prev_ext_ = has_cur_ext;
      return true;
    }

    case kColinear: {
      point_cnt_++;

      // A colinear-point with an extension begins a new path-segment
      if (opcode & WOP_EXTENSION) {
        prev_ext_ = operand;
        has_prev_ext_ = true;
        goto nextOpCode;
      }

      // A colinear-point following an extension cancels the ext
      if (has_prev_ext_) {
        prev_ext_ = 0;
        has_prev_ext_ = false;
        goto nextOpCode;
      }

      if (point_cnt_ > 1) {
        shape.setSegment(prev_x_,
                         prev_y_,
                         prev_ext_,
                         has_prev_ext_,
                         prev_x_,
                         prev_y_,
                         0,
                         false,
                         dw_,
                         dw_,
                         layer_);
        has_prev_ext_ = false;
        return true;
      }

      has_prev_ext_ = false;
      goto nextOpCode;
    }

    case kVia: {
      dbVia* via = dbVia::getVia(block_, operand);

      if (opcode & WOP_VIA_EXIT_TOP) {
        layer_ = via->getTopLayer();
      } else {
        layer_ = via->getBottomLayer();
      }

      if (has_width_ == false) {
        dw_ = layer_->getWidth() >> 1;
      }

      prev_ext_ = 0;
      has_prev_ext_ = false;

      dbBox* box = via->getBBox();

      if (box == nullptr) {
        goto nextOpCode;
      }

      Rect b = box->getBox();
      int xmin = b.xMin() + prev_x_;
      int ymin = b.yMin() + prev_y_;
      int xmax = b.xMax() + prev_x_;
      int ymax = b.yMax() + prev_y_;
      Rect r(xmin, ymin, xmax, ymax);
      shape.setVia(via, r);
      return true;
    }

    case kTechVia: {
      dbTechVia* via = dbTechVia::getTechVia(tech_, operand);

      if (opcode & WOP_VIA_EXIT_TOP) {
        layer_ = via->getTopLayer();
      } else {
        layer_ = via->getBottomLayer();
      }

      if (has_width_ == false) {
        dw_ = layer_->getWidth() >> 1;
      }

      prev_ext_ = 0;
      has_prev_ext_ = false;

      dbBox* box = via->getBBox();

      if (box == nullptr) {
        goto nextOpCode;
      }

      Rect b = box->getBox();
      int xmin = b.xMin() + prev_x_;
      int ymin = b.yMin() + prev_y_;
      int xmax = b.xMax() + prev_x_;
      int ymax = b.yMax() + prev_y_;
      Rect r(xmin, ymin, xmax, ymax);
      shape.setVia(via, r);
      return true;
    }

    case kRect: {
      int deltaX1 = operand;
      int deltaY1;
      int deltaX2;
      int deltaY2;
      nextOp(deltaY1);
      nextOp(deltaX2);
      nextOp(deltaY2);
      shape.setSegmentFromRect(prev_x_ + deltaX1,
                               prev_y_ + deltaY1,
                               prev_x_ + deltaX2,
                               prev_y_ + deltaY2,
                               layer_);
      return true;
    }

    case kIterm:
    case kBterm:
    case kOperand:
    case kProperty:
    case kColor:
    case kViaColor:
    case kNop:
      goto nextOpCode;

    default:
      assert(DB_WIRE_DECODE_INVALID_OPCODE);
      goto nextOpCode;
  }

  return false;
}

int dbWireShapeItr::getShapeId()
{
  return shape_id_;
}

}  // namespace odb
