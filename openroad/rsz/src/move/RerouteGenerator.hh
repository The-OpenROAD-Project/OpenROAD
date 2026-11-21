// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "odb/dbBlockCallBackObj.h"
#include "rsz/Resizer.hh"

namespace odb {
class dbNet;
}  // namespace odb

namespace sta {
class Scene;
}  // namespace sta

namespace rsz {

struct PromotedLayerEval
{
  int promoted_min_layer = 0;  // 1-based routing layer; 0 = skip
  float curr_resistance = 0.0f;
  float est_resistance = 0.0f;
  float reduction_ratio = 0.0f;
};

PromotedLayerEval evaluateReroutePromotion(
    Resizer& resizer,
    odb::dbNet* db_net,
    const std::vector<float>& layer_usage,
    const sta::Scene* corner = nullptr);

// Produces one resistance-aware reroute candidate for a path-driver net when
// promoting to underutilized upper routing layers promises sufficient
// resistance reduction.
class RerouteGenerator : public MoveGenerator,
                         private MoveCommitter::JournalObserver
{
 public:
  explicit RerouteGenerator(const GeneratorContext& context);
  ~RerouteGenerator() override;

  MoveType type() const override { return MoveType::kReroute; }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 private:
  struct PendingAttempt
  {
    odb::dbNet* net = nullptr;
    bool speculative = false;
  };

  class NetDestroyCallback : public odb::dbBlockCallBackObj
  {
   public:
    explicit NetDestroyCallback(RerouteGenerator* owner) : owner_(owner) {}
    void inDbNetDestroy(odb::dbNet* net) override
    {
      owner_->onNetDestroyed(net);
    }

   private:
    RerouteGenerator* owner_;
  };

  void recordAttempt(odb::dbNet* db_net);
  void onNetDestroyed(odb::dbNet* db_net);
  void onBeginJournal() override;
  void onCommitJournal(bool nested) override;
  void onRestoreJournal() override;

  std::unordered_set<odb::dbNet*> attempted_nets_;
  std::vector<std::vector<PendingAttempt>> pending_levels_;
  std::vector<float> layer_usage_cache_;
  int last_usage_refresh_reroutes_ = -1;
  bool usage_cache_from_initialized_ = false;
  NetDestroyCallback net_destroy_cbk_;
};

}  // namespace rsz
