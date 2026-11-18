// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "odb/db.h"
#include "odb/dbTypes.h"

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
                uint32_t mask,
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
                              uint32_t bottom_mask,
                              uint32_t cut_mask,
                              uint32_t top_mask,
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
                              uint32_t bottom_mask,
                              uint32_t cut_mask,
                              uint32_t top_mask,
                              utl::Logger* logger);

}  // namespace odb
