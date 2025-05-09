// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

%ignore drt::TritonRoute::init;

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
  const int num_threads = ord::OpenRoad::openRoad()->getThreadCount();
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
                    repairPDNLayerName,
                    num_threads});
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
  params.num_threads = ord::OpenRoad::openRoad()->getThreadCount();
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
  const int num_threads = ord::OpenRoad::openRoad()->getThreadCount();
  router->updateDesign(fmt::format("{}/{}/updates.bin", dump_dir, worker_dir).c_str(), num_threads);
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
  const int num_threads = ord::OpenRoad::openRoad()->getThreadCount();
  router->fixMaxSpacing(num_threads);
}
void step_end()
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  router->endFR();
}

void check_drc_cmd(const char* drc_file, int x1, int y1, int x2, int y2, const char* marker_name)
{
  auto* router = ord::OpenRoad::openRoad()->getTritonRoute();
  const int num_threads = ord::OpenRoad::openRoad()->getThreadCount();
  router->checkDRC(drc_file, x1, y1, x2, y2, marker_name, num_threads);
}
%} // inline
