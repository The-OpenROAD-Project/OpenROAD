// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "boost/asio/thread_pool.hpp"
#include "odb/geom.h"

namespace odb {
class dbDatabase;
class dbInst;
class dbBTerm;
class dbNet;
class dbWire;
}  // namespace odb

namespace utl {
class Logger;
class CallBackHandler;
}  // namespace utl

namespace stt {
class SteinerTreeBuilder;
}

namespace dst {
class Distributed;
}

namespace drt {

class frDesign;
class frInst;
class DesignCallBack;
class PACallBack;
class FlexDR;
class FlexPA;
class FlexTA;
class FlexDRWorker;
class drUpdate;
struct frDebugSettings;
struct FlexDRViaData;
class frMarker;
struct RouterConfiguration;
class AbstractGraphicsFactory;
class frViaDef;

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
  std::string viaAccessLayer;
  int orSeed = 0;
  double orK = 0;
  int verbose = 1;
  bool cleanPatches = false;
  bool doPa = false;
  bool singleStepDR = false;
  int minAccessPoints = -1;
  bool saveGuideUpdates = false;
  std::string repairPDNLayerName;
  int num_threads;
};

class TritonRoute
{
 public:
  TritonRoute(odb::dbDatabase* db,
              utl::Logger* logger,
              utl::CallBackHandler* callback_handler,
              dst::Distributed* dist,
              stt::SteinerTreeBuilder* stt_builder);
  ~TritonRoute();

  void initGraphics(std::unique_ptr<AbstractGraphicsFactory> graphics_factory);

  frDesign* getDesign() const { return design_.get(); }
  utl::Logger* getLogger() const { return logger_; }
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
  void setDebugSnapshotDir(const std::string& snapshotDir);
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
  void updateDesign(const std::vector<std::string>& updates, int num_threads);
  void updateDesign(const std::string& path, int num_threads);
  void addWorkerResults(
      const std::vector<std::pair<int, std::string>>& results);
  bool getWorkerResults(std::vector<std::pair<int, std::string>>& results);
  int getWorkerResultsSize();
  void sendDesignDist();
  bool writeGlobals(const std::string& name);
  void sendDesignUpdates(const std::string& router_cfg_path, int num_threads);
  void sendGlobalsUpdates(const std::string& router_cfg_path,
                          const std::string& serializedViaData);
  void reportDRC(const std::string& file_name,
                 const std::list<std::unique_ptr<frMarker>>& markers,
                 const std::string& marker_name,
                 odb::Rect drcBox = odb::Rect(0, 0, 0, 0)) const;
  std::vector<int> routeLayerLengths(odb::dbWire* wire) const;
  void checkDRC(const char* filename,
                int x1,
                int y1,
                int x2,
                int y2,
                const std::string& marker_name,
                int num_threads);
  bool initGuide();
  void prep();
  odb::dbDatabase* getDb() const { return db_; }
  void fixMaxSpacing(int num_threads);
  void deleteInstancePAData(frInst* inst, bool delete_inst = false);
  void addInstancePAData(frInst* inst);
  void addAvoidViaDefPA(const frViaDef* via_def);
  void updateDirtyPAData();

 private:
  std::unique_ptr<frDesign> design_;
  std::unique_ptr<frDebugSettings> debug_;
  std::unique_ptr<DesignCallBack> db_callback_;
  std::unique_ptr<PACallBack> pa_callback_;
  std::unique_ptr<RouterConfiguration> router_cfg_;
  odb::dbDatabase* db_{nullptr};
  utl::Logger* logger_{nullptr};
  std::unique_ptr<FlexDR> dr_;  // kept for single stepping
  stt::SteinerTreeBuilder* stt_builder_{nullptr};
  int num_drvs_{-1};
  dst::Distributed* dist_{nullptr};
  bool distributed_{false};
  std::string dist_ip_;
  uint16_t dist_port_{0};
  std::string shared_volume_;
  std::vector<std::pair<int, std::string>> workers_results_;
  absl::Mutex results_mutex_;
  int results_sz_{0};
  unsigned int cloud_sz_{0};
  std::optional<boost::asio::thread_pool> dist_pool_;
  std::unique_ptr<FlexPA> pa_{nullptr};
  std::unique_ptr<AbstractGraphicsFactory> graphics_factory_{nullptr};

  void initDesign();
  void initGraphics();
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
