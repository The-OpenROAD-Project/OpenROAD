// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <any>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "boost/json/array.hpp"
#include "boost/json/object.hpp"
#include "color.h"
#include "glyph_cache.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "web_painter.h"

namespace sta {
class dbSta;
}

namespace gui {
class HeatMapDataSource;
}

namespace utl {
class Logger;
}

namespace web {

class Search;

struct ColoredRect
{
  odb::Rect rect;
  Color color;
  std::string layer;    // empty = draw on all layers
  bool filled = false;  // true = filled rect + outline (DRC markers)
                        // false = centerline (timing paths)
};

struct FlightLine
{
  odb::Point p1;
  odb::Point p2;
  Color color;
};

// The nine canonical anchor names, in gui::Painter::anchors() order.  That
// table (src/gui/src/painter.cpp) is the source of truth for the spelling and
// is duplicated rather than shared because libweb has no link dependency on
// the Qt GUI — the same trade-off spectrumColor() makes in color.h.  Keep the
// two in sync: add_label is one user-facing command, so -anchor has to mean
// the same thing whichever GUI runs it.
const std::vector<std::string>& anchorNames();

// True when `anchor` is one of anchorNames().  The empty string is NOT valid;
// callers that treat empty as "use the default" must substitute "center"
// before asking.
bool isValidAnchor(const std::string& anchor);

// A short text label anchored at a DBU point, drawn on the overlay tile.
// Used by the timing-cone overlay (depth annotations) and by user labels
// (2.12).  `size` is the font pixel size (0 = default) and `anchor` names the
// point of the text box that sits on `pos` — see anchorNames().
struct TextLabel
{
  odb::Point pos;
  std::string text;
  Color color;
  int size = 0;
  std::string anchor = "center";
};

// A user-created text annotation stored on the design (mirrors the Qt GUI's
// gui::Label).  Global (not per-session) so it renders into every client's
// tiles and into save_image, matching the Qt GUI.
struct StoredLabel
{
  odb::Point pos;
  std::string text;
  Color color;
  int size = 0;
  std::string anchor = "center";
  std::string name;
};

struct ColoredPolygon
{
  odb::Polygon poly;
  Color color;
};

// Decimal places needed to print a DBU length in microns without collapsing two
// adjacent DBU onto the same string.  That is the smallest p with 10^p >=
// dbu_per_micron, hence ceil() and not round(): at 2000 DBU/µm (Nangate45)
// round() yields 3, and 1 DBU (0.0005 µm) and 2 DBU (0.001 µm) both print as
// "0.001".  Returns 0 for a scale of 0 or less, which callers treat as "no
// scale known yet" and print raw DBU for.
//
// TODO: this and dbuToMicronString belong in utl alongside to_numeric_string;
// they live here only because tile_generator.h is the web module's shared
// header today.
int dbuPrecision(double dbu_per_micron);

// DBU -> micron string at dbuPrecision(), trailing zeros stripped.  Falls back
// to the raw DBU count when the database has no scale yet, which is the case
// before any LEF has been read.
std::string dbuToMicronString(int dbu, double dbu_per_micron);

// Where one tile sits in DBU space, and how DBU map to its pixels.
//
// The tile grid step is maxDXDY / 2^z — a FRACTIONAL number of DBU — so a
// tile's lower-left corner is fractional too, and `origin_x/origin_y` keep it
// that way.  The client lays the tiles out on that exact grid (the DBU↔latLng
// transform in coordinates.js), so rounding the corner here would shift a
// tile's content by its own fractional part, a different amount in each tile.
// Neighbours then disagree about where the shared edge is and a hairline seam
// opens along it — one that widens as you zoom in (the shift is fixed in DBU,
// so it grows in pixels) and again with the display's dpr.
//
// `cull` is the same window rounded OUTWARD to whole DBU: the query and clip
// window, never the drawing origin.
struct TileFrame
{
  double origin_x = 0.0;
  double origin_y = 0.0;
  double scale = 1.0;  // pixels per DBU
  odb::Rect cull;
  // Pixels of THIS frame per CSS pixel.  Sizes authored in CSS px — pen widths,
  // font heights — are multiplied by it so they come out the same size on every
  // display instead of shrinking as the ratio rises.  The display's dpr for an
  // output-resolution frame; dpr * the supersample factor for a super one.
  double px_per_css = 1.0;

  // DBU → pixels within the tile.  Y counts up from the tile's bottom edge;
  // callers that write into the buffer apply the row flip themselves.
  double pxX(double dbu) const { return (dbu - origin_x) * scale; }
  double pxY(double dbu) const { return (dbu - origin_y) * scale; }
};

struct SelectionResult
{
  std::any object;  // dbInst*, dbNet*, etc.
  std::string name;
  std::string type_name;  // "Inst", "Net", etc. — sent to the JSON API
  odb::Rect bbox;
  // Local-to-root transform of the chiplet the hit came from, so a consumer
  // that re-derives a bbox from `object` (a gui::Descriptor reports it in the
  // object's own block coordinates) can lift it into the same world space
  // `bbox` is already in.  Identity for single-die designs.
  odb::dbTransform world_xfm;
  // Fast-path tag for sort/count.  type_name is a string so `selectAt`
  // can serialize it later, but the sort comparator runs on every
  // result pair — comparing two short strings ("Inst" / "Net") per
  // comparison adds up.  `is_inst` is set alongside type_name and
  // dominates the sort.
  bool is_inst = false;
};

// One node in the chiplet tree rooted at db->getChip().  The root has
// inst==nullptr and an identity world_xfm; descendants accumulate
// dbChipInst transforms top-down.  See collectChiplets().
struct ChipletNode
{
  odb::dbChip* chip = nullptr;
  odb::dbBlock* block = nullptr;    // chip->getBlock()
  odb::dbChipInst* inst = nullptr;  // null for root
  odb::dbTransform world_xfm;       // local-to-root transform
  std::string path;                 // "top.soc_inst.subip" — unique
  std::string parent_path;          // path of the parent ("" for the root)
  std::string name;                 // "top" or inst->getName()
  int depth = 0;
  int global_z = 0;
};

// Walk the dbChip → dbChipInst → masterChip hierarchy depth-first and
// return a flat list with each chiplet's accumulated world transform.
// Related Qt code: `LayoutViewer::getChips()` returns a flat
// (dbChipInst → dbChip) PtrMap with no transform composition — this
// function additionally accumulates `dbTransform`s top-down and assigns
// stable hierarchical paths so the web renderer can place each chiplet.
std::vector<ChipletNode> collectChiplets(odb::dbChip* root);

// Coarse instance category, derived once per inst and reused by both
// isInstVisible and isInstSelectable so the two stay in lock-step.
enum class InstCategory
{
  kStdCells,
  kMacros,
  kPadInput,
  kPadOutput,
  kPadInout,
  kPadPower,
  kPadSpacer,
  kPadAreaIO,
  kPadOther,
  kPhysEndcap,
  kPhysFill,
  kPhysWelltap,
  kPhysTie,
  kPhysAntenna,
  kPhysCover,
  kPhysBump,
  kPhysOther,
  kStdBufInv,
  kStdBufInvTiming,
  kStdClockBufInv,
  kStdClockGate,
  kStdLevelShift,
  kStdSequential,
  kStdCombinational,
};

InstCategory classifyInstance(odb::dbInst* inst, sta::dbSta* sta);

struct TileVisibility
{
  bool stdcells = true;
  bool macros = true;

  // Pad sub-types
  bool pad_input = true;
  bool pad_output = true;
  bool pad_inout = true;
  bool pad_power = true;
  bool pad_spacer = true;
  bool pad_areaio = true;
  bool pad_other = true;

  // Physical sub-types
  bool phys_fill = true;
  bool phys_endcap = true;
  bool phys_welltap = true;
  bool phys_tie = true;
  bool phys_antenna = true;
  bool phys_cover = true;
  bool phys_bump = true;
  bool phys_other = true;

  // Std cell sub-types (used when Liberty/STA is available)
  bool std_bufinv = true;
  bool std_bufinv_timing = true;
  bool std_clock_bufinv = true;
  bool std_clock_gate = true;
  bool std_level_shift = true;
  bool std_sequential = true;
  bool std_combinational = true;

  // Net sub-types (by dbSigType)
  bool net_signal = true;
  bool net_power = true;
  bool net_ground = true;
  bool net_clock = true;
  bool net_reset = true;
  bool net_tieoff = true;
  bool net_scan = true;
  bool net_analog = true;

  // Shapes — routing sub-types
  bool routing = true;            // parent flag (kept for backward compat)
  bool routing_segments = true;   // regular wire segments
  bool routing_vias = true;       // regular vias
  bool special_nets = true;       // parent flag (kept for backward compat)
  bool srouting_segments = true;  // special-net segments/straps
  bool srouting_vias = true;      // special-net vias
  bool pins = true;               // BTerm (IO pin) shapes on tech layers
  bool pin_markers = true;        // BTerm direction markers on _pins layer
  bool pin_names = true;          // BTerm name labels on _pins layer
  bool access_points
      = false;  // dbAccessPoint markers (X), off by default (Qt parity)
  bool regions
      = true;  // dbRegion boundaries overlay, on by default (Qt parity)
  bool mfg_grid = false;  // manufacturing-grid dots, off by default (Qt parity)
  bool gcell_grid = false;  // GCell grid lines, off by default (Qt parity)
  bool rudy = false;        // RUDY congestion heatmap, off by default

  // Shapes — other per-layer geometry (not routing sub-types)
  bool blockages = true;  // master obstructions (LEF OBS)
  bool fills = false;     // dbFill metal fill (off by default, GUI parity)

  // Fill pattern applied to the requested layer's own shapes (routing,
  // special-net, instance OBS/pins).  Per-request because each tile request
  // targets a single layer.  Defaults to solid (the historical behavior).
  FillPattern fill_pattern = FillPattern::kSolid;

  // Instance sub-shapes
  bool inst_names = true;      // Instance name labels on _instances layer
  bool inst_pins = true;       // ITerm (cell pin) shapes on tech layers
  bool inst_pin_names = true;  // ITerm name labels

  // Blockages (dbBlockage / dbObstruction)
  bool placement_blockages = true;
  bool routing_obstructions = true;

  // Rows (off by default, matching GUI)
  bool rows = false;
  // Per-site visibility, populated from any "site_<name>" int keys in the
  // payload during parseFromJson().
  std::unordered_map<std::string, bool> sites;
  bool isSiteVisible(const std::string& site_name) const;

  // Tracks (off by default, matching GUI)
  bool tracks_pref = false;
  bool tracks_non_pref = false;

  // Detailed view (off by default, matching the Qt GUI's Misc/"Detailed view").
  // When on, the sub-resolution cull is relaxed so small features stay visible
  // at zoom-out: instances are not culled at all and shapes fall back to a 1 px
  // limit (mirroring LayoutViewer::instanceSizeLimit()/shapeSizeLimit()).
  bool detailed = false;

  // User text labels (2.12).  On by default like the Qt GUI's Misc/"Labels",
  // which gates RenderThread::drawLabels — and so gates them in Qt's
  // save_image too, since that renders through the same path.
  bool labels = true;

  // Debug
  bool debug = false;

  // When true the tile renderer iterates gui::Gui::renderers() and
  // rasterizes drawObjects() output.  Drives the gpl / cts / mpl debug
  // graphics overlay.  Off by default so tiles stay cheap.
  bool debug_renderers = false;

  // When debug_renderers is on, normally the overlay only renders while
  // the placer is paused (avoids racing against mutating renderer state).
  // Setting debug_live=true opts in to non-blocking streaming: the
  // overlay renders every frame even when not paused, accepting the
  // occasional inconsistency for smoother visualization.
  bool debug_live = false;

  // Per-metal-layer visibility: when has_visible_layers is true, pin marker
  // rendering skips BPin boxes whose tech layer is not in this set.
  std::set<std::string> visible_layers;
  bool has_visible_layers = false;

  // Per-chiplet visibility: when has_visible_chiplets is true, the tile
  // renderer skips ChipletNodes whose `path` is not in this set.  Empty
  // set with the flag off renders every chiplet (default).  Paths match
  // ChipletNode::path produced by collectChiplets() (e.g. "top.soc_inst").
  std::set<std::string> visible_chiplets;
  bool has_visible_chiplets = false;
  bool isChipletVisible(const std::string& path) const;

  // ── Selectability ──
  // Parallel to the visibility flags above: when off, the corresponding
  // class of object is still rendered but is not pickable by selectAt().
  // Mirrors the Qt GUI's displayControls "selectable" column.
  // Defaults are all true (everything selectable), matching the Qt GUI.
  bool stdcells_selectable = true;
  bool macros_selectable = true;

  bool pad_input_selectable = true;
  bool pad_output_selectable = true;
  bool pad_inout_selectable = true;
  bool pad_power_selectable = true;
  bool pad_spacer_selectable = true;
  bool pad_areaio_selectable = true;
  bool pad_other_selectable = true;

  bool phys_fill_selectable = true;
  bool phys_endcap_selectable = true;
  bool phys_welltap_selectable = true;
  bool phys_tie_selectable = true;
  bool phys_antenna_selectable = true;
  bool phys_cover_selectable = true;
  bool phys_bump_selectable = true;
  bool phys_other_selectable = true;

  bool std_bufinv_selectable = true;
  bool std_bufinv_timing_selectable = true;
  bool std_clock_bufinv_selectable = true;
  bool std_clock_gate_selectable = true;
  bool std_level_shift_selectable = true;
  bool std_sequential_selectable = true;
  bool std_combinational_selectable = true;

  bool net_signal_selectable = true;
  bool net_power_selectable = true;
  bool net_ground_selectable = true;
  bool net_clock_selectable = true;
  bool net_reset_selectable = true;
  bool net_tieoff_selectable = true;
  bool net_scan_selectable = true;
  bool net_analog_selectable = true;

  bool pins_selectable = true;
  bool inst_pins_selectable = true;

  bool placement_blockages_selectable = true;
  bool routing_obstructions_selectable = true;

  // Per-site selectability (peer to `sites`).  Defaults to true when
  // unspecified — checked only when the corresponding row is selectable.
  std::unordered_map<std::string, bool> site_selectable;

  // Per-metal-layer selectability.  When has_selectable_layers is true,
  // selectAt() skips layers not in this set.
  std::set<std::string> selectable_layers;
  bool has_selectable_layers = false;

  void parseFromJson(const boost::json::object& json);

  bool isNetVisible(odb::dbNet* net) const;
  bool isInstVisible(odb::dbInst* inst, sta::dbSta* sta) const;
  // Visibility for an already-classified instance.  Lets callers that already
  // computed the category (e.g. the tile render loop) avoid reclassifying.
  bool isCategoryVisible(InstCategory cat) const;

  bool isNetSelectable(odb::dbNet* net) const;
  bool isInstSelectable(odb::dbInst* inst, sta::dbSta* sta) const;
  bool isSiteSelectable(const std::string& site_name) const;
  bool isLayerSelectable(const std::string& layer_name) const;
};

class TileGenerator
{
 public:
  TileGenerator(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);
  ~TileGenerator();

  void eagerInit();
  bool shapesReady() const;

  bool hasSta() const { return sta_ != nullptr; }
  sta::dbSta* getSta() const { return sta_; }
  utl::Logger* getLogger() const { return logger_; }

  int getThreadCount() const { return num_threads_; }
  void setThreadCount(const int num_threads) { num_threads_ = num_threads; }

  odb::Rect getBounds() const;
  int getPinMaxSize() const;

  std::vector<std::string> getLayers() const;
  std::vector<std::string> getSites() const;

  // Per-layer colors matching gui::DisplayControls layer palette.  Computed
  // lazily and cached; the cache is rebuilt only if the tech changes.
  const odb::PtrMap<odb::dbTechLayer, Color>& getLayerColorMap(odb::dbTech* tech
                                                               = nullptr) const;

  // Geometry one dbMaster contributes to one tech layer, in master-local
  // coordinates; the render pass applies each instance's transform.  Bucketing
  // by layer up front is what lets the per-instance pass draw a layer's shapes
  // directly instead of walking all of a master's geometry and calling
  // dbBox::getTechLayer() (a chain of dbTable lookups) on every box, once per
  // layer per tile.  Mirrors gui::LayoutViewer::boxesByLayer.
  struct MasterLayerGeom
  {
    // Obstructions (LEF OBS), drawn before pins.  Polygons and boxes are kept
    // apart because they take different draw calls, and both before the pin
    // shapes because OBS uses a different color — that boundary is the only
    // ordering that affects the result (shapes sharing a color composite
    // order-independently).
    std::vector<odb::Polygon> obs_polys;
    std::vector<odb::Rect> obs_boxes;
    std::vector<odb::Polygon> pin_polys;
    // Pin boxes grouped by MTerm, preserving master MTerm order and geometry
    // order within a pin: the fill pass draws them all, and the ITerm label
    // pass walks each group for the first box big enough to label.
    std::vector<std::pair<odb::dbMTerm*, std::vector<odb::Rect>>> pin_boxes;
  };
  using MasterGeomByLayer = odb::PtrMap<odb::dbMaster, MasterLayerGeom>;

  // Cut and enclosure boxes of one via master (dbTechVia or dbVia) on one
  // layer, in via-local coordinates; the render pass offsets them by the sbox
  // center.  A power grid instantiates a handful of distinct via masters
  // thousands of times, so decomposing each master once removes the
  // per-via-instance walk that dominated special-net rendering.
  using ViaBoxesByMaster = odb::PtrMap<odb::dbObject, std::vector<odb::Rect>>;

  // Immutable snapshot of both caches, published as a whole.  Renders take a
  // shared_ptr copy once per tile and then read it without locking; a
  // concurrent invalidation swaps in a fresh snapshot and leaves the one
  // in-flight renders hold alive.
  struct GeomCache
  {
    odb::PtrMap<odb::dbTechLayer, MasterGeomByLayer> master_geom;
    odb::PtrMap<odb::dbTechLayer, ViaBoxesByMaster> via_boxes;
  };
  std::shared_ptr<const GeomCache> geomCache() const;

  std::vector<SelectionResult> selectAt(
      int dbu_x,
      int dbu_y,
      int zoom = 0,
      const TileVisibility& vis = {},
      const std::set<std::string>& visible_layers = {});

  struct SnapResult
  {
    std::pair<odb::Point, odb::Point> edge;
    int distance = 0;
    bool found = false;
  };

  SnapResult snapAt(int dbu_x,
                    int dbu_y,
                    int search_radius,
                    int point_snap_threshold,
                    bool horizontal,
                    bool vertical,
                    const TileVisibility& vis,
                    const std::set<std::string>& visible_layers) const;

  odb::dbBlock* getBlock() const;
  odb::dbChip* getChip() const;
  odb::dbTech* getTech() const;
  odb::dbDatabase* getDb() const { return db_; }

  // ─── User text labels (2.12) ─────────────────────────────────────────
  // Design-level annotations, global (not per-session), so they render into
  // overlay tiles and save_image for every client — mirrors the Qt GUI.
  // addLabel returns the label's name (auto-generated "label<N>" when `name`
  // is empty; a clashing name is rejected and "" is returned).
  std::string addLabel(const odb::Point& pos,
                       const std::string& text,
                       const Color& color,
                       int size,
                       const std::string& anchor,
                       const std::string& name);
  bool deleteLabel(const std::string& name);
  // Atomically mutate an existing label in place (used for move/edit so the
  // label can never be lost by a delete+add race).  No-op returning false if
  // no label has `name`.
  bool updateLabel(const std::string& name,
                   const odb::Point& pos,
                   const std::string& text,
                   const Color& color,
                   int size,
                   const std::string& anchor);
  void clearLabels();
  // Snapshot of all labels as drawable TextLabels (thread-safe).
  std::vector<TextLabel> labelsForDraw() const;
  // Labels serialized for the client (name/x/y/text/color/size/anchor).
  boost::json::array labelsJson() const;

  // Cached, sorted list of chiplets reachable from db_->getChip().
  // The cache is invalidated by eagerInit() and rebuilt lazily on the
  // next call.  Hot-path call-sites (renderTileBuffer, getBounds,
  // selectAt) read it on every tile / click; the free function
  // `collectChiplets` is kept for tests and one-shot callers.
  const std::vector<ChipletNode>& chiplets() const;

  // Monotonic counter, bumped every time chiplets() rebuilds its cache.
  // Caches derived from the chiplet list poll this to notice a hierarchy
  // change, which no dbBlockCallBackObj reports (see geomCache()).  Refreshes
  // the chiplet cache, so the value returned reflects the live hierarchy.
  uint64_t chipletsGeneration() const;

  std::vector<unsigned char> generateTile(
      const std::string& layer,
      int z,
      int x,
      int y,
      const TileVisibility& vis = {},
      const std::vector<odb::Rect>& highlight_rects = {},
      const std::vector<odb::Polygon>& highlight_polys = {},
      const std::vector<ColoredRect>& colored_rects = {},
      const std::vector<FlightLine>& flight_lines = {},
      const std::map<uint32_t, Color>* module_colors = nullptr,
      const std::set<uint32_t>* focus_net_ids = nullptr,
      const std::set<uint32_t>* route_guide_net_ids = nullptr,
      double dpr = 1.0,
      // Exact device-pixel side length to render, as the client will display
      // it.  Sent explicitly because a tile's CSS box is only a whole number of
      // device pixels when tileSize*dpr is an integer: at a 1.6667 display
      // scale, 256 CSS px is 426.67 device px, and handing the browser the
      // rounded 428 makes it resample every tile.  0 = unspecified, which falls
      // back to the historical 256*dpr.
      int tile_px = 0) const;

  // Render only highlight/overlay shapes (selection, hover, timing, DRC,
  // route guides, flight lines) on a fully transparent background.  Used
  // by the overlay tile layer so base tiles can stay cached when only
  // highlights change.
  std::vector<unsigned char> generateOverlayTile(
      int z,
      int x,
      int y,
      const std::vector<odb::Rect>& highlight_rects = {},
      const std::vector<odb::Polygon>& highlight_polys = {},
      const std::vector<ColoredRect>& colored_rects = {},
      const std::vector<FlightLine>& flight_lines = {},
      const std::set<uint32_t>* route_guide_net_ids = nullptr,
      bool has_visible_layers = false,
      const std::set<std::string>& visible_layers = {},
      double dpr = 1.0,
      int tile_px = 0,
      const std::vector<ColoredPolygon>& colored_polys = {},
      const std::vector<TextLabel>& labels = {}) const;
  std::vector<unsigned char> generateHeatMapTile(gui::HeatMapDataSource& source,
                                                 int z,
                                                 int x,
                                                 int y,
                                                 double dpr = 1.0,
                                                 int tile_px = 0) const;

  // Composite the design (or region) into a top-down RGBA8 pixel buffer.
  // Works without a running web server.  region in DBU; if zero-area,
  // defaults to die + 5% margin.  Each pixel starts at `bg` and the
  // (possibly semi-transparent) tiles are composited on top.  This is the
  // shared core of renderImagePng (which then PNG-encodes) and animated-GIF
  // frame capture (which feeds the buffer to the GIF encoder).  Returns an
  // empty buffer on error (no design / invalid dimensions).
  // `out_width`/`out_height` receive the buffer's pixel dimensions, which
  // the caller cannot predict: they follow from the region and the 16k
  // clamp, not from `width_px` alone.
  std::vector<unsigned char> renderImageBuffer(const odb::Rect& region,
                                               int width_px,
                                               double dbu_per_pixel,
                                               const TileVisibility& vis,
                                               const Color& bg = {},
                                               int* out_width = nullptr,
                                               int* out_height = nullptr) const;

  // Render full design (or region) to PNG bytes, as renderImageBuffer does
  // to raw pixels.  Returns an empty vector on error.
  std::vector<unsigned char> renderImagePng(const odb::Rect& region,
                                            int width_px,
                                            double dbu_per_pixel,
                                            const TileVisibility& vis,
                                            const Color& bg = {},
                                            int* out_width = nullptr,
                                            int* out_height = nullptr) const;

  // Render full design (or region) to a PNG file.  Works without a running
  // web server.  region in DBU; if zero-area, defaults to die + 5% margin.
  void saveImage(const std::string& filename,
                 const odb::Rect& region,
                 int width_px,
                 double dbu_per_pixel,
                 const TileVisibility& vis) const;

  // The layers saveImage composites, bottom to top.  Public so a test can pin
  // the order down: it has to match the zIndex the client gives each layer in
  // display-controls.js, or the saved PNG is not the view on screen.
  static std::vector<std::string> saveImageLayerOrder(
      const TileVisibility& vis,
      const std::vector<std::string>& tech_layers);

  // Render timing path overlay (colored rects + flight lines) to PNG bytes.
  std::vector<unsigned char> renderOverlayPng(
      int width_px,
      const std::vector<ColoredRect>& rects,
      const std::vector<FlightLine>& lines) const;

  // ─── Debug-graphics overlay ──────────────────────────────────────────
  //
  // When `vis.debug_renderers` is on, renderTileBuffer invokes the
  // installed DebugOverlayCallback (if any).  The callback is
  // responsible for iterating any registered gui::Renderer instances
  // and drawing their output onto the image buffer.  Kept as a
  // callback rather than a direct gui::Gui::get() call so that
  // libweb.a has no undefined references to the gui/SWIG library —
  // test executables that link libweb don't need to pull in ord.
  using DebugOverlayCallback
      = std::function<void(std::vector<unsigned char>& image,
                           const TileFrame& frame,
                           bool debug_live)>;
  // Install (or clear with `{}`) the debug-overlay callback.  Global
  // process state; installed by WebServer on serve() and cleared on
  // shutdown.
  static void setDebugOverlayCallback(DebugOverlayCallback callback);

  // Rasterize a WebPainter's recorded DrawOps into the tile's pixel
  // buffer.  Public so that the debug-overlay callback (living in
  // web.cpp, which is only in the main openroad binary) can reuse
  // TileGenerator's line/polygon/bitmap primitives.
  void rasterizeWebPainterOps(std::vector<unsigned char>& image,
                              const std::vector<DrawOp>& ops,
                              const TileFrame& frame) const;

  // ─── Server-side tile cache ──────────────────────────────────────────
  //
  // PNG-encoded "clean" tiles (no per-session overlays) keyed by a string
  // fingerprint of the full render determinant.  Re-rendering tiles at
  // 256*dpr*S is expensive, so pan/zoom-back and visibility toggles reuse
  // the cached bytes.  LRU eviction at kTileCacheCap; cleared on design
  // reload via eagerInit().  Thread-safe (mirrors chiplets_mutex_).
  bool tileCacheGet(const std::string& key,
                    std::vector<unsigned char>& out) const;
  void tileCachePut(std::string key, std::vector<unsigned char> png) const;

  // Install (or clear with `{}`) a callback invoked after a design edit has
  // invalidated the tile cache — WebServer wires this to broadcast a
  // {"type":"refresh"} push so clients re-request tiles (mirrors the Qt GUI's
  // Search::modified → LayoutViewer::fullRepaint).  Set at serve() startup.
  void setDesignChangedCallback(std::function<void()> cb);
  size_t tileCacheSize() const;  // for tests

 private:
  // Render a single tile into a raw RGBA buffer (pre-PNG-encoding).
  // Same signature as generateTile but returns raw pixels.
  std::vector<unsigned char> renderTileBuffer(
      const std::string& layer,
      int z,
      int x,
      int y,
      const TileVisibility& vis = {},
      const std::vector<odb::Rect>& highlight_rects = {},
      const std::vector<odb::Polygon>& highlight_polys = {},
      const std::vector<ColoredRect>& colored_rects = {},
      const std::vector<FlightLine>& flight_lines = {},
      const std::map<uint32_t, Color>* module_colors = nullptr,
      const std::set<uint32_t>* focus_net_ids = nullptr,
      const std::set<uint32_t>* route_guide_net_ids = nullptr,
      double dpr = 1.0,
      // Exact device-pixel side length to render, as the client will display
      // it.  Sent explicitly because a tile's CSS box is only a whole number of
      // device pixels when tileSize*dpr is an integer: at a 1.6667 display
      // scale, 256 CSS px is 426.67 device px, and handing the browser the
      // rounded 428 makes it resample every tile.  0 = unspecified, which falls
      // back to the historical 256*dpr.
      int tile_px = 0) const;
  // `dim` is the square tile side length (buffer stride); -1 derives it from
  // the buffer.  Hot loops pass it explicitly to avoid the per-pixel sqrt.
  void setPixel(std::vector<unsigned char>& image,
                int x,
                int y,
                const Color& c,
                int dim = -1) const;

  void drawDebugOverlay(std::vector<unsigned char>& image,
                        int z,
                        int x,
                        int y) const;

  // Anti-aliased text rendering.  All methods take a pre-resolved FontSize
  // handle so callers lock the glyph cache once per rendering context rather
  // than once per character.
  static int getTextWidth(std::string_view text,
                          const GlyphCache::FontSize& font);
  static int getTextHeight(const GlyphCache::FontSize& font);
  static void drawText(std::vector<unsigned char>& image,
                       int x,
                       int y,
                       std::string_view text,
                       const GlyphCache::FontSize& font,
                       const Color& color);
  // Draw text rotated 90° CW (reads top-to-bottom).
  static void drawTextRotated(std::vector<unsigned char>& image,
                              int x,
                              int y,
                              std::string_view text,
                              const GlyphCache::FontSize& font,
                              const Color& color);

  void drawHighlight(std::vector<unsigned char>& image,
                     const std::vector<odb::Rect>& rects,
                     const std::vector<odb::Polygon>& polys,
                     const TileFrame& frame) const;

  void drawColoredHighlight(std::vector<unsigned char>& image,
                            const std::vector<ColoredRect>& rects,
                            const std::string& current_layer,
                            const TileFrame& frame) const;

  void drawFlightLines(std::vector<unsigned char>& image,
                       const std::vector<FlightLine>& lines,
                       const TileFrame& frame) const;

  void drawColoredPolygons(std::vector<unsigned char>& image,
                           const std::vector<ColoredPolygon>& polys,
                           const TileFrame& frame) const;

  // Draw centered text labels (e.g. timing-cone logic depth) on the overlay.
  void drawTextLabels(std::vector<unsigned char>& image,
                      const std::vector<TextLabel>& labels,
                      const TileFrame& frame) const;

  // Raw (un-encoded) RGBA tile with only the user labels drawn on a
  // transparent background, for the given Leaflet z/x/y.  Used to composite
  // labels into save_image.  `labels` is passed in (snapshot once per image)
  // so the mutex isn't locked and copied per tile.  Returns empty if `labels`
  // is empty.
  std::vector<unsigned char> renderLabelTile(
      int z,
      int x,
      int y,
      const std::vector<TextLabel>& labels,
      double dpr = 1.0,
      int tile_px = 0) const;

  // Private counterpart of setDebugOverlayCallback: invokes the
  // installed callback (if any) for this tile.  See the public API
  // above for rationale.
  void drawRendererOverlay(std::vector<unsigned char>& image,
                           const TileFrame& frame,
                           bool debug_live) const;

  void drawRouteGuides(std::vector<unsigned char>& image,
                       const std::set<uint32_t>& net_ids,
                       const std::string& layer,
                       const Color& color,
                       const TileFrame& frame) const;

  static odb::Rect toPixels(const TileFrame& frame, const odb::Rect& rect);

  // `dim` follows the setPixel/blendPixel convention: pass the buffer's side
  // length to skip re-deriving it with bufferDim(), or -1 to have it computed.
  // Both of these are called once per drawn shape, so on a dense layer the
  // sqrt+lround adds up — callers in the render loop already know the value.
  void fillPolygon(std::vector<unsigned char>& image,
                   const odb::Polygon& poly,
                   const TileFrame& frame,
                   const Color& color,
                   bool blend = false,
                   FillPattern pattern = FillPattern::kSolid,
                   int dim = -1) const;

  // ox/oy are the tile's origin in absolute pixel coordinates, folded onto the
  // pattern lattice (patternAnchor()); they anchor non-solid patterns so the
  // hatch stays seamless across tile boundaries.  Only meaningful when
  // pattern != kSolid.
  void drawFilledRect(std::vector<unsigned char>& buffer,
                      const odb::Rect& rect,
                      const Color& color,
                      FillPattern pattern = FillPattern::kSolid,
                      int ox = 0,
                      int oy = 0,
                      int dim = -1) const;

  static void blendPixel(std::vector<unsigned char>& image,
                         int x,
                         int y,
                         const Color& c,
                         int dim = -1);

  // Endpoints are tile-pixel coordinates, in double so that a segment far
  // outside the tile keeps its exact SLOPE: the clip below runs on the values
  // as given and only its (in-bounds) result is rounded.  Converting an oblique
  // DBU segment through the clamped toPxX/toPxY instead would saturate each
  // axis on its own and rotate the segment — use toPxXd/toPxYd here.
  static void drawLine(std::vector<unsigned char>& image,
                       double x0,
                       double y0,
                       double x1,
                       double y1,
                       const Color& c,
                       int width = 3);

  void computePinLabelMargin();

  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  utl::Logger* logger_;
  int num_threads_ = 0;
  std::unique_ptr<Search> search_;
  int pin_label_margin_dbu_ = 0;  // cached by computePinLabelMargin()

  // Cached layer-color map keyed by tech (see getLayerColorMap).  Each tech is
  // computed once and kept; std::map reference stability means a returned ref
  // stays valid even if another tech is added later.
  mutable std::mutex layer_colors_mutex_;
  mutable odb::PtrMap<odb::dbTech, odb::PtrMap<odb::dbTechLayer, Color>>
      layer_colors_by_tech_;

  // Layer-bucketed master and via-master geometry.  See geomCache(); built on
  // first use and rebuilt whenever Search::revision() or chipletsGeneration()
  // moves.  The via half is keyed off the reachable chiplets' blocks, so a
  // hierarchy edit changes what belongs in it without any block edit.
  mutable std::mutex geom_cache_mutex_;
  mutable std::shared_ptr<const GeomCache> geom_cache_;
  mutable uint64_t geom_cache_revision_ = 0;
  mutable uint64_t geom_cache_chiplet_generation_ = 0;
  std::shared_ptr<const GeomCache> buildGeomCache() const;

  // Cached chiplet traversal.  See chiplets().  Invalidated in
  // eagerInit() and also auto-invalidated when the chiplet hierarchy
  // signature (root pointer + total dbChipInst count) changes — this
  // catches Tcl-driven dbChipInst::create/destroy between eagerInit
  // calls, which dbBlockCallBackObj does not surface.
  mutable std::mutex chiplets_mutex_;
  mutable std::vector<ChipletNode> chiplets_cache_;
  mutable bool chiplets_cache_valid_ = false;
  mutable odb::dbChip* chiplets_cache_root_ = nullptr;
  mutable size_t chiplets_cache_inst_count_ = 0;
  // See chipletsGeneration().
  mutable uint64_t chiplets_cache_generation_ = 0;

  // LRU cache of PNG-encoded clean tiles (see tileCacheGet/Put).  The list
  // holds (key, png) most-recent-first; the index maps key → list iterator.
  using TileCacheEntry = std::pair<std::string, std::vector<unsigned char>>;
  mutable std::mutex tile_cache_mutex_;
  mutable std::list<TileCacheEntry> tile_cache_lru_;
  mutable std::unordered_map<std::string, std::list<TileCacheEntry>::iterator>
      tile_cache_index_;
  static constexpr size_t kTileCacheCap = 512;

  // Design-change → invalidation wiring.  search_ fires on_modified (see
  // Search::setOnModified) on any geometry edit; onDesignChanged() drops the
  // PNG tile cache and invokes design_changed_cb_ (installed by WebServer to
  // broadcast a "refresh" push to clients).  suppress_design_changed_ gates
  // out the storm of index invalidations during eagerInit()/reload, which
  // already clears the cache itself and drives its own refresh.
  void onDesignChanged();
  std::function<void()> design_changed_cb_;
  mutable std::mutex design_changed_cb_mutex_;
  std::atomic_bool suppress_design_changed_{false};

  // Per-block caches for the grid/access-point overlay layers, so tiles
  // don't rescan all BTerms / copy the full gcell vectors on every tile.
  // Dropped by clearOverlayCaches() from eagerInit() (design reload), and by
  // the accessors themselves when Search::revision() has moved: a gcell grid
  // created mid-session by global_route would otherwise stay hidden behind the
  // empty vector cached before it, and onDesignChanged() is debounced, so it
  // cannot be relied on to notice (see dropOverlayCachesIfStale).
  //
  // The accessors hand out a shared_ptr, not a reference into the map: they
  // release overlay_cache_mutex_ on return, and a concurrent
  // clearOverlayCaches() would destroy a referenced vector under a renderer.
  // Ownership costs nothing on the hot path (no data is copied).
  struct BpinAp
  {
    odb::Point point;
    odb::dbTechLayer* layer;
    odb::dbNet* net;  // may be null (unconnected bterm)
    bool has_access;
  };
  using BpinApList = std::shared_ptr<const std::vector<BpinAp>>;
  using GridList = std::shared_ptr<const std::vector<int>>;
  BpinApList bpinAccessPoints(odb::dbBlock* block) const;
  GridList gcellGridX(odb::dbBlock* block) const;
  GridList gcellGridY(odb::dbBlock* block) const;
  GridList gcellGrid(odb::PtrMap<odb::dbBlock, GridList>& cache,
                     odb::dbBlock* block,
                     const std::function<void(odb::dbGCellGrid*,
                                              std::vector<int>&)>& fill) const;
  void clearOverlayCaches() const;
  // Both require overlay_cache_mutex_; see the definitions.
  void dropOverlayCaches() const;
  void dropOverlayCachesIfStale(uint64_t rev) const;

  // Pseudo-layer painters used by renderTileBuffer (one per overlay).
  // Callers gate on the visibility flag; painters handle the rest.  All
  // share one signature so pseudoLayerDefs() can dispatch by table.
  void drawAccessPointsLayer(std::vector<unsigned char>& image,
                             odb::dbBlock* block,
                             const TileFrame& frame,
                             const TileVisibility& vis) const;
  void drawRegionsLayer(std::vector<unsigned char>& image,
                        odb::dbBlock* block,
                        const TileFrame& frame,
                        const TileVisibility& vis) const;
  void drawMfgGridLayer(std::vector<unsigned char>& image,
                        odb::dbBlock* block,
                        const TileFrame& frame,
                        const TileVisibility& vis) const;
  void drawGcellGridLayer(std::vector<unsigned char>& image,
                          odb::dbBlock* block,
                          const TileFrame& frame,
                          const TileVisibility& vis) const;
  void drawRudyLayer(std::vector<unsigned char>& image,
                     odb::dbBlock* block,
                     const TileFrame& frame,
                     const TileVisibility& vis) const;
  void drawHeatMap(std::vector<unsigned char>& image,
                   gui::HeatMapDataSource& source,
                   const TileFrame& frame) const;
  std::shared_ptr<gui::HeatMapDataSource> getHeatMapSource(
      const std::string& name) const;

  // Registry of the self-painting pseudo layers: layer name -> visibility
  // flag -> painter -> paint order.  Single source of truth for the
  // renderTileBuffer dispatch, the pseudo-layer guard and saveImage's
  // layers_to_render — adding an overlay means adding one entry (plus the
  // client layer).
  //
  // `z_index` is only read by saveImageLayerOrder(); see that function for why
  // the value has to mirror the client's.
  struct PseudoLayerDef
  {
    const char* name;
    bool TileVisibility::*flag;
    void (TileGenerator::*painter)(std::vector<unsigned char>&,
                                   odb::dbBlock*,
                                   const TileFrame&,
                                   const TileVisibility&) const;
    int z_index;
  };
  static const std::array<PseudoLayerDef, 5>& pseudoLayerDefs();
  // Draw a rect's edges clamped to the tile (die/core/region outlines).
  void outlineRectInTile(std::vector<unsigned char>& image,
                         const odb::Rect& r,
                         const Color& c,
                         const TileFrame& frame) const;
  mutable std::mutex heatmap_mutex_;
  mutable std::map<std::string, std::shared_ptr<gui::HeatMapDataSource>>
      heatmaps_;
  mutable std::mutex overlay_cache_mutex_;
  mutable odb::PtrMap<odb::dbBlock, BpinApList> bpin_ap_cache_;
  mutable odb::PtrMap<odb::dbBlock, GridList> gcell_x_cache_;
  mutable odb::PtrMap<odb::dbBlock, GridList> gcell_y_cache_;
  // The Search::revision() the three caches above were built at; see
  // dropOverlayCachesIfStale.
  mutable uint64_t overlay_cache_revision_ = 0;

  // User text labels (2.12).  Global design annotations; see addLabel().
  mutable std::mutex labels_mutex_;
  std::vector<StoredLabel> labels_;
  int next_label_id_ = 0;

  static constexpr int kTileSizeInPixel = 256;
};

struct TimingPathSummary;

std::pair<odb::dbITerm*, odb::dbBTerm*> resolvePin(odb::dbBlock* block,
                                                   const std::string& pin_name);

std::tuple<odb::dbITerm*, odb::dbBTerm*, const ChipletNode*> resolvePin(
    const std::vector<ChipletNode>& chiplets,
    const std::string& pin_name);

void collectNetShapes(odb::dbNet* net,
                      odb::dbITerm* drv_iterm,
                      odb::dbBTerm* drv_bterm,
                      odb::dbITerm* snk_iterm,
                      odb::dbBTerm* snk_bterm,
                      const Color& color,
                      std::vector<ColoredRect>& rects,
                      std::vector<FlightLine>& lines,
                      const odb::dbTransform& xfm);

void collectTimingPathShapes(odb::dbBlock* block,
                             const TimingPathSummary& path,
                             std::vector<ColoredRect>& rects,
                             std::vector<FlightLine>& lines);

void collectTimingPathShapes(const std::vector<ChipletNode>& chiplets,
                             const TimingPathSummary& path,
                             std::vector<ColoredRect>& rects,
                             std::vector<FlightLine>& lines);

// ── JSON serialization helpers for TileGenerator responses ──

boost::json::object serializeTechResponse(const TileGenerator& gen);
boost::json::object serializeBoundsResponse(const TileGenerator& gen,
                                            bool shapes_ready);

}  // namespace web
