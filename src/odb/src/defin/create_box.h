// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/db.h"
#include "odb/odb.h"

namespace utl {
class Logger;
}

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
                uint mask,
                utl::Logger* logger);

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
                              uint bottom_mask,
                              uint cut_mask,
                              uint top_mask,
                              utl::Logger* logger);

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
                              uint bottom_mask,
                              uint cut_mask,
                              uint top_mask,
                              utl::Logger* logger);

}  // namespace odb
