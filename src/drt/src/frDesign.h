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

#include <memory>
#include <string>

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
