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

#include "create_box.h"

#include <stdio.h>

#include "utility/Logger.h"
namespace odb {

void create_box(dbSWire* wire,
                dbWireShapeType type,
                dbTechLayer* layer,
                int prev_x,
                int prev_y,
                int prev_ext,
                bool has_prev_ext,
                int cur_x,
                int cur_y,
                int cur_ext,
                bool has_cur_ext,
                int width,
                utl::Logger* logger)
{
  int x1, x2, y1, y2;
  int dw = width >> 1;

  if ((cur_x == prev_x) && (cur_y == prev_y))  // single point
  {
    printf("(%d,%d) (%d,%d)\n", prev_x, prev_y, cur_x, cur_y);
    return;
    assert(0 && "ambiguous path segment");
  } else if (cur_x == prev_x)  // vert. path
  {
    x1 = cur_x - dw;
    x2 = cur_x + dw;

    if (cur_y > prev_y) {
      if (has_prev_ext)
        y1 = prev_y - prev_ext;
      else
        y1 = prev_y;

      if (has_cur_ext)
        y2 = cur_y + cur_ext;
      else
        y2 = cur_y;
    } else {
      if (has_cur_ext)
        y1 = cur_y - cur_ext;
      else
        y1 = cur_y;

      if (has_prev_ext)
        y2 = prev_y + prev_ext;
      else
        y2 = prev_y;
    }

    dbSBox::create(wire, layer, x1, y1, x2, y2, type, dbSBox::VERTICAL);
  } else if (cur_y == prev_y)  // horiz. path
  {
    y1 = cur_y - dw;
    y2 = cur_y + dw;

    if (cur_x > prev_x) {
      if (has_prev_ext)
        x1 = prev_x - prev_ext;
      else
        x1 = prev_x;

      if (has_cur_ext)
        x2 = cur_x + cur_ext;
      else
        x2 = cur_x;
    } else {
      if (has_cur_ext)
        x1 = cur_x - cur_ext;
      else
        x1 = cur_x;

      if (has_prev_ext)
        x2 = prev_x + prev_ext;
      else
        x2 = prev_x;
    }
    dbSBox::create(wire, layer, x1, y1, x2, y2, type, dbSBox::HORIZONTAL);
  } else if (abs(cur_x - prev_x) == abs(cur_y - prev_y)) {  // 45-degree path
    dbSBox::create(wire,
                   layer,
                   prev_x,
                   prev_y,
                   cur_x,
                   cur_y,
                   type,
                   dbSBox::OCTILINEAR,
                   width);
  } else {
    assert(
        0
        && "not orthogonal nor 45-degree path segment");  // illegal:
                                                          // non-orthogonal-path
  }
}

dbTechLayer* create_via_array(dbSWire* wire,
                              dbWireShapeType type,
                              dbTechLayer* layer,
                              dbTechVia* via,
                              int orig_x,
                              int orig_y,
                              int numX,
                              int numY,
                              int stepX,
                              int stepY,
                              utl::Logger* logger)
{
  if (via->getBBox() == NULL) {
    std::string n = via->getName();
    logger->warn(utl::ODB,
                 241,
                 "error: Cannot create a via instance, via ({}) has no shapes",
                 n.c_str());
    return NULL;
  }

  int i, j;
  int x = orig_x;

  for (i = 0; i < numX; ++i) {
    int y = orig_y;

    for (j = 0; j < numY; ++j) {
      dbSBox::create(wire, via, x, y, type);
      y += stepY;
    }

    x += stepX;
  }

  dbTechLayer* top = via->getTopLayer();
  dbTechLayer* bottom = via->getBottomLayer();

  // VIA: implicit layer change...
  if ((top != layer) && (bottom != layer)) {
    std::string vname = via->getName();
    std::string lname = layer->getName();

    logger->warn(utl::ODB,
                 242,
                 "error: Can not determine which direction to continue path,");
    logger->info(
        utl::ODB,
        243,
        "       via ({}) spans above and below the current layer ({}).",
        vname.c_str(),
        lname.c_str());
    return NULL;
  }

  if (top != layer)
    layer = top;

  else if (bottom != layer)
    layer = bottom;

  return layer;
}

dbTechLayer* create_via_array(dbSWire* wire,
                              dbWireShapeType type,
                              dbTechLayer* layer,
                              dbVia* via,
                              int orig_x,
                              int orig_y,
                              int numX,
                              int numY,
                              int stepX,
                              int stepY,
                              utl::Logger* logger)
{
  if (via->getBBox() == NULL) {
    std::string vname = via->getName();
    logger->warn(utl::ODB,
                 244,
                 "error: Cannot create a via instance, via ({}) has no shapes",
                 vname.c_str());
    return NULL;
  }

  int i, j;
  int x = orig_x;

  for (i = 0; i < numX; ++i) {
    int y = orig_y;

    for (j = 0; j < numY; ++j) {
      dbSBox::create(wire, via, x, y, type);
      y += stepY;
    }

    x += stepX;
  }

  dbTechLayer* top = via->getTopLayer();
  dbTechLayer* bottom = via->getBottomLayer();

  // VIA: implicit layer change...
  if ((top != layer) && (bottom != layer)) {
    std::string vname = via->getName();
    std::string lname = layer->getName();

    logger->warn(
        utl::ODB,
        245,
        "error: Net {}: Can not determine which direction to continue path,",
        wire->getNet()->getConstName());
    logger->info(
        utl::ODB,
        246,
        "       via ({}) spans above and below the current layer ({}).",
        vname.c_str(),
        lname.c_str());
    return NULL;
  }

  if (top != layer)
    layer = top;

  else if (bottom != layer)
    layer = bottom;

  return layer;
}

}  // namespace odb
