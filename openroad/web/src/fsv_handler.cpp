// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv_handler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "boost/json/array.hpp"
#include "boost/json/object.hpp"
#include "boost/json/serialize.hpp"
#include "boost/json/value.hpp"
#include "fsv/FieldSolver.h"
#include "fsv/Geometry.h"
#include "fsv/Itf.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"
#include "fsv/Stack.h"
#include "odb/db.h"
#include "request_dispatcher.h"

namespace web {

namespace {

constexpr int kMaxWires = 64;
constexpr int kMinBins = 4;
constexpr int kMaxBins = 100;
constexpr int kDefaultBins = 20;
constexpr double kDefaultEps = 3.9;
constexpr double kDefaultTol = 0.01;
constexpr int64_t kDefaultMinWalks = 65536;
constexpr int64_t kDefaultMaxWalks = 500000;

struct Solve2DParams
{
  bool use_itf = false;
  double eps = kDefaultEps;
  int target_index = 0;
  int bins = kDefaultBins;
  int ground_bins = 0;
  double ground_x_lo = 0.0;
  double ground_x_hi = 0.0;
  double tol_total = kDefaultTol;
  int64_t min_walks = kDefaultMinWalks;
  int64_t max_walks = kDefaultMaxWalks;
  fsv::Geometry2D geometry;
  std::vector<std::string> names;
};

WebSocketResponse jsonResponse(const uint32_t id,
                               const boost::json::object& obj)
{
  WebSocketResponse resp;
  resp.id = id;
  resp.type = WebSocketResponse::kJson;
  const std::string body = boost::json::serialize(obj);
  resp.payload.assign(body.begin(), body.end());
  return resp;
}

WebSocketResponse jsonError(const uint32_t id, const std::string& msg)
{
  boost::json::object obj;
  obj["error"] = msg;
  return jsonResponse(id, obj);
}

double toDouble(const boost::json::value& v)
{
  return v.to_number<double>();
}

boost::json::array toJsonArray(const std::vector<double>& vals)
{
  boost::json::array arr;
  arr.reserve(vals.size());
  for (const double v : vals) {
    arr.emplace_back(std::isfinite(v) ? v : 0.0);
  }
  return arr;
}

boost::json::object serializeStack(const fsv::Stack& stack,
                                   const std::string& source,
                                   const std::vector<fsv::ItfWarning>& warnings)
{
  boost::json::object root;
  root["technology"] = stack.technology();
  root["source"] = source;
  root["background_eps"] = stack.backgroundEps();

  boost::json::array dielectrics;
  for (const auto& d : stack.dielectrics()) {
    boost::json::object obj;
    obj["name"] = d.name;
    obj["bottom"] = d.bottom;
    obj["top"] = d.top;
    obj["eps"] = d.eps;
    dielectrics.emplace_back(std::move(obj));
  }
  root["dielectrics"] = std::move(dielectrics);

  boost::json::array conductors;
  for (const auto& c : stack.conductors()) {
    // Conductors sitting on the ground plane (bottom <= 0) cannot be placed
    // as floating 2D wires above z = 0.
    if (c.bottom <= 0.0) {
      continue;
    }
    boost::json::object obj;
    obj["name"] = c.name;
    obj["bottom"] = c.bottom;
    obj["top"] = c.top;
    obj["thickness"] = c.top - c.bottom;
    obj["min_width"] = c.min_width;
    obj["min_spacing"] = c.min_spacing;

    boost::json::array conformals;
    for (const auto& conf : c.conformals) {
      boost::json::object co;
      co["name"] = conf.name;
      co["side_thickness"] = conf.side_thickness;
      co["top_thickness"] = conf.top_thickness;
      co["eps"] = conf.eps;
      conformals.emplace_back(std::move(co));
    }
    obj["conformals"] = std::move(conformals);
    conductors.emplace_back(std::move(obj));
  }
  root["conductors"] = std::move(conductors);

  boost::json::array warns;
  for (const auto& w : warnings) {
    warns.emplace_back("line " + std::to_string(w.line) + ": " + w.message);
  }
  root["warnings"] = std::move(warns);

  return root;
}

std::optional<std::string> parseSolve2DOptions(const boost::json::object& obj,
                                               const int wire_count,
                                               Solve2DParams& out)
{
  if (const auto* v = obj.if_contains("use_itf")) {
    out.use_itf = v->as_bool();
  }
  if (const auto* v = obj.if_contains("eps")) {
    out.eps = toDouble(*v);
  }
  if (!out.use_itf && (!std::isfinite(out.eps) || out.eps <= 0.0)) {
    return "Permittivity (eps) must be positive.";
  }
  if (const auto* v = obj.if_contains("target_index")) {
    out.target_index = static_cast<int>(v->as_int64());
  }
  if (out.target_index < 0 || out.target_index >= wire_count) {
    return "target_index out of range.";
  }
  if (const auto* v = obj.if_contains("bins")) {
    out.bins = std::clamp(static_cast<int>(v->as_int64()), kMinBins, kMaxBins);
  }
  if (const auto* v = obj.if_contains("ground_bins")) {
    out.ground_bins = std::clamp(static_cast<int>(v->as_int64()), 0, 200);
  }
  if (const auto* v = obj.if_contains("ground_x_lo")) {
    out.ground_x_lo = toDouble(*v);
  }
  if (const auto* v = obj.if_contains("ground_x_hi")) {
    out.ground_x_hi = toDouble(*v);
  }
  if (out.ground_bins > 0
      && (!std::isfinite(out.ground_x_lo) || !std::isfinite(out.ground_x_hi)
          || !(out.ground_x_hi > out.ground_x_lo))) {
    out.ground_bins = 0;
  }
  if (const auto* v = obj.if_contains("tol_total")) {
    out.tol_total = std::clamp(toDouble(*v), 1e-4, 0.2);
  }
  if (const auto* v = obj.if_contains("min_walks")) {
    out.min_walks = std::clamp<int64_t>(v->as_int64(), 1024, 2000000);
  }
  if (const auto* v = obj.if_contains("max_walks")) {
    out.max_walks
        = std::clamp<int64_t>(v->as_int64(), out.min_walks, 5000000);
  } else {
    out.max_walks = std::max(out.max_walks, out.min_walks);
  }
  return std::nullopt;
}

std::optional<std::string> parseSolve2DRequest(const boost::json::object& obj,
                                               const fsv::Stack* stack,
                                               Solve2DParams& out)
{
  const auto* wires_val = obj.if_contains("wires");
  if (!wires_val || !wires_val->is_array()) {
    return "Missing or invalid 'wires' array.";
  }
  const boost::json::array& wires = wires_val->as_array();
  if (wires.empty()) {
    return "At least one wire is required.";
  }
  if (static_cast<int>(wires.size()) > kMaxWires) {
    return "Too many wires (maximum 64).";
  }
  if (auto err = parseSolve2DOptions(obj, static_cast<int>(wires.size()), out)) {
    return err;
  }

  out.names.reserve(wires.size());
  for (size_t i = 0; i < wires.size(); i++) {
    if (!wires[i].is_object()) {
      return "Each wire must be an object.";
    }
    const boost::json::object& w = wires[i].as_object();
    std::string name;
    if (const auto* n = w.if_contains("name"); n && n->is_string()) {
      name = std::string(n->as_string());
    } else {
      name = "wire_" + std::to_string(i);
    }

    const double cx = toDouble(w.at("x"));
    double z0 = toDouble(w.at("z"));
    const double width = toDouble(w.at("width"));
    double z1 = z0 + toDouble(w.at("height"));

    // When solving against an ITF stack and a wire names its stack layer, take
    // the exact layer z extent directly from the stack so conformal dielectrics
    // (matched within 1e-9 um) are never silently lost to rounding.
    if (stack != nullptr) {
      if (const auto* l = w.if_contains("layer"); l && l->is_string()) {
        const auto handle = stack->layer(std::string_view(l->as_string()));
        if (handle.valid()) {
          const auto ext = stack->zExtent(handle);
          z0 = ext.bottom;
          z1 = ext.top;
        }
      }
    }

    if (!std::isfinite(cx) || !std::isfinite(z0) || !std::isfinite(width)
        || !std::isfinite(z1)) {
      return "Wire '" + name + "' has non-finite geometry.";
    }
    if (width <= 0.0 || z1 <= z0) {
      return "Wire '" + name + "' must have positive width and height.";
    }
    if (z0 <= 0.0) {
      return "Wire '" + name + "' must sit above the ground plane (z > 0).";
    }

    const int net = out.geometry.addNet(name);
    out.geometry.addRect(net, {{cx - 0.5 * width, z0}, {cx + 0.5 * width, z1}});
    out.names.push_back(std::move(name));
  }
  return std::nullopt;
}

boost::json::object serializeSolve2DResult(const fsv::CapRow& row,
                                           const Solve2DParams& params,
                                           const double seconds)
{
  boost::json::object root;
  root["total"] = row.total();
  root["total_sigma"] = row.sigma[params.target_index];
  root["ground"] = -row.ground;
  root["ground_sigma"] = row.ground_sigma;
  root["walks"] = row.walks;
  root["hops"] = row.hops;
  root["seconds"] = seconds;
  root["target_index"] = params.target_index;

  boost::json::array couplings;
  for (int i = 0; i < params.geometry.netCount(); i++) {
    if (i == params.target_index) {
      continue;
    }
    boost::json::object c;
    c["index"] = i;
    c["name"] = params.names[i];
    c["cap"] = row.coupling(i);
    c["sigma"] = row.sigma[i];
    couplings.emplace_back(std::move(c));
  }
  root["couplings"] = std::move(couplings);

  boost::json::array charges;
  charges.reserve(row.rect_charges.size());
  for (size_t i = 0; i < row.rect_charges.size(); i++) {
    const auto& rc = row.rect_charges[i];
    boost::json::object ch;
    ch["index"] = static_cast<int>(i);
    ch["name"] = params.names[i];
    ch["bottom"] = toJsonArray(rc.bottom);
    ch["top"] = toJsonArray(rc.top);
    ch["left"] = toJsonArray(rc.left);
    ch["right"] = toJsonArray(rc.right);
    charges.emplace_back(std::move(ch));
  }
  root["charges"] = std::move(charges);

  if (!row.ground_charge.empty()) {
    boost::json::object gch;
    gch["x_lo"] = params.ground_x_lo;
    gch["x_hi"] = params.ground_x_hi;
    gch["values"] = toJsonArray(row.ground_charge);
    root["ground_charge"] = std::move(gch);
  }
  return root;
}

}  // namespace

FieldSolverHandler::FieldSolverHandler(fsv::FieldSolver* field_solver,
                                       odb::dbDatabase* db,
                                       std::shared_ptr<TclEvaluator> tcl_eval)
    : field_solver_(field_solver), db_(db), tcl_eval_(std::move(tcl_eval))
{
}
FieldSolverHandler::~FieldSolverHandler() = default;

fsv::Solver& FieldSolverHandler::ensureSolver()
{
  const std::lock_guard<std::mutex> lock(solver_mutex_);
  if (!solver_) {
    const unsigned hw = std::thread::hardware_concurrency();
    const int threads = std::clamp(static_cast<int>(hw > 0 ? hw : 4), 1, 8);
    solver_ = std::make_unique<fsv::Solver>(
        fsv::SolverOptions{.threads = threads});
  }
  return *solver_;
}

void FieldSolverHandler::registerRequests(RequestDispatcher& dispatcher)
{
  dispatcher.add("fsv_solve_2d",
                 WebSocketRequest::kFsvSolve2D,
                 [this](const WebSocketRequest& req, SessionState&) {
                   return handleSolve2D(req);
                 });
  dispatcher.add("fsv_get_itf",
                 WebSocketRequest::kFsvGetItf,
                 [this](const WebSocketRequest& req, SessionState&) {
                   return handleGetItf(req);
                 });
  dispatcher.add("fsv_load_itf",
                 WebSocketRequest::kFsvLoadItf,
                 [this](const WebSocketRequest& req, SessionState&) {
                   return handleLoadItf(req);
                 });
  dispatcher.add("fsv_clear_itf",
                 WebSocketRequest::kFsvClearItf,
                 [this](const WebSocketRequest& req, SessionState&) {
                   return handleClearItf(req);
                 });
}

void FieldSolverHandler::syncTclStackLocked()
{
  if (field_solver_ == nullptr) {
    return;
  }
  const uint64_t ver = field_solver_->stackVersion();
  if (ver == synced_tcl_version_) {
    return;
  }
  synced_tcl_version_ = ver;
  ++stack_revision_;
  if (field_solver_->hasStack()) {
    stack_ = field_solver_->stack();
    stack_source_ = field_solver_->itfPath();
  } else {
    stack_.reset();
    stack_source_.clear();
  }
}

WebSocketResponse FieldSolverHandler::handleGetItf(const WebSocketRequest& req)
{
  // Lock tcl_eval_->mutex before stack_mutex_ so field_solver_ reads are
  // serialized against concurrent Tcl read_itf / set_field_solver_permittivity.
  std::unique_lock<std::mutex> tcl_lock;
  if (tcl_eval_) {
    tcl_lock = std::unique_lock<std::mutex>(tcl_eval_->mutex);
  }
  const std::lock_guard<std::mutex> lock(stack_mutex_);
  syncTclStackLocked();
  if (!stack_) {
    boost::json::object obj;
    obj["loaded"] = false;
    obj["revision"] = stack_revision_;
    return jsonResponse(req.id, obj);
  }
  boost::json::object resp = serializeStack(*stack_, stack_source_, {});
  if (resp.at("conductors").as_array().empty()) {
    boost::json::object obj;
    obj["loaded"] = false;
    obj["revision"] = stack_revision_;
    return jsonResponse(req.id, obj);
  }
  resp["loaded"] = true;
  resp["revision"] = stack_revision_;
  return jsonResponse(req.id, resp);
}

WebSocketResponse FieldSolverHandler::handleLoadItf(const WebSocketRequest& req)
{
  const boost::json::object& obj = req.json;
  std::vector<fsv::ItfWarning> warnings;
  try {
    fsv::Stack stack;
    std::string source;
    if (const auto* content = obj.if_contains("content");
        content && content->is_string()) {
      std::istringstream iss(std::string(content->as_string()));
      stack = fsv::readItf(iss, &warnings);
      if (const auto* name = obj.if_contains("name");
          name && name->is_string()) {
        source = std::string(name->as_string());
      } else {
        source = "uploaded.itf";
      }
    } else if (const auto* path = obj.if_contains("path");
               path && path->is_string()) {
      source = std::string(path->as_string());
      stack = fsv::readItfFile(source, &warnings);
    } else {
      return jsonError(req.id, "Provide either 'path' or 'content' for ITF.");
    }

    // Lock tcl_eval_->mutex before stack_mutex_ to read db_->getTech() and
    // record the current Tcl stackVersion() atomically with the cache update.
    std::unique_lock<std::mutex> tcl_lock;
    if (tcl_eval_) {
      tcl_lock = std::unique_lock<std::mutex>(tcl_eval_->mutex);
    }
    stack = fsv::frontsideStack(stack,
                                db_ != nullptr ? db_->getTech() : nullptr);
    boost::json::object resp = serializeStack(stack, source, warnings);
    if (resp.at("conductors").as_array().empty()) {
      return jsonError(
          req.id,
          "ITF stack contains no conductor layers above the ground plane.");
    }
    {
      const std::lock_guard<std::mutex> lock(stack_mutex_);
      stack_ = std::move(stack);
      stack_source_ = source;
      if (field_solver_ != nullptr) {
        synced_tcl_version_ = field_solver_->stackVersion();
      }
      ++stack_revision_;
      resp["revision"] = stack_revision_;
    }
    return jsonResponse(req.id, resp);
  } catch (const std::exception& e) {
    return jsonError(req.id, e.what());
  }
}

WebSocketResponse FieldSolverHandler::handleClearItf(
    const WebSocketRequest& req)
{
  std::unique_lock<std::mutex> tcl_lock;
  if (tcl_eval_) {
    tcl_lock = std::unique_lock<std::mutex>(tcl_eval_->mutex);
  }
  const std::lock_guard<std::mutex> lock(stack_mutex_);
  stack_.reset();
  stack_source_.clear();
  if (field_solver_ != nullptr) {
    synced_tcl_version_ = field_solver_->stackVersion();
  }
  ++stack_revision_;
  boost::json::object obj;
  obj["ok"] = true;
  obj["revision"] = stack_revision_;
  return jsonResponse(req.id, obj);
}

WebSocketResponse FieldSolverHandler::handleSolve2D(const WebSocketRequest& req)
{
  bool use_itf = false;
  if (const auto* v = req.json.if_contains("use_itf")) {
    use_itf = v->as_bool();
  }

  std::optional<fsv::Stack> stack_copy;
  if (use_itf) {
    std::unique_lock<std::mutex> tcl_lock;
    if (tcl_eval_) {
      tcl_lock = std::unique_lock<std::mutex>(tcl_eval_->mutex);
    }
    const std::lock_guard<std::mutex> lock(stack_mutex_);
    syncTclStackLocked();
    if (!stack_) {
      return jsonError(req.id, "No ITF stack loaded.");
    }
    stack_copy = *stack_;
  }

  Solve2DParams params;
  if (auto err = parseSolve2DRequest(
          req.json, stack_copy ? &*stack_copy : nullptr, params)) {
    return jsonError(req.id, *err);
  }

  try {
    const auto t0 = std::chrono::steady_clock::now();
    std::unique_ptr<fsv::Scene2D> scene
        = params.use_itf
              ? std::make_unique<fsv::Scene2D>(params.geometry, *stack_copy)
              : std::make_unique<fsv::Scene2D>(params.geometry, params.eps);

    fsv::SolveOptions opts;
    opts.tol_total = params.tol_total;
    opts.min_walks = params.min_walks;
    opts.max_walks = params.max_walks;
    opts.surface_charge_bins = params.bins;
    if (params.ground_bins > 0) {
      opts.ground_charge_bins = params.ground_bins;
      opts.ground_charge_x_lo = params.ground_x_lo;
      opts.ground_charge_x_hi = params.ground_x_hi;
    }

    const fsv::CapRow row
        = ensureSolver().solve(*scene, params.target_index, opts);
    const double seconds
        = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0)
              .count();
    return jsonResponse(req.id, serializeSolve2DResult(row, params, seconds));
  } catch (const std::exception& e) {
    return jsonError(req.id, e.what());
  }
}

}  // namespace web
