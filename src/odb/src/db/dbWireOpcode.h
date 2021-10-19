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

#pragma once

#include "db.h"
#include "odb.h"

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
#define WOP_PATH 0       //  W W W X 0 0 0 0 :  operand = layer-id
#define WOP_SHORT 1      //  W W W X 0 0 0 1 :  operand = junction-id
#define WOP_JUNCTION 2   //  W W W X 0 0 1 0 :  operand = junction-id
#define WOP_RULE 3       //  B X X X 0 0 1 1 :  operand = rule-id
#define WOP_X 4          //  E D X X 0 1 0 0 :  operand = x-coord
#define WOP_Y 5          //  E D X X 0 1 0 1 :  operand = y-coord
#define WOP_COLINEAR 6   //  E X X X 0 1 1 0 :  operand = (e == 1) ? ext : 0
#define WOP_VIA 7        //  T X X X 0 1 1 1 :  operand = via-id
#define WOP_TECH_VIA 8   //  T X X X 1 0 0 0 :  operand = via-id
#define WOP_ITERM 9      //  X X X X 1 0 0 1 :  operand = iterm-id
#define WOP_BTERM 10     //  X X X X 1 0 1 0 :  operand = bterm-id
#define WOP_OPERAND 11   //  X X X X 1 0 1 1 :  operand = integer operand
#define WOP_PROPERTY 12  //  X X X X 1 1 0 0 :  operand = integer operand
#define WOP_VWIRE 13     //  W W W X 1 1 0 1 :  operand = integer operand
#define WOP_RECT 14      //  X X X X 1 1 1 0 :  operand = first offset
#define WOP_NOP 15       //  X X X X 1 1 1 1 :  operand = 0

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
#define WOP_NONE 0x00
#define WOP_COVER 0x20
#define WOP_FIXED 0x40
#define WOP_ROUTED 0x60
#define WOP_NOSHIELD 0x80

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
  int _x;
  int _y;
  dbTechLayer* _layer;
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
  pnt._x = 0;
  pnt._y = 0;
  pnt._layer = nullptr;

prevOpCode:
  ZASSERT(idx >= 0);
  opcode = opcodes[idx];

  switch (opcode & WOP_OPCODE_MASK) {
    case WOP_PATH:
    case WOP_SHORT: {
      if (get_layer) {
        pnt._layer = dbTechLayer::getTechLayer(tech, data[idx]);

        if ((look_for_x == false) && (look_for_y == false))
          return;

        get_layer = false;
      }

      --idx;
      goto prevOpCode;
    }

    case WOP_JUNCTION: {
      idx = data[idx];
      goto prevOpCode;
    }

    case WOP_X: {
      if (look_for_x) {
        look_for_x = false;
        pnt._x = data[idx];

        if ((look_for_y == false) && (get_layer == false))
          return;
      }

      --idx;
      goto prevOpCode;
    }

    case WOP_Y: {
      if (look_for_y) {
        look_for_y = false;
        pnt._y = data[idx];

        if ((look_for_x == false) && (get_layer == false))
          return;
      }

      --idx;
      goto prevOpCode;
    }

    case WOP_VIA: {
      if (get_layer) {
        dbVia* via = dbVia::getVia(block, data[idx]);

        if (opcode & WOP_VIA_EXIT_TOP)
          pnt._layer = via->getTopLayer();
        else
          pnt._layer = via->getBottomLayer();

        if ((look_for_x == false) && (look_for_y == false))
          return;

        get_layer = false;
      }

      --idx;
      goto prevOpCode;
    }

    case WOP_TECH_VIA: {
      if (get_layer) {
        dbTechVia* via = dbTechVia::getTechVia(tech, data[idx]);

        if (opcode & WOP_VIA_EXIT_TOP)
          pnt._layer = via->getTopLayer();
        else
          pnt._layer = via->getBottomLayer();

        if ((look_for_x == false) && (look_for_y == false))
          return;

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
