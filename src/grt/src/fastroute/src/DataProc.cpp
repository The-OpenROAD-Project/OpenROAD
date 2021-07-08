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

#include "DataProc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DataType.h"
#include "flute.h"
#include "utl/Logger.h"

namespace utl {
class Logger;
}

namespace grt {

// Global variables
int XRANGE, YRANGE;
int xGrid, yGrid;
float vCapacity_lb, hCapacity_lb, vCapacity_ub, hCapacity_ub;
int xcorner, ycorner, wTile, hTile;
int enlarge, costheight, ripup_threshold, ahTH;
int numValidNets;  // # nets need to be routed (having pins in different grids)
int numLayers;
int totalOverflow;  // total # overflow
FrNet** nets;
std::vector<Edge> h_edges;
std::vector<Edge> v_edges;

multi_array<float, 2> d1;
multi_array<float, 2> d2;
int verbose;

multi_array<bool, 2> HV;
multi_array<bool, 2> hyperV;
multi_array<bool, 2> hyperH;
multi_array<int, 2> corrEdge;

std::vector<Segment> seglist;
std::vector<int> seglistIndex;  // the index for the segments for each net
std::vector<int> seglistCnt;    // the number of segements for each net
StTree* sttrees;    // the Steiner trees
std::vector<std::vector<DTYPE>> gxs;        // the copy of xs for nets, used for second FLUTE
std::vector<std::vector<DTYPE>> gys;        // the copy of xs for nets, used for second FLUTE
std::vector<std::vector<DTYPE>> gs;  // the copy of vertical sequence for nets, used for second FLUTE
std::vector<Edge3D> h_edges3D;
std::vector<Edge3D> v_edges3D;

std::vector<OrderNetPin> treeOrderPV;
std::vector<OrderTree> treeOrderCong;
int viacost;

multi_array<int, 2> layerGrid;
multi_array<int, 2> viaLink;

multi_array<int, 3> d13D;
multi_array<short, 3> d23D;

multi_array<dirctionT, 3> directions3D;
multi_array<int, 3> corrEdge3D;
multi_array<parent3D, 3> pr3D;

int mazeedge_Threshold;
multi_array<bool, 2> inRegion;

int gridHV, gridH, gridV;
std::vector<int> gridHs;
std::vector<int> gridVs;

int** heap13D;
short** heap23D;

std::vector<float> h_costTable;
std::vector<float> v_costTable;
std::vector<OrderNetEdge> netEO;
std::vector<int> xcor, ycor, dcor;

StTree* sttreesBK;

multi_array<short, 2> parentX1, parentY1, parentX3, parentY3;

float **heap2, **heap1;

std::vector<bool> pop_heap2;

utl::Logger* logger;
}  // namespace grt
