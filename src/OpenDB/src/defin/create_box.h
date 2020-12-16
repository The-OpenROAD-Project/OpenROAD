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

#include "odb.h"
#include "db.h"

namespace odb {

void create_box(dbSWire*        wire,
                dbWireShapeType type,
                dbTechLayer*    layer,
                int             prev_x,
                int             prev_y,
                int             prev_ext,
                bool            has_prev_ext,
                int             cur_x,
                int             cur_y,
                int             cur_ext,
                bool            has_cur_ext,
                int             width);

dbTechLayer* create_via_array(dbSWire*        wire,
                              dbWireShapeType type,
                              dbTechLayer*    layer,
                              dbTechVia*      via,
                              int             orig_x,
                              int             orig_y,
                              int             numX,
                              int             numY,
                              int             stepX,
                              int             stepY);

dbTechLayer* create_via_array(dbSWire*        wire,
                              dbWireShapeType type,
                              dbTechLayer*    layer,
                              dbVia*          via,
                              int             orig_x,
                              int             orig_y,
                              int             numX,
                              int             numY,
                              int             stepX,
                              int             stepY);

}  // namespace odb
