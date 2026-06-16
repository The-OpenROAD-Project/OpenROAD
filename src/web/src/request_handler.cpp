// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "request_handler.h"

#include <algorithm>
#include <any>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#include "boost/json/array.hpp"
#include "boost/json/object.hpp"
#include "boost/json/serialize.hpp"
#include "boost/json/value.hpp"
#include "cli_completer.h"
#include "clock_tree_report.h"
#include "color.h"
#include "gui/descriptor_registry.h"
#include "gui/gui.h"
#include "gui/heatMap.h"
#include "hierarchy_report.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "request_dispatcher.h"
#include "tile_generator.h"
#include "timing_report.h"
#include "utl/Logger.h"
#include "utl/algorithms.h"

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

namespace {

// Read a JSON array of strings as a std::set.  Used for the visible_layers
// field which arrives as ["metal1", "metal2", ...].  Throws when the value
// is not an array of strings — that's a contract violation.
std::set<std::string> arrayAsStringSet(const boost::json::value& v)
{
  std::set<std::string> out;
  for (const auto& item : v.as_array()) {
    out.emplace(item.as_string());
  }
  return out;
}

// Build a JSON-array bbox: [xMin, yMin, xMax, yMax].
boost::json::array bboxArray(const odb::Rect& r)
{
  return boost::json::array{r.xMin(), r.yMin(), r.xMax(), r.yMax()};
}

// Build a JSON-array RGBA: [r, g, b, a].
boost::json::array colorArray(const gui::Painter::Color& c)
{
  return boost::json::array{c.r, c.g, c.b, c.a};
}

// Serialize a boost::json::value into a WebSocketResponse's payload.
void writePayload(WebSocketResponse& resp, const boost::json::value& v)
{
  std::string s = boost::json::serialize(v);
  resp.payload.assign(s.begin(), s.end());
}

}  // namespace

// RAII helper: temporarily sets Descriptor::Property::convert_dbu to a
// micron-aware formatter (matching the Qt GUI's default) for the lifetime
// of the scope.  When `use_dbu` is true the default identity formatter is
// kept so that raw DBU integers are emitted.  Must be held while the
// sta_lock mutex is held — the global static is not otherwise thread-safe.
class [[nodiscard]] ScopedDbuFormat
{
 public:
  ScopedDbuFormat(odb::dbBlock* block, bool use_dbu)
      : saved_(gui::Descriptor::Property::convert_dbu)
  {
    if (use_dbu || !block) {
      return;  // keep default (raw DBU)
    }
    const double dbu_per_micron = block->getDbUnitsPerMicron();
    const int precision
        = static_cast<int>(std::ceil(std::log10(dbu_per_micron)));
    gui::Descriptor::Property::convert_dbu
        = [dbu_per_micron, precision](int value, bool add_units) {
            auto str = utl::to_numeric_string(
                static_cast<double>(value) / dbu_per_micron, precision);
            if (add_units) {
              str += " \xC2\xB5m";  // UTF-8 µm
            }
            return str;
          };
  }
  ~ScopedDbuFormat() { gui::Descriptor::Property::convert_dbu = saved_; }
  ScopedDbuFormat(const ScopedDbuFormat&) = delete;
  ScopedDbuFormat& operator=(const ScopedDbuFormat&) = delete;

 private:
  gui::DBUToString saved_;
};

// Store a Selected in the clickables vector and return its index.
static int storeSelectable(std::vector<gui::Selected>& selectables,
                           const gui::Selected& sel)
{
  int id = static_cast<int>(selectables.size());
  selectables.push_back(sel);
  return id;
}

static void serializeAnyValue(boost::json::object& out,
                              std::string_view field_name,
                              const std::any& value,
                              std::vector<gui::Selected>& selectables,
                              bool short_name = false)
{
  if (auto* sel = std::any_cast<gui::Selected>(&value)) {
    if (*sel) {
      const std::string name
          = short_name ? sel->getShortName() : sel->getName();
      int id = storeSelectable(selectables, *sel);
      out[field_name] = name;
      out[std::string(field_name) + "_select_id"] = id;
      return;
    }
  }
  out[field_name] = gui::Descriptor::Property::toString(value);
}

static boost::json::object serializeProperty(
    const gui::Descriptor::Property& prop,
    std::vector<gui::Selected>& selectables)
{
  boost::json::object o;
  o["name"] = prop.name;

  if (auto* plist = std::any_cast<gui::Descriptor::PropertyList>(&prop.value)) {
    boost::json::array children;
    children.reserve(plist->size());
    for (const auto& [key, val] : *plist) {
      boost::json::object child;
      serializeAnyValue(child, "name", key, selectables, /*short_name=*/true);
      serializeAnyValue(child, "value", val, selectables);
      children.emplace_back(std::move(child));
    }
    o["children"] = std::move(children);
  } else if (auto* sel_set = std::any_cast<gui::SelectionSet>(&prop.value)) {
    boost::json::array children;
    children.reserve(sel_set->size());
    for (const auto& sel : *sel_set) {
      boost::json::object child;
      int id = storeSelectable(selectables, sel);
      child["name"] = sel.getName();
      child["name_select_id"] = id;
      children.emplace_back(std::move(child));
    }
    o["children"] = std::move(children);
  } else if (auto* sel = std::any_cast<gui::Selected>(&prop.value)) {
    if (*sel) {
      int id = storeSelectable(selectables, *sel);
      o["value"] = sel->getName();
      o["value_select_id"] = id;
    }
  } else {
    o["value"] = prop.toString();
  }
  return o;
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

// Return the 0-based position of the iterator within the selection set,
// or -1 if the set is empty.  Mirrors Qt GUI's
// Inspector::getSelectedIteratorPosition().
static int selectionIteratorPosition(const gui::SelectionSet& set,
                                     gui::SelectionSet::const_iterator itr)
{
  if (set.empty() || itr == set.end()) {
    return -1;
  }
  return static_cast<int>(std::distance(set.begin(), itr));
}

// Accumulate highlight shapes from all items in a selection set.
static void collectMultiHighlightShapes(const gui::SelectionSet& selections,
                                        std::vector<odb::Rect>& rects,
                                        std::vector<odb::Polygon>& polys)
{
  rects.clear();
  polys.clear();
  for (const auto& sel : selections) {
    if (!sel) {
      continue;
    }
    ShapeCollector collector;
    sel.highlight(collector);
    rects.insert(rects.end(), collector.rects.begin(), collector.rects.end());
    polys.insert(polys.end(), collector.polys.begin(), collector.polys.end());
  }
}

static void writeInspectPayload(boost::json::object& o,
                                const gui::Selected& sel,
                                std::vector<gui::Selected>& new_selectables,
                                bool can_navigate_back)
{
  o["can_navigate_back"] = can_navigate_back ? 1 : 0;
  if (!sel) {
    o["error"] = "invalid select_id";
    return;
  }

  auto props = sel.getProperties();
  o["name"] = sel.getName();
  o["type"] = sel.getTypeName();
  boost::json::array prop_arr;
  prop_arr.reserve(props.size());
  for (const auto& prop : props) {
    prop_arr.emplace_back(serializeProperty(prop, new_selectables));
  }
  o["properties"] = std::move(prop_arr);

  odb::Rect bbox;
  if (sel.getBBox(bbox)) {
    o["bbox"] = bboxArray(bbox);
  }

  if (sel.isNet()) {
    auto* net = std::any_cast<odb::dbNet*>(sel.getObject());
    if (net && !net->getGuides().empty()) {
      o["has_guides"] = 1;
    }
  }
}

static boost::json::object serializeHeatMapOption(
    const gui::HeatMapDataSource::MapSetting& option)
{
  boost::json::object o;
  if (std::holds_alternative<gui::HeatMapDataSource::MapSettingBoolean>(
          option)) {
    const auto& setting
        = std::get<gui::HeatMapDataSource::MapSettingBoolean>(option);
    o["type"] = "bool";
    o["name"] = setting.name;
    o["label"] = setting.label;
    o["value"] = setting.getter();
  } else {
    const auto& setting
        = std::get<gui::HeatMapDataSource::MapSettingMultiChoice>(option);
    o["type"] = "choice";
    o["name"] = setting.name;
    o["label"] = setting.label;
    o["value"] = setting.getter();
    boost::json::array choices;
    for (const auto& choice : setting.choices()) {
      choices.emplace_back(choice);
    }
    o["choices"] = std::move(choices);
  }
  return o;
}

static boost::json::object serializeHeatMap(gui::HeatMapDataSource& source,
                                            const bool active)
{
  if (active) {
    source.ensureMap();
  }
  const bool populated = source.isPopulated();
  const bool has_data = source.hasData();

  boost::json::object o;
  o["name"] = source.getShortName();
  o["title"] = source.getName();
  o["active"] = active;
  o["settings_group"] = source.getSettingsGroupName();
  o["has_data"] = has_data;
  o["can_adjust_grid"] = source.canAdjustGrid();
  o["show_numbers"] = source.getShowNumbers();
  o["show_legend"] = source.getShowLegend();
  o["supports_numbers"] = true;
  o["units"] = source.getValueUnits();
  o["display_range_increment"] = source.getDisplayRangeIncrement();
  o["display_min"] = source.convertPercentToValue(source.getDisplayRangeMin());
  o["display_max"] = source.convertPercentToValue(source.getDisplayRangeMax());
  o["display_min_limit"]
      = source.convertPercentToValue(source.getDisplayRangeMinimumValue());
  o["display_max_limit"]
      = source.convertPercentToValue(source.getDisplayRangeMaximumValue());
  o["draw_below_min"] = source.getDrawBelowRangeMin();
  o["draw_above_max"] = source.getDrawAboveRangeMax();
  o["log_scale"] = source.getLogScale();
  o["reverse_log"] = source.getReverseLogScale();
  o["grid_x"] = source.getGridXSize();
  o["grid_y"] = source.getGridYSize();
  o["grid_min"] = source.getGridSizeMinimumValue();
  o["grid_max"] = source.getGridSizeMaximumValue();
  o["alpha"] = source.getColorAlpha();
  o["alpha_min"] = source.getColorAlphaMinimum();
  o["alpha_max"] = source.getColorAlphaMaximum();
  o["bounds"] = bboxArray(source.getBounds());

  boost::json::array options;
  for (const auto& option : source.getMapSettings()) {
    options.emplace_back(serializeHeatMapOption(option));
  }
  o["options"] = std::move(options);

  boost::json::array legend;
  if (populated) {
    const auto& generator = source.getColorGenerator();
    const int color_count = generator.getColorCount();
    for (const auto& [color_index, color_value] : source.getLegendValues()) {
      boost::json::object entry;
      entry["value"] = source.formatValue(color_value, true);
      const gui::Painter::Color color
          = generator.getColor(100.0 * color_index / std::max(1, color_count),
                               source.getColorAlpha());
      entry["color"] = colorArray(color);
      legend.emplace_back(std::move(entry));
    }
  }
  o["legend"] = std::move(legend);
  return o;
}

static std::string buildHeatMapsPayloadLocked(SessionState& state)
{
  boost::json::object root;
  root["active"] = state.active_heatmap;
  boost::json::array heatmaps;
  for (const auto& [name, source] : state.heatmaps) {
    heatmaps.emplace_back(
        serializeHeatMap(*source, name == state.active_heatmap));
  }
  root["heatmaps"] = std::move(heatmaps);
  return boost::json::serialize(root);
}

WebSocketResponse TileHandler::serializeBounds(const uint32_t id,
                                               const TileGenerator& gen)
{
  WebSocketResponse resp;
  resp.id = id;
  resp.type = WebSocketResponse::kJson;
  writePayload(resp, serializeBoundsResponse(gen, gen.shapesReady()));
  return resp;
}

WebSocketResponse TileHandler::serializeTech(const uint32_t id,
                                             const TileGenerator& gen)
{
  WebSocketResponse resp;
  resp.id = id;
  resp.type = WebSocketResponse::kJson;
  writePayload(resp, serializeTechResponse(gen));
  return resp;
}

WebSocketResponse TileHandler::renderTile(
    const uint32_t id,
    const std::string& layer,
    const int z,
    const int x,
    const int y,
    const TileVisibility& vis,
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
  resp.id = id;
  resp.type = WebSocketResponse::kPng;
  resp.payload = gen.generateTile(layer,
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

void SelectHandler::registerRequests(RequestDispatcher& d)
{
  d.add("select",
        WebSocketRequest::kSelect,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSelect(req, state);
        });
  d.add("inspect",
        WebSocketRequest::kInspect,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleInspect(req, state);
        });
  d.add("inspect_back",
        WebSocketRequest::kInspectBack,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleInspectBack(req, state);
        });
  d.add("hover",
        WebSocketRequest::kHover,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleHover(req, state);
        });
  d.add("snap",
        WebSocketRequest::kSnap,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleSnap(req);
        });
  d.add("schematic_cone",
        WebSocketRequest::kSchematicCone,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleSchematicCone(req);
        });
  d.add("schematic_full",
        WebSocketRequest::kSchematicFull,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleSchematicFull(req);
        });
  d.add("schematic_inspect",
        WebSocketRequest::kSchematicInspect,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSchematicInspect(req, state);
        });
  d.add("select_next",
        WebSocketRequest::kSelectNext,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSelectNext(req, state);
        });
  d.add("select_prev",
        WebSocketRequest::kSelectPrev,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSelectPrev(req, state);
        });
  d.add("set_focus_nets",
        WebSocketRequest::kSetFocusNets,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSetFocusNets(req, state);
        });
  d.add("set_route_guides",
        WebSocketRequest::kSetRouteGuides,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSetRouteGuides(req, state);
        });
  d.add("get_3d_data",
        WebSocketRequest::kGet3DData,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleGet3DData(req);
        });
}

WebSocketResponse SelectHandler::handleSelect(const WebSocketRequest& req,
                                              SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    TileVisibility vis;
    vis.parseFromJson(req.json);
    // Leaflet allows fractional zoom (zoomSnap: 0); accept either int or
    // double on the wire and truncate to selectAt's integer zoom level.
    const auto& zoom_v = req.json.at("zoom");
    const int zoom = zoom_v.is_int64() ? static_cast<int>(zoom_v.get_int64())
                                       : static_cast<int>(zoom_v.as_double());
    auto results
        = gen_->selectAt(static_cast<int>(req.json.at("dbu_x").as_int64()),
                         static_cast<int>(req.json.at("dbu_y").as_int64()),
                         zoom,
                         vis,
                         arrayAsStringSet(req.json.at("visible_layers")));

    const bool add_to_selection = jsonOr(req.json, "add_to_selection", false);

    // STA's highlight() and getProperties() are not thread-safe;
    // serialize with other STA callers (timing, clock tree, tcl eval).
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getBlock(), use_dbu);

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    boost::json::array selected;
    selected.reserve(results.size());
    for (const auto& r : results) {
      boost::json::object item;
      item["name"] = r.name;
      item["type"] = r.type_name;
      item["bbox"] = bboxArray(r.bbox);
      selected.emplace_back(std::move(item));
    }
    root["selected"] = std::move(selected);

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
          root, inspected_sel, new_selectables, /*can_navigate_back=*/false);
    } else {
      root["can_navigate_back"] = 0;
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
      state.navigation_history.clear();

      if (add_to_selection) {
        // Shift+click: add to existing selection set if we hit something;
        // clicking empty space preserves the current selection.
        if (inspected_sel) {
          state.selection_itr = state.selection_set.insert(inspected_sel).first;
        }
      } else {
        // Normal click: replace selection set.
        state.selection_set.clear();
        if (inspected_sel) {
          state.selection_set.insert(inspected_sel);
        }
        state.selection_itr = state.selection_set.begin();
      }

      // Highlight all items in the selection set.
      collectMultiHighlightShapes(
          state.selection_set, state.highlight_rects, state.highlight_polys);
      state.current_inspected = inspected_sel;

      root["selection_count"]
          = static_cast<int64_t>(state.selection_set.size());
      root["selection_index"] = static_cast<int64_t>(
          selectionIteratorPosition(state.selection_set, state.selection_itr));
    }

    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
      const int select_id
          = static_cast<int>(req.json.at("select_id").as_int64());
      if (select_id >= 0) {
        std::lock_guard<std::mutex> lock(state.selectables_mutex);
        if (select_id < static_cast<int>(state.selectables.size())) {
          sel = state.selectables[select_id];
        }
      } else {
        // select_id < 0: re-inspect the currently inspected object
        // (used when toggling display-unit mode without changing selection).
        std::lock_guard<std::mutex> lock(state.selection_mutex);
        sel = state.current_inspected;
      }
    }

    // STA's highlight() and getProperties() are not thread-safe;
    // serialize with other STA callers (timing, clock tree, tcl eval).
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getBlock(), use_dbu);

    bool can_navigate_back = false;
    int sel_count = 0;
    int sel_index = -1;
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
        // Realign the cycling iterator with the linked target so that
        // selection_index reflects the object actually being rendered, and
        // the next Next/Previous starts from this object. If the link goes
        // outside the multi-selection, point the iterator at end() so the
        // index serializes as -1 and the nav UI is suppressed.
        state.selection_itr = state.selection_set.find(sel);
      }
      can_navigate_back = !state.navigation_history.empty();
      sel_count = static_cast<int>(state.selection_set.size());
      sel_index
          = selectionIteratorPosition(state.selection_set, state.selection_itr);
    }

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    std::vector<gui::Selected> new_selectables;
    writeInspectPayload(root, sel, new_selectables, can_navigate_back);
    root["selection_count"] = static_cast<int64_t>(sel_count);
    root["selection_index"] = static_cast<int64_t>(sel_index);
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      state.selectables = std::move(new_selectables);
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(gen_->getBlock(), use_dbu);
    int sel_count = 0;
    int sel_index = -1;
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
      sel_count = static_cast<int>(state.selection_set.size());
      sel_index
          = selectionIteratorPosition(state.selection_set, state.selection_itr);
    }

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    std::vector<gui::Selected> new_selectables;
    writeInspectPayload(root, sel, new_selectables, can_navigate_back);
    root["selection_count"] = static_cast<int64_t>(sel_count);
    root["selection_index"] = static_cast<int64_t>(sel_index);
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      state.selectables = std::move(new_selectables);
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// Cycle to the next/previous item in the multi-selection set.
// Returns the inspect payload for the newly active item without
// changing the highlight shapes (all selected items stay highlighted).
static WebSocketResponse handleSelectionCycle(
    const WebSocketRequest& req,
    SessionState& state,
    const int direction,
    std::shared_ptr<TclEvaluator>& tcl_eval,
    odb::dbBlock* block)
{
  WebSocketResponse resp;
  resp.id = req.id;
  try {
    gui::Selected sel;

    std::lock_guard<std::mutex> sta_lock(tcl_eval->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(block, use_dbu);
    int sel_count = 0;
    int sel_index = -1;
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      sel_count = static_cast<int>(state.selection_set.size());
      if (sel_count > 0) {
        if (direction > 0) {
          ++state.selection_itr;
          if (state.selection_itr == state.selection_set.end()) {
            state.selection_itr = state.selection_set.begin();
          }
        } else {
          if (state.selection_itr == state.selection_set.begin()) {
            state.selection_itr = state.selection_set.end();
          }
          --state.selection_itr;
        }
        sel = *state.selection_itr;
        state.current_inspected = sel;
        state.hover_rects.clear();
        state.timing_rects.clear();
        state.timing_lines.clear();
        state.navigation_history.clear();
        // Restore selection-set highlights (handleInspect may have
        // replaced them with a single linked object's shapes).
        collectMultiHighlightShapes(
            state.selection_set, state.highlight_rects, state.highlight_polys);
      }
      sel_index
          = selectionIteratorPosition(state.selection_set, state.selection_itr);
    }

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    std::vector<gui::Selected> new_selectables;
    const bool can_navigate_back = false;
    writeInspectPayload(root, sel, new_selectables, can_navigate_back);
    root["selection_count"] = static_cast<int64_t>(sel_count);
    root["selection_index"] = static_cast<int64_t>(sel_index);
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      state.selectables = std::move(new_selectables);
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSelectNext(const WebSocketRequest& req,
                                                  SessionState& state)
{
  return handleSelectionCycle(req, state, +1, tcl_eval_, gen_->getBlock());
}

WebSocketResponse SelectHandler::handleSelectPrev(const WebSocketRequest& req,
                                                  SessionState& state)
{
  return handleSelectionCycle(req, state, -1, tcl_eval_, gen_->getBlock());
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

      const int select_id
          = static_cast<int>(req.json.at("select_id").as_int64());
      if (select_id >= 0) {
        gui::Selected sel;
        {
          std::lock_guard<std::mutex> slock(state.selectables_mutex);
          if (select_id < static_cast<int>(state.selectables.size())) {
            sel = state.selectables[select_id];
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

    resp.type = WebSocketResponse::kJson;
    boost::json::object root;
    root["ok"] = 1;
    root["count"] = count;
    boost::json::array rects;
    rects.reserve(hover_rects.size());
    for (const auto& rect : hover_rects) {
      rects.emplace_back(bboxArray(rect));
    }
    root["rects"] = std::move(rects);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string action = std::string(req.json.at("action").as_string());
    const std::string net_name
        = std::string(req.json.at("net_name").as_string());
    std::lock_guard<std::mutex> lock(state.focus_nets_mutex);
    if (action == "clear") {
      state.focus_net_ids.clear();
    } else {
      odb::dbBlock* block = gen_->getBlock();
      odb::dbNet* net = block ? block->findNet(net_name.c_str()) : nullptr;
      if (net) {
        if (action == "add") {
          state.focus_net_ids.insert(net->getId());
        } else if (action == "remove") {
          state.focus_net_ids.erase(net->getId());
        }
      }
    }
    boost::json::object root;
    root["ok"] = 1;
    root["count"] = static_cast<int>(state.focus_net_ids.size());
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string action = std::string(req.json.at("action").as_string());
    const std::string net_name
        = std::string(req.json.at("net_name").as_string());
    std::lock_guard<std::mutex> lock(state.route_guides_mutex);
    if (action == "clear") {
      state.route_guide_net_ids.clear();
    } else {
      odb::dbBlock* block = gen_->getBlock();
      odb::dbNet* net = block ? block->findNet(net_name.c_str()) : nullptr;
      if (net) {
        if (action == "add") {
          state.route_guide_net_ids.insert(net->getId());
        } else if (action == "remove") {
          state.route_guide_net_ids.erase(net->getId());
        }
      }
    }
    boost::json::object root;
    root["ok"] = 1;
    root["count"] = static_cast<int>(state.route_guide_net_ids.size());
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleSnap(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    TileVisibility vis;
    vis.parseFromJson(req.json);
    auto snap = gen_->snapAt(
        static_cast<int>(req.json.at("dbu_x").as_int64()),
        static_cast<int>(req.json.at("dbu_y").as_int64()),
        static_cast<int>(req.json.at("radius").as_int64()),
        static_cast<int>(req.json.at("point_threshold").as_int64()),
        req.json.at("horizontal").as_bool(),
        req.json.at("vertical").as_bool(),
        vis,
        arrayAsStringSet(req.json.at("visible_layers")));
    boost::json::object root;
    root["found"] = snap.found;
    if (snap.found) {
      const bool is_point = snap.edge.first == snap.edge.second;
      root["is_point"] = is_point;
      root["edge"] = boost::json::array{
          boost::json::array{snap.edge.first.x(), snap.edge.first.y()},
          boost::json::array{snap.edge.second.x(), snap.edge.second.y()}};
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kJson;
  static constexpr int kMaxConeInsts = 150;
  static constexpr int kMaxNetFanout = 30;

  try {
    const std::string inst_name
        = std::string(req.json.at("inst_name").as_string());
    const int fanin_depth
        = static_cast<int>(req.json.at("fanin_depth").as_int64());
    const int fanout_depth
        = static_cast<int>(req.json.at("fanout_depth").as_int64());
    odb::dbBlock* block = gen_->getBlock();
    if (!block) {
      throw std::runtime_error("No block loaded");
    }

    odb::dbInst* target_inst = block->findInst(inst_name.c_str());
    if (!target_inst) {
      throw std::runtime_error("Instance not found: " + inst_name);
    }

    odb::PtrSet<odb::dbInst> all_insts;
    all_insts.insert(target_inst);
    bool cone_full = false;

    // Fanin BFS: follow input pins upstream to their driving instances.
    {
      std::vector<odb::dbInst*> level = {target_inst};
      odb::PtrSet<odb::dbNet> seen_nets;
      for (int d = 0; d < fanin_depth && !cone_full; ++d) {
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
      odb::PtrSet<odb::dbNet> seen_nets;
      for (int d = 0; d < fanout_depth && !cone_full; ++d) {
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
    odb::PtrMap<odb::dbNet, int> net_to_id;
    int next_net_id = 2;  // 0 = const-0, 1 = const-1 reserved by Yosys
    for (odb::dbInst* inst : all_insts) {
      for (odb::dbITerm* iterm : inst->getITerms()) {
        odb::dbNet* net = iterm->getNet();
        if (net && !net_to_id.contains(net)) {
          net_to_id[net] = next_net_id++;
        }
      }
    }

    boost::json::object top;
    top["attributes"] = boost::json::object{};

    boost::json::object ports;
    for (const auto& [net, _id] : net_to_id) {
      for (odb::dbBTerm* bterm : net->getBTerms()) {
        boost::json::object p;
        p["direction"] = ioTypeToDirection(bterm->getIoType());
        p["bits"] = boost::json::array{net_to_id[net]};
        ports[bterm->getName()] = std::move(p);
      }
    }
    top["ports"] = std::move(ports);

    boost::json::object cells;
    for (odb::dbInst* inst : all_insts) {
      boost::json::object cell;
      cell["hide_name"] = 0;
      cell["type"] = inst->getMaster() ? inst->getMaster()->getName()
                                       : std::string("$unknown");
      cell["attributes"] = boost::json::object{};
      cell["parameters"] = boost::json::object{};

      boost::json::object port_directions;
      for (odb::dbITerm* iterm : inst->getITerms()) {
        if (!iterm->getNet() || !net_to_id.contains(iterm->getNet())) {
          continue;
        }
        port_directions[iterm->getMTerm()->getName()]
            = ioTypeToDirection(iterm->getIoType());
      }
      cell["port_directions"] = std::move(port_directions);

      boost::json::object connections;
      for (odb::dbITerm* iterm : inst->getITerms()) {
        odb::dbNet* net = iterm->getNet();
        if (!net || !net_to_id.contains(net)) {
          continue;
        }
        connections[iterm->getMTerm()->getName()]
            = boost::json::array{net_to_id[net]};
      }
      cell["connections"] = std::move(connections);

      cells[inst->getName()] = std::move(cell);
    }
    top["cells"] = std::move(cells);

    boost::json::object netnames;
    for (const auto& [net, net_id] : net_to_id) {
      boost::json::object n;
      n["hide_name"] = 0;
      n["bits"] = boost::json::array{net_id};
      n["attributes"] = boost::json::object{};
      netnames[net->getName()] = std::move(n);
    }
    top["netnames"] = std::move(netnames);

    boost::json::object root;
    root["modules"] = boost::json::object{{"top", std::move(top)}};
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kJson;

  try {
    odb::dbBlock* block = gen_->getBlock();
    if (!block) {
      throw std::runtime_error("No block loaded");
    }

    odb::PtrMap<odb::dbNet, int> net_to_id;
    int next_net_id = 2;
    for (odb::dbNet* net : block->getNets()) {
      net_to_id[net] = next_net_id++;
    }

    boost::json::object top;
    top["attributes"] = boost::json::object{};

    boost::json::object ports;
    for (odb::dbBTerm* bterm : block->getBTerms()) {
      odb::dbNet* net = bterm->getNet();
      if (!net) {
        continue;
      }
      boost::json::object p;
      p["direction"] = ioTypeToDirection(bterm->getIoType());
      p["bits"] = boost::json::array{net_to_id[net]};
      ports[bterm->getName()] = std::move(p);
    }
    top["ports"] = std::move(ports);

    boost::json::object cells;
    for (odb::dbInst* inst : block->getInsts()) {
      boost::json::object cell;
      cell["hide_name"] = 0;
      cell["type"] = inst->getMaster() ? inst->getMaster()->getName()
                                       : std::string("$unknown");
      cell["attributes"] = boost::json::object{};
      cell["parameters"] = boost::json::object{};

      boost::json::object port_directions;
      for (odb::dbITerm* iterm : inst->getITerms()) {
        if (!iterm->getNet()) {
          continue;
        }
        port_directions[iterm->getMTerm()->getName()]
            = ioTypeToDirection(iterm->getIoType());
      }
      cell["port_directions"] = std::move(port_directions);

      boost::json::object connections;
      for (odb::dbITerm* iterm : inst->getITerms()) {
        odb::dbNet* net = iterm->getNet();
        if (!net) {
          continue;
        }
        connections[iterm->getMTerm()->getName()]
            = boost::json::array{net_to_id[net]};
      }
      cell["connections"] = std::move(connections);

      cells[inst->getName()] = std::move(cell);
    }
    top["cells"] = std::move(cells);

    boost::json::object netnames;
    for (odb::dbNet* net : block->getNets()) {
      boost::json::object n;
      n["hide_name"] = 0;
      n["bits"] = boost::json::array{net_to_id[net]};
      n["attributes"] = boost::json::object{};
      netnames[net->getName()] = std::move(n);
    }
    top["netnames"] = std::move(netnames);

    boost::json::object root;
    root["modules"] = boost::json::object{{"top", std::move(top)}};
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse SelectHandler::handleGet3DData(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    odb::dbChip* chip = gen_->getChip();
    if (!chip) {
      boost::json::object info;
      info["info"]
          = "No 3D chip data available. Load a multi-die design to "
            "use this view.";
      writePayload(resp, info);
      return resp;
    }

    boost::json::object root;
    boost::json::array chiplets;

    auto processInst = [&](auto& self,
                           odb::dbChipInst* inst,
                           int offset_x,
                           int offset_y,
                           int offset_z,
                           const std::string& parent_name) -> void {
      odb::dbChip* master_chip = inst->getMasterChip();
      if (master_chip && !master_chip->getChipInsts().empty()) {
        for (odb::dbChipInst* child : master_chip->getChipInsts()) {
          odb::Point3D loc = inst->getLoc();
          self(self,
               child,
               offset_x + loc.x(),
               offset_y + loc.y(),
               offset_z + loc.z(),
               parent_name + std::string(inst->getName()) + "/");
        }
      } else {
        boost::json::object obj;
        obj["name"] = parent_name + std::string(inst->getName());

        odb::Point3D loc = inst->getLoc();
        obj["x"] = offset_x + loc.x();
        obj["y"] = offset_y + loc.y();
        obj["z"] = offset_z + loc.z();

        int w = 0;
        int h = 0;
        int thickness = 0;
        if (master_chip) {
          w = master_chip->getWidth();
          h = master_chip->getHeight();
          thickness = master_chip->getThickness();
          // Fallback: use block bbox if chip has no explicit dimensions
          if (w == 0 || h == 0) {
            if (odb::dbBlock* block = master_chip->getBlock()) {
              if (odb::dbBox* block_bbox = block->getBBox()) {
                const odb::Rect bbox = block_bbox->getBox();
                if (w == 0) {
                  w = bbox.dx();
                }
                if (h == 0) {
                  h = bbox.dy();
                }
              }
            }
          }
        }
        constexpr int kDefaultChipWidth = 100000;
        constexpr int kDefaultChipHeight = 100000;
        constexpr int kDefaultChipThickness = 10000;
        obj["width"] = w > 0 ? w : kDefaultChipWidth;
        obj["height"] = h > 0 ? h : kDefaultChipHeight;
        obj["thickness"] = thickness > 0 ? thickness : kDefaultChipThickness;
        chiplets.emplace_back(std::move(obj));
      }
    };

    for (odb::dbChipInst* inst : chip->getChipInsts()) {
      processInst(processInst, inst, 0, 0, 0, "");
    }
    root["chiplets"] = std::move(chiplets);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kJson;

  try {
    const std::string inst_name
        = std::string(req.json.at("inst_name").as_string());
    odb::dbBlock* block = gen_->getBlock();
    if (!block) {
      throw std::runtime_error("No block loaded");
    }

    odb::dbInst* inst = block->findInst(inst_name.c_str());
    if (!inst) {
      throw std::runtime_error("Instance not found: " + inst_name);
    }

    gui::Selected sel = gui::DescriptorRegistry::instance()->makeSelected(inst);

    // STA's highlight() and getProperties() are not thread-safe;
    // serialize with other STA callers (timing, clock tree, tcl eval).
    std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
    const bool use_dbu = jsonOr(req.json, "use_dbu", false);
    ScopedDbuFormat dbu_fmt(block, use_dbu);

    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.hover_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();
      collectHighlightShapes(sel, state.highlight_rects, state.highlight_polys);
      state.current_inspected = sel;
      state.navigation_history.clear();
    }

    boost::json::object root;
    std::vector<gui::Selected> new_selectables;
    writeInspectPayload(
        root, sel, new_selectables, /*can_navigate_back=*/false);
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      state.selectables = std::move(new_selectables);
    }

    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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

void TclHandler::registerRequests(RequestDispatcher& d)
{
  d.add("tcl_eval",
        WebSocketRequest::kTclEval,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleTclEval(req);
        });
  d.add("tcl_complete",
        WebSocketRequest::kTclComplete,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleTclComplete(req);
        });
}

WebSocketResponse TclHandler::handleTclEval(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    auto result = tcl_eval_->eval(std::string(req.json.at("cmd").as_string()));
    // tclExitHandler (web_serve.cpp) sets this sentinel as the Tcl
    // result whenever `exit`/`quit` is evaluated through the override —
    // whether typed bare in the browser or buried in `eval`/`source`.
    // Convert it to a clean shutdown signal for the browser; the actual
    // teardown is already requested by tclExitHandler via requestStop().
    const bool is_exit = (result.result == kExitResultMsg);
    boost::json::object root;
    if (is_exit) {
      tcl_eval_->logger->info(
          utl::WEB, 40, "Exit requested from web GUI; shutting down.");
      root["result"] = "Exiting OpenROAD.";
      root["is_error"] = false;
      root["action"] = "shutdown";
    } else {
      root["result"] = result.result;
      root["is_error"] = result.is_error;
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TclHandler::handleTclComplete(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string line = std::string(req.json.at("line").as_string());
    const int cursor_pos
        = static_cast<int>(req.json.at("cursor_pos").as_int64());

    // The shared completer reads Tcl state via direct Tcl_Eval, so hold
    // the evaluator mutex for the same reasons regular eval requests do.
    ord::TclCompletion result;
    {
      std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
      result = ord::completeTcl(tcl_eval_->interp, line, cursor_pos);
    }

    boost::json::object root;
    boost::json::array comp_arr;
    comp_arr.reserve(result.completions.size());
    for (const auto& c : result.completions) {
      comp_arr.emplace_back(c);
    }
    root["completions"] = std::move(comp_arr);
    root["mode"] = result.mode;
    root["prefix"] = result.prefix;
    root["replace_start"] = result.replace_start;
    root["replace_end"] = result.replace_end;
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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

void TimingHandler::registerRequests(RequestDispatcher& d)
{
  d.add("timing_report",
        WebSocketRequest::kTimingReport,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleTimingReport(req);
        });
  d.add("timing_highlight",
        WebSocketRequest::kTimingHighlight,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleTimingHighlight(req, state);
        });
  d.add("slack_histogram",
        WebSocketRequest::kSlackHistogram,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleSlackHistogram(req);
        });
  d.add("chart_filters",
        WebSocketRequest::kChartFilters,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleChartFilters(req);
        });
}

WebSocketResponse TimingHandler::handleTimingReport(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    auto paths = timing_report_->getReport(
        req.json.at("is_setup").as_bool(),
        static_cast<int>(req.json.at("max_paths").as_int64()),
        static_cast<float>(jsonOr<double>(
            req.json, "slack_min", -std::numeric_limits<float>::max())),
        static_cast<float>(jsonOr<double>(
            req.json, "slack_max", std::numeric_limits<float>::max())));
    writePayload(resp, serializeTimingPaths(paths));
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kJson;
  try {
    const int path_index
        = static_cast<int>(req.json.at("path_index").as_int64());
    std::vector<ColoredRect> new_rects;
    std::vector<FlightLine> new_lines;

    // path_index < 0 is the clear-highlight signal (no other fields used).
    if (path_index >= 0) {
      const bool is_setup = req.json.at("is_setup").as_bool();
      std::lock_guard<std::mutex> sta_lock(tcl_eval_->mutex);
      auto paths = timing_report_->getReport(is_setup);
      if (path_index < static_cast<int>(paths.size())) {
        odb::dbBlock* block = gen_->getBlock();
        collectTimingPathShapes(block, paths[path_index], new_rects, new_lines);

        const std::string pin_name
            = jsonOr<std::string>(req.json, "pin_name", "");
        if (!pin_name.empty()) {
          static const Color kStageColor{.r = 255, .g = 255, .b = 0, .a = 180};
          auto [iterm, bterm] = resolvePin(block, pin_name);

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
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kJson;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    auto histogram = timing_report_->getSlackHistogram(
        req.json.at("is_setup").as_bool(),
        jsonOr<std::string>(req.json, "path_group", ""),
        jsonOr<std::string>(req.json, "clock_name", ""));
    writePayload(resp, serializeSlackHistogram(histogram));
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    const std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse TimingHandler::handleChartFilters(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    auto filters = timing_report_->getChartFilters();
    writePayload(resp, serializeChartFilters(filters));
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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

void ClockTreeHandler::registerRequests(RequestDispatcher& d)
{
  d.add("clock_tree",
        WebSocketRequest::kClockTree,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleClockTree(req);
        });
  d.add("clock_tree_highlight",
        WebSocketRequest::kClockTreeHighlight,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleClockTreeHighlight(req, state);
        });
}

WebSocketResponse ClockTreeHandler::handleClockTree(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    auto clocks = clock_report_->getReport();
    boost::json::object root;
    boost::json::array clk_arr;
    clk_arr.reserve(clocks.size());
    for (const auto& clk : clocks) {
      boost::json::object o;
      o["name"] = clk.clock_name;
      o["min_arrival"] = clk.min_arrival;
      o["max_arrival"] = clk.max_arrival;
      o["time_unit"] = clk.time_unit;
      boost::json::array nodes;
      nodes.reserve(clk.nodes.size());
      for (const auto& n : clk.nodes) {
        boost::json::object node;
        node["id"] = n.id;
        node["parent_id"] = n.parent_id;
        node["name"] = n.name;
        node["pin_name"] = n.pin_name;
        node["type"] = ClockTreeNode::typeToString(n.type);
        node["arrival"] = n.arrival;
        node["delay"] = n.delay;
        node["fanout"] = n.fanout;
        node["level"] = n.level;
        node["dbu_x"] = n.dbu_x;
        node["dbu_y"] = n.dbu_y;
        nodes.emplace_back(std::move(node));
      }
      o["nodes"] = std::move(nodes);
      clk_arr.emplace_back(std::move(o));
    }
    root["clocks"] = std::move(clk_arr);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string inst_name
        = std::string(req.json.at("inst_name").as_string());
    std::lock_guard<std::mutex> lock(state.selection_mutex);
    state.highlight_rects.clear();
    state.highlight_polys.clear();
    state.timing_rects.clear();
    state.timing_lines.clear();

    if (!inst_name.empty()) {
      odb::dbBlock* block = gen_->getBlock();
      if (block) {
        odb::dbInst* inst = block->findInst(inst_name.c_str());
        if (inst) {
          state.highlight_rects.push_back(inst->getBBox()->getBox());
        }
      }
    }

    const std::string json = "{\"ok\": true}";
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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

void TileHandler::registerRequests(RequestDispatcher& d)
{
  d.add("tile",
        WebSocketRequest::kTile,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleTile(req, state);
        });
  d.add("bounds",
        WebSocketRequest::kBounds,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleTile(req, state);
        });
  d.add("tech",
        WebSocketRequest::kTech,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleTile(req, state);
        });
  d.add("module_hierarchy",
        WebSocketRequest::kModuleHierarchy,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleModuleHierarchy(req);
        });
  d.add("set_module_colors",
        WebSocketRequest::kSetModuleColors,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSetModuleColors(req, state);
        });
  d.add("heatmaps",
        WebSocketRequest::kHeatmaps,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleHeatMaps(req, state);
        });
  d.add("set_active_heatmap",
        WebSocketRequest::kSetActiveHeatmap,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSetActiveHeatMap(req, state);
        });
  d.add("set_heatmap",
        WebSocketRequest::kSetHeatmap,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleSetHeatMap(req, state);
        });
  d.add("heatmap_tile",
        WebSocketRequest::kHeatmapTile,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleHeatMapTile(req, state);
        });
  d.add("overlay_tile",
        WebSocketRequest::kOverlayTile,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleOverlayTile(req, state);
        });
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
  switch (req.type) {
    case WebSocketRequest::kBounds:
      return serializeBounds(req.id, *gen_);
    case WebSocketRequest::kTech:
      return serializeTech(req.id, *gen_);
    case WebSocketRequest::kTile:
      break;
    default: {
      WebSocketResponse resp;
      resp.id = req.id;
      resp.type = WebSocketResponse::kError;
      const std::string err = "Unknown request type";
      resp.payload.assign(err.begin(), err.end());
      return resp;
    }
  }

  TileVisibility vis;
  vis.parseFromJson(req.json);

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

  // Base tiles no longer carry highlights — those are rendered by the
  // overlay tile layer.  Pass empty vectors so renderTileBuffer skips
  // drawHighlight / drawColoredHighlight / drawFlightLines / drawRouteGuides.
  static const std::vector<odb::Rect> no_rects;
  static const std::vector<odb::Polygon> no_polys;
  static const std::vector<ColoredRect> no_colored;
  static const std::vector<FlightLine> no_lines;

  return renderTile(req.id,
                    std::string(req.json.at("layer").as_string()),
                    static_cast<int>(req.json.at("z").as_int64()),
                    static_cast<int>(req.json.at("x").as_int64()),
                    static_cast<int>(req.json.at("y").as_int64()),
                    vis,
                    *gen_,
                    no_rects,
                    no_polys,
                    no_colored,
                    no_lines,
                    mod_ptr,
                    focus_ptr,
                    nullptr);
}

WebSocketResponse TileHandler::handleOverlayTile(const WebSocketRequest& req,
                                                 SessionState& state)
{
  // When debug renderers are active, instance positions change between
  // frames.  Re-derive highlight shapes from the current inspected
  // object so the selection tracks the moving instance.
  const bool debug_renderers = jsonOr(req.json, "debug_renderers", false);
  if (debug_renderers) {
    std::lock_guard<std::mutex> lock(state.selection_mutex);
    if (state.current_inspected) {
      collectHighlightShapes(state.current_inspected,
                             state.highlight_rects,
                             state.highlight_polys);
    }
  }

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

  // Merge DRC overlay shapes
  {
    std::lock_guard<std::mutex> lock(state.drc_mutex);
    colored.insert(
        colored.end(), state.drc_rects.begin(), state.drc_rects.end());
    lines.insert(lines.end(), state.drc_lines.begin(), state.drc_lines.end());
  }

  // Snapshot route guide nets
  std::set<uint32_t> route_guides;
  {
    std::lock_guard<std::mutex> lock(state.route_guides_mutex);
    route_guides = state.route_guide_net_ids;
  }
  const std::set<uint32_t>* route_guide_ptr
      = route_guides.empty() ? nullptr : &route_guides;

  // Parse visible layers so route guides respect layer visibility.
  // has_vis_layers=true means the field was present (even if empty,
  // which means "all layers hidden" — matching pin-marker semantics).
  bool has_vis_layers = false;
  std::set<std::string> vis_layers;
  if (auto it = req.json.find("visible_layers"); it != req.json.end()) {
    has_vis_layers = true;
    const auto& arr = it->value().as_array();
    for (const auto& elem : arr) {
      vis_layers.emplace(elem.as_string());
    }
  }

  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kPng;
  resp.payload
      = gen_->generateOverlayTile(static_cast<int>(req.json.at("z").as_int64()),
                                  static_cast<int>(req.json.at("x").as_int64()),
                                  static_cast<int>(req.json.at("y").as_int64()),
                                  rects,
                                  polys,
                                  colored,
                                  lines,
                                  route_guide_ptr,
                                  has_vis_layers,
                                  vis_layers);
  return resp;
}

WebSocketResponse TileHandler::handleModuleHierarchy(
    const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;
  try {
    odb::dbBlock* block = gen_->getBlock();
    HierarchyReport report(block, gen_->getSta());
    auto result = report.getReport();
    writePayload(resp, serializeHierarchyResult(result));
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kJson;
  std::map<uint32_t, Color> colors;
  const std::string data = std::string(req.json.at("colors").as_string());
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
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string json = buildHeatMapsPayloadLocked(state);
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string name = std::string(req.json.at("name").as_string());
    std::lock_guard<std::mutex> lock(state.heatmap_mutex);
    if (!state.active_heatmap.empty()) {
      auto current = state.heatmaps.find(state.active_heatmap);
      if (current != state.heatmaps.end()) {
        current->second->onHide();
      }
    }

    state.active_heatmap.clear();
    if (!name.empty()) {
      auto next = state.heatmaps.find(name);
      if (next == state.heatmaps.end()) {
        throw std::runtime_error("invalid heat map");
      }
      state.active_heatmap = name;
      next->second->onShow();
    }

    const std::string json = buildHeatMapsPayloadLocked(state);
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kJson;
  try {
    const std::string name = std::string(req.json.at("name").as_string());
    const std::string option = std::string(req.json.at("option").as_string());
    std::lock_guard<std::mutex> lock(state.heatmap_mutex);
    auto source_itr = state.heatmaps.find(name);
    if (source_itr == state.heatmaps.end()) {
      throw std::runtime_error("invalid heat map");
    }

    auto& source = *source_itr->second;
    if (option == "rebuild") {
      source.destroyMap();
      source.ensureMap();
    } else {
      auto settings = source.getSettings();
      auto setting_itr = settings.find(option);
      if (setting_itr == settings.end()) {
        throw std::runtime_error("invalid heat map option");
      }

      const auto& current_value = setting_itr->second;
      const auto& value_v = req.json.at("value");
      if (std::holds_alternative<bool>(current_value)) {
        settings[option] = value_v.as_bool();
      } else if (std::holds_alternative<int>(current_value)) {
        // The frontend's addNumber control runs every value through
        // parseFloat, so int settings can arrive as JSON doubles.  Accept
        // either and round.
        settings[option]
            = value_v.is_int64()
                  ? static_cast<int>(value_v.get_int64())
                  : static_cast<int>(std::round(value_v.as_double()));
      } else if (std::holds_alternative<double>(current_value)) {
        settings[option] = value_v.as_double();
      } else {
        settings[option] = std::string(value_v.as_string());
      }
      source.setSettings(settings);
    }

    const std::string json = buildHeatMapsPayloadLocked(state);
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kPng;
  try {
    const std::string req_name = std::string(req.json.at("name").as_string());
    const int z = static_cast<int>(req.json.at("z").as_int64());
    const int x = static_cast<int>(req.json.at("x").as_int64());
    const int y = static_cast<int>(req.json.at("y").as_int64());
    std::shared_ptr<gui::HeatMapDataSource> source;
    {
      std::lock_guard<std::mutex> lock(state.heatmap_mutex);
      const std::string name
          = req_name.empty() ? state.active_heatmap : req_name;
      auto source_itr = state.heatmaps.find(name);
      if (source_itr == state.heatmaps.end()) {
        throw std::runtime_error("invalid heat map");
      }
      source = source_itr->second;
    }
    resp.payload = gen_->generateHeatMapTile(*source, z, x, y);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
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
  resp.type = WebSocketResponse::kJson;

  try {
    const std::string dir_str = std::string(req.json.at("path").as_string());
    fs::path dir_path
        = dir_str.empty() ? fs::current_path() : fs::path(dir_str);
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

    boost::json::object root;
    root["path"] = dir_path.string();
    root["parent"] = dir_path.parent_path().string();
    boost::json::array arr;
    arr.reserve(entries.size());
    for (const auto& entry : entries) {
      boost::json::object o;
      o["name"] = entry.name;
      o["is_dir"] = entry.is_dir;
      if (!entry.is_dir) {
        o["size"] = static_cast<int>(entry.size);
      }
      arr.emplace_back(std::move(o));
    }
    root["entries"] = std::move(arr);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("list_dir error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

//------------------------------------------------------------------------------
// DRCHandler
//------------------------------------------------------------------------------

DRCHandler::DRCHandler(std::shared_ptr<TileGenerator> gen)
    : gen_(std::move(gen))
{
}

void DRCHandler::registerRequests(RequestDispatcher& d)
{
  d.add("drc_categories",
        WebSocketRequest::kDrcCategories,
        [this](const WebSocketRequest& req, SessionState&) {
          return handleDRCCategories(req);
        });
  d.add("drc_markers",
        WebSocketRequest::kDrcMarkers,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleDRCMarkers(req, state);
        });
  d.add("drc_load_report",
        WebSocketRequest::kDrcLoadReport,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleDRCLoadReport(req, state);
        });
  d.add("drc_update_marker",
        WebSocketRequest::kDrcUpdateMarker,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleDRCUpdateMarker(req, state);
        });
  d.add("drc_update_category_visibility",
        WebSocketRequest::kDrcUpdateCategoryVisibility,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleDRCUpdateCategoryVisibility(req, state);
        });
  d.add("drc_highlight",
        WebSocketRequest::kDrcHighlight,
        [this](const WebSocketRequest& req, SessionState& state) {
          return handleDRCHighlight(req, state);
        });
}

std::pair<odb::dbBlock*, odb::dbChip*> DRCHandler::getBlockAndChip()
{
  odb::dbChip* chip = gen_->getChip();
  if (!chip) {
    throw std::runtime_error("No chip loaded");
  }
  odb::dbBlock* block = chip->getBlock();
  return {block, chip};
}

odb::dbMarker* DRCHandler::findMarkerById(SessionState& state,
                                          odb::dbChip* chip,
                                          int marker_id)
{
  std::lock_guard<std::mutex> lock(state.drc_mutex);
  if (state.active_drc_category.empty()) {
    return nullptr;
  }
  odb::dbMarkerCategory* category
      = chip->findMarkerCategory(state.active_drc_category.c_str());
  if (!category) {
    return nullptr;
  }
  for (odb::dbMarker* marker : category->getAllMarkers()) {
    if (static_cast<int>(marker->getId()) == marker_id) {
      return marker;
    }
  }
  return nullptr;
}

void DRCHandler::refreshDRCOverlay(SessionState& state)
{
  // Must be called with drc_mutex already held.
  state.drc_rects.clear();
  state.drc_lines.clear();

  odb::dbChip* chip = gen_->getChip();
  if (!chip || state.active_drc_category.empty()) {
    return;
  }
  odb::dbBlock* block = chip->getBlock();

  odb::dbMarkerCategory* category
      = chip->findMarkerCategory(state.active_drc_category.c_str());
  if (!category) {
    return;
  }

  // Match the Qt GUI rendering style (dbDescriptors.cpp paintMarker +
  // drcWidget.cpp DRCRenderer::drawObjects):
  //   pen = white (solid), brush = white alpha 50 diagonal cross-hatch.
  // We approximate: Rect/Polygon/Cuboid → filled semi-transparent rect
  // with solid outline.  Line → drawn as a line.  Point → drawn as X.
  // When the marker bbox is too small (< min_box DBU), draw an X at
  // the center instead, matching the GUI's min_box fallback.
  // Color matches Qt GUI's Painter::kHighlight (yellow).
  const Color yellow_fill{.r = 255, .g = 255, .b = 0, .a = 100};
  const Color yellow_line{.r = 255, .g = 255, .b = 0, .a = 255};

  // min_box: cached tech pitch as "minimum visible size" threshold.
  // Default to 200 DBU (0.2um at 1000 dbu/um) if no routing layer available.
  if (min_box_ < 0) {
    constexpr int kDefaultMinBox = 200;
    min_box_ = kDefaultMinBox;
    if (block) {
      odb::dbTech* tech = block->getTech();
      if (tech) {
        for (odb::dbTechLayer* layer : tech->getLayers()) {
          if (layer->getType() == odb::dbTechLayerType::ROUTING) {
            const int pitch = layer->getPitch();
            if (pitch > 0) {
              min_box_ = pitch;
              break;
            }
          }
        }
      }
    }
  }
  const int min_box = min_box_;

  auto emitX = [&](int cx, int cy, int half) {
    // Two diagonal lines forming an X, matching GUI's painter.drawX().
    state.drc_lines.push_back({odb::Point(cx - half, cy - half),
                               odb::Point(cx + half, cy + half),
                               yellow_line});
    state.drc_lines.push_back({odb::Point(cx - half, cy + half),
                               odb::Point(cx + half, cy - half),
                               yellow_line});
  };

  for (odb::dbMarker* marker : category->getAllMarkers()) {
    if (!marker->isVisible()) {
      continue;
    }

    const odb::Rect bbox = marker->getBBox();

    // GUI fallback: if bbox is too small, draw X at center instead of
    // individual shapes (dbDescriptors.cpp paintMarker, min_box check).
    if (bbox.maxDXDY() < min_box) {
      const int cx = bbox.xMin() + bbox.dx() / 2;
      const int cy = bbox.yMin() + bbox.dy() / 2;
      emitX(cx, cy, min_box / 2);
      continue;
    }

    const auto& shapes = marker->getShapes();

    // Fallback: if no shapes, use the bounding box.
    if (shapes.empty()) {
      if (bbox.area() > 0) {
        state.drc_rects.push_back({bbox, yellow_fill, "", /*filled=*/true});
      }
      continue;
    }

    for (const auto& shape : shapes) {
      if (std::holds_alternative<odb::Rect>(shape)) {
        state.drc_rects.push_back(
            {std::get<odb::Rect>(shape), yellow_fill, "", /*filled=*/true});
      } else if (std::holds_alternative<odb::Line>(shape)) {
        const odb::Line& line = std::get<odb::Line>(shape);
        state.drc_lines.push_back({line.pt0(), line.pt1(), yellow_line});
      } else if (std::holds_alternative<odb::Point>(shape)) {
        const odb::Point& pt = std::get<odb::Point>(shape);
        emitX(pt.x(), pt.y(), min_box / 2);
      } else if (std::holds_alternative<odb::Polygon>(shape)) {
        const odb::Polygon& poly = std::get<odb::Polygon>(shape);
        state.drc_rects.push_back(
            {poly.getEnclosingRect(), yellow_fill, "", /*filled=*/true});
      } else if (std::holds_alternative<odb::Cuboid>(shape)) {
        state.drc_rects.push_back(
            {std::get<odb::Cuboid>(shape).getEnclosingRect(),
             yellow_fill,
             "",
             /*filled=*/true});
      }
    }
  }
}

WebSocketResponse DRCHandler::handleDRCCategories(const WebSocketRequest& req)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    auto [block, chip] = getBlockAndChip();

    boost::json::object root;
    boost::json::array categories;
    for (odb::dbMarkerCategory* category : chip->getMarkerCategories()) {
      boost::json::object o;
      o["name"] = std::string(category->getName());
      o["count"] = category->getMarkerCount();
      const std::string desc = category->getDescription();
      if (!desc.empty()) {
        o["description"] = desc;
      }
      const std::string source = category->getSource();
      if (!source.empty()) {
        o["source"] = source;
      }
      categories.emplace_back(std::move(o));
    }
    root["categories"] = std::move(categories);
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("drc_categories error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

// Recursive helper to serialize a marker category tree.
static boost::json::object serializeMarkerCategory(
    odb::dbMarkerCategory* category)
{
  boost::json::object o;
  o["name"] = std::string(category->getName());
  o["count"] = category->getMarkerCount();

  // Subcategories
  auto subcats = category->getMarkerCategories();
  if (subcats.begin() != subcats.end()) {
    boost::json::array arr;
    for (odb::dbMarkerCategory* sub : subcats) {
      arr.emplace_back(serializeMarkerCategory(sub));
    }
    o["subcategories"] = std::move(arr);
  }

  // Markers directly in this category
  auto markers = category->getMarkers();
  if (markers.begin() != markers.end()) {
    boost::json::array marker_arr;
    int idx = 1;
    for (odb::dbMarker* marker : markers) {
      boost::json::object m;
      m["id"] = static_cast<int>(marker->getId());
      m["index"] = idx++;
      m["name"] = marker->getName();
      m["visited"] = marker->isVisited();
      m["visible"] = marker->isVisible();
      m["waived"] = marker->isWaived();
      m["bbox"] = bboxArray(marker->getBBox());

      // Serialize individual shapes so the 3D viewer can highlight
      // the actual cuboids instead of the full bounding box.
      const auto& shapes = marker->getShapes();
      if (!shapes.empty()) {
        boost::json::array rects;
        for (const auto& shape : shapes) {
          if (std::holds_alternative<odb::Rect>(shape)) {
            const odb::Rect& r = std::get<odb::Rect>(shape);
            rects.emplace_back(
                boost::json::array{r.xMin(), r.yMin(), r.xMax(), r.yMax()});
          } else if (std::holds_alternative<odb::Polygon>(shape)) {
            const odb::Rect r
                = std::get<odb::Polygon>(shape).getEnclosingRect();
            rects.emplace_back(
                boost::json::array{r.xMin(), r.yMin(), r.xMax(), r.yMax()});
          } else if (std::holds_alternative<odb::Cuboid>(shape)) {
            const odb::Cuboid& c = std::get<odb::Cuboid>(shape);
            rects.emplace_back(boost::json::array{
                c.xMin(), c.yMin(), c.xMax(), c.yMax(), c.zMin(), c.zMax()});
          }
        }
        m["rects"] = std::move(rects);
      }

      if (odb::dbTechLayer* layer = marker->getTechLayer()) {
        m["layer"] = std::string(layer->getName());
      }

      const std::string comment = marker->getComment();
      if (!comment.empty()) {
        m["comment"] = comment;
      }

      auto sources = marker->getSources();
      if (!sources.empty()) {
        boost::json::array src_arr;
        for (odb::dbObject* src : sources) {
          boost::json::object s;
          switch (src->getObjectType()) {
            case odb::dbNetObj: {
              s["type"] = "Net";
              s["name"] = std::string(static_cast<odb::dbNet*>(src)->getName());
              break;
            }
            case odb::dbInstObj: {
              s["type"] = "Inst";
              s["name"]
                  = std::string(static_cast<odb::dbInst*>(src)->getName());
              break;
            }
            case odb::dbITermObj: {
              s["type"] = "ITerm";
              s["name"]
                  = std::string(static_cast<odb::dbITerm*>(src)->getName());
              break;
            }
            case odb::dbBTermObj: {
              s["type"] = "BTerm";
              s["name"]
                  = std::string(static_cast<odb::dbBTerm*>(src)->getName());
              break;
            }
            default:
              s["type"] = "Object";
              s["name"] = "unknown";
              break;
          }
          src_arr.emplace_back(std::move(s));
        }
        m["sources"] = std::move(src_arr);
      }
      marker_arr.emplace_back(std::move(m));
    }
    o["markers"] = std::move(marker_arr);
  }
  return o;
}

WebSocketResponse DRCHandler::handleDRCMarkers(const WebSocketRequest& req,
                                               SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    auto [block, chip] = getBlockAndChip();

    const std::string cat_name
        = std::string(req.json.at("category").as_string());

    // Clear all markers' visibility so highlights start off.
    // The user explicitly checks individual markers to see them.
    odb::dbMarkerCategory* category = nullptr;
    if (!cat_name.empty()) {
      category = chip->findMarkerCategory(cat_name.c_str());
      if (category) {
        for (odb::dbMarker* marker : category->getAllMarkers()) {
          marker->setVisible(false);
        }
      }
    }

    // Update active category and overlay (now empty since all invisible)
    {
      std::lock_guard<std::mutex> lock(state.drc_mutex);
      state.active_drc_category = cat_name;
      refreshDRCOverlay(state);
    }

    boost::json::object root;
    if (cat_name.empty()) {
      root["subcategories"] = boost::json::array{};
    } else {
      if (!category) {
        root["error"] = "Category not found: " + cat_name;
      } else {
        root = serializeMarkerCategory(category);
        root["total_count"] = category->getMarkerCount();
      }
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("drc_markers error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse DRCHandler::handleDRCLoadReport(const WebSocketRequest& req,
                                                  SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    auto [block, chip] = getBlockAndChip();

    const std::string path = std::string(req.json.at("path").as_string());
    if (path.empty()) {
      throw std::runtime_error("No file path provided");
    }

    odb::dbMarkerCategory* category = nullptr;
    if (path.ends_with(".rpt") || path.ends_with(".drc")) {
      category = odb::dbMarkerCategory::fromTR(chip, "DRC", path);
    } else if (path.ends_with(".json")) {
      auto categories = odb::dbMarkerCategory::fromJSON(chip, path);
      if (!categories.empty()) {
        category = *categories.begin();
      }
    } else {
      throw std::runtime_error("Unsupported file format: " + path);
    }

    boost::json::object root;
    if (category) {
      const std::string name = category->getName();
      root["ok"] = 1;
      root["category"] = name;
      root["count"] = category->getMarkerCount();

      // Auto-select the loaded category
      {
        std::lock_guard<std::mutex> lock(state.drc_mutex);
        state.active_drc_category = name;
        refreshDRCOverlay(state);
      }
    } else {
      root["ok"] = 0;
      root["error"] = "No violations found in report";
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("drc_load_report error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse DRCHandler::handleDRCUpdateMarker(const WebSocketRequest& req,
                                                    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    const int marker_id = static_cast<int>(req.json.at("marker_id").as_int64());
    const std::string field = std::string(req.json.at("field").as_string());
    const bool field_value = req.json.at("value").as_bool();
    auto [block, chip] = getBlockAndChip();

    odb::dbMarker* target = findMarkerById(state, chip, marker_id);
    if (!target) {
      throw std::runtime_error("Marker not found with id "
                               + std::to_string(marker_id));
    }

    if (field == "visited") {
      target->setVisited(field_value);
    } else if (field == "visible") {
      target->setVisible(field_value);
      std::lock_guard<std::mutex> lock(state.drc_mutex);
      refreshDRCOverlay(state);
    } else {
      throw std::runtime_error("Unknown field: " + field);
    }

    boost::json::object root;
    root["ok"] = 1;
    root["id"] = marker_id;
    root["field"] = field;
    root["value"] = field_value;
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("drc_update_marker error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse DRCHandler::handleDRCUpdateCategoryVisibility(
    const WebSocketRequest& req,
    SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    const std::string cat_name
        = std::string(req.json.at("category").as_string());
    const bool visible = req.json.at("visible").as_bool();
    auto [block, chip] = getBlockAndChip();

    std::lock_guard<std::mutex> lock(state.drc_mutex);
    odb::dbMarkerCategory* category
        = chip->findMarkerCategory(cat_name.c_str());
    if (!category) {
      throw std::runtime_error("Category not found: " + cat_name);
    }

    int count = 0;
    for (odb::dbMarker* marker : category->getAllMarkers()) {
      marker->setVisible(visible);
      ++count;
    }
    refreshDRCOverlay(state);

    boost::json::object root;
    root["ok"] = 1;
    root["category"] = cat_name;
    root["visible"] = visible;
    root["count"] = count;
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err
        = std::string("drc_update_category_visibility error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WebSocketResponse DRCHandler::handleDRCHighlight(const WebSocketRequest& req,
                                                 SessionState& state)
{
  WebSocketResponse resp;
  resp.id = req.id;
  resp.type = WebSocketResponse::kJson;

  try {
    const int marker_id = static_cast<int>(req.json.at("marker_id").as_int64());
    const bool open_inspector = req.json.contains("open_inspector")
                                    ? req.json.at("open_inspector").as_bool()
                                    : false;
    auto [block, chip] = getBlockAndChip();

    odb::dbMarker* target = findMarkerById(state, chip, marker_id);

    boost::json::object root;
    if (target) {
      target->setVisited(true);
      odb::Rect bbox = target->getBBox();

      // When the client requests inspector navigation, promote the marker to
      // a canonical selectable so the existing `inspect` flow can populate
      // the Inspector panel.  Mirrors handleSelect's pattern (replace
      // selectables, set current_inspected, clear navigation history) so
      // back-navigation behaves the same as for instances/nets.
      gui::Selected sel;
      int marker_select_id = -1;
      std::vector<gui::Selected> new_selectables;
      if (open_inspector) {
        sel = gui::DescriptorRegistry::instance()->makeSelected(target);
        if (sel) {
          marker_select_id = storeSelectable(new_selectables, sel);
        }
      }

      {
        std::lock_guard<std::mutex> lock(state.selection_mutex);
        state.highlight_rects.clear();
        state.highlight_polys.clear();
        if (sel) {
          state.hover_rects.clear();
          state.timing_rects.clear();
          state.timing_lines.clear();
          collectHighlightShapes(
              sel, state.highlight_rects, state.highlight_polys);
          state.current_inspected = sel;
          state.navigation_history.clear();
        } else {
          state.highlight_rects.push_back(bbox);
        }
      }

      if (sel) {
        std::lock_guard<std::mutex> lock(state.selectables_mutex);
        state.selectables = std::move(new_selectables);
      }

      root["ok"] = 1;
      root["bbox"] = bboxArray(bbox);
      root["name"] = target->getName();
      root["visited"] = true;
      if (odb::dbTechLayer* layer = target->getTechLayer()) {
        root["layer"] = std::string(layer->getName());
      }
      if (marker_select_id >= 0) {
        root["select_id"] = marker_select_id;
      }
    } else {
      // Clear highlight if marker_id is -1 (deselect)
      if (marker_id == -1) {
        std::lock_guard<std::mutex> lock(state.selection_mutex);
        state.highlight_rects.clear();
        state.highlight_polys.clear();
      }
      root["ok"] = 0;
    }
    writePayload(resp, root);
  } catch (const std::exception& e) {
    resp.type = WebSocketResponse::kError;
    std::string err = std::string("drc_highlight error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

}  // namespace web
