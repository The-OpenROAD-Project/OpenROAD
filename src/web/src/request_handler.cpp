// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "request_handler.h"

#include <algorithm>
#include <any>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#include "clock_tree_report.h"
#include "color.h"
#include "gui/descriptor_registry.h"
#include "gui/gui.h"
#include "gui/heatMap.h"
#include "hierarchy_report.h"
#include "json_builder.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "tile_generator.h"
#include "timing_report.h"

namespace web {

//------------------------------------------------------------------------------
// ShapeCollector — a gui::Painter that collects rectangles from
// descriptor->highlight() calls for use in tile rendering.
//------------------------------------------------------------------------------

class ShapeCollector : public gui::Painter
{
 public:
  ShapeCollector() : Painter(nullptr, odb::Rect(), 1.0) {}

  std::vector<odb::Rect> rects;
  std::vector<odb::Polygon> polys;

  void drawRect(const odb::Rect& rect, int, int) override
  {
    rects.push_back(rect);
  }
  void drawPolygon(const odb::Polygon& polygon) override
  {
    polys.push_back(polygon);
  }
  void drawOctagon(const odb::Oct& oct) override { polys.emplace_back(oct); }

  // No-ops
  Color getPenColor() override { return {}; }
  void setPen(odb::dbTechLayer*, bool) override {}
  void setPen(const Color&, bool, int) override {}
  void setPenWidth(int) override {}
  void setBrush(odb::dbTechLayer*, int) override {}
  void setBrush(const Color&, const Brush&) override {}
  void setFont(const Font&) override {}
  void saveState() override {}
  void restoreState() override {}
  void drawLine(const odb::Point&, const odb::Point&) override {}
  void drawCircle(int, int, int) override {}
  void drawX(int, int, int) override {}
  void drawPolygon(const std::vector<odb::Point>&) override {}
  void drawString(int, int, Anchor, const std::string&, bool) override {}
  odb::Rect stringBoundaries(int, int, Anchor, const std::string&) override
  {
    return {};
  }
  void drawRuler(int, int, int, int, bool, const std::string&) override {}
};

//------------------------------------------------------------------------------
// JSON utilities
//------------------------------------------------------------------------------

std::string extract_string(const std::string& json, const std::string& key)
{
  const std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos) {
    return {};
  }
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos) {
    return {};
  }
  auto quote_start = json.find('"', pos + 1);
  if (quote_start == std::string::npos) {
    return {};
  }
  // Find closing quote, skipping escaped quotes
  std::string result;
  for (size_t i = quote_start + 1; i < json.size(); i++) {
    if (json[i] == '\\' && i + 1 < json.size()) {
      switch (json[i + 1]) {
        case '"':
          result += '"';
          break;
        case '\\':
          result += '\\';
          break;
        case 'n':
          result += '\n';
          break;
        case 'r':
          result += '\r';
          break;
        case 't':
          result += '\t';
          break;
        default:
          result += json[i + 1];
          break;
      }
      i++;  // skip the escaped char
    } else if (json[i] == '"') {
      break;  // closing quote
    } else {
      result += json[i];
    }
  }
  return result;
}

std::set<std::string> extract_string_array(const std::string& json,
                                           const std::string& key)
{
  std::set<std::string> result;
  const std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos) {
    return result;
  }
  pos = json.find('[', pos + needle.size());
  if (pos == std::string::npos) {
    return result;
  }
  auto end = json.find(']', pos);
  if (end == std::string::npos) {
    return result;
  }
  // Extract each quoted string between [ and ]
  for (auto i = pos; i < end;) {
    auto qs = json.find('"', i);
    if (qs == std::string::npos || qs >= end) {
      break;
    }
    auto qe = json.find('"', qs + 1);
    if (qe == std::string::npos || qe >= end) {
      break;
    }
    result.insert(json.substr(qs + 1, qe - qs - 1));
    i = qe + 1;
  }
  return result;
}

int extract_int(const std::string& json, const std::string& key)
{
  const std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos) {
    return 0;
  }
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos) {
    return 0;
  }
  // Skip whitespace after colon
  pos++;
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) {
    pos++;
  }
  try {
    return std::stoi(json.substr(pos));
  } catch (...) {
    return 0;
  }
}

int extract_int_or(const std::string& json,
                   const std::string& key,
                   const int default_val)
{
  const std::string needle = "\"" + key + "\"";
  if (json.find(needle) == std::string::npos) {
    return default_val;
  }
  return extract_int(json, key);
}

float extract_float_or(const std::string& json,
                       const std::string& key,
                       float default_val)
{
  const std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos) {
    return default_val;
  }
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos) {
    return default_val;
  }
  pos++;
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) {
    pos++;
  }
  try {
    return std::stof(json.substr(pos));
  } catch (...) {
    return default_val;
  }
}

// Store a Selected in the clickables vector and return its index.
static int storeSelectable(std::vector<gui::Selected>& selectables,
                           const gui::Selected& sel)
{
  int id = static_cast<int>(selectables.size());
  selectables.push_back(sel);
  return id;
}

// Emit a bbox as a JSON array field: "key": [xMin, yMin, xMax, yMax]
static void writeBBox(JsonBuilder& builder, const char* key, const odb::Rect& r)
{
  builder.beginArray(key);
  builder.value(r.xMin());
  builder.value(r.yMin());
  builder.value(r.xMax());
  builder.value(r.yMax());
  builder.endArray();
}

static void serializeAnyValue(JsonBuilder& builder,
                              const char* field_name,
                              const std::any& value,
                              std::vector<gui::Selected>& selectables,
                              bool short_name = false)
{
  if (auto* sel = std::any_cast<gui::Selected>(&value)) {
    if (*sel) {
      const std::string name
          = short_name ? sel->getShortName() : sel->getName();
      int id = storeSelectable(selectables, *sel);
      builder.field(field_name, name);
      const std::string id_key = std::string(field_name) + "_select_id";
      builder.field(id_key, id);
      return;
    }
  }
  std::string str = gui::Descriptor::Property::toString(value);
  builder.field(field_name, str);
}

static void serializeProperty(JsonBuilder& builder,
                              const gui::Descriptor::Property& prop,
                              std::vector<gui::Selected>& selectables)
{
  builder.beginObject();
  builder.field("name", prop.name);

  if (auto* plist = std::any_cast<gui::Descriptor::PropertyList>(&prop.value)) {
    builder.beginArray("children");
    for (const auto& [key, val] : *plist) {
      builder.beginObject();
      serializeAnyValue(builder, "name", key, selectables, /*short_name=*/true);
      serializeAnyValue(builder, "value", val, selectables);
      builder.endObject();
    }
    builder.endArray();
  } else if (auto* sel_set = std::any_cast<gui::SelectionSet>(&prop.value)) {
    builder.beginArray("children");
    for (const auto& sel : *sel_set) {
      builder.beginObject();
      int id = storeSelectable(selectables, sel);
      builder.field("name", sel.getName());
      builder.field("name_select_id", id);
      builder.endObject();
    }
    builder.endArray();
  } else if (auto* sel = std::any_cast<gui::Selected>(&prop.value)) {
    if (*sel) {
      int id = storeSelectable(selectables, *sel);
      builder.field("value", sel->getName());
      builder.field("value_select_id", id);
    }
  } else {
    const std::string val_str = prop.toString();
    builder.field("value", val_str);
  }

  builder.endObject();
}

static void collectHighlightShapes(const gui::Selected& sel,
                                   std::vector<odb::Rect>& rects,
                                   std::vector<odb::Polygon>& polys)
{
  rects.clear();
  polys.clear();
  if (!sel) {
    return;
  }
  ShapeCollector collector;
  sel.highlight(collector);
  rects = std::move(collector.rects);
  polys = std::move(collector.polys);
}

static void writeInspectPayload(JsonBuilder& builder,
                                const gui::Selected& sel,
                                std::vector<gui::Selected>& new_selectables,
                                bool can_navigate_back)
{
  builder.field("can_navigate_back", can_navigate_back ? 1 : 0);
  if (!sel) {
    builder.field("error", "invalid select_id");
    return;
  }

  auto props = sel.getProperties();
  builder.field("name", sel.getName());
  builder.field("type", sel.getTypeName());
  builder.beginArray("properties");
  for (const auto& prop : props) {
    serializeProperty(builder, prop, new_selectables);
  }
  builder.endArray();

  odb::Rect bbox;
  if (sel.getBBox(bbox)) {
    writeBBox(builder, "bbox", bbox);
  }

  if (sel.isNet()) {
    auto* net = std::any_cast<odb::dbNet*>(sel.getObject());
    if (net && !net->getGuides().empty()) {
      builder.field("has_guides", 1);
    }
  }
}

// Serialize a TimingNode to JSON.
static void serializeTimingNode(JsonBuilder& builder, const TimingNode& n)
{
  builder.beginObject();
  builder.field("pin", n.pin_name);
  builder.field("fanout", n.fanout);
  builder.field("rise", n.is_rising);
  builder.field("clk", n.is_clock);
  builder.field("time", n.time);
  builder.field("delay", n.delay);
  builder.field("slew", n.slew);
  builder.field("load", n.load);
  builder.endObject();
}

static double extract_double_value(const std::string& json)
{
  return extract_float_or(json, "value", 0.0F);
}

static bool extract_bool_value(const std::string& json)
{
  if (json.find("\"value\":true") != std::string::npos) {
    return true;
  }
  if (json.find("\"value\":false") != std::string::npos) {
    return false;
  }
  return extract_int_or(json, "value", 0) != 0;
}

static void writeColorArray(JsonBuilder& builder,
                            const char* key,
                            const gui::Painter::Color& color)
{
  builder.beginArray(key);
  builder.value(color.r);
  builder.value(color.g);
  builder.value(color.b);
  builder.value(color.a);
  builder.endArray();
}

static void serializeHeatMapOption(
    JsonBuilder& builder,
    const gui::HeatMapDataSource::MapSetting& option)
{
  builder.beginObject();
  if (std::holds_alternative<gui::HeatMapDataSource::MapSettingBoolean>(
          option)) {
    const auto& setting
        = std::get<gui::HeatMapDataSource::MapSettingBoolean>(option);
    builder.field("type", "bool");
    builder.field("name", setting.name);
    builder.field("label", setting.label);
    builder.field("value", setting.getter());
  } else {
    const auto& setting
        = std::get<gui::HeatMapDataSource::MapSettingMultiChoice>(option);
    builder.field("type", "choice");
    builder.field("name", setting.name);
    builder.field("label", setting.label);
    builder.field("value", setting.getter());
    builder.beginArray("choices");
    for (const auto& choice : setting.choices()) {
      builder.value(choice);
    }
    builder.endArray();
  }
  builder.endObject();
}

static void serializeHeatMap(JsonBuilder& builder,
                             gui::HeatMapDataSource& source,
                             const bool active)
{
  if (active) {
    source.ensureMap();
  }
  const bool populated = source.isPopulated();
  const bool has_data = source.hasData();

  builder.beginObject();
  builder.field("name", source.getShortName());
  builder.field("title", source.getName());
  builder.field("active", active);
  builder.field("settings_group", source.getSettingsGroupName());
  builder.field("has_data", has_data);
  builder.field("can_adjust_grid", source.canAdjustGrid());
  builder.field("show_numbers", source.getShowNumbers());
  builder.field("show_legend", source.getShowLegend());
  builder.field("supports_numbers", true);
  builder.field("units", source.getValueUnits());
  builder.field("display_range_increment", source.getDisplayRangeIncrement());
  builder.field("display_min",
                source.convertPercentToValue(source.getDisplayRangeMin()));
  builder.field("display_max",
                source.convertPercentToValue(source.getDisplayRangeMax()));
  builder.field(
      "display_min_limit",
      source.convertPercentToValue(source.getDisplayRangeMinimumValue()));
  builder.field(
      "display_max_limit",
      source.convertPercentToValue(source.getDisplayRangeMaximumValue()));
  builder.field("draw_below_min", source.getDrawBelowRangeMin());
  builder.field("draw_above_max", source.getDrawAboveRangeMax());
  builder.field("log_scale", source.getLogScale());
  builder.field("reverse_log", source.getReverseLogScale());
  builder.field("grid_x", source.getGridXSize());
  builder.field("grid_y", source.getGridYSize());
  builder.field("grid_min", source.getGridSizeMinimumValue());
  builder.field("grid_max", source.getGridSizeMaximumValue());
  builder.field("alpha", source.getColorAlpha());
  builder.field("alpha_min", source.getColorAlphaMinimum());
  builder.field("alpha_max", source.getColorAlphaMaximum());
  writeBBox(builder, "bounds", source.getBounds());

  builder.beginArray("options");
  for (const auto& option : source.getMapSettings()) {
    serializeHeatMapOption(builder, option);
  }
  builder.endArray();

  builder.beginArray("legend");
  if (populated) {
    const auto& generator = source.getColorGenerator();
    const int color_count = generator.getColorCount();
    for (const auto& [color_index, color_value] : source.getLegendValues()) {
      builder.beginObject();
      builder.field("value", source.formatValue(color_value, true));
      const gui::Painter::Color color
          = generator.getColor(100.0 * color_index / std::max(1, color_count),
                               source.getColorAlpha());
      writeColorArray(builder, "color", color);
      builder.endObject();
    }
  }
  builder.endArray();

  builder.endObject();
}

static std::string buildHeatMapsPayloadLocked(SessionState& state)
{
  JsonBuilder builder;
  builder.beginObject();
  builder.field("active", state.active_heatmap);
  builder.beginArray("heatmaps");
  for (const auto& [name, source] : state.heatmaps) {
    serializeHeatMap(builder, *source, name == state.active_heatmap);
  }
  builder.endArray();
  builder.endObject();
  return builder.str();
}

//------------------------------------------------------------------------------
// dispatch_request — handles BOUNDS, TECH, TILE
//------------------------------------------------------------------------------

WebSocketResponse dispatch_request(
    const WebSocketRequest& req,
    const TileGenerator& gen,
    const std::vector<odb::Rect>& highlight_rects,
    const std::vector<odb::Polygon>& highlight_polys,
    const std::vector<ColoredRect>& colored_rects,
    const std::vector<FlightLine>& flight_lines,
    const std::map<uint32_t, Color>* module_colors,
    const std::set<uint32_t>* focus_net_ids,
    const std::set<uint32_t>* route_guide_net_ids)
{
  WebSocketResponse resp;
  resp.id = req.id;

  switch (req.type) {
    case WebSocketRequest::BOUNDS: {
      resp.type = 0;
      const odb::Rect bounds = gen.getBounds();
      JsonBuilder builder;
      builder.beginObject();
      builder.beginArray("bounds");
      builder.beginArray();
      builder.value(bounds.yMin());
      builder.value(bounds.xMin());
      builder.endArray();
      builder.beginArray();
      builder.value(bounds.yMax());
      builder.value(bounds.xMax());
      builder.endArray();
      builder.endArray();
      builder.field("shapes_ready", gen.shapesReady());
      builder.field("pin_max_size", gen.getPinMaxSize());
      builder.endObject();
      const std::string& json = builder.str();
      resp.payload.assign(json.begin(), json.end());
      break;
    }
    case WebSocketRequest::TECH: {
      resp.type = 0;
      JsonBuilder builder;
      builder.beginObject();
      builder.beginArray("layers");
      for (const auto& name : gen.getLayers()) {
        builder.value(name);
      }
      builder.endArray();
      builder.beginArray("sites");
      for (const auto& name : gen.getSites()) {
        builder.value(name);
      }
      builder.endArray();
      builder.field("has_liberty", gen.hasSta());
      if (gen.getBlock()) {
        builder.field("dbu_per_micron", gen.getBlock()->getDbUnitsPerMicron());
      }
      builder.endObject();
      const std::string& json = builder.str();
      resp.payload.assign(json.begin(), json.end());
      break;
    }
    case WebSocketRequest::TILE: {
      resp.type = 1;
      resp.payload = gen.generateTile(req.layer,
                                      req.z,
                                      req.x,
                                      req.y,
                                      req.vis,
                                      highlight_rects,
                                      highlight_polys,
                                      colored_rects,
                                      flight_lines,
                                      module_colors,
                                      focus_net_ids,
                                      route_guide_net_ids);
      break;
    }
    default: {
      resp.type = 2;
      const std::string err = "Unknown request type";
      resp.payload.assign(err.begin(), err.end());
      break;
    }
  }

  return resp;
}

//------------------------------------------------------------------------------
// SelectHandler
//------------------------------------------------------------------------------

SelectHandler::SelectHandler(std::shared_ptr<TileGenerator> gen,
                             std::shared_ptr<TclEvaluator> tcl_eval)
    : gen_(std::move(gen)), tcl_eval_(std::move(tcl_eval))
{
}

WebSocketResponse SelectHandler::handleSelect(const WebSocketRequest& req,
                                              SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    auto results = gen_->selectAt(req.select_x,
                                  req.select_y,
                                  req.select_zoom,
                                  req.vis,
                                  req.visible_layers);

    // STA's highlight() and getProperties() are not thread-safe;
    // serialize with other STA callers (timing, clock tree, tcl eval).
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);

    // Build JSON response with selection and properties
    resp.type = 0;
    JsonBuilder builder;
    builder.beginObject();
    builder.beginArray("selected");
    for (const auto& r : results) {
      builder.beginObject();
      builder.field("name", r.name);
      builder.field("type", r.type_name);
      writeBBox(builder, "bbox", r.bbox);
      builder.endObject();
    }
    builder.endArray();

    // Pick which result to inspect, cycling through overlapping objects.
    // If the currently inspected object is in the results, select the next one.
    std::vector<gui::Selected> new_selectables;
    auto* registry = gui::DescriptorRegistry::instance();
    gui::Selected inspected_sel;
    if (!results.empty()) {
      int pick = 0;
      if (results.size() > 1) {
        gui::Selected current;
        {
          std::lock_guard<std::mutex> lock(state.selection_mutex);
          current = state.current_inspected;
        }
        if (current) {
          for (int i = 0; i < static_cast<int>(results.size()); ++i) {
            gui::Selected candidate = registry->makeSelected(results[i].object);
            if (candidate == current) {
              pick = (i + 1) % static_cast<int>(results.size());
              break;
            }
          }
        }
      }
      inspected_sel = registry->makeSelected(results[pick].object);
      writeInspectPayload(
          builder, inspected_sel, new_selectables, /*can_navigate_back=*/false);
    } else {
      builder.field("can_navigate_back", 0);
    }
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      state.selectables = std::move(new_selectables);
    }
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.hover_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();
      collectHighlightShapes(
          inspected_sel, state.highlight_rects, state.highlight_polys);
      state.current_inspected = inspected_sel;
      state.navigation_history.clear();
    }

    builder.endObject();
    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleInspect(const WebSocketRequest& req,
                                               SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    gui::Selected sel;
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      if (req.select_id >= 0
          && req.select_id < static_cast<int>(state.selectables.size())) {
        sel = state.selectables[req.select_id];
      }
    }

    // STA's highlight() and getProperties() are not thread-safe;
    // serialize with other STA callers (timing, clock tree, tcl eval).
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);

    bool can_navigate_back = false;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.hover_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();
      collectHighlightShapes(sel, state.highlight_rects, state.highlight_polys);
      if (sel) {
        if (state.current_inspected && state.current_inspected != sel) {
          state.navigation_history.push_back(state.current_inspected);
        }
        state.current_inspected = sel;
      }
      can_navigate_back = !state.navigation_history.empty();
    }

    resp.type = 0;
    JsonBuilder builder;
    builder.beginObject();
    std::vector<gui::Selected> new_selectables;
    writeInspectPayload(builder, sel, new_selectables, can_navigate_back);
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      state.selectables = std::move(new_selectables);
    }
    builder.endObject();
    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleInspectBack(const WebSocketRequest& req,
                                                   SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    gui::Selected sel;
    bool can_navigate_back = false;

    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.hover_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();

      if (!state.navigation_history.empty()) {
        sel = state.navigation_history.back();
        state.navigation_history.pop_back();
        state.current_inspected = sel;
      } else {
        sel = state.current_inspected;
      }

      collectHighlightShapes(sel, state.highlight_rects, state.highlight_polys);
      can_navigate_back = !state.navigation_history.empty();
    }

    resp.type = 0;
    JsonBuilder builder;
    builder.beginObject();
    std::vector<gui::Selected> new_selectables;
    writeInspectPayload(builder, sel, new_selectables, can_navigate_back);
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      state.selectables = std::move(new_selectables);
    }
    builder.endObject();
    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleHover(const WebSocketRequest& req,
                                             SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    int count = 0;
    std::vector<odb::Rect> hover_rects;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.hover_rects.clear();

      if (req.select_id >= 0) {
        gui::Selected sel;
        {
          std::lock_guard<std::mutex> slock(state.selectables_mutex);
          if (req.select_id < static_cast<int>(state.selectables.size())) {
            sel = state.selectables[req.select_id];
          }
        }
        if (sel) {
          ShapeCollector collector;
          sel.highlight(collector);
          if (!collector.rects.empty()) {
            state.hover_rects = std::move(collector.rects);
          } else {
            odb::Rect bbox;
            if (sel.getBBox(bbox)) {
              state.hover_rects.push_back(bbox);
            }
          }
          count = static_cast<int>(state.hover_rects.size());
          hover_rects = state.hover_rects;
        }
      }
      // select_id < 0 just clears hover_rects (mouseleave)
    }

    resp.type = 0;
    JsonBuilder builder;
    builder.beginObject();
    builder.field("ok", 1);
    builder.field("count", count);
    builder.beginArray("rects");
    for (const auto& rect : hover_rects) {
      builder.beginArray();
      builder.value(rect.xMin());
      builder.value(rect.yMin());
      builder.value(rect.xMax());
      builder.value(rect.yMax());
      builder.endArray();
    }
    builder.endArray();
    builder.endObject();
    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSetFocusNets(const WebSocketRequest& req,
                                                    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    std::lock_guard<std::mutex> lock(state.focus_nets_mutex);
    if (req.focus_action == "clear") {
      state.focus_net_ids.clear();
    } else {
      odb::dbBlock* block = gen_->getBlock();
      odb::dbNet* net
          = block ? block->findNet(req.focus_net_name.c_str()) : nullptr;
      if (net) {
        if (req.focus_action == "add") {
          state.focus_net_ids.insert(net->getId());
        } else if (req.focus_action == "remove") {
          state.focus_net_ids.erase(net->getId());
        }
      }
    }
    const int count = static_cast<int>(state.focus_net_ids.size());
    const std::string json
        = R"({"ok":1,"count":)" + std::to_string(count) + "}";
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSetRouteGuides(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    std::lock_guard<std::mutex> lock(state.route_guides_mutex);
    if (req.route_guide_action == "clear") {
      state.route_guide_net_ids.clear();
    } else {
      odb::dbBlock* block = gen_->getBlock();
      odb::dbNet* net
          = block ? block->findNet(req.route_guide_net_name.c_str()) : nullptr;
      if (net) {
        if (req.route_guide_action == "add") {
          state.route_guide_net_ids.insert(net->getId());
        } else if (req.route_guide_action == "remove") {
          state.route_guide_net_ids.erase(net->getId());
        }
      }
    }
    const int count = static_cast<int>(state.route_guide_net_ids.size());
    const std::string json
        = R"({"ok":1,"count":)" + std::to_string(count) + "}";
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSnap(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    auto snap = gen_->snapAt(req.snap_x,
                             req.snap_y,
                             req.snap_radius,
                             req.snap_point_threshold,
                             req.snap_horizontal,
                             req.snap_vertical,
                             req.vis,
                             req.visible_layers);
    JsonBuilder builder;
    builder.beginObject();
    builder.field("found", snap.found);
    if (snap.found) {
      const bool is_point = snap.edge.first == snap.edge.second;
      builder.field("is_point", is_point);
      builder.beginArray("edge");
      builder.beginArray();
      builder.value(snap.edge.first.x());
      builder.value(snap.edge.first.y());
      builder.endArray();
      builder.beginArray();
      builder.value(snap.edge.second.x());
      builder.value(snap.edge.second.y());
      builder.endArray();
      builder.endArray();
    }
    builder.endObject();
    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    std::string err = std::string("snap error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

static const char* ioTypeToDirection(odb::dbIoType io_type)
{
  if (io_type == odb::dbIoType::INPUT) {
    return "input";
  }
  if (io_type == odb::dbIoType::OUTPUT) {
    return "output";
  }
  return "inout";
}

WebSocketResponse SelectHandler::handleSchematicCone(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;

  // Limits to prevent fanout explosions (e.g. clock net at depth > 0).
  static constexpr int kMaxConeInsts = 150;
  static constexpr int kMaxNetFanout = 30;

  try {
    odb::dbBlock* block = gen_->getBlock();
    if (!block) {
      throw std::runtime_error("No block loaded");
    }

    odb::dbInst* target_inst = block->findInst(req.schematic_inst_name.c_str());
    if (!target_inst) {
      throw std::runtime_error("Instance not found: "
                               + req.schematic_inst_name);
    }

    std::set<odb::dbInst*> all_insts;
    all_insts.insert(target_inst);
    bool cone_full = false;

    // Fanin BFS: follow input pins upstream to their driving instances.
    {
      std::vector<odb::dbInst*> level = {target_inst};
      std::set<odb::dbNet*> seen_nets;
      for (int d = 0; d < req.schematic_fanin_depth && !cone_full; ++d) {
        std::vector<odb::dbInst*> next_level;
        for (odb::dbInst* inst : level) {
          for (odb::dbITerm* iterm : inst->getITerms()) {
            if (iterm->getIoType() != odb::dbIoType::INPUT) {
              continue;
            }
            odb::dbNet* net = iterm->getNet();
            if (!net || seen_nets.contains(net)
                || static_cast<int>(net->getITerms().size()) > kMaxNetFanout) {
              continue;
            }
            seen_nets.insert(net);
            for (odb::dbITerm* drv : net->getITerms()) {
              if (drv->getIoType() != odb::dbIoType::OUTPUT) {
                continue;
              }
              odb::dbInst* drv_inst = drv->getInst();
              if (all_insts.insert(drv_inst).second) {
                next_level.push_back(drv_inst);
                if (static_cast<int>(all_insts.size()) >= kMaxConeInsts) {
                  cone_full = true;
                  break;
                }
              }
            }
            if (cone_full) {
              break;
            }
          }
          if (cone_full) {
            break;
          }
        }
        level = next_level;
      }
    }

    // Fanout BFS: follow output pins downstream to their load instances.
    {
      std::vector<odb::dbInst*> level = {target_inst};
      std::set<odb::dbNet*> seen_nets;
      for (int d = 0; d < req.schematic_fanout_depth && !cone_full; ++d) {
        std::vector<odb::dbInst*> next_level;
        for (odb::dbInst* inst : level) {
          for (odb::dbITerm* iterm : inst->getITerms()) {
            if (iterm->getIoType() != odb::dbIoType::OUTPUT) {
              continue;
            }
            odb::dbNet* net = iterm->getNet();
            if (!net || seen_nets.contains(net)
                || static_cast<int>(net->getITerms().size()) > kMaxNetFanout) {
              continue;
            }
            seen_nets.insert(net);
            for (odb::dbITerm* load : net->getITerms()) {
              if (load->getIoType() != odb::dbIoType::INPUT) {
                continue;
              }
              odb::dbInst* load_inst = load->getInst();
              if (all_insts.insert(load_inst).second) {
                next_level.push_back(load_inst);
                if (static_cast<int>(all_insts.size()) >= kMaxConeInsts) {
                  cone_full = true;
                  break;
                }
              }
            }
            if (cone_full) {
              break;
            }
          }
          if (cone_full) {
            break;
          }
        }
        level = next_level;
      }
    }

    // Collect all nets that touch any visited instance.
    std::map<odb::dbNet*, int> net_to_id;
    int next_net_id = 2;  // 0 = const-0, 1 = const-1 reserved by Yosys
    for (odb::dbInst* inst : all_insts) {
      for (odb::dbITerm* iterm : inst->getITerms()) {
        odb::dbNet* net = iterm->getNet();
        if (net && !net_to_id.contains(net)) {
          net_to_id[net] = next_net_id++;
        }
      }
    }

    JsonBuilder builder;
    builder.beginObject();
    builder.beginObject("modules");
    builder.beginObject("top");

    // Module-level attributes (required by Yosys JSON schema)
    builder.beginObject("attributes");
    builder.endObject();

    // Top-level ports (dbBTerm) connected to any visited net
    builder.beginObject("ports");
    for (const auto& [net, _id] : net_to_id) {
      for (odb::dbBTerm* bterm : net->getBTerms()) {
        builder.beginObject(bterm->getName());
        builder.field("direction", ioTypeToDirection(bterm->getIoType()));
        builder.beginArray("bits");
        builder.value(net_to_id[net]);
        builder.endArray();
        builder.endObject();
      }
    }
    builder.endObject();

    // Cells (instances)
    builder.beginObject("cells");
    for (odb::dbInst* inst : all_insts) {
      builder.beginObject(inst->getName());
      builder.field("hide_name", 0);
      // Use master cell name as type; fall back to "$unknown" for safety
      const std::string cell_type = inst->getMaster()
                                        ? inst->getMaster()->getName()
                                        : std::string("$unknown");
      builder.field("type", cell_type);
      // Required Yosys JSON fields (netlistsvg guards these with || {}, but
      // providing them avoids any version-specific surprises)
      builder.beginObject("attributes");
      builder.endObject();
      builder.beginObject("parameters");
      builder.endObject();

      builder.beginObject("port_directions");
      for (odb::dbITerm* iterm : inst->getITerms()) {
        if (!iterm->getNet() || !net_to_id.contains(iterm->getNet())) {
          continue;
        }
        builder.field(iterm->getMTerm()->getName(),
                      ioTypeToDirection(iterm->getIoType()));
      }
      builder.endObject();

      builder.beginObject("connections");
      for (odb::dbITerm* iterm : inst->getITerms()) {
        odb::dbNet* net = iterm->getNet();
        if (!net || !net_to_id.contains(net)) {
          continue;
        }
        builder.beginArray(iterm->getMTerm()->getName());
        builder.value(net_to_id[net]);
        builder.endArray();
      }
      builder.endObject();

      builder.endObject();
    }
    builder.endObject();

    // Net names for better labeling
    builder.beginObject("netnames");
    for (const auto& [net, net_id] : net_to_id) {
      builder.beginObject(net->getName());
      builder.field("hide_name", 0);
      builder.beginArray("bits");
      builder.value(net_id);
      builder.endArray();
      builder.beginObject("attributes");
      builder.endObject();
      builder.endObject();
    }
    builder.endObject();

    builder.endObject();  // top
    builder.endObject();  // modules
    builder.endObject();  // root

    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSchematicFull(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;

  try {
    odb::dbBlock* block = gen_->getBlock();
    if (!block) {
      throw std::runtime_error("No block loaded");
    }

    std::map<odb::dbNet*, int> net_to_id;
    int next_net_id = 2;
    for (odb::dbNet* net : block->getNets()) {
      net_to_id[net] = next_net_id++;
    }

    JsonBuilder builder;
    builder.beginObject();
    builder.beginObject("modules");
    builder.beginObject("top");

    builder.beginObject("attributes");
    builder.endObject();

    builder.beginObject("ports");
    for (odb::dbBTerm* bterm : block->getBTerms()) {
      odb::dbNet* net = bterm->getNet();
      if (!net) {
        continue;
      }
      builder.beginObject(bterm->getName());
      builder.field("direction", ioTypeToDirection(bterm->getIoType()));
      builder.beginArray("bits");
      builder.value(net_to_id[net]);
      builder.endArray();
      builder.endObject();
    }
    builder.endObject();

    builder.beginObject("cells");
    for (odb::dbInst* inst : block->getInsts()) {
      builder.beginObject(inst->getName());
      builder.field("hide_name", 0);
      const std::string cell_type = inst->getMaster()
                                        ? inst->getMaster()->getName()
                                        : std::string("$unknown");
      builder.field("type", cell_type);
      builder.beginObject("attributes");
      builder.endObject();
      builder.beginObject("parameters");
      builder.endObject();

      builder.beginObject("port_directions");
      for (odb::dbITerm* iterm : inst->getITerms()) {
        if (!iterm->getNet()) {
          continue;
        }
        builder.field(iterm->getMTerm()->getName(),
                      ioTypeToDirection(iterm->getIoType()));
      }
      builder.endObject();

      builder.beginObject("connections");
      for (odb::dbITerm* iterm : inst->getITerms()) {
        odb::dbNet* net = iterm->getNet();
        if (!net) {
          continue;
        }
        builder.beginArray(iterm->getMTerm()->getName());
        builder.value(net_to_id[net]);
        builder.endArray();
      }
      builder.endObject();

      builder.endObject();
    }
    builder.endObject();

    builder.beginObject("netnames");
    for (odb::dbNet* net : block->getNets()) {
      builder.beginObject(net->getName());
      builder.field("hide_name", 0);
      builder.beginArray("bits");
      builder.value(net_to_id[net]);
      builder.endArray();
      builder.beginObject("attributes");
      builder.endObject();
      builder.endObject();
    }
    builder.endObject();

    builder.endObject();  // top
    builder.endObject();  // modules
    builder.endObject();  // root

    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSchematicInspect(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;

  try {
    odb::dbBlock* block = gen_->getBlock();
    if (!block) {
      throw std::runtime_error("No block loaded");
    }

    odb::dbInst* inst = block->findInst(req.schematic_inst_name.c_str());
    if (!inst) {
      throw std::runtime_error("Instance not found: "
                               + req.schematic_inst_name);
    }

    gui::Selected sel = gui::DescriptorRegistry::instance()->makeSelected(inst);

    // STA's highlight() and getProperties() are not thread-safe;
    // serialize with other STA callers (timing, clock tree, tcl eval).
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);

    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.hover_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();
      collectHighlightShapes(sel, state.highlight_rects, state.highlight_polys);
      state.current_inspected = sel;
      state.navigation_history.clear();
    }

    JsonBuilder builder;
    builder.beginObject();
    std::vector<gui::Selected> new_selectables;
    writeInspectPayload(
        builder, sel, new_selectables, /*can_navigate_back=*/false);
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      state.selectables = std::move(new_selectables);
    }
    builder.endObject();

    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

//------------------------------------------------------------------------------
// TclHandler
//------------------------------------------------------------------------------

TclHandler::TclHandler(std::shared_ptr<TclEvaluator> tcl_eval)
    : tcl_eval_(std::move(tcl_eval))
{
}

WebSocketResponse TclHandler::handleTclEval(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    auto result = tcl_eval_->eval(req.tcl_cmd);
    JsonBuilder builder;
    builder.beginObject();
    builder.field("output", result.output);
    builder.field("result", result.result);
    builder.field("is_error", result.is_error);
    builder.endObject();
    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// Helper: find the start of the word at cursor_pos in line.
// Word boundaries are: whitespace, [, ], {, }
static int findWordStart(const std::string& line, int cursor_pos)
{
  static const std::string kBoundary = " \t\n\r[]{}";
  int pos = cursor_pos - 1;
  while (pos >= 0 && kBoundary.find(line[pos]) == std::string::npos) {
    --pos;
  }
  return pos + 1;
}

// Helper: find the enclosing command name for argument completion.
// Scans backwards from word_start past flags (-xxx) and their values
// to find the first non-flag word (or the first word after '[').
static std::string findEnclosingCommand(const std::string& line, int word_start)
{
  static const std::string kBoundary = " \t\n\r[]{}";
  // Collect all words before the current position
  std::vector<std::string> words;
  int pos = 0;
  while (pos < word_start) {
    // skip whitespace/boundaries
    while (pos < word_start && kBoundary.find(line[pos]) != std::string::npos) {
      if (line[pos] == '[') {
        // bracket resets context
        words.clear();
      }
      ++pos;
    }
    if (pos >= word_start) {
      break;
    }
    // extract word
    const int start = pos;
    while (pos < word_start && kBoundary.find(line[pos]) == std::string::npos) {
      ++pos;
    }
    words.push_back(line.substr(start, pos - start));
  }

  // Walk backwards to find the first non-flag word
  for (int i = static_cast<int>(words.size()) - 1; i >= 0; --i) {
    if (!words[i].empty() && words[i][0] != '-') {
      return words[i];
    }
  }
  return {};
}

// Evaluate a Tcl command that returns a list, sort it, and return
// the elements as a vector of strings.  Returns empty on error.
static std::vector<std::string> getTclList(TclEvaluator& eval,
                                           const std::string& tcl_cmd)
{
  auto result = eval.eval("join [lsort [" + tcl_cmd + "]] \\n");
  std::vector<std::string> items;
  if (result.is_error) {
    return items;
  }
  std::istringstream stream(result.result);
  std::string item;
  while (std::getline(stream, item)) {
    if (!item.empty()) {
      items.push_back(std::move(item));
    }
  }
  return items;
}

WebSocketResponse TclHandler::handleTclComplete(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    const std::string& line = req.tcl_complete_line;
    int cursor_pos = req.tcl_complete_cursor_pos;
    if (cursor_pos < 0) {
      cursor_pos = static_cast<int>(line.size());
    }
    cursor_pos = std::min(cursor_pos, static_cast<int>(line.size()));

    const int word_start = findWordStart(line, cursor_pos);
    const std::string prefix = line.substr(word_start, cursor_pos - word_start);

    std::string mode;
    std::vector<std::string> completions;

    if (!prefix.empty() && prefix[0] == '$') {
      // Variable completion
      mode = "variables";
      const std::string var_prefix = prefix.substr(1);  // strip $
      const bool starts_with_colon
          = !var_prefix.empty() && var_prefix[0] == ':';
      std::string tcl_cmd = "info vars " + var_prefix;
      if (!var_prefix.empty() && var_prefix.back() == ':'
          && (var_prefix.size() == 1
              || var_prefix[var_prefix.size() - 2] != ':')) {
        tcl_cmd += ":";
      }
      tcl_cmd += "*";

      for (auto var : getTclList(*tcl_eval_, tcl_cmd)) {
        if (!starts_with_colon && !var.empty() && var[0] == ':') {
          var = var.substr(2);
        }
        completions.push_back("$" + var);
      }

      // Add namespaces
      for (const auto& ns : getTclList(*tcl_eval_, "namespace children")) {
        std::string name = ns;
        if (!starts_with_colon && !name.empty() && name[0] == ':') {
          name = name.substr(2);
        }
        completions.push_back("$" + name);
      }
    } else if (!prefix.empty() && prefix[0] == '-') {
      // Argument completion
      mode = "arguments";
      const std::string cmd_name = findEnclosingCommand(line, word_start);
      if (!cmd_name.empty()) {
        std::string tcl_cmd = "if {[info exists sta::cmd_args(" + cmd_name
                              + ")]} { set sta::cmd_args(" + cmd_name
                              + ") } else { list }";
        auto result = tcl_eval_->eval(tcl_cmd);
        if (!result.is_error && !result.result.empty()) {
          // Parse flags with regex
          static const std::regex kArgMatcher("-[a-zA-Z0-9_]+");
          const std::string args_str = result.result;
          std::sregex_iterator it(
              args_str.begin(), args_str.end(), kArgMatcher);
          std::sregex_iterator end;
          std::set<std::string> unique_args;
          while (it != end) {
            unique_args.insert(it->str());
            ++it;
          }
          for (const auto& arg : unique_args) {
            if (prefix.size() <= 1 || arg.substr(0, prefix.size()) == prefix) {
              completions.push_back(arg);
            }
          }
        }
      }
    } else {
      // Command completion
      mode = "commands";
      // Get OpenROAD registered commands
      for (auto& cmd : getTclList(*tcl_eval_, "array names sta::cmd_args")) {
        completions.push_back(std::move(cmd));
      }
      // Get namespace commands
      for (const auto& ns : getTclList(*tcl_eval_, "namespace children")) {
        for (auto ns_cmd :
             getTclList(*tcl_eval_, "info commands " + ns + "::*")) {
          // Remove leading ::
          if (ns_cmd.size() > 2 && ns_cmd[0] == ':' && ns_cmd[1] == ':') {
            ns_cmd = ns_cmd.substr(2);
          }
          completions.push_back(std::move(ns_cmd));
        }
      }

      // Filter by prefix if non-empty
      if (!prefix.empty()) {
        const bool add_colons = prefix[0] == ':';
        std::vector<std::string> filtered;
        for (const auto& c : completions) {
          std::string match_target = c;
          if (add_colons && !c.empty() && c[0] != ':') {
            match_target = "::" + c;
          }
          if (match_target.substr(0, prefix.size()) == prefix) {
            filtered.push_back(add_colons && c[0] != ':' ? "::" + c : c);
          }
        }
        completions = std::move(filtered);
      }
    }

    JsonBuilder builder;
    builder.beginObject();
    builder.beginArray("completions");
    for (const auto& c : completions) {
      builder.value(c);
    }
    builder.endArray();
    builder.field("mode", mode);
    builder.field("prefix", prefix);
    builder.field("replace_start", word_start);
    builder.field("replace_end", cursor_pos);
    builder.endObject();
    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

//------------------------------------------------------------------------------
// TimingHandler
//------------------------------------------------------------------------------

TimingHandler::TimingHandler(std::shared_ptr<TileGenerator> gen,
                             std::shared_ptr<TimingReport> timing_report,
                             std::shared_ptr<TclEvaluator> tcl_eval)
    : gen_(std::move(gen)),
      timing_report_(std::move(timing_report)),
      tcl_eval_(std::move(tcl_eval))
{
}

WebSocketResponse TimingHandler::handleTimingReport(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    auto paths = timing_report_->getReport(req.timing_is_setup,
                                           req.timing_max_paths,
                                           req.timing_slack_min,
                                           req.timing_slack_max);
    JsonBuilder builder;
    builder.beginObject();
    builder.beginArray("paths");
    for (const auto& p : paths) {
      builder.beginObject();
      builder.field("start_clk", p.start_clk);
      builder.field("end_clk", p.end_clk);
      builder.field("required", p.required);
      builder.field("arrival", p.arrival);
      builder.field("slack", p.slack);
      builder.field("skew", p.skew);
      builder.field("path_delay", p.path_delay);
      builder.field("logic_depth", p.logic_depth);
      builder.field("fanout", p.fanout);
      builder.field("start_pin", p.start_pin);
      builder.field("end_pin", p.end_pin);
      builder.beginArray("data_nodes");
      for (const auto& n : p.data_nodes) {
        serializeTimingNode(builder, n);
      }
      builder.endArray();
      builder.beginArray("capture_nodes");
      for (const auto& n : p.capture_nodes) {
        serializeTimingNode(builder, n);
      }
      builder.endArray();
      builder.endObject();
    }
    builder.endArray();
    builder.endObject();
    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TimingHandler::handleTimingHighlight(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    std::vector<ColoredRect> new_rects;
    std::vector<FlightLine> new_lines;

    if (req.timing_path_index >= 0) {
      std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
      auto paths = timing_report_->getReport(req.timing_highlight_setup);
      if (req.timing_path_index < static_cast<int>(paths.size())) {
        odb::dbBlock* block = gen_->getBlock();
        collectTimingPathShapes(
            block, paths[req.timing_path_index], new_rects, new_lines);

        if (!req.timing_pin_name.empty()) {
          static const Color kStageColor{.r = 255, .g = 255, .b = 0, .a = 180};
          auto [iterm, bterm] = resolvePin(block, req.timing_pin_name);

          odb::dbNet* net = nullptr;
          if (iterm) {
            net = iterm->getNet();
          } else if (bterm) {
            net = bterm->getNet();
          }

          if (net) {
            collectNetShapes(net,
                             iterm,
                             bterm,
                             nullptr,
                             nullptr,
                             kStageColor,
                             new_rects,
                             new_lines);
          }
        }
      }
    }

    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.timing_rects = std::move(new_rects);
      state.timing_lines = std::move(new_lines);
      state.highlight_rects.clear();
      state.highlight_polys.clear();
    }

    const std::string json = "{\"ok\": true}";
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TimingHandler::handleSlackHistogram(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    auto histogram = timing_report_->getSlackHistogram(
        req.histogram_is_setup, req.histogram_path_group, req.histogram_clock);
    JsonBuilder builder;
    builder.beginObject();
    builder.beginArray("bins");
    for (const auto& bin : histogram.bins) {
      builder.beginObject();
      builder.field("lower", bin.lower);
      builder.field("upper", bin.upper);
      builder.field("count", bin.count);
      builder.field("negative", bin.is_negative);
      builder.endObject();
    }
    builder.endArray();
    builder.field("unconstrained_count", histogram.unconstrained_count);
    builder.field("total_endpoints", histogram.total_endpoints);
    builder.field("time_unit", histogram.time_unit);
    builder.endObject();
    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TimingHandler::handleChartFilters(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    auto filters = timing_report_->getChartFilters();
    JsonBuilder builder;
    builder.beginObject();
    builder.beginArray("path_groups");
    for (const auto& name : filters.path_groups) {
      builder.value(name);
    }
    builder.endArray();
    builder.beginArray("clocks");
    for (const auto& name : filters.clocks) {
      builder.value(name);
    }
    builder.endArray();
    builder.endObject();
    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

//------------------------------------------------------------------------------
// ClockTreeHandler
//------------------------------------------------------------------------------

ClockTreeHandler::ClockTreeHandler(
    std::shared_ptr<TileGenerator> gen,
    std::shared_ptr<ClockTreeReport> clock_report,
    std::shared_ptr<TclEvaluator> tcl_eval)
    : gen_(std::move(gen)),
      clock_report_(std::move(clock_report)),
      tcl_eval_(std::move(tcl_eval))
{
}

WebSocketResponse ClockTreeHandler::handleClockTree(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    auto clocks = clock_report_->getReport();
    JsonBuilder builder;
    builder.beginObject();
    builder.beginArray("clocks");
    for (const auto& clk : clocks) {
      builder.beginObject();
      builder.field("name", clk.clock_name);
      builder.field("min_arrival", clk.min_arrival);
      builder.field("max_arrival", clk.max_arrival);
      builder.field("time_unit", clk.time_unit);
      builder.beginArray("nodes");
      for (const auto& n : clk.nodes) {
        builder.beginObject();
        builder.field("id", n.id);
        builder.field("parent_id", n.parent_id);
        builder.field("name", n.name);
        builder.field("pin_name", n.pin_name);
        builder.field("type", ClockTreeNode::typeToString(n.type));
        builder.field("arrival", n.arrival);
        builder.field("delay", n.delay);
        builder.field("fanout", n.fanout);
        builder.field("level", n.level);
        builder.field("dbu_x", n.dbu_x);
        builder.field("dbu_y", n.dbu_y);
        builder.endObject();
      }
      builder.endArray();
      builder.endObject();
    }
    builder.endArray();
    builder.endObject();
    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse ClockTreeHandler::handleClockTreeHighlight(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    std::lock_guard<std::mutex> lock(state.selection_mutex);
    state.highlight_rects.clear();
    state.highlight_polys.clear();
    state.timing_rects.clear();
    state.timing_lines.clear();

    if (!req.clock_tree_inst_name.empty()) {
      odb::dbBlock* block = gen_->getBlock();
      if (block) {
        odb::dbInst* inst = block->findInst(req.clock_tree_inst_name.c_str());
        if (inst) {
          state.highlight_rects.push_back(inst->getBBox()->getBox());
        }
      }
    }

    const std::string json = "{\"ok\": true}";
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

//------------------------------------------------------------------------------
// TileHandler
//------------------------------------------------------------------------------

TileHandler::TileHandler(std::shared_ptr<TileGenerator> gen)
    : gen_(std::move(gen))
{
}

void TileHandler::initializeHeatMaps(SessionState& state)
{
  std::lock_guard<std::mutex> lock(state.heatmap_mutex);
  state.heatmaps.clear();
  for (const auto& source_handle : gui::getRegisteredHeatMapSources()) {
    auto source = source_handle->createInstance();
    source->setChip(gen_->getChip());
    state.heatmaps[source_handle->getShortName()] = std::move(source);
  }
}

WebSocketResponse TileHandler::handleTile(const WebSocketRequest& req,
                                          SessionState& state)
{
  // Snapshot current highlight state
  std::vector<odb::Rect> rects;
  std::vector<odb::Polygon> polys;
  std::vector<ColoredRect> colored;
  std::vector<FlightLine> lines;
  {
    std::lock_guard<std::mutex> lock(state.selection_mutex);
    rects = state.highlight_rects;
    rects.insert(
        rects.end(), state.hover_rects.begin(), state.hover_rects.end());
    polys = state.highlight_polys;
    colored = state.timing_rects;
    lines = state.timing_lines;
  }

  // Snapshot module colors for _modules layer
  std::map<uint32_t, Color> mod_colors;
  {
    std::lock_guard<std::mutex> lock(state.module_colors_mutex);
    mod_colors = state.module_colors;
  }
  const std::map<uint32_t, Color>* mod_ptr
      = mod_colors.empty() ? nullptr : &mod_colors;

  // Snapshot focus nets
  std::set<uint32_t> focus_nets;
  {
    std::lock_guard<std::mutex> lock(state.focus_nets_mutex);
    focus_nets = state.focus_net_ids;
  }
  const std::set<uint32_t>* focus_ptr
      = focus_nets.empty() ? nullptr : &focus_nets;

  // Snapshot route guide nets
  std::set<uint32_t> route_guides;
  {
    std::lock_guard<std::mutex> lock(state.route_guides_mutex);
    route_guides = state.route_guide_net_ids;
  }
  const std::set<uint32_t>* route_guide_ptr
      = route_guides.empty() ? nullptr : &route_guides;

  return dispatch_request(req,
                          *gen_,
                          rects,
                          polys,
                          colored,
                          lines,
                          mod_ptr,
                          focus_ptr,
                          route_guide_ptr);
}

WebSocketResponse TileHandler::handleModuleHierarchy(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    odb::dbBlock* block = gen_->getBlock();
    HierarchyReport report(block, gen_->getSta());
    auto result = report.getReport();

    JsonBuilder builder;
    builder.beginObject();
    builder.beginArray("nodes");
    for (const auto& n : result.nodes) {
      builder.beginObject();
      builder.field("id", n.id);
      builder.field("parent_id", n.parent_id);
      builder.field("inst_name", n.inst_name);
      builder.field("module_name", n.module_name);
      builder.field("insts", n.insts);
      builder.field("macros", n.macros);
      builder.field("modules", n.modules);
      builder.field("area", n.area);
      builder.field("local_insts", n.local_insts);
      builder.field("local_macros", n.local_macros);
      builder.field("local_modules", n.local_modules);
      if (n.node_kind != HierarchyNodeKind::MODULE) {
        builder.field("node_kind", static_cast<int>(n.node_kind));
      }
      if (n.node_kind == HierarchyNodeKind::MODULE) {
        builder.field("odb_id", static_cast<int>(n.odb_id));
      }
      builder.endObject();
    }
    builder.endArray();
    builder.endObject();
    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TileHandler::handleSetModuleColors(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;

  // Parse compact format: "id:r,g,b,a;id:r,g,b,a;..."
  std::map<uint32_t, Color> colors;
  const std::string data = extract_string(req.vis.raw_json_, "colors");
  if (!data.empty()) {
    size_t pos = 0;
    while (pos < data.size()) {
      size_t colon = data.find(':', pos);
      if (colon == std::string::npos) {
        break;
      }
      const uint32_t mod_id
          = static_cast<uint32_t>(std::stoul(data.substr(pos, colon - pos)));
      pos = colon + 1;

      auto next_num = [&]() -> int {
        size_t end = data.find_first_of(",;", pos);
        if (end == std::string::npos) {
          end = data.size();
        }
        const int val = std::stoi(data.substr(pos, end - pos));
        pos = end + 1;
        return val;
      };

      const uint8_t r = static_cast<uint8_t>(next_num());
      const uint8_t g = static_cast<uint8_t>(next_num());
      const uint8_t b = static_cast<uint8_t>(next_num());
      const uint8_t a = static_cast<uint8_t>(next_num());
      colors[mod_id] = Color{.r = r, .g = g, .b = b, .a = a};
    }
  }

  const int count = static_cast<int>(colors.size());
  {
    std::lock_guard<std::mutex> lock(state.module_colors_mutex);
    state.module_colors = std::move(colors);
  }

  const std::string ok = R"({"ok":1,"count":)" + std::to_string(count) + "}";
  resp.payload.assign(ok.begin(), ok.end());
  return resp;
}

WebSocketResponse TileHandler::handleHeatMaps(const WebSocketRequest& req,
                                              SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    const std::string json = buildHeatMapsPayloadLocked(state);
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TileHandler::handleSetActiveHeatMap(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    std::lock_guard<std::mutex> lock(state.heatmap_mutex);
    if (!state.active_heatmap.empty()) {
      auto current = state.heatmaps.find(state.active_heatmap);
      if (current != state.heatmaps.end()) {
        current->second->onHide();
      }
    }

    state.active_heatmap.clear();
    if (!req.heatmap_name.empty()) {
      auto next = state.heatmaps.find(req.heatmap_name);
      if (next == state.heatmaps.end()) {
        throw std::runtime_error("invalid heat map");
      }
      state.active_heatmap = req.heatmap_name;
      next->second->onShow();
    }

    const std::string json = buildHeatMapsPayloadLocked(state);
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TileHandler::handleSetHeatMap(const WebSocketRequest& req,
                                                SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    std::lock_guard<std::mutex> lock(state.heatmap_mutex);
    auto source_itr = state.heatmaps.find(req.heatmap_name);
    if (source_itr == state.heatmaps.end()) {
      throw std::runtime_error("invalid heat map");
    }

    auto& source = *source_itr->second;
    if (req.heatmap_option == "rebuild") {
      source.destroyMap();
      source.ensureMap();
    } else {
      auto settings = source.getSettings();
      auto setting_itr = settings.find(req.heatmap_option);
      if (setting_itr == settings.end()) {
        throw std::runtime_error("invalid heat map option");
      }

      const auto& current_value = setting_itr->second;
      if (std::holds_alternative<bool>(current_value)) {
        settings[req.heatmap_option] = extract_bool_value(req.raw_json);
      } else if (std::holds_alternative<int>(current_value)) {
        settings[req.heatmap_option] = extract_int(req.raw_json, "value");
      } else if (std::holds_alternative<double>(current_value)) {
        settings[req.heatmap_option] = extract_double_value(req.raw_json);
      } else {
        settings[req.heatmap_option] = req.heatmap_string_value;
      }
      source.setSettings(settings);
    }

    const std::string json = buildHeatMapsPayloadLocked(state);
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TileHandler::handleHeatMapTile(const WebSocketRequest& req,
                                                 SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 1;
  try {
    std::shared_ptr<gui::HeatMapDataSource> source;
    {
      std::lock_guard<std::mutex> lock(state.heatmap_mutex);
      const std::string name
          = req.heatmap_name.empty() ? state.active_heatmap : req.heatmap_name;
      auto source_itr = state.heatmaps.find(name);
      if (source_itr == state.heatmaps.end()) {
        throw std::runtime_error("invalid heat map");
      }
      source = source_itr->second;
    }
    resp.payload = gen_->generateHeatMapTile(*source, req.z, req.x, req.y);
  } catch (const std::exception& e) {
    resp.type = 2;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse handleListDir(const WebSocketRequest& req)
{
  namespace fs = std::filesystem;

  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = 0;

  try {
    fs::path dir_path
        = req.dir_path.empty() ? fs::current_path() : fs::path(req.dir_path);
    dir_path = fs::canonical(dir_path);

    struct Entry
    {
      std::string name;
      bool is_dir;
      std::uintmax_t size;
    };
    std::vector<Entry> entries;

    for (const auto& entry : fs::directory_iterator(
             dir_path, fs::directory_options::skip_permission_denied)) {
      const auto& name = entry.path().filename().string();
      // Skip hidden files/directories.
      if (!name.empty() && name[0] == '.') {
        continue;
      }
      bool is_dir = entry.is_directory();
      std::uintmax_t size = 0;
      if (!is_dir) {
        std::error_code ec;
        size = entry.file_size(ec);
        if (ec) {
          size = 0;
        }
      }
      entries.push_back({name, is_dir, size});
    }

    // Sort: directories first, then alphabetical within each group.
    std::ranges::sort(entries, [](const Entry& a, const Entry& b) {
      if (a.is_dir != b.is_dir) {
        return a.is_dir > b.is_dir;
      }
      return a.name < b.name;
    });

    JsonBuilder builder;
    builder.beginObject();
    builder.field("path", dir_path.string());
    builder.field("parent", dir_path.parent_path().string());
    builder.beginArray("entries");
    for (const auto& entry : entries) {
      builder.beginObject();
      builder.field("name", entry.name);
      builder.field("is_dir", entry.is_dir);
      if (!entry.is_dir) {
        builder.field("size", static_cast<int>(entry.size));
      }
      builder.endObject();
    }
    builder.endArray();
    builder.endObject();

    const std::string& json = builder.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    std::string err = std::string("list_dir error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

}  // namespace web
