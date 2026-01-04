// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "create_box.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

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
                uint32_t mask,
                utl::Logger* logger)
{
  int x1, x2, y1, y2;
  int dw = width >> 1;

  if ((cur_x == prev_x) && (cur_y == prev_y))  // single point
  {
    logger->warn(utl::ODB,
                 274,
                 "Zero length path segment ({},{}) ({},{})",
                 prev_x,
                 prev_y,
                 cur_x,
                 cur_y);
    return;
  }
  if (cur_x == prev_x)  // vert. path
  {
    x1 = cur_x - dw;
    x2 = cur_x + dw;

    if (cur_y > prev_y) {
      if (has_prev_ext) {
        y1 = prev_y - prev_ext;
      } else {
        y1 = prev_y;
      }

      if (has_cur_ext) {
        y2 = cur_y + cur_ext;
      } else {
        y2 = cur_y;
      }
    } else {
      if (has_cur_ext) {
        y1 = cur_y - cur_ext;
      } else {
        y1 = cur_y;
      }

      if (has_prev_ext) {
        y2 = prev_y + prev_ext;
      } else {
        y2 = prev_y;
      }
    }

    odb::dbSBox* box
        = dbSBox::create(wire, layer, x1, y1, x2, y2, type, dbSBox::VERTICAL);
    box->setLayerMask(mask);
  } else if (cur_y == prev_y)  // horiz. path
  {
    y1 = cur_y - dw;
    y2 = cur_y + dw;

    if (cur_x > prev_x) {
      if (has_prev_ext) {
        x1 = prev_x - prev_ext;
      } else {
        x1 = prev_x;
      }

      if (has_cur_ext) {
        x2 = cur_x + cur_ext;
      } else {
        x2 = cur_x;
      }
    } else {
      if (has_cur_ext) {
        x1 = cur_x - cur_ext;
      } else {
        x1 = cur_x;
      }

      if (has_prev_ext) {
        x2 = prev_x + prev_ext;
      } else {
        x2 = prev_x;
      }
    }
    odb::dbSBox* box
        = dbSBox::create(wire, layer, x1, y1, x2, y2, type, dbSBox::HORIZONTAL);
    box->setLayerMask(mask);
  } else if (abs(cur_x - prev_x) == abs(cur_y - prev_y)) {  // 45-degree path
    odb::dbSBox* box = dbSBox::create(wire,
                                      layer,
                                      prev_x,
                                      prev_y,
                                      cur_x,
                                      cur_y,
                                      type,
                                      dbSBox::OCTILINEAR,
                                      width);
    box->setLayerMask(mask);
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
                              uint32_t bottom_mask,
                              uint32_t cut_mask,
                              uint32_t top_mask,
                              utl::Logger* logger)
{
  if (via->getBBox() == nullptr) {
    std::string n = via->getName();
    logger->warn(utl::ODB,
                 241,
                 "error: Cannot create a via instance, via ({}) has no shapes",
                 n.c_str());
    return nullptr;
  }

  int i, j;
  int x = orig_x;

  for (i = 0; i < numX; ++i) {
    int y = orig_y;

    for (j = 0; j < numY; ++j) {
      odb::dbSBox* box = dbSBox::create(wire, via, x, y, type);
      box->setViaLayerMask(bottom_mask, cut_mask, top_mask);
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
    return nullptr;
  }

  if (top != layer) {
    layer = top;

  } else if (bottom != layer) {
    layer = bottom;
  }

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
                              uint32_t bottom_mask,
                              uint32_t cut_mask,
                              uint32_t top_mask,
                              utl::Logger* logger)
{
  if (via->getBBox() == nullptr) {
    std::string vname = via->getName();
    logger->warn(utl::ODB,
                 244,
                 "error: Cannot create a via instance, via ({}) has no shapes",
                 vname.c_str());
    return nullptr;
  }

  int i, j;
  int x = orig_x;

  for (i = 0; i < numX; ++i) {
    int y = orig_y;

    for (j = 0; j < numY; ++j) {
      dbSBox* box = dbSBox::create(wire, via, x, y, type);
      box->setViaLayerMask(bottom_mask, cut_mask, top_mask);
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
        276,
        "       via ({}) spans above and below the current layer ({}).",
        vname.c_str(),
        lname.c_str());
    return nullptr;
  }

  if (top != layer) {
    layer = top;

  } else if (bottom != layer) {
    layer = bottom;
  }

  return layer;
}

}  // namespace odb
