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

#include <cstring>
#include "ord/OpenRoad.hh"
#include "triton_route/TritonRoute.h"
#include "utl/Logger.h"
%}

%include "../../Exception.i"

%inline %{

int detailed_route_num_drvs()
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  return router->getNumDRVs();
}

void detailed_route_distributed(const char* remote_ip,
                                unsigned short remote_port,
                                const char* sharedVolume,
                                unsigned int cloud_sz)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->setDistributed(true);
  router->setWorkerIpPort(remote_ip, remote_port);
  router->setSharedVolume(sharedVolume);
  router->setCloudSize(cloud_sz);
}

void detailed_route_set_default_via(const char* viaName)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->addUserSelectedVia(viaName);
}

void detailed_route_set_unidirectional_layer(const char* layerName)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->setUnidirectionalLayer(layerName);
}

void detailed_route_cmd(const char* outputMazeFile,
                        const char* outputDrcFile,
                        const char* outputCmapFile,
                        const char* outputGuideCoverageFile,
                        const char* dbProcessNode,
                        bool enableViaGen,
                        int drouteEndIter,
                        const char* viaInPinBottomLayer,
                        const char* viaInPinTopLayer,
                        int orSeed,
                        double orK,
                        const char* bottomRoutingLayer,
                        const char* topRoutingLayer,
                        int verbose,
                        bool cleanPatches,
                        bool noPa,
                        bool singleStepDR,
                        int minAccessPoints,
                        bool saveGuideUpdates,
                        const char* repairPDNLayerName,
                        int drcReportIterStep)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  std::optional<int> drcReportIterStepOpt;
  if (drcReportIterStep > 0) {
    drcReportIterStepOpt = drcReportIterStep;
  }
  router->setParams({outputMazeFile,
                    outputDrcFile,
                    drcReportIterStepOpt,
                    outputCmapFile,
                    outputGuideCoverageFile,
                    dbProcessNode,
                    enableViaGen,
                    drouteEndIter,
                    viaInPinBottomLayer,
                    viaInPinTopLayer,
                    orSeed,
                    orK,
                    bottomRoutingLayer,
                    topRoutingLayer,
                    verbose,
                    cleanPatches,
                    !noPa,
                    singleStepDR,
                    minAccessPoints,
                    saveGuideUpdates,
                    repairPDNLayerName});
  router->main();
  router->setDistributed(false);
}

void pin_access_cmd(const char* dbProcessNode,
                    const char* bottomRoutingLayer,
                    const char* topRoutingLayer,
                    int verbose,
                    int minAccessPoints)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  drt::ParamStruct params;
  params.dbProcessNode = dbProcessNode;
  params.bottomRoutingLayer = bottomRoutingLayer;
  params.topRoutingLayer = topRoutingLayer;
  params.verbose = verbose;
  params.minAccessPoints = minAccessPoints;
  router->setParams(params);
  router->pinAccess();
  router->setDistributed(false);
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
                             bool dump_dr,
                             bool pa,
                             bool maze,
                             int x1, int y1, int x2, int y2,
                             int iter,
                             bool pa_markers,
                             bool pa_edge,
                             bool pa_commit,
                             const char* dumpDir,
                             bool ta,
                             bool write_net_tracks,
                             bool dump_last_worker)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->setDebugNetName(net_name);
  router->setDebugPinName(pin_name);
  router->setDebugDR(dr);
  router->setDebugDumpDR(dump_dr, dumpDir);
  router->setDebugPA(pa);
  router->setDebugMaze(maze);
  router->setDebugBox(x1, y1, x2, y2);
  router->setDebugIter(iter);
  router->setDebugPaMarkers(pa_markers);
  router->setDebugPaEdge(pa_edge);
  router->setDebugPaCommit(pa_commit);
  router->setDebugTA(ta);
  router->setDebugWriteNetTracks(write_net_tracks);
  router->setDumpLastWorker(dump_last_worker);
}

void
set_worker_debug_params(int maze_end_iter,
                        int drc_cost,
                        int marker_cost,
                        int fixed_shape_cost,
                        int marker_decay,
                        int ripup_mode,
                        int follow_guide)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->setDebugWorkerParams(maze_end_iter, drc_cost, marker_cost, fixed_shape_cost,
                               marker_decay, ripup_mode, follow_guide);
}

void
run_worker_cmd(const char* dump_dir, const char* worker_dir, const char* drc_rpt)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->updateGlobals(fmt::format("{}/init_router_cfg.bin", dump_dir).c_str());
  router->resetDb(fmt::format("{}/design.odb", dump_dir).c_str());
  router->updateDesign(fmt::format("{}/{}/updates.bin", dump_dir, worker_dir).c_str());
  router->updateGlobals(fmt::format("{}/{}/worker_router_cfg.bin", dump_dir, worker_dir).c_str());
  
  router->debugSingleWorker(fmt::format("{}/{}", dump_dir, worker_dir), drc_rpt);
}

void detailed_route_step_drt(int size,
                             int offset,
                             int mazeEndIter,
                             int workerDRCCost,
                             int workerMarkerCost,
                             int workerFixedShapeCost,
                             float workerMarkerDecay,
                             int ripupMode,
                             bool followGuide)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->stepDR(size, offset, mazeEndIter, workerDRCCost,
                 workerMarkerCost, workerFixedShapeCost,
                 workerMarkerDecay, ripupMode, followGuide);
}
void fix_max_spacing_cmd()
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->fixMaxSpacing();
}
void step_end()
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->endFR();
}

void check_drc_cmd(const char* drc_file, int x1, int y1, int x2, int y2, const char* marker_name)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->checkDRC(drc_file, x1, y1, x2, y2, marker_name);
}
%} // inline
