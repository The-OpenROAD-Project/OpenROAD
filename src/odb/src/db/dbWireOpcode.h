// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cassert>

#include "odb/db.h"

namespace odb {

///
/// dbWireCodec: - Opcode definitions
///
#define WOP_NON_DEFAULT_WIDTH_POINT_CNT \
  16  // Count of points before a "width" is inserted. Optimization for
      // searching data-structure. This count is always a power of 2.

#define WOP_OPCODE_MASK \
  0x1F  // Mask to determine opcode from op-byte. Currently the opcode is 5
        // bits. There are 3 bits for various flags.

//
// opcodes               BIT: 7 6 5 4 3 2 1 0 ---- (W W W) ==  WOP_<WIRE_TYPE>,
//                                               T == WOP_VIA_EXIT_TOP,
//                                               E == WOP_EXTENSION,
//                                               D == WOP_DEFAULT_WIDTH,
//                                               B == WOP_BLOCK_RULE
//                                               X == unused bit
enum WireOp
{
  kPath = 0,       //  W W W 0 0 0 0 0 :  operand = layer-id
  kShort = 1,      //  W W W 0 0 0 0 1 :  operand = junction-id
  kJunction = 2,   //  W W W 0 0 0 1 0 :  operand = junction-id
  kRule = 3,       //  B X X 0 0 0 1 1 :  operand = rule-id
  kX = 4,          //  E D X 0 0 1 0 0 :  operand = x-coord
  kY = 5,          //  E D X 0 0 1 0 1 :  operand = y-coord
  kColinear = 6,   //  E X X 0 0 1 1 0 :  operand = (e == 1) ? ext : 0
  kVia = 7,        //  T X X 0 0 1 1 1 :  operand = via-id
  kTechVia = 8,    //  T X X 0 1 0 0 0 :  operand = via-id
  kIterm = 9,      //  X X X 0 1 0 0 1 :  operand = iterm-id
  kBterm = 10,     //  X X X 0 1 0 1 0 :  operand = bterm-id
  kOperand = 11,   //  X X X 0 1 0 1 1 :  operand = integer operand
  kProperty = 12,  //  X X X 0 1 1 0 0 :  operand = integer operand
  kVwire = 13,     //  W W W 0 1 1 0 1 :  operand = integer operand
  kRect = 14,      //  X X X 0 1 1 1 0 :  operand = first offset
  kNop = 15,       //  X X X 0 1 1 1 1 :  operand = 0
  kColor = 16,     //  X X X 1 0 0 0 0 :  operand = integer operand
  kViaColor = 17   //  X X X 1 0 0 0 1 :  operand = via color
};

// opcode-flags
#define WOP_VIA_EXIT_TOP \
  0x80  // This flag indicates the path exited through the top or bottom via
        // layer.
#define WOP_DEFAULT_WIDTH \
  0x40  // This flag indicates the path-width at this point is the default
        // layer-width.
#define WOP_EXTENSION \
  0x80  // This flag indicates the point/via has an extension operand
#define WOP_BLOCK_RULE \
  0x80  // This flag indicates non-default-rule is a block rule

// wire-type-flags
#define WOP_WIRE_TYPE_MASK 0xE0
enum WireType
{
  kNone = 0x00,
  kCover = 0x20,
  kFixed = 0x40,
  kRouted = 0x60,
  kNoShield = 0x80
};

//////////////////////////////////////////////////////////////////////////////////
//
// getPrevPoint - This function walks backwards from a given index and finds
//                the previous point relative to that index. If requested,
//                it keeps walking backward to determine the layer the point is
//                on.
//
//////////////////////////////////////////////////////////////////////////////////

struct WirePoint
{
  int x = 0;
  int y = 0;
  dbTechLayer* layer = nullptr;
};

template <class O, class D>
inline void getPrevPoint(dbTech* tech,
                         dbBlock* block,
                         O& opcodes,
                         D& data,
                         int idx,
                         bool get_layer,
                         WirePoint& pnt)
{
  unsigned char opcode;
  bool look_for_x = true;
  bool look_for_y = true;
  // quiets compiler warnings
  pnt.x = 0;
  pnt.y = 0;
  pnt.layer = nullptr;

prevOpCode:
  assert(idx >= 0);
  opcode = opcodes[idx];

  switch (opcode & WOP_OPCODE_MASK) {
    case kPath:
    case kShort: {
      if (get_layer) {
        pnt.layer = dbTechLayer::getTechLayer(tech, data[idx]);

        if ((look_for_x == false) && (look_for_y == false)) {
          return;
        }

        get_layer = false;
      }

      --idx;
      goto prevOpCode;
    }

    case kJunction: {
      idx = data[idx];
      goto prevOpCode;
    }

    case kX: {
      if (look_for_x) {
        look_for_x = false;
        pnt.x = data[idx];

        if ((look_for_y == false) && (get_layer == false)) {
          return;
        }
      }

      --idx;
      goto prevOpCode;
    }

    case kY: {
      if (look_for_y) {
        look_for_y = false;
        pnt.y = data[idx];

        if ((look_for_x == false) && (get_layer == false)) {
          return;
        }
      }

      --idx;
      goto prevOpCode;
    }

    case kVia: {
      if (get_layer) {
        dbVia* via = dbVia::getVia(block, data[idx]);

        if (opcode & WOP_VIA_EXIT_TOP) {
          pnt.layer = via->getTopLayer();
        } else {
          pnt.layer = via->getBottomLayer();
        }

        if ((look_for_x == false) && (look_for_y == false)) {
          return;
        }

        get_layer = false;
      }

      --idx;
      goto prevOpCode;
    }

    case kTechVia: {
      if (get_layer) {
        dbTechVia* via = dbTechVia::getTechVia(tech, data[idx]);

        if (opcode & WOP_VIA_EXIT_TOP) {
          pnt.layer = via->getTopLayer();
        } else {
          pnt.layer = via->getBottomLayer();
        }

        if ((look_for_x == false) && (look_for_y == false)) {
          return;
        }

        get_layer = false;
      }

      --idx;
      goto prevOpCode;
    }

    default:
      --idx;
      goto prevOpCode;
  }
}

}  // namespace odb
