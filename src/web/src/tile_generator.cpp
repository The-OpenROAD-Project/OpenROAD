// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "tile_generator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
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
#include "web_painter.h"

namespace web {

namespace {

constexpr float kPinMarkerSizeRatio = 0.02;
constexpr int kMinPinMarkerSize = 8;
constexpr int kMinPinNameSizePixels = 20;
constexpr int kPinLabelFontHeight = 14;  // pre-baked atlas size for pin labels
constexpr int kItermLabelFontHeight = 10;  // atlas size for ITerm pin labels
constexpr int kMinItermLabelBoxPx = 10;    // min pin-box pixel dim for labels
constexpr int kMinInstNameFontPx = 10;     // minimum readable font size
constexpr int kMaxInstNameFontPx = 40;     // cap font size for large macros
constexpr int kMinInstNameBoxPx = 20;      // min instance pixel dim for names

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
    {"pins",               &TileVisibility::pins,               true},
    {"pin_markers",        &TileVisibility::pin_markers,        true},
    {"pin_names",          &TileVisibility::pin_names,          true},
    {"inst_names",         &TileVisibility::inst_names,         true},
    {"inst_pins",          &TileVisibility::inst_pins,          true},
    {"inst_pin_names",     &TileVisibility::inst_pin_names,     true},
    {"blockages",              &TileVisibility::blockages,              true},
    {"placement_blockages",    &TileVisibility::placement_blockages,    true},
    {"routing_obstructions",   &TileVisibility::routing_obstructions,   true},
    {"rows",                   &TileVisibility::rows,                   false},
    {"tracks_pref",            &TileVisibility::tracks_pref,            false},
    {"tracks_non_pref",        &TileVisibility::tracks_non_pref,        false},
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
  switch (classifyInstance(inst, sta)) {
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
}

TileGenerator::~TileGenerator() = default;

void TileGenerator::eagerInit()
{
  // Invalidate the chiplet cache: setTopChip below may swap to a fresh
  // dbChip whose ChipletNode addresses (dbBlock*, dbChip*, etc.) differ
  // from the previous design's.
  {
    std::lock_guard lock(chiplets_mutex_);
    chiplets_cache_.clear();
    chiplets_cache_valid_ = false;
  }

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
  }
  return chiplets_cache_;
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
odb::Rect TileGenerator::toPixels(const double scale,
                                  const odb::Rect& rect,
                                  const odb::Rect& dbu_tile)
{
  return odb::Rect((rect.xMin() - dbu_tile.xMin()) * scale,
                   (rect.yMin() - dbu_tile.yMin()) * scale,
                   std::ceil((rect.xMax() - dbu_tile.xMin()) * scale),
                   std::ceil((rect.yMax() - dbu_tile.yMin()) * scale));
}

void TileGenerator::setPixel(std::vector<unsigned char>& image,
                             const int x,
                             const int y,
                             const Color& c) const
{
  if (x < 0 || x >= kTileSizeInPixel || y < 0 || y >= kTileSizeInPixel) {
    return;
  }
  const int index = (y * kTileSizeInPixel + x) * 4;
  image[index + 0] = c.r;
  image[index + 1] = c.g;
  image[index + 2] = c.b;
  image[index + 3] = c.a;
}

void TileGenerator::fillPolygon(std::vector<unsigned char>& image,
                                const odb::Polygon& poly,
                                const odb::Rect& dbu_tile,
                                const double scale,
                                const Color& color,
                                const bool blend) const
{
  const auto& points = poly.getPoints();
  const int n = static_cast<int>(points.size());
  if (n < 3) {
    return;
  }

  // Convert polygon points to pixel coordinates (floating point for precision).
  std::vector<double> px(n), py(n);
  for (int i = 0; i < n; ++i) {
    px[i] = (points[i].x() - dbu_tile.xMin()) * scale;
    py[i] = (points[i].y() - dbu_tile.yMin()) * scale;
  }

  // Compute pixel bounding box, clamped to tile.
  const double min_py = std::ranges::min(py);
  const double max_py = std::ranges::max(py);
  const int iy_min = std::max(0, static_cast<int>(min_py));
  const int iy_max
      = std::min(kTileSizeInPixel, static_cast<int>(std::ceil(max_py)));

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
      const int ix_max = std::min(
          kTileSizeInPixel, static_cast<int>(std::ceil(x_intercepts[k + 1])));
      const int draw_y = 255 - iy;
      for (int ix = ix_min; ix < ix_max; ++ix) {
        if (blend) {
          blendPixel(image, ix, draw_y, color);
        } else {
          setPixel(image, ix, draw_y, color);
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
      if (layer->getRoutingLevel() > 0
          || layer->getType() == odb::dbTechLayerType::CUT) {
        const std::string name = layer->getName();
        if (seen.insert(name).second) {
          layers.push_back(name);
        }
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
  debugPrint(logger_,
             utl::WEB,
             "select",
             1,
             "selectAt dbu=({},{}) zoom={} margin={}",
             dbu_x,
             dbu_y,
             zoom,
             margin);

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
        results.push_back({inst, label, "Inst", toWorld(bbox), true});
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
    const std::set<uint32_t>* route_guide_net_ids) const
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
                                       route_guide_net_ids);

  std::vector<unsigned char> png_data;
  const unsigned error = lodepng::encode(
      png_data, image_buffer, kTileSizeInPixel, kTileSizeInPixel);
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
    const std::set<std::string>& visible_layers) const
{
  constexpr int kBufferSize = kTileSizeInPixel * kTileSizeInPixel * 4;
  std::vector<unsigned char> image(kBufferSize, 0);  // fully transparent

  if (!getChip()) {
    // No design — return blank transparent PNG.
    std::vector<unsigned char> png;
    lodepng::encode(png, image, kTileSizeInPixel, kTileSizeInPixel);
    return png;
  }

  // Short-circuit: if there's nothing to draw, return a blank tile.
  if (highlight_rects.empty() && highlight_polys.empty()
      && colored_rects.empty() && flight_lines.empty()
      && (!route_guide_net_ids || route_guide_net_ids->empty())) {
    std::vector<unsigned char> png;
    lodepng::encode(png, image, kTileSizeInPixel, kTileSizeInPixel);
    return png;
  }

  // Compute tile bounding box in DBU (same math as renderTileBuffer).
  const double num_tiles_at_zoom = pow(2, z);
  y = num_tiles_at_zoom - 1 - y;  // flip Y
  const odb::Rect full_bounds = getBounds();
  if (full_bounds.maxDXDY() <= 0) {
    std::vector<unsigned char> png;
    lodepng::encode(png, image, kTileSizeInPixel, kTileSizeInPixel);
    return png;
  }
  const double tile_dbu_size = full_bounds.maxDXDY() / num_tiles_at_zoom;
  const int dbu_x_min = full_bounds.xMin() + x * tile_dbu_size;
  const int dbu_y_min = full_bounds.yMin() + y * tile_dbu_size;
  const int dbu_x_max = full_bounds.xMin() + std::ceil((x + 1) * tile_dbu_size);
  const int dbu_y_max = full_bounds.yMin() + std::ceil((y + 1) * tile_dbu_size);
  const odb::Rect dbu_tile(dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max);
  const double scale = kTileSizeInPixel / tile_dbu_size;

  // Draw highlight shapes onto transparent buffer.
  if (!highlight_rects.empty() || !highlight_polys.empty()) {
    drawHighlight(image, highlight_rects, highlight_polys, dbu_tile, scale);
  }
  if (!colored_rects.empty()) {
    // Pass empty layer name so all colored rects are drawn regardless of
    // their per-layer tag (the overlay sits above all base layers).
    drawColoredHighlight(image, colored_rects, "", dbu_tile, scale);
  }
  if (!flight_lines.empty()) {
    drawFlightLines(image, flight_lines, dbu_tile, scale);
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
        drawRouteGuides(image,
                        *route_guide_net_ids,
                        layer->getName(),
                        it->second,
                        dbu_tile,
                        scale);
      }
    }
  }

  std::vector<unsigned char> png;
  lodepng::encode(png, image, kTileSizeInPixel, kTileSizeInPixel);
  return png;
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
    const std::set<uint32_t>* route_guide_net_ids) const
{
  static_assert(sizeof(Color) == 4);
  constexpr int kBufferSize = kTileSizeInPixel * kTileSizeInPixel * 4;
  // The world (final) tile buffer.  Inside the per-chiplet loop, a
  // local alias `image_buffer` may instead refer to local_image_buffer
  // when the chiplet's orient ≠ R0 (slow-path); the alias is composited
  // back here at the end of each chiplet iteration via reverse mapping.
  std::vector<unsigned char> world_image_buffer(kBufferSize, 0);

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
    const int dbu_x_min_world = full_bounds.xMin() + x * tile_dbu_size;
    const int dbu_y_min_world = full_bounds.yMin() + y * tile_dbu_size;
    const int dbu_x_max_world
        = full_bounds.xMin() + std::ceil((x + 1) * tile_dbu_size);
    const int dbu_y_max_world
        = full_bounds.yMin() + std::ceil((y + 1) * tile_dbu_size);
    const odb::Rect dbu_tile_world(
        dbu_x_min_world, dbu_y_min_world, dbu_x_max_world, dbu_y_max_world);
    const double scale = kTileSizeInPixel / tile_dbu_size;

    // Per-chiplet rendering loop.  Mirrors RenderThread::drawChips() in
    // the Qt GUI: walks dbChip → dbChipInst → masterChip and draws each
    // chiplet's block in its own frame, accumulating into the same tile
    // image.  The world tile rect is shifted into each chiplet's local
    // frame for translation-only transforms; non-R0 orientations log
    // and skip for v1 (followup work to support full transforms).
    const std::vector<ChipletNode>& chiplet_nodes = chiplets();
    // The die-outline overlay only makes sense in multi-die designs; in
    // single-chip layouts there is no chiplet to demarcate, and adding
    // it caused regressions because every "expect transparent" test
    // started seeing a gray frame.
    const bool draw_die_outline = chiplet_nodes.size() > 1;
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
        local_image_buffer.resize(kBufferSize, 0);
      }
      // Alias the buffer the chiplet loop writes into.  In the R0
      // fast-path it's the world buffer (so writes land directly).  In
      // the slow-path it's a per-chiplet local buffer that the
      // reverse-mapping block at the end of this iteration composites
      // back onto world_image_buffer.
      auto& image_buffer = use_local ? local_image_buffer : world_image_buffer;

      odb::Rect dbu_tile = dbu_tile_world;
      if (use_local) {
        odb::dbTransform inv_xfm = node.world_xfm;
        inv_xfm.invert();
        inv_xfm.apply(dbu_tile);
      } else {
        const odb::Point xfm_off = node.world_xfm.getOffset();
        dbu_tile = odb::Rect(dbu_x_min_world - xfm_off.x(),
                             dbu_y_min_world - xfm_off.y(),
                             dbu_x_max_world - xfm_off.x(),
                             dbu_y_max_world - xfm_off.y());
      }
      const int dbu_x_min = dbu_tile.xMin();
      const int dbu_y_min = dbu_tile.yMin();
      const int dbu_x_max = dbu_tile.xMax();
      const int dbu_y_max = dbu_tile.yMax();

      // Mirrors RenderThread::drawChip() in the Qt GUI: outline the die
      // boundary so the chiplet shape is visible regardless of which
      // tech layer is active. Drawn once per layer-pass on every tile,
      // but only in multi-die designs where the demarcation is useful.
      if (draw_die_outline) {
        const odb::Rect die = block->getDieArea();
        if (die.area() > 0) {
          const int xl = die.xMin();
          const int yl = die.yMin();
          const int xh = die.xMax();
          const int yh = die.yMax();
          const int64_t pixel_xl = (int64_t) ((xl - dbu_x_min) * scale);
          const int64_t pixel_yl = (int64_t) ((yl - dbu_y_min) * scale);
          const int64_t pixel_xh
              = (int64_t) std::ceil((xh - dbu_x_min) * scale);
          const int64_t pixel_yh
              = (int64_t) std::ceil((yh - dbu_y_min) * scale);

          const int loop_xl = std::clamp<int64_t>(pixel_xl, 0, 256);
          const int loop_yl = std::clamp<int64_t>(pixel_yl, 0, 256);
          const int loop_xh = std::clamp<int64_t>(pixel_xh, 0, 256);
          const int loop_yh = std::clamp<int64_t>(pixel_yh, 0, 256);

          const int draw_xl = std::clamp<int64_t>(pixel_xl, 0, 255);
          const int draw_yl = std::clamp<int64_t>(pixel_yl, 0, 255);
          const int draw_xh = std::clamp<int64_t>(pixel_xh, 0, 255);
          const int draw_yh = std::clamp<int64_t>(pixel_yh, 0, 255);

          constexpr Color die_outline{.r = 128, .g = 128, .b = 128, .a = 255};
          if (dbu_x_min <= xl && xl <= dbu_x_max) {
            for (int iy = loop_yl; iy < loop_yh; ++iy) {
              setPixel(image_buffer, draw_xl, 255 - iy, die_outline);
            }
          }
          if (dbu_x_min <= xh && xh <= dbu_x_max) {
            for (int iy = loop_yl; iy < loop_yh; ++iy) {
              setPixel(image_buffer, draw_xh, 255 - iy, die_outline);
            }
          }
          if (dbu_y_min <= yl && yl <= dbu_y_max) {
            for (int ix = loop_xl; ix < loop_xh; ++ix) {
              setPixel(image_buffer, ix, 255 - draw_yl, die_outline);
            }
          }
          if (dbu_y_min <= yh && yh <= dbu_y_max) {
            for (int ix = loop_xl; ix < loop_xh; ++ix) {
              setPixel(image_buffer, ix, 255 - draw_yh, die_outline);
            }
          }
        }
      }

      odb::dbTechLayer* tech_layer = tech->findLayer(layer.c_str());
      Color color{.r = 200, .g = 200, .b = 200, .a = 180};
      if (tech_layer) {
        const auto it = layer_colors.find(tech_layer);
        if (it != layer_colors.end()) {
          color = it->second;
        }
      }
      const Color obs_color = color.lighter();

      // Special "_modules" layer: draw filled module-colored rectangles
      const bool modules_layer
          = (layer == "_modules" && module_colors && !module_colors->empty());
      if (modules_layer) {
        for (odb::dbInst* inst : search_->searchInsts(
                 block, dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max)) {
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
              255, (int) std::ceil((inst_bbox.xMax() - dbu_x_min) * scale));
          const int pyh = std::min(
              255, (int) std::ceil((inst_bbox.yMax() - dbu_y_min) * scale));
          for (int iy = pyl; iy < pyh; ++iy) {
            for (int ix = pxl; ix < pxh; ++ix) {
              blendPixel(image_buffer, ix, 255 - iy, c);
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
        const bool draw_pin_names
            = (static_cast<int>(die_pin_size * scale) >= kMinPinNameSizePixels);
        const auto pin_label_font = fontAtlasGetFont(kPinLabelFontHeight);

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
                fillPolygon(
                    image_buffer, marker_poly, dbu_tile, scale, marker_color);
              }

              // Draw the box rect itself (same as GUI painter.drawRect).
              if (box_rect.overlaps(dbu_tile)) {
                const odb::Rect overlap = box_rect.intersect(dbu_tile);
                const odb::Rect draw = toPixels(scale, overlap, dbu_tile);
                drawFilledRect(image_buffer, draw, marker_color);
              }

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
                const int anchor_py = 255 - anchor_py_raw;

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

                if (px > -block_w && px < kTileSizeInPixel && py > -block_h
                    && py < kTileSizeInPixel) {
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

      // Special "_instances" layer: only draw instance borders, no routing
      const bool instances_only = (layer == "_instances");

      // "_modules" and "_pins" layers handle their own drawing above;
      // skip all other drawing (instances, routing, etc.)
      if (!modules_layer && !pins_layer) {
        const auto iterm_font = fontAtlasGetFont(kItermLabelFontHeight);
        const int iterm_font_h = getTextHeight(iterm_font);

        // Draw instances
        for (odb::dbInst* inst : search_->searchInsts(
                 block, dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max)) {
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

          const int loop_xl = std::clamp<int64_t>(pixel_xl, 0, 256);
          const int loop_yl = std::clamp<int64_t>(pixel_yl, 0, 256);
          const int loop_xh = std::clamp<int64_t>(pixel_xh, 0, 256);
          const int loop_yh = std::clamp<int64_t>(pixel_yh, 0, 256);

          const int draw_xl = std::clamp<int64_t>(pixel_xl, 0, 255);
          const int draw_yl = std::clamp<int64_t>(pixel_yl, 0, 255);
          const int draw_xh = std::clamp<int64_t>(pixel_xh, 0, 255);
          const int draw_yh = std::clamp<int64_t>(pixel_yh, 0, 255);

          if (instances_only) {
            // Draw the rectangle border (instances-only layer)
            const Color gray{.r = 128, .g = 128, .b = 128, .a = 255};
            if (dbu_x_min <= xl && xl <= dbu_x_max) {
              for (int iy = loop_yl; iy < loop_yh; ++iy) {
                const int draw_y = (255 - iy);
                setPixel(image_buffer, draw_xl, draw_y, gray);
              }
            }
            if (dbu_x_min <= xh && xh <= dbu_x_max) {
              for (int iy = loop_yl; iy < loop_yh; ++iy) {
                const int draw_y = (255 - iy);
                setPixel(image_buffer, draw_xh, draw_y, gray);
              }
            }
            if (dbu_y_min <= yl && yl <= dbu_y_max) {
              for (int ix = loop_xl; ix < loop_xh; ++ix) {
                const int draw_y = (255 - draw_yl);
                setPixel(image_buffer, ix, draw_y, gray);
              }
            }
            if (dbu_y_min <= yh && yh <= dbu_y_max) {
              for (int ix = loop_xl; ix < loop_xh; ++ix) {
                const int draw_y = (255 - draw_yh);
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
              if (std::max(box_px_w, box_px_h) >= kMinInstNameBoxPx) {
                const int font_px
                    = std::clamp(static_cast<int>(box_px_min * 0.4),
                                 kMinInstNameFontPx,
                                 kMaxInstNameFontPx);
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
                  const int64_t cy = 255 - (pixel_yl + pixel_yh) / 2;

                  if (rotate) {
                    const int64_t px = cx - font_h / 2;
                    const int64_t py = cy - text_w / 2;
                    if (px > -font_h && px < kTileSizeInPixel && py > -text_w
                        && py < kTileSizeInPixel) {
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
                    if (px > -text_w && px < kTileSizeInPixel && py > -font_h
                        && py < kTileSizeInPixel) {
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
          } else {
            // Layer-specific: obstructions and pins
            if (vis.blockages) {
              for (odb::dbPolygon* poly_obs :
                   master->getPolygonObstructions()) {
                if (tech_layer && poly_obs->getTechLayer() != tech_layer) {
                  continue;
                }
                odb::Polygon poly = poly_obs->getPolygon();
                inst->getTransform().apply(poly);
                fillPolygon(image_buffer, poly, dbu_tile, scale, obs_color);
              }
              for (odb::dbBox* obs : master->getObstructions(false)) {
                if (tech_layer && obs->getTechLayer() != tech_layer) {
                  continue;
                }
                odb::Rect box = obs->getBox();
                inst->getTransform().apply(box);
                if (!box.overlaps(dbu_tile)) {
                  continue;
                }
                const odb::Rect overlap = box.intersect(dbu_tile);
                const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

                drawFilledRect(image_buffer, draw, obs_color);
              }
            }

            if (vis.inst_pins) {
              for (odb::dbMTerm* mterm : master->getMTerms()) {
                for (odb::dbMPin* mpin : mterm->getMPins()) {
                  for (odb::dbPolygon* poly_geom : mpin->getPolygonGeometry()) {
                    if (tech_layer && poly_geom->getTechLayer() != tech_layer) {
                      continue;
                    }
                    odb::Polygon poly = poly_geom->getPolygon();
                    inst->getTransform().apply(poly);
                    fillPolygon(image_buffer, poly, dbu_tile, scale, color);
                  }
                  for (odb::dbBox* geom : mpin->getGeometry(false)) {
                    if (tech_layer && geom->getTechLayer() != tech_layer) {
                      continue;
                    }
                    odb::Rect box = geom->getBox();
                    inst->getTransform().apply(box);
                    if (!box.overlaps(dbu_tile)) {
                      continue;
                    }
                    const odb::Rect overlap = box.intersect(dbu_tile);
                    const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

                    drawFilledRect(image_buffer, draw, color);
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
                    if (tech_layer && geom->getTechLayer() != tech_layer) {
                      continue;
                    }
                    odb::Rect box = geom->getBox();
                    xfm.apply(box);
                    if (!box.overlaps(dbu_tile)) {
                      continue;
                    }

                    // Skip if pin box is too small in pixels.
                    const int box_px_w = static_cast<int>(box.dx() * scale);
                    const int box_px_h = static_cast<int>(box.dy() * scale);
                    if (box_px_w < kMinItermLabelBoxPx
                        && box_px_h < kMinItermLabelBoxPx) {
                      continue;
                    }

                    const std::string name(mterm->getName());
                    const int text_w = getTextWidth(name, iterm_font);

                    // Center of pin box in pixel coords.
                    const odb::Point center = box.center();
                    const int cx = static_cast<int>(
                        (center.x() - dbu_tile.xMin()) * scale);
                    const int cy = 255
                                   - static_cast<int>(
                                       (center.y() - dbu_tile.yMin()) * scale);

                    // Rotate 90° if box is taller than wide and text overflows.
                    const bool rotate
                        = (box_px_h > box_px_w) && (text_w > box_px_w);

                    if (rotate) {
                      const int px = cx - iterm_font_h / 2;
                      const int py = cy - text_w / 2;
                      if (px > -iterm_font_h && px < kTileSizeInPixel
                          && py > -text_w && py < kTileSizeInPixel) {
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
                      if (px > -text_w && px < kTileSizeInPixel
                          && py > -iterm_font_h && py < kTileSizeInPixel) {
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
          }
        }

        // Draw routing shapes (wires, vias) and BTerm shapes on top of
        // instances
        if (!instances_only && tech_layer && (vis.routing || vis.pins)) {
          for (const auto& shape : search_->searchBoxShapes(block,
                                                            tech_layer,
                                                            dbu_x_min,
                                                            dbu_y_min,
                                                            dbu_x_max,
                                                            dbu_y_max)) {
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
                && focus_net_ids->find(net->getId()) == focus_net_ids->end()) {
              continue;
            }
            const odb::Rect& box = std::get<0>(shape);
            if (!box.overlaps(dbu_tile)) {
              continue;
            }
            const odb::Rect overlap = box.intersect(dbu_tile);
            const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

            drawFilledRect(image_buffer, draw, color);
          }
        }

        // Draw special net shapes (power/ground straps) on top of instances
        if (!instances_only && tech_layer && vis.special_nets
            && vis.srouting_segments) {
          for (const auto& shape : search_->searchSNetShapes(block,
                                                             tech_layer,
                                                             dbu_x_min,
                                                             dbu_y_min,
                                                             dbu_x_max,
                                                             dbu_y_max)) {
            odb::dbNet* snet = std::get<2>(shape);
            if (!vis.isNetVisible(snet)) {
              continue;
            }
            if (focus_net_ids && !focus_net_ids->empty()
                && focus_net_ids->find(snet->getId()) == focus_net_ids->end()) {
              continue;
            }
            const odb::Rect box = std::get<0>(shape)->getBox();
            if (!box.overlaps(dbu_tile)) {
              continue;
            }
            const odb::Polygon& poly = std::get<1>(shape);
            fillPolygon(image_buffer, poly, dbu_tile, scale, color);
          }
        }

        // Draw special net vias — decompose into individual cut boxes
        if (!instances_only && tech_layer && vis.special_nets
            && vis.srouting_vias) {
          for (const auto& shape : search_->searchSNetViaShapes(block,
                                                                tech_layer,
                                                                dbu_x_min,
                                                                dbu_y_min,
                                                                dbu_x_max,
                                                                dbu_y_max)) {
            odb::dbNet* via_net = std::get<1>(shape);
            if (!vis.isNetVisible(via_net)) {
              continue;
            }
            if (focus_net_ids && !focus_net_ids->empty()
                && focus_net_ids->find(via_net->getId())
                       == focus_net_ids->end()) {
              continue;
            }
            odb::dbSBox* sbox = std::get<0>(shape);
            std::vector<odb::dbBox*> via_boxes;
            if (auto tech_via = sbox->getTechVia()) {
              via_boxes.assign(tech_via->getBoxes().begin(),
                               tech_via->getBoxes().end());
            } else if (auto block_via = sbox->getBlockVia()) {
              via_boxes.assign(block_via->getBoxes().begin(),
                               block_via->getBoxes().end());
            }
            const odb::Point origin((sbox->xMin() + sbox->xMax()) / 2,
                                    (sbox->yMin() + sbox->yMax()) / 2);
            for (odb::dbBox* vbox : via_boxes) {
              if (vbox->getTechLayer() != tech_layer) {
                continue;
              }
              odb::Rect box = vbox->getBox();
              box.moveDelta(origin.x(), origin.y());
              if (!box.overlaps(dbu_tile)) {
                continue;
              }
              const odb::Rect overlap = box.intersect(dbu_tile);
              const odb::Rect draw = toPixels(scale, overlap, dbu_tile);
              drawFilledRect(image_buffer, draw, color);
            }
          }
        }

        // Draw via enclosures from adjacent cut layers onto this metal layer.
        // Vias are indexed by their cut layer in the search structure.  When
        // rendering a routing layer we look up the cut layers immediately above
        // and below, search for vias there, and draw only the enclosure boxes
        // that belong to the current routing layer.
        if (!instances_only && tech_layer && vis.special_nets
            && vis.srouting_vias
            && tech_layer->getType() == odb::dbTechLayerType::ROUTING) {
          odb::dbTechLayer* adj_cuts[2]
              = {tech_layer->getLowerLayer(), tech_layer->getUpperLayer()};
          for (odb::dbTechLayer* cut_layer : adj_cuts) {
            if (!cut_layer
                || cut_layer->getType() != odb::dbTechLayerType::CUT) {
              continue;
            }
            for (const auto& shape : search_->searchSNetViaShapes(block,
                                                                  cut_layer,
                                                                  dbu_x_min,
                                                                  dbu_y_min,
                                                                  dbu_x_max,
                                                                  dbu_y_max)) {
              odb::dbNet* via_net = std::get<1>(shape);
              if (!vis.isNetVisible(via_net)) {
                continue;
              }
              if (focus_net_ids && !focus_net_ids->empty()
                  && !focus_net_ids->contains(via_net->getId())) {
                continue;
              }
              odb::dbSBox* sbox = std::get<0>(shape);
              odb::dbSet<odb::dbBox> via_boxes;
              if (auto tech_via = sbox->getTechVia()) {
                via_boxes = tech_via->getBoxes();
              } else if (auto block_via = sbox->getBlockVia()) {
                via_boxes = block_via->getBoxes();
              }
              const odb::Point origin((sbox->xMin() + sbox->xMax()) / 2,
                                      (sbox->yMin() + sbox->yMax()) / 2);
              for (odb::dbBox* vbox : via_boxes) {
                if (vbox->getTechLayer() != tech_layer) {
                  continue;
                }
                odb::Rect box = vbox->getBox();
                box.moveDelta(origin.x(), origin.y());
                if (!box.overlaps(dbu_tile)) {
                  continue;
                }
                const odb::Rect overlap = box.intersect(dbu_tile);
                const odb::Rect draw = toPixels(scale, overlap, dbu_tile);
                drawFilledRect(image_buffer, draw, color);
              }
            }
          }
        }

        // Draw placement blockages (dbBlockage) on the _instances layer.
        // Diagonal white hash lines in pixel space, with period anchored in dbu
        // coordinates so the pattern is seamless across tile boundaries.
        if (instances_only && vis.placement_blockages) {
          const Color hash_color{.r = 255, .g = 255, .b = 255, .a = 180};
          constexpr int kPixelPeriod = 20;  // pixels between line centers
          constexpr int kLineWidth = 2;     // pixels wide
          for (odb::dbBlockage* blk : search_->searchBlockages(
                   block, dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max)) {
            odb::Rect box = blk->getBBox()->getBox();
            if (!box.overlaps(dbu_tile)) {
              continue;
            }
            const odb::Rect overlap = box.intersect(dbu_tile);
            const odb::Rect draw = toPixels(scale, overlap, dbu_tile);
            // Offset in absolute pixel coordinates for seamless tiling
            const int ox = (int) (dbu_x_min * scale);
            const int oy = (int) (dbu_y_min * scale);
            for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
              for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
                if (((ix + ox) + (iy + oy)) % kPixelPeriod < kLineWidth) {
                  blendPixel(image_buffer, ix, 255 - iy, hash_color);
                }
              }
            }
          }
        }

        // Draw routing obstructions (dbObstruction) on per-layer tiles.
        // Same diagonal white hash lines.
        if (!instances_only && tech_layer && vis.routing_obstructions) {
          const Color hash_color{.r = 255, .g = 255, .b = 255, .a = 180};
          constexpr int kPixelPeriod = 20;
          constexpr int kLineWidth = 2;
          for (odb::dbObstruction* obs :
               search_->searchObstructions(block,
                                           tech_layer,
                                           dbu_x_min,
                                           dbu_y_min,
                                           dbu_x_max,
                                           dbu_y_max)) {
            odb::Rect box = obs->getBBox()->getBox();
            if (!box.overlaps(dbu_tile)) {
              continue;
            }
            const odb::Rect overlap = box.intersect(dbu_tile);
            const odb::Rect draw = toPixels(scale, overlap, dbu_tile);
            const int ox = (int) (dbu_x_min * scale);
            const int oy = (int) (dbu_y_min * scale);
            for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
              for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
                if (((ix + ox) + (iy + oy)) % kPixelPeriod < kLineWidth) {
                  blendPixel(image_buffer, ix, 255 - iy, hash_color);
                }
              }
            }
          }
        }

        // Draw rows (and individual sites when zoomed in) on _instances layer.
        if (instances_only && vis.rows) {
          const Color row_color{
              .r = 60, .g = 180, .b = 60, .a = 180};  // green outlines

          // Lambda to draw a rectangle outline.
          auto draw_outline = [&](const odb::Rect& rect) {
            const odb::Rect draw = toPixels(scale, rect, dbu_tile);
            for (int ix = draw.xMin(); ix <= draw.xMax(); ++ix) {
              blendPixel(image_buffer, ix, 255 - draw.yMin(), row_color);
              blendPixel(image_buffer, ix, 255 - draw.yMax(), row_color);
            }
            for (int iy = draw.yMin(); iy <= draw.yMax(); ++iy) {
              blendPixel(image_buffer, draw.xMin(), 255 - iy, row_color);
              blendPixel(image_buffer, draw.xMax(), 255 - iy, row_color);
            }
          };

          for (const auto& [row_rect, row] : search_->searchRows(
                   block, dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max)) {
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
              if (site_w_px >= 5) {
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
          if (grid) {
            Color track_color = color;
            track_color.a = 150;
            const bool is_horizontal
                = tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;

            // X-direction tracks (vertical lines on screen)
            // Preferred for vertical layers, non-preferred for horizontal
            // layers
            if ((!is_horizontal && vis.tracks_pref)
                || (is_horizontal && vis.tracks_non_pref)) {
              std::vector<int> x_grid;
              grid->getGridX(x_grid);
              debugPrint(logger_,
                         utl::WEB,
                         "tile",
                         1,
                         "  x_tracks: count={} tile=[{},{},{},{}]",
                         x_grid.size(),
                         dbu_x_min,
                         dbu_y_min,
                         dbu_x_max,
                         dbu_y_max);
              for (int tx : x_grid) {
                if (tx < dbu_x_min || tx > dbu_x_max) {
                  continue;
                }
                const int px = static_cast<int>((tx - dbu_x_min) * scale);
                if (px >= 0 && px < kTileSizeInPixel) {
                  for (int py = 0; py < kTileSizeInPixel; ++py) {
                    blendPixel(image_buffer, px, py, track_color);
                  }
                }
              }
            }

            // Y-direction tracks (horizontal lines on screen)
            // Preferred for horizontal layers, non-preferred for vertical
            // layers
            if ((is_horizontal && vis.tracks_pref)
                || (!is_horizontal && vis.tracks_non_pref)) {
              std::vector<int> y_grid;
              grid->getGridY(y_grid);
              debugPrint(logger_,
                         utl::WEB,
                         "tile",
                         1,
                         "  y_tracks: count={}",
                         y_grid.size());
              for (int ty : y_grid) {
                if (ty < dbu_y_min || ty > dbu_y_max) {
                  continue;
                }
                const int py = 255 - static_cast<int>((ty - dbu_y_min) * scale);
                if (py >= 0 && py < kTileSizeInPixel) {
                  for (int px = 0; px < kTileSizeInPixel; ++px) {
                    blendPixel(image_buffer, px, py, track_color);
                  }
                }
              }
            }
          }
        }

      }  // end if (!modules_layer && !pins_layer)

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
        for (int py_w = 0; py_w < kTileSizeInPixel; ++py_w) {
          for (int px_w = 0; px_w < kTileSizeInPixel; ++px_w) {
            // World pixel center → world DBU.
            odb::Point pt(
                std::lround(dbu_x_min_world + (px_w + 0.5) / scale),
                std::lround(dbu_y_min_world
                            + (kTileSizeInPixel - 1 - py_w + 0.5) / scale));
            // World DBU → local DBU.
            inv_xfm.apply(pt);
            // Local DBU → local pixel.
            const int px_l = std::floor((pt.x() - dbu_x_min) * scale);
            const int py_l = kTileSizeInPixel - 1
                             - std::floor((pt.y() - dbu_y_min) * scale);
            if (px_l < 0 || px_l >= kTileSizeInPixel || py_l < 0
                || py_l >= kTileSizeInPixel) {
              continue;
            }
            const int src_idx = (py_l * kTileSizeInPixel + px_l) * 4;
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
            blendPixel(world_image_buffer, px_w, py_w, src_color);
          }
        }
      }
    }  // end per-chiplet for-loop

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
      drawHighlight(world_image_buffer,
                    highlight_rects,
                    highlight_polys,
                    dbu_tile_world,
                    scale);
    }
    if (!colored_rects.empty()) {
      drawColoredHighlight(
          world_image_buffer, colored_rects, layer, dbu_tile_world, scale);
    }
    if (!flight_lines.empty()) {
      drawFlightLines(world_image_buffer, flight_lines, dbu_tile_world, scale);
    }
    if (route_guide_net_ids && !route_guide_net_ids->empty()
        && world_layer_found) {
      drawRouteGuides(world_image_buffer,
                      *route_guide_net_ids,
                      layer,
                      world_color,
                      dbu_tile_world,
                      scale);
    }
    if (vis.debug_renderers) {
      // The callback (installed by WebServer at startup) decides
      // whether to draw (honoring pause/live semantics) and handles
      // the gui::Gui::get() access itself.  Keeping Gui:: references
      // out of tile_generator means test executables that link libweb
      // don't transitively need gui.a / ord.a.
      drawRendererOverlay(
          world_image_buffer, dbu_tile_world, scale, vis.debug_live);
    }
  }

  if (vis.debug) {
    drawDebugOverlay(world_image_buffer, z, x, y);
  }

  return world_image_buffer;
}

std::vector<unsigned char> TileGenerator::generateHeatMapTile(
    gui::HeatMapDataSource& source,
    const int z,
    const int x,
    int y) const
{
  constexpr int kBufferSize = kTileSizeInPixel * kTileSizeInPixel * 4;
  std::vector<unsigned char> image_buffer(kBufferSize, 0);

  const double num_tiles_at_zoom = pow(2, z);
  if (x < 0 || y < 0 || x >= num_tiles_at_zoom || y >= num_tiles_at_zoom) {
    return {};
  }

  y = num_tiles_at_zoom - 1 - y;
  const odb::Rect hm_bounds = getBounds();
  const double tile_dbu_size = hm_bounds.maxDXDY() / num_tiles_at_zoom;
  const int dbu_x_min = hm_bounds.xMin() + x * tile_dbu_size;
  const int dbu_y_min = hm_bounds.yMin() + y * tile_dbu_size;
  const int dbu_x_max = hm_bounds.xMin() + std::ceil((x + 1) * tile_dbu_size);
  const int dbu_y_max = hm_bounds.yMin() + std::ceil((y + 1) * tile_dbu_size);
  const odb::Rect dbu_tile(dbu_x_min, dbu_y_min, dbu_x_max, dbu_y_max);
  const double scale = kTileSizeInPixel / tile_dbu_size;
  constexpr double kTextRectMargin = 0.8;
  constexpr int kHeatmapFontHeight = 14;
  const auto heatmap_font = fontAtlasGetFont(kHeatmapFontHeight);
  const Color text_color{.r = 255, .g = 255, .b = 255, .a = 255};

  for (const auto& map_point : source.getVisibleMap(dbu_tile, scale)) {
    if (!map_point.rect.overlaps(dbu_tile)) {
      continue;
    }
    const odb::Rect overlap = map_point.rect.intersect(dbu_tile);
    const odb::Rect draw = toPixels(scale, overlap, dbu_tile);
    const Color color{.r = static_cast<uint8_t>(map_point.color.r),
                      .g = static_cast<uint8_t>(map_point.color.g),
                      .b = static_cast<uint8_t>(map_point.color.b),
                      .a = static_cast<uint8_t>(map_point.color.a)};

    for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        blendPixel(image_buffer, ix, 255 - iy, color);
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
    if (center_x < dbu_tile.xMin() || center_x >= dbu_tile.xMax()
        || center_y < dbu_tile.yMin() || center_y >= dbu_tile.yMax()) {
      continue;
    }

    const int pixel_x = std::lround((center_x - dbu_tile.xMin()) * scale);
    const int pixel_y = 255 - std::lround((center_y - dbu_tile.yMin()) * scale);
    drawText(image_buffer,
             pixel_x - text_width / 2,
             pixel_y - text_height / 2,
             text,
             heatmap_font,
             text_color);
  }

  std::vector<unsigned char> png_data;
  unsigned error = lodepng::encode(
      png_data, image_buffer, kTileSizeInPixel, kTileSizeInPixel);
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

void TileGenerator::saveImage(const std::string& filename,
                              const odb::Rect& region,
                              const int width_px,
                              const double dbu_per_pixel,
                              const TileVisibility& vis) const
{
  odb::dbBlock* block = getBlock();
  if (!block) {
    logger_->error(utl::WEB, 20, "No design loaded.");
    return;
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
    return;
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

  // Layers to render (bottom to top): _instances, tech layers, _pins.
  std::vector<std::string> layers_to_render;
  layers_to_render.emplace_back("_instances");
  for (const auto& name : getLayers()) {
    layers_to_render.push_back(name);
  }
  if (vis.pins) {
    layers_to_render.emplace_back("_pins");
  }

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
  // to target scale).
  std::vector<unsigned char> final_buf(4UL * final_w * final_h, 0);
  for (int fy = 0; fy < final_h; ++fy) {
    for (int fx = 0; fx < final_w; ++fx) {
      // Map final pixel to tile-span pixel.
      const int sx = crop_x + static_cast<int>(fx * tile_scale / scale);
      const int sy = crop_y + static_cast<int>(fy * tile_scale / scale);
      if (sx >= 0 && sx < tile_span_w && sy >= 0 && sy < tile_span_h) {
        const int src_idx = (sy * tile_span_w + sx) * 4;
        const int dst_idx = (fy * final_w + fx) * 4;
        std::memcpy(&final_buf[dst_idx], &output[src_idx], 4);
      }
    }
  }

  // Encode to PNG and save.
  std::vector<unsigned char> png_data;
  const unsigned error = lodepng::encode(png_data, final_buf, final_w, final_h);
  if (error) {
    logger_->error(
        utl::WEB, 23, "PNG encode error: {}", lodepng_error_text(error));
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
  const int last = kTileSizeInPixel - 1;

  // Draw 1-pixel yellow border
  for (int i = 0; i < kTileSizeInPixel; ++i) {
    setPixel(image, i, 0, yellow);
    setPixel(image, i, last, yellow);
    setPixel(image, 0, i, yellow);
    setPixel(image, last, i, yellow);
  }

  // Build the label string "z=<zoom> <x>/<y>"
  const std::string label = "z=" + std::to_string(z) + " " + std::to_string(x)
                            + "/" + std::to_string(y);

  drawText(image, 4, 4, label, fontAtlasGetFont(20), yellow);
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

inline int toPxX(int dbu_x, const odb::Rect& tile, double scale)
{
  return static_cast<int>((dbu_x - tile.xMin()) * scale);
}

// Y is flipped: DBU grows up, pixel rows grow down.
inline int toPxY(int dbu_y, const odb::Rect& tile, double scale)
{
  return 255 - static_cast<int>((dbu_y - tile.yMin()) * scale);
}

}  // namespace

/* static */
void TileGenerator::setDebugOverlayCallback(DebugOverlayCallback callback)
{
  getDebugOverlayCallback() = std::move(callback);
}

void TileGenerator::drawRendererOverlay(std::vector<unsigned char>& image,
                                        const odb::Rect& dbu_tile,
                                        const double scale,
                                        const bool debug_live) const
{
  auto& callback = getDebugOverlayCallback();
  if (!callback) {
    return;
  }
  callback(image, dbu_tile, scale, debug_live);
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
                                           const odb::Rect& dbu_tile,
                                           const double scale) const
{
  {
    for (const DrawOp& op : ops) {
      if (const auto* r = std::get_if<DrawRectOp>(&op)) {
        const odb::Rect px = toPixels(scale, r->rect, dbu_tile);
        // Fill first (if the brush paints), outline on top.
        if (r->brush.style != gui::Painter::Brush::kNone
            && r->brush.color.a > 0) {
          const Color fill = toTileColor(r->brush.color);
          for (int iy = px.yMin(); iy < px.yMax(); ++iy) {
            for (int ix = px.xMin(); ix < px.xMax(); ++ix) {
              blendPixel(image, ix, 255 - iy, fill);
            }
          }
        }
        if (r->pen.color.a > 0 && px.dx() >= 1 && px.dy() >= 1) {
          const Color pen = toTileColor(r->pen.color);
          const int w = penWidthPx(r->pen, scale);
          const int x0 = px.xMin();
          const int x1 = px.xMax() - 1;
          const int y0 = 255 - px.yMin();
          const int y1 = 255 - (px.yMax() - 1);
          drawLine(image, x0, y0, x1, y0, pen, w);
          drawLine(image, x1, y0, x1, y1, pen, w);
          drawLine(image, x1, y1, x0, y1, pen, w);
          drawLine(image, x0, y1, x0, y0, pen, w);
        }
      } else if (const auto* l = std::get_if<DrawLineOp>(&op)) {
        if (l->pen.color.a == 0) {
          continue;
        }
        const int x0 = toPxX(l->p1.x(), dbu_tile, scale);
        const int y0 = toPxY(l->p1.y(), dbu_tile, scale);
        const int x1 = toPxX(l->p2.x(), dbu_tile, scale);
        const int y1 = toPxY(l->p2.y(), dbu_tile, scale);
        drawLine(image,
                 x0,
                 y0,
                 x1,
                 y1,
                 toTileColor(l->pen.color),
                 penWidthPx(l->pen, scale));
      } else if (const auto* c = std::get_if<DrawCircleOp>(&op)) {
        // Simple midpoint circle (outline only).
        const int cx = toPxX(c->cx, dbu_tile, scale);
        const int cy = toPxY(c->cy, dbu_tile, scale);
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
        const int cx = toPxX(xop->cx, dbu_tile, scale);
        const int cy = toPxY(xop->cy, dbu_tile, scale);
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
                      dbu_tile,
                      scale,
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
            drawLine(image,
                     toPxX(a.x(), dbu_tile, scale),
                     toPxY(a.y(), dbu_tile, scale),
                     toPxX(b.x(), dbu_tile, scale),
                     toPxY(b.y(), dbu_tile, scale),
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
        int ax = toPxX(s->x, dbu_tile, scale);
        int ay = toPxY(s->y, dbu_tile, scale);
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
                               const Color& c)
{
  if (x < 0 || x >= kTileSizeInPixel || y < 0 || y >= kTileSizeInPixel) {
    return;
  }
  const int i = (y * kTileSizeInPixel + x) * 4;
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
                                   const Color& color) const
{
  for (int iy = rect.yMin(); iy < rect.yMax(); ++iy) {
    for (int ix = rect.xMin(); ix < rect.xMax(); ++ix) {
      const int draw_y = (255 - iy);
      setPixel(buffer, ix, draw_y, color);
    }
  }
}

void TileGenerator::drawHighlight(std::vector<unsigned char>& image,
                                  const std::vector<odb::Rect>& rects,
                                  const std::vector<odb::Polygon>& polys,
                                  const odb::Rect& dbu_tile,
                                  const double scale) const
{
  const Color fill{.r = 255, .g = 255, .b = 0, .a = 30};
  const Color border{.r = 255, .g = 255, .b = 0, .a = 255};

  for (const odb::Rect& rect : rects) {
    if (!dbu_tile.overlaps(rect)) {
      continue;
    }
    const odb::Rect overlap = rect.intersect(dbu_tile);
    const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

    // Semi-transparent yellow fill
    for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        blendPixel(image, ix, 255 - iy, fill);
      }
    }

    // Solid yellow border (only where edge is within the tile)
    const odb::Rect full_draw = toPixels(scale, rect, dbu_tile);
    if (full_draw.xMin() >= 0 && full_draw.xMin() < kTileSizeInPixel) {
      for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
        setPixel(image, full_draw.xMin(), 255 - iy, border);
      }
    }
    if (full_draw.xMax() > 0 && full_draw.xMax() <= kTileSizeInPixel) {
      const int rx = full_draw.xMax() - 1;
      for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
        setPixel(image, rx, 255 - iy, border);
      }
    }
    if (full_draw.yMin() >= 0 && full_draw.yMin() < kTileSizeInPixel) {
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        setPixel(image, ix, 255 - full_draw.yMin(), border);
      }
    }
    if (full_draw.yMax() > 0 && full_draw.yMax() <= kTileSizeInPixel) {
      const int ty = full_draw.yMax() - 1;
      for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
        setPixel(image, ix, 255 - ty, border);
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
    fillPolygon(image, poly, dbu_tile, scale, fill, /*blend=*/true);

    // Solid yellow border — draw each edge
    const auto& points = poly.getPoints();
    const int n = static_cast<int>(points.size());
    for (int i = 0; i < n - 1; ++i) {
      const int px0
          = static_cast<int>((points[i].x() - dbu_tile.xMin()) * scale);
      const int py0
          = 255 - static_cast<int>((points[i].y() - dbu_tile.yMin()) * scale);
      const int px1
          = static_cast<int>((points[i + 1].x() - dbu_tile.xMin()) * scale);
      const int py1
          = 255
            - static_cast<int>((points[i + 1].y() - dbu_tile.yMin()) * scale);
      drawLine(image, px0, py0, px1, py1, border);
    }
  }
}

void TileGenerator::drawColoredHighlight(std::vector<unsigned char>& image,
                                         const std::vector<ColoredRect>& rects,
                                         const std::string& current_layer,
                                         const odb::Rect& dbu_tile,
                                         const double scale) const
{
  const bool draw_all = current_layer.empty() || current_layer == "_instances";
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
    const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

    if (cr.filled) {
      // DRC marker style: semi-transparent filled rect with solid outline.
      // Matches the Qt GUI's DRCRenderer (white pen + white-alpha brush).

      // Fill interior
      const int pxl = std::max(0, draw.xMin());
      const int pyl = std::max(0, draw.yMin());
      const int pxh = std::min(255, draw.xMax());
      const int pyh = std::min(255, draw.yMax());
      for (int iy = pyl; iy <= pyh; ++iy) {
        for (int ix = pxl; ix <= pxh; ++ix) {
          blendPixel(image, ix, 255 - iy, cr.color);
        }
      }

      // Solid outline
      Color outline = cr.color;
      outline.a = 255;
      // Bottom edge
      for (int ix = pxl; ix <= pxh; ++ix) {
        blendPixel(image, ix, 255 - pyl, outline);
      }
      // Top edge
      for (int ix = pxl; ix <= pxh; ++ix) {
        blendPixel(image, ix, 255 - pyh, outline);
      }
      // Left edge
      for (int iy = pyl; iy <= pyh; ++iy) {
        blendPixel(image, pxl, 255 - iy, outline);
      }
      // Right edge
      for (int iy = pyl; iy <= pyh; ++iy) {
        blendPixel(image, pxh, 255 - iy, outline);
      }
    } else {
      // Timing path style: centerline through the shape.
      const int cx = (draw.xMin() + draw.xMax()) / 2;
      const int cy = (draw.yMin() + draw.yMax()) / 2;

      Color line_color = cr.color;
      line_color.a = 255;

      if (draw.dx() >= draw.dy()) {
        drawLine(
            image, draw.xMin(), 255 - cy, draw.xMax(), 255 - cy, line_color);
      } else {
        drawLine(
            image, cx, 255 - draw.yMin(), cx, 255 - draw.yMax(), line_color);
      }
    }
  }
}

/* static */
void TileGenerator::drawLine(std::vector<unsigned char>& image,
                             int x0,
                             int y0,
                             int x1,
                             int y1,
                             const Color& c,
                             int width)
{
  // Bresenham's line algorithm
  int dx = std::abs(x1 - x0);
  int dy = std::abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = dx - dy;
  const int r = (width - 1) / 2;

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
                                    const odb::Rect& dbu_tile,
                                    const double scale) const
{
  for (const auto& fl : lines) {
    // Convert DBU to pixel coordinates
    int px0 = static_cast<int>((fl.p1.x() - dbu_tile.xMin()) * scale);
    int py0 = 255 - static_cast<int>((fl.p1.y() - dbu_tile.yMin()) * scale);
    int px1 = static_cast<int>((fl.p2.x() - dbu_tile.xMin()) * scale);
    int py1 = 255 - static_cast<int>((fl.p2.y() - dbu_tile.yMin()) * scale);

    // Rough bounding-box check: skip if line can't cross this tile
    int lx0 = std::min(px0, px1), lx1 = std::max(px0, px1);
    int ly0 = std::min(py0, py1), ly1 = std::max(py0, py1);
    if (lx1 < 0 || lx0 >= kTileSizeInPixel || ly1 < 0
        || ly0 >= kTileSizeInPixel) {
      continue;
    }

    Color c = fl.color;
    c.a = 220;
    drawLine(image, px0, py0, px1, py1, c);
  }
}

void TileGenerator::drawRouteGuides(std::vector<unsigned char>& image,
                                    const std::set<uint32_t>& net_ids,
                                    const std::string& layer,
                                    const Color& layer_color,
                                    const odb::Rect& dbu_tile,
                                    const double scale) const
{
  odb::dbBlock* block = getBlock();
  if (!block) {
    return;
  }

  const Color fill{
      .r = layer_color.r, .g = layer_color.g, .b = layer_color.b, .a = 50};
  const Color border{
      .r = layer_color.r, .g = layer_color.g, .b = layer_color.b, .a = 180};

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
      const odb::Rect draw = toPixels(scale, overlap, dbu_tile);

      // Semi-transparent fill
      for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
        for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
          blendPixel(image, ix, 255 - iy, fill);
        }
      }

      // Border (only where guide edge is within this tile)
      const odb::Rect full_draw = toPixels(scale, box, dbu_tile);
      if (full_draw.xMin() >= 0 && full_draw.xMin() < kTileSizeInPixel) {
        for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
          blendPixel(image, full_draw.xMin(), 255 - iy, border);
        }
      }
      if (full_draw.xMax() > 0 && full_draw.xMax() <= kTileSizeInPixel) {
        const int rx = full_draw.xMax() - 1;
        for (int iy = draw.yMin(); iy < draw.yMax(); ++iy) {
          blendPixel(image, rx, 255 - iy, border);
        }
      }
      if (full_draw.yMin() >= 0 && full_draw.yMin() < kTileSizeInPixel) {
        for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
          blendPixel(image, ix, 255 - full_draw.yMin(), border);
        }
      }
      if (full_draw.yMax() > 0 && full_draw.yMax() <= kTileSizeInPixel) {
        const int ty = full_draw.yMax() - 1;
        for (int ix = draw.xMin(); ix < draw.xMax(); ++ix) {
          blendPixel(image, ix, 255 - ty, border);
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

  boost::json::array layers_arr;
  boost::json::array backside_layers_arr;
  if (node.chip) {
    if (odb::dbTech* tech = node.chip->getTech()) {
      const auto& layer_colors = gen.getLayerColorMap(tech);
      for (odb::dbTechLayer* layer : tech->getLayers()) {
        if (layer->getRoutingLevel() > 0
            || layer->getType() == odb::dbTechLayerType::CUT) {
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
          if (layer->isBackside()) {
            backside_layers_arr.emplace_back(std::move(layer_obj));
          } else {
            layers_arr.emplace_back(std::move(layer_obj));
          }
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
  if (!backside_layers_arr.empty()) {
    boost::json::object backside_node;
    backside_node["name"] = "Backside";
    backside_node["type"] = "category";
    backside_node["layers"] = std::move(backside_layers_arr);
    backside_node["instances"] = boost::json::array{};
    instances_arr.emplace_back(std::move(backside_node));
  }
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

  boost::json::array sites;
  for (const auto& name : gen.getSites()) {
    sites.emplace_back(name);
  }
  out["sites"] = std::move(sites);

  out["has_liberty"] = gen.hasSta();
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
