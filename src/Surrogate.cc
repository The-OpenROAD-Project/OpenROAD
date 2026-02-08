// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "ord/Surrogate.hh"

#include <algorithm>
#include <array>
#include <atomic>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "ord/OpenRoad.hh"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/MinMax.hh"
#include "sta/Sdc.hh"
#include "sta/Units.hh"
#include "tcl.h"
#include "tclDecls.h"
#include "utl/Logger.h"

namespace ord {
namespace {

constexpr utl::ToolId kTool = utl::ORD;

template <typename T>
T clamp(const T& v, const T& lo, const T& hi)
{
  return std::min(hi, std::max(lo, v));
}

std::optional<double> readEnvDouble(const char* name)
{
  const char* value = std::getenv(name);
  if (!value || value[0] == '\0') {
    return std::nullopt;
  }
  char* end = nullptr;
  const double parsed = std::strtod(value, &end);
  if (end == value || !std::isfinite(parsed)) {
    return std::nullopt;
  }
  return parsed;
}

std::string readTextFile(const std::string& path)
{
  std::ifstream f(path);
  if (!f.good()) {
    throw std::runtime_error("Could not open file: " + path);
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

boost::json::value loadJsonFile(const std::string& path)
{
  const std::string text = readTextFile(path);
  try {
    return boost::json::parse(text);
  } catch (const std::exception& e) {
    throw std::runtime_error("Failed to parse JSON: " + path + " (" + e.what()
                             + ")");
  }
}

std::optional<double> jsonToDouble(const boost::json::value& v)
{
  if (v.is_double()) {
    return v.as_double();
  }
  if (v.is_int64()) {
    return static_cast<double>(v.as_int64());
  }
  if (v.is_uint64()) {
    return static_cast<double>(v.as_uint64());
  }
  if (v.is_bool()) {
    return v.as_bool() ? 1.0 : 0.0;
  }
  return std::nullopt;
}

std::optional<double> getObjDouble(const boost::json::object& obj,
                                   const std::string& key)
{
  const auto it = obj.find(key);
  if (it == obj.end()) {
    return std::nullopt;
  }
  return jsonToDouble(it->value());
}

struct DesignStats
{
  int dbu_per_micron = 0;
  std::int64_t num_insts = 0;
  std::int64_t num_core_insts = 0;
  std::int64_t num_macros = 0;
  std::int64_t num_nets = 0;
  std::int64_t num_pins = 0;
  std::int64_t num_connections = 0;
  std::int64_t sum_net_degree = 0;
  double total_core_area_um2 = 0.0;
  double total_macro_area_um2 = 0.0;
  double core_bbox_area_um2 = 0.0;
  double core_bbox_aspect = 1.0;
  double inst_utilization = 0.0;
  std::int64_t num_routing_layers = 0;
  std::int64_t used_routing_layers = 0;
  double hpwl_factor_sum = 0.0;
  std::int64_t nets_degree_ge2 = 0;
  double hpwl_factor_p90 = 0.0;
  double hpwl_factor_p95 = 0.0;
  double hpwl_factor_p99 = 0.0;
};

DesignStats computeDesignStats(odb::dbBlock* block)
{
  DesignStats s;
  if (!block) {
    return s;
  }
  s.dbu_per_micron = block->getDbUnitsPerMicron();
  const double scale
      = (s.dbu_per_micron > 0) ? static_cast<double>(s.dbu_per_micron) : 1.0;

  s.num_pins = static_cast<std::int64_t>(block->getBTerms().size());
  for (odb::dbInst* inst : block->getInsts()) {
    if (!inst) {
      continue;
    }
    odb::dbMaster* master = inst->getMaster();
    if (!master) {
      continue;
    }
    s.num_pins += static_cast<std::int64_t>(inst->getITerms().size());
    const std::int64_t area_dbu2
        = static_cast<std::int64_t>(master->getWidth())
          * static_cast<std::int64_t>(master->getHeight());
    const double area_um2 = static_cast<double>(area_dbu2) / (scale * scale);
    if (!(area_um2 > 0.0) || !std::isfinite(area_um2)) {
      continue;
    }
    if (inst->isBlock()) {
      s.total_macro_area_um2 += area_um2;
      s.num_macros++;
    } else if (master->isCore()) {
      s.total_core_area_um2 += area_um2;
      s.num_core_insts++;
    } else {
      s.total_core_area_um2 += area_um2;
      s.num_core_insts++;
    }
    s.num_insts++;
  }

  std::vector<double> hpwl_factors;
  hpwl_factors.reserve(block->getNets().size());
  for (odb::dbNet* net : block->getNets()) {
    if (!net || net->isSpecial()) {
      continue;
    }
    std::int64_t deg = 0;
    for (odb::dbITerm* it : net->getITerms()) {
      if (it) {
        deg++;
      }
    }
    for (odb::dbBTerm* bt : net->getBTerms()) {
      if (bt) {
        deg++;
      }
    }
    if (deg <= 0) {
      continue;
    }
    s.num_nets++;
    s.sum_net_degree += deg;
    s.num_connections += deg;

    if (deg >= 2) {
      const double d = static_cast<double>(deg);
      const double f = (d - 1.0) / (d + 1.0);
      s.hpwl_factor_sum += f;
      s.nets_degree_ge2++;
      hpwl_factors.push_back(f);
    }
  }

  if (!hpwl_factors.empty()) {
    const auto quantile = [&](const double q) -> double {
      if (!(q >= 0.0 && q <= 1.0)) {
        return 0.0;
      }
      const std::size_t idx
          = static_cast<std::size_t>(std::floor(q * (hpwl_factors.size() - 1)));
      std::nth_element(hpwl_factors.begin(),
                       hpwl_factors.begin() + static_cast<std::ptrdiff_t>(idx),
                       hpwl_factors.end());
      return hpwl_factors[idx];
    };
    s.hpwl_factor_p90 = quantile(0.90);
    s.hpwl_factor_p95 = quantile(0.95);
    s.hpwl_factor_p99 = quantile(0.99);
  }

  const odb::Rect core = block->getCoreArea();
  if (core.dx() > 0 && core.dy() > 0) {
    const double w_um = static_cast<double>(core.dx()) / scale;
    const double h_um = static_cast<double>(core.dy()) / scale;
    if (w_um > 0.0 && h_um > 0.0) {
      s.core_bbox_area_um2 = w_um * h_um;
      s.core_bbox_aspect = clamp(w_um / h_um, 0.2, 5.0);
      s.inst_utilization = (s.core_bbox_area_um2 > 0.0)
                               ? (s.total_core_area_um2 / s.core_bbox_area_um2)
                               : 0.0;
    }
  }

  odb::dbTech* tech = block->getTech();
  if (tech) {
    std::int64_t layers = 0;
    for (odb::dbTechLayer* layer : tech->getLayers()) {
      if (!layer || layer->getType() != odb::dbTechLayerType::ROUTING) {
        continue;
      }
      const int level = layer->getRoutingLevel();
      if (level <= 0) {
        continue;
      }
      layers = std::max<std::int64_t>(layers, level);
    }
    s.num_routing_layers = layers;
  }
  return s;
}

std::optional<std::array<double, 4>> parseFourDoubles(const std::string& s)
{
  std::array<double, 4> out{};
  int idx = 0;
  const char* p = s.c_str();
  while (*p != '\0' && idx < 4) {
    while (*p != '\0'
           && (std::isspace(static_cast<unsigned char>(*p)) || *p == '{'
               || *p == '}' || *p == ',')) {
      p++;
    }
    if (*p == '\0') {
      break;
    }
    char* end = nullptr;
    const double v = std::strtod(p, &end);
    if (end == p || !std::isfinite(v)) {
      break;
    }
    out[static_cast<std::size_t>(idx++)] = v;
    p = end;
  }
  if (idx != 4) {
    return std::nullopt;
  }
  return out;
}

struct WireRC
{
  double r_per_um = 0.0;  // ohm/um
  double c_per_um = 0.0;  // F/um
};

WireRC estimateWireRC(odb::dbTech* tech, const int dbu_per_micron)
{
  WireRC rc;
  if (!tech || dbu_per_micron <= 0) {
    return rc;
  }

  double r_sum = 0.0;
  double c_sum = 0.0;
  int count = 0;
  for (odb::dbTechLayer* layer : tech->getLayers()) {
    if (!layer || layer->getType() != odb::dbTechLayerType::ROUTING) {
      continue;
    }
    const int routing_level = layer->getRoutingLevel();
    if (routing_level <= 0) {
      continue;
    }

    const double w_um
        = static_cast<double>(layer->getMinWidth()) / dbu_per_micron;
    if (w_um <= 0.0) {
      continue;
    }

    const double r_sheet = layer->getResistance();        // ohm/sq
    const double cap_area = layer->getCapacitance();      // pF/um^2
    const double cap_edge = layer->getEdgeCapacitance();  // pF/um
    const double r_per_um = (r_sheet > 0.0) ? (r_sheet / w_um) : 0.0;
    const double c_per_um_pf = std::max(0.0, cap_area * w_um + cap_edge);
    const double c_per_um = c_per_um_pf * 1e-12;

    if (!(r_per_um > 0.0) || !(c_per_um > 0.0)) {
      continue;
    }
    r_sum += r_per_um;
    c_sum += c_per_um;
    count++;
  }

  if (count > 0) {
    rc.r_per_um = r_sum / count;
    rc.c_per_um = c_sum / count;
  }
  return rc;
}

std::optional<int> findRoutingLevelByName(odb::dbTech* tech,
                                          const std::string& name)
{
  if (!tech) {
    return std::nullopt;
  }
  for (odb::dbTechLayer* layer : tech->getLayers()) {
    if (!layer || layer->getType() != odb::dbTechLayerType::ROUTING) {
      continue;
    }
    if (layer->getName() == name) {
      const int level = layer->getRoutingLevel();
      return level > 0 ? std::optional<int>(level) : std::nullopt;
    }
  }
  return std::nullopt;
}

std::int64_t estimateUsedRoutingLayers(odb::dbTech* tech,
                                       const std::int64_t fallback_layers)
{
  const char* min_name = std::getenv("MIN_ROUTING_LAYER");
  const char* max_name = std::getenv("MAX_ROUTING_LAYER");
  if (!min_name || !max_name || !tech) {
    return std::max<std::int64_t>(1, fallback_layers);
  }
  const auto min_level = findRoutingLevelByName(tech, min_name);
  const auto max_level = findRoutingLevelByName(tech, max_name);
  if (!min_level || !max_level || *min_level <= 0 || *max_level <= 0
      || *max_level < *min_level) {
    return std::max<std::int64_t>(1, fallback_layers);
  }
  const std::int64_t used
      = static_cast<std::int64_t>(*max_level) - *min_level + 1;
  return std::max<std::int64_t>(
      1, std::min(used, std::max<std::int64_t>(1, fallback_layers)));
}

double computeAvgCellWidthSites(OpenRoad* openroad, const DesignStats& stats)
{
  if (!openroad || stats.dbu_per_micron <= 0 || stats.num_core_insts <= 0
      || !(stats.total_core_area_um2 > 0.0)) {
    return 4.0;
  }

  const char* site_name = std::getenv("PLACE_SITE");
  if (!site_name || site_name[0] == '\0') {
    return 4.0;
  }

  odb::dbDatabase* db = openroad->getDb();
  if (!db) {
    return 4.0;
  }

  odb::dbSite* site = nullptr;
  for (odb::dbLib* lib : db->getLibs()) {
    if (!lib) {
      continue;
    }
    site = lib->findSite(site_name);
    if (site) {
      break;
    }
  }
  if (!site) {
    return 4.0;
  }

  const double site_w_um
      = static_cast<double>(site->getWidth()) / stats.dbu_per_micron;
  const double site_h_um
      = static_cast<double>(site->getHeight()) / stats.dbu_per_micron;
  if (!(site_w_um > 0.0) || !(site_h_um > 0.0)) {
    return 4.0;
  }

  const double avg_cell_area_um2
      = stats.total_core_area_um2 / static_cast<double>(stats.num_core_insts);
  if (!(avg_cell_area_um2 > 0.0)) {
    return 4.0;
  }

  const double avg_cell_w_um = avg_cell_area_um2 / site_h_um;
  const double avg_cell_w_sites = avg_cell_w_um / site_w_um;
  return clamp(avg_cell_w_sites, 1.0, 20.0);
}

struct StaSnapshot
{
  double clock_period_user = 0.0;
  double time_scale = 1.0;  // seconds per user unit
  bool worst_path_valid = false;
  double worst_path_delay_user = 0.0;
};

StaSnapshot getStaSnapshot(OpenRoad* openroad)
{
  StaSnapshot snap;
  if (!openroad) {
    return snap;
  }
  sta::dbSta* sta = openroad->getSta();
  if (!sta) {
    return snap;
  }
  sta::Sdc* sdc = sta->sdc();
  if (!sdc) {
    return snap;
  }
  sta::Clock* clk0 = nullptr;
  for (sta::Clock* clk : sdc->clks()) {
    if (clk && clk->period() > 0.0f) {
      clk0 = clk;
      break;
    }
  }
  if (!clk0) {
    return snap;
  }

  sta::Units* units = sta->units();
  if (!units || !units->timeUnit()) {
    return snap;
  }
  snap.time_scale = units->timeUnit()->scale();
  if (!(snap.time_scale > 0.0) || !std::isfinite(snap.time_scale)) {
    snap.time_scale = 1.0;
  }

  const double period_s = clk0->period();
  snap.clock_period_user = units->timeUnit()->staToUser(period_s);

  sta->updateTiming(true);
  const sta::Slack ws = sta->worstSlack(sta::MinMax::max());
  if (!(ws < 1e20) || !std::isfinite(ws)) {
    return snap;
  }
  const double delay_s = period_s - ws;
  if (!(delay_s > 0.0) || !std::isfinite(delay_s)) {
    return snap;
  }
  snap.worst_path_valid = true;
  snap.worst_path_delay_user = units->timeUnit()->staToUser(delay_s);
  return snap;
}

struct Calibration
{
  double builtin_length_scale = 1.0;
  double builtin_timing_scale = 1.0;
  double builtin_ref_clock_user = 0.0;
};

void applyCalibrationOverridesFromEnv(Calibration& c)
{
  if (const auto v = readEnvDouble("SURROGATE_BUILTIN_LENGTH_SCALE")) {
    c.builtin_length_scale = *v;
  }
  if (const auto v = readEnvDouble("SURROGATE_BUILTIN_TIMING_SCALE")) {
    c.builtin_timing_scale = *v;
  }
  if (const auto v = readEnvDouble("SURROGATE_BUILTIN_REF_CLOCK_USER")) {
    c.builtin_ref_clock_user = *v;
  }
}

Calibration loadCalibrationFromEnv()
{
  Calibration c;
  applyCalibrationOverridesFromEnv(c);
  return c;
}

struct BaselineMetrics
{
  std::optional<double> baseline_ecp_user;
  std::optional<double> baseline_routed_wl_um;
  std::optional<double> baseline_power_total_user;
  std::optional<double> baseline_power_leak_user;
  std::optional<double> baseline_power_internal_user;
  std::optional<double> baseline_power_switching_user;
  std::optional<double> baseline_inst_area_um2;
  std::optional<double> baseline_core_area_um2;
};

BaselineMetrics readBaselineMetrics(OpenRoad* openroad,
                                    const std::string& ws_json_path,
                                    const std::string& wl_json_path)
{
  BaselineMetrics m;
  if (!openroad) {
    return m;
  }
  sta::dbSta* sta = openroad->getSta();
  if (!sta || !sta->units() || !sta->units()->timeUnit()) {
    return m;
  }

  if (!ws_json_path.empty()) {
    const auto v = loadJsonFile(ws_json_path);
    if (v.is_object()) {
      const auto& obj = v.as_object();
      if (const auto fmax = getObjDouble(obj, "finish__timing__fmax")) {
        if (*fmax > 0.0 && std::isfinite(*fmax)) {
          const double ecp_s = 1.0 / (*fmax);
          m.baseline_ecp_user = sta->units()->timeUnit()->staToUser(ecp_s);
        }
      }
      m.baseline_power_total_user = getObjDouble(obj, "finish__power__total");
      m.baseline_power_leak_user
          = getObjDouble(obj, "finish__power__leakage__total");
      m.baseline_power_internal_user
          = getObjDouble(obj, "finish__power__internal__total");
      m.baseline_power_switching_user
          = getObjDouble(obj, "finish__power__switching__total");
      m.baseline_inst_area_um2
          = getObjDouble(obj, "finish__design__instance__area");
      m.baseline_core_area_um2
          = getObjDouble(obj, "finish__design__core__area");
    }
  }

  if (!wl_json_path.empty()) {
    const auto v = loadJsonFile(wl_json_path);
    if (v.is_object()) {
      const auto& obj = v.as_object();
      if (const auto wl
          = getObjDouble(obj, "detailedroute__route__wirelength")) {
        m.baseline_routed_wl_um = wl;
      } else if (const auto wl_iter = getObjDouble(
                     obj, "detailedroute__route__wirelength__iter:4")) {
        m.baseline_routed_wl_um = wl_iter;
      }
    }
  }

  return m;
}

enum class KnobType
{
  kFloat,
  kInt,
  kBinary
};

enum class KnobId
{
  kClockPeriod,
  kCoreUtilization,
  kCoreAspectRatio,
  kTnsEndPercent,
  kGlobalPadding,
  kDetailPadding,
  kPlaceDensity,
  kEnableDpo,
  kPinLayerAdjust,
  kAboveLayerAdjust,
  kDensityMarginAddon,
  kCtsClusterSize,
  kCtsClusterDiameter,
  kUnknown
};

KnobId knobIdFromName(const std::string& name)
{
  if (name == "clock_period") {
    return KnobId::kClockPeriod;
  }
  if (name == "core_utilization") {
    return KnobId::kCoreUtilization;
  }
  if (name == "core_aspect_ratio") {
    return KnobId::kCoreAspectRatio;
  }
  if (name == "tns_end_percent") {
    return KnobId::kTnsEndPercent;
  }
  if (name == "global_padding") {
    return KnobId::kGlobalPadding;
  }
  if (name == "detail_padding") {
    return KnobId::kDetailPadding;
  }
  if (name == "place_density") {
    return KnobId::kPlaceDensity;
  }
  if (name == "enable_dpo") {
    return KnobId::kEnableDpo;
  }
  if (name == "pin_layer_adjust") {
    return KnobId::kPinLayerAdjust;
  }
  if (name == "above_layer_adjust") {
    return KnobId::kAboveLayerAdjust;
  }
  if (name == "density_margin_addon") {
    return KnobId::kDensityMarginAddon;
  }
  if (name == "cts_cluster_size") {
    return KnobId::kCtsClusterSize;
  }
  if (name == "cts_cluster_diameter") {
    return KnobId::kCtsClusterDiameter;
  }
  return KnobId::kUnknown;
}

struct KnobSpec
{
  std::string name;
  KnobType type = KnobType::kFloat;
  KnobId id = KnobId::kUnknown;
  double min_v = 0.0;
  double max_v = 0.0;
  double step = 0.0;
};

std::vector<std::string> splitCsv(const std::string& s)
{
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
    if (c == ',' || c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      if (!cur.empty()) {
        out.push_back(cur);
        cur.clear();
      }
      continue;
    }
    cur.push_back(c);
  }
  if (!cur.empty()) {
    out.push_back(cur);
  }
  return out;
}

std::vector<KnobSpec> parseSpace(const boost::json::value& v)
{
  if (!v.is_object()) {
    throw std::runtime_error("Invalid space JSON (expected object)");
  }
  const auto& obj = v.as_object();
  std::vector<KnobSpec> specs;
  specs.reserve(obj.size());

  for (const auto& kv : obj) {
    const std::string name = std::string(kv.key());
    if (!kv.value().is_object()) {
      continue;
    }
    const auto& spec_obj = kv.value().as_object();
    const auto type_it = spec_obj.find("type");
    const auto mm_it = spec_obj.find("minmax");
    if (type_it == spec_obj.end() || mm_it == spec_obj.end()) {
      continue;
    }
    if (!type_it->value().is_string() || !mm_it->value().is_array()) {
      continue;
    }
    const std::string type = std::string(type_it->value().as_string());
    const auto& mm = mm_it->value().as_array();
    if (mm.size() != 2) {
      continue;
    }
    const auto min_d = jsonToDouble(mm[0]);
    const auto max_d = jsonToDouble(mm[1]);
    if (!min_d || !max_d) {
      continue;
    }
    KnobSpec s;
    s.name = name;
    s.id = knobIdFromName(name);
    s.min_v = *min_d;
    s.max_v = *max_d;
    s.step = 0.0;
    if (const auto step_it = spec_obj.find("step"); step_it != spec_obj.end()) {
      if (const auto step_d = jsonToDouble(step_it->value())) {
        s.step = *step_d;
      }
    }

    if (type == "float") {
      s.type = KnobType::kFloat;
    } else if (type == "int") {
      s.type = KnobType::kInt;
    } else if (type == "binary") {
      s.type = KnobType::kBinary;
    } else {
      continue;
    }
    specs.push_back(std::move(s));
  }

  return specs;
}

struct Knobs
{
  double clock_period_user = 0.0;
  double core_utilization_pct = 70.0;
  double core_aspect_ratio = 1.0;
  double tns_end_percent = 100.0;
  double global_padding = 0.0;
  double detail_padding = 0.0;
  double place_density = 0.65;
  double enable_dpo = 1.0;
  double pin_layer_adjust = 0.35;
  double above_layer_adjust = 0.35;
  double density_margin_addon = -1.0;
  double cts_cluster_size = 20.0;
  double cts_cluster_diameter = 100.0;
};

Knobs defaultKnobsFromEnv(odb::dbBlock* block, const DesignStats& stats)
{
  Knobs k;
  k.clock_period_user = 0.0;
  if (const auto v = readEnvDouble("CORE_UTILIZATION")) {
    k.core_utilization_pct = *v;
  }
  if (const auto v = readEnvDouble("CORE_ASPECT_RATIO")) {
    k.core_aspect_ratio = *v;
  }
  if (const auto v = readEnvDouble("TNS_END_PERCENT")) {
    k.tns_end_percent = *v;
  }
  if (const auto v = readEnvDouble("CELL_PAD_IN_SITES_GLOBAL_PLACEMENT")) {
    k.global_padding = *v;
  }
  if (const auto v = readEnvDouble("CELL_PAD_IN_SITES_DETAIL_PLACEMENT")) {
    k.detail_padding = *v;
  }
  if (const auto v = readEnvDouble("PLACE_DENSITY")) {
    k.place_density = *v;
  }
  if (const auto v = readEnvDouble("ENABLE_DPO")) {
    k.enable_dpo = *v;
  }
  if (const auto v = readEnvDouble("PIN_LAYER_ADJUST")) {
    k.pin_layer_adjust = *v;
  }
  if (const auto v = readEnvDouble("ABOVE_LAYER_ADJUST")) {
    k.above_layer_adjust = *v;
  }
  if (const auto v = readEnvDouble("PLACE_DENSITY_LB_ADDON")) {
    k.density_margin_addon = *v;
  }
  if (const auto v = readEnvDouble("CTS_CLUSTER_SIZE")) {
    k.cts_cluster_size = *v;
  }
  if (const auto v = readEnvDouble("CTS_CLUSTER_DIAMETER")) {
    k.cts_cluster_diameter = *v;
  }

  const char* util_env = std::getenv("CORE_UTILIZATION");
  if (!util_env || util_env[0] == '\0') {
    const double inst_area = std::max(
        1e-9, stats.total_core_area_um2 + stats.total_macro_area_um2);
    const char* core_area_env = std::getenv("CORE_AREA");
    if (core_area_env && core_area_env[0] != '\0') {
      if (const auto rect = parseFourDoubles(core_area_env)) {
        const double w = std::abs((*rect)[2] - (*rect)[0]);
        const double h = std::abs((*rect)[3] - (*rect)[1]);
        const double core_area = w * h;
        if (core_area > 0.0) {
          k.core_utilization_pct
              = clamp(100.0 * inst_area / core_area, 20.0, 99.0);
        }
      }
    } else if (block && stats.core_bbox_area_um2 > 0.0) {
      k.core_utilization_pct
          = clamp(100.0 * inst_area / stats.core_bbox_area_um2, 20.0, 99.0);
    }
  }

  const char* ar_env = std::getenv("CORE_ASPECT_RATIO");
  if (!ar_env || ar_env[0] == '\0') {
    const char* core_area_env = std::getenv("CORE_AREA");
    if (core_area_env && core_area_env[0] != '\0') {
      if (const auto rect = parseFourDoubles(core_area_env)) {
        const double w = std::abs((*rect)[2] - (*rect)[0]);
        const double h = std::abs((*rect)[3] - (*rect)[1]);
        if (w > 0.0 && h > 0.0) {
          k.core_aspect_ratio = clamp(w / h, 0.2, 5.0);
        }
      }
    } else if (block && stats.core_bbox_area_um2 > 0.0) {
      k.core_aspect_ratio = clamp(stats.core_bbox_aspect, 0.2, 5.0);
    }
  }
  return k;
}

void applyBaseParamsJson(Knobs& k, const boost::json::value& v)
{
  if (!v.is_object()) {
    return;
  }
  const auto& obj = v.as_object();
  auto apply = [&](const char* name, double& dst) {
    const auto it = obj.find(name);
    if (it == obj.end()) {
      return;
    }
    const auto d = jsonToDouble(it->value());
    if (!d) {
      return;
    }
    dst = *d;
  };
  apply("clock_period", k.clock_period_user);
  apply("core_utilization", k.core_utilization_pct);
  apply("core_aspect_ratio", k.core_aspect_ratio);
  apply("tns_end_percent", k.tns_end_percent);
  apply("global_padding", k.global_padding);
  apply("detail_padding", k.detail_padding);
  apply("place_density", k.place_density);
  apply("enable_dpo", k.enable_dpo);
  apply("pin_layer_adjust", k.pin_layer_adjust);
  apply("above_layer_adjust", k.above_layer_adjust);
  apply("density_margin_addon", k.density_margin_addon);
  apply("cts_cluster_size", k.cts_cluster_size);
  apply("cts_cluster_diameter", k.cts_cluster_diameter);
}

double sampleFromSpec(const KnobSpec& s, std::mt19937_64& rng)
{
  if (s.type == KnobType::kBinary) {
    std::uniform_int_distribution<int> dist(0, 1);
    return static_cast<double>(dist(rng));
  }

  const double lo = s.min_v;
  const double hi = s.max_v;
  if (!(hi >= lo) || !std::isfinite(lo) || !std::isfinite(hi)) {
    return lo;
  }

  if (s.step > 0.0) {
    const double n_steps = std::floor((hi - lo) / s.step + 1e-9);
    const int n = static_cast<int>(std::max(0.0, n_steps));
    std::uniform_int_distribution<int> dist(0, n);
    const int i = dist(rng);
    const double v = lo + static_cast<double>(i) * s.step;
    if (s.type == KnobType::kInt) {
      return std::round(v);
    }
    return v;
  }

  std::uniform_real_distribution<double> dist(lo, hi);
  const double v = dist(rng);
  if (s.type == KnobType::kInt) {
    return std::round(v);
  }
  return v;
}

struct EvalResult
{
  double objective = 0.0;
  boost::json::object params;
  boost::json::object outputs;
  boost::json::object features;
};

struct ModelContext
{
  OpenRoad* openroad = nullptr;
  utl::Logger* logger = nullptr;
  odb::dbBlock* block = nullptr;
  DesignStats stats;
  StaSnapshot sta;
  WireRC wire_rc;
  double avg_cell_width_sites = 4.0;
  Calibration calib;
  BaselineMetrics baseline_metrics;
  Knobs baseline_knobs;
  bool include_features = false;

  double net_degree_mean() const
  {
    if (stats.num_nets <= 0) {
      return 0.0;
    }
    return static_cast<double>(stats.sum_net_degree)
           / static_cast<double>(stats.num_nets);
  }
};

struct SimOut
{
  double hpwl_est = 0.0;
  double detour = 1.0;
  double routability = 0.0;
  double routed_wl = 0.0;
  double delay_user = 0.0;
  double core_area_um2 = 0.0;
  double inst_area_um2 = 0.0;
  double power_user = 0.0;
  double pressure = 0.0;
  double speedup = 0.0;
  double fail_risk = 0.0;
};

template <typename PredictFn>
double calibrateScaleLogSearch(double initial_scale,
                               const double actual_value,
                               const double min_scale,
                               const double max_scale,
                               PredictFn&& predict)
{
  auto abs_error = [&](const double scale) -> double {
    const double y = predict(scale);
    if (!(y > 0.0) || !std::isfinite(y)) {
      return std::numeric_limits<double>::infinity();
    }
    return std::abs(y - actual_value);
  };

  double best_scale = clamp(initial_scale, min_scale, max_scale);
  double best_err = abs_error(best_scale);

  constexpr int kSweepPoints = 41;
  for (int i = 0; i < kSweepPoints; i++) {
    const double t
        = static_cast<double>(i) / static_cast<double>(kSweepPoints - 1);
    const double scale = min_scale * std::pow(max_scale / min_scale, t);
    const double e = abs_error(scale);
    if (e < best_err) {
      best_err = e;
      best_scale = scale;
    }
  }

  if (!std::isfinite(best_err)) {
    return best_scale;
  }

  double lo = std::max(min_scale, best_scale / 2.0);
  double hi = std::min(max_scale, best_scale * 2.0);
  const double phi = (std::sqrt(5.0) - 1.0) * 0.5;
  double a = std::log(lo);
  double b = std::log(hi);
  double c = b - phi * (b - a);
  double d = a + phi * (b - a);
  double f_c = abs_error(std::exp(c));
  double f_d = abs_error(std::exp(d));
  for (int iter = 0; iter < 24; iter++) {
    if (f_c < f_d) {
      b = d;
      d = c;
      f_d = f_c;
      c = b - phi * (b - a);
      f_c = abs_error(std::exp(c));
    } else {
      a = c;
      c = d;
      f_c = f_d;
      d = a + phi * (b - a);
      f_d = abs_error(std::exp(d));
    }
  }
  return clamp(std::exp(0.5 * (a + b)), min_scale, max_scale);
}

SimOut simulateOnce(const ModelContext& ctx, const Knobs& k, const int fidelity)
{
  SimOut o;

  const int fid = clamp(fidelity, 1, 3);
  int coupling_iters = 5;
  if (fid == 1) {
    coupling_iters = 2;
  } else if (fid == 2) {
    coupling_iters = 3;
  }

  const double speedup_base = (fid == 1) ? 0.06 : 0.05;
  const double speedup_repair = (fid == 1) ? 0.22 : 0.18;
  const double speedup_dpo = 0.05;
  const double speedup_cap = (fid == 1) ? 0.35 : 0.30;
  const double speedup_k = (fid == 1) ? 1.35 : 1.25;

  const double area_pressure_coeff = (fid == 1) ? 0.35 : 0.45;
  const double wl_pressure_coeff = (fid == 1) ? 0.20 : 0.25;
  const double thrash_coeff = (fid == 1) ? 1.5 : 2.0;
  const double thrash_density_coeff = (fid == 1) ? 0.7 : 1.0;
  const double wire_weight = (fid == 1) ? 1.5 : 2.0;

  const double clock_period_user
      = (k.clock_period_user > 0.0) ? k.clock_period_user
                                    : std::max(1e-9, ctx.sta.clock_period_user);
  const double clock_period_s = clock_period_user * ctx.sta.time_scale;

  const double util_target = clamp(k.core_utilization_pct / 100.0, 0.20, 0.99);
  const double core_aspect = clamp(k.core_aspect_ratio, 0.2, 5.0);

  const double global_pad = std::max(0.0, k.global_padding);
  const double detail_pad = std::max(0.0, k.detail_padding);
  const double pad_sum = global_pad + detail_pad;

  const double avg_cell_sites = std::max(1.0, ctx.avg_cell_width_sites);
  const double global_pad_factor
      = (avg_cell_sites + 2.0 * global_pad) / avg_cell_sites;
  const double detail_pad_factor
      = (avg_cell_sites + 2.0 * detail_pad) / avg_cell_sites;
  const double padded_utilization
      = util_target * std::max(global_pad_factor, detail_pad_factor);

  const double inst_core_area = ctx.stats.total_core_area_um2;
  const double inst_macro_area = ctx.stats.total_macro_area_um2;
  const double inst_active_area
      = std::max(1e-9, inst_core_area + inst_macro_area);
  const double macro_frac = inst_macro_area / inst_active_area;

  double core_area_est = inst_active_area / util_target;
  core_area_est = std::max(core_area_est, inst_active_area);
  const double core_w = std::sqrt(core_area_est * core_aspect);
  const double core_h = (core_w > 0.0) ? (core_area_est / core_w) : 0.0;

  // -------- Placement + routing (wirelength proxy) --------
  double k_place = 0.35;
  k_place *= 1.0 + 0.25 * std::max(0.0, util_target - 0.60);
  k_place *= 1.0 + 0.04 * pad_sum;

  const double density_lb_est = clamp(util_target + 0.03 * pad_sum, 0.20, 0.95);
  const double density_addon = (k.density_margin_addon >= 0.0)
                                   ? clamp(k.density_margin_addon, 0.0, 0.99)
                                   : -1.0;
  const double place_density_est
      = (density_addon >= 0.0)
            ? clamp(density_lb_est + (1.0 - density_lb_est) * density_addon
                        + 0.01,
                    0.0,
                    1.0)
            : clamp(std::max(k.place_density, density_lb_est), 0.0, 1.0);
  const double density_knob
      = (density_addon >= 0.0)
            ? density_addon
            : clamp((place_density_est - density_lb_est)
                        / std::max(1e-9, 1.0 - density_lb_est),
                    0.0,
                    0.99);
  k_place *= 1.0 - 0.10 * density_knob;
  k_place *= 1.0 + 0.25 * macro_frac;
  k_place *= 1.0 - 0.02 * (k.enable_dpo > 0.5);
  k_place = clamp(k_place, 0.12, 0.95);

  constexpr double kUtilSpanExp = 0.30;
  const double core_span
      = (core_w + core_h) * std::pow(util_target, kUtilSpanExp);

  double hpwl_factor_sum = ctx.stats.hpwl_factor_sum;
  if (!(hpwl_factor_sum > 0.0) || !std::isfinite(hpwl_factor_sum)) {
    const double avg_degree = clamp(ctx.net_degree_mean(), 1.0, 10.0);
    const double net_term
        = std::sqrt(std::max<std::int64_t>(1, ctx.stats.num_nets));
    hpwl_factor_sum = net_term * (1.0 + 0.40 * avg_degree);
  }

  const double length_scale
      = clamp(ctx.calib.builtin_length_scale, 0.01, 100.0);
  const double hpwl_est = core_span * hpwl_factor_sum * k_place * length_scale;
  o.hpwl_est = hpwl_est;

  const std::int64_t tech_layers
      = std::max<std::int64_t>(1, ctx.stats.num_routing_layers);
  const std::int64_t routing_layers = std::max<std::int64_t>(
      1,
      ctx.stats.used_routing_layers > 0 ? ctx.stats.used_routing_layers
                                        : tech_layers);
  constexpr double kCapacityExp = 0.55;
  const double pin_capacity
      = std::pow(clamp(1.0 - k.pin_layer_adjust, 0.05, 1.0), kCapacityExp);
  const double above_capacity
      = std::pow(clamp(1.0 - k.above_layer_adjust, 0.05, 1.0), kCapacityExp);
  const std::int64_t pin_layers = std::min<std::int64_t>(2, routing_layers);
  const std::int64_t above_layers
      = std::max<std::int64_t>(0, routing_layers - pin_layers);
  const double pin_weight = 3.0;
  const double denom = pin_weight * pin_layers + above_layers;
  const double avg_capacity = clamp(
      (pin_capacity * pin_weight * pin_layers + above_capacity * above_layers)
          / std::max(1.0, denom),
      0.05,
      1.0);

  double util_cong = util_target;
  util_cong *= 1.0 - 0.02 * pad_sum;
  const double util_over = std::max(0.0, padded_utilization - 1.0);
  util_cong *= clamp(1.0 + 25.0 * util_over, 1.0, 25.0);
  util_cong *= 1.0 + 0.12 * density_knob;
  const double density_pressure = std::max(0.0, place_density_est - 0.80);
  const double density_penalty
      = clamp(1.0 + 4.0 * std::pow(density_pressure / 0.10, 3.0), 1.0, 10.0);
  util_cong *= density_penalty;
  util_cong *= 1.0 + 0.60 * macro_frac;
  util_cong *= 1.0 - 0.03 * (k.enable_dpo > 0.5);
  util_cong = clamp(util_cong, 0.05, 2.50);
  const double util_cong_base = util_cong;

  const double routability = clamp(util_cong / avg_capacity, 0.0, 10.0);
  const double detour
      = 1.0 + 0.55 * std::pow(std::max(0.0, routability - 1.0), 2.0);
  o.detour = detour;

  const double signal_routed_wl0 = hpwl_est * detour;
  double routed_wl_est0 = signal_routed_wl0;

  const double seqs
      = std::max(1.0, std::round(ctx.stats.num_core_insts * 0.10));
  const double cluster_sz = std::max(1.0, k.cts_cluster_size);
  const double clusters = std::ceil(seqs / cluster_sz);
  const double clk_levels = std::max(1.0, std::log2(clusters + 1.0));
  const double clk_local = 0.5 * seqs * std::max(0.0, k.cts_cluster_diameter);
  const double clk_global = 0.5 * clusters * std::sqrt(core_area_est);
  const double clk_wl_est = (clk_local + clk_global) * detour;
  routed_wl_est0 += clk_wl_est;

  // -------- Timing proxy (wire RC + congestion/repair coupling) --------
  const double timing_scale
      = clamp(ctx.calib.builtin_timing_scale, 0.01, 100.0);
  const double base_path_delay_user = ctx.sta.worst_path_valid
                                          ? ctx.sta.worst_path_delay_user
                                          : clock_period_user;
  const double base_path_delay_s = base_path_delay_user * ctx.sta.time_scale;

  const int logic_depth = static_cast<int>(
      std::round(std::log2(ctx.stats.num_insts + 1.0) * 8.0 + 10.0));
  const double depth_for_wires
      = std::max(1.0, static_cast<double>(logic_depth));

  const double hpwl_factor_mean
      = (ctx.stats.nets_degree_ge2 > 0)
            ? (ctx.stats.hpwl_factor_sum / ctx.stats.nets_degree_ge2)
            : 0.0;
  double hpwl_factor_crit = hpwl_factor_mean;
  if (fid >= 3 && ctx.stats.hpwl_factor_p99 > 0.0) {
    hpwl_factor_crit = ctx.stats.hpwl_factor_p99;
  } else if (fid >= 2 && ctx.stats.hpwl_factor_p95 > 0.0) {
    hpwl_factor_crit = ctx.stats.hpwl_factor_p95;
  } else if (ctx.stats.hpwl_factor_p90 > 0.0) {
    hpwl_factor_crit = ctx.stats.hpwl_factor_p90;
  }
  const double crit_len_factor = 1.25 + 0.40 * std::max(0.0, routability - 1.0);
  const double crit_net_len0
      = (core_span * hpwl_factor_crit * k_place * length_scale * detour)
        * crit_len_factor;

  const double wire_delay0_s = 0.5 * ctx.wire_rc.r_per_um * ctx.wire_rc.c_per_um
                               * crit_net_len0 * crit_net_len0;
  const double wire_delay0_s_scaled = wire_delay0_s * timing_scale;
  const double out_res = 2500.0;
  const double wire_cap = ctx.wire_rc.c_per_um * crit_net_len0 * 1.2;
  const double wire_load_delay0_s_scaled = (out_res * wire_cap) * timing_scale;

  const double clk_level_factor = 1.0 + 0.10 * std::min(8.0, clk_levels);
  const double clk_path_len
      = 0.5 * std::max(0.0, k.cts_cluster_diameter)
        + 0.35 * std::sqrt(core_area_est) * clk_level_factor;
  const double clk_wire_delay_s = 0.5 * ctx.wire_rc.r_per_um
                                  * ctx.wire_rc.c_per_um * clk_path_len
                                  * clk_path_len;
  const double clk_delay_s_scaled = 0.25 * clk_wire_delay_s * timing_scale;

  const double repair_frac = clamp(k.tns_end_percent / 100.0, 0.0, 1.0);

  double wire_delay_s = wire_delay0_s_scaled;
  double wire_load_delay_s = wire_load_delay0_s_scaled;
  double pressure = 0.0;
  double speedup = 0.0;
  double area_scale = 1.0;
  double wl_scale = 1.0;
  double routability_eff = routability;
  double padded_util_eff = padded_utilization;
  double place_density_eff = place_density_est;

  for (int iter = 0; iter < coupling_iters; iter++) {
    const double base_gate_delay_s = base_path_delay_s * timing_scale;
    const double base_delay_s
        = base_gate_delay_s
          + wire_weight * depth_for_wires * (wire_load_delay_s + wire_delay_s);
    const double total_delay_s = base_delay_s + clk_delay_s_scaled;
    pressure = (clock_period_s > 0.0)
                   ? std::max(0.0, total_delay_s / clock_period_s - 1.0)
                   : 0.0;

    const double rout_penalty_iter
        = 1.0
          / (1.0 + 1.3 * std::pow(std::max(0.0, routability_eff - 1.0), 2.0));
    const double max_speedup
        = clamp((speedup_base + speedup_repair * repair_frac
                 + speedup_dpo * (k.enable_dpo > 0.5))
                    * rout_penalty_iter,
                0.0,
                speedup_cap);
    speedup = max_speedup * (1.0 - std::exp(-speedup_k * pressure));

    const double base_area_overhead = 0.025 * repair_frac;
    const double base_wl_overhead = 0.015 * repair_frac;
    const double pressure_sat = 1.0 - std::exp(-pressure);
    area_scale = clamp(1.0 + base_area_overhead + 0.70 * speedup
                           + area_pressure_coeff * pressure_sat * repair_frac,
                       1.0,
                       2.5);

    const double rout_over_wl = std::max(0.0, routability_eff - 1.0);
    const double dens_over_wl = std::max(0.0, place_density_eff - 0.80);
    const double util_over_wl = std::max(0.0, padded_util_eff - 0.90);
    const double cong_gate
        = clamp(rout_over_wl / 0.50 + dens_over_wl / 0.10 + util_over_wl / 0.10,
                0.0,
                1.0);

    const double pressure_ratio = pressure / (pressure + 1.0);
    const double thrash = 1.0
                          + thrash_coeff * (pressure_ratio * pressure_ratio)
                                * cong_gate
                                * (1.0 + thrash_density_coeff * dens_over_wl);
    wl_scale
        = clamp((1.0 + base_wl_overhead + 0.35 * speedup
                 + wl_pressure_coeff * pressure_sat * repair_frac * cong_gate
                 + 0.15 * std::pow(rout_over_wl, 2.0))
                    * thrash,
                1.0,
                4.0);

    const double inst_area_scale_total
        = clamp(macro_frac + (1.0 - macro_frac) * area_scale, 1.0, 10.0);
    const double padded_util_new = padded_utilization * inst_area_scale_total;
    const double place_density_new = clamp(
        std::max(place_density_est, density_lb_est * inst_area_scale_total),
        0.0,
        1.0);

    const double relax = clamp(0.65 - 0.25 * pressure_ratio, 0.25, 0.65);
    const auto mix = [&](const double prev, const double next) {
      return prev + relax * (next - prev);
    };
    padded_util_eff = mix(padded_util_eff, padded_util_new);
    place_density_eff
        = clamp(mix(place_density_eff, place_density_new), 0.0, 1.0);

    const double density_pressure_eff = std::max(0.0, place_density_eff - 0.80);
    const double density_penalty_eff = clamp(
        1.0 + 4.0 * std::pow(density_pressure_eff / 0.10, 3.0), 1.0, 10.0);
    const double density_ratio
        = density_penalty_eff / std::max(1.0, density_penalty);

    const double util_over_eff = std::max(0.0, padded_util_eff - 1.0);
    const double util_over_penalty_eff
        = clamp(1.0 + 25.0 * util_over_eff, 1.0, 25.0);
    double util_cong_eff = util_cong_base * inst_area_scale_total;
    util_cong_eff *= density_ratio;
    util_cong_eff *= util_over_penalty_eff;
    util_cong_eff *= std::pow(clamp(wl_scale, 1.0, 3.0), 0.35);
    util_cong_eff = clamp(util_cong_eff, 0.05, 25.0);
    const double routability_new
        = clamp(util_cong_eff / avg_capacity, 0.0, 10.0);
    routability_eff = mix(routability_eff, routability_new);

    const double cong_delta = std::max(0.0, routability_eff - routability);
    const double cong_delay = 1.0 + 0.80 * std::pow(cong_delta, 1.5);
    const double len_scale
        = std::max(1.0, wl_scale) * std::sqrt(std::max(1.0, cong_delay));
    const double wire_load_new = wire_load_delay0_s_scaled * len_scale;
    const double wire_delay_new
        = wire_delay0_s_scaled * (wl_scale * wl_scale) * cong_delay;
    wire_load_delay_s = mix(wire_load_delay_s, wire_load_new);
    wire_delay_s = mix(wire_delay_s, wire_delay_new);
  }

  {
    const double base_gate_delay_s = base_path_delay_s * timing_scale;
    const double base_delay_s
        = base_gate_delay_s
          + wire_weight * depth_for_wires * (wire_load_delay_s + wire_delay_s);
    const double total_delay_s = base_delay_s + clk_delay_s_scaled;
    pressure = (clock_period_s > 0.0)
                   ? std::max(0.0, total_delay_s / clock_period_s - 1.0)
                   : 0.0;

    const double rout_penalty_iter
        = 1.0
          / (1.0 + 1.3 * std::pow(std::max(0.0, routability_eff - 1.0), 2.0));
    const double max_speedup
        = clamp((speedup_base + speedup_repair * repair_frac
                 + speedup_dpo * (k.enable_dpo > 0.5))
                    * rout_penalty_iter,
                0.0,
                speedup_cap);
    speedup = max_speedup * (1.0 - std::exp(-speedup_k * pressure));
  }

  const double base_gate_delay_s = base_path_delay_s * timing_scale;
  const double wire_speedup = 0.25 * speedup;
  double delay_s = base_gate_delay_s * (1.0 - speedup)
                   + wire_weight * depth_for_wires
                         * (wire_load_delay_s + wire_delay_s)
                         * (1.0 - wire_speedup);
  delay_s += clk_delay_s_scaled;

  if (ctx.calib.builtin_ref_clock_user > 0.0 && clock_period_user > 0.0) {
    const double tight_ratio
        = ctx.calib.builtin_ref_clock_user / clock_period_user;
    const double wall_start = 1.20;
    const double wall = std::max(0.0, tight_ratio - wall_start);
    delay_s *= (1.0 + 15.0 * wall * wall);
  }

  o.delay_user
      = (ctx.sta.time_scale > 0.0) ? (delay_s / ctx.sta.time_scale) : delay_s;
  o.pressure = pressure;
  o.speedup = speedup;
  o.routed_wl = routed_wl_est0 * wl_scale;
  o.routability = routability_eff;

  const double instance_area_est
      = inst_macro_area + inst_core_area * area_scale;
  o.inst_area_um2 = instance_area_est;
  o.core_area_um2 = core_area_est;

  double fail_risk = 0.0;
  const double dpl_risk = std::max(0.0, padded_util_eff - 0.75) / 0.10;
  const double dens_risk = std::max(0.0, place_density_eff - 0.72) / 0.10;
  const double cong_risk = std::max(0.0, routability_eff - 1.05) / 1.25;
  fail_risk += dpl_risk * dpl_risk;
  fail_risk += dens_risk * dens_risk;
  fail_risk += cong_risk * cong_risk;
  if (padded_util_eff >= 1.0 || place_density_eff >= 0.99
      || routability_eff >= 8.0) {
    fail_risk += 50.0;
  }
  o.fail_risk = clamp(fail_risk, 0.0, 100.0);

  o.power_user = 0.0;
  if (ctx.baseline_metrics.baseline_power_total_user
      && ctx.baseline_metrics.baseline_inst_area_um2
      && ctx.baseline_metrics.baseline_routed_wl_um
      && ctx.calib.builtin_ref_clock_user > 0.0) {
    const double ref_clock = ctx.calib.builtin_ref_clock_user;
    const double freq_scale = clamp(ref_clock / clock_period_user, 0.2, 5.0);
    const double area_scale_rel = clamp(
        instance_area_est
            / std::max(1e-9, *ctx.baseline_metrics.baseline_inst_area_um2),
        0.25,
        4.0);
    const double wl_scale_rel = clamp(
        o.routed_wl
            / std::max(1e-9, *ctx.baseline_metrics.baseline_routed_wl_um),
        0.25,
        4.0);

    const double base_total = *ctx.baseline_metrics.baseline_power_total_user;
    double base_leak = ctx.baseline_metrics.baseline_power_leak_user.value_or(
        0.25 * base_total);
    double base_dyn = 0.0;
    if (ctx.baseline_metrics.baseline_power_internal_user) {
      base_dyn += *ctx.baseline_metrics.baseline_power_internal_user;
    }
    if (ctx.baseline_metrics.baseline_power_switching_user) {
      base_dyn += *ctx.baseline_metrics.baseline_power_switching_user;
    }
    if (!(base_dyn > 0.0) || !std::isfinite(base_dyn)) {
      base_dyn = std::max(0.0, base_total - base_leak);
    }
    if (!(base_leak > 0.0) || !std::isfinite(base_leak)) {
      base_leak = std::max(0.0, base_total - base_dyn);
    }

    const double cap_scale
        = clamp(0.5 * area_scale_rel + 0.5 * wl_scale_rel, 0.25, 4.0);
    const double leak = base_leak * area_scale_rel;
    const double dyn = base_dyn * freq_scale * cap_scale;
    o.power_user = leak + dyn;
  }
  return o;
}

boost::json::object knobsToJson(const Knobs& k,
                                const std::unordered_set<std::string>& include)
{
  boost::json::object obj;
  auto add_f = [&](const std::string& name, double v) {
    if (!include.empty() && include.find(name) == include.end()) {
      return;
    }
    obj[name] = v;
  };
  auto add_i = [&](const std::string& name, double v) {
    if (!include.empty() && include.find(name) == include.end()) {
      return;
    }
    obj[name] = static_cast<std::int64_t>(std::llround(v));
  };

  add_f("clock_period", k.clock_period_user);
  add_i("core_utilization", k.core_utilization_pct);
  add_f("core_aspect_ratio", k.core_aspect_ratio);
  add_i("tns_end_percent", k.tns_end_percent);
  add_i("global_padding", k.global_padding);
  add_i("detail_padding", k.detail_padding);
  add_f("place_density", k.place_density);
  add_i("enable_dpo", k.enable_dpo);
  add_f("pin_layer_adjust", k.pin_layer_adjust);
  add_f("above_layer_adjust", k.above_layer_adjust);
  add_f("density_margin_addon", k.density_margin_addon);
  add_i("cts_cluster_size", k.cts_cluster_size);
  add_i("cts_cluster_diameter", k.cts_cluster_diameter);
  return obj;
}

Knobs applySampleToKnobs(const Knobs& base,
                         const std::vector<KnobSpec>& space,
                         const std::unordered_set<std::string>& frozen,
                         std::mt19937_64& rng)
{
  Knobs k = base;
  for (const auto& spec : space) {
    if (frozen.find(spec.name) != frozen.end()) {
      continue;
    }
    const double v = sampleFromSpec(spec, rng);
    switch (spec.id) {
      case KnobId::kClockPeriod:
        k.clock_period_user = v;
        break;
      case KnobId::kCoreUtilization:
        k.core_utilization_pct = v;
        break;
      case KnobId::kCoreAspectRatio:
        k.core_aspect_ratio = v;
        break;
      case KnobId::kTnsEndPercent:
        k.tns_end_percent = v;
        break;
      case KnobId::kGlobalPadding:
        k.global_padding = v;
        break;
      case KnobId::kDetailPadding:
        k.detail_padding = v;
        break;
      case KnobId::kPlaceDensity:
        k.place_density = v;
        break;
      case KnobId::kEnableDpo:
        k.enable_dpo = v;
        break;
      case KnobId::kPinLayerAdjust:
        k.pin_layer_adjust = v;
        break;
      case KnobId::kAboveLayerAdjust:
        k.above_layer_adjust = v;
        break;
      case KnobId::kDensityMarginAddon:
        k.density_margin_addon = v;
        break;
      case KnobId::kCtsClusterSize:
        k.cts_cluster_size = v;
        break;
      case KnobId::kCtsClusterDiameter:
        k.cts_cluster_diameter = v;
        break;
      case KnobId::kUnknown:
        break;
    }
  }
  return k;
}

boost::json::object outputsToJson(const SimOut& o,
                                  const std::string& objective_name)
{
  boost::json::object out;
  out["power"] = o.power_user;
  out["area"] = o.core_area_um2;
  out["instance_area"] = o.inst_area_um2;
  if (objective_name == "routed_wirelength") {
    out["routed_wirelength"] = o.routed_wl;
    out["effective_clock_period"] = o.delay_user;
  } else {
    out["effective_clock_period"] = o.delay_user;
    out["routed_wirelength"] = o.routed_wl;
  }
  return out;
}

boost::json::object featuresToJson(const ModelContext& ctx,
                                   const Knobs& k,
                                   const double seed,
                                   const double noise,
                                   const int fidelity,
                                   const SimOut& o)
{
  boost::json::object f;
  f["seed"] = seed;
  f["noise"] = noise;
  f["fidelity"] = fidelity;
  f["builtin_length_scale"] = ctx.calib.builtin_length_scale;
  f["builtin_timing_scale"] = ctx.calib.builtin_timing_scale;
  f["clock_period"] = k.clock_period_user;
  f["design_num_insts"] = ctx.stats.num_insts;
  f["design_num_nets"] = ctx.stats.num_nets;
  f["design_total_core_area"] = ctx.stats.total_core_area_um2;
  f["design_total_macro_area"] = ctx.stats.total_macro_area_um2;
  f["tech_num_routing_layers"] = ctx.stats.num_routing_layers;
  f["sta_base_path_delay_valid"] = ctx.sta.worst_path_valid;
  f["sta_base_path_delay_user"] = ctx.sta.worst_path_delay_user;
  if (ctx.include_features) {
    f["surrogate_hpwl_est"] = o.hpwl_est;
    f["surrogate_detour"] = o.detour;
    f["surrogate_routability"] = o.routability;
    f["surrogate_pressure"] = o.pressure;
    f["surrogate_speedup"] = o.speedup;
    f["surrogate_fail_risk"] = o.fail_risk;
    f["surrogate_power"] = o.power_user;
    f["knob_clock_period"] = k.clock_period_user;
    f["knob_core_utilization"] = k.core_utilization_pct;
    f["knob_core_aspect_ratio"] = k.core_aspect_ratio;
    f["knob_tns_end_percent"] = k.tns_end_percent;
    f["knob_global_padding"] = k.global_padding;
    f["knob_detail_padding"] = k.detail_padding;
    f["knob_place_density"] = k.place_density;
    f["knob_enable_dpo"] = k.enable_dpo;
    f["knob_pin_layer_adjust"] = k.pin_layer_adjust;
    f["knob_above_layer_adjust"] = k.above_layer_adjust;
    f["knob_density_margin_addon"] = k.density_margin_addon;
    f["knob_cts_cluster_size"] = k.cts_cluster_size;
    f["knob_cts_cluster_diameter"] = k.cts_cluster_diameter;
  }
  return f;
}

struct OptArgs
{
  std::string space_file;
  std::string space_json;
  std::string base_params_file;
  std::string objective = "effective_clock_period";
  bool minimize = true;
  std::int64_t samples = 0;
  int top_n = 1;
  std::string seed = "auto";
  double noise = 0.0;
  int fidelity = 2;
  bool multi_fidelity = false;
  double shrink = 0.15;
  double time_budget_s = 0.0;
  int threads = 0;
  bool portfolio = false;
  double portfolio_shrink = 0.25;
  std::string freeze;
  std::string calibrate_ws_file;
  std::string calibrate_wl_file;
  bool reset_calibration = false;
  std::string format = "simple";
  std::string output;
  bool include_features = false;
};

std::uint64_t parseSeed(const std::string& s)
{
  if (s == "auto") {
    const auto now
        = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::random_device rd;
    const std::uint64_t mix = (static_cast<std::uint64_t>(now) << 1)
                              ^ static_cast<std::uint64_t>(rd());
    return mix ? mix : 1ULL;
  }

  std::uint64_t value = 0;
  const char* begin = s.data();
  const char* end = begin + s.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value, 10);
  if (ec != std::errc{} || ptr == begin) {
    return 1ULL;
  }
  return value ? value : 1ULL;
}

void writeJsonToFile(const std::string& path, const boost::json::value& v)
{
  std::ofstream f(path);
  if (!f.good()) {
    throw std::runtime_error("Could not write output JSON: " + path);
  }
  f << boost::json::serialize(v) << "\n";
}

OptArgs parseOptimizeArgs(utl::Logger* logger, int objc, Tcl_Obj* const objv[])
{
  OptArgs a;
  for (int i = 1; i < objc; i++) {
    const std::string arg = Tcl_GetString(objv[i]);
    auto need = [&](const char* what) -> std::string {
      if (i + 1 >= objc) {
        throw std::runtime_error(std::string("Missing value for ") + what);
      }
      return std::string(Tcl_GetString(objv[++i]));
    };
    if (arg == "-builtin") {
      continue;
    }
    if (arg == "-space_file") {
      a.space_file = need("-space_file");
    } else if (arg == "-space") {
      a.space_json = need("-space");
    } else if (arg == "-objective") {
      a.objective = need("-objective");
    } else if (arg == "-minimize") {
      a.minimize = true;
    } else if (arg == "-maximize") {
      a.minimize = false;
    } else if (arg == "-samples") {
      a.samples = std::stoll(need("-samples"));
    } else if (arg == "-top_n") {
      a.top_n = std::stoi(need("-top_n"));
    } else if (arg == "-seed") {
      a.seed = need("-seed");
    } else if (arg == "-noise") {
      a.noise = std::stod(need("-noise"));
    } else if (arg == "-fidelity") {
      a.fidelity = std::stoi(need("-fidelity"));
    } else if (arg == "-multi_fidelity") {
      a.multi_fidelity = true;
    } else if (arg == "-shrink") {
      a.shrink = std::stod(need("-shrink"));
    } else if (arg == "-time_budget_s") {
      a.time_budget_s = std::stod(need("-time_budget_s"));
    } else if (arg == "-threads") {
      a.threads = std::stoi(need("-threads"));
    } else if (arg == "-portfolio") {
      a.portfolio = true;
    } else if (arg == "-portfolio_shrink") {
      a.portfolio_shrink = std::stod(need("-portfolio_shrink"));
    } else if (arg == "-freeze") {
      a.freeze = need("-freeze");
    } else if (arg == "-base_params_file") {
      a.base_params_file = need("-base_params_file");
    } else if (arg == "-calibrate_ws_file") {
      a.calibrate_ws_file = need("-calibrate_ws_file");
    } else if (arg == "-calibrate_wl_file") {
      a.calibrate_wl_file = need("-calibrate_wl_file");
    } else if (arg == "-reset_calibration") {
      a.reset_calibration = true;
    } else if (arg == "-format") {
      a.format = need("-format");
    } else if (arg == "-output") {
      a.output = need("-output");
    } else if (arg == "-include_features") {
      a.include_features = true;
    } else {
      if (logger) {
        logger->warn(
            kTool, 4200, "surrogate_optimize ignoring unknown arg {}", arg);
      }
    }
  }

  if (a.space_file.empty() && a.space_json.empty()) {
    throw std::runtime_error(
        "surrogate_optimize requires -space_file or -space");
  }
  if (a.samples <= 0) {
    throw std::runtime_error("surrogate_optimize requires -samples > 0");
  }
  a.top_n = std::max(1, a.top_n);
  if (a.output.empty()) {
    throw std::runtime_error("surrogate_optimize requires -output <path>");
  }
  a.fidelity = clamp(a.fidelity, 1, 3);
  return a;
}

int surrogateSupportedFeaturesCmd(ClientData,
                                  Tcl_Interp* interp,
                                  int,
                                  Tcl_Obj* const[])
{
  boost::json::array feats;
  for (const char* name : {"effective_clock_period",
                           "routed_wirelength",
                           "area",
                           "instance_area",
                           "power",
                           "surrogate_hpwl_est",
                           "surrogate_detour",
                           "surrogate_routability",
                           "surrogate_pressure",
                           "surrogate_speedup",
                           "surrogate_fail_risk",
                           "surrogate_power"}) {
    feats.emplace_back(name);
  }
  const std::string out = boost::json::serialize(feats);
  Tcl_SetObjResult(interp,
                   Tcl_NewStringObj(out.c_str(), static_cast<int>(out.size())));
  return TCL_OK;
}

int surrogateOptimizeCmd(ClientData clientData,
                         Tcl_Interp* interp,
                         int objc,
                         Tcl_Obj* const objv[])
{
  OpenRoad* openroad = static_cast<OpenRoad*>(clientData);
  utl::Logger* logger = openroad ? openroad->getLogger() : nullptr;

  try {
    const OptArgs args = parseOptimizeArgs(logger, objc, objv);
    odb::dbDatabase* db = openroad ? openroad->getDb() : nullptr;
    odb::dbChip* chip = db ? db->getChip() : nullptr;
    odb::dbBlock* block = chip ? chip->getBlock() : nullptr;
    if (!block) {
      throw std::runtime_error("No design loaded (dbBlock is null)");
    }

    boost::json::value space_v = args.space_json.empty()
                                     ? loadJsonFile(args.space_file)
                                     : boost::json::parse(args.space_json);
    std::vector<KnobSpec> space = parseSpace(space_v);

    std::unordered_set<std::string> frozen;
    for (const std::string& name : splitCsv(args.freeze)) {
      frozen.insert(name);
    }

    DesignStats stats = computeDesignStats(block);
    stats.used_routing_layers
        = estimateUsedRoutingLayers(block->getTech(), stats.num_routing_layers);
    Knobs baseline = defaultKnobsFromEnv(block, stats);
    std::unordered_set<std::string> base_param_keys;
    if (!args.base_params_file.empty()) {
      const boost::json::value base_v = loadJsonFile(args.base_params_file);
      applyBaseParamsJson(baseline, base_v);
      if (base_v.is_object()) {
        for (const auto& kv : base_v.as_object()) {
          base_param_keys.insert(std::string(kv.key()));
        }
      }
    }

    ModelContext ctx;
    ctx.openroad = openroad;
    ctx.logger = logger;
    ctx.block = block;
    ctx.stats = stats;
    ctx.sta = getStaSnapshot(openroad);
    if (!(baseline.clock_period_user > 0.0)) {
      baseline.clock_period_user
          = (ctx.sta.clock_period_user > 0.0) ? ctx.sta.clock_period_user : 1.0;
    }
    ctx.wire_rc = estimateWireRC(block->getTech(), stats.dbu_per_micron);
    ctx.avg_cell_width_sites = computeAvgCellWidthSites(openroad, stats);
    ctx.calib = Calibration{};
    ctx.baseline_metrics = BaselineMetrics{};
    ctx.baseline_knobs = baseline;
    ctx.include_features = args.include_features;

    if (args.reset_calibration) {
      ctx.calib = Calibration{};
    }

    if (!args.calibrate_ws_file.empty() || !args.calibrate_wl_file.empty()) {
      ctx.baseline_metrics = readBaselineMetrics(
          openroad, args.calibrate_ws_file, args.calibrate_wl_file);
      if (ctx.calib.builtin_ref_clock_user <= 0.0) {
        ctx.calib.builtin_ref_clock_user = baseline.clock_period_user;
      }

      // Calibrate length/timing scales; response can be non-linear due to
      // pressure/repair coupling, so use a small log-space search.
      for (int iter = 0; iter < 2; iter++) {
        if (ctx.baseline_metrics.baseline_routed_wl_um) {
          auto predict_wl = [&](const double scale) -> double {
            ctx.calib.builtin_length_scale = scale;
            return simulateOnce(ctx, baseline, 2).routed_wl;
          };
          ctx.calib.builtin_length_scale = calibrateScaleLogSearch(
              ctx.calib.builtin_length_scale,
              *ctx.baseline_metrics.baseline_routed_wl_um,
              0.01,
              100.0,
              predict_wl);
        }
        if (ctx.baseline_metrics.baseline_ecp_user) {
          auto predict_ecp = [&](const double scale) -> double {
            ctx.calib.builtin_timing_scale = scale;
            return simulateOnce(ctx, baseline, 2).delay_user;
          };
          ctx.calib.builtin_timing_scale
              = calibrateScaleLogSearch(ctx.calib.builtin_timing_scale,
                                        *ctx.baseline_metrics.baseline_ecp_user,
                                        0.01,
                                        100.0,
                                        predict_ecp);
        }
      }
      ctx.calib.builtin_length_scale
          = clamp(ctx.calib.builtin_length_scale, 0.01, 100.0);
      ctx.calib.builtin_timing_scale
          = clamp(ctx.calib.builtin_timing_scale, 0.01, 100.0);
    }

    if (ctx.calib.builtin_ref_clock_user <= 0.0) {
      ctx.calib.builtin_ref_clock_user = baseline.clock_period_user;
    }

    applyCalibrationOverridesFromEnv(ctx.calib);
    ctx.calib.builtin_length_scale
        = clamp(ctx.calib.builtin_length_scale, 0.01, 100.0);
    ctx.calib.builtin_timing_scale
        = clamp(ctx.calib.builtin_timing_scale, 0.01, 100.0);
    if (ctx.calib.builtin_ref_clock_user <= 0.0) {
      ctx.calib.builtin_ref_clock_user = baseline.clock_period_user;
    }

    const std::uint64_t seed = parseSeed(args.seed);
    const int max_threads = 256;
    int threads = args.threads;
    if (threads <= 0) {
      threads = static_cast<int>(std::thread::hardware_concurrency());
    }
    threads = clamp(threads, 1, max_threads);

    const auto t0 = std::chrono::steady_clock::now();
    const double total_budget_s = args.time_budget_s;
    const double coarse_budget_s = (args.multi_fidelity && total_budget_s > 0.0)
                                       ? (0.70 * total_budget_s)
                                       : total_budget_s;
    auto timed_out = [&]() -> bool {
      if (!(total_budget_s > 0.0)) {
        return false;
      }
      const auto t1 = std::chrono::steady_clock::now();
      const double dt = std::chrono::duration<double>(t1 - t0).count();
      return dt >= total_budget_s;
    };
    auto coarse_timed_out = [&]() -> bool {
      if (!(coarse_budget_s > 0.0)) {
        return false;
      }
      const auto t1 = std::chrono::steady_clock::now();
      const double dt = std::chrono::duration<double>(t1 - t0).count();
      return dt >= coarse_budget_s;
    };

    struct Scored
    {
      double obj = 0.0;
      Knobs knobs;
      SimOut sim;
    };
    auto better
        = [&](double a, double b) { return args.minimize ? (a < b) : (a > b); };

    const std::unordered_set<std::string> include_knobs = [&]() {
      std::unordered_set<std::string> out;
      for (const auto& s : space) {
        out.insert(s.name);
      }
      for (const auto& k : base_param_keys) {
        out.insert(k);
      }
      return out;
    }();

    std::vector<Scored> best;
    best.reserve(args.top_n);

    const auto eval_one = [&](std::mt19937_64& rng,
                              std::normal_distribution<double>& noise_dist,
                              const Knobs& k,
                              const int fid) -> Scored {
      const SimOut sim = simulateOnce(ctx, k, fid);
      double obj = 0.0;
      if (args.objective == "routed_wirelength") {
        obj = sim.routed_wl;
      } else if (args.objective == "power") {
        obj = sim.power_user;
      } else if (args.objective == "area") {
        obj = sim.core_area_um2;
      } else if (args.objective == "instance_area") {
        obj = sim.inst_area_um2;
      } else {
        obj = sim.delay_user;
      }
      obj *= (1.0 + 0.12 * sim.fail_risk);
      if (args.noise > 0.0) {
        obj += noise_dist(rng) * args.noise;
      }
      return Scored{.obj = obj, .knobs = k, .sim = sim};
    };

    const std::int64_t n0 = args.samples;
    const std::int64_t coarse_n
        = args.multi_fidelity ? static_cast<std::int64_t>(std::round(n0 * 0.80))
                              : n0;
    const std::int64_t refine_n = args.multi_fidelity ? (n0 - coarse_n) : 0;

    std::int64_t samples_evaluated = 0;
    std::int64_t refinements_evaluated = 0;

    auto insert_best = [&](std::vector<Scored>& vec, const Scored& s) {
      if (static_cast<int>(vec.size()) < args.top_n) {
        vec.push_back(s);
        if (static_cast<int>(vec.size()) == args.top_n) {
          std::ranges::sort(vec, [&](const Scored& x, const Scored& y) {
            return better(x.obj, y.obj);
          });
        }
        return;
      }
      if (!vec.empty() && better(s.obj, vec.back().obj)) {
        vec.back() = s;
        for (std::size_t j = vec.size() - 1; j > 0; j--) {
          if (better(vec[j].obj, vec[j - 1].obj)) {
            std::swap(vec[j], vec[j - 1]);
          } else {
            break;
          }
        }
      }
    };

    std::atomic<std::int64_t> coarse_idx{0};
    std::atomic<std::int64_t> coarse_evaluated{0};
    std::atomic<bool> stop{false};
    std::vector<std::vector<Scored>> local_best(
        static_cast<std::size_t>(threads));
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threads));

    const double portfolio_shrink = clamp(args.portfolio_shrink, 0.0, 1.0);
    const auto portfolio_space_for_mode
        = [&](const int mode) -> std::vector<KnobSpec> {
      if (!args.portfolio) {
        return space;
      }
      std::vector<KnobSpec> local = space;
      for (auto& spec : local) {
        if (frozen.find(spec.name) != frozen.end()) {
          continue;
        }

        double shrink = 1.0;
        if (mode == 0) {
          shrink = 1.0;
        } else if (mode == 1) {
          shrink = portfolio_shrink;
        } else if (mode == 2) {
          // Route-focused exploration: keep routing knobs global, keep most
          // others local.
          const bool is_route = (spec.id == KnobId::kPinLayerAdjust
                                 || spec.id == KnobId::kAboveLayerAdjust
                                 || spec.id == KnobId::kDensityMarginAddon);
          shrink = is_route ? 1.0 : portfolio_shrink;
        } else {
          // Floorplan-focused exploration: keep floorplan knobs global, keep
          // most others local.
          const bool is_floorplan = (spec.id == KnobId::kCoreUtilization
                                     || spec.id == KnobId::kCoreAspectRatio);
          shrink = is_floorplan ? 1.0 : portfolio_shrink;
        }

        if (!(shrink < 1.0)) {
          continue;
        }

        double cur = 0.0;
        switch (spec.id) {
          case KnobId::kClockPeriod:
            cur = baseline.clock_period_user;
            break;
          case KnobId::kCoreUtilization:
            cur = baseline.core_utilization_pct;
            break;
          case KnobId::kCoreAspectRatio:
            cur = baseline.core_aspect_ratio;
            break;
          case KnobId::kTnsEndPercent:
            cur = baseline.tns_end_percent;
            break;
          case KnobId::kGlobalPadding:
            cur = baseline.global_padding;
            break;
          case KnobId::kDetailPadding:
            cur = baseline.detail_padding;
            break;
          case KnobId::kPlaceDensity:
            cur = baseline.place_density;
            break;
          case KnobId::kEnableDpo:
            cur = baseline.enable_dpo;
            break;
          case KnobId::kPinLayerAdjust:
            cur = baseline.pin_layer_adjust;
            break;
          case KnobId::kAboveLayerAdjust:
            cur = baseline.above_layer_adjust;
            break;
          case KnobId::kDensityMarginAddon:
            cur = baseline.density_margin_addon;
            break;
          case KnobId::kCtsClusterSize:
            cur = baseline.cts_cluster_size;
            break;
          case KnobId::kCtsClusterDiameter:
            cur = baseline.cts_cluster_diameter;
            break;
          case KnobId::kUnknown:
            cur = 0.5 * (spec.min_v + spec.max_v);
            break;
        }

        const double span = spec.max_v - spec.min_v;
        const double half = 0.5 * span * shrink;
        double lo = clamp(cur - half, spec.min_v, spec.max_v);
        double hi = clamp(cur + half, spec.min_v, spec.max_v);
        if (lo > hi) {
          std::swap(lo, hi);
        }
        spec.min_v = lo;
        spec.max_v = hi;
      }
      return local;
    };

    std::vector<std::vector<KnobSpec>> portfolio_spaces;
    const int num_portfolio_modes = args.portfolio ? 4 : 1;
    portfolio_spaces.reserve(static_cast<std::size_t>(num_portfolio_modes));
    for (int mode = 0; mode < num_portfolio_modes; mode++) {
      portfolio_spaces.push_back(portfolio_space_for_mode(mode));
    }

    for (int tid = 0; tid < threads; tid++) {
      workers.emplace_back([&, tid]() {
        const std::uint64_t tid_seed
            = seed
              ^ (0x9E3779B97F4A7C15ULL
                 + static_cast<std::uint64_t>(tid) * 0xBF58476D1CE4E5B9ULL);
        std::mt19937_64 rng(tid_seed ? tid_seed : 1ULL);
        std::normal_distribution<double> noise_dist(0.0, 1.0);

        auto& vec = local_best[static_cast<std::size_t>(tid)];
        vec.reserve(args.top_n);

        while (true) {
          if (stop.load(std::memory_order_relaxed)) {
            break;
          }
          const std::int64_t i
              = coarse_idx.fetch_add(1, std::memory_order_relaxed);
          if (i >= coarse_n) {
            break;
          }
          if ((i & 0x1FFF) == 0 && coarse_timed_out()) {
            stop.store(true, std::memory_order_relaxed);
            break;
          }

          const std::vector<KnobSpec>& thread_space
              = portfolio_spaces[static_cast<std::size_t>(
                  tid % num_portfolio_modes)];
          const Knobs k
              = applySampleToKnobs(baseline, thread_space, frozen, rng);
          Scored s = eval_one(
              rng, noise_dist, k, args.multi_fidelity ? 1 : args.fidelity);
          coarse_evaluated.fetch_add(1, std::memory_order_relaxed);
          insert_best(vec, s);
        }
      });
    }

    for (auto& t : workers) {
      t.join();
    }
    workers.clear();

    samples_evaluated = coarse_evaluated.load(std::memory_order_relaxed);
    for (auto& vec : local_best) {
      for (auto& s : vec) {
        insert_best(best, s);
      }
      vec.clear();
    }
    std::ranges::sort(best, [&](const Scored& x, const Scored& y) {
      return better(x.obj, y.obj);
    });

    if (args.multi_fidelity) {
      std::mt19937_64 promote_rng(seed ^ 0xA5A5A5A5A5A5A5A5ULL);
      std::normal_distribution<double> promote_noise_dist(0.0, 1.0);
      for (auto& s : best) {
        s = eval_one(promote_rng, promote_noise_dist, s.knobs, args.fidelity);
      }
      std::ranges::sort(best, [&](const Scored& x, const Scored& y) {
        return better(x.obj, y.obj);
      });
    }

    if (best.empty()) {
      throw std::runtime_error(
          "surrogate_optimize could not evaluate any samples");
    }

    if (args.multi_fidelity && refine_n > 0) {
      const double shrink = clamp(args.shrink, 0.0, 1.0);

      const auto local_space_for_center
          = [&](const Knobs& base_k) -> std::vector<KnobSpec> {
        std::vector<KnobSpec> local = space;
        for (auto& spec : local) {
          if (frozen.find(spec.name) != frozen.end()) {
            continue;
          }
          double cur = 0.0;
          switch (spec.id) {
            case KnobId::kClockPeriod:
              cur = base_k.clock_period_user;
              break;
            case KnobId::kCoreUtilization:
              cur = base_k.core_utilization_pct;
              break;
            case KnobId::kCoreAspectRatio:
              cur = base_k.core_aspect_ratio;
              break;
            case KnobId::kTnsEndPercent:
              cur = base_k.tns_end_percent;
              break;
            case KnobId::kGlobalPadding:
              cur = base_k.global_padding;
              break;
            case KnobId::kDetailPadding:
              cur = base_k.detail_padding;
              break;
            case KnobId::kPlaceDensity:
              cur = base_k.place_density;
              break;
            case KnobId::kEnableDpo:
              cur = base_k.enable_dpo;
              break;
            case KnobId::kPinLayerAdjust:
              cur = base_k.pin_layer_adjust;
              break;
            case KnobId::kAboveLayerAdjust:
              cur = base_k.above_layer_adjust;
              break;
            case KnobId::kDensityMarginAddon:
              cur = base_k.density_margin_addon;
              break;
            case KnobId::kCtsClusterSize:
              cur = base_k.cts_cluster_size;
              break;
            case KnobId::kCtsClusterDiameter:
              cur = base_k.cts_cluster_diameter;
              break;
            case KnobId::kUnknown:
              cur = 0.5 * (spec.min_v + spec.max_v);
              break;
          }

          const double span = spec.max_v - spec.min_v;
          const double half = 0.5 * span * shrink;
          double lo = clamp(cur - half, spec.min_v, spec.max_v);
          double hi = clamp(cur + half, spec.min_v, spec.max_v);
          if (lo > hi) {
            std::swap(lo, hi);
          }
          spec.min_v = lo;
          spec.max_v = hi;
        }
        return local;
      };

      std::vector<std::vector<KnobSpec>> local_spaces;
      local_spaces.reserve(best.size());
      for (const auto& c : best) {
        local_spaces.push_back(local_space_for_center(c.knobs));
      }

      for (auto& vec : local_best) {
        vec.clear();
      }
      stop.store(false, std::memory_order_relaxed);

      std::atomic<std::int64_t> refine_idx{0};
      std::atomic<std::int64_t> refine_evaluated{0};
      for (int tid = 0; tid < threads; tid++) {
        workers.emplace_back([&, tid]() {
          const std::uint64_t tid_seed
              = seed
                ^ (0xD1B54A32D192ED03ULL
                   + static_cast<std::uint64_t>(tid) * 0x94D049BB133111EBULL);
          std::mt19937_64 rng(tid_seed ? tid_seed : 1ULL);
          std::normal_distribution<double> noise_dist(0.0, 1.0);

          auto& vec = local_best[static_cast<std::size_t>(tid)];
          vec.reserve(args.top_n);

          while (true) {
            if (stop.load(std::memory_order_relaxed)) {
              break;
            }
            const std::int64_t i
                = refine_idx.fetch_add(1, std::memory_order_relaxed);
            if (i >= refine_n) {
              break;
            }
            if ((i & 0x3FFF) == 0 && timed_out()) {
              stop.store(true, std::memory_order_relaxed);
              break;
            }
            const std::size_t cidx = static_cast<std::size_t>(
                i % static_cast<std::int64_t>(best.size()));
            const Scored& center = best[cidx];
            const Knobs k = applySampleToKnobs(
                center.knobs, local_spaces[cidx], frozen, rng);
            Scored s = eval_one(rng, noise_dist, k, args.fidelity);
            refine_evaluated.fetch_add(1, std::memory_order_relaxed);
            insert_best(vec, s);
          }
        });
      }

      for (auto& t : workers) {
        t.join();
      }
      workers.clear();

      refinements_evaluated = refine_evaluated.load(std::memory_order_relaxed);
      for (auto& vec : local_best) {
        for (auto& s : vec) {
          insert_best(best, s);
        }
        vec.clear();
      }
      std::ranges::sort(best, [&](const Scored& x, const Scored& y) {
        return better(x.obj, y.obj);
      });
    }

    boost::json::array top_arr;
    for (const auto& s : best) {
      boost::json::object entry;
      entry["objective"] = s.obj;
      {
        boost::json::object params = knobsToJson(s.knobs, include_knobs);
        params["clock_period"] = s.knobs.clock_period_user;
        entry["params"] = std::move(params);
      }
      entry["outputs"] = outputsToJson(s.sim, args.objective);
      if (args.include_features) {
        entry["features"] = featuresToJson(ctx,
                                           s.knobs,
                                           static_cast<double>(seed),
                                           args.noise,
                                           args.fidelity,
                                           s.sim);
      }
      top_arr.emplace_back(std::move(entry));
    }

    const Scored& best1 = best.front();
    boost::json::object out_obj;
    boost::json::object obj_desc;
    obj_desc["name"] = args.objective;
    obj_desc["sense"] = args.minimize ? "minimize" : "maximize";
    out_obj["objective"] = std::move(obj_desc);
    out_obj["samples"] = args.samples;
    out_obj["samples_evaluated"] = samples_evaluated;
    out_obj["refinements_evaluated"] = refinements_evaluated;
    out_obj["threads"] = threads;
    out_obj["portfolio"] = args.portfolio;
    out_obj["portfolio_shrink"] = args.portfolio_shrink;
    out_obj["seed"] = seed;
    out_obj["noise"] = args.noise;
    out_obj["fidelity"] = args.fidelity;
    out_obj["multi_fidelity"] = args.multi_fidelity;
    out_obj["shrink"] = args.shrink;
    out_obj["time_budget_s"] = args.time_budget_s;
    out_obj["best_objective"] = best1.obj;
    {
      boost::json::object params = knobsToJson(best1.knobs, include_knobs);
      params["clock_period"] = best1.knobs.clock_period_user;
      out_obj["best_params"] = std::move(params);
    }
    out_obj["best_outputs"] = outputsToJson(best1.sim, args.objective);
    if (args.include_features) {
      out_obj["best_features"] = featuresToJson(ctx,
                                                best1.knobs,
                                                static_cast<double>(seed),
                                                args.noise,
                                                args.fidelity,
                                                best1.sim);
    }
    out_obj["top"] = std::move(top_arr);

    writeJsonToFile(args.output, out_obj);

    std::ostringstream result;
    if (args.format == "simple") {
      result << "best_objective=" << best1.obj;
    } else {
      result << boost::json::serialize(out_obj);
    }
    const std::string res = result.str();
    Tcl_SetObjResult(
        interp, Tcl_NewStringObj(res.c_str(), static_cast<int>(res.size())));
    return TCL_OK;
  } catch (const std::exception& e) {
    if (logger) {
      logger->warn(kTool, 4201, "surrogate_optimize failed: {}", e.what());
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }
}

int surrogateEvalCmd(ClientData clientData,
                     Tcl_Interp* interp,
                     int objc,
                     Tcl_Obj* const objv[])
{
  OpenRoad* openroad = static_cast<OpenRoad*>(clientData);
  utl::Logger* logger = openroad ? openroad->getLogger() : nullptr;

  try {
    std::string params_file;
    std::string params_json;
    std::string output;
    bool include_features = false;

    for (int i = 1; i < objc; i++) {
      const std::string arg = Tcl_GetString(objv[i]);
      auto need = [&](const char* what) -> std::string {
        if (i + 1 >= objc) {
          throw std::runtime_error(std::string("Missing value for ") + what);
        }
        return std::string(Tcl_GetString(objv[++i]));
      };
      if (arg == "-builtin") {
        continue;
      }
      if (arg == "-params_file") {
        params_file = need("-params_file");
      } else if (arg == "-params") {
        params_json = need("-params");
      } else if (arg == "-output") {
        output = need("-output");
      } else if (arg == "-include_features") {
        include_features = true;
      }
    }

    if (params_file.empty() && params_json.empty()) {
      throw std::runtime_error(
          "surrogate_eval requires -params_file or -params");
    }

    odb::dbDatabase* db = openroad ? openroad->getDb() : nullptr;
    odb::dbChip* chip = db ? db->getChip() : nullptr;
    odb::dbBlock* block = chip ? chip->getBlock() : nullptr;
    if (!block) {
      throw std::runtime_error("No design loaded (dbBlock is null)");
    }

    DesignStats stats = computeDesignStats(block);
    stats.used_routing_layers
        = estimateUsedRoutingLayers(block->getTech(), stats.num_routing_layers);
    Knobs baseline = defaultKnobsFromEnv(block, stats);
    boost::json::value params_v = params_json.empty()
                                      ? loadJsonFile(params_file)
                                      : boost::json::parse(params_json);
    applyBaseParamsJson(baseline, params_v);

    ModelContext ctx;
    ctx.openroad = openroad;
    ctx.logger = logger;
    ctx.block = block;
    ctx.stats = stats;
    ctx.sta = getStaSnapshot(openroad);
    if (!(baseline.clock_period_user > 0.0)) {
      baseline.clock_period_user
          = (ctx.sta.clock_period_user > 0.0) ? ctx.sta.clock_period_user : 1.0;
    }
    ctx.wire_rc = estimateWireRC(block->getTech(), stats.dbu_per_micron);
    ctx.avg_cell_width_sites = computeAvgCellWidthSites(openroad, stats);
    ctx.calib = loadCalibrationFromEnv();
    ctx.baseline_metrics = BaselineMetrics{};
    ctx.baseline_knobs = baseline;
    ctx.include_features = include_features;
    if (ctx.calib.builtin_ref_clock_user <= 0.0) {
      ctx.calib.builtin_ref_clock_user = baseline.clock_period_user;
    }
    const SimOut sim = simulateOnce(ctx, baseline, 2);

    boost::json::object out;
    {
      boost::json::object params = knobsToJson(baseline, {});
      params["clock_period"] = baseline.clock_period_user;
      out["params"] = std::move(params);
    }
    out["outputs"] = outputsToJson(sim, "effective_clock_period");
    if (include_features) {
      out["features"] = featuresToJson(ctx, baseline, 0.0, 0.0, 2, sim);
    }
    if (!output.empty()) {
      writeJsonToFile(output, out);
    }
    const std::string res = boost::json::serialize(out);
    Tcl_SetObjResult(
        interp, Tcl_NewStringObj(res.c_str(), static_cast<int>(res.size())));
    return TCL_OK;
  } catch (const std::exception& e) {
    if (logger) {
      logger->warn(kTool, 4202, "surrogate_eval failed: {}", e.what());
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }
}

}  // namespace

void initSurrogate(Tcl_Interp* interp, OpenRoad* openroad)
{
  if (!interp) {
    return;
  }
  Tcl_CreateObjCommand(interp,
                       "surrogate_supported_features",
                       surrogateSupportedFeaturesCmd,
                       nullptr,
                       nullptr);
  Tcl_CreateObjCommand(
      interp, "surrogate_optimize", surrogateOptimizeCmd, openroad, nullptr);
  Tcl_CreateObjCommand(
      interp, "surrogate_eval", surrogateEvalCmd, openroad, nullptr);
}

}  // namespace ord
