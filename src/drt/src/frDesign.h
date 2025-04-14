// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "db/obj/frBlock.h"
#include "db/obj/frMaster.h"
#include "db/tech/frTechObject.h"
#include "distributed/drUpdate.h"
#include "frBaseTypes.h"
#include "frRegionQuery.h"
#include "global.h"

namespace drt {
namespace io {
class Parser;
}
class frDesign
{
 public:
  // constructors
  frDesign(Logger* logger, RouterConfiguration* router_cfg)
      : topBlock_(nullptr),
        tech_(std::make_unique<frTechObject>()),
        rq_(std::make_unique<frRegionQuery>(this, logger, router_cfg)),
        updates_sz_(0),
        version_(0),
        router_cfg_(router_cfg)
  {
  }
  frDesign() : topBlock_(nullptr), tech_(nullptr), rq_(nullptr) {}
  // getters
  frBlock* getTopBlock() const { return topBlock_.get(); }
  frTechObject* getTech() const { return tech_.get(); }
  frRegionQuery* getRegionQuery() const { return rq_.get(); }
  std::vector<std::unique_ptr<frMaster>>& getMasters() { return masters_; }
  const std::vector<std::unique_ptr<frMaster>>& getMasters() const
  {
    return masters_;
  }
  const std::vector<std::string>& getUserSelectedVias() const
  {
    return user_selected_vias_;
  }
  // setters
  void setTopBlock(std::unique_ptr<frBlock> in) { topBlock_ = std::move(in); }
  void setTech(std::unique_ptr<frTechObject> in) { tech_ = std::move(in); }
  void addMaster(std::unique_ptr<frMaster> in)
  {
    name2master_[in->getName()] = in.get();
    masters_.push_back(std::move(in));
  }
  void addUserSelectedVia(const std::string& viaName)
  {
    user_selected_vias_.push_back(viaName);
  }
  // others
  friend class io::Parser;
  bool isHorizontalLayer(frLayerNum l) const
  {
    return getTech()->isHorizontalLayer(l);
  }
  bool isVerticalLayer(frLayerNum l) const
  {
    return getTech()->isVerticalLayer(l);
  }
  std::vector<frTrackPattern*> getPrefDirTracks(frCoord layerNum) const
  {
    return getTopBlock()->getTrackPatterns(layerNum, isVerticalLayer(layerNum));
  }
  std::vector<frTrackPattern*> getNonPrefDirTracks(frCoord layerNum) const
  {
    return getTopBlock()->getTrackPatterns(layerNum,
                                           !isVerticalLayer(layerNum));
  }

  void addUpdate(const drUpdate& update)
  {
    if (updates_.empty()) {
      updates_.resize(static_cast<size_t>(router_cfg_->MAX_THREADS) * 2);
    }
    auto num_batches = updates_.size();
    updates_[updates_sz_++ % num_batches].push_back(update);
  }
  const std::vector<std::vector<drUpdate>>& getUpdates() const
  {
    return updates_;
  }
  bool hasUpdates() const { return updates_sz_ != 0; }
  void clearUpdates()
  {
    updates_.clear();
    updates_sz_ = 0;
  }
  void incrementVersion() { ++version_; }
  int getVersion() const { return version_; }

 private:
  std::unique_ptr<frBlock> topBlock_;
  std::map<frString, frMaster*> name2master_;
  std::vector<std::unique_ptr<frMaster>> masters_;
  std::unique_ptr<frTechObject> tech_;
  std::unique_ptr<frRegionQuery> rq_;
  std::vector<std::vector<drUpdate>> updates_;
  int updates_sz_;
  std::vector<std::string> user_selected_vias_;
  int version_;
  RouterConfiguration* router_cfg_;
};
}  // namespace drt
