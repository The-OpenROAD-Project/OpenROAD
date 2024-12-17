/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <tcl.h>

#include <boost/asio/thread_pool.hpp>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <vector>

#include "odb/geom.h"

namespace odb {
class dbDatabase;
class dbInst;
class dbBTerm;
class dbNet;
}  // namespace odb

namespace utl {
class Logger;
}

namespace gui {
class Gui;
}

namespace stt {
class SteinerTreeBuilder;
}

namespace dst {
class Distributed;
}

namespace drt {

class frDesign;
class DesignCallBack;
class FlexDR;
class FlexDRWorker;
class drUpdate;
struct frDebugSettings;
class FlexDR;
struct FlexDRViaData;
class frMarker;
struct RouterConfiguration;

struct ParamStruct
{
  std::string outputMazeFile;
  std::string outputDrcFile;
  std::optional<int> drcReportIterStep;
  std::string outputCmapFile;
  std::string outputGuideCoverageFile;
  std::string dbProcessNode;
  bool enableViaGen = false;
  int drouteEndIter = -1;
  std::string viaInPinBottomLayer;
  std::string viaInPinTopLayer;
  int orSeed = 0;
  double orK = 0;
  std::string bottomRoutingLayer;
  std::string topRoutingLayer;
  int verbose = 1;
  bool cleanPatches = false;
  bool doPa = false;
  bool singleStepDR = false;
  int minAccessPoints = -1;
  bool saveGuideUpdates = false;
  std::string repairPDNLayerName;
};

class TritonRoute
{
 public:
  TritonRoute();
  ~TritonRoute();
  void init(Tcl_Interp* tcl_interp,
            odb::dbDatabase* db,
            utl::Logger* logger,
            dst::Distributed* dist,
            stt::SteinerTreeBuilder* stt_builder);

  frDesign* getDesign() const { return design_.get(); }
  RouterConfiguration* getRouterConfiguration() const
  {
    return router_cfg_.get();
  }

  int main();
  void endFR();
  void pinAccess(const std::vector<odb::dbInst*>& target_insts
                 = std::vector<odb::dbInst*>());
  void stepDR(int size,
              int offset,
              int mazeEndIter,
              unsigned int workerDRCCost,
              unsigned int workerMarkerCost,
              unsigned int workerFixedShapeCost,
              float workerMarkerDecay,
              int ripupMode,
              bool followGuide);

  int getNumDRVs() const;

  void setDebugDR(bool on = true);
  void setDebugDumpDR(bool on, const std::string& dumpDir);
  void setDebugMaze(bool on = true);
  void setDebugPA(bool on = true);
  void setDebugTA(bool on = true);
  void setDebugWriteNetTracks(bool on = true);
  void setDebugNetName(const char* name);  // for DR
  void setDebugPinName(const char* name);  // for PA
  void setDebugBox(int x1, int y1, int x2, int y2);
  void setDebugIter(int iter);
  void setDebugPaMarkers(bool on = true);
  void setDumpLastWorker(bool on = true);
  void setDebugWorkerParams(int mazeEndIter,
                            int drcCost,
                            int markerCost,
                            int fixedShapeCost,
                            float markerDecay,
                            int ripupMode,
                            int followGuide);
  void setDistributed(bool on = true);
  void setWorkerIpPort(const char* ip, unsigned short port);
  void setSharedVolume(const std::string& vol);
  void setCloudSize(unsigned int cloud_sz) { cloud_sz_ = cloud_sz; }
  unsigned int getCloudSize() const { return cloud_sz_; }
  void setDebugPaEdge(bool on = true);
  void setDebugPaCommit(bool on = true);
  void reportConstraints();

  void setParams(const ParamStruct& params);
  void addUserSelectedVia(const std::string& viaName);
  void setUnidirectionalLayer(const std::string& layerName);
  frDebugSettings* getDebugSettings() const { return debug_.get(); }
  // This runs a serialized worker from file_name.  It is intended
  // for debugging and not general usage.
  std::string runDRWorker(const std::string& workerStr, FlexDRViaData* viaData);
  void debugSingleWorker(const std::string& dumpDir, const std::string& drcRpt);
  void updateGlobals(const char* file_name);
  void resetDb(const char* file_name);
  void clearDesign();
  void updateDesign(const std::vector<std::string>& updates);
  void updateDesign(const std::string& path);
  void addWorkerResults(
      const std::vector<std::pair<int, std::string>>& results);
  bool getWorkerResults(std::vector<std::pair<int, std::string>>& results);
  int getWorkerResultsSize();
  void sendDesignDist();
  bool writeGlobals(const std::string& name);
  void sendDesignUpdates(const std::string& router_cfg_path);
  void sendGlobalsUpdates(const std::string& router_cfg_path,
                          const std::string& serializedViaData);
  void reportDRC(const std::string& file_name,
                 const std::list<std::unique_ptr<frMarker>>& markers,
                 const std::string& marker_name,
                 odb::Rect drcBox = odb::Rect(0, 0, 0, 0)) const;
  void checkDRC(const char* filename,
                int x1,
                int y1,
                int x2,
                int y2,
                const std::string& marker_name);
  bool initGuide();
  void prep();
  odb::dbDatabase* getDb() const { return db_; }
  void fixMaxSpacing();

 private:
  std::unique_ptr<frDesign> design_;
  std::unique_ptr<frDebugSettings> debug_;
  std::unique_ptr<DesignCallBack> db_callback_;
  std::unique_ptr<RouterConfiguration> router_cfg_;
  odb::dbDatabase* db_{nullptr};
  utl::Logger* logger_{nullptr};
  std::unique_ptr<FlexDR> dr_;  // kept for single stepping
  stt::SteinerTreeBuilder* stt_builder_{nullptr};
  int num_drvs_{-1};
  gui::Gui* gui_{nullptr};
  dst::Distributed* dist_{nullptr};
  bool distributed_{false};
  std::string dist_ip_;
  uint16_t dist_port_{0};
  std::string shared_volume_;
  std::vector<std::pair<int, std::string>> workers_results_;
  std::mutex results_mutex_;
  int results_sz_{0};
  unsigned int cloud_sz_{0};
  boost::asio::thread_pool dist_pool_{1};

  void initDesign();
  void gr();
  void ta();
  void dr();
  void applyUpdates(const std::vector<std::vector<drUpdate>>& updates);
  void getDRCMarkers(std::list<std::unique_ptr<frMarker>>& markers,
                     const odb::Rect& requiredDrcBox);
  void repairPDNVias();
  friend class FlexDR;
};

}  // namespace drt
