////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018, Iowa State University All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
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
////////////////////////////////////////////////////////////////////////////////

#ifndef __ROUTE_H__
#define __ROUTE_H__

#include "DataType.h"

namespace grt {

#define SAMEX 0
#define SAMEY 1

extern float* costHVH;  // Horizontal first Z
extern float* costVHV;  // Vertical first Z
extern float* costH;    // Horizontal segment cost
extern float* costV;    // Vertical segment cost
extern float* costLR;   // Left and right boundary cost
extern float* costTB;   // Top and bottom boundary cost

extern float* costHVHtest;  // Vertical first Z
extern float* costVtest;    // Vertical segment cost
extern float* costTBtest;   // Top and bottom boundary cost

// old functions for segment list data structure
extern void routeSegL(Segment* seg);
extern void routeLAll(bool firstTime);
// new functions for tree data structure
extern void newrouteL(int netID, RouteType ripuptype, bool viaGuided);
extern void newrouteZ(int netID, int threshold);
extern void newrouteZ_edge(int netID, int edgeID);
extern void newrouteLAll(bool firstTime, bool viaGuided);
extern void newrouteZAll(int threshold);
extern void routeMonotonicAll(int threshold);
extern void routeMonotonic(int netID, int edgeID, int threshold);
extern void routeLVAll(int threshold, int expand, float LOGIS_COF);
extern void spiralRouteAll();
extern void newrouteLInMaze(int netID);
}  // namespace grt
#endif /* __ROUTE_H__ */
