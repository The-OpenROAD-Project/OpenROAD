// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// RepairTargetCollector endpoint scans and chunked target collection.

#include "TestResizerMtFixture.hh"

namespace rsz {

TEST_F(TestResizerMt, CollectorEndpointScanMatchesStaSlack)
{
  resizer_.runRepairSetupPreamble();
  RepairTargetCollector collector(&resizer_);

  // A one-second margin makes every constrained endpoint a violator.
  collector.init(1.0f);
  const sta::MinMax* max = resizer_.maxAnalysisMode();
  size_t violating = 0;
  for (sta::Vertex* endpoint : sta_->endpoints()) {
    const sta::Slack expected = sta_->slack(endpoint, max);
    EXPECT_FLOAT_EQ(ResizerTestPeer::endpointPathSlack(collector, endpoint),
                    expected);
    if (sta::fuzzyLess(expected, 1.0f)) {
      ++violating;
    }
  }

  const std::vector<std::pair<const sta::Pin*, sta::Slack>>& violators
      = collector.getViolatingEndpoints();
  EXPECT_GT(violators.size(), 0u);
  EXPECT_EQ(violators.size(), violating);
  for (size_t index = 1; index < violators.size(); ++index) {
    EXPECT_LE(violators[index - 1].second, violators[index].second);
  }
}

TEST_F(TestResizerMt, CollectorParallelScanMatchesSerialScan)
{
  resizer_.runRepairSetupPreamble();
  RepairTargetCollector serial(&resizer_);
  serial.init(1.0f);
  ASSERT_EQ(ResizerTestPeer::scanThreadPool(serial), nullptr);

  // Force the worker path on the small test design.
  utl::ThreadPool scan_pool(3);
  RepairTargetCollector parallel(&resizer_);
  parallel.setScanThreadPool(&scan_pool, 0);
  parallel.init(1.0f);

  ASSERT_NE(ResizerTestPeer::scanThreadPool(parallel), nullptr);
  ASSERT_EQ(parallel.getViolatingEndpoints().size(),
            serial.getViolatingEndpoints().size());
  for (size_t index = 0; index < serial.getViolatingEndpoints().size();
       ++index) {
    EXPECT_EQ(parallel.getViolatingEndpoints()[index].first,
              serial.getViolatingEndpoints()[index].first);
    EXPECT_FLOAT_EQ(parallel.getViolatingEndpoints()[index].second,
                    serial.getViolatingEndpoints()[index].second);
  }
}

TEST_F(TestResizerMt, CollectorChunkedCritPathTargetsMatchFullCollection)
{
  resizer_.runRepairSetupPreamble();
  RepairTargetCollector collector(&resizer_);
  collector.init(1.0f);
  const std::function<bool(sta::Pin*)> valid = [this](sta::Pin* pin) {
    return resizer_.isEditableLogicStdCell(resizer_.network()->instance(pin));
  };
  const size_t endpoint_count = collector.getViolatingEndpoints().size();
  std::unordered_set<sta::Pin*> full_visited;
  const std::vector<Target> full = collector.collectCritPathDriverPinTargets(
      valid, 0, endpoint_count, full_visited);
  ASSERT_FALSE(full.empty());

  // Single-endpoint chunks with one shared visited set reproduce the full
  // collection, including the cross-endpoint driver dedup.
  std::vector<Target> chunked;
  std::unordered_set<sta::Pin*> visited;
  for (size_t begin = 0; begin < endpoint_count; ++begin) {
    for (Target& target : collector.collectCritPathDriverPinTargets(
             valid, begin, begin + 1, visited)) {
      chunked.push_back(std::move(target));
    }
  }

  ASSERT_EQ(chunked.size(), full.size());
  for (size_t index = 0; index < full.size(); ++index) {
    EXPECT_EQ(chunked[index].driver_pin, full[index].driver_pin);
    EXPECT_EQ(chunked[index].violated_endpoint_pin,
              full[index].violated_endpoint_pin);
    EXPECT_FLOAT_EQ(chunked[index].slack, full[index].slack);
  }

  EXPECT_TRUE(collector
                  .collectCritPathDriverPinTargets(
                      valid, endpoint_count, endpoint_count + 1, visited)
                  .empty());
}

}  // namespace rsz
