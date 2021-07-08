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

#ifndef __DATAPROC_H__
#define __DATAPROC_H__

#include "DataType.h"
#include "boost/multi_array.hpp"
#include <vector>

#define BUFFERSIZE 800
#define STRINGLEN 100
#define MAXEDGES 10000000

#define MAXLEN 20000

namespace utl {
class Logger;
}

namespace grt {

using boost::multi_array;

// global variables
extern int XRANGE, YRANGE;
extern int xGrid, yGrid, vCapacity, hCapacity;
extern float vCapacity_lb, hCapacity_lb, vCapacity_ub, hCapacity_ub;
extern int verbose;

extern int enlarge, costheight, ripup_threshold;
extern int xcorner, ycorner, wTile, hTile, ahTH;

extern int
    numValidNets;  // # nets need to be routed (having pins in different grids)
extern int numLayers;
extern int totalOverflow;  // total # overflow
extern FrNet** nets;
extern Edge *h_edges, *v_edges;

extern multi_array<float, 2> d1;
extern multi_array<float, 2> d2;

extern bool** HV;
extern bool** hyperV;
extern bool** hyperH;
extern int** corrEdge;

extern Segment* seglist;
extern int* seglistIndex;  // the index for the segments for each net
extern int* seglistCnt;    // the number of segements for each net

extern StTree* sttrees;  // the Steiner trees
extern std::vector<std::vector<DTYPE>> gxs;        // the copy of xs for nets, used for second FLUTE
extern std::vector<std::vector<DTYPE>> gys;        // the copy of xs for nets, used for second FLUTE
extern std::vector<std::vector<DTYPE>> gs;  // the copy of vertical sequence for nets, used for second FLUTE

extern std::vector<OrderNetPin> treeOrderPV;
extern std::vector<OrderTree> treeOrderCong;
extern int viacost;

extern Edge3D* h_edges3D;
extern Edge3D* v_edges3D;

extern int** layerGrid;
extern int** viaLink;

extern multi_array<int, 3> d13D;
extern multi_array<short, 3> d23D;

extern dirctionT*** directions3D;
extern int*** corrEdge3D;
extern parent3D*** pr3D;

extern int mazeedge_Threshold;
extern bool** inRegion;

extern int gridHV, gridH, gridV;
extern int *gridHs, *gridVs;

extern int** heap13D;
extern short** heap23D;

extern float *h_costTable, *v_costTable;

extern std::vector<OrderNetEdge> netEO;

extern int *xcor, *ycor, *dcor;

extern StTree* sttreesBK;

extern multi_array<short, 2> parentX1, parentY1, parentX3, parentY3;

extern float **heap2, **heap1;
extern bool* pop_heap2;

extern utl::Logger* logger;

template <class T>
T ADIFF(T x, T y)
{
  if (x > y) {
    return (x - y);
  } else {
    return (y - x);
  }
}
}  // namespace grt

#endif /* __DATAPROC_H__ */
