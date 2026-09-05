// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "boost/json/object.hpp"
#include "boost/json/value.hpp"
#include "boost/json/value_to.hpp"
#include "color.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "tcl.h"
#include "tile_generator.h"
#include "utl/Logger.h"

namespace web {

class RequestDispatcher;
class TimingReport;
class ClockTreeReport;

// Sentinel string set as the Tcl result by WebServer::tclExitHandler
// when the browser-side Tcl `exit`/`quit` is invoked.  TclHandler
// detects this in handleTclEval and converts the response to a clean
// shutdown signal for the browser.
inline constexpr const char* kExitResultMsg = "_WEB_EXITING_";

// Thread-safe Tcl command evaluation.  Log output emitted while the
// command runs is captured by WebLogSink (registered on the logger via
// addSink) and pushed to clients as {"type":"log",...} messages — do
// NOT redirect the logger to a string here.  redirectStringBegin clears
// the entire sink list, which would unhook WebLogSink (and any other
// sink) for the duration of the command and break log streaming.  After
// each eval the optional drain_output hook is invoked so any buffered
// log output reaches clients before the eval response is sent.
struct TclEvaluator
{
  Tcl_Interp* interp;
  utl::Logger* logger;
  std::mutex mutex;
  std::function<void()> drain_output;

  struct Result
  {
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
    const int rc = Tcl_Eval(interp, cmd.c_str());
    Result r;
    r.result = Tcl_GetStringResult(interp);
    r.is_error = (rc != TCL_OK);
    // Flush Tcl stdout/stderr so `puts` output — which sta::ReportTcl
    // encapsulates into utl::Logger — reaches WebLogSink and the browser
    // console.  The web eval path has no Tcl event loop to flush the channel
    // buffer like the CLI/GUI do, so a `puts` would otherwise never surface.
    if (Tcl_Channel out = Tcl_GetStdChannel(TCL_STDOUT)) {
      Tcl_Flush(out);
    }
    if (Tcl_Channel err = Tcl_GetStdChannel(TCL_STDERR)) {
      Tcl_Flush(err);
    }
    if (drain_output) {
      drain_output();
    }
    return r;
  }
};

struct WebSocketRequest
{
  enum Type
  {
    kTile,
    kBounds,
    kTech,
    kSelect,
    kInspect,
    kInspectBack,
    kHover,
    kTclEval,
    kTclComplete,
    kTimingReport,
    kTimingHighlight,
    kTimingCone,
    kClockTree,
    kClockTreeHighlight,
    kSlackHistogram,
    kFanoutHistogram,
    kNetLengthHistogram,
    kSelectFanoutBin,
    kSelectNetLengthBin,
    kFind,
    kChartFilters,
    kModuleHierarchy,
    kSetModuleColors,
    kSetFocusNets,
    kSetRouteGuides,
    kHeatmaps,
    kSetActiveHeatmap,
    kSetHeatmap,
    kHeatmapTile,
    kListDir,
    kSnap,
    kSchematicCone,
    kSchematicFull,
    kSchematicInspect,
    kDrcCategories,
    kDrcMarkers,
    kDrcLoadReport,
    kDrcUpdateMarker,
    kDrcUpdateCategoryVisibility,
    kDrcHighlight,
    kSelectNext,
    kSelectPrev,
    kSetProperty,
    kTriggerAction,
    kHighlight,
    kUnhighlight,
    kClearHighlights,
    kListSelection,
    kInspectSelection,
    kInspectGroup,
    kDeselect,
    kSelectLayer,
    kDebugContinue,
    kDebugCharts,
    kSetDisplayState,
    kGet3DData,
    kOverlayTile,
    kAddLabel,
    kDeleteLabel,
    kUpdateLabel,
    kClearLabels,
    kListLabels,
    kContextAction,
    kCancel,
    kCustomUi,
    kGlobalConnectInfo,
    kGlobalConnectDelete,
    kGlobalConnectApply,
    kBufferInfo,
    kInsertBuffer,
    kPolyDecomp,
    kRendererControls,
    kSetRendererControl,
    kUnknown
  };

  uint32_t id = 0;
  Type type = kUnknown;
  boost::json::object json;  // parsed payload; empty on parse failure
  // Original `"type"` string from the JSON, even when not registered.
  // Used by the kUnknown error path for diagnosability.  Empty when
  // the message was malformed (parse threw) or had no `type` field.
  std::string raw_type;
  // Set to the boost::json exception message when JSON parsing or one
  // of the required envelope reads (id/type) failed.  Surfaced in the
  // kUnknown error payload so WEB-0043 names the actual parse error.
  std::string parse_error;
};

struct WebSocketResponse
{
  enum PayloadType : uint8_t
  {
    kJson = 0,
    kPng = 1,
    kError = 2
  };

  uint32_t id = 0;
  PayloadType type = kJson;
  std::vector<unsigned char> payload;
  // Original `"type"` string from the request, used by the kError
  // logging path for diagnosability.  Annotated by WebSocketSession::on_read
  // after the handler returns; handlers do not need to set it.
  std::string request_type;
};

// Shared mutable state for a WebSocket session.
// Handlers receive a reference; WebSocketSession owns the instance.
struct SessionState
{
  // Raised by the session's odb destroy callbacks when any selectable
  // object type is destroyed (by trigger_action or a Tcl command).  The
  // Selected wrappers below hold raw odb pointers, so the whole selection
  // state must be dropped before the next dereference; handlers consume
  // the flag via consumeStaleSelection() before touching any Selected.
  std::atomic<bool> selection_stale{false};

  // Raised by the session's odb geometry callbacks when a placement or
  // master swap moves an object (a set_property X/Y edit, or a Tcl command).
  // The highlight groups hold shapes derived from the old geometry, and the
  // edit can come from ANOTHER session -- whose broadcast only asks this
  // client to redraw, which would re-send the stale rectangles.  The overlay
  // handler rebuilds them when it sees this set.  Not folded into
  // selection_stale: nothing here dangles, so the selection itself must
  // survive.
  std::atomic<bool> highlight_geometry_stale{false};

  std::mutex selection_mutex;
  std::vector<odb::Rect> highlight_rects;
  std::vector<odb::Polygon> highlight_polys;
  // Flight lines emitted by selection highlights (unrouted nets draw
  // driver→sink lines); rendered in the overlay next to timing lines.
  std::vector<FlightLine> highlight_lines;
  std::vector<odb::Rect> hover_rects;
  std::vector<ColoredRect> timing_rects;
  std::vector<FlightLine> timing_lines;
  // Misc > Flywires only: highlight selected nets with straight
  // driver->sink lines instead of their routed wire/guides (GUI
  // isFlywireHighlightOnly() parity).
  bool flywires_only = false;
  // Last Options > "Show polygon decomposition" value this session derived
  // its highlight shapes under.  The setting itself is server-global (it
  // lives in gui::Gui, where the ITerm/MTerm descriptors read it); this copy
  // exists only so the overlay handler can spot that the shapes it holds
  // predate a change and re-derive them, exactly as it does for
  // flywires_only.
  bool poly_decomp = false;
  // Which selection the highlight_* vectors were derived from, or kNone while
  // they hold nothing.  A flywires_only flip has to re-derive them from the
  // SAME source: the multi-selection normally, but a single object when the
  // user followed an inspector link out of the selection set (handleInspect
  // deliberately narrows the highlight to the link target).  kNone doubles as
  // the "dismissed" state, so a flip cannot resurrect highlights the user
  // cleared.
  //
  // Tracking the source is a shim over the fact that selection_set and
  // current_inspected are two overlapping answers to "what is selected"; the
  // Qt GUI keeps only the set and narrows it on a link follow.
  enum class HighlightSource : uint8_t
  {
    kNone,
    kInspected,
    kSelectionSet
  };
  HighlightSource highlight_source = HighlightSource::kNone;

  std::mutex selectables_mutex;
  std::vector<gui::Selected> selectables;

  gui::Selected current_inspected;
  std::vector<gui::Selected> navigation_history;

  // Multi-selection set and iterator position (mirrors Qt GUI's SelectionSet).
  gui::SelectionSet selection_set;
  gui::SelectionSet::const_iterator selection_itr = selection_set.end();

  // Color-coded highlight groups (mirrors Qt GUI's HighlightSet: 16 fixed
  // groups colored by gui::Painter::kHighlightColors).  An object lives in
  // at most one group.  highlight_group_rects is the derived overlay
  // snapshot, rebuilt on every mutation (not per tile).  Both guarded by
  // selection_mutex.
  std::array<gui::SelectionSet, gui::kNumHighlightSet> highlight_groups;
  std::vector<ColoredRect> highlight_group_rects;
  // Octilinear group members (special-wire shapes) keep their outline rather
  // than collapsing to a bounding rect.  Guarded by selection_mutex, rebuilt
  // with highlight_group_rects.
  std::vector<ColoredPolygon> highlight_group_polys;
  // Flight lines emitted by group members whose highlight() draws lines
  // (e.g. unrouted nets), tinted with the group color.  Guarded by
  // selection_mutex, rebuilt with highlight_group_rects.
  std::vector<FlightLine> highlight_group_lines;

  std::mutex module_colors_mutex;
  std::map<uint32_t, Color> module_colors;  // odb module id → RGBA color

  std::mutex focus_nets_mutex;
  std::set<uint32_t> focus_net_ids;  // dbNet ODB IDs

  std::mutex route_guides_mutex;
  std::set<uint32_t> route_guide_net_ids;  // dbNet ODB IDs

  std::mutex drc_mutex;
  std::string active_drc_category;     // name of active top-level category
  std::vector<ColoredRect> drc_rects;  // filled rect shapes for overlay
  std::vector<FlightLine> drc_lines;   // line/X shapes for overlay

  // Timing-cone overlay (fanin/fanout).  cone_rects highlights instances,
  // cone_lines are the slack-colored flight lines and cone_labels are the
  // per-pin logic-depth annotations.  Populated by handleTimingCone and merged
  // into the overlay by handleOverlayTile.
  std::mutex cone_mutex;
  std::vector<ColoredRect> cone_rects;
  std::vector<FlightLine> cone_lines;
  std::vector<TextLabel> cone_labels;

  std::mutex heatmap_mutex;
  std::map<std::string, std::shared_ptr<gui::HeatMapDataSource>> heatmaps;
  std::string active_heatmap;

  // Tile-request ids the client has abandoned (pan/zoom away).  Populated by
  // the inline `cancel` handler and consumed at the top of handleTile so a
  // still-queued render is skipped.  Best-effort (a render already running on
  // a worker thread is not interrupted).
  std::mutex cancelled_mutex;
  std::set<uint32_t> cancelled_ids;
};

// Map an HTTP request target onto an embedded asset path.
//
// Strips the query string and fragment, which the asset lookup must not see:
// the viewer's options are passed as query parameters (?mergetiles=0 and
// friends), and matching "/?mergetiles=0" against the asset table simply fails,
// so the whole page 404s.  Also maps "/" onto the index document.
std::string assetPathFromTarget(std::string_view target);

// Optional-field accessor: returns the JSON value at `key` converted to T,
// or `default_val` when the key is missing.  Throws
// (boost::system::system_error) when the key is present but the JSON type
// doesn't convert to T — that's a frontend/backend contract violation, surface
// it.
//
// For required fields, prefer the bare boost::json idiom
// `obj.at(key).as_int64()` / `as_string()` / `as_bool()` / `as_double()`,
// which throws on either missing or wrong-typed input.
//
// Throwing is safe here: WebSocketSession dispatches every handler through
// invoke_handler(), which turns an escaped exception into an error response
// for that one request.  It was not always so — the handlers ran with no
// try/catch above them and out of io_context::run(), so a wrong-typed field
// terminated the process.
template <class T>
T jsonOr(const boost::json::object& obj, std::string_view key, T default_val)
{
  if (auto* v = obj.if_contains(key)) {
    return boost::json::value_to<T>(*v);
  }
  return default_val;
}

// Drops the entire selection state (selectables, inspected object,
// history, selection set, highlight/hover shapes) when a destroy callback
// flagged it stale.  Must be called before dereferencing any stored
// gui::Selected.  Returns true when the state was cleared.
bool consumeStaleSelection(SessionState& state);

// Build a kError response carrying `message`.  The three-line
// type/string/assign dance is easy to get subtly wrong (the payload is bytes,
// not a string), so it lives in one place.
WebSocketResponse errorResponse(uint32_t id, std::string_view message);

// Deepest tile zoom the server will address.  2^kMaxTileZoom must stay inside
// an int, since the grid size is computed as an int in several renderers.
// The client mirrors this ceiling in maxUsefulZoom() (ui-utils.js).
constexpr int kMaxTileZoom = 30;

// Read the z/x/y grid coordinates of a tile request into `z`/`x`/`y`.
// Returns false, with `error` describing the offending field, when a
// coordinate is missing, is not an integer, or the zoom is outside
// [0, kMaxTileZoom].  x and y are not bounded to the grid: Leaflet asks for
// off-grid tiles as a matter of course and the renderers answer with a
// transparent tile.
//
// The type check is not paranoia about hand-written clients: JSON.stringify
// serializes NaN and Infinity as `null`, so any client-side arithmetic that
// goes non-finite -- an unbounded zoom being the one we hit -- reaches the
// server as a null where a number belongs.  Reporting that as an error beats
// rendering the transparent tiles that a degenerate zoom would otherwise
// produce, which look to the user like the design vanished.
bool parseTileCoords(const boost::json::object& json,
                     int& z,
                     int& x,
                     int& y,
                     std::string& error);

// Handles SELECT, INSPECT, and HOVER requests.
class SelectHandler
{
 public:
  SelectHandler(std::shared_ptr<TileGenerator> gen,
                std::shared_ptr<TclEvaluator> tcl_eval);
  void registerRequests(RequestDispatcher& dispatcher);

  // Called after a request mutates the database (e.g. an accepted
  // set_property) so every connected client re-renders.  The session
  // wires this to SessionRegistry::broadcast; invoked with the STA lock
  // released.
  void setBroadcastFn(std::function<void(const std::string&)> fn)
  {
    broadcast_fn_ = std::move(fn);
  }

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
  WebSocketResponse handleSelectFanoutBin(const WebSocketRequest& req,
                                          SessionState& state);
  WebSocketResponse handleSelectNetLengthBin(const WebSocketRequest& req,
                                             SessionState& state);
  WebSocketResponse handleFind(const WebSocketRequest& req,
                               SessionState& state);
  WebSocketResponse handleSetRouteGuides(const WebSocketRequest& req,
                                         SessionState& state);
  WebSocketResponse handleSelectNext(const WebSocketRequest& req,
                                     SessionState& state);
  WebSocketResponse handleSelectPrev(const WebSocketRequest& req,
                                     SessionState& state);
  WebSocketResponse handleSetProperty(const WebSocketRequest& req,
                                      SessionState& state);
  WebSocketResponse handleTriggerAction(const WebSocketRequest& req,
                                        SessionState& state);
  WebSocketResponse handleHighlight(const WebSocketRequest& req,
                                    SessionState& state);
  WebSocketResponse handleUnhighlight(const WebSocketRequest& req,
                                      SessionState& state);
  WebSocketResponse handleClearHighlights(const WebSocketRequest& req,
                                          SessionState& state);
  WebSocketResponse handleListSelection(const WebSocketRequest& req,
                                        SessionState& state);
  WebSocketResponse handleInspectSelection(const WebSocketRequest& req,
                                           SessionState& state);
  WebSocketResponse handleInspectGroup(const WebSocketRequest& req,
                                       SessionState& state);
  WebSocketResponse handleDeselect(const WebSocketRequest& req,
                                   SessionState& state);
  WebSocketResponse handleSelectLayer(const WebSocketRequest& req,
                                      SessionState& state);
  WebSocketResponse handleSnap(const WebSocketRequest& req);
  WebSocketResponse handleSchematicCone(const WebSocketRequest& req);
  WebSocketResponse handleSchematicFull(const WebSocketRequest& req);
  WebSocketResponse handleSchematicInspect(const WebSocketRequest& req,
                                           SessionState& state);
  WebSocketResponse handleGet3DData(const WebSocketRequest& req);
  WebSocketResponse handleContextAction(const WebSocketRequest& req,
                                        SessionState& state);

 private:
  // Build a multi-selection from `matched` nets: caps the count, writes count/
  // truncated/selection_* into `root`, and updates `state` (selection set,
  // highlights, inspector).  Shared by handleSelectFanoutBin and
  // handleSelectNetLengthBin.  Caller must hold tcl_eval_->mutex.
  void selectMatchedNets(std::vector<odb::dbNet*>& matched,
                         SessionState& state,
                         boost::json::object& root,
                         bool use_dbu);

  std::shared_ptr<TileGenerator> gen_;
  std::shared_ptr<TclEvaluator> tcl_eval_;
  std::function<void(const std::string&)> broadcast_fn_;
};

// Handles TCL_EVAL requests.
class TclHandler
{
 public:
  explicit TclHandler(std::shared_ptr<TclEvaluator> tcl_eval);
  void registerRequests(RequestDispatcher& dispatcher);

  WebSocketResponse handleTclEval(const WebSocketRequest& req);
  WebSocketResponse handleTclComplete(const WebSocketRequest& req);

 private:
  std::shared_ptr<TclEvaluator> tcl_eval_;
};

// Handles DB-editing utilities: Global Connect (list/delete rules; add/apply/
// clear go through existing Tcl commands via TCL_EVAL) and Insert Buffer
// (list pins/masters + perform the insertion through the odb API).
class EditHandler
{
 public:
  EditHandler(std::shared_ptr<TileGenerator> gen,
              std::shared_ptr<TclEvaluator> tcl_eval);
  void registerRequests(RequestDispatcher& dispatcher);

  // Global Connect
  WebSocketResponse handleGlobalConnectInfo(const WebSocketRequest& req);
  WebSocketResponse handleGlobalConnectDelete(const WebSocketRequest& req);
  WebSocketResponse handleGlobalConnectApply(const WebSocketRequest& req);
  // Insert Buffer
  WebSocketResponse handleBufferInfo(const WebSocketRequest& req);
  WebSocketResponse handleInsertBuffer(const WebSocketRequest& req);

 private:
  std::shared_ptr<TileGenerator> gen_;
  // Serializes DB access against the Tcl write path (and other edits); the
  // same mutex TclEvaluator holds while running commands that mutate odb.
  std::shared_ptr<TclEvaluator> tcl_eval_;
};

// Handles TIMING_REPORT and TIMING_HIGHLIGHT requests.
class TimingHandler
{
 public:
  TimingHandler(std::shared_ptr<TileGenerator> gen,
                std::shared_ptr<TimingReport> timing_report,
                std::shared_ptr<TclEvaluator> tcl_eval);
  void registerRequests(RequestDispatcher& dispatcher);

  WebSocketResponse handleTimingReport(const WebSocketRequest& req);
  WebSocketResponse handleTimingHighlight(const WebSocketRequest& req,
                                          SessionState& state);
  WebSocketResponse handleTimingCone(const WebSocketRequest& req,
                                     SessionState& state);
  WebSocketResponse handleSlackHistogram(const WebSocketRequest& req);
  WebSocketResponse handleFanoutHistogram(const WebSocketRequest& req);
  WebSocketResponse handleNetLengthHistogram(const WebSocketRequest& req);
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
  void registerRequests(RequestDispatcher& dispatcher);

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
  void registerRequests(RequestDispatcher& dispatcher);

  // Push a message to every connected client after a label mutation.  Labels
  // live in the shared TileGenerator, so one client's edit changes what all
  // of them should be drawing; without this the others keep stale handles and
  // a stale overlay until something unrelated makes them reload.  The session
  // wires this to SessionRegistry::broadcast, as it does for SelectHandler.
  void setBroadcastFn(std::function<void(const std::string&)> fn)
  {
    broadcast_fn_ = std::move(fn);
  }

  void initializeHeatMaps(SessionState& state);
  WebSocketResponse handleTile(const WebSocketRequest& req,
                               SessionState& state);
  WebSocketResponse handleOverlayTile(const WebSocketRequest& req,
                                      SessionState& state);
  WebSocketResponse handleModuleHierarchy(const WebSocketRequest& req);
  // User text labels (2.12).  Labels live in the shared TileGenerator, so all
  // clients and save_image see them.
  WebSocketResponse handleAddLabel(const WebSocketRequest& req);
  WebSocketResponse handleDeleteLabel(const WebSocketRequest& req);
  WebSocketResponse handleUpdateLabel(const WebSocketRequest& req);
  WebSocketResponse handleClearLabels(const WebSocketRequest& req);
  WebSocketResponse handleListLabels(const WebSocketRequest& req);
  // Tell every client the label set changed.  Public so the Tcl-driven
  // entry points (add_label and friends) can announce their edits too:
  // labels sit outside ODB, so no design-change callback covers them.
  void broadcastLabelsChanged();
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
  // Marks a tile-request id as cancelled so a still-queued render is skipped.
  // Registered run_inline so it executes on the read thread, ahead of the
  // posted render it cancels.
  WebSocketResponse handleCancel(const WebSocketRequest& req,
                                 SessionState& state);

 private:
  static WebSocketResponse serializeBounds(uint32_t id,
                                           const TileGenerator& gen);
  static WebSocketResponse serializeTech(uint32_t id, const TileGenerator& gen);
  static WebSocketResponse renderTile(
      uint32_t id,
      const std::string& layer,
      int z,
      int x,
      int y,
      const TileVisibility& vis,
      const TileGenerator& gen,
      const std::vector<odb::Rect>& highlight_rects,
      const std::vector<odb::Polygon>& highlight_polys,
      const std::vector<ColoredRect>& colored_rects,
      const std::vector<FlightLine>& flight_lines,
      const std::map<uint32_t, Color>* module_colors,
      const std::set<uint32_t>* focus_net_ids,
      const std::set<uint32_t>* route_guide_net_ids,
      double dpr = 1.0,
      int tile_px = 0);

  std::shared_ptr<TileGenerator> gen_;
  std::function<void(const std::string&)> broadcast_fn_;
};

// Handles DRC_CATEGORIES, DRC_MARKERS, DRC_LOAD_REPORT,
// DRC_UPDATE_MARKER, and DRC_HIGHLIGHT requests.
class DRCHandler
{
 public:
  explicit DRCHandler(std::shared_ptr<TileGenerator> gen);
  void registerRequests(RequestDispatcher& dispatcher);

  WebSocketResponse handleDRCCategories(const WebSocketRequest& req);
  WebSocketResponse handleDRCMarkers(const WebSocketRequest& req,
                                     SessionState& state);
  WebSocketResponse handleDRCLoadReport(const WebSocketRequest& req,
                                        SessionState& state);
  WebSocketResponse handleDRCUpdateMarker(const WebSocketRequest& req,
                                          SessionState& state);
  WebSocketResponse handleDRCUpdateCategoryVisibility(
      const WebSocketRequest& req,
      SessionState& state);
  WebSocketResponse handleDRCHighlight(const WebSocketRequest& req,
                                       SessionState& state);

 private:
  std::shared_ptr<TileGenerator> gen_;
  int min_box_ = -1;  // cached tech pitch for marker rendering threshold

  // Returns block and chip, throwing if either is null.
  std::pair<odb::dbBlock*, odb::dbChip*> getBlockAndChip();

  // Find a marker by ID in the active category. Returns nullptr if not found.
  odb::dbMarker* findMarkerById(SessionState& state,
                                odb::dbChip* chip,
                                int marker_id);

  // Recompute DRC overlay rects from the active category's visible markers.
  void refreshDRCOverlay(SessionState& state);
};

// Handles LIST_DIR requests (server-side file browsing).
WebSocketResponse handleListDir(const WebSocketRequest& req);

}  // namespace web
