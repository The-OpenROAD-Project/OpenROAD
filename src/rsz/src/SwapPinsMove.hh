// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <unordered_map>
#include <vector>

#include "BaseMove.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/Scene.hh"

// Provide gtest's FRIEND_TEST without pulling in the gtest header,
// because gtest is a dev-only dependency in Bazel and cannot be linked
// into production libraries (layering_check would reject it).
// The body matches gtest/gtest_prod.h exactly, so the guard makes it
// safe even if a TU also includes the real header later.
#ifndef FRIEND_TEST
#define FRIEND_TEST(test_case_name, test_name) \
  friend class test_case_name##_##test_name##_Test
#endif

namespace rsz {

class SwapPinsMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const sta::Path* drvr_path, float setup_slack_margin) override;

  const char* name() override { return "SwapPinsMove"; }

  void reportSwappablePins();

 private:
  // Allow the unit test to drive swapPins() directly without going
  // through full repair_timing. Test class must live in namespace rsz.
  FRIEND_TEST(TestResizer, SwapPinsFeedthroughModNet);

  using LibertyPortVec = std::vector<sta::LibertyPort*>;
  bool swapPins(sta::Instance* inst,
                sta::LibertyPort* port1,
                sta::LibertyPort* port2);
  void equivCellPins(const sta::LibertyCell* cell,
                     sta::LibertyPort* input_port,
                     LibertyPortVec& ports);
  void annotateInputSlews(sta::Instance* inst,
                          const sta::Scene* scene,
                          const sta::MinMax* min_max);
  void findSwapPinCandidate(sta::LibertyPort* input_port,
                            sta::LibertyPort* drvr_port,
                            const LibertyPortVec& equiv_ports,
                            float load_cap,
                            const sta::Scene* scene,
                            const sta::MinMax* min_max,
                            sta::LibertyPort** swap_port);
  void resetInputSlews();

  std::unordered_map<sta::LibertyPort*, LibertyPortVec> equiv_pin_map_;
};

}  // namespace rsz
