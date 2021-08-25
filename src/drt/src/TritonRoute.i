/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

%{

#include "ord/OpenRoad.hh"
#include "triton_route/TritonRoute.h"
 
%}

%include "../../Exception.i"

%inline %{

int detailed_route_num_drvs()
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  return router->getNumDRVs();
}

void detailed_route_cmd(const char* guideFile,
                        const char* outputGuideFile,
                        const char* outputMazeFile,
                        const char* outputDrcFile,
                        const char* outputCmapFile,
                        const char* dbProcessNode,
                        bool enableViaGen,
                        int drouteEndIter,
                        int drouteViaInPinBottomLayerNum,
                        int drouteViaInPinTopLayerNum,
                        int orSeed,
                        double orK,
                        const char* bottomRoutingLayer,
                        const char* topRoutingLayer,
                        int verbose)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->setParams({guideFile,
                    outputGuideFile,
                    outputMazeFile,
                    outputDrcFile,
                    outputCmapFile,
                    dbProcessNode,
                    enableViaGen,
                    drouteEndIter,
                    drouteViaInPinBottomLayerNum,
                    drouteViaInPinTopLayerNum,
                    orSeed,
                    orK,
                    bottomRoutingLayer,
                    topRoutingLayer,
                    verbose});
  router->main();
}

void detailed_route_cmd(const char* param_file)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->readParams(param_file);
  router->main();
}

void report_constraints()
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->reportConstraints();
}

void
set_detailed_route_debug_cmd(const char* net_name,
                             const char* pin_name,
                             bool dr,
                             bool pa,
                             bool maze,
                             int gcell_x, int gcell_y,
                             int iter,
                             bool pa_markers,
                             bool pa_combining)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->setDebugNetName(net_name);
  router->setDebugPinName(pin_name);
  router->setDebugDR(dr);
  router->setDebugPA(pa);
  router->setDebugMaze(maze);
  if (gcell_x >= 0) {
    router->setDebugGCell(gcell_x, gcell_y);
  }
  router->setDebugIter(iter);
  router->setDebugPaMarkers(pa_markers);
  router->setDebugPaCombining(pa_combining);
}

%} // inline
