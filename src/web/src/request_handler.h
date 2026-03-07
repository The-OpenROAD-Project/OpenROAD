// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "gui/gui.h"
#include "odb/db.h"
#include "tcl.h"
#include "tile_generator.h"
#include "utl/Logger.h"

namespace web {

class TimingReport;

// Thread-safe Tcl command evaluation with output capture.
struct TclEvaluator
{
  Tcl_Interp* interp;
  utl::Logger* logger;
  std::mutex mutex;

  struct Result
  {
    std::string output;
    std::string result;
    bool is_error;
  };

  TclEvaluator(Tcl_Interp* interp, utl::Logger* logger)
      : interp(interp), logger(logger)
  {
  }

  Result eval(const std::string& cmd)
  {
    std::lock_guard<std::mutex> lock(mutex);
    logger->redirectStringBegin();
    int rc = Tcl_Eval(interp, cmd.c_str());
    Result r;
    r.output = logger->redirectStringEnd();
    r.result = Tcl_GetStringResult(interp);
    r.is_error = (rc != TCL_OK);
    return r;
  }
};

struct WsRequest
{
  uint32_t id = 0;
  enum Type
  {
    TILE,
    BOUNDS,
    LAYERS,
    INFO,
    SELECT,
    INSPECT,
    HOVER,
    TCL_EVAL,
    TIMING_REPORT,
    TIMING_HIGHLIGHT,
    UNKNOWN
  } type
      = UNKNOWN;
  std::string layer;
  int z = 0;
  int x = 0;
  int y = 0;

  // SELECT fields
  int select_x = 0;
  int select_y = 0;
  int select_zoom = 0;

  // INSPECT / HOVER fields
  int select_id = -1;

  // TCL_EVAL fields
  std::string tcl_cmd;

  // TIMING_REPORT fields
  bool timing_is_setup = true;
  int timing_max_paths = 100;

  // TIMING_HIGHLIGHT fields
  int timing_path_index = -1;  // -1 = clear
  bool timing_highlight_setup = true;
  std::string timing_pin_name;  // optional: highlight this pin's net in yellow

  // Visibility flags (default: all visible)
  TileVisibility vis;
};

struct WsResponse
{
  uint32_t id = 0;
  // 0 = JSON payload, 1 = PNG payload, 2 = error
  uint8_t type = 0;
  std::vector<unsigned char> payload;
};

// Shared mutable state for a WebSocket session.
// Handlers receive a reference; WsSession owns the instance.
struct SessionState
{
  std::mutex selection_mutex;
  std::vector<odb::Rect> highlight_rects;
  std::vector<ColoredRect> timing_rects;
  std::vector<FlightLine> timing_lines;

  std::mutex selectables_mutex;
  std::vector<gui::Selected> selectables;
};

// Minimal JSON field extraction (no JSON library dependency).
std::string extract_string(const std::string& json, const std::string& key);
int extract_int(const std::string& json, const std::string& key);
int extract_int_or(const std::string& json,
                   const std::string& key,
                   int default_val);

// Escape a string for safe inclusion in a JSON string value.
std::string json_escape(const std::string& s);

// Dispatch BOUNDS/LAYERS/TILE/INFO requests (used by HTTP and WebSocket).
WsResponse dispatch_request(const WsRequest& req,
                            const std::shared_ptr<TileGenerator>& gen,
                            const std::vector<odb::Rect>& highlight_rects = {},
                            const std::vector<ColoredRect>& colored_rects = {},
                            const std::vector<FlightLine>& flight_lines = {});

// Handles SELECT, INSPECT, and HOVER requests.
class SelectHandler
{
 public:
  explicit SelectHandler(std::shared_ptr<TileGenerator> gen);

  WsResponse handleSelect(const WsRequest& req, SessionState& state);
  WsResponse handleInspect(const WsRequest& req, SessionState& state);
  WsResponse handleHover(const WsRequest& req, SessionState& state);

 private:
  std::shared_ptr<TileGenerator> gen_;
};

// Handles TCL_EVAL requests.
class TclHandler
{
 public:
  explicit TclHandler(std::shared_ptr<TclEvaluator> tcl_eval);

  WsResponse handleTclEval(const WsRequest& req);

 private:
  std::shared_ptr<TclEvaluator> tcl_eval_;
};

// Handles TIMING_REPORT and TIMING_HIGHLIGHT requests.
class TimingHandler
{
 public:
  TimingHandler(std::shared_ptr<TileGenerator> gen,
                std::shared_ptr<TimingReport> timing_report,
                std::shared_ptr<TclEvaluator> tcl_eval);

  WsResponse handleTimingReport(const WsRequest& req);
  WsResponse handleTimingHighlight(const WsRequest& req, SessionState& state);

 private:
  std::shared_ptr<TileGenerator> gen_;
  std::shared_ptr<TimingReport> timing_report_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
};

// Handles TILE/BOUNDS/LAYERS/INFO requests.
class TileHandler
{
 public:
  explicit TileHandler(std::shared_ptr<TileGenerator> gen);

  WsResponse handleTile(const WsRequest& req, SessionState& state);

 private:
  std::shared_ptr<TileGenerator> gen_;
};

}  // namespace web
