// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// Shared fixtures and helpers of the TestResizerMt test binary.  Include this
// header first: it opens the private members the tests inspect.

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

// Expose scaled-library fixture hooks without changing OpenSTA production APIs.
#include "DelayEstimator.hh"
#include "RepairSetupProfiler.hh"
#include "rsz/Resizer.hh"
#define private public
#define protected public
// RepairSetupContext pulls in RepairTargetCollector, whose scan internals
// are exercised directly below.
#include "RepairSetupContext.hh"
#undef protected
#undef private
#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#define private public
#include "SizeUpMatchGenerator.hh"
#undef private
#include "VtSwapMtCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Parasitics.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Sta.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#define protected public
#include "tst/IntegratedFixture.h"
#undef protected
#include "MoveGenerator.hh"
#include "utl/ThreadPool.h"

namespace rsz {

class TestResizerMt : public tst::IntegratedFixture
{
 protected:
  TestResizerMt()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/rsz/test/")
  {
    readLiberty(getFilePath("_main/test/Nangate45/Nangate45_lvt.lib"));
    if (!updateLib(lib_, "_main/test/Nangate45/Nangate45_lvt.lef")) {
      throw std::runtime_error("failed to load Nangate45_lvt.lef");
    }
  }

  void SetUp() override
  {
    readVerilogAndSetup("TestResizerMt_DelayEstimator.v");
    sta_->updateTiming(true);
  }

  sta::Vertex* findEndpointVertex(const char* endpoint_name) const
  {
    for (sta::Vertex* endpoint : sta_->endpoints()) {
      const sta::Pin* pin = endpoint != nullptr ? endpoint->pin() : nullptr;
      if (pin != nullptr
          && std::string(sta_->network()->pathName(pin)) == endpoint_name) {
        return endpoint;
      }
    }
    return nullptr;
  }

  Target makeTarget(const char* endpoint_name,
                    const char* inst_name,
                    const char* pin_name) const
  {
    odb::dbInst* db_inst = block_->findInst(inst_name);
    if (db_inst == nullptr) {
      ADD_FAILURE() << "missing db instance " << inst_name;
      return {};
    }

    sta::Instance* inst = db_network_->dbToSta(db_inst);
    if (inst == nullptr) {
      ADD_FAILURE() << "missing sta instance " << inst_name;
      return {};
    }

    sta::Pin* pin = db_network_->findPin(inst, pin_name);
    if (pin == nullptr) {
      ADD_FAILURE() << "missing pin " << inst_name << "/" << pin_name;
      return {};
    }

    sta::Vertex* endpoint = findEndpointVertex(endpoint_name);
    if (endpoint == nullptr) {
      ADD_FAILURE() << "missing endpoint " << endpoint_name;
      return {};
    }

    // The worst slack path reads required times, which updateTiming() leaves
    // to the caller.
    sta_->findRequireds();
    sta::Path* path = sta_->vertexWorstSlackPath(endpoint, sta::MinMax::max());
    if (path == nullptr) {
      ADD_FAILURE() << "missing worst slack path for " << endpoint_name;
      return {};
    }

    sta::PathExpanded expanded(path, sta_.get());
    int path_index = -1;
    for (int index = expanded.startIndex(); index < expanded.size(); ++index) {
      const sta::Path* path_vertex = expanded.path(index);
      if (path_vertex != nullptr && path_vertex->pin(sta_.get()) == pin) {
        path_index = index;
        break;
      }
    }
    if (path_index < 0) {
      ADD_FAILURE() << "missing target pin on path for " << inst_name << "/"
                    << pin_name;
      return {};
    }

    return makePathDriverTarget(
        path, expanded, path_index, path->slack(sta_.get()), resizer_);
  }

  GeneratorContext makeGeneratorContext(
      Resizer& resizer,
      MoveCommitter& committer,
      const OptimizerRunConfig& run_config,
      const OptimizationPolicyConfig& policy_config) const
  {
    return GeneratorContext{.resizer = resizer,
                            .committer = committer,
                            .run_config = run_config,
                            .policy_config = policy_config};
  }

  void placeInst(const char* inst_name, int x, int y) const
  {
    odb::dbInst* db_inst = block_->findInst(inst_name);
    ASSERT_NE(db_inst, nullptr);
    db_inst->setLocation(x, y);
    db_inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }
};

class StubBufferMoveCandidate : public MoveCandidate
{
 public:
  StubBufferMoveCandidate(Resizer& resizer,
                          const Target& target,
                          const int move_count,
                          const bool accepted = true)
      : MoveCandidate(resizer, target),
        move_count_(move_count),
        accepted_(accepted)
  {
  }

  MoveResult apply() override
  {
    return {.accepted = accepted_,
            .type = MoveType::kBuffer,
            .move_count = accepted_ ? move_count_ : 0,
            .touched_instances = {}};
  }

  MoveType type() const override { return MoveType::kBuffer; }

 private:
  const int move_count_;
  const bool accepted_;
};

class ProfileTestDirectory
{
 public:
  ProfileTestDirectory()
      : directory_(
            (std::filesystem::temp_directory_path() / "rsz-profile-test-XXXXXX")
                .string())
  {
    if (mkdtemp(directory_.data()) == nullptr) {
      throw std::runtime_error("cannot create profiler test directory");
    }
  }

  ~ProfileTestDirectory()
  {
    std::error_code error;
    std::filesystem::remove_all(directory_, error);
    EXPECT_FALSE(error) << error.message();
  }

  std::filesystem::path root() const
  {
    return std::filesystem::path(directory_) / "profile";
  }

 private:
  std::string directory_;
};

inline std::string readProfileFile(const std::filesystem::path& path)
{
  std::ifstream input(path);
  EXPECT_TRUE(input.is_open()) << path;
  std::ostringstream contents;
  contents << input.rdbuf();

  return contents.str();
}

}  // namespace rsz
