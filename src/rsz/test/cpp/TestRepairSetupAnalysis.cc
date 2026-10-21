// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// Setup repair profiler mode 3: the per-move analysis.

#include "TestResizerMtFixture.hh"

namespace rsz {

TEST_F(TestResizerMt, PreparedTargetEndpointSurvivesPathRefresh)
{
  Target target = makeTarget("out0", "load0", "Z");
  const sta::Pin* endpoint = target.endpointPin(resizer_);
  ASSERT_NE(endpoint, nullptr);
  const sta::Pin* violated_endpoint = sta_->network()->findPin("path_out");
  ASSERT_NE(violated_endpoint, nullptr);
  target.violated_endpoint_pin = violated_endpoint;
  target.endpoint_path = nullptr;
  target.driver_path = nullptr;
  sta_->delaysInvalid();
  sta_->findRequireds();
  EXPECT_EQ(target.endpointPin(resizer_), endpoint);
  EXPECT_EQ(target.violatedEndpointPin(resizer_), violated_endpoint);
}

TEST_F(TestResizerMt, AnalysisSerialTargetOffPreservesOriginalReference)
{
  const Target target = makeTarget("out0", "load0", "Z");
  RepairSetupProfiler profiler(resizer_);
  RepairSetupAnalysisTarget live(resizer_, target);
  EXPECT_TRUE(live.refresh());
  EXPECT_EQ(&live.target(), &target);
  EXPECT_EQ(live.target().driver_path, target.driver_path);
}

TEST_F(TestResizerMt, AnalysisSerialTargetRebindsAcrossStaUpdates)
{
  ProfileTestDirectory directory;
  resizer_.runRepairSetupPreamble();
  est::IncrementalParasiticsGuard guard(resizer_.estimateParasitics());
  Target target = makeTarget("out0", "load0", "Z");
  const sta::Pin* violated_endpoint = sta_->network()->findPin("path_out");
  ASSERT_NE(violated_endpoint, nullptr);
  target.violated_endpoint_pin = violated_endpoint;
  RepairSetupProfiler profiler(resizer_,
                               RepairSetupProfiler::Mode::kAnalysis,
                               directory.root().string());
  profiler.beginPhase("LEGACY", 1);
  RepairSetupAnalysisTarget live(resizer_, target);
  ASSERT_TRUE(live.refresh());
  const Target* address = &live.target();
  sta::Instance* inst = db_network_->dbToSta(block_->findInst("load0"));
  ASSERT_TRUE(
      resizer_.replaceCell(inst, db_network_->findLibertyCell("BUF_X8")));

  resizer_.updateParasiticsAndTiming();
  target.endpoint_path = nullptr;
  target.driver_path = nullptr;
  ASSERT_TRUE(live.refresh());
  EXPECT_EQ(&live.target(), address);
  EXPECT_EQ(live.target().driver_path->pin(sta_.get()), target.driver_pin);
  EXPECT_EQ(live.target().driver_path->transition(sta_.get()),
            target.driver_rf);
  EXPECT_EQ(live.target().endpointPin(resizer_), target.endpoint_pin);
  EXPECT_EQ(live.target().violatedEndpointPin(resizer_), violated_endpoint);

  RepairSetupAnalysis observer(
      resizer_, live.target(), live.originalEndpoint());
  EXPECT_EQ(observer.violatedEndpoint(), "path_out");

  StubBufferMoveCandidate rejected(resizer_, live.target(), 1, false);
  MoveCommitter committer(resizer_);
  committer.init();
  RepairSetupProfiler::moveGenerated(rejected);
  RepairSetupProfiler::moveSelected(
      rejected, &live.target(), live.originalEndpoint());
  EXPECT_FALSE(committer.commit(rejected).accepted);
  sta_->delaysInvalid();
  sta_->findRequireds();
  ASSERT_TRUE(live.refresh());
  RepairSetupProfiler::moveSelected(
      rejected, &live.target(), live.originalEndpoint());
  EXPECT_FALSE(committer.commit(rejected).accepted);
  EXPECT_EQ(&rejected.target(), address);
  profiler.complete(true);
}

TEST_F(TestResizerMt, AnalysisSerialTargetReportsDriverPrefixFallback)
{
  ProfileTestDirectory directory;
  resizer_.runRepairSetupPreamble();
  Target target = makeTarget("out0", "load0", "Z");
  target.endpoint_pin = sta_->network()->findPin("path_out");
  ASSERT_NE(target.endpoint_pin, nullptr);
  RepairSetupProfiler profiler(resizer_,
                               RepairSetupProfiler::Mode::kAnalysis,
                               directory.root().string());
  RepairSetupAnalysisTarget live(resizer_, target);
  ASSERT_TRUE(live.refresh());
  EXPECT_EQ(live.target().endpoint_path, live.target().driver_path);
  EXPECT_EQ(live.target().endpointPin(resizer_), target.driver_pin);
  EXPECT_EQ(live.originalEndpoint(), target.endpoint_pin);
  RepairSetupAnalysis observer(
      resizer_, live.target(), live.originalEndpoint());
  EXPECT_EQ(observer.selectedEndpoint(), "path_out");
  EXPECT_EQ(observer.selectedPathEnd(), "load0/Z");
  EXPECT_STREQ(observer.pathContext(), "driver_prefix");
  EXPECT_FALSE(observer.snapshot().pins.empty());
  profiler.complete(true);
}

// A LEGACY target does not carry its violated endpoint, so the analysis
// reports the endpoint the policy repairs.  An instance target reports none.
TEST_F(TestResizerMt, AnalysisReportsPolicyEndpointOfLegacyTarget)
{
  resizer_.runRepairSetupPreamble();
  sta_->findRequireds();
  const Target target = makeTarget("out0", "load0", "Z");
  ASSERT_NE(target.driver_pin, nullptr);
  EXPECT_EQ(target.violated_endpoint_pin, nullptr);
  const sta::Pin* policy_endpoint = sta_->network()->findPin("path_out");
  ASSERT_NE(policy_endpoint, nullptr);

  const RepairSetupAnalysis legacy(resizer_, target, nullptr, policy_endpoint);
  EXPECT_EQ(legacy.violatedEndpoint(), "path_out");

  Target instance_target;
  instance_target.views = kInstanceView;
  instance_target.driver_pin = target.driver_pin;
  const RepairSetupAnalysis instance(
      resizer_, instance_target, nullptr, policy_endpoint);
  EXPECT_TRUE(instance.violatedEndpoint().empty());
}

TEST_F(TestResizerMt, AnalysisCapturesBothRfAndSharedFanin)
{
  resizer_.runRepairSetupPreamble();
  sta_->findRequireds();
  const Target target = makeTarget("out0", "load0", "Z");
  ASSERT_NE(target.driver_pin, nullptr);
  RepairSetupAnalysis observer(resizer_, target);
  const AnalysisSnapshot snapshot = observer.snapshot();
  bool rise = false;
  bool fall = false;
  bool sibling = false;
  bool driver = false;
  for (const AnalysisPinState& pin : snapshot.pins) {
    if (pin.pin == "load0/Z") {
      rise |= pin.rf == "rise";
      fall |= pin.rf == "fall";
      ASSERT_TRUE(pin.arrival.has_value());
      ASSERT_TRUE(pin.required.has_value());
      ASSERT_TRUE(pin.slack.has_value());
      EXPECT_NEAR(*pin.required - *pin.arrival, *pin.slack, 1e-13);
    }

    sibling |= pin.pin == "load1/Z"
               && pin.role.find("sibling_output") != std::string::npos;
    driver |= pin.pin == "target/Z" && pin.pin_cap.has_value()
              && pin.wire_cap.has_value();
  }

  EXPECT_TRUE(rise);
  EXPECT_TRUE(fall);
  EXPECT_TRUE(sibling);
  EXPECT_TRUE(driver);
  EXPECT_FALSE(snapshot.arcs.empty());
  EXPECT_FALSE(snapshot.connections.empty());
  EXPECT_FALSE(snapshot.paths.empty());
}

TEST_F(TestResizerMt, AnalysisIncludesExtractedWireCapacitance)
{
  resizer_.runRepairSetupPreamble();
  sta_->findRequireds();
  const Target target = makeTarget("out0", "load0", "Z");
  const sta::Pin* driver = sta_->network()->findPin("target/Z");
  ASSERT_NE(driver, nullptr);
  sta::Scene* scene = sta_->cmdScene();
  constexpr float wire_cap = 1.5e-15;
  for (const sta::RiseFall* rf : sta::RiseFall::range()) {
    float pin_cap, sdc_wire_cap, fanout;
    bool has_net_load;
    sta_->graphDelayCalc()->netCaps(driver,
                                    rf,
                                    scene,
                                    sta::MinMax::max(),
                                    pin_cap,
                                    sdc_wire_cap,
                                    fanout,
                                    has_net_load);
    ASSERT_FALSE(has_net_load);

    // A reduced Pi model holds total capacitance, unlike SDC wire load.
    scene->parasitics(sta::MinMax::max())
        ->makePiElmore(driver, rf, sta::MinMax::max(), wire_cap, 1.0, pin_cap);
  }

  RepairSetupAnalysis observer(resizer_, target);
  int checked = 0;
  for (const AnalysisPinState& pin : observer.snapshot().pins) {
    if (pin.pin == "target/Z") {
      ASSERT_TRUE(pin.wire_cap.has_value());
      EXPECT_NEAR(*pin.wire_cap, wire_cap, 1e-20);
      ++checked;
    }
  }

  EXPECT_EQ(checked, 2);
}

// The fields of one CSV line as the analysis writes it (quoted fields, ""
// escapes a quote).
static std::vector<std::string> csvFields(const std::string& line)
{
  std::vector<std::string> fields(1);
  bool quoted = false;
  for (size_t index = 0; index < line.size(); ++index) {
    const char ch = line[index];
    if (ch == '"' && quoted && index + 1 < line.size()
        && line[index + 1] == '"') {
      fields.back() += '"';
      ++index;
    } else if (ch == '"') {
      quoted = !quoted;
    } else if (ch == ',' && !quoted) {
      fields.emplace_back();
    } else {
      fields.back() += ch;
    }
  }
  return fields;
}

// The rows of a CSV file keyed by its header.
static std::vector<std::map<std::string, std::string>> csvRows(
    const std::string& contents)
{
  std::istringstream lines(contents);
  std::string line;
  std::vector<std::map<std::string, std::string>> rows;
  if (!std::getline(lines, line)) {
    return rows;
  }

  const std::vector<std::string> header = csvFields(line);
  while (std::getline(lines, line)) {
    const std::vector<std::string> fields = csvFields(line);
    EXPECT_EQ(fields.size(), header.size()) << line;
    std::map<std::string, std::string>& row = rows.emplace_back();
    for (size_t index = 0; index < header.size() && index < fields.size();
         ++index) {
      row[header[index]] = fields[index];
    }
  }
  return rows;
}

TEST_F(TestResizerMt, AnalysisWritesBadMoveReportAndRevertedOutcome)
{
  ProfileTestDirectory directory;
  resizer_.runRepairSetupPreamble();
  est::IncrementalParasiticsGuard guard(resizer_.estimateParasitics());
  sta::Instance* instance
      = db_network_->dbToSta(block_->findInst("path_target"));
  sta::LibertyCell* strong = db_network_->findLibertyCell("NAND2_X4");
  sta::LibertyCell* weak = db_network_->findLibertyCell("NAND2_X1");
  ASSERT_NE(strong, nullptr);
  ASSERT_NE(weak, nullptr);
  ASSERT_TRUE(resizer_.replaceCell(instance, strong));
  resizer_.updateParasiticsAndTiming();
  const Target target = makeTarget("path_out", "path_target", "ZN");
  ASSERT_NE(target.driver_pin, nullptr);
  SizeUpCandidate candidate(
      resizer_, target, target.driver_pin, instance, weak);
  SizeUpCandidate unselected(
      resizer_, target, target.driver_pin, instance, strong);

  MoveCommitter committer(resizer_);
  committer.init();

  {
    RepairSetupProfiler profiler(resizer_,
                                 RepairSetupProfiler::Mode::kAnalysis,
                                 directory.root().string());
    profiler.beginPhase("LEGACY", 1);
    const ProfileIteration iteration("legacy_pass");
    RepairSetupProfiler::moveGenerated(unselected);
    RepairSetupProfiler::moveGenerated(candidate);
    RepairSetupProfiler::moveEstimated(candidate,
                                       {.legal = true, .score = 1e-12f});
    RepairSetupProfiler::moveSelected(candidate);
    committer.beginJournal();
    ASSERT_TRUE(committer.commit(candidate).accepted);
    committer.restoreJournal();
    profiler.complete(true);
  }

  const std::filesystem::path run = directory.root() / "run_1";
  const std::filesystem::path analysis = run / "analysis";
  const std::string summary = readProfileFile(analysis / "bad_moves.md");
  EXPECT_NE(summary.find("target_rf_slack_loss"), std::string::npos);
  EXPECT_NE(summary.find("reverted"), std::string::npos);
  const std::string report = readProfileFile(analysis / "moves/move_000001.md");
  EXPECT_NE(report.find("NAND2_X4"), std::string::npos);
  EXPECT_NE(report.find("NAND2_X1"), std::string::npos);
  EXPECT_NE(report.find("rise"), std::string::npos);
  EXPECT_NE(report.find("fall"), std::string::npos);
  EXPECT_NE(report.find("Run-local outcome\n\nreverted"), std::string::npos);
  // The outcome leads the report, ahead of the interpretation.
  EXPECT_LT(report.find("Run-local outcome"), report.find("## Interpretation"));
  // The selected endpoint datapath passes the target and marks its rows, so
  // the target prefix it contains is not repeated.
  EXPECT_NE(report.find("### Selected target endpoint"), std::string::npos);
  EXPECT_LT(report.find("### Global worst endpoint"),
            report.find("### Selected target endpoint"));
  EXPECT_NE(report.find("**`path_target/ZN`** (target)"), std::string::npos);
  EXPECT_EQ(report.find("Target prefix"), std::string::npos);
  EXPECT_NE(
      readProfileFile(analysis / "analysis_pins.csv").find("path_target/A2"),
      std::string::npos);
  EXPECT_NE(
      readProfileFile(run / "stats.csv")
          .find(",\"count\",\"analysis_selected_snapshots\",\"\",1,1,1\n"),
      std::string::npos);

  // The analysis scripts read these columns of the move and outcome tables.
  const std::vector<std::map<std::string, std::string>> moves
      = csvRows(readProfileFile(analysis / "analysis_moves.csv"));
  ASSERT_EQ(moves.size(), 1u);
  const std::map<std::string, std::string>& move = moves.front();
  EXPECT_EQ(move.at("move_id"), "1");
  EXPECT_EQ(move.at("phase"), "LEGACY");
  EXPECT_EQ(move.at("applied"), "1");
  EXPECT_EQ(move.at("before_master"), "NAND2_X4");
  EXPECT_EQ(move.at("after_master"), "NAND2_X1");
  EXPECT_NE(move.at("findings").find("target_rf_slack_loss"),
            std::string::npos);
  for (const char* column : {"weighted_score",
                             "wns_before_s",
                             "wns_after_s",
                             "wns_delta_s",
                             "tns_delta_s",
                             "target_before_s",
                             "target_after_s"}) {
    EXPECT_FALSE(move.at(column).empty()) << column;
  }
  // A serial candidate has no prepared window and its estimate no raw gain,
  // so the window gain and the estimate error stay empty.
  for (const char* column :
       {"raw_gain_s", "actual_window_gain_s", "estimate_error_s"}) {
    EXPECT_TRUE(move.at(column).empty()) << column;
  }

  const std::vector<std::map<std::string, std::string>> outcomes
      = csvRows(readProfileFile(analysis / "analysis_outcomes.csv"));
  ASSERT_EQ(outcomes.size(), 1u);
  EXPECT_EQ(outcomes.front().at("move_id"), "1");
  EXPECT_EQ(outcomes.front().at("applied"), "1");
  EXPECT_EQ(outcomes.front().at("outcome"), "reverted");
}

TEST_F(TestResizerMt, AnalysisKeepsInputCapWithoutTimingPath)
{
  resizer_.runRepairSetupPreamble();
  odb::dbNet* floating = odb::dbNet::create(block_, "analysis_floating");
  odb::dbITerm* input = block_->findInst("path_target")->findITerm("A2");
  input->disconnect();
  input->connect(floating);
  sta_->findRequireds();
  const Target target = makeTarget("path_out", "path_target", "ZN");
  ASSERT_NE(target.driver_pin, nullptr);
  RepairSetupAnalysis observer(resizer_, target);
  const AnalysisSnapshot snapshot = observer.snapshot();
  int observed = 0;
  for (const AnalysisPinState& pin : snapshot.pins) {
    if (pin.pin == "path_target/A2") {
      ++observed;
      ASSERT_TRUE(pin.input_cap.has_value());
      EXPECT_GT(*pin.input_cap, 0.0);
      EXPECT_FALSE(pin.arrival.has_value());
      EXPECT_FALSE(pin.slack.has_value());
    }
  }

  EXPECT_EQ(observed, 2);
}

TEST_F(TestResizerMt, AnalysisExplainsSharedInputLoadLoss)
{
  ProfileTestDirectory directory;
  resizer_.runRepairSetupPreamble();
  est::IncrementalParasiticsGuard guard(resizer_.estimateParasitics());
  sta_->findRequireds();
  const Target target = makeTarget("out0", "load0", "Z");
  ASSERT_NE(target.driver_pin, nullptr);
  sta::Instance* instance = db_network_->dbToSta(block_->findInst("load0"));
  sta::LibertyCell* large = db_network_->findLibertyCell("BUF_X32");
  ASSERT_NE(large, nullptr);
  SizeUpCandidate candidate(
      resizer_, target, target.driver_pin, instance, large);

  MoveCommitter committer(resizer_);
  committer.init();

  {
    RepairSetupProfiler profiler(resizer_,
                                 RepairSetupProfiler::Mode::kAnalysis,
                                 directory.root().string());
    profiler.beginPhase("LEGACY", 1);
    const ProfileIteration iteration("legacy_pass");
    RepairSetupProfiler::moveGenerated(candidate);
    RepairSetupProfiler::moveEstimated(candidate,
                                       {.legal = true, .score = 1e-12f});
    RepairSetupProfiler::moveSelected(candidate);
    EXPECT_TRUE(committer.commit(candidate).accepted);
    profiler.complete(true);
  }

  const std::filesystem::path analysis = directory.root() / "run_1/analysis";
  const std::string report = readProfileFile(analysis / "moves/move_000001.md");
  EXPECT_NE(report.find("input_cap_growth_with_side_loss"), std::string::npos);
  // The target input's connection row carries its Cin growth.
  const size_t input_row = report.find("`load0/A` | `load0` | load | ");
  ASSERT_NE(input_row, std::string::npos);
  EXPECT_NE(report.substr(input_row, report.find('\n', input_row) - input_row)
                .find(" -> "),
            std::string::npos);
  EXPECT_NE(report.find("load0/A"), std::string::npos);
  EXPECT_NE(report.find("load1/A"), std::string::npos);
  EXPECT_NE(report.find("target/Z"), std::string::npos);
  EXPECT_NE(report.find("checkpoint_accepted"), std::string::npos);
  EXPECT_EQ(block_->findInst("load0")->getMaster()->getName(),
            std::string("BUF_X32"));
}

}  // namespace rsz
