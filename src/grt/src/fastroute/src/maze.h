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

#ifndef __MAZE_H__
#define __MAZE_H__

#include "DataType.h"

namespace grt {

// Maze-routing in different orders
extern void mazeRouteMSMD(int iter,
                          int expand,
                          float height,
                          int ripup_threshold,
                          int mazeedge_Threshold,
                          bool ordering,
                          int cost_type,
                          float LOGIS_COF,
                          int VIA,
                          int slope,
                          int L);
// Maze-routing for multi-source, multi-destination
extern void convertToMazeroute();

extern void updateCongestionHistory(int round, int upType, bool stopDEC, int &max_adj);

extern int getOverflow2D(int* maxOverflow);
extern int getOverflow2Dmaze(int* maxOverflow, int* tUsage);
extern int getOverflow3D(void);

extern void str_accu(int rnd);

extern void InitLastUsage(int upType);
extern void InitEstUsage();

}  // namespace grt
#endif /* __MAZE_H__ */
