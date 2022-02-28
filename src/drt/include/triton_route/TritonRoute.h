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

#ifndef _TRITONROUTE_H_
#define _TRITONROUTE_H_

#include <tcl.h>

#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
namespace fr {
class frDesign;
class DesignCallBack;
struct frDebugSettings;
}  // namespace fr

namespace odb {
class dbDatabase;
class dbInst;
}
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
namespace triton_route {

struct ParamStruct
{
  std::string guideFile;
  std::string outputGuideFile;
  std::string outputMazeFile;
  std::string outputDrcFile;
  std::string outputCmapFile;
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

  fr::frDesign* getDesign() const { return design_.get(); }

  int main();
  void pinAccess(std::vector<odb::dbInst*> target_insts = std::vector<odb::dbInst*>());

  int getNumDRVs() const;

  void setDebugDR(bool on = true);
  void setDebugDumpDR(bool on = true);
  void setDebugMaze(bool on = true);
  void setDebugPA(bool on = true);
  void setDebugNetName(const char* name);  // for DR
  void setDebugPinName(const char* name);  // for PA
  void setDebugWorker(int x, int y);
  void setDebugIter(int iter);
  void setDebugPaMarkers(bool on = true);
  void setDistributed(bool on = true);
  void setWorkerIpPort(const char* ip, unsigned short port);
  void setLocalIpPort(const char* ip, unsigned short port);
  void setSharedVolume(const std::string& vol);
  void setDebugPaEdge(bool on = true);
  void setDebugPaCommit(bool on = true);
  void reportConstraints();

  void readParams(const std::string& fileName);
  void setParams(const ParamStruct& params);

  // This runs a serialized worker from file_name.  It is intended
  // for debugging and not general usage.
  std::string runDRWorker(const std::string& workerStr);
  void updateGlobals(const char* file_name);
  void updateDesign(const char* file_name);
  void addWorkerResult(int idx, std::string workerStr);
  bool getWorkerResult(int& idx, std::string& workerStr);
  int getWorkerResultsSize();

 private:
  std::unique_ptr<fr::frDesign> design_;
  std::unique_ptr<fr::frDebugSettings> debug_;
  std::unique_ptr<fr::DesignCallBack> db_callback_;
  odb::dbDatabase* db_;
  utl::Logger* logger_;
  stt::SteinerTreeBuilder* stt_builder_;
  int num_drvs_;
  gui::Gui* gui_;
  dst::Distributed* dist_;
  bool distributed_;
  std::string dist_ip_;
  unsigned short dist_port_;
  std::string local_ip_;
  unsigned short local_port_;
  std::string shared_volume_;
  std::queue<std::pair<int, std::string>> workers_results_;
  std::mutex results_mutex_;
  int results_sz_;

  void initDesign();
  void initGuide();
  void prep();
  void gr();
  void ta();
  void dr();
  void endFR();
};
}  // namespace triton_route
#endif
