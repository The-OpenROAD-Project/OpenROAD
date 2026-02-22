// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "boost/asio/post.hpp"
#include "boost/asio/thread_pool.hpp"
#include "boost/bind/bind.hpp"
#include "db/infra/frTime.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/RoutingJobDescription.h"
#include "distributed/frArchive.h"
#include "distributed/paUpdate.h"
#include "dr/FlexDR.h"
#include "drt/TritonRoute.h"
#include "dst/Distributed.h"
#include "dst/JobCallBack.h"
#include "dst/JobMessage.h"
#include "global.h"
#include "omp.h"
#include "pa/FlexPA.h"
#include "utl/Logger.h"

namespace asio = boost::asio;
namespace odb {
class dbDatabase;
}
namespace drt {

class RoutingCallBack : public dst::JobCallBack
{
 public:
  RoutingCallBack(TritonRoute* router,
                  dst::Distributed* dist,
                  utl::Logger* logger)
      : router_(router),
        dist_(dist),
        logger_(logger),
        pa_(router->getDesign(),
            logger,
            nullptr,
            router->getRouterConfiguration())
  {
  }
  void onRoutingJobReceived(dst::JobMessage& msg, dst::Socket& sock) override
  {
    if (msg.getJobType() != dst::JobMessage::kRouting) {
      return;
    }
    RoutingJobDescription* desc
        = static_cast<RoutingJobDescription*>(msg.getJobDescription());
    if (init_) {
      init_ = false;
      omp_set_num_threads(router_->getRouterConfiguration()->MAX_THREADS);
    }
    auto workers = desc->getWorkers();
    int size = workers.size();
    std::vector<std::pair<int, std::string>> results;
    asio::thread_pool reply_pool(1);
    int prev_perc = 0;
    int cnt = 0;
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < workers.size(); i++) {  // NOLINT
      std::pair<int, std::string> result
          = {workers.at(i).first,
             router_->runDRWorker(workers.at(i).second, &via_data_)};
#pragma omp critical
      {
        results.push_back(result);
        ++cnt;
        if (cnt * 1.0 / size >= prev_perc / 100.0 + 0.1 && prev_perc < 90) {
          prev_perc += 10;
          if (prev_perc % desc->getSendEvery() == 0) {
            asio::post(reply_pool,
                       boost::bind(&RoutingCallBack::sendResult,
                                   this,
                                   results,
                                   boost::ref(sock),
                                   false,
                                   cnt));
            results.clear();
          }
        }
      }
    }
    reply_pool.join();
    sendResult(results, sock, true, cnt);
  }

  void onFrDesignUpdated(dst::JobMessage& msg, dst::Socket& sock) override
  {
    if (msg.getJobType() != dst::JobMessage::kUpdateDesign) {
      return;
    }
    dst::JobMessage result(dst::JobMessage::kUpdateDesign);
    RoutingJobDescription* desc
        = static_cast<RoutingJobDescription*>(msg.getJobDescription());
    if (!desc->getGlobalsPath().empty()) {
      if (router_cfg_path_ != desc->getGlobalsPath()) {
        router_cfg_path_ = desc->getGlobalsPath();
        router_->setSharedVolume(desc->getSharedDir());
        router_->updateGlobals(desc->getGlobalsPath().c_str());
      }
    }
    if ((desc->isDesignUpdate() && !desc->getUpdates().empty())
        || !desc->getDesignPath().empty()) {
      frTime t;
      logger_->report("Design Update");
      if (desc->isDesignUpdate()) {
        router_->updateDesign(desc->getUpdates(),
                              router_->getRouterConfiguration()->MAX_THREADS);
      } else {
        router_->resetDb(desc->getDesignPath().c_str());
      }
      t.print(logger_);
    }
    if (!desc->getViaData().empty()) {
      std::stringstream stream(
          desc->getViaData(),
          std::ios_base::binary | std::ios_base::in | std::ios_base::out);
      frIArchive ar(stream);
      ar >> via_data_;
    }
    dist_->sendResult(result, sock);
    sock.close();
  }

  void onPinAccessJobReceived(dst::JobMessage& msg, dst::Socket& sock) override
  {
    if (msg.getJobType() != dst::JobMessage::kPinAccess) {
      return;
    }
    PinAccessJobDescription* desc
        = static_cast<PinAccessJobDescription*>(msg.getJobDescription());
    logger_->report("Received PA Job");
    dst::JobMessage result(dst::JobMessage::kSuccess);
    switch (desc->getType()) {
      case PinAccessJobDescription::UPDATE_PA: {
        paUpdate update;
        paUpdate::deserialize(router_->getDesign(), update, desc->getPath());
        for (auto& [pin, pa_vec] : update.getPinAccess()) {
          for (auto& pa : pa_vec) {
            int idx = pa->getId();
            pin->setPinAccess(idx, std::move(pa));
          }
        }
        for (const auto& [term, aps] : update.getGroupResults()) {
          term->setAccessPoints(aps);
        }
        break;
      }
      case PinAccessJobDescription::UPDATE_PATTERNS:
        pa_.applyPatternsFile(desc->getPath().c_str());
        break;
      case PinAccessJobDescription::INIT_PA:
        pa_.setDesign(router_->getDesign());
        pa_.init();
        break;
      case PinAccessJobDescription::INST_ROWS: {
        auto instRows = deserializeInstRows(desc->getPath());
        omp_set_num_threads(router_->getRouterConfiguration()->MAX_THREADS);
#pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < instRows.size(); i++) {  // NOLINT
          pa_.genInstRowPattern(instRows.at(i));
        }
        paUpdate update;
        for (const auto& row : instRows) {
          for (auto inst : row) {
            for (const auto& iterm : inst->getInstTerms()) {
              update.addGroupResult({iterm.get(), iterm->getAccessPoints()});
            }
          }
        }
        auto uResultDesc = std::make_unique<PinAccessJobDescription>();
        uResultDesc->setPath(fmt::format("{}.res", desc->getPath()));
        paUpdate::serialize(update, uResultDesc->getPath());
        result.setJobDescription(std::move(uResultDesc));
        break;
      }
    }

    dist_->sendResult(result, sock);
    sock.close();
  }
  void onGRDRInitJobReceived(dst::JobMessage& msg, dst::Socket& sock) override
  {
    if (msg.getJobType() != dst::JobMessage::kGrdrInit) {
      return;
    }
    router_->initGuide();
    router_->prep();
    router_->getDesign()->getRegionQuery()->initDRObj();
    dst::JobMessage result(dst::JobMessage::kSuccess);
    dist_->sendResult(result, sock);
    sock.close();
  }

 private:
  void sendResult(const std::vector<std::pair<int, std::string>>& results,
                  dst::Socket& sock,
                  bool finish,
                  int cnt)
  {
    dst::JobMessage result;
    if (finish) {
      result.setJobType(dst::JobMessage::kSuccess);
    } else {
      result.setJobType(dst::JobMessage::kNone);
    }
    auto uResultDesc = std::make_unique<RoutingJobDescription>();
    auto resultDesc = static_cast<RoutingJobDescription*>(uResultDesc.get());
    resultDesc->setWorkers(results);
    result.setJobDescription(std::move(uResultDesc));
    dist_->sendResult(result, sock);
    if (finish) {
      sock.close();
    }
  }

  std::vector<std::vector<frInst*>> deserializeInstRows(
      const std::string& file_path)
  {
    std::vector<std::vector<frInst*>> instRows;
    paUpdate update;
    paUpdate::deserialize(router_->getDesign(), update, file_path);
    instRows = update.getInstRows();
    return instRows;
  }

  TritonRoute* router_;
  dst::Distributed* dist_;
  utl::Logger* logger_;
  std::string design_path_;
  std::string router_cfg_path_;
  bool init_{true};
  FlexDRViaData via_data_;
  FlexPA pa_;
};

}  // namespace drt
