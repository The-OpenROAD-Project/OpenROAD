/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

// Temporary until ifp/include/ifp/InitFloorplan.hh can be swig'ed

#include "ifp/InitFloorplan.hh"
#include "odb/db.h"
#include "ord/Design.h"
#include "ord/Floorplan.h"

namespace ord {

Floorplan::Floorplan(Design& design) : design_(design)
{
}

void Floorplan::initialize(const odb::Rect& die_area,
                           const odb::Rect& core_area,
                           const std::string& site_name)
{
  ifp::initFloorplan(die_area.xMin(),
                     die_area.yMin(),
                     die_area.xMax(),
                     die_area.yMax(),
                     core_area.xMin(),
                     core_area.yMin(),
                     core_area.xMax(),
                     core_area.yMax(),
                     site_name.c_str(),
                     design_.getBlock()->getDataBase(),
                     design_.getLogger());
}

void Floorplan::initialize(double utilization,
                           double aspect_ratio,
                           int core_space_bottom,
                           int core_space_top,
                           int core_space_left,
                           int core_space_right,
                           const std::string& site_name)
{
  ifp::initFloorplan(utilization,
                     aspect_ratio,
                     core_space_bottom,
                     core_space_top,
                     core_space_left,
                     core_space_right,
                     site_name.c_str(),
                     design_.getBlock()->getDataBase(),
                     design_.getLogger());
}

}  // namespace ord
