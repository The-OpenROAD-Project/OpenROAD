// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "tile_generator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <numbers>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "boost/json/array.hpp"
#include "color.h"
#include "db_sta/dbSta.hh"
#include "font_atlas.h"
#include "glyph_cache.h"
#include "gui/gui.h"
#include "gui/heatMap.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "request_handler.h"
#include "search.h"
#include "third-party/lodepng/lodepng.h"
#include "timing_report.h"
#include "utl/Logger.h"
#include "utl/algorithms.h"
#include "web_painter.h"

namespace web {

int dbuPrecision(const double dbu_per_micron)
{
  if (dbu_per_micron <= 0) {
    return 0;
  }
  // The epsilon defends against a libm whose log10 returns a hair above an
  // exact integer for a power of ten (3.0000000000000004 for 1000), which would
  // ceil() to one digit too many.  It cannot round the result down: log10 of a
  // non-power-of-ten integer is never that close to an integer from above.
  // std::max guards a sub-unity scale, which dbDatabase cannot currently report
  // (getDbuPerMicron returns uint32_t and 0 is handled above).
  return std::max(
      0, static_cast<int>(std::ceil(std::log10(dbu_per_micron) - 1e-9)));
}

std::string dbuToMicronString(const int dbu, const double dbu_per_micron)
{
  if (dbu_per_micron <= 0) {
    return std::to_string(dbu);
  }
  return utl::to_numeric_string(dbu / dbu_per_micron,
                                dbuPrecision(dbu_per_micron));
}

namespace {

// Supersample factor for band-limited tile rasterization (anti-moiré).  The
// tile is rendered at kCoverageSupersample x the output resolution and then
// Lanczos-2 decimated.  S=2 is sufficient to suppress the bump-array beat
// (DSP-validated); larger S only adds cost.
constexpr int kCoverageSupersample = 2;

// Extra binomial prefilter convolved into the Lanczos-2 taps, in source
// (super-pixel) space.  Lanczos-2 alone is a SOFT filter that leaks ~10-20 %
// just below the output Nyquist, so a dense periodic array (bumps, vias, dense
// routing) whose pitch lands near the output Nyquist — the worst moiré-beat
// regime — survives the decimation as a low-frequency beat.  [1,2,1]/4 has an
// EXACT zero at the source Nyquist and unit DC gain: it deepens the stopband so
// that near-Nyquist tone is nulled instead of leaked, while leaving the local
// MEAN (and any resolved detail well below Nyquist) untouched.  Because it is
// DC-preserving it can only band-limit, never merge geometry into an opaque
// block — it cannot reintroduce the rejected "merged sheet" artifact.
constexpr std::array<double, 3> kLanczosPrefilterBinomial = {0.25, 0.5, 0.25};

constexpr float kPinMarkerSizeRatio = 0.02;
constexpr int kMinPinMarkerSize = 8;
constexpr int kMinPinNameSizePixels = 20;
constexpr int kPinLabelFontHeight = 14;  // pre-baked atlas size for pin labels
constexpr int kItermLabelFontHeight = 10;  // atlas size for ITerm pin labels
constexpr int kMinItermLabelBoxPx = 10;    // min pin-box pixel dim for labels
constexpr int kMinInstNameFontPx = 10;     // minimum readable font size
constexpr int kMaxInstNameFontPx = 40;     // cap font size for large macros
constexpr int kMinInstNameBoxPx = 20;      // min instance pixel dim for names
// Minimum on-screen feature size (output CSS px) below which geometry is CULLED
// at the search level instead of drawn.  A regular sub-pixel array (dense
// bumps/vias) cannot be drawn both discretely (→ moiré) and band-limited (→ a
// merged "sheet"); like the Qt GUI, we sidestep the dilemma by not returning
// what is too small to read.  At/above this size each feature is rasterized
// normally and the supersample + Lanczos downsample only anti-aliases it.
//
// The Qt GUI uses TWO limits (layoutViewer.cpp): shapeSizeLimit() =
// nominalViewableResolution = 5 px for shapes, but instanceSizeLimit() =
// fineViewableResolution = 1 px for instances (and 0 in module/detailed view).
// We deliberately apply this single 5 px limit to ALL searches, INCLUDING
// instances — i.e. more aggressively than Qt for instances — so dense bump
// arrays (which are kPhysBump instances) vanish at zoom-out; a 1 px instance
// limit would redraw them in the 1–5 px band and bring the moiré beat back.
// The `_modules` overview is the exception (passes 0, mirroring Qt's module
// view) so the module-colored map is not emptied at zoom-out.
//
// Note the cull is anisotropic, matching the Qt predicates: instances, rows and
// blockages are culled by HEIGHT (MinHeightPredicate, box.dy()); shapes/vias by
// the LARGER dimension (MinSizePredicate, box.maxDXDY()), so a long thin wire
// survives on its length.
constexpr double kMinViewablePx = 5.0;

// Die/core/region outline color: Qt pen Qt::gray width 0 (drawChip,
// renderThread.cpp:1174).
constexpr Color kOutlineGray{.r = 128, .g = 128, .b = 128, .a = 255};

// DBU -> tile-pixel conversion shared by the drawing primitives, in double.
// Unclamped: an oblique segment must be converted through these and clipped
// by drawLine, because saturating x and y independently (as the int
// overloads below do) rotates the segment instead of shortening it.
inline double toPxXd(int dbu_x, const TileFrame& frame)
{
  return frame.pxX(dbu_x);
}

// Y is flipped: DBU grows up, pixel rows grow down.  `dim` is the side of the
// buffer being painted: tile_px for a plain tile, tile_px*kCoverageSupersample
// on the supersampled render path (pass bufferDim(image) there).
inline double toPxYd(int dbu_y, const TileFrame& frame, int dim)
{
  return dim - 1 - frame.pxY(dbu_y);
}

// Clamped int form, so a far-outside coordinate can't overflow the cast at
// extreme zoom.  Only safe for POINTS (dots, glyph anchors, circle/X centres)
// and for the corners of axis-aligned rects, where losing the exact off-tile
// coordinate cannot tilt anything; oblique segments must use toPxXd/toPxYd.
inline int toPxX(int dbu_x, const TileFrame& frame)
{
  return static_cast<int>(std::clamp(frame.pxX(dbu_x), -1.0e7, 1.0e7));
}

// Clamped int form; see toPxX for when it may be used.  Deliberately clamps the
// offset rather than delegating to toPxYd and clamping the flipped result — the
// two differ by `dim` once saturated.
inline int toPxY(int dbu_y, const TileFrame& frame, int dim)
{
  return dim - 1
         - static_cast<int>(std::clamp(frame.pxY(dbu_y), -1.0e7, 1.0e7));
}

// Width in buffer pixels of a line meant to read as ONE CSS pixel: the overlay
// hairlines (die/core/region outlines, grid dots and lines).  Authoring a
// literal 1 px is wrong twice over — it reads a third as thick as everything
// else on a 3x display, and on the supersampled render path it fades to ~1/S
// intensity once lanczos2Downsample decimates the buffer back.  See
// penWidthCss for the 3 CSS px pen the gui::Painter ops default to.
inline int hairlineCss(const TileFrame& frame)
{
  return std::max(1, static_cast<int>(std::lround(frame.px_per_css)));
}

}  // namespace

void TileVisibility::parseFromJson(const boost::json::object& json)
{
  struct BoolField
  {
    const char* key;
    bool TileVisibility::*field;
    bool default_val;
  };

  // clang-format off
  // NOLINTBEGIN(modernize-use-designated-initializers)
  static const BoolField kFields[] = {
    {"stdcells",           &TileVisibility::stdcells,           true},
    {"macros",             &TileVisibility::macros,             true},
    {"pad_input",          &TileVisibility::pad_input,          true},
    {"pad_output",         &TileVisibility::pad_output,         true},
    {"pad_inout",          &TileVisibility::pad_inout,          true},
    {"pad_power",          &TileVisibility::pad_power,          true},
    {"pad_spacer",         &TileVisibility::pad_spacer,         true},
    {"pad_areaio",         &TileVisibility::pad_areaio,         true},
    {"pad_other",          &TileVisibility::pad_other,          true},
    {"phys_fill",          &TileVisibility::phys_fill,          true},
    {"phys_endcap",        &TileVisibility::phys_endcap,        true},
    {"phys_welltap",       &TileVisibility::phys_welltap,       true},
    {"phys_tie",           &TileVisibility::phys_tie,           true},
    {"phys_antenna",       &TileVisibility::phys_antenna,       true},
    {"phys_cover",         &TileVisibility::phys_cover,         true},
    {"phys_bump",          &TileVisibility::phys_bump,          true},
    {"phys_other",         &TileVisibility::phys_other,         true},
    {"std_bufinv",         &TileVisibility::std_bufinv,         true},
    {"std_bufinv_timing",  &TileVisibility::std_bufinv_timing,  true},
    {"std_clock_bufinv",   &TileVisibility::std_clock_bufinv,   true},
    {"std_clock_gate",     &TileVisibility::std_clock_gate,     true},
    {"std_level_shift",    &TileVisibility::std_level_shift,    true},
    {"std_sequential",     &TileVisibility::std_sequential,     true},
    {"std_combinational",  &TileVisibility::std_combinational,  true},
    {"net_signal",         &TileVisibility::net_signal,         true},
    {"net_power",          &TileVisibility::net_power,          true},
    {"net_ground",         &TileVisibility::net_ground,         true},
    {"net_clock",          &TileVisibility::net_clock,          true},
    {"net_reset",          &TileVisibility::net_reset,          true},
    {"net_tieoff",         &TileVisibility::net_tieoff,         true},
    {"net_scan",           &TileVisibility::net_scan,           true},
    {"net_analog",         &TileVisibility::net_analog,         true},
    {"routing",            &TileVisibility::routing,            true},
    {"routing_segments",   &TileVisibility::routing_segments,   true},
    {"routing_vias",       &TileVisibility::routing_vias,       true},
    {"special_nets",       &TileVisibility::special_nets,       true},
    {"srouting_segments",  &TileVisibility::srouting_segments,  true},
    {"srouting_vias",      &TileVisibility::srouting_vias,      true},
    {"labels",             &TileVisibility::labels,             true},
    {"pins",               &TileVisibility::pins,               true},
    {"pin_markers",        &TileVisibility::pin_markers,        true},
    {"pin_names",          &TileVisibility::pin_names,          true},
    {"access_points",      &TileVisibility::access_points,      false},
    {"regions",            &TileVisibility::regions,            true},
    {"mfg_grid",           &TileVisibility::mfg_grid,           false},
    {"gcell_grid",         &TileVisibility::gcell_grid,         false},
    {"rudy",               &TileVisibility::rudy,               false},
    {"inst_names",         &TileVisibility::inst_names,         true},
    {"inst_pins",          &TileVisibility::inst_pins,          true},
    {"inst_pin_names",     &TileVisibility::inst_pin_names,     true},
    {"blockages",              &TileVisibility::blockages,              true},
    {"placement_blockages",    &TileVisibility::placement_blockages,    true},
    {"routing_obstructions",   &TileVisibility::routing_obstructions,   true},
    {"fills",                  &TileVisibility::fills,                  false},
    {"rows",                   &TileVisibility::rows,                   false},
    {"tracks_pref",            &TileVisibility::tracks_pref,            false},
    {"tracks_non_pref",        &TileVisibility::tracks_non_pref,        false},
    {"detailed",               &TileVisibility::detailed,               false},
    {"debug",                  &TileVisibility::debug,                  false},
    {"debug_renderers",        &TileVisibility::debug_renderers,        false},
    {"debug_live",             &TileVisibility::debug_live,             false},
  };
  // NOLINTEND(modernize-use-designated-initializers)
  // clang-format on

  // Visibility flags are nominally always sent by the web frontend, but
  // tests and the saveImage Tcl entry point can pass partial payloads;
  // fall back to the per-field default when a flag is omitted.
  for (const auto& f : kFields) {
    this->*(f.field) = jsonOr<bool>(json, f.key, f.default_val);
  }

  // Bound the visibility-filter sizes so a malformed/oversized payload
  // can't make us allocate unbounded memory or thrash later contains()
  // checks.  Real designs never come close to this cap (a few dozen
  // layers and a handful of chiplets); the limit only kicks in on bad
  // input.
  constexpr size_t kMaxVisibilityEntries = 10000;

  visible_layers.clear();
  has_visible_layers = false;
  if (auto it = json.find("visible_layers"); it != json.end()) {
    has_visible_layers = true;
    const auto& arr = it->value().as_array();
    const size_t count = std::min(arr.size(), kMaxVisibilityEntries);
    for (size_t i = 0; i < count; ++i) {
      visible_layers.emplace(arr[i].as_string());
    }
  }

  visible_chiplets.clear();
  has_visible_chiplets = false;
  if (auto it = json.find("visible_chiplets"); it != json.end()) {
    has_visible_chiplets = true;
    const auto& arr = it->value().as_array();
    const size_t count = std::min(arr.size(), kMaxVisibilityEntries);
    for (size_t i = 0; i < count; ++i) {
      visible_chiplets.emplace(arr[i].as_string());
    }
  }

  // Per-layer fill pattern for the requested layer (int mirrors FillPattern /
  // gui::Painter::Brush).  Defaults to solid; clamp unknown values so a bad
  // payload can't index outside the enum.
  const int64_t pattern
      = jsonOr<int64_t>(json, "pattern", static_cast<int>(FillPattern::kSolid));
  fill_pattern = (pattern >= static_cast<int>(FillPattern::kNone)
                  && pattern <= static_cast<int>(FillPattern::kDots))
                     ? static_cast<FillPattern>(pattern)
                     : FillPattern::kSolid;

  // ── Selectability peers ──
  // clang-format off
  // NOLINTBEGIN(modernize-use-designated-initializers)
  static const BoolField kSelectableFields[] = {
    {"s_stdcells",             &TileVisibility::stdcells_selectable,             true},
    {"s_macros",               &TileVisibility::macros_selectable,               true},
    {"s_pad_input",            &TileVisibility::pad_input_selectable,            true},
    {"s_pad_output",           &TileVisibility::pad_output_selectable,           true},
    {"s_pad_inout",            &TileVisibility::pad_inout_selectable,            true},
    {"s_pad_power",            &TileVisibility::pad_power_selectable,            true},
    {"s_pad_spacer",           &TileVisibility::pad_spacer_selectable,           true},
    {"s_pad_areaio",           &TileVisibility::pad_areaio_selectable,           true},
    {"s_pad_other",            &TileVisibility::pad_other_selectable,            true},
    {"s_phys_fill",            &TileVisibility::phys_fill_selectable,            true},
    {"s_phys_endcap",          &TileVisibility::phys_endcap_selectable,          true},
    {"s_phys_welltap",         &TileVisibility::phys_welltap_selectable,         true},
    {"s_phys_tie",             &TileVisibility::phys_tie_selectable,             true},
    {"s_phys_antenna",         &TileVisibility::phys_antenna_selectable,         true},
    {"s_phys_cover",           &TileVisibility::phys_cover_selectable,           true},
    {"s_phys_bump",            &TileVisibility::phys_bump_selectable,            true},
    {"s_phys_other",           &TileVisibility::phys_other_selectable,           true},
    {"s_std_bufinv",           &TileVisibility::std_bufinv_selectable,           true},
    {"s_std_bufinv_timing",    &TileVisibility::std_bufinv_timing_selectable,    true},
    {"s_std_clock_bufinv",     &TileVisibility::std_clock_bufinv_selectable,     true},
    {"s_std_clock_gate",       &TileVisibility::std_clock_gate_selectable,       true},
    {"s_std_level_shift",      &TileVisibility::std_level_shift_selectable,      true},
    {"s_std_sequential",       &TileVisibility::std_sequential_selectable,       true},
    {"s_std_combinational",    &TileVisibility::std_combinational_selectable,    true},
    {"s_net_signal",           &TileVisibility::net_signal_selectable,           true},
    {"s_net_power",            &TileVisibility::net_power_selectable,            true},
    {"s_net_ground",           &TileVisibility::net_ground_selectable,           true},
    {"s_net_clock",            &TileVisibility::net_clock_selectable,            true},
    {"s_net_reset",            &TileVisibility::net_reset_selectable,            true},
    {"s_net_tieoff",           &TileVisibility::net_tieoff_selectable,           true},
    {"s_net_scan",             &TileVisibility::net_scan_selectable,             true},
    {"s_net_analog",           &TileVisibility::net_analog_selectable,           true},
    {"s_pins",                 &TileVisibility::pins_selectable,                 true},
    {"s_inst_pins",            &TileVisibility::inst_pins_selectable,            true},
    {"s_placement_blockages",  &TileVisibility::placement_blockages_selectable,  true},
    {"s_routing_obstructions", &TileVisibility::routing_obstructions_selectable, true},
  };
  // NOLINTEND(modernize-use-designated-initializers)
  // clang-format on
  for (const auto& f : kSelectableFields) {
    this->*(f.field) = jsonOr<bool>(json, f.key, f.default_val);
  }

  selectable_layers.clear();
  has_selectable_layers = false;
  if (auto it = json.find("selectable_layers"); it != json.end()) {
    has_selectable_layers = true;
    const auto& arr = it->value().as_array();
    const size_t count = std::min(arr.size(), kMaxVisibilityEntries);
    for (size_t i = 0; i < count; ++i) {
      selectable_layers.emplace(arr[i].as_string());
    }
  }

  // Per-site flags are only consulted when rows are visible; skip the
  // full-object scan otherwise.
  sites.clear();
  site_selectable.clear();
  if (rows) {
    constexpr std::string_view kVisPrefix = "site_";
    constexpr std::string_view kSelPrefix = "s_site_";
    for (const auto& [key, value] : json) {
      const std::string_view k(key.data(), key.size());
      if (k.starts_with(kSelPrefix)) {
        site_selectable.emplace(std::string(k.substr(kSelPrefix.size())),
                                value.as_bool());
      } else if (k.starts_with(kVisPrefix)) {
        sites.emplace(std::string(k.substr(kVisPrefix.size())),
                      value.as_bool());
      }
    }
  }
}

bool TileVisibility::isChipletVisible(const std::string& path) const
{
  if (!has_visible_chiplets) {
    return true;
  }
  return visible_chiplets.contains(path);
}

bool TileVisibility::isSiteVisible(const std::string& site_name) const
{
  if (!rows) {
    return false;
  }
  auto it = sites.find(site_name);
  return it != sites.end() && it->second;
}

bool TileVisibility::isNetVisible(odb::dbNet* net) const
{
  switch (net->getSigType().getValue()) {
    case odb::dbSigType::SIGNAL:
      return net_signal;
    case odb::dbSigType::POWER:
      return net_power;
    case odb::dbSigType::GROUND:
      return net_ground;
    case odb::dbSigType::CLOCK:
      return net_clock;
    case odb::dbSigType::RESET:
      return net_reset;
    case odb::dbSigType::TIEOFF:
      return net_tieoff;
    case odb::dbSigType::SCAN:
      return net_scan;
    case odb::dbSigType::ANALOG:
      return net_analog;
  }
  return true;
}

bool TileVisibility::isNetSelectable(odb::dbNet* net) const
{
  switch (net->getSigType().getValue()) {
    case odb::dbSigType::SIGNAL:
      return net_signal_selectable;
    case odb::dbSigType::POWER:
      return net_power_selectable;
    case odb::dbSigType::GROUND:
      return net_ground_selectable;
    case odb::dbSigType::CLOCK:
      return net_clock_selectable;
    case odb::dbSigType::RESET:
      return net_reset_selectable;
    case odb::dbSigType::TIEOFF:
      return net_tieoff_selectable;
    case odb::dbSigType::SCAN:
      return net_scan_selectable;
    case odb::dbSigType::ANALOG:
      return net_analog_selectable;
  }
  return true;
}

InstCategory classifyInstance(odb::dbInst* inst, sta::dbSta* sta)
{
  odb::dbMaster* master = inst->getMaster();
  const odb::dbMasterType mtype = master->getType();

  if (sta) {
    using IT = sta::dbSta::InstType;
    switch (sta->getInstanceType(inst)) {
      case IT::BLOCK:
        return InstCategory::kMacros;
      case IT::PAD_INPUT:
        return InstCategory::kPadInput;
      case IT::PAD_OUTPUT:
        return InstCategory::kPadOutput;
      case IT::PAD_INOUT:
        return InstCategory::kPadInout;
      case IT::PAD_POWER:
        return InstCategory::kPadPower;
      case IT::PAD_SPACER:
        return InstCategory::kPadSpacer;
      case IT::PAD_AREAIO:
        return InstCategory::kPadAreaIO;
      case IT::PAD:
        return InstCategory::kPadOther;
      case IT::ENDCAP:
        return InstCategory::kPhysEndcap;
      case IT::FILL:
        return InstCategory::kPhysFill;
      case IT::TAPCELL:
        return InstCategory::kPhysWelltap;
      case IT::TIE:
        return InstCategory::kPhysTie;
      case IT::ANTENNA:
        return InstCategory::kPhysAntenna;
      case IT::COVER:
        return InstCategory::kPhysCover;
      case IT::BUMP:
        return InstCategory::kPhysBump;
      case IT::LEF_OTHER:
        return InstCategory::kPhysOther;
      case IT::STD_BUF:
      case IT::STD_INV:
        return InstCategory::kStdBufInv;
      case IT::STD_BUF_TIMING_REPAIR:
      case IT::STD_INV_TIMING_REPAIR:
        return InstCategory::kStdBufInvTiming;
      case IT::STD_BUF_CLK_TREE:
      case IT::STD_INV_CLK_TREE:
        return InstCategory::kStdClockBufInv;
      case IT::STD_CLOCK_GATE:
        return InstCategory::kStdClockGate;
      case IT::STD_LEVEL_SHIFT:
        return InstCategory::kStdLevelShift;
      case IT::STD_SEQUENTIAL:
        return InstCategory::kStdSequential;
      case IT::STD_COMBINATIONAL:
        return InstCategory::kStdCombinational;
      case IT::STD_CELL:
      case IT::STD_PHYSICAL:
      case IT::STD_OTHER:
      default:
        return InstCategory::kStdCells;
    }
  }

  // Fallback: dbMasterType-only classification (no Liberty)
  if (mtype.isBlock()) {
    return InstCategory::kMacros;
  }
  if (mtype.isPad()) {
    if (mtype == odb::dbMasterType::PAD_INPUT) {
      return InstCategory::kPadInput;
    }
    if (mtype == odb::dbMasterType::PAD_OUTPUT) {
      return InstCategory::kPadOutput;
    }
    if (mtype == odb::dbMasterType::PAD_INOUT) {
      return InstCategory::kPadInout;
    }
    if (mtype == odb::dbMasterType::PAD_POWER) {
      return InstCategory::kPadPower;
    }
    if (mtype == odb::dbMasterType::PAD_SPACER) {
      return InstCategory::kPadSpacer;
    }
    if (mtype == odb::dbMasterType::PAD_AREAIO) {
      return InstCategory::kPadAreaIO;
    }
    return InstCategory::kPadOther;
  }
  if (mtype.isEndCap()) {
    return InstCategory::kPhysEndcap;
  }
  if (master->isFiller()) {
    return InstCategory::kPhysFill;
  }
  if (mtype == odb::dbMasterType::CORE_WELLTAP) {
    return InstCategory::kPhysWelltap;
  }
  if (mtype == odb::dbMasterType::CORE_TIEHIGH
      || mtype == odb::dbMasterType::CORE_TIELOW) {
    return InstCategory::kPhysTie;
  }
  if (mtype == odb::dbMasterType::CORE_ANTENNACELL) {
    return InstCategory::kPhysAntenna;
  }
  if (mtype.isCover()) {
    if (mtype == odb::dbMasterType::COVER_BUMP) {
      return InstCategory::kPhysBump;
    }
    return InstCategory::kPhysCover;
  }
  if (mtype == odb::dbMasterType::CORE_SPACER
      || inst->getSourceType() == odb::dbSourceType::DIST) {
    return InstCategory::kPhysOther;
  }
  return InstCategory::kStdCells;
}

bool TileVisibility::isInstVisible(odb::dbInst* inst, sta::dbSta* sta) const
{
  return isCategoryVisible(classifyInstance(inst, sta));
}

bool TileVisibility::isCategoryVisible(InstCategory cat) const
{
  switch (cat) {
    case InstCategory::kStdCells:
      return stdcells;
    case InstCategory::kMacros:
      return macros;
    case InstCategory::kPadInput:
      return pad_input;
    case InstCategory::kPadOutput:
      return pad_output;
    case InstCategory::kPadInout:
      return pad_inout;
    case InstCategory::kPadPower:
      return pad_power;
    case InstCategory::kPadSpacer:
      return pad_spacer;
    case InstCategory::kPadAreaIO:
      return pad_areaio;
    case InstCategory::kPadOther:
      return pad_other;
    case InstCategory::kPhysEndcap:
      return phys_endcap;
    case InstCategory::kPhysFill:
      return phys_fill;
    case InstCategory::kPhysWelltap:
      return phys_welltap;
    case InstCategory::kPhysTie:
      return phys_tie;
    case InstCategory::kPhysAntenna:
      return phys_antenna;
    case InstCategory::kPhysCover:
      return phys_cover;
    case InstCategory::kPhysBump:
      return phys_bump;
    case InstCategory::kPhysOther:
      return phys_other;
    case InstCategory::kStdBufInv:
      return std_bufinv;
    case InstCategory::kStdBufInvTiming:
      return std_bufinv_timing;
    case InstCategory::kStdClockBufInv:
      return std_clock_bufinv;
    case InstCategory::kStdClockGate:
      return std_clock_gate;
    case InstCategory::kStdLevelShift:
      return std_level_shift;
    case InstCategory::kStdSequential:
      return std_sequential;
    case InstCategory::kStdCombinational:
      return std_combinational;
  }
  return stdcells;
}

bool TileVisibility::isInstSelectable(odb::dbInst* inst, sta::dbSta* sta) const
{
  switch (classifyInstance(inst, sta)) {
    case InstCategory::kStdCells:
      return stdcells_selectable;
    case InstCategory::kMacros:
      return macros_selectable;
    case InstCategory::kPadInput:
      return pad_input_selectable;
    case InstCategory::kPadOutput:
      return pad_output_selectable;
    case InstCategory::kPadInout:
      return pad_inout_selectable;
    case InstCategory::kPadPower:
      return pad_power_selectable;
    case InstCategory::kPadSpacer:
      return pad_spacer_selectable;
    case InstCategory::kPadAreaIO:
      return pad_areaio_selectable;
    case InstCategory::kPadOther:
      return pad_other_selectable;
    case InstCategory::kPhysEndcap:
      return phys_endcap_selectable;
    case InstCategory::kPhysFill:
      return phys_fill_selectable;
    case InstCategory::kPhysWelltap:
      return phys_welltap_selectable;
    case InstCategory::kPhysTie:
      return phys_tie_selectable;
    case InstCategory::kPhysAntenna:
      return phys_antenna_selectable;
    case InstCategory::kPhysCover:
      return phys_cover_selectable;
    case InstCategory::kPhysBump:
      return phys_bump_selectable;
    case InstCategory::kPhysOther:
      return phys_other_selectable;
    case InstCategory::kStdBufInv:
      return std_bufinv_selectable;
    case InstCategory::kStdBufInvTiming:
      return std_bufinv_timing_selectable;
    case InstCategory::kStdClockBufInv:
      return std_clock_bufinv_selectable;
    case InstCategory::kStdClockGate:
      return std_clock_gate_selectable;
    case InstCategory::kStdLevelShift:
      return std_level_shift_selectable;
    case InstCategory::kStdSequential:
      return std_sequential_selectable;
    case InstCategory::kStdCombinational:
      return std_combinational_selectable;
  }
  return stdcells_selectable;
}

bool TileVisibility::isSiteSelectable(const std::string& site_name) const
{
  auto it = site_selectable.find(site_name);
  // Default: selectable when not explicitly listed.
  return it == site_selectable.end() || it->second;
}

bool TileVisibility::isLayerSelectable(const std::string& layer_name) const
{
  // When the client doesn't transmit a list, treat all layers as selectable.
  return !has_selectable_layers || selectable_layers.contains(layer_name);
}

//////////////////////////////////////////////////

TileGenerator::TileGenerator(odb::dbDatabase* db,
                             sta::dbSta* sta,
                             utl::Logger* logger)
    : db_(db),
      sta_(sta),
      logger_(logger),
      search_(std::make_unique<Search>(logger))
{
  odb::dbChip* chip = db_->getChip();
  if (chip) {
    search_->setTopChip(chip);
  }
  computePinLabelMargin();
  // Drop the PNG tile cache (and push a client refresh) whenever a design edit
  // invalidates a spatial index.  eagerInit() suppresses this while it
  // rebuilds.
  search_->setOnModified([this] { onDesignChanged(); });
}

TileGenerator::~TileGenerator() = default;

void TileGenerator::eagerInit()
{
  // Reindexing clears every spatial index, which would otherwise fire
  // onDesignChanged() many times (and risk a refresh push racing the socket
  // accept).  We already clear the tile cache and drive a refresh explicitly
  // here / in the session, so suppress the per-index callbacks for the
  // duration.  A scope guard restores the flag on every exit path, so a future
  // early return or exception can't leave invalidation permanently disabled.
  suppress_design_changed_.store(true);
  struct SuppressGuard
  {
    std::atomic_bool& flag;
    ~SuppressGuard() { flag.store(false); }
  } suppress_guard{suppress_design_changed_};

  // Invalidate the chiplet cache: setTopChip below may swap to a fresh
  // dbChip whose ChipletNode addresses (dbBlock*, dbChip*, etc.) differ
  // from the previous design's.
  {
    std::lock_guard lock(chiplets_mutex_);
    chiplets_cache_.clear();
    chiplets_cache_valid_ = false;
  }
  // Overlay caches are keyed by dbBlock*, which may be stale after a
  // design swap.
  clearOverlayCaches();

  odb::dbChip* chip = db_->getChip();
  if (chip) {
    search_->setTopChip(chip);
  }
  // Index every block reachable via dbChipInst, not just the top block,
  // so the recursive tile renderer can query searchInsts/searchBoxShapes
  // for any chiplet's master block.
  if (chip) {
    for (const ChipletNode& node : chiplets()) {
      if (node.block) {
        search_->eagerInit(node.block);
      }
    }
  }
  computePinLabelMargin();

  // A reload can replace the dbTech and reuse its memory address, which would
  // make stale entries in the cache compare equal to a freshly allocated tech.
  // Clearing here ties cache lifetime to design loading.
  {
    std::lock_guard lock(layer_colors_mutex_);
    layer_colors_by_tech_.clear();
  }
  // Same address-reuse hazard, for the dbMaster/dbVia keys: a reload's fresh
  // objects can land where the old ones were, so a revision bump alone would
  // not prove the keys still mean the same thing.  Dropped unconditionally
  // here, where a reload is known to be in progress.
  {
    std::lock_guard lock(geom_cache_mutex_);
    geom_cache_.reset();
  }

  // Tiles depend on the design geometry, so a reload invalidates every cached
  // PNG.  Clearing here ties cache lifetime to design loading.
  {
    std::lock_guard lock(tile_cache_mutex_);
    tile_cache_lru_.clear();
    tile_cache_index_.clear();
  }
  // suppress_guard restores suppress_design_changed_ to false on return.
}

void TileGenerator::setDesignChangedCallback(std::function<void()> cb)
{
  std::lock_guard lock(design_changed_cb_mutex_);
  design_changed_cb_ = std::move(cb);
}

void TileGenerator::onDesignChanged()
{
  // Suppressed during eagerInit()'s bulk reindex (see eagerInit).
  if (suppress_design_changed_.load()) {
    return;
  }

  // A design edit invalidated a spatial index, so every cached PNG may now be
  // stale.  Drop the whole cache; tiles re-render lazily on the next request.
  {
    std::lock_guard lock(tile_cache_mutex_);
    tile_cache_lru_.clear();
    tile_cache_index_.clear();
  }

  // The geometry cache is NOT dropped here.  This hook is debounced (see
  // below), so an edit arriving while an index is already invalid never
  // reaches it — geomCache() polls Search::revision() instead, which is not.
  // The overlay caches (access points, gcell grids) are derived from the design
  // in the same way and an empty entry is indistinguishable from "no data" —
  // caching the gcell grid before global_route created it would keep the
  // overlay blank until a page reload — so they poll the revision too.

  // Tell connected clients to re-request (mirrors Qt's fullRepaint).  The
  // Search-level debounce (announceModified fires only on a valid→invalid
  // transition) keeps this from flooding during batch edits.
  std::function<void()> cb;
  {
    std::lock_guard lock(design_changed_cb_mutex_);
    cb = design_changed_cb_;
  }
  if (cb) {
    cb();
  }
}

bool TileGenerator::tileCacheGet(const std::string& key,
                                 std::vector<unsigned char>& out) const
{
  std::lock_guard lock(tile_cache_mutex_);
  const auto it = tile_cache_index_.find(key);
  if (it == tile_cache_index_.end()) {
    return false;
  }
  // Move the hit to the front (most-recently-used).
  tile_cache_lru_.splice(tile_cache_lru_.begin(), tile_cache_lru_, it->second);
  out = it->second->second;
  return true;
}

void TileGenerator::tileCachePut(std::string key,
                                 std::vector<unsigned char> png) const
{
  std::lock_guard lock(tile_cache_mutex_);
  const auto it = tile_cache_index_.find(key);
  if (it != tile_cache_index_.end()) {
    // Already present: refresh contents and promote to most-recent.
    it->second->second = std::move(png);
    tile_cache_lru_.splice(
        tile_cache_lru_.begin(), tile_cache_lru_, it->second);
    return;
  }
  tile_cache_lru_.emplace_front(key, std::move(png));
  tile_cache_index_[std::move(key)] = tile_cache_lru_.begin();
  if (tile_cache_lru_.size() > kTileCacheCap) {
    tile_cache_index_.erase(tile_cache_lru_.back().first);
    tile_cache_lru_.pop_back();
  }
}

size_t TileGenerator::tileCacheSize() const
{
  std::lock_guard lock(tile_cache_mutex_);
  return tile_cache_lru_.size();
}

namespace {

// Cheap-to-compute fingerprint of the current chiplet hierarchy:
// total dbChipInst count reachable from `root`.  If the count changes
// the cache is rebuilt — covers the common Tcl mutation patterns
// (create/destroy chiplet instances) that ODB doesn't notify about.
size_t countChipInsts(odb::dbChip* root)
{
  if (!root) {
    return 0;
  }
  size_t total = 0;
  std::vector<odb::dbChip*> stack{root};
  while (!stack.empty()) {
    odb::dbChip* curr = stack.back();
    stack.pop_back();
    for (odb::dbChipInst* inst : curr->getChipInsts()) {
      ++total;
      if (odb::dbChip* master = inst->getMasterChip()) {
        stack.push_back(master);
      }
    }
  }
  return total;
}

}  // namespace

const std::vector<ChipletNode>& TileGenerator::chiplets() const
{
  // ODB itself is not thread-safe, so callers serialize web requests
  // against design mutations.  The fingerprint check (root pointer +
  // dbChipInst count) only needs to detect *sequential* Tcl mutations
  // — taking the lock before reading root/count keeps the fingerprint
  // and the cached values consistent with each other.
  //
  // Lifetime contract: the returned reference is valid only as long as
  // eagerInit() does not run.  eagerInit() executes on design load /
  // reload, which is gated upstream against tile/select requests.  Hot-
  // path callers (renderTileBuffer, selectAt) MUST NOT trigger
  // eagerInit while iterating the returned vector.
  std::lock_guard lock(chiplets_mutex_);
  odb::dbChip* root = db_->getChip();
  const size_t inst_count = countChipInsts(root);
  const bool fingerprint_changed = chiplets_cache_root_ != root
                                   || chiplets_cache_inst_count_ != inst_count;
  if (!chiplets_cache_valid_ || fingerprint_changed) {
    chiplets_cache_ = collectChiplets(root);
    chiplets_cache_root_ = root;
    chiplets_cache_inst_count_ = inst_count;
    chiplets_cache_valid_ = true;
    ++chiplets_cache_generation_;
  }
  return chiplets_cache_;
}

uint64_t TileGenerator::chipletsGeneration() const
{
  // Refresh first so the counter reflects the live hierarchy rather than
  // whatever the last chiplets() caller saw.
  chiplets();
  std::lock_guard lock(chiplets_mutex_);
  return chiplets_cache_generation_;
}

void TileGenerator::computePinLabelMargin()
{
  odb::dbBlock* block = getBlock();
  if (!block) {
    pin_label_margin_dbu_ = 0;
    return;
  }
  const int pin_size = getPinMaxSize();
  if (pin_size <= 0) {
    pin_label_margin_dbu_ = 0;
    return;
  }
  int max_text_px = 0;
  const auto pin_font = fontAtlasGetFont(kPinLabelFontHeight);
  for (odb::dbBTerm* term : block->getBTerms()) {
    const int w = pin_font.textWidth(term->getName());
    max_text_px = std::max(w, max_text_px);
  }
  const int label_px = kMinPinNameSizePixels + 3 + max_text_px;
  pin_label_margin_dbu_ = label_px * pin_size / kMinPinNameSizePixels;
}

bool TileGenerator::shapesReady() const
{
  return search_->shapesReady();
}

/* static */
odb::Rect TileGenerator::toPixels(const TileFrame& frame, const odb::Rect& rect)
{
  return odb::Rect(std::floor(frame.pxX(rect.xMin())),
                   std::floor(frame.pxY(rect.yMin())),
                   std::ceil(frame.pxX(rect.xMax())),
                   std::ceil(frame.pxY(rect.yMax())));
}

// Default overlay pen width (3 CSS px) in this frame's pixels.  A fixed 3
// physical px is a third as thick, relative to everything else, on a 3x
// display.
static int penWidthCss(const TileFrame& frame)
{
  constexpr double kOverlayPenCssPx = 3.0;
  return std::max(
      1, static_cast<int>(std::lround(kOverlayPenCssPx * frame.px_per_css)));
}

// The tile origin in absolute (whole-design) pixel space, folded onto a
// `period`-pixel lattice.  Hatches and dot fills are phased off this so the
// pattern runs unbroken across a tile boundary, and they only ever use it
// modulo the period — so folding costs nothing and buys two things: the raw
// product overflows int at deep zoom (a DBU is worth thousands of pixels
// there), and the fold is non-negative, which keeps the phase right for a
// design whose bounds start left of the origin.
static int latticeAnchor(const double origin_dbu,
                         const double scale,
                         const int64_t period)
{
  const int64_t px = std::llround(origin_dbu * scale);
  return static_cast<int>(((px % period) + period) % period);
}

// Anchor for the FillPattern hatches.  kPatternLattice is a common multiple of
// every period in patternCovers(), so one fold serves them all.
static int patternAnchor(const double origin_dbu, const double scale)
{
  constexpr int64_t kPatternLattice = 24;
  return latticeAnchor(origin_dbu, scale, kPatternLattice);
}

// The frame of tile (x, y) on the grid that divides `bounds` into steps of
// `tile_dbu_size`, drawn at `scale` pixels per DBU.  One definition shared by
// the layer, overlay and heat-map tile paths: the browser stacks those three on
// top of each other, so any disagreement here slides an overlay off the shapes
// it annotates.
static TileFrame tileFrame(const odb::Rect& bounds,
                           const int x,
                           const int y,
                           const double tile_dbu_size,
                           const double scale,
                           const double px_per_css = 1.0)
{
  TileFrame frame;
  frame.origin_x = bounds.xMin() + x * tile_dbu_size;
  frame.origin_y = bounds.yMin() + y * tile_dbu_size;
  frame.scale = scale;
  frame.px_per_css = px_per_css;
  // Query/clip window: the tile rounded OUTWARD to whole DBU, so a shape
  // reaching even fractionally into the tile is still found and drawn.
  frame.cull
      = odb::Rect(static_cast<int>(std::floor(frame.origin_x)),
                  static_cast<int>(std::floor(frame.origin_y)),
                  static_cast<int>(std::ceil(frame.origin_x + tile_dbu_size)),
                  static_cast<int>(std::ceil(frame.origin_y + tile_dbu_size)));
  return frame;
}

// Tiles are square; recover the side length from a packed RGBA buffer so the
// drawing primitives work at any resolution — 256 (dpr=1) or the supersampled
// 256*dpr*S buffer used for band-limited rendering.  Callers in a hot loop pass
// the precomputed dimension via the `dim` parameter to skip this.
static int bufferDim(const std::vector<unsigned char>& image)
{
  return static_cast<int>(std::lround(std::sqrt(image.size() / 4.0)));
}

void TileGenerator::setPixel(std::vector<unsigned char>& image,
                             const int x,
                             const int y,
                             const Color& c,
                             int dim) const
{
  if (dim < 0) {
    dim = bufferDim(image);
  }
  if (x < 0 || x >= dim || y < 0 || y >= dim) {
    return;
  }
  const int index = (y * dim + x) * 4;
  image[index + 0] = c.r;
  image[index + 1] = c.g;
  image[index + 2] = c.b;
  image[index + 3] = c.a;
}

namespace {

// FillPattern (color.h) is a web-local mirror of gui::Painter::Brush so the
// tile server, the JS frontend and the Qt GUI all agree on the integer
// ordering the "pattern" request field carries.  Keep them locked together.
static_assert(static_cast<int>(FillPattern::kNone)
                      == static_cast<int>(gui::Painter::Brush::kNone)
                  && static_cast<int>(FillPattern::kSolid)
                         == static_cast<int>(gui::Painter::Brush::kSolid)
                  && static_cast<int>(FillPattern::kDiagonal)
                         == static_cast<int>(gui::Painter::Brush::kDiagonal)
                  && static_cast<int>(FillPattern::kCross)
                         == static_cast<int>(gui::Painter::Brush::kCross)
                  && static_cast<int>(FillPattern::kDots)
                         == static_cast<int>(gui::Painter::Brush::kDots),
              "web::FillPattern must mirror gui::Painter::Brush values");

// How the web layer tree groups a tech layer, mirroring the Qt GUI
// (displayControls.cpp).  Single source of truth shared by getLayers() (which
// includes everything except kSkip) and buildLayerHierarchy() (which routes
// each group to its own tree folder).
enum class LayerGroup
{
  kSkip,     // not shown in the web layer tree
  kRouting,  // routing/cut → main "Layers" group (backside split out there)
  kImplant,  // IMPLANT → "Implant" category
  kOther,    // MASTERSLICE/OVERLAP/other → "Other" category
};

LayerGroup classifyLayer(odb::dbTechLayer* layer)
{
  if (layer == nullptr) {
    return LayerGroup::kSkip;
  }
  if (layer->getRoutingLevel() > 0
      || layer->getType() == odb::dbTechLayerType::CUT) {
    return LayerGroup::kRouting;
  }
  switch (layer->getType()) {
    case odb::dbTechLayerType::IMPLANT:
      return LayerGroup::kImplant;
    case odb::dbTechLayerType::MASTERSLICE:
    case odb::dbTechLayerType::OVERLAP:
    case odb::dbTechLayerType::NONE:
      return LayerGroup::kOther;
    default:
      return LayerGroup::kSkip;
  }
}

// Whether `pattern` paints the pixel at absolute coordinates (ax, ay).
// Coordinates are absolute (tile-local + tile origin) so adjacent tiles share
// one continuous lattice and the hatch never seams.  Same modulo-lattice trick
// the placement-blockage hash uses; periods chosen so layer fills read as a
// texture rather than solid.
// Non-negative remainder of v modulo a positive period.  One modulo plus a
// conditional (the remainder is only ever negative for negative v).
int wrapMod(const int v, const int period)
{
  const int r = v % period;
  return r < 0 ? r + period : r;
}

bool patternCovers(const FillPattern pattern, const int ax, const int ay)
{
  constexpr int kHatchPeriod = 8;  // pixels between hatch lines
  constexpr int kHatchWidth = 2;   // line thickness in pixels
  constexpr int kDotPeriod = 6;    // pixels between dots
  constexpr int kDotSize = 2;      // dot footprint in pixels
  switch (pattern) {
    case FillPattern::kNone:
      return false;
    case FillPattern::kSolid:
      return true;
    case FillPattern::kDiagonal:
      return wrapMod(ax + ay, kHatchPeriod) < kHatchWidth;
    case FillPattern::kCross:
      return wrapMod(ax, kHatchPeriod) < kHatchWidth
             || wrapMod(ay, kHatchPeriod) < kHatchWidth;
    case FillPattern::kDots:
      return wrapMod(ax, kDotPeriod) < kDotSize
             && wrapMod(ay, kDotPeriod) < kDotSize;
  }
  return true;
}

}  // namespace

void TileGenerator::fillPolygon(std::vector<unsigned char>& image,
                                const odb::Polygon& poly,
                                const TileFrame& frame,
                                const Color& color,
                                const bool blend,
                                const FillPattern pattern,
                                int dim) const
{
  // kNone paints nothing: skip all scanline setup and the pixel loop.
  if (pattern == FillPattern::kNone) {
    return;
  }
  // Tile origin in absolute pixel space, anchoring non-solid patterns.
  const int ox = patternAnchor(frame.origin_x, frame.scale);
  const int oy = patternAnchor(frame.origin_y, frame.scale);
  const auto& points = poly.getPoints();
  const int n = static_cast<int>(points.size());
  if (n < 3) {
    return;
  }
  if (dim < 0) {
    dim = bufferDim(image);
  }

  // Convert polygon points to pixel coordinates (floating point for precision).
  std::vector<double> px(n), py(n);
  for (int i = 0; i < n; ++i) {
    px[i] = frame.pxX(points[i].x());
    py[i] = frame.pxY(points[i].y());
  }

  // Compute pixel bounding box, clamped to tile.
  const double min_py = std::ranges::min(py);
  const double max_py = std::ranges::max(py);
  const int iy_min = std::max(0, static_cast<int>(min_py));
  const int iy_max = std::min(dim, static_cast<int>(std::ceil(max_py)));

  // Scanline fill: for each row, find edge intersections and fill between
  // pairs.
  std::vector<double> x_intercepts;
  for (int iy = iy_min; iy < iy_max; ++iy) {
    const double scanline = iy + 0.5;  // test at pixel center
    x_intercepts.clear();

    for (int i = 0, j = n - 1; i < n; j = i++) {
      // Skip degenerate (horizontal) edges and only process edges that
      // straddle the scanline.
      if ((py[i] <= scanline) == (py[j] <= scanline)) {
        continue;
      }
      const double x
          = px[i] + (scanline - py[i]) / (py[j] - py[i]) * (px[j] - px[i]);
      x_intercepts.push_back(x);
    }

    std::ranges::sort(x_intercepts);

    for (size_t k = 0; k + 1 < x_intercepts.size(); k += 2) {
      const int ix_min = std::max(0, static_cast<int>(x_intercepts[k]));
      const int ix_max
          = std::min(dim, static_cast<int>(std::ceil(x_intercepts[k + 1])));
      const int draw_y = dim - 1 - iy;
      for (int ix = ix_min; ix < ix_max; ++ix) {
        if (pattern != FillPattern::kSolid
            && !patternCovers(pattern, ix + ox, iy + oy)) {
          continue;
        }
        if (blend) {
          blendPixel(image, ix, draw_y, color, dim);
        } else {
          setPixel(image, ix, draw_y, color, dim);
        }
      }
    }
  }
}

odb::Rect TileGenerator::getBounds() const
{
  // Union of every reachable chiplet's bbox in world coordinates.
  // Returned bbox drives both zoom-to-fit framing and `scale` in
  // renderTileBuffer.  For single-chip designs we stick to the
  // dbBlock BBox so existing per-instance pixel tests stay valid; for
  // multi-die designs we merge each chiplet's die area as well, since
  // (a) the chiplet outline overlay needs to land inside the viewport
  // and (b) chiplets translated apart have no single "BBox" without
  // the die-area inclusion.  Two bounds-semantics for two render
  // modes is deliberate — see MultiDieBoundsIncludeChipletDieAreas.
  odb::dbChip* root = getChip();
  if (!root) {
    return {};
  }
  odb::Rect bounds;
  bounds.mergeInit();
  bool any = false;
  const std::vector<ChipletNode>& nodes = chiplets();
  // In single-chip designs the previous behavior used only the top
  // block's bbox; expanding to the full die-area changes `scale` for
  // every tile and breaks tests that depend on the marker/label
  // pixel-size threshold.  Only multi-die designs (more than one
  // chiplet) benefit from the die-area expansion needed by the
  // chiplet outline overlay.
  const bool is_multi_die = nodes.size() > 1;
  for (const ChipletNode& node : nodes) {
    if (!node.block) {
      continue;
    }
    odb::Rect b = node.block->getBBox()->getBox();
    node.world_xfm.apply(b);
    bounds.merge(b);
    if (is_multi_die) {
      const odb::Rect die = node.block->getDieArea();
      if (die.area() > 0) {
        odb::Rect d = die;
        node.world_xfm.apply(d);
        bounds.merge(d);
      }
    }
    any = true;
  }
  if (!any) {
    return {};
  }
  if (pin_label_margin_dbu_ > 0) {
    bounds.set_xlo(bounds.xMin() - pin_label_margin_dbu_);
    bounds.set_ylo(bounds.yMin() - pin_label_margin_dbu_);
    bounds.set_xhi(bounds.xMax() + pin_label_margin_dbu_);
    bounds.set_yhi(bounds.yMax() + pin_label_margin_dbu_);
  }
  return bounds;
}

int TileGenerator::getPinMaxSize() const
{
  odb::dbBlock* block = getBlock();
  if (!block) {
    return 0;
  }
  const odb::Rect die = block->getDieArea();
  const int die_max_dim = std::max(die.dx(), die.dy());
  return std::max(static_cast<int>(kPinMarkerSizeRatio * die_max_dim),
                  kMinPinMarkerSize);
}

std::vector<std::string> TileGenerator::getLayers() const
{
  // Collect the union of routing/cut layers from every tech reachable
  // through dbChipInst.  This makes layers exclusive to a chiplet's
  // tech show up in the web display-controls panel.  Names are
  // deduplicated; insertion order follows the layer enumeration order
  // of the first tech that contributes the layer.  Caveat: two techs
  // with the same layer name (e.g. "M1" in 40nm and 5nm) collapse to
  // one entry — the rendered color comes from each chiplet's own tech
  // via getLayerColorMap(), but the user only gets one visibility
  // toggle for the merged name.
  std::vector<std::string> layers;
  std::set<std::string> seen;
  odb::PtrSet<odb::dbTech> visited_techs;

  auto collectFromTech = [&](odb::dbTech* tech) {
    if (!tech || !visited_techs.insert(tech).second) {
      return;
    }
    for (odb::dbTechLayer* layer : tech->getLayers()) {
      // Include every layer the web tree shows (classifyLayer != kSkip) so the
      // flat list, layer_colors and the saveReport prerender cover the
      // Implant/Other categories too.
      if (classifyLayer(layer) == LayerGroup::kSkip) {
        continue;
      }
      const std::string name = layer->getName();
      if (seen.insert(name).second) {
        layers.push_back(name);
      }
    }
  };

  // Top-tech first so single-chip designs preserve the previous order.
  // In multi-tech (3DBlox) designs getTech() returns nullptr; the chiplets()
  // loop below contributes every layer through its own dbTech.
  collectFromTech(getTech());
  for (const ChipletNode& node : chiplets()) {
    if (node.chip) {
      collectFromTech(node.chip->getTech());
    }
    if (node.block) {
      for (odb::dbBlock* child : node.block->getChildren()) {
        collectFromTech(child->getTech());
      }
    }
  }
  return layers;
}

// Build per-layer colors that match gui::DisplayControls::techInit.  The two
// must stay in sync so the GUI and web frontend show the same colors for the
// same design.  Walks every dbTechLayer in tech order (not just routing/cut)
// because the random fallback shares one PRNG and the iteration order is what
// determines which layer gets which random color.
static odb::PtrMap<odb::dbTechLayer, Color> buildLayerColorMap(
    odb::dbTech* tech)
{
  odb::PtrMap<odb::dbTechLayer, Color> colors;
  if (!tech) {
    return colors;
  }

  // From http://vrl.cs.brown.edu/color seeded with #00F, #F00, #0D0
  static constexpr std::array<Color, 14> kMetalColors = {{
      // NOLINTBEGIN(modernize-use-designated-initializers)
      {0, 0, 254, 180},
      {254, 0, 0, 180},
      {9, 221, 0, 180},
      {190, 244, 81, 180},
      {222, 33, 96, 180},  // Metal 5
      {32, 216, 253, 180},
      {253, 108, 160, 180},
      {117, 63, 194, 180},
      {128, 155, 49, 180},
      {234, 63, 252, 180},  // Metal 10
      {9, 96, 19, 180},
      {214, 120, 239, 180},
      {192, 222, 164, 180},
      {110, 68, 107, 180},  // Metal 14
                            // NOLINTEND(modernize-use-designated-initializers)
  }};
  static constexpr std::array<Color, 14> kCutColors = {{
      // NOLINTBEGIN(modernize-use-designated-initializers)
      {126, 126, 255, 180},
      {255, 126, 126, 180},
      {4, 110, 0, 180},
      {95, 122, 40, 180},
      {111, 17, 48, 180},  // Cut 5
      {16, 108, 126, 180},
      {126, 54, 80, 180},
      {58, 32, 97, 180},
      {225, 255, 136, 180},
      {117, 32, 126, 180},  // Cut 10
      {18, 192, 38, 180},
      {107, 60, 119, 180},
      {96, 111, 82, 180},
      {220, 136, 214, 180},  // Cut 14
                             // NOLINTEND(modernize-use-designated-initializers)
  }};

  std::mt19937 rng(1);
  auto random_color = [&rng]() {
    const int blue = 50 + rng() % 200;
    const int green = 50 + rng() % 200;
    const int red = 50 + rng() % 200;
    return Color{.r = static_cast<unsigned char>(red),
                 .g = static_cast<unsigned char>(green),
                 .b = static_cast<unsigned char>(blue),
                 .a = 180};
  };

  size_t metal = 0;
  size_t via = 0;
  for (odb::dbTechLayer* layer : tech->getLayers()) {
    if (layer == nullptr) {
      continue;
    }
    Color c;
    if (layer->isBackside()) {
      c = random_color();
    } else {
      const odb::dbTechLayerType type = layer->getType();
      if (type == odb::dbTechLayerType::ROUTING) {
        c = (metal < kMetalColors.size()) ? kMetalColors[metal++]
                                          : random_color();
      } else if (type == odb::dbTechLayerType::CUT) {
        // GUI: a CUT layer that appears before any ROUTING layer gets a random
        // color so cuts don't claim the metal palette slots.
        c = (via < kCutColors.size() && metal != 0) ? kCutColors[via++]
                                                    : random_color();
      } else {
        c = random_color();
      }
    }
    colors[layer] = c;
  }
  return colors;
}

void TileGenerator::clearOverlayCaches() const
{
  {
    std::lock_guard lock(heatmap_mutex_);
    heatmaps_.clear();
  }
  std::lock_guard lock(overlay_cache_mutex_);
  dropOverlayCaches();
}

// Caller holds overlay_cache_mutex_.  All three caches are derived from the
// same design state and are invalidated as a unit, so one recorded revision
// covers them.
void TileGenerator::dropOverlayCaches() const
{
  bpin_ap_cache_.clear();
  gcell_x_cache_.clear();
  gcell_y_cache_.clear();
}

// Caller holds overlay_cache_mutex_.  `rev` must have been read BEFORE the lock
// was taken, so an edit landing while a cache is being built leaves the
// recorded revision behind the live one and the next call rebuilds — the same
// ordering geomCache() relies on.
void TileGenerator::dropOverlayCachesIfStale(const uint64_t rev) const
{
  if (overlay_cache_revision_ != rev) {
    dropOverlayCaches();
    overlay_cache_revision_ = rev;
  }
}

TileGenerator::BpinApList TileGenerator::bpinAccessPoints(
    odb::dbBlock* block) const
{
  const uint64_t rev = search_->revision();
  std::lock_guard lock(overlay_cache_mutex_);
  dropOverlayCachesIfStale(rev);
  auto [it, inserted] = bpin_ap_cache_.try_emplace(block);
  if (inserted) {
    auto aps = std::make_shared<std::vector<BpinAp>>();
    for (odb::dbBTerm* term : block->getBTerms()) {
      for (odb::dbBPin* pin : term->getBPins()) {
        for (odb::dbAccessPoint* ap : pin->getAccessPoints()) {
          if (ap) {
            aps->push_back({ap->getPoint(),
                            ap->getLayer(),
                            term->getNet(),
                            ap->hasAccess()});
          }
        }
      }
    }
    it->second = std::move(aps);
  }
  return it->second;
}

// Shared body of gcellGridX/Y.  `fill` copies one axis out of the block's grid;
// a pointer-to-member would be ambiguous, dbGCellGrid::getGridX/Y each having a
// second, reference-returning overload.
TileGenerator::GridList TileGenerator::gcellGrid(
    odb::PtrMap<odb::dbBlock, GridList>& cache,
    odb::dbBlock* block,
    const std::function<void(odb::dbGCellGrid*, std::vector<int>&)>& fill) const
{
  const uint64_t rev = search_->revision();
  std::lock_guard lock(overlay_cache_mutex_);
  dropOverlayCachesIfStale(rev);
  auto [it, inserted] = cache.try_emplace(block);
  if (inserted) {
    auto grid_lines = std::make_shared<std::vector<int>>();
    if (odb::dbGCellGrid* grid = block->getGCellGrid()) {
      fill(grid, *grid_lines);  // sorted ascending by odb
    }
    it->second = std::move(grid_lines);
  }
  return it->second;
}

TileGenerator::GridList TileGenerator::gcellGridX(odb::dbBlock* block) const
{
  return gcellGrid(gcell_x_cache_, block, [](auto* grid, auto& out) {
    grid->getGridX(out);
  });
}

TileGenerator::GridList TileGenerator::gcellGridY(odb::dbBlock* block) const
{
  return gcellGrid(gcell_y_cache_, block, [](auto* grid, auto& out) {
    grid->getGridY(out);
  });
}

const odb::PtrMap<odb::dbTechLayer, Color>& TileGenerator::getLayerColorMap(
    odb::dbTech* req_tech) const
{
  std::lock_guard lock(layer_colors_mutex_);
  // In multi-tech databases db_->getTech() throws ODB-0432, so resolve the
  // single tech via getTechs() instead.  When no caller-supplied tech and
  // more than one is loaded, return an empty map: per-tech color lookups go
  // through the explicit-tech overload (see buildLayerHierarchy).
  odb::dbTech* tech = req_tech;
  if (!tech) {
    auto techs = db_->getTechs();
    if (techs.size() == 1) {
      tech = *techs.begin();
    }
  }
  auto [it, inserted] = layer_colors_by_tech_.try_emplace(tech);
  if (inserted) {
    it->second = buildLayerColorMap(tech);
  }
  return it->second;
}

std::shared_ptr<const TileGenerator::GeomCache> TileGenerator::buildGeomCache()
    const
{
  auto cache = std::make_shared<GeomCache>();

  // Master OBS and pin geometry, bucketed by the layer each box/polygon sits
  // on.  Collects exactly the sources the per-instance render pass draws, so a
  // (layer, master) pair absent from the map provably has nothing to draw.
  for (odb::dbLib* lib : db_->getLibs()) {
    for (odb::dbMaster* master : lib->getMasters()) {
      for (odb::dbPolygon* poly_obs : master->getPolygonObstructions()) {
        if (odb::dbTechLayer* lyr = poly_obs->getTechLayer()) {
          cache->master_geom[lyr][master].obs_polys.push_back(
              poly_obs->getPolygon());
        }
      }
      for (odb::dbBox* obs : master->getObstructions(false)) {
        if (odb::dbTechLayer* lyr = obs->getTechLayer()) {
          cache->master_geom[lyr][master].obs_boxes.push_back(obs->getBox());
        }
      }
      for (odb::dbMTerm* mterm : master->getMTerms()) {
        for (odb::dbMPin* mpin : mterm->getMPins()) {
          for (odb::dbPolygon* poly_geom : mpin->getPolygonGeometry()) {
            if (odb::dbTechLayer* lyr = poly_geom->getTechLayer()) {
              cache->master_geom[lyr][master].pin_polys.push_back(
                  poly_geom->getPolygon());
            }
          }
          for (odb::dbBox* geom : mpin->getGeometry(false)) {
            odb::dbTechLayer* lyr = geom->getTechLayer();
            if (!lyr) {
              continue;
            }
            auto& groups = cache->master_geom[lyr][master].pin_boxes;
            // MTerms are visited in master order and a pin's boxes
            // consecutively, so extending the trailing group when the MTerm
            // repeats is enough to keep each pin's boxes together and in order.
            if (groups.empty() || groups.back().first != mterm) {
              groups.emplace_back(mterm, std::vector<odb::Rect>{});
            }
            groups.back().second.push_back(geom->getBox());
          }
        }
      }
    }
  }

  // Via masters.  dbSBox reports its master as either a dbTechVia (tech-wide)
  // or a dbVia (per block), so both are collected into one map keyed by the
  // common dbObject base.
  auto add_via_boxes = [&cache](odb::dbObject* via_master,
                                const odb::dbSet<odb::dbBox>& boxes) {
    for (odb::dbBox* box : boxes) {
      if (odb::dbTechLayer* lyr = box->getTechLayer()) {
        cache->via_boxes[lyr][via_master].push_back(box->getBox());
      }
    }
  };
  for (odb::dbTech* tech : db_->getTechs()) {
    for (odb::dbTechVia* via : tech->getVias()) {
      add_via_boxes(via, via->getBoxes());
    }
  }
  // One node per dbChipInst, so every instance of a shared master chip reports
  // the same block.  Visit each block once: appending a via's boxes again per
  // instance would leave duplicates for the render pass to redraw, once per
  // instance of the chiplet.
  std::unordered_set<odb::dbBlock*> seen_blocks;
  for (const ChipletNode& node : chiplets()) {
    if (!node.block || !seen_blocks.insert(node.block).second) {
      continue;
    }
    for (odb::dbVia* via : node.block->getVias()) {
      add_via_boxes(via, via->getBoxes());
    }
  }

  return cache;
}

std::shared_ptr<const TileGenerator::GeomCache> TileGenerator::geomCache() const
{
  // Read both keys before building so an edit landing mid-build leaves the
  // recorded values behind the live ones, and the next call rebuilds.
  const uint64_t rev = search_->revision();
  // Creating a dbChipInst fires no block callback, so it cannot move
  // Search::revision() — but it does make an already-populated block's vias
  // newly reachable, which is exactly what via_boxes is keyed off.  Every other
  // source of new geometry reaches revision() transitively: a dbVia or dbMaster
  // only becomes drawable once an sbox or instance references it, and those do
  // notify.  chipletsGeneration() closes that one gap.
  const uint64_t chiplet_generation = chipletsGeneration();
  std::lock_guard lock(geom_cache_mutex_);
  if (!geom_cache_ || geom_cache_revision_ != rev
      || geom_cache_chiplet_generation_ != chiplet_generation) {
    geom_cache_ = buildGeomCache();
    geom_cache_revision_ = rev;
    geom_cache_chiplet_generation_ = chiplet_generation;
  }
  return geom_cache_;
}

std::vector<std::string> TileGenerator::getSites() const
{
  std::set<std::string> seen;
  std::vector<std::string> sites;
  odb::dbBlock* block = getBlock();
  if (!block) {
    return sites;
  }
  for (odb::dbRow* row : block->getRows()) {
    odb::dbSite* site = row->getSite();
    if (site && seen.insert(site->getName()).second) {
      sites.push_back(site->getName());
    }
  }
  return sites;
}

TileGenerator::SnapResult TileGenerator::snapAt(
    const int dbu_x,
    const int dbu_y,
    const int search_radius,
    const int point_snap_threshold,
    const bool horizontal,
    const bool vertical,
    const TileVisibility& vis,
    const std::set<std::string>& visible_layers) const
{
  odb::dbBlock* block = getBlock();
  if (!block) {
    return {};
  }
  auto sr = search_->searchNearestEdge(block,
                                       odb::Point(dbu_x, dbu_y),
                                       search_radius,
                                       point_snap_threshold,
                                       horizontal,
                                       vertical,
                                       vis,
                                       visible_layers);
  SnapResult result;
  result.edge = sr.edge;
  result.distance = sr.distance;
  result.found = sr.found;
  return result;
}

std::vector<SelectionResult> TileGenerator::selectAt(
    const int dbu_x,
    const int dbu_y,
    const int zoom,
    const TileVisibility& vis,
    const std::set<std::string>& visible_layers)
{
  std::vector<SelectionResult> results;
  odb::dbChip* root = getChip();
  if (!root) {
    return results;
  }
  // Compute a search margin of 2 pixels at the current zoom level.
  // This accounts for coordinate conversion rounding between the client's
  // Leaflet CRS.Simple coordinates and the server's DBU space.
  const int num_tiles = 1 << std::max(0, zoom);
  const int margin
      = std::max(1, getBounds().maxDXDY() / (kTileSizeInPixel * num_tiles) * 2);
  const double dbu_per_micron = db_->getDbuPerMicron();
  debugPrint(logger_,
             utl::WEB,
             "select",
             1,
             "selectAt um=({},{}) zoom={} margin_um={}",
             dbuToMicronString(dbu_x, dbu_per_micron),
             dbuToMicronString(dbu_y, dbu_per_micron),
             zoom,
             dbuToMicronString(margin, dbu_per_micron));

  odb::PtrSet<odb::dbNet> seen_nets;

  // Iterate every chiplet so clicks inside a translated/rotated
  // dbChipInst land on the right object.  We map the world click into
  // the chiplet's local frame using the inverse of world_xfm so that
  // R90/R180/R270/mirrored chiplets work the same as R0 ones.
  for (const ChipletNode& node : chiplets()) {
    if (!vis.isChipletVisible(node.path)) {
      continue;
    }
    odb::dbBlock* block = node.block;
    if (!block || !node.chip) {
      continue;
    }
    odb::dbTech* tech = node.chip->getTech();
    if (!tech) {
      continue;
    }
    odb::dbTransform inv_xfm = node.world_xfm;
    inv_xfm.invert();
    odb::Point click_pt(dbu_x, dbu_y);
    inv_xfm.apply(click_pt);
    const int local_x = click_pt.x();
    const int local_y = click_pt.y();
    const int x_lo = local_x - margin;
    const int y_lo = local_y - margin;
    const int x_hi = local_x + margin;
    const int y_hi = local_y + margin;

    // Map a local-frame rect back to world coordinates for the result
    // bbox.  dbTransform::apply(Rect&) returns the axis-aligned
    // bounding box of the rotated rect, which is what the frontend
    // wants for zoom-to-bbox.
    auto toWorld = [&](odb::Rect r) {
      node.world_xfm.apply(r);
      return r;
    };

    // Search instances in this chiplet's block — require both visible
    // and selectable.
    for (odb::dbInst* inst :
         search_->searchInsts(block, x_lo, y_lo, x_hi, y_hi)) {
      const odb::Rect bbox = inst->getBBox()->getBox();
      if (bbox.intersects(click_pt) && vis.isInstVisible(inst, sta_)
          && vis.isInstSelectable(inst, sta_)) {
        std::string label = inst->getName();
        if (!node.path.empty() && node.inst != nullptr) {
          std::string prefixed = node.path;
          prefixed += '/';
          prefixed += label;
          label = std::move(prefixed);
        }
        results.push_back(
            {inst, label, "Inst", toWorld(bbox), node.world_xfm, true});
      }
    }

    // Search nets via routing shapes on each layer.
    for (odb::dbTechLayer* layer : tech->getLayers()) {
      if (layer->getRoutingLevel() <= 0
          && layer->getType() != odb::dbTechLayerType::CUT) {
        continue;
      }
      if (!visible_layers.empty()
          && !visible_layers.contains(layer->getName())) {
        continue;
      }
      if (!vis.isLayerSelectable(layer->getName())) {
        continue;
      }

      // Regular routing shapes (wires, vias) and BTerm shapes.
      // Picks require both visible and selectable: routing segments/vias
      // have no dedicated selectable flag (Qt parity — clicks resolve
      // through the net), so the net's own selectability gates the
      // result.  BTerm picks gate on the dedicated `pins_selectable`
      // flag.
      if (vis.routing || vis.pins) {
        for (const auto& shape :
             search_->searchBoxShapes(block, layer, x_lo, y_lo, x_hi, y_hi)) {
          const auto type = std::get<1>(shape);
          if (type == Search::kBterm && (!vis.pins || !vis.pins_selectable)) {
            continue;
          }
          if (type == Search::kWire && !(vis.routing && vis.routing_segments)) {
            continue;
          }
          if (type == Search::kVia && !(vis.routing && vis.routing_vias)) {
            continue;
          }
          odb::dbNet* net = std::get<2>(shape);
          if (seen_nets.contains(net)) {
            continue;
          }
          const odb::Rect& box = std::get<0>(shape);
          if (box.intersects(click_pt) && vis.isNetVisible(net)
              && vis.isNetSelectable(net)) {
            seen_nets.insert(net);
            results.push_back({net,
                               net->getName(),
                               "Net",
                               toWorld(net->getTermBBox()),
                               node.world_xfm,
                               false});
          }
        }
      }

      // Special net vias
      if (vis.special_nets && vis.srouting_vias) {
        for (const auto& shape : search_->searchSNetViaShapes(
                 block, layer, x_lo, y_lo, x_hi, y_hi)) {
          odb::dbNet* net = std::get<1>(shape);
          if (seen_nets.contains(net)) {
            continue;
          }
          const odb::Rect box = std::get<0>(shape)->getBox();
          if (box.intersects(click_pt) && vis.isNetVisible(net)
              && vis.isNetSelectable(net)) {
            seen_nets.insert(net);
            results.push_back({net,
                               net->getName(),
                               "Net",
                               toWorld(net->getTermBBox()),
                               node.world_xfm,
                               false});
          }
        }
      }

      // Special net shapes (segments/straps)
      if (vis.special_nets && vis.srouting_segments) {
        for (const auto& shape :
             search_->searchSNetShapes(block, layer, x_lo, y_lo, x_hi, y_hi)) {
          odb::dbNet* net = std::get<2>(shape);
          if (seen_nets.contains(net)) {
            continue;
          }
          const odb::Rect box = std::get<0>(shape)->getBox();
          if (box.intersects(click_pt) && vis.isNetVisible(net)
              && vis.isNetSelectable(net)) {
            seen_nets.insert(net);
            results.push_back({net,
                               net->getName(),
                               "Net",
                               toWorld(net->getTermBBox()),
                               node.world_xfm,
                               false});
          }
        }
      }
    }
  }

  // Sort instances by area descending so larger instances (macros) come
  // first; nets keep their per-chiplet insertion order behind insts.
  std::ranges::stable_sort(results, [](const auto& a, const auto& b) {
    if (a.is_inst != b.is_inst) {
      return a.is_inst;
    }
    return a.bbox.area() > b.bbox.area();
  });

  debugPrint(
      logger_,
      utl::WEB,
      "select",
      1,
      "  selected={} (insts={}, nets={})",
      results.size(),
      std::ranges::count_if(results, [](const auto& r) { return r.is_inst; }),
      seen_nets.size());
  return results;
}

odb::dbBlock* TileGenerator::getBlock() const
{
  odb::dbChip* chip = db_->getChip();
  return chip ? chip->getBlock() : nullptr;
}

namespace {

// Recursive helper for collectChiplets.  Records the current chip,
// then recurses into each dbChipInst with the accumulated world
// transform.  See dbInst::getHierTransform() for the canonical
// concat order: child_local.concat(parent_world) yields the
// local-to-root transform.
void collectChipletsRec(odb::dbChip* chip,
                        odb::dbChipInst* inst,
                        const odb::dbTransform& parent_world_xfm,
                        const std::string& parent_path,
                        const int depth,
                        const int parent_global_z,
                        std::vector<ChipletNode>& out)
{
  if (!chip) {
    return;
  }
  ChipletNode node;
  node.chip = chip;
  node.block = chip->getBlock();
  node.inst = inst;
  node.depth = depth;
  node.parent_path = parent_path;
  if (inst != nullptr) {
    odb::dbTransform local = inst->getTransform();
    local.concat(parent_world_xfm);
    node.world_xfm = local;
    node.name = inst->getName();
    node.path = parent_path + "." + node.name;
    node.global_z = parent_global_z + inst->getLoc().z();
  } else {
    node.world_xfm = parent_world_xfm;
    if (node.block) {
      node.name = node.block->getName();
    } else {
      node.name = "top";
    }
    node.path = node.name;
    node.global_z = parent_global_z;
  }
  out.push_back(node);

  for (odb::dbChipInst* child : chip->getChipInsts()) {
    collectChipletsRec(child->getMasterChip(),
                       child,
                       node.world_xfm,
                       node.path,
                       depth + 1,
                       node.global_z,
                       out);
  }
}

}  // namespace

std::vector<ChipletNode> collectChiplets(odb::dbChip* root)
{
  std::vector<ChipletNode> out;
  if (!root) {
    return out;
  }
  collectChipletsRec(
      root, nullptr, odb::dbTransform{}, std::string{}, 0, 0, out);

  std::ranges::stable_sort(out, [](const ChipletNode& a, const ChipletNode& b) {
    if (a.global_z != b.global_z) {
      return a.global_z < b.global_z;
    }
    return a.depth < b.depth;
  });

  return out;
}

odb::dbChip* TileGenerator::getChip() const
{
  return db_->getChip();
}

odb::dbTech* TileGenerator::getTech() const
{
  // db_->getTech() throws ODB-0432 when more than one tech is loaded
  // (multi-chiplet 3DBlox designs).  The web GUI emits per-tech data via
  // layer_hierarchy, so callers that still want "the" tech only get one
  // when the database actually has a single tech; otherwise nullptr.
  auto techs = db_->getTechs();
  if (techs.size() == 1) {
    return *techs.begin();
  }
  return nullptr;
}

std::vector<unsigned char> TileGenerator::generateTile(
    const std::string& layer,
    const int z,
    const int x,
    int y,
    const TileVisibility& vis,
    const std::vector<odb::Rect>& highlight_rects,
    const std::vector<odb::Polygon>& highlight_polys,
    const std::vector<ColoredRect>& colored_rects,
    const std::vector<FlightLine>& flight_lines,
    const std::map<uint32_t, Color>* module_colors,
    const std::set<uint32_t>* focus_net_ids,
    const std::set<uint32_t>* route_guide_net_ids,
    const double dpr,
    const int tile_px) const
{
  auto image_buffer = renderTileBuffer(layer,
                                       z,
                                       x,
                                       y,
                                       vis,
                                       highlight_rects,
                                       highlight_polys,
                                       colored_rects,
                                       flight_lines,
                                       module_colors,
                                       focus_net_ids,
                                       route_guide_net_ids,
                                       dpr,
                                       tile_px);

  // The buffer is square at the output resolution; recover its side rather
  // than assuming the requested one, which is 0 when the client did not name
  // it.
  const int rendered_px = bufferDim(image_buffer);
  std::vector<unsigned char> png_data;
  const unsigned error
      = lodepng::encode(png_data, image_buffer, rendered_px, rendered_px);
  if (error) {
    logger_->report("PNG encoder error: {}", lodepng_error_text(error));
  }

  if (logger_->debugCheck(utl::WEB, "tile_generator", 1)) {
    const std::string filename = "/tmp/tile_" + layer + "_" + std::to_string(z)
                                 + "_" + std::to_string(x) + "_"
                                 + std::to_string(y) + ".png";
    lodepng::save_file(png_data, filename);
  }

  return png_data;
}

std::vector<unsigned char> TileGenerator::generateOverlayTile(
    const int z,
    const int x,
    int y,
    const std::vector<odb::Rect>& highlight_rects,
    const std::vector<odb::Polygon>& highlight_polys,
    const std::vector<ColoredRect>& colored_rects,
    const std::vector<FlightLine>& flight_lines,
    const std::set<uint32_t>* route_guide_net_ids,
    const bool has_visible_layers,
    const std::set<std::string>& visible_layers,
    const double dpr,
    const int requested_tile_px,
    const std::vector<ColoredPolygon>& colored_polys,
    const std::vector<TextLabel>& labels) const
{
  // Same contract as renderTileBuffer: the client states the device-pixel
  // square it will display this tile in, because an overlay drawn at a
  // different size than the layer tiles beneath it is both blurry and
  // misregistered against them.  0 falls back to the historical 256*dpr.
  const double effective_dpr = dpr > 0.0 ? dpr : 1.0;
  const int dim
      = requested_tile_px > 0
            ? requested_tile_px
            : static_cast<int>(std::lround(kTileSizeInPixel * effective_dpr));
  std::vector<unsigned char> image(static_cast<size_t>(dim) * dim * 4,
                                   0);  // fully transparent

  if (!getChip()) {
    // No design — return blank transparent PNG.
    std::vector<unsigned char> png;
    lodepng::encode(png, image, dim, dim);
    return png;
  }

  // Short-circuit: if there's nothing to draw, return a blank tile.
  if (highlight_rects.empty() && highlight_polys.empty()
      && colored_rects.empty() && colored_polys.empty() && flight_lines.empty()
      && labels.empty()
      && (!route_guide_net_ids || route_guide_net_ids->empty())) {
    std::vector<unsigned char> png;
    lodepng::encode(png, image, dim, dim);
    return png;
  }

  // Compute tile bounding box in DBU (same math as renderTileBuffer).
  const double num_tiles_at_zoom = pow(2, z);
  y = num_tiles_at_zoom - 1 - y;  // flip Y
  const odb::Rect full_bounds = getBounds();
  if (full_bounds.maxDXDY() <= 0) {
    std::vector<unsigned char> png;
    lodepng::encode(png, image, dim, dim);
    return png;
  }
  // Same exact grid as renderTileBuffer: the overlay is drawn on top of those
  // tiles, so a highlight has to land on the shape it highlights.
  const double tile_dbu_size = full_bounds.maxDXDY() / num_tiles_at_zoom;
  const TileFrame frame = tileFrame(
      full_bounds, x, y, tile_dbu_size, dim / tile_dbu_size, effective_dpr);

  // Draw highlight shapes onto transparent buffer.
  if (!highlight_rects.empty() || !highlight_polys.empty()) {
    drawHighlight(image, highlight_rects, highlight_polys, frame);
  }
  if (!colored_rects.empty()) {
    // Pass empty layer name so all colored rects are drawn regardless of
    // their per-layer tag (the overlay sits above all base layers).
    drawColoredHighlight(image, colored_rects, "", frame);
  }
  if (!colored_polys.empty()) {
    drawColoredPolygons(image, colored_polys, frame);
  }
  if (!flight_lines.empty()) {
    drawFlightLines(image, flight_lines, frame);
  }
  if (!labels.empty()) {
    drawTextLabels(image, labels, frame);
  }
  if (route_guide_net_ids && !route_guide_net_ids->empty()) {
    // Draw route guides only for visible tech layers.
    for (odb::dbTech* tech : db_->getTechs()) {
      const auto& colors = getLayerColorMap(tech);
      for (odb::dbTechLayer* layer : tech->getLayers()) {
        // Skip layers the user has hidden in Display Controls.
        // has_visible_layers=true with an empty set means all hidden.
        if (has_visible_layers && !visible_layers.contains(layer->getName())) {
          continue;
        }
        const auto it = colors.find(layer);
        if (it == colors.end()) {
          continue;
        }
        drawRouteGuides(
            image, *route_guide_net_ids, layer->getName(), it->second, frame);
      }
    }
  }

  std::vector<unsigned char> png;
  lodepng::encode(png, image, dim, dim);
  return png;
}

// Forward declaration; defined below near compositePixel.  Separable Lanczos-2
// decimation of a straight-alpha RGBA buffer (anti-moiré band-limit).
static std::vector<unsigned char> lanczos2Downsample(
    const std::vector<unsigned char>& src,
    int src_dim,
    int dst_dim);

namespace {

// This master's shapes on the layer whose cache slice is `by_master`, or null
// when it has none there (so the caller can skip it outright).
const TileGenerator::MasterLayerGeom* findMasterGeom(
    const TileGenerator::MasterGeomByLayer* by_master,
    odb::dbMaster* master)
{
  if (!by_master) {
    return nullptr;
  }
  const auto it = by_master->find(master);
  return it == by_master->end() ? nullptr : &it->second;
}

// The via-local boxes this sbox's via master contributes to the layer whose
// cache slice is `by_master`, or null when it contributes none.  Resolving the
// master costs one dbBox flag test plus a table lookup, instead of walking and
// layer-testing every box of the via for every sbox.
const std::vector<odb::Rect>* findViaBoxes(
    const TileGenerator::ViaBoxesByMaster* by_master,
    odb::dbSBox* sbox)
{
  if (!by_master) {
    return nullptr;
  }
  odb::dbObject* via_master = nullptr;
  if (odb::dbTechVia* tech_via = sbox->getTechVia()) {
    via_master = tech_via;
  } else if (odb::dbVia* block_via = sbox->getBlockVia()) {
    via_master = block_via;
  }
  if (!via_master) {
    return nullptr;
  }
  const auto it = by_master->find(via_master);
  return it == by_master->end() ? nullptr : &it->second;
}

}  // namespace

void TileGenerator::outlineRectInTile(std::vector<unsigned char>& image,
                                      const odb::Rect& r,
                                      const Color& c,
                                      const TileFrame& frame) const
{
  const odb::Rect& dbu_tile = frame.cull;
  const int xl = r.xMin();
  const int yl = r.yMin();
  const int xh = r.xMax();
  const int yh = r.yMax();
  const int64_t pixel_xl = static_cast<int64_t>(frame.pxX(xl));
  const int64_t pixel_yl = static_cast<int64_t>(frame.pxY(yl));
  const int64_t pixel_xh = static_cast<int64_t>(std::ceil(frame.pxX(xh)));
  const int64_t pixel_yh = static_cast<int64_t>(std::ceil(frame.pxY(yh)));

  // frame.scale is in buffer pixels per DBU, so the clamps must follow the
  // buffer the caller handed us: tile_px for a plain tile,
  // tile_px*kCoverageSupersample for the supersampled render path.
  const int dim = bufferDim(image);
  const int loop_xl = std::clamp<int64_t>(pixel_xl, 0, dim);
  const int loop_yl = std::clamp<int64_t>(pixel_yl, 0, dim);
  const int loop_xh = std::clamp<int64_t>(pixel_xh, 0, dim);
  const int loop_yh = std::clamp<int64_t>(pixel_yh, 0, dim);

  const int draw_xl = std::clamp<int64_t>(pixel_xl, 0, dim - 1);
  const int draw_yl = std::clamp<int64_t>(pixel_yl, 0, dim - 1);
  const int draw_xh = std::clamp<int64_t>(pixel_xh, 0, dim - 1);
  const int draw_yh = std::clamp<int64_t>(pixel_yh, 0, dim - 1);

  // Thickness grows with the buffer so the edge survives decimation; each
  // band is drawn inward from the edge to keep the rect's extent exact.
  const int t = hairlineCss(frame);
  if (dbu_tile.xMin() <= xl && xl <= dbu_tile.xMax()) {
    for (int iy = loop_yl; iy < loop_yh; ++iy) {
      for (int k = 0; k < t; ++k) {
        setPixel(image, draw_xl + k, dim - 1 - iy, c, dim);
      }
    }
  }
  if (dbu_tile.xMin() <= xh && xh <= dbu_tile.xMax()) {
    for (int iy = loop_yl; iy < loop_yh; ++iy) {
      for (int k = 0; k < t; ++k) {
        setPixel(image, draw_xh - k, dim - 1 - iy, c, dim);
      }
    }
  }
  if (dbu_tile.yMin() <= yl && yl <= dbu_tile.yMax()) {
    for (int ix = loop_xl; ix < loop_xh; ++ix) {
      for (int k = 0; k < t; ++k) {
        setPixel(image, ix, dim - 1 - draw_yl - k, c, dim);
      }
    }
  }
  if (dbu_tile.yMin() <= yh && yh <= dbu_tile.yMax()) {
    for (int ix = loop_xl; ix < loop_xh; ++ix) {
      for (int k = 0; k < t; ++k) {
        setPixel(image, ix, dim - 1 - draw_yh + k, c, dim);
      }
    }
  }
}

// Special "_access_points" layer: dbAccessPoint markers (X).  Mirrors GUI
// RenderThread::drawAccessPoints (renderThread.cpp:1484-1550).
void TileGenerator::drawAccessPointsLayer(std::vector<unsigned char>& image,
                                          odb::dbBlock* block,
                                          const TileFrame& frame,
                                          const TileVisibility& vis) const
{
  const odb::Rect& dbu_tile = frame.cull;
  constexpr Color kApHasAccess{.r = 0, .g = 255, .b = 0, .a = 255};
  constexpr Color kApNoAccess{.r = 255, .g = 0, .b = 0, .a = 255};
  constexpr int kApSizeDbu = 100;             // match GUI shape_size
  constexpr int kApHalfDbu = kApSizeDbu / 2;  // marker reach past its centre
  const int dim = bufferDim(image);
  // LOD: skip when the 100-DBU marker would be sub-pixel (mirror GUI).  The
  // limit is in CSS pixels, so scale it by the buffer's px-per-CSS-px.
  if (kApSizeDbu * frame.scale < frame.px_per_css) {
    return;
  }
  const int half_px = static_cast<int>(kApSizeDbu * frame.scale / 2.0);
  const int stroke = hairlineCss(frame);

  // Cull against the tile grown by the marker's reach, not against the tile
  // itself: the X extends half a marker past its centre, so an access point
  // just outside this tile still paints arms into it.  Culling on the bare
  // centre made the neighbouring tile skip the marker entirely, which chopped
  // the X along every tile seam (PR #10806 review).  drawLine clips to the
  // buffer, so drawing a marker centred outside the tile is safe.
  odb::Rect ap_tile;
  dbu_tile.bloat(kApHalfDbu, ap_tile);

  // Draw one access point whose point is already in GLOBAL DBU.
  auto draw_ap = [&](const odb::Point& pt,
                     odb::dbTechLayer* ap_lyr,
                     const bool has_access) {
    if (vis.has_visible_layers && ap_lyr
        && !vis.visible_layers.contains(ap_lyr->getName())) {
      return;  // access point on a hidden layer
    }
    if (!ap_tile.intersects(pt)) {
      return;  // viewport cull, inflated by the marker reach
    }
    const int px = toPxX(pt.x(), frame);
    const int py = toPxY(pt.y(), frame, dim);
    const Color c = has_access ? kApHasAccess : kApNoAccess;
    drawLine(image,
             px - half_px,
             py - half_px,
             px + half_px,
             py + half_px,
             c,
             stroke);
    drawLine(image,
             px - half_px,
             py + half_px,
             px + half_px,
             py - half_px,
             c,
             stroke);
  };

  // (a) IO pins (BPins) — global coords, no transform.  The per-block
  // cache avoids rescanning every BTerm on every tile; holding the shared_ptr
  // keeps the list alive across a concurrent cache clear.
  const BpinApList bpin_aps = bpinAccessPoints(block);
  for (const BpinAp& ap : *bpin_aps) {
    if (ap.net && !vis.isNetVisible(ap.net)) {
      continue;  // null-safe net filter
    }
    draw_ap(ap.point, ap.layer, ap.has_access);
  }
  // (b) Instance pins (ITerms) — translate by instance origin only.
  //     Gate on inst_pins to mirror GUI areInstancePinsVisible().
  //     searchInsts gets the grown tile for the same seam reason as draw_ap.
  if (vis.inst_pins) {
    for (odb::dbInst* inst : search_->searchInsts(block,
                                                  ap_tile.xMin(),
                                                  ap_tile.yMin(),
                                                  ap_tile.xMax(),
                                                  ap_tile.yMax())) {
      int ox = 0;
      int oy = 0;
      inst->getLocation(ox, oy);
      const odb::dbTransform xform({ox, oy});  // translation only!
      for (odb::dbITerm* it : inst->getITerms()) {
        for (odb::dbAccessPoint* ap : it->getPrefAccessPoints()) {
          if (!ap) {
            continue;
          }
          odb::Point pt = ap->getPoint();
          xform.apply(pt);
          draw_ap(pt, ap->getLayer(), ap->hasAccess());
        }
      }
    }
  }
}

// Special "_regions" layer: dbRegion boundaries as semi-transparent
// filled rects with a 1px gray outline.  Mirrors GUI
// RenderThread::drawRegions (renderThread.cpp:1405-1426).
void TileGenerator::drawRegionsLayer(std::vector<unsigned char>& image,
                                     odb::dbBlock* block,
                                     const TileFrame& frame,
                                     const TileVisibility& /*vis*/) const
{
  const odb::Rect& dbu_tile = frame.cull;
  // GUI defaults: fill = region_color_ (0x70,0x70,0x70,0x70), outline
  // pen = Qt::gray — same gray as the die outline (kOutlineGray).
  constexpr Color kRegionFill{.r = 0x70, .g = 0x70, .b = 0x70, .a = 0x70};
  for (odb::dbRegion* region : block->getRegions()) {
    for (odb::dbBox* box : region->getBoundaries()) {
      const odb::Rect r = box->getBox();
      if (r.area() <= 0 || !r.overlaps(dbu_tile)) {
        continue;
      }
      drawFilledRect(
          image, toPixels(frame, r.intersect(dbu_tile)), kRegionFill);
      // Outline: same clamped edge drawing as the die/core outline.
      outlineRectInTile(image, r, kOutlineGray, frame);
    }
  }
}

// Special "_mfg_grid" layer: white dots at manufacturing-grid points.
// Mirrors GUI RenderThread::drawManufacturingGrid (renderThread.cpp:1359-1403).
void TileGenerator::drawMfgGridLayer(std::vector<unsigned char>& image,
                                     odb::dbBlock* block,
                                     const TileFrame& frame,
                                     const TileVisibility& vis) const
{
  const odb::Rect& dbu_tile = frame.cull;
  odb::dbTech* tech = block->getTech();
  if (!tech || !tech->hasManufacturingGrid()) {
    return;
  }
  const int grid = tech->getManufacturingGrid();  // DBU
  if (grid <= 0) {
    return;
  }
  const int dim = bufferDim(image);
  // Adaptive decimation instead of the Qt behaviour of hiding the grid whenever
  // points would be closer than kMinViewablePx.  A manufacturing grid is far
  // finer than a die: nangate45's 10 DBU grid on a 9.3 mm die only clears a
  // 5 px spacing at z=15, so the Qt rule makes the overlay unreachable in
  // practice (PR #10806 review).  Here we keep the on-screen spacing at
  // kMinViewablePx by drawing every k-th grid line: k == 1 is the exact grid
  // once it is legible, and below that this is a SUBGRID (k*grid), not the
  // literal manufacturing grid.
  //
  // k depends only on scale/dim — never on the tile — so neighbouring tiles at
  // the same zoom agree on the same absolute lattice and the seams line up.
  //
  // "Detailed view" tightens the target spacing, so the lattice gets denser and
  // lands closer to the real grid (measured on bp_quad: coverage 24% -> 37%,
  // dot period 5 px -> 4).  It stops at 4 px rather than the 1 px the toggle
  // uses for shapes, for two reasons:
  //  - a grid tiles the plane, unlike sparse shapes, and the tile is Lanczos-
  //    decimated on the way out, which spreads every dot over ~3 output px.
  //    Below ~4 px the dots merge into a solid white sheet that hides the
  //    design (measured at a 1 px and a 2 px target: every pixel of the tile
  //    lit), which is worse than showing nothing.
  //  - the target can never reach 0, because this loop is O(points in tile) —
  //    the raw grid on a 9.3 mm die at zoom-out is ~10^11 points per tile.
  constexpr double kDetailedGridPx = 4.0;
  const double target_px = vis.detailed ? kDetailedGridPx : kMinViewablePx;
  const double dbu_per_css = frame.px_per_css / frame.scale;
  const int k = std::max(
      1, static_cast<int>(std::ceil(target_px * dbu_per_css / grid)));
  const int step = k * grid;
  constexpr Color kGridDot{.r = 255, .g = 255, .b = 255, .a = 255};
  const int dot = hairlineCss(frame);
  // Anchor on absolute multiples of `step` (seamless across tiles).  Integer
  // division truncates toward zero, so snap explicitly to handle negative
  // coordinates (tile bounds include the label margin, which can go
  // negative near the origin); the Qt version only handles positive ones.
  int first_x = dbu_tile.xMin() / step * step;
  if (first_x < dbu_tile.xMin()) {
    first_x += step;
  }
  int last_x = dbu_tile.xMax() / step * step;
  if (last_x > dbu_tile.xMax()) {
    last_x -= step;
  }
  int first_y = dbu_tile.yMin() / step * step;
  if (first_y < dbu_tile.yMin()) {
    first_y += step;
  }
  int last_y = dbu_tile.yMax() / step * step;
  if (last_y > dbu_tile.yMax()) {
    last_y -= step;
  }
  for (int gx = first_x; gx <= last_x; gx += step) {
    const int px = toPxX(gx, frame);
    for (int gy = first_y; gy <= last_y; gy += step) {
      const int py = toPxY(gy, frame, dim);
      // One dot per grid point; `dot` px wide so it survives decimation.
      for (int dy = 0; dy < dot; ++dy) {
        for (int dx = 0; dx < dot; ++dx) {
          setPixel(image, px + dx, py + dy, kGridDot, dim);
        }
      }
    }
  }
}

// Special "_gcell_grid" layer: white grid lines at GCell boundaries.
// Mirrors GUI RenderThread::drawGCellGrid (renderThread.cpp:1304-1357).
void TileGenerator::drawGcellGridLayer(std::vector<unsigned char>& image,
                                       odb::dbBlock* block,
                                       const TileFrame& frame,
                                       const TileVisibility& /*vis*/) const
{
  const odb::Rect& dbu_tile = frame.cull;
  odb::dbGCellGrid* grid = block->getGCellGrid();
  const odb::Rect die_area = block->getDieArea();
  if (!grid || !dbu_tile.intersects(die_area)) {
    return;
  }
  const odb::Rect draw_bounds = dbu_tile.intersect(die_area);
  constexpr Color kGridLine{.r = 255, .g = 255, .b = 255, .a = 255};
  const int dim = bufferDim(image);
  // Pixel range of draw_bounds inside this tile (Y flipped).
  const int pxl = toPxX(draw_bounds.xMin(), frame);
  const int pxh = toPxX(draw_bounds.xMax(), frame);
  const int pyl = toPxY(draw_bounds.yMin(), frame, dim);
  const int pyh = toPxY(draw_bounds.yMax(), frame, dim);

  const int stroke = hairlineCss(frame);
  auto draw_v = [&](const int x) {
    if (x < draw_bounds.xMin() || draw_bounds.xMax() < x) {
      return;
    }
    const int px = toPxX(x, frame);
    drawLine(image, px, pyl, px, pyh, kGridLine, stroke);
  };
  auto draw_h = [&](const int y) {
    if (y < draw_bounds.yMin() || draw_bounds.yMax() < y) {
      return;
    }
    const int py = toPxY(y, frame, dim);
    drawLine(image, pxl, py, pxh, py, kGridLine, stroke);
  };

  // Per-block cache + binary search: only visit the grid lines inside
  // this tile instead of copying/scanning the full vectors.  Holding the
  // shared_ptr keeps the vectors alive across a concurrent cache clear.
  const GridList x_grid = gcellGridX(block);
  const GridList y_grid = gcellGridY(block);
  for (auto itx = std::ranges::lower_bound(*x_grid, draw_bounds.xMin());
       itx != x_grid->end() && *itx <= draw_bounds.xMax();
       ++itx) {
    draw_v(*itx);
  }
  for (auto ity = std::ranges::lower_bound(*y_grid, draw_bounds.yMin());
       ity != y_grid->end() && *ity <= draw_bounds.yMax();
       ++ity) {
    draw_h(*ity);
  }

  // Close the mesh at the die boundary.  The grid lines are the gcell
  // START edges (e.g. gcd/ihp: last line at 172800 vs die 185960), so the
  // top/right edges have no line.  The Qt GUI looks closed because it
  // always draws the die outline separately (renderThread.cpp:1188-1191);
  // the web viewer only draws a die outline for multi-die designs, so
  // close the grid here.
  draw_v(die_area.xMin());
  draw_v(die_area.xMax());
  draw_h(die_area.yMin());
  draw_h(die_area.yMax());
}

void TileGenerator::drawRudyLayer(std::vector<unsigned char>& image,
                                  odb::dbBlock* /*block*/,
                                  const TileFrame& frame,
                                  const TileVisibility& /*vis*/) const
{
  auto rudy = getHeatMapSource("RUDY");
  if (rudy) {
    drawHeatMap(image, *rudy, frame);
  }
}

/* static */
std::vector<std::string> TileGenerator::saveImageLayerOrder(
    const TileVisibility& vis,
    const std::vector<std::string>& tech_layers)
{
  // Bottom to top, by the SAME z order the client stacks the layers in on
  // screen (addPseudoLayer / the layer loop in display-controls.js): Leaflet
  // paints by zIndex, so a PNG composited in any other order is not the view
  // the user saw.  Compositing in registry order — every pseudo layer last —
  // put the manufacturing grid over the routing and the pin markers over the
  // tech layers.  These three mirror the client's non-pseudo values;
  // PseudoLayerDef::z_index carries the overlays'.
  constexpr int kInstancesZ = 0;
  constexpr int kPinsZ = 1;
  constexpr int kTechLayerZBase = 3;

  std::vector<std::pair<int, std::string>> ordered;
  ordered.reserve(tech_layers.size() + 2 + pseudoLayerDefs().size());
  ordered.emplace_back(kInstancesZ, "_instances");
  if (vis.pins) {
    ordered.emplace_back(kPinsZ, "_pins");
  }
  int layer_z = kTechLayerZBase;
  for (const std::string& name : tech_layers) {
    ordered.emplace_back(layer_z++, name);
  }
  for (const PseudoLayerDef& def : pseudoLayerDefs()) {
    if (vis.*def.flag) {
      ordered.emplace_back(def.z_index, def.name);
    }
  }
  std::ranges::stable_sort(ordered, {}, &std::pair<int, std::string>::first);

  std::vector<std::string> names;
  names.reserve(ordered.size());
  for (auto& [z, name] : ordered) {
    names.push_back(std::move(name));
  }
  return names;
}

const std::array<TileGenerator::PseudoLayerDef, 5>&
TileGenerator::pseudoLayerDefs()
{
  // z_index mirrors addPseudoLayer() in display-controls.js (see
  // saveImageLayerOrder); those values in turn follow the GUI's paint order
  // (renderThread.cpp:1201-1298), where the manufacturing grid goes down before
  // the routing layers and access points, regions and the gcell grid on top.
  static const std::array<PseudoLayerDef, 5> defs = {{
      {.name = "_access_points",
       .flag = &TileVisibility::access_points,
       .painter = &TileGenerator::drawAccessPointsLayer,
       .z_index = 1000},
      {.name = "_regions",
       .flag = &TileVisibility::regions,
       .painter = &TileGenerator::drawRegionsLayer,
       .z_index = 1001},
      {.name = "_mfg_grid",
       .flag = &TileVisibility::mfg_grid,
       .painter = &TileGenerator::drawMfgGridLayer,
       .z_index = 2},
      {.name = "_gcell_grid",
       .flag = &TileVisibility::gcell_grid,
       .painter = &TileGenerator::drawGcellGridLayer,
       .z_index = 1002},
      {.name = "_rudy",
       .flag = &TileVisibility::rudy,
       .painter = &TileGenerator::drawRudyLayer,
       .z_index = 1003},
  }};
  return defs;
}

std::vector<unsigned char> TileGenerator::renderTileBuffer(
    const std::string& layer,
    const int z,
    const int x,
    int y,
    const TileVisibility& vis,
    const std::vector<odb::Rect>& highlight_rects,
    const std::vector<odb::Polygon>& highlight_polys,
    const std::vector<ColoredRect>& colored_rects,
    const std::vector<FlightLine>& flight_lines,
    const std::map<uint32_t, Color>* module_colors,
    const std::set<uint32_t>* focus_net_ids,
    const std::set<uint32_t>* route_guide_net_ids,
    const double dpr,
    const int requested_tile_px) const
{
  static_assert(sizeof(Color) == 4);
  // Output tile size in physical pixels: exactly what the client will display
  // the tile in, so the image maps 1:1 onto the device grid (no browser
  // resampling → no re-aliased moiré).  The client sends the count rather than
  // letting it be derived, because a tile's CSS box is only a whole number of
  // device pixels when tileSize*dpr is an integer — at a 1.6667 display scale
  // 256 CSS px is 426.67 device px, and the rounded 428 would be resampled.
  // 0 means an older client that sends only dpr; fall back to 256*dpr.
  const int tile_px
      = requested_tile_px > 0
            ? requested_tile_px
            : static_cast<int>(std::lround(kTileSizeInPixel * dpr));
  // Band-limit factor: the tile is rasterized at tile_px*kCoverageSupersample
  // and Lanczos-2 decimated back to tile_px, prefiltering the dense periodic
  // geometry (bump arrays) that otherwise aliases into a moiré beat.
  const int super = tile_px * kCoverageSupersample;
  const int super_buffer_size = super * super * 4;
  // Super-pixels per CSS pixel (= dpr * supersample).  Pixel-specified sizes
  // (fonts, stroke widths, label-visibility thresholds) are authored in CSS px
  // and multiplied by this to render at the supersampled resolution, so they
  // look identical across dpr after decimation.  Taken from dpr rather than
  // from tile_px/256: with a device-exact tile size those differ slightly, and
  // a CSS pixel is defined by the display, not by the tile's pixel count.
  const double effective_dpr = dpr > 0.0 ? dpr : 1.0;
  const double super_per_css = kCoverageSupersample * effective_dpr;
  // The tile's CSS side length, which is 256 only while tile_px is 256*dpr.
  // Drives the sub-resolution cull below, whose threshold is authored in CSS
  // px.
  const double css_tile_px = tile_px / effective_dpr;
  // The OUTPUT (tile_px) buffer returned to the caller.  Every blank/early
  // return yields a transparent tile at the output resolution; the per-chiplet
  // loop draws into the supersampled `super_buffer` (allocated below), which is
  // decimated into this buffer after the loop.
  std::vector<unsigned char> world_image_buffer(
      static_cast<size_t>(tile_px) * tile_px * 4, 0);

  // No design loaded at all — return a blank raw RGBA buffer.  The
  // caller (generateTile) will PNG-encode it before sending to the
  // browser.  IMPORTANT: this contract returns *raw pixels*, never PNG
  // bytes; an earlier version PNG-encoded here, which then caused
  // lodepng to error out ("image too small to contain all pixels")
  // when generateTile tried to encode the small PNG buffer as
  // 256×256 RGBA.  We test for getChip(), not getBlock(), because
  // 3DBX/multi-die designs create a HIER design top chiplet that
  // itself has no block — the actual geometry lives in dbChipInst
  // master chips, which the chiplet loop below traverses.
  if (!getChip()) {
    return world_image_buffer;
  }

  // Per-layer colors are resolved per chiplet below, using each chiplet's
  // own dbTech.  A single global getLayerColorMap() call returns an empty
  // map in multi-tech (3DBlox) designs, which would paint every layer in
  // the fallback gray.

  // Determine our tile's bounding box in dbu coordinates.
  const double num_tiles_at_zoom = pow(2, z);
  if (x >= 0 && y >= 0 && x < num_tiles_at_zoom && y < num_tiles_at_zoom) {
    y = num_tiles_at_zoom - 1 - y;  // flip
    const odb::Rect full_bounds = getBounds();
    // Guard against an empty/invalid design footprint.  Without this,
    // tile_dbu_size becomes 0 and `scale` blows up to infinity, which
    // either produces garbage pixels or silently no-ops the render.
    if (full_bounds.maxDXDY() <= 0) {
      return world_image_buffer;
    }
    const double tile_dbu_size = full_bounds.maxDXDY() / num_tiles_at_zoom;
    const double scale = static_cast<double>(super) / tile_dbu_size;
    // Drawing happens in the supersampled buffer, so this frame is in
    // super-pixels; the output-resolution overlays below build their own.
    const TileFrame world_frame
        = tileFrame(full_bounds, x, y, tile_dbu_size, scale, super_per_css);
    const double dbu_x_min_world = world_frame.origin_x;
    const double dbu_y_min_world = world_frame.origin_y;
    const odb::Rect& dbu_tile_world = world_frame.cull;
    // Sub-resolution cull limit (see kMinViewablePx), passed to every Search::*
    // call below.  kMinViewablePx output CSS px in DBU = kMinViewablePx *
    // (DBU per output CSS px), and DBU-per-output-CSS-px = tile_dbu_size /
    // kTileSizeInPixel.  Zoomed in far enough this rounds to 0, which the
    // Search predicates treat as "no cull" — intended: when features are
    // already large there is nothing sub-resolution to drop.
    const int size_limit_dbu = static_cast<int>(
        std::lround(kMinViewablePx * tile_dbu_size / css_tile_px));
    // "Detailed view" (vis.detailed) relaxes the sub-resolution cull so small
    // features stay visible at zoom-out, mirroring the Qt GUI's Misc/"Detailed
    // view": instances are not culled at all (limit 0, like
    // LayoutViewer::instanceSizeLimit() in detailed/module view) and shapes
    // fall back to a 1 px limit (like shapeSizeLimit()) instead of the default
    // kMinViewablePx (5 px).  Off by default, so the moiré fix's 5 px cull is
    // unchanged in the normal view; enabling it knowingly re-admits the dense
    // sub-pixel geometry (and its moiré) — the same trade-off as the Qt view.
    const int instance_size_limit_dbu = vis.detailed ? 0 : size_limit_dbu;
    const int shape_size_limit_dbu
        = vis.detailed
              ? static_cast<int>(std::lround(1.0 * tile_dbu_size / css_tile_px))
              : size_limit_dbu;
    // The geometry the tile request resolves to: which slice of the design it
    // covers and at what resolution.  `y` here is already flipped out of the
    // client's Leaflet convention, so it will not match the requested y.  The
    // micron window depends only on the design bounds and z; tile_px/super and
    // the cull limits are the dpr-derived part.
    const double dbu_per_micron = db_->getDbuPerMicron();
    debugPrint(logger_,
               utl::WEB,
               "tile",
               2,
               "  tile frame: layer={} z={} x={} y_flipped={} "
               "um=({},{})-({},{}) tile_px={} super={} inst_limit_um={} "
               "shape_limit_um={}",
               layer,
               z,
               x,
               y,
               dbuToMicronString(dbu_tile_world.xMin(), dbu_per_micron),
               dbuToMicronString(dbu_tile_world.yMin(), dbu_per_micron),
               dbuToMicronString(dbu_tile_world.xMax(), dbu_per_micron),
               dbuToMicronString(dbu_tile_world.yMax(), dbu_per_micron),
               tile_px,
               super,
               dbuToMicronString(instance_size_limit_dbu, dbu_per_micron),
               dbuToMicronString(shape_size_limit_dbu, dbu_per_micron));

    // One snapshot for the whole tile: taken under a lock here, then read
    // lock-free by the per-chiplet loop below (see GeomCache).
    const std::shared_ptr<const GeomCache> geom_cache = geomCache();
    // Supersampled render buffer (RGBA, super x super).  The per-chiplet loop
    // draws into this; it is Lanczos-2 decimated into world_image_buffer after
    // the loop, before the (crisp, output-resolution) overlays are drawn.
    // thread_local (renders run one-per-thread) so the large buffer is reused
    // across tiles; assign() re-zeroes it (drawing is sparse, so it must start
    // transparent).
    static thread_local std::vector<unsigned char> super_buffer;
    super_buffer.assign(super_buffer_size, 0);

    // Per-chiplet rendering loop.  Mirrors RenderThread::drawChips() in
    // the Qt GUI: walks dbChip → dbChipInst → masterChip and draws each
    // chiplet's block in its own frame, accumulating into the same tile
    // image.  The world tile rect is shifted into each chiplet's local
    // frame for translation-only transforms; non-R0 orientations log
    // and skip for v1 (followup work to support full transforms).
    const std::vector<ChipletNode>& chiplet_nodes = chiplets();
    // Multi-die designs draw the die outline on EVERY layer pass (chiplet
    // demarcation).  Single-die designs draw die+core outlines too (Qt
    // drawChip does it unconditionally), but only on the always-rendered
    // "_instances" pass — drawing on every pass put a gray frame on every
    // tech-layer tile and broke every "expect transparent" test.
    const bool draw_die_outline = chiplet_nodes.size() > 1;
    // "_instances" pass: instance borders only (no routing) + the always-on
    // die/core outlines.
    const bool instances_only = (layer == "_instances");
    for (const ChipletNode& node : chiplet_nodes) {
      if (!vis.isChipletVisible(node.path)) {
        continue;
      }
      odb::dbBlock* block = node.block;
      if (!block || !node.chip) {
        continue;
      }
      odb::dbTech* tech = node.chip->getTech();
      if (!tech) {
        continue;
      }
      // Per-layer colors mirror gui::DisplayControls so the GUI and web
      // frontend agree on which color belongs to which layer.  Resolved per
      // chiplet because each chiplet has its own dbTech in 3DBlox designs.
      const auto& layer_colors = getLayerColorMap(tech);

      // Translation-only fast path: the local tile is the world tile
      // shifted by -offset, and pixel coordinates land in the same place
      // because both shape coords and tile origin are in the same local
      // frame.  Non-R0 orientations need full per-shape transforms; for
      // now we render them as if R0 (visible, but slightly misplaced).
      std::vector<unsigned char> local_image_buffer;
      // We only branch on the 2D part of the orient.  3DBlox "MZ"
      // (mirror about Z) is stored as {orient_2d=R0, mirror_z_=true} in
      // dbOrientType3D / dbTransform; in the XY plane that's the
      // identity, so the R0 fast-path produces correct pixels.  If
      // future renderers need to react to mirror_z_ (e.g. flipped pin
      // labels or 3D viewer parity) this branch is the place.
      const bool use_local
          = (node.world_xfm.getOrient() != odb::dbOrientType::R0);
      if (use_local) {
        local_image_buffer.resize(super_buffer_size, 0);
      }
      // Alias the buffer the chiplet loop writes into.  In the R0
      // fast-path it's the world buffer (so writes land directly).  In
      // the slow-path it's a per-chiplet local buffer that the
      // reverse-mapping block at the end of this iteration composites
      // back onto world_image_buffer.
      auto& image_buffer = use_local ? local_image_buffer : super_buffer;

      // This tile expressed in the chiplet's own frame.  A translation moves
      // the exact origin by exactly the offset; the query window moves with it.
      TileFrame frame = world_frame;
      if (use_local) {
        odb::dbTransform inv_xfm = node.world_xfm;
        inv_xfm.invert();
        inv_xfm.apply(frame.cull);
        // Non-R0 chiplets are drawn as if R0 (see above), so their origin comes
        // from the transformed window instead of an exact mapping of the world
        // corner — one more approximation in a path that is already one.
        frame.origin_x = frame.cull.xMin();
        frame.origin_y = frame.cull.yMin();
      } else {
        const odb::Point xfm_off = node.world_xfm.getOffset();
        frame.origin_x -= xfm_off.x();
        frame.origin_y -= xfm_off.y();
        frame.cull = odb::Rect(dbu_tile_world.xMin() - xfm_off.x(),
                               dbu_tile_world.yMin() - xfm_off.y(),
                               dbu_tile_world.xMax() - xfm_off.x(),
                               dbu_tile_world.yMax() - xfm_off.y());
      }
      // Local aliases: `dbu_tile` clips and queries, the `dbu_*` corners place
      // pixels.  Keep them apart — rounding the latter is what opens a seam.
      const odb::Rect& dbu_tile = frame.cull;
      const double dbu_x_min = frame.origin_x;
      const double dbu_y_min = frame.origin_y;
      const double dbu_x_max = frame.origin_x + tile_dbu_size;
      const double dbu_y_max = frame.origin_y + tile_dbu_size;

      // Per-layer fill pattern applied to this layer's own filled shapes:
      // routing segments, special-net shapes/vias and instance pins.  Instance
      // OBS stay solid (drawn with obs_color), matching the Qt GUI, which
      // brushes pins with the layer pattern but fills obstructions solid.
      // pat_ox/pat_oy anchor the pattern in absolute pixel space so the hatch
      // tiles seamlessly.
      const FillPattern layer_pattern = vis.fill_pattern;
      const int pat_ox = patternAnchor(dbu_x_min, scale);
      const int pat_oy = patternAnchor(dbu_y_min, scale);

      // Mirrors RenderThread::drawChip() in the Qt GUI: die outline on
      // every layer pass in multi-die designs (chiplet demarcation), and
      // on the _instances pass for all designs (Qt draws it always;
      // scoping to _instances keeps tech-layer tiles transparent).
      if (draw_die_outline || instances_only) {
        const odb::Rect die = block->getDieArea();
        if (die.area() > 0) {
          outlineRectInTile(image_buffer, die, kOutlineGray, frame);
        }
      }
      // Core area outline (Qt drawChip draws it right after the die).
      // Unset core -> empty polygon -> mergeInit()-inverted rect, so
      // check min<max explicitly rather than area()>0.
      if (instances_only) {
        const odb::Rect core = block->getCoreArea();
        if (core.xMin() < core.xMax() && core.yMin() < core.yMax()) {
          outlineRectInTile(image_buffer, core, kOutlineGray, frame);
        }
      }

      odb::dbTechLayer* tech_layer = tech->findLayer(layer.c_str());

      // This layer's slices of the geometry cache.  Null means no master (resp.
      // no via master) in the design has anything on this layer, so the
      // corresponding draw pass has nothing to do at all.
      const MasterGeomByLayer* layer_master_geom = nullptr;
      const ViaBoxesByMaster* layer_via_boxes = nullptr;
      if (tech_layer) {
        const auto mit = geom_cache->master_geom.find(tech_layer);
        if (mit != geom_cache->master_geom.end()) {
          layer_master_geom = &mit->second;
        }
        const auto vit = geom_cache->via_boxes.find(tech_layer);
        if (vit != geom_cache->via_boxes.end()) {
          layer_via_boxes = &vit->second;
        }
      }

      Color color{.r = 200, .g = 200, .b = 200, .a = 180};
      if (tech_layer) {
        const auto it = layer_colors.find(tech_layer);
        if (it != layer_colors.end()) {
          color = it->second;
        }
      }
      const Color obs_color = color.lighter();

      // Clip a shape's bbox to this tile and paint it.  Shared by every
      // per-layer shape below (routing wires/vias, special-net vias, master
      // obstructions, cell-pin boxes, pin-direction boxes, fills): they all do
      // overlaps -> intersect -> toPixels -> drawFilledRect.  pattern defaults
      // to solid; the shapes that honor the layer fill pattern (routing,
      // special-net, pins) pass layer_pattern, while OBS, fills and markers
      // stay solid by omitting it.
      auto draw_box_in_tile
          = [&](const odb::Rect& box,
                const Color& c,
                const FillPattern pattern = FillPattern::kSolid) {
              if (!box.overlaps(dbu_tile)) {
                return;
              }
              // image_buffer is the supersampled raster, so its side is
              // `super`; passing it skips a sqrt+lround per shape.
              drawFilledRect(image_buffer,
                             toPixels(frame, box.intersect(dbu_tile)),
                             c,
                             pattern,
                             pat_ox,
                             pat_oy,
                             super);
            };

      // Special "_modules" layer: draw filled module-colored rectangles
      const bool modules_layer
          = (layer == "_modules" && module_colors && !module_colors->empty());
      if (modules_layer) {
        // The module-colored overview shows every instance regardless of size
        // (mirrors Qt's instanceSizeLimit() == 0 in module view), so pass 0 —
        // no sub-resolution cull — instead of size_limit_dbu, which would empty
        // the map at zoom-out.
        for (odb::dbInst* inst : search_->searchInsts(block,
                                                      dbu_tile.xMin(),
                                                      dbu_tile.yMin(),
                                                      dbu_tile.xMax(),
                                                      dbu_tile.yMax(),
                                                      /*min_height=*/0)) {
          odb::Rect inst_bbox = inst->getBBox()->getBox();
          if (!dbu_tile.overlaps(inst_bbox)) {
            continue;
          }
          if (inst->getMaster()->isFiller()) {
            continue;
          }
          odb::dbModule* mod = inst->getModule();
          if (!mod) {
            continue;
          }
          auto it = module_colors->find(mod->getId());
          if (it == module_colors->end()) {
            continue;
          }
          const Color& c = it->second;
          const int pxl
              = std::max(0, (int) ((inst_bbox.xMin() - dbu_x_min) * scale));
          const int pyl
              = std::max(0, (int) ((inst_bbox.yMin() - dbu_y_min) * scale));
          const int pxh = std::min(
              super - 1,
              (int) std::ceil((inst_bbox.xMax() - dbu_x_min) * scale));
          const int pyh = std::min(
              super - 1,
              (int) std::ceil((inst_bbox.yMax() - dbu_y_min) * scale));
          for (int iy = pyl; iy < pyh; ++iy) {
            for (int ix = pxl; ix < pxh; ++ix) {
              blendPixel(image_buffer, ix, super - 1 - iy, c);
            }
          }
        }
      }

      // Special "_pins" layer: draw IO pin direction markers
      const bool pins_layer = (layer == "_pins");
      if (pins_layer && vis.pins) {
        const odb::Rect die_area = block->getDieArea();
        // Match GUI: scale markers to min(die, viewport) so they shrink
        // when zoomed in (GUI renderThread.cpp:1598-1602).
        const int die_max_dim = std::max(die_area.dx(), die_area.dy());
        const int tile_extent = static_cast<int>(tile_dbu_size);
        const int effective_dim = std::min(die_max_dim, tile_extent);
        const int pin_max_size
            = std::max(static_cast<int>(kPinMarkerSizeRatio * effective_dim),
                       kMinPinMarkerSize);
        const int qw = pin_max_size / 4;  // quarter-width of marker

        // Show pin names when the full (die-relative) marker is large enough
        // in pixels.  pin_max_size shrinks with zoom, but the die-relative
        // size grows as scale increases, so names appear when zoomed in.
        const int die_pin_size
            = std::max(static_cast<int>(kPinMarkerSizeRatio * die_max_dim),
                       kMinPinMarkerSize);
        const bool draw_pin_names = (static_cast<int>(die_pin_size * scale)
                                     >= kMinPinNameSizePixels * super_per_css);
        // Fonts are rasterized at the supersampled resolution so labels come
        // out the intended CSS size after the tile is decimated to tile_px.
        const auto pin_label_font = fontAtlasGetFont(
            static_cast<int>(std::lround(kPinLabelFontHeight * super_per_css)));

        // Marker templates (same as GUI renderThread.cpp).
        // Defined for "top edge" orientation; rotated per actual edge.
        using Pts = std::vector<odb::Point>;
        const Pts in_marker{// arrow pointing into block
                            {qw, pin_max_size},
                            {0, 0},
                            {-qw, pin_max_size},
                            {qw, pin_max_size}};
        const Pts out_marker{// arrow pointing out of block
                             {0, pin_max_size},
                             {-qw, 0},
                             {qw, 0},
                             {0, pin_max_size}};
        const Pts bi_marker{// diamond
                            {0, 0},
                            {-qw, pin_max_size / 2},
                            {0, pin_max_size},
                            {qw, pin_max_size / 2},
                            {0, 0}};

        // Iterate per-box like the GUI (each dbBox gets its own marker).
        for (odb::dbBTerm* term : block->getBTerms()) {
          // Respect net-type visibility (Power, Ground, etc.).
          if (!vis.isNetVisible(term->getNet())) {
            continue;
          }
          for (odb::dbBPin* pin : term->getBPins()) {
            const odb::dbPlacementStatus status = pin->getPlacementStatus();
            if (status == odb::dbPlacementStatus::NONE
                || status == odb::dbPlacementStatus::UNPLACED) {
              continue;
            }

            for (odb::dbBox* box : pin->getBoxes()) {
              if (!box) {
                continue;
              }

              // Skip pins on hidden tech layers.
              if (vis.has_visible_layers) {
                odb::dbTechLayer* box_layer = box->getTechLayer();
                if (box_layer
                    && !vis.visible_layers.contains(box_layer->getName())) {
                  continue;
                }
              }

              const odb::Rect box_rect = box->getBox();

              // Layer color for this box.
              Color marker_color{.r = 200, .g = 200, .b = 200, .a = 220};
              odb::dbTechLayer* pin_layer = box->getTechLayer();
              if (pin_layer) {
                const auto it = layer_colors.find(pin_layer);
                if (it != layer_colors.end()) {
                  marker_color = it->second;
                  marker_color.a = 220;
                }
              }

              // Center and edge distances from this specific box.
              const odb::Point pin_center = box_rect.center();

              const int dist_to_left
                  = std::abs(box_rect.xMin() - die_area.xMin());
              const int dist_to_right
                  = std::abs(box_rect.xMax() - die_area.xMax());
              const int dist_to_top
                  = std::abs(box_rect.yMax() - die_area.yMax());
              const int dist_to_bot
                  = std::abs(box_rect.yMin() - die_area.yMin());
              const std::array<int, 4> dists{
                  dist_to_left, dist_to_right, dist_to_top, dist_to_bot};
              const int arg_min = static_cast<int>(std::distance(
                  dists.begin(), std::ranges::min_element(dists)));

              odb::dbTransform xfm(pin_center);
              if (arg_min == 0) {  // left
                xfm.setOrient(odb::dbOrientType::R90);
                if (dist_to_left == 0) {
                  xfm.setOffset({die_area.xMin(), pin_center.y()});
                }
              } else if (arg_min == 1) {  // right
                xfm.setOrient(odb::dbOrientType::R270);
                if (dist_to_right == 0) {
                  xfm.setOffset({die_area.xMax(), pin_center.y()});
                }
              } else if (arg_min == 2) {  // top
                // No rotation needed.
                if (dist_to_top == 0) {
                  xfm.setOffset({pin_center.x(), die_area.yMax()});
                }
              } else {  // bottom
                xfm.setOrient(odb::dbOrientType::MX);
                if (dist_to_bot == 0) {
                  xfm.setOffset({pin_center.x(), die_area.yMin()});
                }
              }

              // Select template based on IO direction.
              const Pts* tmpl = &bi_marker;
              const auto pin_dir = term->getIoType();
              if (pin_dir == odb::dbIoType::INPUT) {
                tmpl = &in_marker;
              } else if (pin_dir == odb::dbIoType::OUTPUT) {
                tmpl = &out_marker;
              }

              // Transform template to final marker polygon.
              std::vector<odb::Point> marker_pts;
              marker_pts.reserve(tmpl->size());
              for (const auto& pt : *tmpl) {
                odb::Point new_pt = pt;
                xfm.apply(new_pt);
                marker_pts.push_back(new_pt);
              }
              const odb::Polygon marker_poly(marker_pts);

              // Only draw if marker intersects this tile.
              const odb::Rect marker_bbox = marker_poly.getEnclosingRect();
              if (marker_bbox.overlaps(dbu_tile)) {
                fillPolygon(image_buffer,
                            marker_poly,
                            frame,
                            marker_color,
                            /*blend=*/false,
                            FillPattern::kSolid,
                            super);
              }

              // Draw the box rect itself (same as GUI painter.drawRect).
              draw_box_in_tile(box_rect, marker_color);

              // Draw pin name label when zoomed in enough.
              if (draw_pin_names && vis.pin_names) {
                const std::string name = term->getName();
                const odb::Point anchor_pt = xfm.getOffset();
                const int text_w = getTextWidth(name, pin_label_font);
                const int text_h = getTextHeight(pin_label_font);
                const int text_margin_px = 3;
                const bool rotated = (arg_min == 2 || arg_min == 3);

                // For rotated text, width/height swap.
                const int block_w = rotated ? text_h : text_w;
                const int block_h = rotated ? text_w : text_h;

                // Convert anchor to pixel coords.
                const int anchor_px = static_cast<int>(
                    (anchor_pt.x() - dbu_tile.xMin()) * scale);
                const int anchor_py_raw = static_cast<int>(
                    (anchor_pt.y() - dbu_tile.yMin()) * scale);
                const int anchor_py = super - 1 - anchor_py_raw;

                // Position text outward (away from die center), matching the
                // GUI.
                const int marker_px = static_cast<int>(pin_max_size * scale);
                int px;
                int py;
                if (arg_min == 0) {  // left — text to the left (outward)
                  px = anchor_px - marker_px - text_margin_px - text_w;
                  py = anchor_py - text_h / 2;
                } else if (arg_min
                           == 1) {  // right — text to the right (outward)
                  px = anchor_px + marker_px + text_margin_px;
                  py = anchor_py - text_h / 2;
                } else if (arg_min
                           == 2) {  // top — rotated, above marker (outward)
                  px = anchor_px - block_w / 2;
                  py = anchor_py - marker_px - text_margin_px - block_h;
                } else {  // bottom — rotated, below marker (outward)
                  px = anchor_px - block_w / 2;
                  py = anchor_py + marker_px + text_margin_px;
                }

                if (px > -block_w && px < super && py > -block_h
                    && py < super) {
                  const Color text_color{.r = marker_color.r,
                                         .g = marker_color.g,
                                         .b = marker_color.b,
                                         .a = 255};
                  if (rotated) {
                    drawTextRotated(
                        image_buffer, px, py, name, pin_label_font, text_color);
                  } else {
                    drawText(
                        image_buffer, px, py, name, pin_label_font, text_color);
                  }
                }
              }
            }
          }
        }
      }

      // Self-painting pseudo layers (see pseudoLayerDefs): dispatch by
      // name and gate on the layer's visibility flag.
      bool pseudo_overlay = false;
      for (const PseudoLayerDef& def : pseudoLayerDefs()) {
        if (layer == def.name) {
          pseudo_overlay = true;
          if (vis.*def.flag) {
            (this->*def.painter)(image_buffer, block, frame, vis);
          }
          break;
        }
      }

      // On a tech-layer tile the per-instance pass paints only master
      // obstructions (vis.blockages) and cell-pin shapes (vis.inst_pins).  It
      // draws nothing when both are off, or when no master has geometry on this
      // layer at all (implant/marker layers, upper metals) — yet the query
      // still runs, and at zoom-out its box spans the design, so it walks every
      // instance and throws the result away.  Skip it in those cases.  A null
      // tech_layer means "no layer filter" (the _instances tile, or a layer
      // name this chiplet's tech doesn't have), in which case the pass draws
      // unconditionally and must not be skipped.
      const bool inst_pass_draws
          = instances_only || !tech_layer
            || ((vis.blockages || vis.inst_pins) && layer_master_geom);

      // Pseudo layers ("_modules", "_pins" and the overlays above) handle
      // their own drawing; skip all other drawing (instances, routing, etc.)
      const bool pseudo_layer = modules_layer || pins_layer || pseudo_overlay;
      if (!pseudo_layer) {
        const auto iterm_font = fontAtlasGetFont(static_cast<int>(
            std::lround(kItermLabelFontHeight * super_per_css)));
        const int iterm_font_h = getTextHeight(iterm_font);

        // Draw instances.  instance_size_limit_dbu culls sub-resolution
        // instances at the RTree level (Qt-parity), so dense bump arrays vanish
        // at zoom-out — unless "Detailed view" is on, which sets the limit to
        // 0.
        const Search::InstRange insts
            = inst_pass_draws ? search_->searchInsts(block,
                                                     dbu_tile.xMin(),
                                                     dbu_tile.yMin(),
                                                     dbu_tile.xMax(),
                                                     dbu_tile.yMax(),
                                                     instance_size_limit_dbu)
                              : Search::InstRange{};
        for (odb::dbInst* inst : insts) {
          odb::Rect inst_bbox = inst->getBBox()->getBox();
          if (!dbu_tile.overlaps(inst_bbox)) {
            continue;
          }
          odb::dbMaster* master = inst->getMaster();

          if (!vis.isInstVisible(inst, sta_)) {
            continue;
          }
          const int xl = inst_bbox.xMin();
          const int yl = inst_bbox.yMin();
          const int xh = inst_bbox.xMax();
          const int yh = inst_bbox.yMax();

          const int64_t pixel_xl = (int64_t) ((xl - dbu_x_min) * scale);
          const int64_t pixel_yl = (int64_t) ((yl - dbu_y_min) * scale);
          const int64_t pixel_xh
              = (int64_t) std::ceil((xh - dbu_x_min) * scale);
          const int64_t pixel_yh
              = (int64_t) std::ceil((yh - dbu_y_min) * scale);

          const int loop_xl = std::clamp<int64_t>(pixel_xl, 0, super);
          const int loop_yl = std::clamp<int64_t>(pixel_yl, 0, super);
          const int loop_xh = std::clamp<int64_t>(pixel_xh, 0, super);
          const int loop_yh = std::clamp<int64_t>(pixel_yh, 0, super);

          const int draw_xl = std::clamp<int64_t>(pixel_xl, 0, super - 1);
          const int draw_yl = std::clamp<int64_t>(pixel_yl, 0, super - 1);
          const int draw_xh = std::clamp<int64_t>(pixel_xh, 0, super - 1);
          const int draw_yh = std::clamp<int64_t>(pixel_yh, 0, super - 1);

          // Sub-resolution instances (incl. dense bump arrays) were already
          // culled by searchInsts(size_limit_dbu) above — matching the Qt GUI —
          // so everything reaching here is large enough to draw discretely; no
          // coverage-tint LOD (which read as a merged "sheet").

          if (instances_only) {
            // Draw the rectangle border (instances-only layer)
            const Color gray{.r = 128, .g = 128, .b = 128, .a = 255};
            if (dbu_x_min <= xl && xl <= dbu_x_max) {
              for (int iy = loop_yl; iy < loop_yh; ++iy) {
                const int draw_y = (super - 1 - iy);
                setPixel(image_buffer, draw_xl, draw_y, gray);
              }
            }
            if (dbu_x_min <= xh && xh <= dbu_x_max) {
              for (int iy = loop_yl; iy < loop_yh; ++iy) {
                const int draw_y = (super - 1 - iy);
                setPixel(image_buffer, draw_xh, draw_y, gray);
              }
            }
            if (dbu_y_min <= yl && yl <= dbu_y_max) {
              for (int ix = loop_xl; ix < loop_xh; ++ix) {
                const int draw_y = (super - 1 - draw_yl);
                setPixel(image_buffer, ix, draw_y, gray);
              }
            }
            if (dbu_y_min <= yh && yh <= dbu_y_max) {
              for (int ix = loop_xl; ix < loop_xh; ++ix) {
                const int draw_y = (super - 1 - draw_yh);
                setPixel(image_buffer, ix, draw_y, gray);
              }
            }

            // Draw instance name label when zoomed in enough.
            // Font scales to ~40% of the smaller box dimension, clamped
            // to [kMinInstNameFontPx, kMaxInstNameFontPx].  Text is
            // elided from the left ("...suffix") to fit 90% of the
            // available dimension, matching the Qt GUI's behavior.
            if (vis.inst_names) {
              const int box_px_w = (int) (pixel_xh - pixel_xl);
              const int box_px_h = (int) (pixel_yh - pixel_yl);
              const int box_px_min = std::min(box_px_w, box_px_h);
              if (std::max(box_px_w, box_px_h)
                  >= kMinInstNameBoxPx * super_per_css) {
                const int font_px = std::clamp(
                    static_cast<int>(box_px_min * 0.4),
                    static_cast<int>(
                        std::lround(kMinInstNameFontPx * super_per_css)),
                    static_cast<int>(
                        std::lround(kMaxInstNameFontPx * super_per_css)));
                const auto inst_font = fontAtlasGetFont(font_px);
                const int font_h = getTextHeight(inst_font);

                // Skip if font would dominate the cell (> 50% of cross
                // dimension), matching GUI's kNonCoreScaleLimit = 2.0.
                if (2 * font_h <= box_px_min) {
                  constexpr Color name_color{
                      .r = 255, .g = 255, .b = 0, .a = 220};
                  const std::string full_name = inst->getName();
                  const int full_w = getTextWidth(full_name, inst_font);

                  // Rotate if taller than wide and text overflows (85%).
                  const bool rotate
                      = (box_px_h > box_px_w) && (full_w > box_px_w * 85 / 100);

                  // Available width for text (90% of relevant dim).
                  const int avail
                      = rotate ? (box_px_h * 9 / 10) : (box_px_w * 9 / 10);

                  // Elide from the left if text is too wide.  Maintain a
                  // running prefix width so each candidate "..." +
                  // name.substr(skip) is evaluated in O(1) using
                  //   textWidth(name.substr(skip))
                  //     = full_w - prefix_w - kern(name[skip-1], name[skip])
                  // giving O(N) total instead of O(N^2).
                  std::string name = full_name;
                  int text_w = full_w;
                  if (text_w > avail && name.size() > 4) {
                    const int dots_w = getTextWidth("...", inst_font);
                    const size_t n = name.size();
                    int prefix_w = 0;
                    for (size_t skip = 1; skip < n - 1; ++skip) {
                      prefix_w += inst_font.glyph(name[skip - 1]).advance;
                      if (skip >= 2) {
                        prefix_w
                            += inst_font.kern(name[skip - 2], name[skip - 1]);
                      }
                      const int suffix_w
                          = full_w - prefix_w
                            - inst_font.kern(name[skip - 1], name[skip]);
                      const int w
                          = dots_w + inst_font.kern('.', name[skip]) + suffix_w;
                      if (w <= avail) {
                        name = "..." + name.substr(skip);
                        text_w = w;
                        break;
                      }
                    }
                  }

                  // Center of instance bbox in pixel coords.
                  const int64_t cx = (pixel_xl + pixel_xh) / 2;
                  const int64_t cy = super - 1 - (pixel_yl + pixel_yh) / 2;

                  if (rotate) {
                    const int64_t px = cx - font_h / 2;
                    const int64_t py = cy - text_w / 2;
                    if (px > -font_h && px < super && py > -text_w
                        && py < super) {
                      drawTextRotated(image_buffer,
                                      (int) px,
                                      (int) py,
                                      name,
                                      inst_font,
                                      name_color);
                    }
                  } else {
                    const int64_t px = cx - text_w / 2;
                    const int64_t py = cy - font_h / 2;
                    if (px > -text_w && px < super && py > -font_h
                        && py < super) {
                      drawText(image_buffer,
                               (int) px,
                               (int) py,
                               name,
                               inst_font,
                               name_color);
                    }
                  }
                }
              }
            }
          } else if (!tech_layer) {
            // No layer filter (a layer name this chiplet's tech doesn't have):
            // draw every master shape in the fallback color, as before.  The
            // geometry cache is layer-keyed and so cannot serve this; it is not
            // a hot path — only multi-tech designs reach it.
            if (vis.blockages) {
              for (odb::dbPolygon* poly_obs :
                   master->getPolygonObstructions()) {
                odb::Polygon poly = poly_obs->getPolygon();
                inst->getTransform().apply(poly);
                fillPolygon(image_buffer,
                            poly,
                            frame,
                            obs_color,
                            /*blend=*/false,
                            FillPattern::kSolid,
                            super);
              }
              for (odb::dbBox* obs : master->getObstructions(false)) {
                odb::Rect box = obs->getBox();
                inst->getTransform().apply(box);
                draw_box_in_tile(box, obs_color);
              }
            }

            if (vis.inst_pins) {
              for (odb::dbMTerm* mterm : master->getMTerms()) {
                for (odb::dbMPin* mpin : mterm->getMPins()) {
                  for (odb::dbPolygon* poly_geom : mpin->getPolygonGeometry()) {
                    odb::Polygon poly = poly_geom->getPolygon();
                    inst->getTransform().apply(poly);
                    fillPolygon(image_buffer,
                                poly,
                                frame,
                                color,
                                /*blend=*/false,
                                layer_pattern,
                                super);
                  }
                  for (odb::dbBox* geom : mpin->getGeometry(false)) {
                    odb::Rect box = geom->getBox();
                    inst->getTransform().apply(box);
                    draw_box_in_tile(box, color, layer_pattern);
                  }
                }
              }
            }

            // Draw ITerm name labels when zoomed in and pins are visible.
            if (vis.inst_pins && vis.inst_pin_names) {
              constexpr Color iterm_label_color{
                  .r = 255, .g = 255, .b = 0, .a = 220};
              const odb::dbTransform xfm = inst->getTransform();

              for (odb::dbMTerm* mterm : master->getMTerms()) {
                bool drawn = false;
                for (odb::dbMPin* mpin : mterm->getMPins()) {
                  for (odb::dbBox* geom : mpin->getGeometry(false)) {
                    odb::Rect box = geom->getBox();
                    xfm.apply(box);
                    if (!box.overlaps(dbu_tile)) {
                      continue;
                    }

                    // Skip if pin box is too small in pixels.
                    const int box_px_w = static_cast<int>(box.dx() * scale);
                    const int box_px_h = static_cast<int>(box.dy() * scale);
                    if (box_px_w < kMinItermLabelBoxPx * super_per_css
                        && box_px_h < kMinItermLabelBoxPx * super_per_css) {
                      continue;
                    }

                    const std::string name(mterm->getName());
                    const int text_w = getTextWidth(name, iterm_font);

                    // Center of pin box in pixel coords.
                    const odb::Point center = box.center();
                    const int cx = static_cast<int>(
                        (center.x() - dbu_tile.xMin()) * scale);
                    const int cy = super - 1
                                   - static_cast<int>(
                                       (center.y() - dbu_tile.yMin()) * scale);

                    // Rotate 90° if box is taller than wide and text overflows.
                    const bool rotate
                        = (box_px_h > box_px_w) && (text_w > box_px_w);

                    if (rotate) {
                      const int px = cx - iterm_font_h / 2;
                      const int py = cy - text_w / 2;
                      if (px > -iterm_font_h && px < super && py > -text_w
                          && py < super) {
                        drawTextRotated(image_buffer,
                                        px,
                                        py,
                                        name,
                                        iterm_font,
                                        iterm_label_color);
                      }
                    } else {
                      const int px = cx - text_w / 2;
                      const int py = cy - iterm_font_h / 2;
                      if (px > -text_w && px < super && py > -iterm_font_h
                          && py < super) {
                        drawText(image_buffer,
                                 px,
                                 py,
                                 name,
                                 iterm_font,
                                 iterm_label_color);
                      }
                    }

                    drawn = true;
                    break;  // only label first geometry per pin
                  }
                  if (drawn) {
                    break;
                  }
                }
              }
            }
          } else if (const MasterLayerGeom* mg
                     = findMasterGeom(layer_master_geom, master)) {
            // Layer-specific obstructions and pins, read straight out of the
            // geometry cache: this master's shapes ON THIS LAYER, with no
            // getTechLayer() calls and no walk over geometry belonging to other
            // layers.  A master with nothing here was already skipped by
            // findMasterGeom returning null.
            const odb::dbTransform xfm = inst->getTransform();

            if (vis.blockages) {
              for (const odb::Polygon& src : mg->obs_polys) {
                odb::Polygon poly = src;
                xfm.apply(poly);
                fillPolygon(image_buffer,
                            poly,
                            frame,
                            obs_color,
                            /*blend=*/false,
                            FillPattern::kSolid,
                            super);
              }
              for (const odb::Rect& src : mg->obs_boxes) {
                odb::Rect box = src;
                xfm.apply(box);
                draw_box_in_tile(box, obs_color);
              }
            }

            if (vis.inst_pins) {
              for (const odb::Polygon& src : mg->pin_polys) {
                odb::Polygon poly = src;
                xfm.apply(poly);
                fillPolygon(image_buffer,
                            poly,
                            frame,
                            color,
                            /*blend=*/false,
                            layer_pattern,
                            super);
              }
              for (const auto& [mterm, boxes] : mg->pin_boxes) {
                for (const odb::Rect& src : boxes) {
                  odb::Rect box = src;
                  xfm.apply(box);
                  draw_box_in_tile(box, color, layer_pattern);
                }
              }
            }

            // Draw ITerm name labels when zoomed in and pins are visible.
            // One label per pin: the first box big enough and inside the tile,
            // in the same order the master declares them.
            if (vis.inst_pins && vis.inst_pin_names) {
              constexpr Color iterm_label_color{
                  .r = 255, .g = 255, .b = 0, .a = 220};

              for (const auto& [mterm, boxes] : mg->pin_boxes) {
                for (const odb::Rect& src : boxes) {
                  odb::Rect box = src;
                  xfm.apply(box);
                  if (!box.overlaps(dbu_tile)) {
                    continue;
                  }

                  // Skip if pin box is too small in pixels.
                  const int box_px_w = static_cast<int>(box.dx() * scale);
                  const int box_px_h = static_cast<int>(box.dy() * scale);
                  if (box_px_w < kMinItermLabelBoxPx * super_per_css
                      && box_px_h < kMinItermLabelBoxPx * super_per_css) {
                    continue;
                  }

                  const std::string name(mterm->getName());
                  const int text_w = getTextWidth(name, iterm_font);

                  // Center of pin box in pixel coords.
                  const odb::Point center = box.center();
                  const int cx = static_cast<int>((center.x() - dbu_tile.xMin())
                                                  * scale);
                  const int cy = super - 1
                                 - static_cast<int>(
                                     (center.y() - dbu_tile.yMin()) * scale);

                  // Rotate 90° if box is taller than wide and text overflows.
                  const bool rotate
                      = (box_px_h > box_px_w) && (text_w > box_px_w);

                  if (rotate) {
                    const int px = cx - iterm_font_h / 2;
                    const int py = cy - text_w / 2;
                    if (px > -iterm_font_h && px < super && py > -text_w
                        && py < super) {
                      drawTextRotated(image_buffer,
                                      px,
                                      py,
                                      name,
                                      iterm_font,
                                      iterm_label_color);
                    }
                  } else {
                    const int px = cx - text_w / 2;
                    const int py = cy - iterm_font_h / 2;
                    if (px > -text_w && px < super && py > -iterm_font_h
                        && py < super) {
                      drawText(image_buffer,
                               px,
                               py,
                               name,
                               iterm_font,
                               iterm_label_color);
                    }
                  }

                  break;  // only label first geometry per pin
                }
              }
            }
          }
        }

        // Draw routing shapes (wires, vias) and BTerm shapes on top of
        // instances
        if (!instances_only && tech_layer && (vis.routing || vis.pins)) {
          for (const auto& shape :
               search_->searchBoxShapes(block,
                                        tech_layer,
                                        dbu_tile.xMin(),
                                        dbu_tile.yMin(),
                                        dbu_tile.xMax(),
                                        dbu_tile.yMax(),
                                        shape_size_limit_dbu)) {
            const auto type = std::get<1>(shape);
            if (type == Search::kBterm && !vis.pins) {
              continue;
            }
            if (type == Search::kWire
                && !(vis.routing && vis.routing_segments)) {
              continue;
            }
            if (type == Search::kVia && !(vis.routing && vis.routing_vias)) {
              continue;
            }
            odb::dbNet* net = std::get<2>(shape);
            if (!vis.isNetVisible(net)) {
              continue;
            }
            if (focus_net_ids && !focus_net_ids->empty()
                && !focus_net_ids->contains(net->getId())) {
              continue;
            }
            const odb::Rect& box = std::get<0>(shape);
            draw_box_in_tile(box, color, layer_pattern);
          }
        }

        // Draw special net shapes (power/ground straps) on top of instances
        if (!instances_only && tech_layer && vis.special_nets
            && vis.srouting_segments) {
          for (const auto& shape :
               search_->searchSNetShapes(block,
                                         tech_layer,
                                         dbu_tile.xMin(),
                                         dbu_tile.yMin(),
                                         dbu_tile.xMax(),
                                         dbu_tile.yMax(),
                                         shape_size_limit_dbu)) {
            odb::dbNet* snet = std::get<2>(shape);
            if (!vis.isNetVisible(snet)) {
              continue;
            }
            if (focus_net_ids && !focus_net_ids->empty()
                && !focus_net_ids->contains(snet->getId())) {
              continue;
            }
            const odb::Rect box = std::get<0>(shape)->getBox();
            if (!box.overlaps(dbu_tile)) {
              continue;
            }
            const odb::Polygon& poly = std::get<1>(shape);
            fillPolygon(image_buffer,
                        poly,
                        frame,
                        color,
                        /*blend=*/false,
                        layer_pattern,
                        super);
          }
        }

        // Draw special net vias — decompose into individual cut boxes
        // layer_via_boxes null ⇒ no via master in the design has a box on this
        // layer, so the search would return vias none of whose boxes could be
        // drawn.  Skip the query too, not just the inner loop.
        if (!instances_only && tech_layer && vis.special_nets
            && vis.srouting_vias && layer_via_boxes) {
          for (const auto& shape :
               search_->searchSNetViaShapes(block,
                                            tech_layer,
                                            dbu_tile.xMin(),
                                            dbu_tile.yMin(),
                                            dbu_tile.xMax(),
                                            dbu_tile.yMax(),
                                            shape_size_limit_dbu)) {
            odb::dbNet* via_net = std::get<1>(shape);
            if (!vis.isNetVisible(via_net)) {
              continue;
            }
            if (focus_net_ids && !focus_net_ids->empty()
                && !focus_net_ids->contains(via_net->getId())) {
              continue;
            }
            odb::dbSBox* sbox = std::get<0>(shape);
            const std::vector<odb::Rect>* via_boxes
                = findViaBoxes(layer_via_boxes, sbox);
            if (!via_boxes) {
              continue;
            }
            const odb::Point origin((sbox->xMin() + sbox->xMax()) / 2,
                                    (sbox->yMin() + sbox->yMax()) / 2);
            for (const odb::Rect& src : *via_boxes) {
              odb::Rect box = src;
              box.moveDelta(origin.x(), origin.y());
              draw_box_in_tile(box, color, layer_pattern);
            }
          }
        }

        // Draw via enclosures from adjacent cut layers onto this metal layer.
        // Vias are indexed by their cut layer in the search structure.  When
        // rendering a routing layer we look up the cut layers immediately above
        // and below, search for vias there, and draw only the enclosure boxes
        // that belong to the current routing layer.
        if (!instances_only && tech_layer && vis.special_nets
            && vis.srouting_vias && layer_via_boxes
            && tech_layer->getType() == odb::dbTechLayerType::ROUTING) {
          odb::dbTechLayer* adj_cuts[2]
              = {tech_layer->getLowerLayer(), tech_layer->getUpperLayer()};
          for (odb::dbTechLayer* cut_layer : adj_cuts) {
            if (!cut_layer
                || cut_layer->getType() != odb::dbTechLayerType::CUT) {
              continue;
            }
            for (const auto& shape :
                 search_->searchSNetViaShapes(block,
                                              cut_layer,
                                              dbu_tile.xMin(),
                                              dbu_tile.yMin(),
                                              dbu_tile.xMax(),
                                              dbu_tile.yMax(),
                                              shape_size_limit_dbu)) {
              odb::dbNet* via_net = std::get<1>(shape);
              if (!vis.isNetVisible(via_net)) {
                continue;
              }
              if (focus_net_ids && !focus_net_ids->empty()
                  && !focus_net_ids->contains(via_net->getId())) {
                continue;
              }
              odb::dbSBox* sbox = std::get<0>(shape);
              // Indexed by the layer being DRAWN (tech_layer, the routing
              // layer), not the cut layer being searched — these are the via's
              // enclosure boxes landing on this metal.
              const std::vector<odb::Rect>* via_boxes
                  = findViaBoxes(layer_via_boxes, sbox);
              if (!via_boxes) {
                continue;
              }
              const odb::Point origin((sbox->xMin() + sbox->xMax()) / 2,
                                      (sbox->yMin() + sbox->yMax()) / 2);
              for (const odb::Rect& src : *via_boxes) {
                odb::Rect box = src;
                box.moveDelta(origin.x(), origin.y());
                draw_box_in_tile(box, color, layer_pattern);
              }
            }
          }
        }

        // Draw placement blockages (dbBlockage) on the _instances layer.
        // Diagonal white hash lines in pixel space, with period anchored in dbu
        // coordinates so the pattern is seamless across tile boundaries.
        if (instances_only && vis.placement_blockages) {
          const Color hash_color{.r = 255, .g = 255, .b = 255, .a = 180};
          // In output pixels; scaled to the supersampled raster grid.
          const int kPixelPeriod
              = static_cast<int>(std::lround(20 * super_per_css));
          const int kLineWidth
              = static_cast<int>(std::lround(2 * super_per_css));
          for (odb::dbBlockage* blk :
               search_->searchBlockages(block,
                                        dbu_tile.xMin(),
                                        dbu_tile.yMin(),
                                        dbu_tile.xMax(),
                                        dbu_tile.yMax(),
                                        shape_size_limit_dbu)) {
            odb::Rect box = blk->getBBox()->getBox();
            if (!box.overlaps(dbu_tile)) {
              continue;
            }
            const odb::Rect overlap = box.intersect(dbu_tile);
            const odb::Rect draw = toPixels(frame, overlap);
            // Offset in absolute pixel coordinates for seamless tiling
            const int ox = latticeAnchor(dbu_x_min, scale, kPixelPeriod);
            const int oy = latticeAnchor(dbu_y_min, scale, kPixelPeriod);
            for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
              for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
                if (((ix + ox) + (iy + oy)) % kPixelPeriod < kLineWidth) {
                  blendPixel(image_buffer, ix, super - 1 - iy, hash_color);
                }
              }
            }
          }
        }

        // Draw routing obstructions (dbObstruction) on per-layer tiles.
        // Same diagonal white hash lines.
        if (!instances_only && tech_layer && vis.routing_obstructions) {
          const Color hash_color{.r = 255, .g = 255, .b = 255, .a = 180};
          const int kPixelPeriod
              = static_cast<int>(std::lround(20 * super_per_css));
          const int kLineWidth
              = static_cast<int>(std::lround(2 * super_per_css));
          for (odb::dbObstruction* obs :
               search_->searchObstructions(block,
                                           tech_layer,
                                           dbu_tile.xMin(),
                                           dbu_tile.yMin(),
                                           dbu_tile.xMax(),
                                           dbu_tile.yMax(),
                                           shape_size_limit_dbu)) {
            odb::Rect box = obs->getBBox()->getBox();
            if (!box.overlaps(dbu_tile)) {
              continue;
            }
            const odb::Rect overlap = box.intersect(dbu_tile);
            const odb::Rect draw = toPixels(frame, overlap);
            const int ox = latticeAnchor(dbu_x_min, scale, kPixelPeriod);
            const int oy = latticeAnchor(dbu_y_min, scale, kPixelPeriod);
            for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
              for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
                if (((ix + ox) + (iy + oy)) % kPixelPeriod < kLineWidth) {
                  blendPixel(image_buffer, ix, super - 1 - iy, hash_color);
                }
              }
            }
          }
        }

        // Draw metal fill (dbFill) on per-layer tiles.  Mirrors the GUI, which
        // draws fills in a darker variant of the layer color (lighter(50)).
        if (!instances_only && tech_layer && vis.fills) {
          const Color fill_color = color.darken(0.5);
          // Cull sub-pixel fills at low zoom, mirroring the GUI's shape_limit
          // (renderThread.cpp passes fineViewableResolution).  scale is
          // pixels/DBU, so 1/scale is the DBU span of one pixel; 0 at high
          // zoom leaves searchFills unfiltered.
          const int min_size = static_cast<int>(1.0 / scale);
          for (odb::dbFill* fill : search_->searchFills(block,
                                                        tech_layer,
                                                        dbu_tile.xMin(),
                                                        dbu_tile.yMin(),
                                                        dbu_tile.xMax(),
                                                        dbu_tile.yMax(),
                                                        min_size)) {
            odb::Rect box;
            fill->getRect(box);
            draw_box_in_tile(box, fill_color);
          }
        }

        // Draw rows (and individual sites when zoomed in) on _instances layer.
        if (instances_only && vis.rows) {
          const Color row_color{
              .r = 60, .g = 180, .b = 60, .a = 180};  // green outlines

          // Lambda to draw a rectangle outline.
          auto draw_outline = [&](const odb::Rect& rect) {
            const odb::Rect draw = toPixels(frame, rect);
            for (int ix = draw.xMin(); ix <= draw.xMax(); ++ix) {
              blendPixel(image_buffer, ix, super - 1 - draw.yMin(), row_color);
              blendPixel(image_buffer, ix, super - 1 - draw.yMax(), row_color);
            }
            for (int iy = draw.yMin(); iy <= draw.yMax(); ++iy) {
              blendPixel(image_buffer, draw.xMin(), super - 1 - iy, row_color);
              blendPixel(image_buffer, draw.xMax(), super - 1 - iy, row_color);
            }
          };

          for (const auto& [row_rect, row] :
               search_->searchRows(block,
                                   dbu_tile.xMin(),
                                   dbu_tile.yMin(),
                                   dbu_tile.xMax(),
                                   dbu_tile.yMax(),
                                   shape_size_limit_dbu)) {
            if (!row_rect.overlaps(dbu_tile)) {
              continue;
            }
            odb::dbSite* site = row->getSite();
            if (site && !vis.isSiteVisible(site->getName())) {
              continue;
            }

            // Always draw the row outline.
            draw_outline(row_rect);

            // Draw individual sites when zoomed in enough (site >= 5px).
            // Matches GUI nominalViewableResolution threshold.
            if (site) {
              int site_w = site->getWidth();
              int site_h = site->getHeight();

              // Swap dimensions for rotated orientations.
              switch (row->getOrient().getValue()) {
                case odb::dbOrientType::R90:
                case odb::dbOrientType::R270:
                case odb::dbOrientType::MYR90:
                case odb::dbOrientType::MXR90:
                  std::swap(site_w, site_h);
                  break;
                default:
                  break;
              }

              const int site_w_px = static_cast<int>(site_w * scale);
              if (site_w_px >= 5 * super_per_css) {
                odb::Point pt = row->getOrigin();
                const int spacing = row->getSpacing();
                const int count = row->getSiteCount();
                const bool horizontal
                    = (row->getDirection() == odb::dbRowDir::HORIZONTAL);

                for (int i = 0; i < count; ++i) {
                  const odb::Rect site_rect(
                      pt.x(), pt.y(), pt.x() + site_w, pt.y() + site_h);
                  if (site_rect.overlaps(dbu_tile)) {
                    draw_outline(site_rect);
                  }
                  if (horizontal) {
                    pt.addX(spacing);
                  } else {
                    pt.addY(spacing);
                  }
                }
              }
            }
          }
        }

        // Draw tracks on per-layer tiles
        if (!instances_only && tech_layer
            && (vis.tracks_pref || vis.tracks_non_pref)) {
          odb::dbTrackGrid* grid = block->findTrackGrid(tech_layer);
          debugPrint(logger_,
                     utl::WEB,
                     "tile",
                     1,
                     "tracks: layer={} grid={} pref={} non_pref={}",
                     layer,
                     grid != nullptr,
                     vis.tracks_pref,
                     vis.tracks_non_pref);
          // Clip the tracks to the die area, as the Qt GUI does
          // (RenderThread::drawTracks: draw_bounds = die.intersect(bounds)).
          // A track line drawn to the tile edge runs past the chip in any
          // design whose block bbox reaches beyond the die area — and the
          // viewport follows that bbox (see getBounds) — leaving tracks
          // floating outside the die.  Same shape as drawGCellGridLayer.
          const odb::Rect die_area = block->getDieArea();
          if (grid && dbu_tile.intersects(die_area)) {
            const odb::Rect draw_bounds = dbu_tile.intersect(die_area);
            // Span of the clipped region in buffer pixels (Y flipped).  Shared
            // by every track of this tile, so neighbouring tiles agree on where
            // the lines stop and the seams stay aligned.
            const int pxl = toPxX(draw_bounds.xMin(), frame);
            const int pxh = toPxX(draw_bounds.xMax(), frame);
            const int pyl = toPxY(draw_bounds.yMin(), frame, super);
            const int pyh = toPxY(draw_bounds.yMax(), frame, super);

            Color track_color = color;
            track_color.a = 150;
            const bool is_horizontal
                = tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;
            // One buffer pixel, as before this clip existed: hairlineCss()
            // would read as a thickness change rather than the intended
            // geometry fix.  drawLine composites with blendPixel, so the
            // 150-alpha blend is unchanged too.
            constexpr int kTrackWidth = 1;

            // Walk only the lines inside `draw_bounds`, over odb's own
            // (memoized, sorted) grid vector.  dbTrackGrid offers a
            // const-ref accessor next to the out-parameter one, so neither
            // the copy nor the full scan is needed: metal1 of bp_quad has
            // 18 947 x-tracks, and the copy alone was ~178 KB per layer-tile.
            // drawGCellGridLayer keeps a cache of its own only because
            // dbGCellGrid lacks this accessor.
            const auto draw_clipped = [&](const std::vector<int>& lines,
                                          const int lo,
                                          const int hi,
                                          const auto& draw_one) {
              for (auto it = std::ranges::lower_bound(lines, lo);
                   it != lines.end() && *it <= hi;
                   ++it) {
                draw_one(*it);
              }
            };

            // X-direction tracks (vertical lines on screen)
            // Preferred for vertical layers, non-preferred for horizontal
            // layers
            if ((!is_horizontal && vis.tracks_pref)
                || (is_horizontal && vis.tracks_non_pref)) {
              const std::vector<int>& x_grid = grid->getGridX();
              debugPrint(logger_,
                         utl::WEB,
                         "tile",
                         1,
                         "  x_tracks: count={} tile=[{},{},{},{}]",
                         x_grid.size(),
                         dbu_tile.xMin(),
                         dbu_tile.yMin(),
                         dbu_tile.xMax(),
                         dbu_tile.yMax());
              draw_clipped(
                  x_grid, draw_bounds.xMin(), draw_bounds.xMax(), [&](int tx) {
                    const int px = toPxX(tx, frame);
                    drawLine(image_buffer,
                             px,
                             pyl,
                             px,
                             pyh,
                             track_color,
                             kTrackWidth);
                  });
            }

            // Y-direction tracks (horizontal lines on screen)
            // Preferred for horizontal layers, non-preferred for vertical
            // layers
            if ((is_horizontal && vis.tracks_pref)
                || (!is_horizontal && vis.tracks_non_pref)) {
              const std::vector<int>& y_grid = grid->getGridY();
              debugPrint(logger_,
                         utl::WEB,
                         "tile",
                         1,
                         "  y_tracks: count={}",
                         y_grid.size());
              draw_clipped(
                  y_grid, draw_bounds.yMin(), draw_bounds.yMax(), [&](int ty) {
                    const int py = toPxY(ty, frame, super);
                    drawLine(image_buffer,
                             pxl,
                             py,
                             pxh,
                             py,
                             track_color,
                             kTrackWidth);
                  });
            }
          }
        }

      }  // end if (!pseudo_layer)

      if (use_local) {
        // Slow-path compositing for chiplets with non-R0 orientations.
        // Forward-mapping (iterate the local buffer, write to world)
        // leaves gaps when the rotation is non-identity because some
        // world pixels never get a source.  We do reverse-mapping
        // instead: for each world destination pixel we map back into
        // the local frame, sample the local buffer if present, and
        // use blendPixel() to alpha-composite onto image_buffer.
        odb::dbTransform inv_xfm = node.world_xfm;
        inv_xfm.invert();
        for (int py_w = 0; py_w < super; ++py_w) {
          for (int px_w = 0; px_w < super; ++px_w) {
            // World pixel center → world DBU.
            odb::Point pt(std::lround(dbu_x_min_world + (px_w + 0.5) / scale),
                          std::lround(dbu_y_min_world
                                      + (super - 1 - py_w + 0.5) / scale));
            // World DBU → local DBU.
            inv_xfm.apply(pt);
            // Local DBU → local pixel.
            const int px_l = std::floor((pt.x() - dbu_x_min) * scale);
            const int py_l
                = super - 1 - std::floor((pt.y() - dbu_y_min) * scale);
            if (px_l < 0 || px_l >= super || py_l < 0 || py_l >= super) {
              continue;
            }
            const int src_idx = (py_l * super + px_l) * 4;
            const unsigned char a_src = local_image_buffer[src_idx + 3];
            if (a_src == 0) {
              continue;
            }
            const Color src_color{
                .r = local_image_buffer[src_idx + 0],
                .g = local_image_buffer[src_idx + 1],
                .b = local_image_buffer[src_idx + 2],
                .a = a_src,
            };
            blendPixel(super_buffer, px_w, py_w, src_color);
          }
        }
      }
    }  // end per-chiplet for-loop

    // Band-limit: Lanczos-2 decimate the supersampled fills into the output
    // tile.  This is the anti-moiré step — prefiltering the dense periodic
    // geometry so no beat survives at the output (physical) pixel grid.
    // Empty tiles (common while panning) skip the decimation entirely:
    // world_image_buffer already holds a transparent tile_px buffer, and a
    // transparent super buffer cannot alias.  any_of early-exits as soon as it
    // hits drawn content, so non-empty tiles pay almost nothing for the check.
    const bool any_drawn = std::ranges::any_of(
        super_buffer, [](const unsigned char b) { return b != 0; });
    if (any_drawn) {
      world_image_buffer = lanczos2Downsample(super_buffer, super, tile_px);
    }

    // Overlays draw at the OUTPUT resolution (crisp lines/text, not band-
    // limited), so they map DBU to pixels with the output-space scale.
    // `scale` is super-space (super px per DBU); the output buffer is tile_px
    // = super / kCoverageSupersample on a side.  Dividing by super_per_css
    // (= dpr * kCoverageSupersample) instead would land in CSS space and
    // shrink every overlay to 1/dpr of the tile on HiDPI.
    const double scale_out = scale / kCoverageSupersample;
    // Same tile, same exact origin, output-resolution scale — and one physical
    // pixel per CSS pixel per dpr, rather than the supersampled frame's.
    TileFrame out_frame = world_frame;
    out_frame.scale = scale_out;
    out_frame.px_per_css = effective_dpr;

    // Overlays render once in world space, on top of all chiplets.
    // Their geometry (timing paths, DRC rects, flight lines) is already
    // expressed in world DBU and isn't tied to any single chiplet's
    // local frame.  route_guides keys on the top-chip tech layer; in
    // multi-tech (3DBlox) designs there is no single top tech, so we
    // search every tech for the requested layer name and use that tech's
    // color map.
    Color world_color{.r = 200, .g = 200, .b = 200, .a = 180};
    bool world_layer_found = false;
    for (odb::dbTech* world_tech : db_->getTechs()) {
      odb::dbTechLayer* world_tech_layer = world_tech->findLayer(layer.c_str());
      if (!world_tech_layer) {
        continue;
      }
      world_layer_found = true;
      const auto& world_layer_colors = getLayerColorMap(world_tech);
      const auto it = world_layer_colors.find(world_tech_layer);
      if (it != world_layer_colors.end()) {
        world_color = it->second;
      }
      break;
    }
    if (!highlight_rects.empty() || !highlight_polys.empty()) {
      drawHighlight(
          world_image_buffer, highlight_rects, highlight_polys, out_frame);
    }
    if (!colored_rects.empty()) {
      drawColoredHighlight(world_image_buffer, colored_rects, layer, out_frame);
    }
    if (!flight_lines.empty()) {
      drawFlightLines(world_image_buffer, flight_lines, out_frame);
    }
    if (route_guide_net_ids && !route_guide_net_ids->empty()
        && world_layer_found) {
      drawRouteGuides(world_image_buffer,
                      *route_guide_net_ids,
                      layer,
                      world_color,
                      out_frame);
    }
    if (vis.debug_renderers) {
      // The callback (installed by WebServer at startup) decides
      // whether to draw (honoring pause/live semantics) and handles
      // the gui::Gui::get() access itself.  Keeping Gui:: references
      // out of tile_generator means test executables that link libweb
      // don't transitively need gui.a / ord.a.
      drawRendererOverlay(world_image_buffer, out_frame, vis.debug_live);
    }
  }

  if (vis.debug) {
    drawDebugOverlay(world_image_buffer, z, x, y);
  }

  return world_image_buffer;
}

std::shared_ptr<gui::HeatMapDataSource> TileGenerator::getHeatMapSource(
    const std::string& name) const
{
  const std::lock_guard<std::mutex> lock(heatmap_mutex_);
  const auto it = heatmaps_.find(name);
  if (it != heatmaps_.end()) {
    return it->second;
  }
  const auto reg = gui::findRegisteredHeatMapSource(name);
  if (!reg) {
    return nullptr;
  }
  auto ptr = reg->createInstance();
  if (ptr) {
    ptr->setChip(db_->getChip());
  }
  heatmaps_[name] = ptr;
  return ptr;
}

void TileGenerator::drawHeatMap(std::vector<unsigned char>& image_buffer,
                                gui::HeatMapDataSource& source,
                                const TileFrame& frame) const
{
  const odb::Rect& dbu_tile = frame.cull;
  const int dim = bufferDim(image_buffer);
  const double scale = frame.scale;
  constexpr double kTextRectMargin = 0.8;
  // Authored in CSS px, so it scales with the display like every other
  // pixel-specified size; a fixed 14 would shrink as the ratio rises.
  constexpr int kHeatmapFontHeightCss = 14;
  const int heatmap_font_px = std::max(
      1,
      static_cast<int>(std::lround(kHeatmapFontHeightCss * frame.px_per_css)));
  const auto heatmap_font = fontAtlasGetFont(heatmap_font_px);
  const Color text_color{.r = 255, .g = 255, .b = 255, .a = 255};

  for (const auto& map_point : source.getVisibleMap(dbu_tile, scale)) {
    if (!map_point.rect.overlaps(dbu_tile)) {
      continue;
    }
    const odb::Rect overlap = map_point.rect.intersect(dbu_tile);
    const odb::Rect draw = toPixels(frame, overlap);
    const Color color{.r = static_cast<uint8_t>(map_point.color.r),
                      .g = static_cast<uint8_t>(map_point.color.g),
                      .b = static_cast<uint8_t>(map_point.color.b),
                      .a = static_cast<uint8_t>(map_point.color.a)};

    for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        blendPixel(image_buffer, ix, dim - 1 - iy, color, dim);
      }
    }

    if (!source.getShowNumbers() || !map_point.has_value) {
      continue;
    }

    const std::string text = source.formatValue(map_point.value, false);
    const int text_width = getTextWidth(text, heatmap_font);
    const int text_height = getTextHeight(heatmap_font);
    const double rect_width = map_point.rect.dx() * scale;
    const double rect_height = map_point.rect.dy() * scale;
    if (text_width >= kTextRectMargin * rect_width
        || text_height >= kTextRectMargin * rect_height) {
      continue;
    }

    const double center_x
        = 0.5 * (map_point.rect.xMin() + map_point.rect.xMax());
    const double center_y
        = 0.5 * (map_point.rect.yMin() + map_point.rect.yMax());

    const int pixel_x = std::lround(frame.pxX(center_x));
    const int pixel_y = dim - 1 - std::lround(frame.pxY(center_y));

    // The label is centered on the bin center and may straddle the boundary
    // between adjacent tiles.  Skipping when the center falls outside this tile
    // would drop the half of the text that spills into the neighbor (e.g.
    // "29.89" rendering as ".89").  Instead skip only when the whole text box
    // is off-tile; each tile draws its slice and drawText/blendPixel clip
    // per-pixel, so the slices join seamlessly across the seam.  Note the box
    // spans the half-open range [min, max), so max <= 0 is fully off-tile.
    const int text_px_min = pixel_x - text_width / 2;
    const int text_px_max = text_px_min + text_width;
    const int text_py_min = pixel_y - text_height / 2;
    const int text_py_max = text_py_min + text_height;

    if (text_px_max <= 0 || text_px_min >= dim || text_py_max <= 0
        || text_py_min >= dim) {
      continue;
    }

    drawText(
        image_buffer, text_px_min, text_py_min, text, heatmap_font, text_color);
  }
}

std::vector<unsigned char> TileGenerator::generateHeatMapTile(
    gui::HeatMapDataSource& source,
    const int z,
    const int x,
    int y,
    const double dpr,
    const int requested_tile_px) const
{
  // Same contract as the layer and overlay tiles: the client states the
  // device-pixel square, so the heat map is as crisp as the layers under it
  // instead of being a 256 px image stretched over them.  0 falls back to the
  // historical 256*dpr.
  const double effective_dpr = dpr > 0.0 ? dpr : 1.0;
  const int dim
      = requested_tile_px > 0
            ? requested_tile_px
            : static_cast<int>(std::lround(kTileSizeInPixel * effective_dpr));
  std::vector<unsigned char> image_buffer(static_cast<size_t>(dim) * dim * 4,
                                          0);

  const double num_tiles_at_zoom = pow(2, z);
  if (x < 0 || y < 0 || x >= num_tiles_at_zoom || y >= num_tiles_at_zoom) {
    return {};
  }

  y = num_tiles_at_zoom - 1 - y;
  const odb::Rect hm_bounds = getBounds();
  const double tile_dbu_size = hm_bounds.maxDXDY() / num_tiles_at_zoom;
  const double scale = dim / tile_dbu_size;
  // Same exact grid as the layer tiles this heat map is drawn over.
  const TileFrame frame
      = tileFrame(hm_bounds, x, y, tile_dbu_size, scale, effective_dpr);

  drawHeatMap(image_buffer, source, frame);

  std::vector<unsigned char> png_data;
  const unsigned error = lodepng::encode(png_data, image_buffer, dim, dim);
  if (error) {
    logger_->report("PNG encoder error: {}", lodepng_error_text(error));
  }
  return png_data;
}

// Alpha-composite src onto dst (Porter-Duff "over").
static void compositePixel(unsigned char* dst, const unsigned char* src)
{
  const int sa = src[3];
  if (sa == 0) {
    return;
  }
  if (sa == 255 || dst[3] == 0) {
    std::memcpy(dst, src, 4);
    return;
  }
  const int da = dst[3];
  const int out_a = sa + da * (255 - sa) / 255;
  if (out_a == 0) {
    return;
  }
  for (int c = 0; c < 3; ++c) {
    dst[c] = (src[c] * sa + dst[c] * da * (255 - sa) / 255) / out_a;
  }
  dst[3] = out_a;
}

// ---------------------------------------------------------------------------
// Band-limited decimation (anti-moiré).
//
// Dense periodic geometry (bump/UBM arrays) rasterized with ~1px coverage
// aliases: a near-Nyquist fundamental beats into a low-frequency moiré that no
// post-blur can remove.  The fix is to prefilter DURING sampling: rasterize the
// whole tile at a supersample factor S and decimate back to the output grid
// with a separable Lanczos-2 kernel whose cutoff sits at the *output* Nyquist.
// Validated by DSP: at output pitch {0.5,1,2}px the beat drops ~10x while a
// resolved grid (pitch >=4px) keeps its contrast.
// ---------------------------------------------------------------------------

static constexpr double kLanczosPi = std::numbers::pi;

// Lanczos-2 windowed-sinc: L(t) = sinc(t) * sinc(t/2) for |t| < 2, else 0.
static double lanczos2Kernel(const double t)
{
  if (t == 0.0) {
    return 1.0;
  }
  if (t <= -2.0 || t >= 2.0) {
    return 0.0;
  }
  const double pt = kLanczosPi * t;
  return (std::sin(pt) / pt) * (std::sin(pt / 2.0) / (pt / 2.0));
}

// For integer decimation S = src_dim/dst_dim, precompute per-output-pixel taps
// (clamped source index + normalized weight).  The kernel argument is scaled by
// 1/S so the cutoff is the destination Nyquist (this is what suppresses the
// beat).  The Lanczos weights are additionally convolved with a binomial
// prefilter (kLanczosPrefilterBinomial) in source space, which puts an exact
// zero at the source Nyquist and deepens the soft Lanczos stopband so a dense
// periodic array near the output Nyquist is nulled rather than leaked.  Edge
// pixels clamp the source index and renormalize by the weight sum, so uniform
// regions and the tile border stay flat (DC-preserving → no seam, no sheet).
static std::vector<std::vector<std::pair<int, float>>> buildLanczos2Taps(
    const int src_dim,
    const int dst_dim)
{
  const double s = static_cast<double>(src_dim) / dst_dim;
  const double radius = 2.0 * s;  // support, in source samples
  std::vector<std::vector<std::pair<int, float>>> taps(dst_dim);
  for (int o = 0; o < dst_dim; ++o) {
    const double c = (o + 0.5) * s - 0.5;  // source-sample center
    const int i0 = static_cast<int>(std::ceil(c - radius));
    const int i1 = static_cast<int>(std::floor(c + radius));
    // Accumulate the Lanczos weight of each source sample, spread by the
    // binomial prefilter onto its neighbours; clamp-to-edge and merge by
    // clamped index (a std::map keeps the indices sorted and deduplicated).
    std::map<int, double> acc;
    for (int i = i0; i <= i1; ++i) {
      const double w = lanczos2Kernel((i - c) / s);
      if (w == 0.0) {
        continue;
      }
      for (int k = -1; k <= 1; ++k) {
        const int idx = std::clamp(i + k, 0, src_dim - 1);
        acc[idx] += w * kLanczosPrefilterBinomial[k + 1];
      }
    }
    double wsum = 0.0;
    for (const auto& [idx, w] : acc) {
      wsum += w;
    }
    std::vector<std::pair<int, float>>& row = taps[o];
    row.reserve(acc.size());
    for (const auto& [idx, w] : acc) {
      row.emplace_back(idx, static_cast<float>(wsum != 0.0 ? w / wsum : w));
    }
  }
  return taps;
}

// Memoized Lanczos-2 taps.  (src_dim, dst_dim) depend only on dpr — a few
// distinct values per session — so the taps (and their sin/cos) are built once
// and reused across every tile.  Entries are never erased, so the returned
// reference stays valid after the lock is released.
static const std::vector<std::vector<std::pair<int, float>>>& getLanczos2Taps(
    const int src_dim,
    const int dst_dim)
{
  static std::mutex mu;
  static std::map<std::pair<int, int>,
                  std::vector<std::vector<std::pair<int, float>>>>
      cache;
  const std::lock_guard<std::mutex> lock(mu);
  const std::pair<int, int> key{src_dim, dst_dim};
  auto it = cache.find(key);
  if (it == cache.end()) {
    it = cache.emplace(key, buildLanczos2Taps(src_dim, dst_dim)).first;
  }
  return it->second;
}

// Separable Lanczos-2 downsample of a straight-alpha RGBA buffer from src_dim^2
// to dst_dim^2.  Alpha is premultiplied before convolution and un-premultiplied
// after (a straight-alpha convolution dark-fringes coverage edges).  src and
// dst are square (tiles are square).
static std::vector<unsigned char> lanczos2Downsample(
    const std::vector<unsigned char>& src,
    const int src_dim,
    const int dst_dim)
{
  std::vector<unsigned char> dst(static_cast<size_t>(dst_dim) * dst_dim * 4, 0);
  if (dst_dim <= 0) {
    return dst;
  }

  const std::vector<std::vector<std::pair<int, float>>>& taps
      = getLanczos2Taps(src_dim, dst_dim);

  // Horizontal pass: premultiply + convolve along X into a float intermediate
  // indexed [src_row][dst_col][channel].  Reused across calls on this thread;
  // every element is overwritten below, so no re-zeroing is needed.
  static thread_local std::vector<float> inter;
  inter.resize(static_cast<size_t>(src_dim) * dst_dim * 4);
  for (int sy = 0; sy < src_dim; ++sy) {
    const unsigned char* srow = &src[static_cast<size_t>(sy) * src_dim * 4];
    for (int ox = 0; ox < dst_dim; ++ox) {
      float r = 0, g = 0, b = 0, a = 0;
      for (const auto& [sx, w] : taps[ox]) {
        const unsigned char* p = &srow[static_cast<size_t>(sx) * 4];
        const float pa = p[3];
        r += w * (p[0] * pa / 255.0f);
        g += w * (p[1] * pa / 255.0f);
        b += w * (p[2] * pa / 255.0f);
        a += w * pa;
      }
      float* out = &inter[(static_cast<size_t>(sy) * dst_dim + ox) * 4];
      out[0] = r;
      out[1] = g;
      out[2] = b;
      out[3] = a;
    }
  }

  // Vertical pass: convolve along Y, un-premultiply, clamp (Lanczos
  // overshoots).
  for (int oy = 0; oy < dst_dim; ++oy) {
    for (int ox = 0; ox < dst_dim; ++ox) {
      float r = 0, g = 0, b = 0, a = 0;
      for (const auto& [sy, w] : taps[oy]) {
        const float* p = &inter[(static_cast<size_t>(sy) * dst_dim + ox) * 4];
        r += w * p[0];
        g += w * p[1];
        b += w * p[2];
        a += w * p[3];
      }
      unsigned char* d = &dst[(static_cast<size_t>(oy) * dst_dim + ox) * 4];
      const float ai = std::clamp(a, 0.0f, 255.0f);
      if (ai <= 0.0f) {
        d[0] = d[1] = d[2] = d[3] = 0;
        continue;
      }
      const auto unpremult = [&](const float ch) {
        return static_cast<unsigned char>(
            std::lround(std::clamp(ch * 255.0f / ai, 0.0f, 255.0f)));
      };
      d[0] = unpremult(r);
      d[1] = unpremult(g);
      d[2] = unpremult(b);
      d[3] = static_cast<unsigned char>(std::lround(ai));
    }
  }
  return dst;
}

std::vector<unsigned char> TileGenerator::renderImageBuffer(
    const odb::Rect& region,
    const int width_px,
    const double dbu_per_pixel,
    const TileVisibility& vis,
    const Color& bg,
    int* out_width,
    int* out_height) const
{
  odb::dbBlock* block = getBlock();
  if (!block) {
    logger_->error(utl::WEB, 20, "No design loaded.");
    return {};
  }

  // Determine rendering region (DBU).
  odb::Rect area = region;
  if (area.dx() == 0 || area.dy() == 0) {
    area = block->getDieArea();
    if (area.dx() == 0 || area.dy() == 0) {
      area = block->getBBox()->getBox();
    }
    // Bloat by 5% like GUI headless default.
    const int margin_x = area.dx() * 5 / 100;
    const int margin_y = area.dy() * 5 / 100;
    area.bloat(std::max(margin_x, margin_y), area);
  }

  // Determine scale (pixels per DBU).
  double scale = 0;
  if (width_px > 0) {
    scale = static_cast<double>(width_px) / area.dx();
  } else if (dbu_per_pixel > 0) {
    scale = 1.0 / dbu_per_pixel;
  } else {
    // Default: 1024px wide.
    scale = 1024.0 / area.dx();
  }

  const int img_w = static_cast<int>(std::ceil(area.dx() * scale));
  const int img_h = static_cast<int>(std::ceil(area.dy() * scale));

  if (img_w <= 0 || img_h <= 0) {
    logger_->error(utl::WEB, 21, "Invalid image dimensions.");
    return {};
  }

  // Cap image size at 16k x 16k to prevent excessive memory usage.
  constexpr int kMaxDim = 16384;
  if (img_w > kMaxDim || img_h > kMaxDim) {
    logger_->warn(utl::WEB,
                  22,
                  "Image dimensions {}x{} exceed max {}; clamping.",
                  img_w,
                  img_h,
                  kMaxDim);
    scale = std::min(static_cast<double>(kMaxDim) / area.dx(),
                     static_cast<double>(kMaxDim) / area.dy());
  }

  const int final_w = static_cast<int>(std::ceil(area.dx() * scale));
  const int final_h = static_cast<int>(std::ceil(area.dy() * scale));

  // Compute zoom level that gives tile_scale close to our target scale.
  // tile_scale = kTileSizeInPixel / (maxDXDY / 2^z)
  // We want tile_scale >= scale, so z = ceil(log2(scale * maxDXDY / 256)).
  const odb::Rect bounds = getBounds();
  const double max_dxdy = bounds.maxDXDY();
  const int z = std::max(0,
                         static_cast<int>(std::ceil(
                             std::log2(scale * max_dxdy / kTileSizeInPixel))));
  const int num_tiles = static_cast<int>(std::pow(2, z));
  const double tile_dbu_size = max_dxdy / num_tiles;
  const double tile_scale = kTileSizeInPixel / tile_dbu_size;

  // Determine which tiles overlap our area.
  const int tx_min = std::max(
      0, static_cast<int>((area.xMin() - bounds.xMin()) / tile_dbu_size));
  const int ty_min = std::max(
      0, static_cast<int>((area.yMin() - bounds.yMin()) / tile_dbu_size));
  const int tx_max
      = std::min(num_tiles - 1,
                 static_cast<int>(
                     std::ceil((area.xMax() - bounds.xMin()) / tile_dbu_size)));
  const int ty_max
      = std::min(num_tiles - 1,
                 static_cast<int>(
                     std::ceil((area.yMax() - bounds.yMin()) / tile_dbu_size)));

  // Allocate output buffer (RGBA).
  const int tile_span_w = (tx_max - tx_min + 1) * kTileSizeInPixel;
  const int tile_span_h = (ty_max - ty_min + 1) * kTileSizeInPixel;
  std::vector<unsigned char> output(4UL * tile_span_w * tile_span_h, 0);

  const std::vector<std::string> layers_to_render
      = saveImageLayerOrder(vis, getLayers());

  // Snapshot the user labels once (locks labels_mutex_ + copies) instead of
  // per tile — the set is identical for every tile in the image.  Skipped
  // when Labels is off, so a saved image can reproduce a view with them
  // hidden; Qt gates its own drawLabels the same way.
  const std::vector<TextLabel> labels
      = vis.labels ? labelsForDraw() : std::vector<TextLabel>{};

  // Render each tile, compositing all layers.
  for (int ty = ty_min; ty <= ty_max; ++ty) {
    for (int tx = tx_min; tx <= tx_max; ++tx) {
      // Tile position in the output buffer.
      const int out_ox = (tx - tx_min) * kTileSizeInPixel;
      // Y is flipped: tile_y in generateTile is bottom-up, output is top-down.
      const int out_oy = (ty_max - ty) * kTileSizeInPixel;

      // Leaflet-style y coordinate (before the flip in renderTileBuffer).
      const int leaflet_y = num_tiles - 1 - ty;

      for (const auto& layer : layers_to_render) {
        auto tile_buf = renderTileBuffer(layer, z, tx, leaflet_y, vis);

        // Composite tile onto output at (out_ox, out_oy).
        for (int py = 0; py < kTileSizeInPixel; ++py) {
          for (int px = 0; px < kTileSizeInPixel; ++px) {
            const int src_idx = (py * kTileSizeInPixel + px) * 4;
            const int dst_x = out_ox + px;
            const int dst_y = out_oy + py;
            if (dst_x >= tile_span_w || dst_y >= tile_span_h) {
              continue;
            }
            const int dst_idx = (dst_y * tile_span_w + dst_x) * 4;
            compositePixel(&output[dst_idx], &tile_buf[src_idx]);
          }
        }
      }

      // Composite user text labels on top of all layers (Qt-parity: labels
      // appear in save_image).
      auto label_buf = labels.empty()
                           ? std::vector<unsigned char>()
                           : renderLabelTile(z, tx, leaflet_y, labels);
      if (!label_buf.empty()) {
        for (int py = 0; py < kTileSizeInPixel; ++py) {
          for (int px = 0; px < kTileSizeInPixel; ++px) {
            const int src_idx = (py * kTileSizeInPixel + px) * 4;
            const int dst_x = out_ox + px;
            const int dst_y = out_oy + py;
            if (dst_x >= tile_span_w || dst_y >= tile_span_h) {
              continue;
            }
            const int dst_idx = (dst_y * tile_span_w + dst_x) * 4;
            compositePixel(&output[dst_idx], &label_buf[src_idx]);
          }
        }
      }
    }
  }

  // Crop to the exact requested area.
  // The tile span covers a larger region; compute the pixel offset of the
  // area's origin within the tile span.
  const int crop_x = static_cast<int>(
      (area.xMin() - bounds.xMin() - tx_min * tile_dbu_size) * tile_scale);
  const int crop_y_bottom = static_cast<int>(
      (area.yMin() - bounds.yMin() - ty_min * tile_dbu_size) * tile_scale);
  // In the output buffer, y=0 is the top (ty_max), and area.yMin maps
  // to the bottom.  The crop origin in output coords:
  const int crop_y
      = tile_span_h - crop_y_bottom - static_cast<int>(area.dy() * tile_scale);

  // Resample to exact requested dimensions (nearest-neighbor from tile_scale
  // to target scale).  Start from the requested background color and composite
  // the (possibly semi-transparent) tiles on top, so the saved image matches
  // the viewer's background instead of coming out transparent.
  std::vector<unsigned char> final_buf(4UL * final_w * final_h);
  for (int fy = 0; fy < final_h; ++fy) {
    for (int fx = 0; fx < final_w; ++fx) {
      const int dst_idx = (fy * final_w + fx) * 4;
      // Start each pixel at the background, then composite the tile on top.
      final_buf[dst_idx + 0] = bg.r;
      final_buf[dst_idx + 1] = bg.g;
      final_buf[dst_idx + 2] = bg.b;
      final_buf[dst_idx + 3] = bg.a;
      // Map final pixel to tile-span pixel.
      const int sx = crop_x + static_cast<int>(fx * tile_scale / scale);
      const int sy = crop_y + static_cast<int>(fy * tile_scale / scale);
      if (sx >= 0 && sx < tile_span_w && sy >= 0 && sy < tile_span_h) {
        const int src_idx = (sy * tile_span_w + sx) * 4;
        compositePixel(&final_buf[dst_idx], &output[src_idx]);
      }
    }
  }

  if (out_width) {
    *out_width = final_w;
  }
  if (out_height) {
    *out_height = final_h;
  }
  return final_buf;
}

std::vector<unsigned char> TileGenerator::renderImagePng(
    const odb::Rect& region,
    const int width_px,
    const double dbu_per_pixel,
    const TileVisibility& vis,
    const Color& bg,
    int* out_width,
    int* out_height) const
{
  int final_w = 0;
  int final_h = 0;
  const std::vector<unsigned char> final_buf = renderImageBuffer(
      region, width_px, dbu_per_pixel, vis, bg, &final_w, &final_h);
  if (final_buf.empty()) {
    return {};  // renderImageBuffer already logged the error.
  }

  // Encode to PNG.
  std::vector<unsigned char> png_data;
  const unsigned error = lodepng::encode(png_data, final_buf, final_w, final_h);
  if (error) {
    logger_->error(
        utl::WEB, 23, "PNG encode error: {}", lodepng_error_text(error));
    return {};
  }
  if (out_width) {
    *out_width = final_w;
  }
  if (out_height) {
    *out_height = final_h;
  }
  return png_data;
}

void TileGenerator::saveImage(const std::string& filename,
                              const odb::Rect& region,
                              const int width_px,
                              const double dbu_per_pixel,
                              const TileVisibility& vis) const
{
  int final_w = 0;
  int final_h = 0;
  const std::vector<unsigned char> png_data = renderImagePng(
      region, width_px, dbu_per_pixel, vis, /*bg=*/{}, &final_w, &final_h);
  if (png_data.empty()) {
    return;
  }
  lodepng::save_file(png_data, filename);
  logger_->info(
      utl::WEB, 24, "Saved {}x{} image to {}", final_w, final_h, filename);
}

std::vector<unsigned char> TileGenerator::renderOverlayPng(
    int width_px,
    const std::vector<ColoredRect>& rects,
    const std::vector<FlightLine>& lines) const
{
  odb::dbBlock* block = getBlock();
  if (!block || (rects.empty() && lines.empty())) {
    return {};
  }

  // Same area computation as renderLayerPng.
  odb::Rect area = block->getDieArea();
  if (area.dx() == 0 || area.dy() == 0) {
    area = block->getBBox()->getBox();
  }
  const int margin = area.maxDXDY() * 5 / 100;
  area.bloat(margin, area);

  if (width_px <= 0) {
    width_px = 1024;
  }
  const double scale = static_cast<double>(width_px) / area.dx();
  const int final_w = static_cast<int>(std::ceil(area.dx() * scale));
  const int final_h = static_cast<int>(std::ceil(area.dy() * scale));
  if (final_w <= 0 || final_h <= 0) {
    return {};
  }

  const odb::Rect bounds = getBounds();
  const double max_dxdy = bounds.maxDXDY();
  const int z = std::max(0,
                         static_cast<int>(std::ceil(
                             std::log2(scale * max_dxdy / kTileSizeInPixel))));
  const int num_tiles = static_cast<int>(std::pow(2, z));
  const double tile_dbu_size = max_dxdy / num_tiles;
  const double tile_scale = kTileSizeInPixel / tile_dbu_size;

  const int tx_min = std::max(
      0, static_cast<int>((area.xMin() - bounds.xMin()) / tile_dbu_size));
  const int ty_min = std::max(
      0, static_cast<int>((area.yMin() - bounds.yMin()) / tile_dbu_size));
  const int tx_max
      = std::min(num_tiles - 1,
                 static_cast<int>(
                     std::ceil((area.xMax() - bounds.xMin()) / tile_dbu_size)));
  const int ty_max
      = std::min(num_tiles - 1,
                 static_cast<int>(
                     std::ceil((area.yMax() - bounds.yMin()) / tile_dbu_size)));

  const int tile_span_w = (tx_max - tx_min + 1) * kTileSizeInPixel;
  const int tile_span_h = (ty_max - ty_min + 1) * kTileSizeInPixel;
  std::vector<unsigned char> output(4UL * tile_span_w * tile_span_h, 0);

  // Render on _instances layer with all visibility off so only overlays draw.
  TileVisibility vis;
  vis.stdcells = false;
  vis.macros = false;
  vis.pad_input = false;
  vis.pad_output = false;
  vis.pad_inout = false;
  vis.pad_power = false;
  vis.pad_spacer = false;
  vis.pad_areaio = false;
  vis.pad_other = false;
  vis.phys_fill = false;
  vis.phys_endcap = false;
  vis.phys_welltap = false;
  vis.phys_tie = false;
  vis.phys_antenna = false;
  vis.phys_cover = false;
  vis.phys_bump = false;
  vis.phys_other = false;
  vis.std_bufinv = false;
  vis.std_bufinv_timing = false;
  vis.std_clock_bufinv = false;
  vis.std_clock_gate = false;
  vis.std_level_shift = false;
  vis.std_sequential = false;
  vis.std_combinational = false;
  vis.routing = false;
  vis.routing_segments = false;
  vis.routing_vias = false;
  vis.special_nets = false;
  vis.srouting_segments = false;
  vis.srouting_vias = false;
  vis.pins = false;
  vis.pin_names = false;
  vis.inst_names = false;
  vis.inst_pins = false;
  vis.inst_pin_names = false;
  vis.blockages = false;
  vis.placement_blockages = false;
  vis.routing_obstructions = false;

  for (int ty = ty_min; ty <= ty_max; ++ty) {
    for (int tx = tx_min; tx <= tx_max; ++tx) {
      const int out_ox = (tx - tx_min) * kTileSizeInPixel;
      const int out_oy = (ty_max - ty) * kTileSizeInPixel;
      const int leaflet_y = num_tiles - 1 - ty;

      auto tile_buf = renderTileBuffer(
          "_instances", z, tx, leaflet_y, vis, {}, {}, rects, lines);

      for (int py = 0; py < kTileSizeInPixel; ++py) {
        for (int px = 0; px < kTileSizeInPixel; ++px) {
          const int src_idx = (py * kTileSizeInPixel + px) * 4;
          const int dst_x = out_ox + px;
          const int dst_y = out_oy + py;
          if (dst_x >= tile_span_w || dst_y >= tile_span_h) {
            continue;
          }
          const int dst_idx = (dst_y * tile_span_w + dst_x) * 4;
          compositePixel(&output[dst_idx], &tile_buf[src_idx]);
        }
      }
    }
  }

  // Crop and resample.
  const int crop_x = static_cast<int>(
      (area.xMin() - bounds.xMin() - tx_min * tile_dbu_size) * tile_scale);
  const int crop_y_bottom = static_cast<int>(
      (area.yMin() - bounds.yMin() - ty_min * tile_dbu_size) * tile_scale);
  const int crop_y
      = tile_span_h - crop_y_bottom - static_cast<int>(area.dy() * tile_scale);

  std::vector<unsigned char> final_buf(4UL * final_w * final_h, 0);
  for (int fy = 0; fy < final_h; ++fy) {
    for (int fx = 0; fx < final_w; ++fx) {
      const int sx = crop_x + static_cast<int>(fx * tile_scale / scale);
      const int sy = crop_y + static_cast<int>(fy * tile_scale / scale);
      if (sx >= 0 && sx < tile_span_w && sy >= 0 && sy < tile_span_h) {
        const int src_idx = (sy * tile_span_w + sx) * 4;
        const int dst_idx = (fy * final_w + fx) * 4;
        std::memcpy(&final_buf[dst_idx], &output[src_idx], 4);
      }
    }
  }

  std::vector<unsigned char> png_data;
  lodepng::encode(png_data, final_buf, final_w, final_h);
  return png_data;
}

void TileGenerator::drawDebugOverlay(std::vector<unsigned char>& image,
                                     const int z,
                                     const int x,
                                     const int y) const
{
  const Color yellow{.r = 255, .g = 255, .b = 0, .a = 255};
  // The output buffer is tile_px = 256*dpr on a side, NOT kTileSizeInPixel.
  // Recover the real dimension: hardcoding 256 boxed the whole overlay into
  // the top-left 256x256 corner of a HiDPI tile, so the "tile" outline drew
  // at 1/dpr of the tile it was supposed to trace.
  const int dim = bufferDim(image);
  const int last = dim - 1;
  // Pixel-authored sizes (border inset, font height) are in CSS px; scale them
  // to physical px so the overlay looks identical across dpr.
  const double px_per_css = static_cast<double>(dim) / kTileSizeInPixel;

  // Draw 1-pixel yellow border
  for (int i = 0; i < dim; ++i) {
    setPixel(image, i, 0, yellow, dim);
    setPixel(image, i, last, yellow, dim);
    setPixel(image, 0, i, yellow, dim);
    setPixel(image, last, i, yellow, dim);
  }

  // Build the label string "z=<zoom> <x>/<y>"
  const std::string label = "z=" + std::to_string(z) + " " + std::to_string(x)
                            + "/" + std::to_string(y);

  const int margin = static_cast<int>(std::lround(4 * px_per_css));
  // Floor at 10px, matching rasterizeWebPainterOps: a dpr < 0.5 (browser zoom
  // out) would otherwise round the height toward 0 and hand fontAtlasGetFont a
  // degenerate size.
  const int font_px
      = std::max(10, static_cast<int>(std::lround(20 * px_per_css)));
  drawText(image, margin, margin, label, fontAtlasGetFont(font_px), yellow);
}

namespace {

// Process-wide debug-overlay callback installed by WebServer at serve()
// time.  Nullable; when not set, drawRendererOverlay is a no-op.  This
// indirection keeps gui::Gui::get() out of tile_generator.cpp so that
// libweb.a has no undefined references to the full gui/SWIG library —
// test binaries can link libweb without pulling in ord::OpenRoad::openRoad.
TileGenerator::DebugOverlayCallback& getDebugOverlayCallback()
{
  static TileGenerator::DebugOverlayCallback callback;
  return callback;
}

// Convert a gui::Painter::Color to our internal Color (same RGBA layout).
Color toTileColor(const gui::Painter::Color& c)
{
  return Color{
      .r = static_cast<unsigned char>(c.r),
      .g = static_cast<unsigned char>(c.g),
      .b = static_cast<unsigned char>(c.b),
      .a = static_cast<unsigned char>(c.a),
  };
}

}  // namespace

/* static */
void TileGenerator::setDebugOverlayCallback(DebugOverlayCallback callback)
{
  getDebugOverlayCallback() = std::move(callback);
}

void TileGenerator::drawRendererOverlay(std::vector<unsigned char>& image,
                                        const TileFrame& frame,
                                        const bool debug_live) const
{
  auto& callback = getDebugOverlayCallback();
  if (!callback) {
    return;
  }
  callback(image, frame, debug_live);
}

// Convert a PenState width to pixel width for rasterization.
// Cosmetic pens are always 1 screen pixel (matching Qt semantics).
static int penWidthPx(const PenState& pen, double scale)
{
  if (pen.cosmetic) {
    return std::max(1, pen.width);
  }
  return std::max(1, static_cast<int>(pen.width * scale));
}

// Rasterize a single WebPainter's recorded DrawOps into a pixel buffer.
// Exposed so that the WebServer-installed debug-overlay callback can
// reuse tile_generator's line / polygon / bitmap primitives.
void TileGenerator::rasterizeWebPainterOps(std::vector<unsigned char>& image,
                                           const std::vector<DrawOp>& ops,
                                           const TileFrame& frame) const
{
  const double scale = frame.scale;
  {
    // Buffer side length (tile_px = 256*dpr) — every Y flip below is relative
    // to it, not to a fixed 256.
    const int dim = bufferDim(image);
    for (const DrawOp& op : ops) {
      if (const auto* r = std::get_if<DrawRectOp>(&op)) {
        const odb::Rect px = toPixels(frame, r->rect);
        // Fill first (if the brush paints), outline on top.
        if (r->brush.style != gui::Painter::Brush::kNone
            && r->brush.color.a > 0) {
          const Color fill = toTileColor(r->brush.color);
          for (int iy = px.yMin(); iy < px.yMax(); ++iy) {
            for (int ix = px.xMin(); ix < px.xMax(); ++ix) {
              blendPixel(image, ix, dim - 1 - iy, fill, dim);
            }
          }
        }
        if (r->pen.color.a > 0 && px.dx() >= 1 && px.dy() >= 1) {
          const Color pen = toTileColor(r->pen.color);
          const int w = penWidthPx(r->pen, scale);
          const int x0 = px.xMin();
          const int x1 = px.xMax() - 1;
          const int y0 = dim - 1 - px.yMin();
          const int y1 = dim - 1 - (px.yMax() - 1);
          drawLine(image, x0, y0, x1, y0, pen, w);
          drawLine(image, x1, y0, x1, y1, pen, w);
          drawLine(image, x1, y1, x0, y1, pen, w);
          drawLine(image, x0, y1, x0, y0, pen, w);
        }
      } else if (const auto* l = std::get_if<DrawLineOp>(&op)) {
        if (l->pen.color.a == 0) {
          continue;
        }
        // Oblique segment: convert in double so drawLine's clip keeps the
        // slope (see toPxXd).
        drawLine(image,
                 toPxXd(l->p1.x(), frame),
                 toPxYd(l->p1.y(), frame, dim),
                 toPxXd(l->p2.x(), frame),
                 toPxYd(l->p2.y(), frame, dim),
                 toTileColor(l->pen.color),
                 penWidthPx(l->pen, scale));
      } else if (const auto* c = std::get_if<DrawCircleOp>(&op)) {
        // Simple midpoint circle (outline only).
        const int cx = toPxX(c->cx, frame);
        const int cy = toPxY(c->cy, frame, dim);
        const int pr = std::max(1, static_cast<int>(c->r * scale));
        if (c->pen.color.a == 0) {
          continue;
        }
        const Color pen = toTileColor(c->pen.color);
        int dx = pr;
        int dy = 0;
        int err = 1 - dx;
        while (dx >= dy) {
          blendPixel(image, cx + dx, cy + dy, pen);
          blendPixel(image, cx + dy, cy + dx, pen);
          blendPixel(image, cx - dy, cy + dx, pen);
          blendPixel(image, cx - dx, cy + dy, pen);
          blendPixel(image, cx - dx, cy - dy, pen);
          blendPixel(image, cx - dy, cy - dx, pen);
          blendPixel(image, cx + dy, cy - dx, pen);
          blendPixel(image, cx + dx, cy - dy, pen);
          ++dy;
          if (err < 0) {
            err += 2 * dy + 1;
          } else {
            --dx;
            err += 2 * (dy - dx) + 1;
          }
        }
      } else if (const auto* xop = std::get_if<DrawXOp>(&op)) {
        if (xop->pen.color.a == 0) {
          continue;
        }
        const int cx = toPxX(xop->cx, frame);
        const int cy = toPxY(xop->cy, frame, dim);
        const int half = std::max(1, static_cast<int>(xop->size * scale / 2));
        const Color pen = toTileColor(xop->pen.color);
        const int w = penWidthPx(xop->pen, scale);
        drawLine(image, cx - half, cy - half, cx + half, cy + half, pen, w);
        drawLine(image, cx - half, cy + half, cx + half, cy - half, pen, w);
      } else if (const auto* p = std::get_if<DrawPolygonOp>(&op)) {
        if (p->brush.style != gui::Painter::Brush::kNone
            && p->brush.color.a > 0) {
          odb::Polygon poly;
          poly.setPoints(p->points);
          fillPolygon(image,
                      poly,
                      frame,
                      toTileColor(p->brush.color),
                      /*blend=*/true);
        }
        if (p->pen.color.a > 0) {
          const Color pen = toTileColor(p->pen.color);
          const int w = penWidthPx(p->pen, scale);
          const int n = static_cast<int>(p->points.size());
          for (int i = 0; i < n; ++i) {
            const odb::Point& a = p->points[i];
            const odb::Point& b = p->points[(i + 1) % n];
            // Polygon edges are arbitrary segments — same reason as DrawLineOp.
            drawLine(image,
                     toPxXd(a.x(), frame),
                     toPxYd(a.y(), frame, dim),
                     toPxXd(b.x(), frame),
                     toPxYd(b.y(), frame, dim),
                     pen,
                     w);
          }
        }
      } else if (const auto* s = std::get_if<DrawStringOp>(&op)) {
        if (s->pen.color.a == 0 || s->text.empty()) {
          continue;
        }
        const auto str_font = fontAtlasGetFont(std::max(10, s->font.size));
        const int tw = getTextWidth(s->text, str_font);
        const int th = getTextHeight(str_font);
        int ax = toPxX(s->x, frame);
        int ay = toPxY(s->y, frame, dim);
        // Adjust anchor: text renders with top-left at (ax, ay).
        switch (s->anchor) {
          case gui::Painter::kBottomLeft:
            ay -= th;
            break;
          case gui::Painter::kBottomRight:
            ax -= tw;
            ay -= th;
            break;
          case gui::Painter::kTopLeft:
            break;
          case gui::Painter::kTopRight:
            ax -= tw;
            break;
          case gui::Painter::kCenter:
            ax -= tw / 2;
            ay -= th / 2;
            break;
          case gui::Painter::kBottomCenter:
            ax -= tw / 2;
            ay -= th;
            break;
          case gui::Painter::kTopCenter:
            ax -= tw / 2;
            break;
          case gui::Painter::kLeftCenter:
            ay -= th / 2;
            break;
          case gui::Painter::kRightCenter:
            ax -= tw;
            ay -= th / 2;
            break;
        }
        const Color pen = toTileColor(s->pen.color);
        if (s->rotate_90) {
          drawTextRotated(image, ax, ay, s->text, str_font, pen);
        } else {
          drawText(image, ax, ay, s->text, str_font, pen);
        }
      }
    }
  }
}

/* static */
int TileGenerator::getTextWidth(const std::string_view text,
                                const GlyphCache::FontSize& font)
{
  return font.textWidth(text);
}

/* static */
int TileGenerator::getTextHeight(const GlyphCache::FontSize& font)
{
  return font.cellHeight();
}

/* static */
void TileGenerator::drawText(std::vector<unsigned char>& image,
                             const int x,
                             const int y,
                             const std::string_view text,
                             const GlyphCache::FontSize& font,
                             const Color& color)
{
  int cursor_x = x;
  for (size_t i = 0; i < text.size(); ++i) {
    const auto gi = font.glyph(text[i]);
    if (gi.alpha != nullptr) {
      for (int row = 0; row < gi.bmp_height; ++row) {
        for (int col = 0; col < gi.bmp_width; ++col) {
          const unsigned char alpha_val = gi.alpha[row * gi.bmp_width + col];
          if (alpha_val == 0) {
            continue;
          }
          Color src = color;
          src.a = static_cast<unsigned char>(
              (static_cast<int>(color.a) * alpha_val) / 255);
          blendPixel(
              image, cursor_x + gi.x_offset + col, y + gi.y_offset + row, src);
        }
      }
    }
    cursor_x += gi.advance;
    if (i + 1 < text.size()) {
      cursor_x += font.kern(text[i], text[i + 1]);
    }
  }
}

/* static */
void TileGenerator::drawTextRotated(std::vector<unsigned char>& image,
                                    const int x,
                                    const int y,
                                    const std::string_view text,
                                    const GlyphCache::FontSize& font,
                                    const Color& color)
{
  // 90° CW rotation: characters stack downward (y increasing).
  const int ch_h = font.cellHeight();
  int cursor_y = y;
  for (size_t i = 0; i < text.size(); ++i) {
    const auto gi = font.glyph(text[i]);
    if (gi.alpha != nullptr) {
      for (int row = 0; row < gi.bmp_height; ++row) {
        for (int col = 0; col < gi.bmp_width; ++col) {
          const unsigned char alpha_val = gi.alpha[row * gi.bmp_width + col];
          if (alpha_val == 0) {
            continue;
          }
          Color src = color;
          src.a = static_cast<unsigned char>(
              (static_cast<int>(color.a) * alpha_val) / 255);
          // Rotate 90° CW: (x_off+col, y_off+row) → screen
          //   (x + (H-1-(y_off+row)), cursor_y + x_off+col)
          const int px = x + (ch_h - 1 - gi.y_offset - row);
          const int py = cursor_y + gi.x_offset + col;
          blendPixel(image, px, py, src);
        }
      }
    }
    cursor_y += gi.advance;
    if (i + 1 < text.size()) {
      cursor_y += font.kern(text[i], text[i + 1]);
    }
  }
}

/* static */
void TileGenerator::blendPixel(std::vector<unsigned char>& image,
                               const int x,
                               const int y,
                               const Color& c,
                               int dim)
{
  if (dim < 0) {
    dim = bufferDim(image);
  }
  if (x < 0 || x >= dim || y < 0 || y >= dim) {
    return;
  }
  const int i = (y * dim + x) * 4;
  const float src_a = c.a / 255.0f;
  const float dst_a = image[i + 3] / 255.0f;
  const float out_a = src_a + dst_a * (1.0f - src_a);

  if (out_a <= 0.0f) {
    image[i + 0] = 0;
    image[i + 1] = 0;
    image[i + 2] = 0;
    image[i + 3] = 0;
    return;
  }

  const auto blend_channel = [&](const int src, const int dst) {
    const float out = (src * src_a + dst * dst_a * (1.0f - src_a)) / out_a;
    return static_cast<unsigned char>(std::lround(out));
  };

  image[i + 0] = blend_channel(c.r, image[i + 0]);
  image[i + 1] = blend_channel(c.g, image[i + 1]);
  image[i + 2] = blend_channel(c.b, image[i + 2]);
  image[i + 3] = static_cast<unsigned char>(std::lround(out_a * 255.0f));
}

void TileGenerator::drawFilledRect(std::vector<unsigned char>& buffer,
                                   const odb::Rect& rect,
                                   const Color& color,
                                   const FillPattern pattern,
                                   const int ox,
                                   const int oy,
                                   int dim) const
{
  // kNone paints nothing: skip the pixel loop entirely.
  if (pattern == FillPattern::kNone) {
    return;
  }
  if (dim < 0) {
    dim = bufferDim(buffer);
  }
  for (int iy = rect.yMin(); iy < rect.yMax(); ++iy) {
    const int draw_y = dim - 1 - iy;
    for (int ix = rect.xMin(); ix < rect.xMax(); ++ix) {
      if (pattern != FillPattern::kSolid
          && !patternCovers(pattern, ix + ox, iy + oy)) {
        continue;
      }
      setPixel(buffer, ix, draw_y, color, dim);
    }
  }
}

void TileGenerator::drawHighlight(std::vector<unsigned char>& image,
                                  const std::vector<odb::Rect>& rects,
                                  const std::vector<odb::Polygon>& polys,
                                  const TileFrame& frame) const
{
  const odb::Rect& dbu_tile = frame.cull;
  const Color fill{.r = 255, .g = 255, .b = 0, .a = 30};
  const Color border{.r = 255, .g = 255, .b = 0, .a = 255};
  const int dim = bufferDim(image);

  for (const odb::Rect& rect : rects) {
    if (!dbu_tile.overlaps(rect)) {
      continue;
    }
    const odb::Rect overlap = rect.intersect(dbu_tile);
    const odb::Rect draw = toPixels(frame, overlap);

    // Semi-transparent yellow fill
    for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        blendPixel(image, ix, dim - 1 - iy, fill, dim);
      }
    }

    // Solid yellow border (only where edge is within the tile)
    const odb::Rect full_draw = toPixels(frame, rect);
    if (full_draw.xMin() >= 0 && full_draw.xMin() < dim) {
      for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
        setPixel(image, full_draw.xMin(), dim - 1 - iy, border, dim);
      }
    }
    if (full_draw.xMax() > 0 && full_draw.xMax() <= dim) {
      const int rx = full_draw.xMax() - 1;
      for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
        setPixel(image, rx, dim - 1 - iy, border, dim);
      }
    }
    if (full_draw.yMin() >= 0 && full_draw.yMin() < dim) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        setPixel(image, ix, dim - 1 - full_draw.yMin(), border, dim);
      }
    }
    if (full_draw.yMax() > 0 && full_draw.yMax() <= dim) {
      const int ty = full_draw.yMax() - 1;
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        setPixel(image, ix, dim - 1 - ty, border, dim);
      }
    }
  }

  // Polygon highlights (octilinear shapes)
  for (const odb::Polygon& poly : polys) {
    const odb::Rect bbox = poly.getEnclosingRect();
    if (!dbu_tile.overlaps(bbox)) {
      continue;
    }

    // Semi-transparent yellow fill
    fillPolygon(image, poly, frame, fill, /*blend=*/true);

    // Solid yellow border — draw each edge.  Oblique by nature (these are the
    // octagons of octilinear SWires), so convert in double and let drawLine
    // clip: a vertex a whole die outside the tile overflows an int cast, and
    // saturating it per axis would tilt the edge.
    const auto& points = poly.getPoints();
    const int n = static_cast<int>(points.size());
    for (int i = 0; i < n - 1; ++i) {
      drawLine(image,
               toPxXd(points[i].x(), frame),
               toPxYd(points[i].y(), frame, dim),
               toPxXd(points[i + 1].x(), frame),
               toPxYd(points[i + 1].y(), frame, dim),
               border,
               penWidthCss(frame));
    }
  }
}

void TileGenerator::drawColoredPolygons(
    std::vector<unsigned char>& image,
    const std::vector<ColoredPolygon>& polys,
    const TileFrame& frame) const
{
  const odb::Rect& dbu_tile = frame.cull;
  const int dim = bufferDim(image);
  for (const ColoredPolygon& cp : polys) {
    const odb::Polygon& poly = cp.poly;
    if (!dbu_tile.overlaps(poly.getEnclosingRect())) {
      continue;
    }
    Color border = cp.color;
    border.a = 255;

    // Semi-transparent fill in the group's color.
    fillPolygon(image, poly, frame, cp.color, /*blend=*/true);

    // Solid outline in the group's color — draw each edge, wrapping the last
    // point back to the first so the outline is closed even if the point list
    // is open.  Octilinear by nature, so convert in double and let drawLine
    // clip (see drawHighlight): saturating each axis would tilt the edge.
    const auto& points = poly.getPoints();
    const int n = static_cast<int>(points.size());
    for (int i = 0; i < n; ++i) {
      const odb::Point& p0 = points[i];
      const odb::Point& p1 = points[(i + 1) % n];
      drawLine(image,
               toPxXd(p0.x(), frame),
               toPxYd(p0.y(), frame, dim),
               toPxXd(p1.x(), frame),
               toPxYd(p1.y(), frame, dim),
               border,
               penWidthCss(frame));
    }
  }
}

void TileGenerator::drawColoredHighlight(std::vector<unsigned char>& image,
                                         const std::vector<ColoredRect>& rects,
                                         const std::string& current_layer,
                                         const TileFrame& frame) const
{
  const odb::Rect& dbu_tile = frame.cull;
  const bool draw_all = current_layer.empty() || current_layer == "_instances";
  const int dim = bufferDim(image);
  for (const auto& cr : rects) {
    // Layer filtering: draw on _instances (overview), overlay (empty
    // current_layer), or matching layer.
    if (!draw_all && !cr.layer.empty() && cr.layer != current_layer) {
      continue;
    }
    if (!dbu_tile.overlaps(cr.rect)) {
      continue;
    }
    const odb::Rect overlap = cr.rect.intersect(dbu_tile);
    const odb::Rect draw = toPixels(frame, overlap);

    if (cr.filled) {
      // DRC marker style: semi-transparent filled rect with solid outline.
      // Matches the Qt GUI's DRCRenderer (white pen + white-alpha brush).

      // Fill interior
      const int pxl = std::max(0, draw.xMin());
      const int pyl = std::max(0, draw.yMin());
      const int pxh = std::min(dim - 1, draw.xMax());
      const int pyh = std::min(dim - 1, draw.yMax());
      for (int iy = pyl; iy <= pyh; ++iy) {
        for (int ix = pxl; ix <= pxh; ++ix) {
          blendPixel(image, ix, dim - 1 - iy, cr.color, dim);
        }
      }

      // Solid outline
      Color outline = cr.color;
      outline.a = 255;
      // Bottom edge
      for (int ix = pxl; ix <= pxh; ++ix) {
        blendPixel(image, ix, dim - 1 - pyl, outline, dim);
      }
      // Top edge
      for (int ix = pxl; ix <= pxh; ++ix) {
        blendPixel(image, ix, dim - 1 - pyh, outline, dim);
      }
      // Left edge
      for (int iy = pyl; iy <= pyh; ++iy) {
        blendPixel(image, pxl, dim - 1 - iy, outline, dim);
      }
      // Right edge
      for (int iy = pyl; iy <= pyh; ++iy) {
        blendPixel(image, pxh, dim - 1 - iy, outline, dim);
      }
    } else {
      // Timing path style: centerline through the shape.
      const int cx = (draw.xMin() + draw.xMax()) / 2;
      const int cy = (draw.yMin() + draw.yMax()) / 2;

      Color line_color = cr.color;
      line_color.a = 255;

      if (draw.dx() >= draw.dy()) {
        drawLine(image,
                 draw.xMin(),
                 dim - 1 - cy,
                 draw.xMax(),
                 dim - 1 - cy,
                 line_color);
      } else {
        drawLine(image,
                 cx,
                 dim - 1 - draw.yMin(),
                 cx,
                 dim - 1 - draw.yMax(),
                 line_color);
      }
    }
  }
}

/* static */
void TileGenerator::drawLine(std::vector<unsigned char>& image,
                             const double fx0,
                             const double fy0,
                             const double fx1,
                             const double fy1,
                             const Color& c,
                             const int width)
{
  const int r = (width - 1) / 2;
  int x0 = 0;
  int y0 = 0;
  int x1 = 0;
  int y1 = 0;

  // Liang-Barsky clip against the tile (with a margin for the brush
  // radius) BEFORE running Bresenham: callers may pass endpoints far
  // outside the tile (flywires, region edges at deep zoom) and an
  // unclipped line iterates one step per pixel of its full length.
  // The bound follows the buffer, like setPixel/drawFilledRect: overlay
  // painters run on the supersampled buffer, where a hardcoded 256 would
  // clip away everything past the first quadrant.
  //
  // Clipping (not saturating) is also what keeps the slope exact: the clip
  // moves both endpoints along the segment's own parameter t, so the
  // in-bounds result is collinear with the input no matter how far outside
  // the tile the endpoints started.
  {
    const double lo = -r - 1.0;
    const double hi = bufferDim(image) + r;
    const double dxf = fx1 - fx0;
    const double dyf = fy1 - fy0;
    double t0 = 0.0;
    double t1 = 1.0;
    auto clip = [&t0, &t1](double p, double q) {
      if (p == 0.0) {
        return q >= 0.0;  // parallel: inside iff q >= 0
      }
      const double t = q / p;
      if (p < 0) {
        if (t > t1) {
          return false;
        }
        t0 = std::max(t0, t);
      } else {
        if (t < t0) {
          return false;
        }
        t1 = std::min(t1, t);
      }
      return true;
    };
    if (!clip(-dxf, fx0 - lo) || !clip(dxf, hi - fx0) || !clip(-dyf, fy0 - lo)
        || !clip(dyf, hi - fy0)) {
      return;  // fully outside the tile
    }
    // Both ends now lie inside [lo, hi], so the casts cannot overflow.
    x1 = static_cast<int>(std::lround(fx0 + t1 * dxf));
    y1 = static_cast<int>(std::lround(fy0 + t1 * dyf));
    x0 = static_cast<int>(std::lround(fx0 + t0 * dxf));
    y0 = static_cast<int>(std::lround(fy0 + t0 * dyf));
  }

  // Bresenham's line algorithm
  int dx = std::abs(x1 - x0);
  int dy = std::abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = dx - dy;

  while (true) {
    if (r <= 0) {
      blendPixel(image, x0, y0, c);
    } else {
      for (int dy2 = -r; dy2 <= r; dy2++) {
        for (int dx2 = -r; dx2 <= r; dx2++) {
          blendPixel(image, x0 + dx2, y0 + dy2, c);
        }
      }
    }

    if (x0 == x1 && y0 == y1) {
      break;
    }
    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void TileGenerator::drawFlightLines(std::vector<unsigned char>& image,
                                    const std::vector<FlightLine>& lines,
                                    const TileFrame& frame) const
{
  const int dim = bufferDim(image);
  for (const auto& fl : lines) {
    // Convert DBU to pixel coordinates in double and let drawLine clip:
    // flywires can be far longer than a tile, and the clamped int conversion
    // would tilt them once one axis saturated at extreme zoom.
    const double px0 = toPxXd(fl.p1.x(), frame);
    const double py0 = toPxYd(fl.p1.y(), frame, dim);
    const double px1 = toPxXd(fl.p2.x(), frame);
    const double py1 = toPxYd(fl.p2.y(), frame, dim);

    // Rough bounding-box check: skip if line can't cross this tile
    const double lx0 = std::min(px0, px1), lx1 = std::max(px0, px1);
    const double ly0 = std::min(py0, py1), ly1 = std::max(py0, py1);
    if (lx1 < 0 || lx0 >= dim || ly1 < 0 || ly0 >= dim) {
      continue;
    }

    Color c = fl.color;
    c.a = 220;
    drawLine(image, px0, py0, px1, py1, c, penWidthCss(frame));
  }
}

namespace {

// One anchor name and where it puts the text box relative to the anchor
// point, as an edge to pin per axis.  kMin is the low-coordinate edge in
// PIXEL space, so vertically it is the box's top (pixel rows grow downward
// while DBU grow up).
struct AnchorEntry
{
  enum Edge
  {
    kMin,
    kMid,
    kMax
  };
  const char* name;
  Edge h;
  Edge v;
};

// The one table behind anchorNames(), isValidAnchor() and the placement in
// drawTextLabels, so a name can never be accepted and then not drawn.  The
// spellings must match gui::Painter::anchors(); the order is only that of the
// literal in painter.cpp and means nothing (anchors() is a std::map, so it
// iterates alphabetically).  The edges reproduce the switch over
// gui::Painter::Anchor in drawPainterOps.
constexpr AnchorEntry kAnchors[] = {
    {"bottom left", AnchorEntry::kMin, AnchorEntry::kMax},
    {"bottom right", AnchorEntry::kMax, AnchorEntry::kMax},
    {"top left", AnchorEntry::kMin, AnchorEntry::kMin},
    {"top right", AnchorEntry::kMax, AnchorEntry::kMin},
    {"center", AnchorEntry::kMid, AnchorEntry::kMid},
    {"bottom center", AnchorEntry::kMid, AnchorEntry::kMax},
    {"top center", AnchorEntry::kMid, AnchorEntry::kMin},
    {"left center", AnchorEntry::kMin, AnchorEntry::kMid},
    {"right center", AnchorEntry::kMax, AnchorEntry::kMid},
};

// "center" is both the documented default and the unrecognized-name
// fallback; pin its slot so reordering kAnchors cannot quietly change either.
constexpr size_t kCenterAnchor = 4;
static_assert(std::string_view(kAnchors[kCenterAnchor].name) == "center");

// The entry for `anchor`, or the "center" entry when it names nothing.
const AnchorEntry& anchorEntry(const std::string& anchor)
{
  for (const AnchorEntry& a : kAnchors) {
    if (anchor == a.name) {
      return a;
    }
  }
  return kAnchors[kCenterAnchor];
}

}  // namespace

const std::vector<std::string>& anchorNames()
{
  static const std::vector<std::string> names = [] {
    std::vector<std::string> out;
    out.reserve(std::size(kAnchors));
    for (const AnchorEntry& a : kAnchors) {
      out.emplace_back(a.name);
    }
    return out;
  }();
  return names;
}

bool isValidAnchor(const std::string& anchor)
{
  return std::any_of(
      std::begin(kAnchors),
      std::end(kAnchors),
      [&anchor](const AnchorEntry& a) { return anchor == a.name; });
}

void TileGenerator::drawTextLabels(std::vector<unsigned char>& image,
                                   const std::vector<TextLabel>& labels,
                                   const TileFrame& frame) const
{
  const int dim = bufferDim(image);
  for (const auto& label : labels) {
    if (label.text.empty()) {
      continue;
    }
    // The label's size is authored in CSS px, so it scales with the display
    // like every other pixel-specified size; a fixed value would shrink as
    // the ratio rises.
    constexpr int kDefaultLabelFontHeightCss = 14;
    const int font_px = std::max(
        1,
        static_cast<int>(std::lround(
            (label.size > 0 ? label.size : kDefaultLabelFontHeightCss)
            * frame.px_per_css)));
    const GlyphCache::FontSize& font = fontAtlasGetFont(font_px);
    const int text_width = getTextWidth(label.text, font);
    const int text_height = getTextHeight(font);
    const int pixel_x = toPxX(label.pos.x(), frame);
    const int pixel_y = toPxY(label.pos.y(), frame, dim);

    // Position the text box relative to the anchor point.  An unrecognized
    // name centres, matching Painter::stringToAnchor's fallback; the API
    // layers reject one before it gets here, so this is only the last word.
    // The box may straddle a tile seam; skip only when it is fully off-tile so
    // each tile draws its slice (drawText clips per-pixel).
    const AnchorEntry& a = anchorEntry(label.anchor);
    int text_px_min = pixel_x - text_width / 2;
    if (a.h == AnchorEntry::kMin) {
      text_px_min = pixel_x;
    } else if (a.h == AnchorEntry::kMax) {
      text_px_min = pixel_x - text_width;
    }
    int text_py_min = pixel_y - text_height / 2;
    if (a.v == AnchorEntry::kMin) {
      text_py_min = pixel_y;
    } else if (a.v == AnchorEntry::kMax) {
      text_py_min = pixel_y - text_height;
    }

    if (text_px_min + text_width <= 0 || text_px_min >= dim
        || text_py_min + text_height <= 0 || text_py_min >= dim) {
      continue;
    }
    drawText(image, text_px_min, text_py_min, label.text, font, label.color);
  }
}

std::vector<unsigned char> TileGenerator::renderLabelTile(
    const int z,
    const int x,
    int y,
    const std::vector<TextLabel>& labels,
    const double dpr,
    const int requested_tile_px) const
{
  if (labels.empty()) {
    return {};
  }
  // Same contract as generateOverlayTile: the caller states the device-pixel
  // square it will composite this tile into, so the glyphs land at the same
  // size and registration as the tiles beneath.
  const double effective_dpr = dpr > 0.0 ? dpr : 1.0;
  const int dim
      = requested_tile_px > 0
            ? requested_tile_px
            : static_cast<int>(std::lround(kTileSizeInPixel * effective_dpr));
  std::vector<unsigned char> image(static_cast<size_t>(dim) * dim * 4,
                                   0);  // transparent

  // Tile bounding box in DBU (same math as generateOverlayTile).
  const double num_tiles_at_zoom = pow(2, z);
  y = num_tiles_at_zoom - 1 - y;  // flip Y
  const odb::Rect full_bounds = getBounds();
  if (full_bounds.maxDXDY() <= 0) {
    return {};
  }
  const double tile_dbu_size = full_bounds.maxDXDY() / num_tiles_at_zoom;
  const TileFrame frame = tileFrame(
      full_bounds, x, y, tile_dbu_size, dim / tile_dbu_size, effective_dpr);

  drawTextLabels(image, labels, frame);
  return image;
}

//------------------------------------------------------------------------------
// User text labels (2.12) — global design annotations
//------------------------------------------------------------------------------

std::string TileGenerator::addLabel(const odb::Point& pos,
                                    const std::string& text,
                                    const Color& color,
                                    const int size,
                                    const std::string& anchor,
                                    const std::string& name)
{
  std::lock_guard<std::mutex> lock(labels_mutex_);
  auto name_in_use = [this](const std::string& n) {
    for (const auto& l : labels_) {
      if (l.name == n) {
        return true;
      }
    }
    return false;
  };
  std::string label_name = name;
  if (label_name.empty()) {
    // Auto-generate; skip ids already taken by a user-named label.
    do {
      label_name = "label" + std::to_string(next_label_id_++);
    } while (name_in_use(label_name));
  } else if (name_in_use(label_name)) {
    // Reject a duplicate explicit name (mirrors the Qt GUI, which warns
    // GUI-44).
    return "";
  }
  labels_.push_back(
      {pos, text, color, size, anchor.empty() ? "center" : anchor, label_name});
  return label_name;
}

bool TileGenerator::deleteLabel(const std::string& name)
{
  std::lock_guard<std::mutex> lock(labels_mutex_);
  for (auto it = labels_.begin(); it != labels_.end(); ++it) {
    if (it->name == name) {
      labels_.erase(it);
      return true;
    }
  }
  return false;
}

bool TileGenerator::updateLabel(const std::string& name,
                                const odb::Point& pos,
                                const std::string& text,
                                const Color& color,
                                const int size,
                                const std::string& anchor)
{
  std::lock_guard<std::mutex> lock(labels_mutex_);
  for (auto& l : labels_) {
    if (l.name == name) {
      l.pos = pos;
      l.text = text;
      l.color = color;
      l.size = size;
      l.anchor = anchor.empty() ? "center" : anchor;
      return true;
    }
  }
  return false;
}

void TileGenerator::clearLabels()
{
  std::lock_guard<std::mutex> lock(labels_mutex_);
  labels_.clear();
}

std::vector<TextLabel> TileGenerator::labelsForDraw() const
{
  std::lock_guard<std::mutex> lock(labels_mutex_);
  std::vector<TextLabel> out;
  out.reserve(labels_.size());
  for (const auto& l : labels_) {
    out.push_back({l.pos, l.text, l.color, l.size, l.anchor});
  }
  return out;
}

boost::json::array TileGenerator::labelsJson() const
{
  std::lock_guard<std::mutex> lock(labels_mutex_);
  boost::json::array arr;
  arr.reserve(labels_.size());
  for (const auto& l : labels_) {
    boost::json::object o;
    o["name"] = l.name;
    o["x"] = l.pos.x();
    o["y"] = l.pos.y();
    o["text"] = l.text;
    o["size"] = l.size;
    o["anchor"] = l.anchor;
    boost::json::object c;
    c["r"] = static_cast<int>(l.color.r);
    c["g"] = static_cast<int>(l.color.g);
    c["b"] = static_cast<int>(l.color.b);
    c["a"] = static_cast<int>(l.color.a);
    o["color"] = std::move(c);
    arr.emplace_back(std::move(o));
  }
  return arr;
}

void TileGenerator::drawRouteGuides(std::vector<unsigned char>& image,
                                    const std::set<uint32_t>& net_ids,
                                    const std::string& layer,
                                    const Color& layer_color,
                                    const TileFrame& frame) const
{
  const odb::Rect& dbu_tile = frame.cull;
  odb::dbBlock* block = getBlock();
  if (!block) {
    return;
  }

  const Color fill{
      .r = layer_color.r, .g = layer_color.g, .b = layer_color.b, .a = 50};
  const Color border{
      .r = layer_color.r, .g = layer_color.g, .b = layer_color.b, .a = 180};
  const int dim = bufferDim(image);

  for (const uint32_t net_id : net_ids) {
    odb::dbNet* net = odb::dbNet::getNet(block, net_id);
    if (!net) {
      continue;
    }
    for (odb::dbGuide* guide : net->getGuides()) {
      if (guide->getLayer()->getName() != layer) {
        continue;
      }
      const odb::Rect box = guide->getBox();
      if (!dbu_tile.overlaps(box)) {
        continue;
      }
      const odb::Rect overlap = box.intersect(dbu_tile);
      const odb::Rect draw = toPixels(frame, overlap);

      // Semi-transparent fill
      for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
        for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
          blendPixel(image, ix, dim - 1 - iy, fill, dim);
        }
      }

      // Border (only where guide edge is within this tile)
      const odb::Rect full_draw = toPixels(frame, box);
      if (full_draw.xMin() >= 0 && full_draw.xMin() < dim) {
        for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
          blendPixel(image, full_draw.xMin(), dim - 1 - iy, border, dim);
        }
      }
      if (full_draw.xMax() > 0 && full_draw.xMax() <= dim) {
        const int rx = full_draw.xMax() - 1;
        for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
          blendPixel(image, rx, dim - 1 - iy, border, dim);
        }
      }
      if (full_draw.yMin() >= 0 && full_draw.yMin() < dim) {
        for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
          blendPixel(image, ix, dim - 1 - full_draw.yMin(), border, dim);
        }
      }
      if (full_draw.yMax() > 0 && full_draw.yMax() <= dim) {
        const int ty = full_draw.yMax() - 1;
        for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
          blendPixel(image, ix, dim - 1 - ty, border, dim);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
// Timing path highlight shape collection
//------------------------------------------------------------------------------

std::pair<odb::dbITerm*, odb::dbBTerm*> resolvePin(odb::dbBlock* block,
                                                   const std::string& pin_name)
{
  odb::dbITerm* iterm = block->findITerm(pin_name.c_str());
  if (iterm) {
    return {iterm, nullptr};
  }
  return {nullptr, block->findBTerm(pin_name.c_str())};
}

static odb::dbNet* getNetFromPin(odb::dbITerm* iterm, odb::dbBTerm* bterm)
{
  if (iterm) {
    return iterm->getNet();
  }
  if (bterm) {
    return bterm->getNet();
  }
  return nullptr;
}

static odb::Point getPinLocation(odb::dbITerm* iterm, odb::dbBTerm* bterm)
{
  if (iterm) {
    int x, y;
    if (iterm->getAvgXY(&x, &y)) {
      return {x, y};
    }
    // Fallback to instance center
    odb::Rect bbox = iterm->getInst()->getBBox()->getBox();
    return {(bbox.xMin() + bbox.xMax()) / 2, (bbox.yMin() + bbox.yMax()) / 2};
  }
  if (bterm) {
    for (odb::dbBPin* bpin : bterm->getBPins()) {
      odb::Rect r = bpin->getBBox();
      return {(r.xMin() + r.xMax()) / 2, (r.yMin() + r.yMax()) / 2};
    }
  }
  return {0, 0};
}

void collectNetShapes(odb::dbNet* net,
                      odb::dbITerm* drv_iterm,
                      odb::dbBTerm* drv_bterm,
                      odb::dbITerm* snk_iterm,
                      odb::dbBTerm* snk_bterm,
                      const Color& color,
                      std::vector<ColoredRect>& rects,
                      std::vector<FlightLine>& lines)
{
  odb::dbWire* wire = net->getWire();
  if (wire) {
    odb::dbWireShapeItr itr;
    odb::dbShape shape;
    for (itr.begin(wire); itr.next(shape);) {
      if (shape.isVia()) {
        std::vector<odb::dbShape> via_boxes;
        odb::dbShape::getViaBoxes(shape, via_boxes);
        for (const auto& vbox : via_boxes) {
          odb::dbTechLayer* layer = vbox.getTechLayer();
          rects.push_back(
              {vbox.getBox(), color, layer ? layer->getName() : ""});
        }
      } else {
        odb::dbTechLayer* layer = shape.getTechLayer();
        rects.push_back({shape.getBox(), color, layer ? layer->getName() : ""});
      }
    }
  } else {
    // Unrouted: draw flight line between driver and sink
    odb::Point p1 = getPinLocation(drv_iterm, drv_bterm);
    odb::Point p2 = getPinLocation(snk_iterm, snk_bterm);
    lines.push_back({p1, p2, color});
  }
}

void collectTimingPathShapes(odb::dbBlock* block,
                             const TimingPathSummary& path,
                             std::vector<ColoredRect>& rects,
                             std::vector<FlightLine>& lines)
{
  static const Color kLaunchClkColor{
      .r = 0, .g = 255, .b = 255, .a = 180};                            // Cyan
  static const Color kSignalColor{.r = 255, .g = 0, .b = 0, .a = 180};  // Red
  static const Color kCaptureClkColor{
      .r = 0, .g = 255, .b = 0, .a = 180};  // Green

  // Track nets already collected to avoid duplicates
  odb::PtrSet<odb::dbNet> seen_nets;

  auto process_nodes = [&](const std::vector<TimingNode>& nodes,
                           const Color& clk_color,
                           const Color& data_color) {
    for (size_t i = 0; i + 1 < nodes.size(); i++) {
      auto [a_iterm, a_bterm] = resolvePin(block, nodes[i].pin_name);
      auto [b_iterm, b_bterm] = resolvePin(block, nodes[i + 1].pin_name);

      odb::dbNet* net_a = getNetFromPin(a_iterm, a_bterm);
      odb::dbNet* net_b = getNetFromPin(b_iterm, b_bterm);

      // Only draw when consecutive pins are on the same net (wire segment)
      if (net_a && net_a == net_b && seen_nets.insert(net_a).second) {
        const Color& c = nodes[i].is_clock ? clk_color : data_color;
        collectNetShapes(
            net_a, a_iterm, a_bterm, b_iterm, b_bterm, c, rects, lines);
      }
    }
  };

  // data_nodes: launch clock (is_clock=true) then signal portion
  process_nodes(path.data_nodes, kLaunchClkColor, kSignalColor);

  // capture_nodes: capture clock path
  process_nodes(path.capture_nodes, kCaptureClkColor, kCaptureClkColor);
}

namespace {

// Build the layer_hierarchy JSON by walking the ChipletNode tree produced
// by collectChipletsRec.  Reading identity (name, path) straight from
// ChipletNode keeps layer_hierarchy.path byte-identical to the parallel
// chipletData[*].path the frontend uses to key visibleChiplets — they
// share a single source of truth.
boost::json::object buildLayerHierarchy(
    const ChipletNode& node,
    const std::unordered_map<std::string, std::vector<const ChipletNode*>>&
        children_by_parent,
    const TileGenerator& gen)
{
  boost::json::object json_node;
  json_node["name"] = node.name;
  json_node["type"] = (node.inst == nullptr) ? "block" : "instance";
  json_node["path"] = node.path;

  // Classify tech layers into the same groups the Qt GUI uses
  // (displayControls.cpp): routing/cut layers stay in the main "Layers"
  // group (with backside split into its own category), IMPLANT layers go
  // under "Implant", and every other tech-layer type (MASTERSLICE, OVERLAP,
  // …) goes under "Other".  Unlike the Qt GUI (where "Other" defaults
  // unchecked), the web starts every category visible.
  boost::json::array layers_arr;
  boost::json::array backside_layers_arr;
  boost::json::array implant_layers_arr;
  boost::json::array other_layers_arr;
  if (node.chip) {
    if (odb::dbTech* tech = node.chip->getTech()) {
      const auto& layer_colors = gen.getLayerColorMap(tech);
      for (odb::dbTechLayer* layer : tech->getLayers()) {
        const LayerGroup group = classifyLayer(layer);
        if (group == LayerGroup::kSkip) {
          continue;
        }
        boost::json::object layer_obj;
        layer_obj["name"] = layer->getName();
        Color c{.r = 200, .g = 200, .b = 200, .a = 180};
        auto it = layer_colors.find(layer);
        if (it != layer_colors.end()) {
          c = it->second;
        }
        layer_obj["color"] = boost::json::array{static_cast<int>(c.r),
                                                static_cast<int>(c.g),
                                                static_cast<int>(c.b)};
        switch (group) {
          case LayerGroup::kRouting:
            if (layer->isBackside()) {
              backside_layers_arr.emplace_back(std::move(layer_obj));
            } else {
              layers_arr.emplace_back(std::move(layer_obj));
            }
            break;
          case LayerGroup::kImplant:
            implant_layers_arr.emplace_back(std::move(layer_obj));
            break;
          case LayerGroup::kOther:
            other_layers_arr.emplace_back(std::move(layer_obj));
            break;
          case LayerGroup::kSkip:
            break;  // guarded above; kept for -Wswitch exhaustiveness
        }
      }
    }
  }
  json_node["layers"] = std::move(layers_arr);

  boost::json::array instances_arr;
  if (auto it = children_by_parent.find(node.path);
      it != children_by_parent.end()) {
    for (const ChipletNode* child : it->second) {
      instances_arr.emplace_back(
          buildLayerHierarchy(*child, children_by_parent, gen));
    }
  }
  // Emit category folders, mirroring the Backside node.  Each is a pure UI
  // grouping (no chiplet path) and is only added when it has layers.
  auto emit_category
      = [&instances_arr](const char* name, boost::json::array&& cat_layers) {
          if (cat_layers.empty()) {
            return;
          }
          boost::json::object cat_node;
          cat_node["name"] = name;
          cat_node["type"] = "category";
          cat_node["layers"] = std::move(cat_layers);
          cat_node["instances"] = boost::json::array{};
          instances_arr.emplace_back(std::move(cat_node));
        };
  emit_category("Backside", std::move(backside_layers_arr));
  emit_category("Implant", std::move(implant_layers_arr));
  emit_category("Other", std::move(other_layers_arr));
  json_node["instances"] = std::move(instances_arr);

  return json_node;
}

}  // namespace

boost::json::object serializeTechResponse(const TileGenerator& gen)
{
  boost::json::object out;

  boost::json::array layers;
  for (const auto& name : gen.getLayers()) {
    layers.emplace_back(name);
  }
  out["layers"] = std::move(layers);

  // Flat layer_colors array aligned with out["layers"].  In multi-tech
  // (3DBlox) designs gen.getTech() is nullptr; resolve each name in the
  // first tech that defines it, mirroring how the world overlay above
  // looks up its color.  The main frontend prefers layer_hierarchy.color,
  // but this fallback keeps clients that consume the flat array correct.
  boost::json::array layer_color_arr;
  for (const auto& name : gen.getLayers()) {
    Color c{.r = 200, .g = 200, .b = 200, .a = 180};
    for (odb::dbTech* t : gen.getDb()->getTechs()) {
      odb::dbTechLayer* layer = t->findLayer(name.c_str());
      if (!layer) {
        continue;
      }
      const auto& cmap = gen.getLayerColorMap(t);
      auto it = cmap.find(layer);
      if (it != cmap.end()) {
        c = it->second;
      }
      break;
    }
    layer_color_arr.emplace_back(boost::json::array{
        static_cast<int>(c.r), static_cast<int>(c.g), static_cast<int>(c.b)});
  }
  out["layer_colors"] = std::move(layer_color_arr);

  // The 16 highlight-group colors, straight from the Qt GUI's palette so
  // the client swatches can never drift from what the overlay renders.
  boost::json::array highlight_colors;
  for (const auto& c : gui::Painter::kHighlightColors) {
    highlight_colors.emplace_back(boost::json::array{c.r, c.g, c.b, c.a});
  }
  out["highlight_colors"] = std::move(highlight_colors);

  boost::json::array sites;
  for (const auto& name : gen.getSites()) {
    sites.emplace_back(name);
  }
  out["sites"] = std::move(sites);

  out["has_liberty"] = gen.hasSta();
  // Lets the client skip creating the default-on "_regions" tile layer
  // (and its per-viewport requests) when the design has no dbRegion.
  out["has_regions"] = gen.getBlock() && !gen.getBlock()->getRegions().empty();
  // For 3DBlox designs the top dbChip is HIER and has no dbBlock; the
  // chiplet list below is still emitted so the frontend can group layers
  // by chiplet.
  if (gen.getBlock()) {
    out["dbu_per_micron"] = gen.getBlock()->getDbUnitsPerMicron();
    out["block_name"] = gen.getBlock()->getName();
  } else {
    odb::dbChip* top_chip = gen.getChip();
    out["block_name"] = top_chip ? top_chip->getName() : "";
  }

  // Walk the ChipletNode vector once, accumulating both the hierarchical
  // index (children_by_parent) that buildLayerHierarchy consumes and the
  // flat array the frontend exposes as `chiplets`.  Both outputs read
  // identity (name, path) straight from collectChipletsRec, so
  // layer_hierarchy.path stays byte-identical to chiplets[*].path.
  auto orientStr = [](const odb::dbOrientType& o) {
    return std::string(odb::dbOrientType(o).getString());
  };
  std::unordered_map<std::string, std::vector<const ChipletNode*>>
      children_by_parent;
  const ChipletNode* root_node = nullptr;
  boost::json::array chiplets;
  for (const ChipletNode& n : gen.chiplets()) {
    if (n.parent_path.empty()) {
      root_node = &n;
    } else {
      children_by_parent[n.parent_path].push_back(&n);
    }

    boost::json::object entry;
    entry["path"] = n.path;
    entry["name"] = n.name;
    entry["depth"] = n.depth;
    if (n.parent_path.empty()) {
      entry["parent"] = nullptr;
    } else {
      entry["parent"] = n.parent_path;
    }
    if (n.chip && n.chip->getBlock()) {
      entry["master"] = n.chip->getBlock()->getName();
    } else {
      entry["master"] = "";
    }
    if (n.block) {
      odb::Rect b = n.block->getBBox()->getBox();
      boost::json::array local_bbox{b.xMin(), b.yMin(), b.xMax(), b.yMax()};
      entry["bbox_dbu_local"] = std::move(local_bbox);
      odb::Rect bw = b;
      n.world_xfm.apply(bw);
      boost::json::array world_bbox{bw.xMin(), bw.yMin(), bw.xMax(), bw.yMax()};
      entry["bbox_dbu_world"] = std::move(world_bbox);
    }
    const odb::Point off = n.world_xfm.getOffset();
    entry["world_origin_dbu"] = boost::json::array{off.x(), off.y()};
    entry["orient"] = orientStr(n.world_xfm.getOrient());
    chiplets.emplace_back(std::move(entry));
  }
  if (root_node) {
    out["layer_hierarchy"]
        = buildLayerHierarchy(*root_node, children_by_parent, gen);
  }
  out["chiplets"] = std::move(chiplets);
  return out;
}

boost::json::object serializeBoundsResponse(const TileGenerator& gen,
                                            bool shapes_ready)
{
  const odb::Rect bounds = gen.getBounds();
  boost::json::object out;
  out["bounds"]
      = boost::json::array{boost::json::array{bounds.yMin(), bounds.xMin()},
                           boost::json::array{bounds.yMax(), bounds.xMax()}};
  out["shapes_ready"] = shapes_ready;
  out["pin_max_size"] = gen.getPinMaxSize();
  return out;
}

}  // namespace web
