// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "color.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "tcl.h"
#include "tile_generator.h"
#include "utl/Logger.h"

namespace web {

class TimingReport;
class ClockTreeReport;

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
    const int rc = Tcl_Eval(interp, cmd.c_str());
    Result r;
    r.output = logger->redirectStringEnd();
    r.result = Tcl_GetStringResult(interp);
    r.is_error = (rc != TCL_OK);
    return r;
  }
};

struct WebSocketRequest
{
  enum Type
  {
    TILE,
    BOUNDS,
    TECH,
    SELECT,
    INSPECT,
    INSPECT_BACK,
    HOVER,
    TCL_EVAL,
    TCL_COMPLETE,
    TIMING_REPORT,
    TIMING_HIGHLIGHT,
    CLOCK_TREE,
    CLOCK_TREE_HIGHLIGHT,
    SLACK_HISTOGRAM,
    CHART_FILTERS,
    MODULE_HIERARCHY,
    SET_MODULE_COLORS,
    SET_FOCUS_NETS,
    SET_ROUTE_GUIDES,
    HEATMAPS,
    SET_ACTIVE_HEATMAP,
    SET_HEATMAP,
    HEATMAP_TILE,
    LIST_DIR,
    SNAP,
    SCHEMATIC_CONE,
    SCHEMATIC_FULL,
    SCHEMATIC_INSPECT,
    UNKNOWN
  };

  uint32_t id = 0;
  Type type = UNKNOWN;
  std::string layer;
  int z = 0;
  int x = 0;
  int y = 0;

  // SELECT fields
  int select_x = 0;
  int select_y = 0;
  int select_zoom = 0;
  std::set<std::string> visible_layers;

  // INSPECT / HOVER fields
  int select_id = -1;

  // TCL_EVAL fields
  std::string tcl_cmd;

  // TCL_COMPLETE fields
  std::string tcl_complete_line;
  int tcl_complete_cursor_pos = -1;

  // SCHEMATIC_CONE / SCHEMATIC_INSPECT fields
  std::string schematic_inst_name;
  int schematic_fanin_depth = 1;
  int schematic_fanout_depth = 1;

  // TIMING_REPORT fields
  bool timing_is_setup = true;
  int timing_max_paths = 100;
  float timing_slack_min = -std::numeric_limits<float>::max();
  float timing_slack_max = std::numeric_limits<float>::max();

  // TIMING_HIGHLIGHT fields
  int timing_path_index = -1;  // -1 = clear
  bool timing_highlight_setup = true;
  std::string timing_pin_name;  // optional: highlight this pin's net in yellow

  // CLOCK_TREE_HIGHLIGHT fields
  std::string clock_tree_inst_name;

  // SLACK_HISTOGRAM fields
  bool histogram_is_setup = true;
  std::string histogram_path_group;
  std::string histogram_clock;

  // SET_FOCUS_NETS fields
  std::string focus_action;  // "add", "remove", "clear"
  std::string focus_net_name;

  // SET_ROUTE_GUIDES fields
  std::string route_guide_action;  // "add", "remove", "clear"
  std::string route_guide_net_name;

  // LIST_DIR fields
  std::string dir_path;

  // SNAP fields
  int snap_x = 0;
  int snap_y = 0;
  int snap_radius = 0;
  int snap_point_threshold = 10;
  bool snap_horizontal = true;
  bool snap_vertical = true;

  // Heat map fields
  std::string heatmap_name;
  std::string heatmap_option;
  std::string heatmap_string_value;
  std::string raw_json;

  // Visibility flags (default: all visible)
  TileVisibility vis;
};

struct WebSocketResponse
{
  uint32_t id = 0;
  // 0 = JSON payload, 1 = PNG payload, 2 = error
  uint8_t type = 0;
  std::vector<unsigned char> payload;
};

// Shared mutable state for a WebSocket session.
// Handlers receive a reference; WebSocketSession owns the instance.
struct SessionState
{
  std::mutex selection_mutex;
  std::vector<odb::Rect> highlight_rects;
  std::vector<odb::Polygon> highlight_polys;
  std::vector<odb::Rect> hover_rects;
  std::vector<ColoredRect> timing_rects;
  std::vector<FlightLine> timing_lines;

  std::mutex selectables_mutex;
  std::vector<gui::Selected> selectables;

  gui::Selected current_inspected;
  std::vector<gui::Selected> navigation_history;

  std::mutex module_colors_mutex;
  std::map<uint32_t, Color> module_colors;  // odb module id → RGBA color

  std::mutex focus_nets_mutex;
  std::set<uint32_t> focus_net_ids;  // dbNet ODB IDs

  std::mutex route_guides_mutex;
  std::set<uint32_t> route_guide_net_ids;  // dbNet ODB IDs

  std::mutex heatmap_mutex;
  std::map<std::string, std::shared_ptr<gui::HeatMapDataSource>> heatmaps;
  std::string active_heatmap;
};

// Minimal JSON field extraction (no JSON library dependency).
std::string extract_string(const std::string& json, const std::string& key);
int extract_int(const std::string& json, const std::string& key);
int extract_int_or(const std::string& json,
                   const std::string& key,
                   int default_val);
float extract_float_or(const std::string& json,
                       const std::string& key,
                       float default_val);
std::set<std::string> extract_string_array(const std::string& json,
                                           const std::string& key);

// Dispatch BOUNDS/LAYERS/TILE/INFO requests (used by HTTP and WebSocket).
WebSocketResponse dispatch_request(
    const WebSocketRequest& req,
    const TileGenerator& gen,
    const std::vector<odb::Rect>& highlight_rects = {},
    const std::vector<odb::Polygon>& highlight_polys = {},
    const std::vector<ColoredRect>& colored_rects = {},
    const std::vector<FlightLine>& flight_lines = {},
    const std::map<uint32_t, Color>* module_colors = nullptr,
    const std::set<uint32_t>* focus_net_ids = nullptr,
    const std::set<uint32_t>* route_guide_net_ids = nullptr);

// Handles SELECT, INSPECT, and HOVER requests.
class SelectHandler
{
 public:
  SelectHandler(std::shared_ptr<TileGenerator> gen,
                std::shared_ptr<TclEvaluator> tcl_eval);

  WebSocketResponse handleSelect(const WebSocketRequest& req,
                                 SessionState& state);
  WebSocketResponse handleInspect(const WebSocketRequest& req,
                                  SessionState& state);
  WebSocketResponse handleInspectBack(const WebSocketRequest& req,
                                      SessionState& state);
  WebSocketResponse handleHover(const WebSocketRequest& req,
                                SessionState& state);
  WebSocketResponse handleSetFocusNets(const WebSocketRequest& req,
                                       SessionState& state);
  WebSocketResponse handleSetRouteGuides(const WebSocketRequest& req,
                                         SessionState& state);
  WebSocketResponse handleSnap(const WebSocketRequest& req);
  WebSocketResponse handleSchematicCone(const WebSocketRequest& req);
  WebSocketResponse handleSchematicFull(const WebSocketRequest& req);
  WebSocketResponse handleSchematicInspect(const WebSocketRequest& req,
                                           SessionState& state);

 private:
  std::shared_ptr<TileGenerator> gen_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
};

// Handles TCL_EVAL requests.
class TclHandler
{
 public:
  explicit TclHandler(std::shared_ptr<TclEvaluator> tcl_eval);

  WebSocketResponse handleTclEval(const WebSocketRequest& req);
  WebSocketResponse handleTclComplete(const WebSocketRequest& req);

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

  WebSocketResponse handleTimingReport(const WebSocketRequest& req);
  WebSocketResponse handleTimingHighlight(const WebSocketRequest& req,
                                          SessionState& state);
  WebSocketResponse handleSlackHistogram(const WebSocketRequest& req);
  WebSocketResponse handleChartFilters(const WebSocketRequest& req);

 private:
  std::shared_ptr<TileGenerator> gen_;
  std::shared_ptr<TimingReport> timing_report_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
};

// Handles CLOCK_TREE and CLOCK_TREE_HIGHLIGHT requests.
class ClockTreeHandler
{
 public:
  ClockTreeHandler(std::shared_ptr<TileGenerator> gen,
                   std::shared_ptr<ClockTreeReport> clock_report,
                   std::shared_ptr<TclEvaluator> tcl_eval);

  WebSocketResponse handleClockTree(const WebSocketRequest& req);
  WebSocketResponse handleClockTreeHighlight(const WebSocketRequest& req,
                                             SessionState& state);

 private:
  std::shared_ptr<TileGenerator> gen_;
  std::shared_ptr<ClockTreeReport> clock_report_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
};

// Handles TILE/BOUNDS/TECH requests.
class TileHandler
{
 public:
  explicit TileHandler(std::shared_ptr<TileGenerator> gen);

  void initializeHeatMaps(SessionState& state);
  WebSocketResponse handleTile(const WebSocketRequest& req,
                               SessionState& state);
  WebSocketResponse handleModuleHierarchy(const WebSocketRequest& req);
  WebSocketResponse handleSetModuleColors(const WebSocketRequest& req,
                                          SessionState& state);
  WebSocketResponse handleHeatMaps(const WebSocketRequest& req,
                                   SessionState& state);
  WebSocketResponse handleSetActiveHeatMap(const WebSocketRequest& req,
                                           SessionState& state);
  WebSocketResponse handleSetHeatMap(const WebSocketRequest& req,
                                     SessionState& state);
  WebSocketResponse handleHeatMapTile(const WebSocketRequest& req,
                                      SessionState& state);

 private:
  std::shared_ptr<TileGenerator> gen_;
};

// Handles LIST_DIR requests (server-side file browsing).
WebSocketResponse handleListDir(const WebSocketRequest& req);

}  // namespace web
