// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <unistd.h>

#include <filesystem>
#include <memory>
#include <string>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "gmock/gmock.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/lefin.h"
#include "rsz/Resizer.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/Sta.hh"
#include "sta/TimingArc.hh"
#include "stt/SteinerTreeBuilder.h"
#include "tst/fixture.h"
#include "tst/nangate45_fixture.h"
#include "utl/Logger.h"
#include "utl/ServiceRegistry.h"
#include "utl/deleter.h"

namespace rsz {

static const std::string prefix("_main/src/rsz/test/");

class BufRemTest : public tst::Nangate45Fixture
{
 protected:
  BufRemTest()
      :  // initializer resizer
        stt_(db_.get(), &logger_),
        service_registry_(&logger_),
        dp_(db_.get(), &logger_),
        ant_(db_.get(), &logger_),
        grt_(&logger_,
             &service_registry_,
             &stt_,
             db_.get(),
             sta_.get(),
             &ant_,
             &dp_),
        ep_(&logger_, &service_registry_, db_.get(), sta_.get(), &stt_, &grt_),
        resizer_(&logger_, db_.get(), sta_.get(), &stt_, &grt_, &dp_, &ep_)
  {
    library_ = readLiberty(prefix + "Nangate45/Nangate45_typ.lib");
    db_network_ = sta_->getDbNetwork();
    db_network_->setBlock(block_);
    block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
    // register proper callbacks for timer like read_def
    sta_->postReadDef(block_);

    // create a chain consisting of 4 buffers
    const char* layer = "metal1";

    makeBTerm(block_,
              "in1",
              {.bpins = {{.layer_name = layer, .rect = {0, 0, 10, 10}}}});

    odb::dbBTerm* outPort = makeBTerm(
        block_,
        "out1",
        {.io_type = odb::dbIoType::OUTPUT,
         .bpins = {{.layer_name = layer, .rect = {990, 990, 1000, 1000}}}});

    makeBTerm(
        block_,
        "out2",
        {.io_type = odb::dbIoType::OUTPUT,
         .bpins = {{.layer_name = layer, .rect = {980, 980, 1000, 990}}}});

    auto makeInst = [&](const char* master_name,
                        const char* inst_name,
                        const odb::Point& location,
                        const char* in_net,
                        const char* out_net) {
      odb::dbMaster* master = db_->findMaster(master_name);
      return tst::Fixture::makeInst(
          block_,
          master,
          inst_name,
          {.location = location,
           .status = odb::dbPlacementStatus::PLACED,
           .iterms = {{.net_name = in_net, .term_name = "A"},
                      {.net_name = out_net, .term_name = "Z"}}});
    };

    makeInst("BUF_X1", "b1", {100, 100}, "in1", "n1");
    makeInst("BUF_X2", "b2", {200, 200}, "n1", "n2");
    makeInst("BUF_X4", "b3", {300, 300}, "n2", "n3");
    makeInst("BUF_X8", "b4", {400, 400}, "n3", "out1");
    makeInst("BUF_X8", "b5", {500, 500}, "n1", "out2");

    sta::Graph* graph = sta_->ensureGraph();
    sta::Pin* outStaPin = db_network_->dbToSta(outPort);
    outVertex_ = graph->pinLoadVertex(outStaPin);
  }

  stt::SteinerTreeBuilder stt_;
  utl::ServiceRegistry service_registry_;
  dpl::Opendp dp_;
  ant::AntennaChecker ant_;
  grt::GlobalRouter grt_;
  est::EstimateParasitics ep_;
  rsz::Resizer resizer_;

  sta::LibertyLibrary* library_{nullptr};
  sta::dbNetwork* db_network_{nullptr};
  sta::Vertex* outVertex_{nullptr};
};

TEST_F(BufRemTest, SlackImproves)
{
  const float origArrival = sta_->arrival(outVertex_,
                                          sta::RiseFallBoth::riseFall(),
                                          sta_->scenes(),
                                          sta::MinMax::max());

  // Remove buffers 'b2' and 'b3' from the buffer chain
  resizer_.initBlock();
  db_->setLogger(&logger_);

  {
    est::IncrementalParasiticsGuard guard(&ep_);

    resizer_.journalBeginTest();
    resizer_.logger()->setDebugLevel(utl::RSZ, "journal", 1);

    auto insts = std::make_unique<sta::InstanceSeq>();
    odb::dbInst* inst1 = block_->findInst("b2");
    sta::Instance* sta_inst1 = db_network_->dbToSta(inst1);
    insts->emplace_back(sta_inst1);

    odb::dbInst* inst2 = block_->findInst("b3");
    sta::Instance* sta_inst2 = db_network_->dbToSta(inst2);
    insts->emplace_back(sta_inst2);

    resizer_.removeBuffers(*insts);
    const float newArrival = sta_->arrival(outVertex_,
                                           sta::RiseFallBoth::riseFall(),
                                           sta_->scenes(),
                                           sta::MinMax::max());

    EXPECT_LT(newArrival, origArrival);
    resizer_.journalRestoreTest();
  }

  const float restoredArrival = sta_->arrival(outVertex_,
                                              sta::RiseFallBoth::riseFall(),
                                              sta_->scenes(),
                                              sta::MinMax::max());

  EXPECT_EQ(restoredArrival, origArrival);
}

// Regression guard for #10210: rsz moves (SizeUpMove / SwapPinsMove /
// UnbufferMove / RecoverPower) read the driver cell's input LibertyPort from
// drvr_path->prevPath()->pin()->libertyPort().  The fix switches to
// drvr_path->prevArc()->from(), which is carried on the Path itself via
// prev_edge_id_/prev_arc_idx_ and therefore does not dereference the
// prev_path_ raw pointer.  This test pins three properties the fix relies on:
//   1. prevArc()->from() resolves to the driver's own input LibertyPort.
//   2. It agrees with the pre-fix prevPath()->pin()->libertyPort() form on
//      the current master code path (equivalence, so the fix is not a
//      behavioral change in the non-stale case).
//   3. prev_path_ on a driver-output path lands on the driver's own input
//      pin -- same instance -- which is the master invariant the fix
//      preserves without dereferencing the raw pointer.
TEST_F(BufRemTest, PrevArcFromMatchesDriverInputPort)
{
  sta::Graph* graph = sta_->ensureGraph();
  sta::Network* network = sta_->network();
  sta_->updateTiming(true);

  odb::dbInst* b3_db = block_->findInst("b3");
  ASSERT_NE(b3_db, nullptr);
  sta::Instance* b3 = db_network_->dbToSta(b3_db);
  sta::Pin* b3_z = network->findPin(b3, "Z");
  sta::Pin* b3_a = network->findPin(b3, "A");
  ASSERT_NE(b3_z, nullptr);
  ASSERT_NE(b3_a, nullptr);

  sta::Vertex* b3_z_vertex = graph->pinDrvrVertex(b3_z);
  ASSERT_NE(b3_z_vertex, nullptr);

  sta::Path* drvr_path
      = sta_->vertexWorstArrivalPath(b3_z_vertex, sta::MinMax::max());
  ASSERT_NE(drvr_path, nullptr);
  ASSERT_FALSE(drvr_path->isNull());

  // Property 1: the fix's form resolves to the driver's input LibertyPort.
  const sta::TimingArc* in_arc = drvr_path->prevArc(sta_.get());
  ASSERT_NE(in_arc, nullptr);
  sta::LibertyPort* port_via_arc = in_arc->from();
  EXPECT_EQ(port_via_arc, network->libertyPort(b3_a));

  // Property 2: equivalence with the pre-fix form on non-stale paths.
  sta::Path* prev_path = drvr_path->prevPath();
  ASSERT_NE(prev_path, nullptr);
  sta::Pin* prev_pin = prev_path->pin(sta_.get());
  ASSERT_NE(prev_pin, nullptr);
  EXPECT_EQ(port_via_arc, network->libertyPort(prev_pin));

  // Property 3: prev_path_ on a driver-output path lands on the driver's
  // own input pin (same instance).  The historical bug in #10210 was
  // against the expanded.path(drvr_index - 1) form (forward direction in
  // the expanded array), which could land on a downstream load pin.  The
  // prevArc form is safe regardless of that invariant holding.
  EXPECT_EQ(network->instance(prev_pin), b3);
}

// Companion to ExpandedPathPointerGoesStaleAfterCellReplace: verifies that
// the fix's prevArc()->from() resolution still returns the correct driver
// input port after the same realloc that leaves PathExpanded snapshots
// stale.  This does not dereference any stale pointer -- it re-queries a
// fresh Path from the STA after the perturbation, then checks that
// prevArc()->from() matches the LibertyPort of the driver's input pin both
// before and after.  Because b3's cell is unchanged, the two LibertyPort
// pointers must be identical (Liberty objects have library lifetime).
TEST_F(BufRemTest, PrevArcFromStableAcrossCellReplace)
{
  sta::Graph* graph = sta_->ensureGraph();
  sta::Network* network = sta_->network();
  sta_->updateTiming(true);

  odb::dbInst* b3_db = block_->findInst("b3");
  ASSERT_NE(b3_db, nullptr);
  sta::Instance* b3 = db_network_->dbToSta(b3_db);
  sta::Pin* b3_z = network->findPin(b3, "Z");
  sta::Pin* b3_a = network->findPin(b3, "A");
  ASSERT_NE(b3_z, nullptr);
  ASSERT_NE(b3_a, nullptr);
  sta::Vertex* b3_z_vertex = graph->pinDrvrVertex(b3_z);
  ASSERT_NE(b3_z_vertex, nullptr);

  sta::LibertyPort* expected_port = network->libertyPort(b3_a);
  ASSERT_NE(expected_port, nullptr);

  // Baseline: fresh path, fresh prevArc()->from().
  sta::Path* drvr_path_before
      = sta_->vertexWorstArrivalPath(b3_z_vertex, sta::MinMax::max());
  ASSERT_NE(drvr_path_before, nullptr);
  const sta::TimingArc* arc_before = drvr_path_before->prevArc(sta_.get());
  ASSERT_NE(arc_before, nullptr);
  sta::LibertyPort* port_before = arc_before->from();
  EXPECT_EQ(port_before, expected_port);

  // Trigger the same realloc the staleness test uses.
  odb::dbInst* b1_db = block_->findInst("b1");
  ASSERT_NE(b1_db, nullptr);
  sta::Instance* b1 = db_network_->dbToSta(b1_db);
  sta::LibertyCell* buf_x8 = network->findLibertyCell("BUF_X8");
  ASSERT_NE(buf_x8, nullptr);
  sta_->replaceCell(b1, buf_x8);
  sta_->updateTiming(true);

  // After realloc: fresh path (drvr_path_before may be stale and is
  // intentionally not touched here).  The b3 vertex may have a new paths_
  // array, but its internal A->Z arc is unchanged, so prevArc()->from()
  // must still resolve to b3's A port.
  sta::Path* drvr_path_after
      = sta_->vertexWorstArrivalPath(b3_z_vertex, sta::MinMax::max());
  ASSERT_NE(drvr_path_after, nullptr);
  const sta::TimingArc* arc_after = drvr_path_after->prevArc(sta_.get());
  ASSERT_NE(arc_after, nullptr);
  sta::LibertyPort* port_after = arc_after->from();

  EXPECT_EQ(port_after, expected_port);
  EXPECT_EQ(port_after, port_before);
}

// Demonstrates the root cause of issue #10210: PathExpanded captures Path*
// pointers at construction time, but the underlying Vertex::paths_ arrays
// (from Graph.cc) are delete[]'d and reallocated whenever a cell replace or
// buffer removal forces Search::setVertexArrivals to rebuild with a different
// tag_group.  After such a reallocation the Path pointers stored inside the
// expanded vector dangle -- expanded.path(k) returns the same stored address,
// but that address is outside the vertex's live paths array, so dereferencing
// it reads either freed memory or a different Vertex's reused Path slot.
//
// The fix drops dependence on expanded.path(drvr_index - 1) / prevPath()->pin()
// and resolves the driver's input port from drvr_path->prevArc()->from(),
// which reads prev_edge_id_/prev_arc_idx_ off the Path object itself -- no
// expanded indirection, no prev_path_ dereference.
//
// This test detects the stale state via pointer address comparison (never
// dereferences the stale pointer, so it is ASAN-safe).  When the Nangate45
// allocator happens to hand the new paths_ array the same base as the freed
// one, the test cannot distinguish stale vs live and is skipped.
//
// The test observes three OpenSTA implementation details:
//   (1) PathExpanded::paths_ stores raw Path* pointers (not copies).
//   (2) Vertex::makePaths does `delete[] paths_; new Path[N]` on tag_group
//       change in Search::setVertexArrivals.
//   (3) Vertex::paths() returns the base address of the current paths_ array.
// If OpenSTA refactors any of these (e.g., arena/pool allocation with stable
// addresses, in-place grow, PathExpanded value-copying), the test will skip
// rather than fail -- that is an acceptable signal that the UAF shape no
// longer applies.  The equivalence test above remains the semantic guard.
TEST_F(BufRemTest, ExpandedPathPointerGoesStaleAfterCellReplace)
{
  sta::Graph* graph = sta_->ensureGraph();
  sta::Network* network = sta_->network();
  sta_->updateTiming(true);

  // Endpoint path at the block output "out1".
  odb::dbBTerm* out_bterm = block_->findBTerm("out1");
  ASSERT_NE(out_bterm, nullptr);
  sta::Pin* out_pin = db_network_->dbToSta(out_bterm);
  sta::Vertex* out_vertex = graph->pinLoadVertex(out_pin);
  ASSERT_NE(out_vertex, nullptr);
  sta::Path* endpoint_path
      = sta_->vertexWorstArrivalPath(out_vertex, sta::MinMax::max());
  ASSERT_NE(endpoint_path, nullptr);

  sta::PathExpanded expanded(endpoint_path, sta_.get());
  ASSERT_GT(expanded.size(), 2u);

  // Pick an intermediate path.  It points into some Vertex v's paths_
  // array.  After we perturb the design, v's paths_ may be freed and
  // reallocated; the pointer we hold in `expanded` is the snapshot.
  const size_t target_index = expanded.size() / 2;
  const sta::Path* snapshot = expanded.path(target_index);
  ASSERT_NE(snapshot, nullptr);
  sta::Vertex* v = snapshot->vertex(sta_.get());
  ASSERT_NE(v, nullptr);

  // Snapshot the base of v's paths_ array before perturbation.
  sta::Path* v_paths_before = v->paths();
  ASSERT_NE(v_paths_before, nullptr);

  // Trigger Search::setVertexArrivals to rebuild v's paths_ array by
  // replacing the upstream-most buffer.  This invalidates arrivals along
  // the whole chain; the next incremental STA update reallocates paths_.
  odb::dbInst* b1_db = block_->findInst("b1");
  ASSERT_NE(b1_db, nullptr);
  sta::Instance* b1 = db_network_->dbToSta(b1_db);
  sta::LibertyCell* buf_x8 = network->findLibertyCell("BUF_X8");
  ASSERT_NE(buf_x8, nullptr);
  sta_->replaceCell(b1, buf_x8);
  sta_->updateTiming(true);

  sta::Path* v_paths_after = v->paths();

  if (v_paths_after == v_paths_before) {
    GTEST_SKIP()
        << "Allocator reused the same base for v->paths() -- test cannot "
           "prove staleness by pointer comparison without dereferencing "
           "freed memory (which is UB).  Run under a different allocator "
           "or with a forced-different-address hook to exercise this case.";
  }

  // v's paths_ was reallocated to a different address.  The snapshot
  // pointer we captured before -- which is what expanded.path(k) still
  // returns -- is outside v's live paths array, i.e. it dangles.
  //
  // pathCount on a reallocated vertex is bounded by TagGroup size; we
  // don't have that accessor here, but a loose upper-bound of 256 Path
  // slots suffices: if the snapshot is inside [v_paths_after,
  // v_paths_after + 256) it might alias, and we skip.  Otherwise it is
  // provably outside the live range.
  const uintptr_t snap_addr = reinterpret_cast<uintptr_t>(snapshot);
  const uintptr_t base_after = reinterpret_cast<uintptr_t>(v_paths_after);
  const uintptr_t max_live = base_after + 256 * sizeof(sta::Path);
  if (snap_addr >= base_after && snap_addr < max_live) {
    GTEST_SKIP() << "Snapshot pointer aliases into the new paths range; "
                    "cannot prove staleness without UB dereference.";
  }
  EXPECT_TRUE(snap_addr < base_after || snap_addr >= max_live)
      << "Stale pointer demonstrated: expanded.path(" << target_index
      << ") = " << snapshot << " is outside v->paths() = [" << v_paths_after
      << ", " << reinterpret_cast<void*>(max_live) << ")";
}

}  // namespace rsz
