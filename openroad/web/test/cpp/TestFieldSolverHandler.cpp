// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <string>

#include "boost/json/array.hpp"
#include "boost/json/object.hpp"
#include "boost/json/parse.hpp"
#include "boost/json/serialize.hpp"
#include "boost/json/value.hpp"
#include "fsv/FieldSolver.h"
#include "fsv_handler.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "request_handler.h"
#include "utl/Logger.h"

namespace web {
namespace {

boost::json::object dispatchSolve(FieldSolverHandler& handler,
                                  const std::string& json_str)
{
  WebSocketRequest req;
  req.json = boost::json::parse(json_str).as_object();
  req.id = static_cast<uint32_t>(req.json.at("id").as_int64());
  req.type = WebSocketRequest::kFsvSolve2D;
  WebSocketResponse resp = handler.handleSolve2D(req);
  EXPECT_EQ(resp.type, WebSocketResponse::kJson);
  const std::string body(resp.payload.begin(), resp.payload.end());
  return boost::json::parse(body).as_object();
}

TEST(FieldSolverHandlerTest, SolvesThreeWiresWithSurfaceCharges)
{
  FieldSolverHandler handler;
  const std::string req = R"({
    "id": 7,
    "type": "fsv_solve_2d",
    "eps": 3.9,
    "target_index": 0,
    "bins": 12,
    "ground_bins": 16,
    "ground_x_lo": -1.5,
    "ground_x_hi": 1.5,
    "min_walks": 32768,
    "max_walks": 65536,
    "wires": [
      {"name": "target", "x": 0.0, "z": 0.4, "width": 0.2, "height": 0.2},
      {"name": "w1", "x": -0.4, "z": 0.4, "width": 0.2, "height": 0.2},
      {"name": "w2", "x": 0.4, "z": 0.4, "width": 0.2, "height": 0.2}
    ]
  })";
  const boost::json::object res = dispatchSolve(handler, req);
  ASSERT_FALSE(res.contains("error")) << boost::json::serialize(res);

  const double total = res.at("total").to_number<double>();
  const double ground = res.at("ground").to_number<double>();
  EXPECT_GT(total, 0.0);
  EXPECT_GT(ground, 0.0);

  const auto& couplings = res.at("couplings").as_array();
  ASSERT_EQ(couplings.size(), 2u);
  const double c1 = couplings[0].as_object().at("cap").to_number<double>();
  const double c2 = couplings[1].as_object().at("cap").to_number<double>();
  EXPECT_GT(c1, 0.0);
  EXPECT_GT(c2, 0.0);
  // Symmetric neighbors have approximately equal coupling to the centered wire.
  EXPECT_NEAR(c1, c2, 0.1 * (c1 + c2));

  const auto& charges = res.at("charges").as_array();
  ASSERT_EQ(charges.size(), 3u);
  const auto& target_charge = charges[0].as_object();
  EXPECT_EQ(target_charge.at("bottom").as_array().size(), 12u);
  EXPECT_EQ(target_charge.at("top").as_array().size(), 12u);
  EXPECT_EQ(target_charge.at("left").as_array().size(), 12u);
  EXPECT_EQ(target_charge.at("right").as_array().size(), 12u);

  ASSERT_TRUE(res.contains("ground_charge"));
  const auto& gch = res.at("ground_charge").as_object();
  const auto& gvals = gch.at("values").as_array();
  ASSERT_EQ(gvals.size(), 16u);
  EXPECT_LT(gvals[8].to_number<double>(), 0.0);
}

TEST(FieldSolverHandlerTest, RejectsOverlappingWiresCleanly)
{
  FieldSolverHandler handler;
  const std::string req = R"({
    "id": 8,
    "type": "fsv_solve_2d",
    "eps": 3.9,
    "wires": [
      {"name": "target", "x": 0.0, "z": 0.4, "width": 0.2, "height": 0.2},
      {"name": "w1", "x": 0.05, "z": 0.4, "width": 0.2, "height": 0.2}
    ]
  })";
  const boost::json::object res = dispatchSolve(handler, req);
  EXPECT_TRUE(res.contains("error"));
}

TEST(FieldSolverHandlerTest, RejectsInvalidInputs)
{
  FieldSolverHandler handler;
  const boost::json::object bad_eps = dispatchSolve(
      handler,
      R"({"id":1,"type":"fsv_solve_2d","eps":-1,"wires":[{"name":"t","x":0,"z":0.5,"width":0.2,"height":0.2}]})");
  EXPECT_TRUE(bad_eps.contains("error"));

  const boost::json::object below_ground = dispatchSolve(
      handler,
      R"({"id":2,"type":"fsv_solve_2d","eps":3.9,"wires":[{"name":"t","x":0,"z":0,"width":0.2,"height":0.2}]})");
  EXPECT_TRUE(below_ground.contains("error"));
}

TEST(FieldSolverHandlerTest, LoadsItfAndSolvesWithStack)
{
  FieldSolverHandler handler;
  const std::string itf = R"(
TECHNOLOGY = test_stack
DIELECTRIC TOP { THICKNESS = 0.5 ER = 4.0 }
CONDUCTOR M2 { THICKNESS = 0.3 WMIN = 0.2 SMIN = 0.2 }
DIELECTRIC ILD2 { THICKNESS = 0.6 ER = 4.2 }
DIELECTRIC ILD2_C { THICKNESS = 0 ER = 3.5 SW_T = 0.02 TW_T = 0.02 IS_CONFORMAL ASSOCIATED_CONDUCTOR = M1 }
CONDUCTOR M1 { THICKNESS = 0.2 WMIN = 0.14 SMIN = 0.14 }
DIELECTRIC ILD1 { THICKNESS = 0.5 ER = 3.9 }
DIELECTRIC SUB { THICKNESS = 0.2 ER = 3.9 }
)";

  WebSocketRequest load_req;
  load_req.id = 10;
  load_req.type = WebSocketRequest::kFsvLoadItf;
  load_req.json["id"] = 10;
  load_req.json["type"] = "fsv_load_itf";
  load_req.json["name"] = "test.itf";
  load_req.json["content"] = itf;

  WebSocketResponse load_resp = handler.handleLoadItf(load_req);
  const std::string load_body(load_resp.payload.begin(),
                              load_resp.payload.end());
  const boost::json::object loaded = boost::json::parse(load_body).as_object();
  ASSERT_FALSE(loaded.contains("error")) << load_body;
  EXPECT_EQ(loaded.at("technology").as_string(), "test_stack");
  ASSERT_EQ(loaded.at("conductors").as_array().size(), 2u);
  ASSERT_GE(loaded.at("dielectrics").as_array().size(), 3u);

  const auto& m1 = loaded.at("conductors").as_array()[0].as_object();
  EXPECT_EQ(m1.at("name").as_string(), "M1");
  const double m1_z = m1.at("bottom").to_number<double>();
  const double m1_h = m1.at("thickness").to_number<double>();
  EXPECT_NEAR(m1_z, 0.5, 1e-6);
  EXPECT_NEAR(m1_h, 0.2, 1e-6);
  EXPECT_EQ(m1.at("conformals").as_array().size(), 1u);

  // Solve with use_itf = true on M1 wires, including a rounded z/height
  // accompanied by "layer": "M1" so the handler resolves exact zExtent.
  const std::string solve_req = R"({
    "id": 11,
    "type": "fsv_solve_2d",
    "use_itf": true,
    "bins": 10,
    "min_walks": 16384,
    "max_walks": 32768,
    "wires": [
      {"name": "target", "layer": "M1", "x": 0.0, "z": 0.5001, "width": 0.14, "height": 0.1999},
      {"name": "w1", "layer": "M1", "x": 0.34, "z": 0.5, "width": 0.14, "height": 0.2}
    ]
  })";
  const boost::json::object solved = dispatchSolve(handler, solve_req);
  ASSERT_FALSE(solved.contains("error")) << boost::json::serialize(solved);
  EXPECT_GT(solved.at("total").to_number<double>(), 0.0);
  ASSERT_EQ(solved.at("charges").as_array().size(), 2u);

  // Clearing ITF makes subsequent use_itf = true fail cleanly.
  WebSocketRequest clear_req;
  clear_req.id = 12;
  clear_req.type = WebSocketRequest::kFsvClearItf;
  handler.handleClearItf(clear_req);
  const boost::json::object after_clear = dispatchSolve(handler, solve_req);
  EXPECT_TRUE(after_clear.contains("error"));
}

TEST(FieldSolverHandlerTest, HidesBacksideLayersFromTechLef)
{
  odb::dbDatabase* db = odb::dbDatabase::create();
  odb::dbTech* tech = odb::dbTech::create(db, "gt2_test");
  auto* bm1
      = odb::dbTechLayer::create(tech, "BM1", odb::dbTechLayerType::ROUTING);
  bm1->setBackside(true);
  odb::dbTechLayer::create(tech, "BPR", odb::dbTechLayerType::ROUTING);
  odb::dbTechLayer::create(tech, "M0", odb::dbTechLayerType::ROUTING);

  FieldSolverHandler handler(/*field_solver=*/nullptr, db);
  const std::string itf = R"(
TECHNOLOGY = gt2_test
DIELECTRIC M0_diel { THICKNESS = 0.05 ER = 2.4 }
CONDUCTOR M0 { THICKNESS = 0.024 WMIN = 0.012 SMIN = 0.012 }
DIELECTRIC BPR_diel { THICKNESS = 0.09 ER = 3.9 }
CONDUCTOR BPR { THICKNESS = 0.09 WMIN = 0.032 SMIN = 0.112 }
DIELECTRIC BM1_diel { THICKNESS = 0.412 ER = 3.9 }
CONDUCTOR BM1 { THICKNESS = 0.112 WMIN = 0.056 SMIN = 0.056 }
DIELECTRIC dummy_diel { THICKNESS = 0.0001 ER = 2.5 }
)";

  WebSocketRequest load_req;
  load_req.id = 20;
  load_req.type = WebSocketRequest::kFsvLoadItf;
  load_req.json["id"] = 20;
  load_req.json["type"] = "fsv_load_itf";
  load_req.json["name"] = "gt2.itf";
  load_req.json["content"] = itf;

  WebSocketResponse load_resp = handler.handleLoadItf(load_req);
  const std::string load_body(load_resp.payload.begin(),
                              load_resp.payload.end());
  const boost::json::object loaded = boost::json::parse(load_body).as_object();
  ASSERT_FALSE(loaded.contains("error")) << load_body;

  // BM1 (backside) is hidden; BPR and M0 remain, shifted above z = 0.
  const auto& conductors = loaded.at("conductors").as_array();
  ASSERT_EQ(conductors.size(), 2u);
  EXPECT_EQ(conductors[0].as_object().at("name").as_string(), "BPR");
  EXPECT_NEAR(conductors[0].as_object().at("bottom").to_number<double>(),
              0.300,
              1e-6);
  EXPECT_EQ(conductors[1].as_object().at("name").as_string(), "M0");
  EXPECT_NEAR(conductors[1].as_object().at("bottom").to_number<double>(),
              0.390,
              1e-6);

  odb::dbDatabase::destroy(db);
}

TEST(FieldSolverHandlerTest, SyncsUpdatedTclItfStack)
{
  odb::dbDatabase* db = odb::dbDatabase::create();
  utl::Logger logger;
  fsv::FieldSolver field_solver(db, &logger);
  FieldSolverHandler handler(&field_solver, db);

  auto getItf = [&]() {
    WebSocketRequest req;
    req.id = 30;
    req.type = WebSocketRequest::kFsvGetItf;
    WebSocketResponse resp = handler.handleGetItf(req);
    const std::string body(resp.payload.begin(), resp.payload.end());
    return boost::json::parse(body).as_object();
  };

  // Initially no ITF loaded.
  EXPECT_FALSE(getItf().at("loaded").as_bool());

  // First Tcl read_itf is picked up.
  field_solver.readItf("openroad/fsv/test/sky130.itf");
  const auto first = getItf();
  ASSERT_TRUE(first.at("loaded").as_bool());
  EXPECT_EQ(first.at("technology").as_string(), "skywater130_nominal");

  // A subsequent Tcl read_itf replaces the cached session stack.
  field_solver.readItf("openroad/fsv/test/GT2.itf");
  const auto second = getItf();
  ASSERT_TRUE(second.at("loaded").as_bool());
  EXPECT_EQ(second.at("technology").as_string(), "GT2");
  EXPECT_GT(second.at("revision").to_number<uint64_t>(),
            first.at("revision").to_number<uint64_t>());

  // Switching back to uniform permittivity in Tcl clears the cached stack.
  field_solver.setPermittivity(3.9);
  EXPECT_FALSE(getItf().at("loaded").as_bool());

  odb::dbDatabase::destroy(db);
}

}  // namespace
}  // namespace web
