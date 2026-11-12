// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>

#include "gtest/gtest.h"
#include "pex/Model.h"
#include "pex/ModelJson.h"

namespace pex {
namespace {

Model sample()
{
  Model model;
  LateralTable& t = model.addLateral({"m2", "m1", "m3"},
                                     LateralTable({0.14, 0.21}, {0.14, 0.7}));
  const int n = t.spacingCount();
  for (int w = 0; w < t.widthCount(); w++) {
    for (int l = 0; l < n; l++) {
      for (int r = l; r < n; r++) {
        const double v = 0.001 * ((w * n + l) * n + r);
        t.set(w, l, r, {1 + v, 2 + v, 3 + v, 4 + v, 5 + v});
      }
    }
  }
  CrossingTable& crossing = model.addCrossing(
      {"m2", "m3", ""},
      CrossingTable({0.1, 0.9, 2.7}, {{0.88, 0.9}, {0.73, 0.8}, {0.48, 0.6}}));
  model.addCrossing({"m2", "m3", "m1"}, CrossingTable({0.1}, {{0.5, 0.4}}));
  crossing.setReach(
      {0.14, {0, 0.3}, {0, 2}, {{0.5, 0.4}, {1, 1}, {0.3, 0.2}, {1, 1}}});
  model.addCrossing(
      {"m2", "m1", ""},
      CrossingTable({0.1, 0.9},
                    {{0.8, 0.85}, {0.5, 0.6}, {0.9, 0.95}, {0.7, 0.75}},
                    {0.14}));
  DiagonalTable& d = model.addDiagonal(
      {"m2", "m1", ""},
      DiagonalTable({0.14, 0.42}, {-0.07, 0, 0.5}, {0.14, 0.28}, {0.14}));
  model
      .addDiagonal({"m2", "m1", "m3"},
                   DiagonalTable({0.14}, {0}, {0.14}, {0.14}))
      .set(0, 0, 0, 0, {0.01, 0.005, -0.001, -0.004});
  const int nf = d.farCount();
  const int ns = d.spacingCount();
  const int ng = d.gapCount();
  for (int i = 0; i < static_cast<int>(d.values().size()); i++) {
    const double v = 0.01 * i;
    d.set(i / nf / ns / ng,
          (i / nf / ns) % ng,
          (i / nf) % ns,
          i % nf,
          {v, 1.1 * v, -0.1 * v});
  }
  EndTable& free = model.addEnd({"m2", "", ""}, EndTable({0.14, 0.7}));
  free.set(0, 0, 0, {0.031});
  free.set(1, 0, 0, {0.09});
  EndTable& e = model.addEnd({"m2", "m1", "m3"},
                             EndTable({0.14}, {0.14}, {0.14, 0.56}));
  for (int i = 0; i < static_cast<int>(e.values().size()); i++) {
    const double v = 0.01 * (i + 1);
    e.set(0, i / 3, i % 3, {v, v / 2, v / 4, v / 8, v / 16});
  }
  e.setSides(0, 0, 1, {0.07, 0.01, 0.02, 0.03, 0.004});
  e.setCorners(0, {{-0.03, -0.01, -0.005}, {0.02, 0.004, 0.003}});
  return model;
}

TEST(ModelJson, RoundTrip)
{
  const Model model = sample();
  std::stringstream text;
  writeModel(model, {"sky130", "sky130.itf", "abc123"}, text);
  EXPECT_NE(text.str().find("\"source\":\"sky130.itf\""), std::string::npos);
  ModelSource source;
  const Model back = readModel(text, &source);
  EXPECT_EQ(source.technology, "sky130");
  EXPECT_EQ(source.file, "sky130.itf");
  EXPECT_EQ(source.fingerprint, "abc123");
  ASSERT_EQ(back.laterals().size(), 1u);
  const LateralTable* t = back.lateral({"m2", "m1", "m3"});
  ASSERT_NE(t, nullptr);
  const LateralTable& e = *model.lateral({"m2", "m1", "m3"});
  EXPECT_EQ(t->widths(), e.widths());
  EXPECT_EQ(t->spacings(), e.spacings());
  ASSERT_EQ(t->values().size(), e.values().size());
  for (size_t i = 0; i < e.values().size(); i++) {
    EXPECT_EQ(t->values()[i].ground, e.values()[i].ground);
    EXPECT_EQ(t->values()[i].left, e.values()[i].left);
    EXPECT_EQ(t->values()[i].right, e.values()[i].right);
    EXPECT_EQ(t->values()[i].below, e.values()[i].below);
    EXPECT_EQ(t->values()[i].above, e.values()[i].above);
  }
  const CrossingTable* c = back.crossing({"m2", "m3", ""});
  ASSERT_NE(c, nullptr);
  EXPECT_EQ(c->gaps(), model.crossing({"m2", "m3", ""})->gaps());
  ASSERT_EQ(c->values().size(), 3u);
  EXPECT_EQ(c->values()[1].total, 0.73);
  EXPECT_EQ(c->values()[2].cross, 0.6);
  EXPECT_TRUE(c->spacings().empty());
  ASSERT_TRUE(c->hasReach());
  const CrossingTable* planed = back.crossing({"m2", "m3", "m1"});
  ASSERT_NE(planed, nullptr);
  EXPECT_EQ(planed->values()[0].cross, 0.4);
  EXPECT_EQ(c->reach().width, 0.14);
  EXPECT_EQ(c->reach().past, (std::vector<double>{0, 0.3}));
  EXPECT_EQ(c->reach().extents, (std::vector<double>{0, 2}));
  EXPECT_EQ(c->reach().at(1, 0).total, 0.3);
  EXPECT_EQ(c->reach().at(1, 0).cross, 0.2);
  const CrossingTable* beside = back.crossing({"m2", "m1", ""});
  ASSERT_NE(beside, nullptr);
  EXPECT_FALSE(beside->hasReach());
  EXPECT_EQ(beside->spacings(), (std::vector<double>{0.14}));
  EXPECT_EQ(beside->at(0, 1).total, 0.5);
  EXPECT_EQ(beside->at(1, 1).cross, 0.75);
  ASSERT_EQ(back.diagonals().size(), 2u);
  const DiagonalTable* opposite = back.diagonal({"m2", "m1", "m3"});
  ASSERT_NE(opposite, nullptr);
  EXPECT_EQ(opposite->at(0, 0, 0, 0).plane_delta, -0.004);
  const DiagonalTable* d = back.diagonal({"m2", "m1", ""});
  ASSERT_NE(d, nullptr);
  EXPECT_EQ(d->gaps(), model.diagonal({"m2", "m1", ""})->gaps());
  EXPECT_EQ(d->farSpacings(), model.diagonal({"m2", "m1", ""})->farSpacings());
  ASSERT_EQ(d->values().size(), 2u * 3 * 3 * 2);
  EXPECT_EQ(d->values()[5].coupling, 0.05);
  EXPECT_DOUBLE_EQ(d->values()[5].total_delta, 1.1 * 0.05);
  EXPECT_DOUBLE_EQ(d->values()[5].lateral_delta, -0.1 * 0.05);
  const EndTable* end = back.end({"m2", "", ""});
  ASSERT_NE(end, nullptr);
  EXPECT_EQ(end->widths(), model.end({"m2", "", ""})->widths());
  EXPECT_TRUE(end->spacings().empty());
  EXPECT_EQ(end->at(1, 0, 0).total, 0.09);
  const EndTable* covered = back.end({"m2", "m1", "m3"});
  ASSERT_NE(covered, nullptr);
  EXPECT_EQ(covered->gaps(), (std::vector<double>{0.14, 0.56}));
  ASSERT_EQ(covered->values().size(), 2u * 3);
  // [spacing][gap] = [1][1] is the fifth value.
  EXPECT_EQ(covered->at(0, 1, 1).total, 0.05);
  EXPECT_EQ(covered->at(0, 1, 1).below, 0.025);
  EXPECT_EQ(covered->at(0, 1, 1).above, 0.0125);
  EXPECT_EQ(covered->at(0, 1, 1).lateral, 0.00625);
  EXPECT_EQ(covered->at(0, 1, 1).facing, 0.003125);
  EXPECT_FALSE(end->hasSides());
  ASSERT_TRUE(covered->hasSides());
  EXPECT_EQ(covered->sides(0, 0, 1).total, 0.07);
  EXPECT_EQ(covered->sides(0, 0, 1).above, 0.02);
  EXPECT_EQ(covered->sides(0, 0, 1).left, 0.03);
  EXPECT_EQ(covered->sides(0, 1, 0).left, 0.004);
  EXPECT_EQ(covered->sides(0, 1, 0).right, 0.03);
  EXPECT_FALSE(end->hasCorners());
  ASSERT_TRUE(covered->hasCorners());
  EXPECT_EQ(covered->corners(0).inner.total, -0.03);
  EXPECT_EQ(covered->corners(0).inner.above, -0.005);
  EXPECT_EQ(covered->corners(0).outer.below, 0.004);
}

// A lateral table is its own mirror image, so the file holds only the
// points with left <= right and the reader restores the rest.
TEST(ModelJson, WritesHalfTheLateralGrid)
{
  Model model;
  LateralTable& t = model.addLateral({"m1", "", ""}, LateralTable({1, 2}, {1}));
  for (int w = 0; w < 2; w++) {
    t.set(w, 0, 0, {1, 2, 2, 0, 0});
    t.set(w, 0, 1, {3, 4, 0, 0, 0});
    t.set(w, 1, 1, {5, 0, 0, 0, 0});
  }
  std::stringstream text;
  writeModel(model, {}, text);
  // The four table kinds, the widths, the spacings, the values and 2 × 3
  // points.
  const std::string written = text.str();
  EXPECT_EQ(std::count(written.begin(), written.end(), '['), 4 + 3 + 6);
  const Model back = readModel(text);
  const LateralTable* b = back.lateral({"m1", "", ""});
  ASSERT_NE(b, nullptr);
  EXPECT_EQ(b->at(1, 0, 1).left, 4);
  EXPECT_EQ(b->at(1, 1, 0).right, 4);
  EXPECT_EQ(b->at(1, 1, 0).left, 0);
  EXPECT_EQ(b->at(1, 1, 0).ground, 3);
}

// Values carry seven significant digits, enough for solves good to a
// fraction of a percent, and no more.
TEST(ModelJson, RoundsToSevenDigits)
{
  Model model;
  model.addEnd({"m2", "", ""}, EndTable({1.0 / 3})).set(0, 0, 0, {2.0 / 3});
  std::stringstream text;
  writeModel(model, {}, text);
  EXPECT_NE(text.str().find("\"widths\":[3.333333E-1]"), std::string::npos);
  EXPECT_NE(text.str().find("\"values\":[[6.666667E-1,"), std::string::npos);
  const Model back = readModel(text);
  EXPECT_NEAR(back.endExcess({"m2", "", ""}, 0.5), 2.0 / 3, 1e-7);
}

// A failed write leaves an existing file as it was.
TEST(ModelJson, FailedWriteKeepsTheFile)
{
  const std::string path = testing::TempDir() + "/pex_model.json";
  writeModelFile(sample(), {"t", "first", "f"}, path);
  // The temporary file the write goes through cannot be opened.
  std::filesystem::create_directory(path + ".tmp");
  EXPECT_THROW(writeModelFile(sample(), {"t", "second", "f"}, path),
               std::runtime_error);
  std::filesystem::remove(path + ".tmp");
  ModelSource source;
  readModelFile(path, &source);
  EXPECT_EQ(source.file, "first");
}

TEST(ModelJson, RejectsOtherFormats)
{
  // An earlier version, whose end tables had no neighbors, or a later one.
  for (const int version : {1, 99}) {
    std::stringstream text("{\"format\": \"pex model\", \"version\": "
                           + std::to_string(version) + "}");
    EXPECT_THROW(readModel(text), std::runtime_error);
  }
  std::stringstream other("{\"format\": \"something\", \"version\": 1}");
  EXPECT_THROW(readModel(other), std::runtime_error);
  std::stringstream broken("{\"format\": ");
  EXPECT_THROW(readModel(broken), std::runtime_error);
  std::stringstream wrong(
      "{\"format\": \"pex model\", \"version\": 2, \"lateral\": [{\"layer\": "
      "\"m\", \"below\": \"\", \"above\": \"\", \"widths\": [1], "
      "\"spacings\": [1], \"values\": [[1, 2, 3, 4, 5]]}], \"crossing\": [], "
      "\"diagonal\": [], \"end\": []}");
  EXPECT_THROW(readModel(wrong), std::runtime_error);
  EXPECT_THROW(readModelFile("/nonexistent/model.json"), std::runtime_error);
  // A table the model class rejects is a malformed file.
  std::stringstream bad(
      "{\"format\": \"pex model\", \"version\": 2, \"end\": [{\"layer\": "
      "\"m\", \"below\": \"\", \"above\": \"\", \"widths\": [2, 1], "
      "\"spacings\": [], \"gaps\": [], \"values\": [[0.1, 0, 0, 0, 0], "
      "[0.2, 0, 0, 0, 0]]}]}");
  EXPECT_THROW(readModel(bad), std::runtime_error);
  // A kind of table the file lacks is absent, not an error.
  std::stringstream bare("{\"format\": \"pex model\", \"version\": 2}");
  EXPECT_TRUE(readModel(bare).laterals().empty());
}

}  // namespace
}  // namespace pex
