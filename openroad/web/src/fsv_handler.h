// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "fsv/Stack.h"
#include "request_handler.h"

namespace fsv {
class FieldSolver;
class Solver;
}  // namespace fsv

namespace odb {
class dbDatabase;
}  // namespace odb

namespace web {

class RequestDispatcher;

// Handles interactive 2D cross-section field solver requests (`fsv_solve_2d`,
// `fsv_get_itf`, `fsv_load_itf`, `fsv_clear_itf`).  Lazily initializes a
// shared fsv::Solver on first use so sessions that never open the Field Solver
// panel incur no thread-pool or Green's-table overhead.  If an ITF stack was
// loaded via Tcl `read_itf` on OpenROAD's `fsv::FieldSolver`, `fsv_get_itf`
// imports it automatically.  An ITF loaded here is reduced to its frontside
// the way `read_itf` does (see fsv::frontsideStack).
class FieldSolverHandler
{
 public:
  explicit FieldSolverHandler(
      fsv::FieldSolver* field_solver = nullptr,
      odb::dbDatabase* db = nullptr,
      std::shared_ptr<TclEvaluator> tcl_eval = nullptr);
  ~FieldSolverHandler();

  void registerRequests(RequestDispatcher& dispatcher);
  WebSocketResponse handleSolve2D(const WebSocketRequest& req);
  WebSocketResponse handleGetItf(const WebSocketRequest& req);
  WebSocketResponse handleLoadItf(const WebSocketRequest& req);
  WebSocketResponse handleClearItf(const WebSocketRequest& req);

 private:
  fsv::Solver& ensureSolver();
  // Caller must hold tcl_eval_->mutex (if non-null) and stack_mutex_.
  void syncTclStackLocked();

  fsv::FieldSolver* field_solver_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  // Serializes reads of field_solver_ and db_->getTech() against Tcl commands
  // (read_itf, set_field_solver_permittivity, read_lef).
  std::shared_ptr<TclEvaluator> tcl_eval_;

  std::mutex solver_mutex_;
  std::unique_ptr<fsv::Solver> solver_;

  std::mutex stack_mutex_;
  std::optional<fsv::Stack> stack_;
  std::string stack_source_;
  uint64_t synced_tcl_version_ = 0;
  uint64_t stack_revision_ = 0;
};

}  // namespace web
