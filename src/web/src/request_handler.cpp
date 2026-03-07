// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "request_handler.h"

#include <any>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "gui/descriptor_registry.h"
#include "gui/gui.h"
#include "odb/db.h"
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

  void drawRect(const odb::Rect& rect, int, int) override
  {
    rects.push_back(rect);
  }
  void drawPolygon(const odb::Polygon& polygon) override
  {
    rects.push_back(polygon.getEnclosingRect());
  }
  void drawOctagon(const odb::Oct& oct) override
  {
    rects.push_back(oct.getEnclosingRect());
  }

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

std::string json_escape(const std::string& s)
{
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[8];
          snprintf(
              buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
          out += buf;
        } else {
          out += c;
        }
        break;
    }
  }
  return out;
}

// Store a Selected in the clickables vector and return its index.
static int storeSelectable(std::vector<gui::Selected>& selectables,
                           const gui::Selected& sel)
{
  int id = static_cast<int>(selectables.size());
  selectables.push_back(sel);
  return id;
}

static void serializeAnyValue(std::stringstream& ss,
                              const char* field,
                              const std::any& value,
                              std::vector<gui::Selected>& selectables,
                              bool short_name = false)
{
  if (auto* sel = std::any_cast<gui::Selected>(&value)) {
    if (*sel) {
      std::string name = short_name ? sel->getShortName() : sel->getName();
      int id = storeSelectable(selectables, *sel);
      ss << "\"" << field << "\": \"" << json_escape(name) << "\", \""
         << field << "_select_id\": " << id;
      return;
    }
  }
  std::string str = gui::Descriptor::Property::toString(value);
  ss << "\"" << field << "\": \"" << json_escape(str) << "\"";
}

static void serializeProperty(std::stringstream& ss,
                               const gui::Descriptor::Property& prop,
                               std::vector<gui::Selected>& selectables)
{
  ss << "{\"name\": \"" << json_escape(prop.name) << "\"";

  if (auto* plist
      = std::any_cast<gui::Descriptor::PropertyList>(&prop.value)) {
    ss << ", \"children\": [";
    bool first = true;
    for (const auto& [key, val] : *plist) {
      if (!first) {
        ss << ", ";
      }
      ss << "{";
      serializeAnyValue(ss, "name", key, selectables, /*short_name=*/true);
      ss << ", ";
      serializeAnyValue(ss, "value", val, selectables);
      ss << "}";
      first = false;
    }
    ss << "]";
  } else if (auto* sel_set
             = std::any_cast<gui::SelectionSet>(&prop.value)) {
    ss << ", \"children\": [";
    bool first = true;
    for (const auto& sel : *sel_set) {
      if (!first) {
        ss << ", ";
      }
      int id = storeSelectable(selectables, sel);
      ss << "{\"name\": \"" << json_escape(sel.getName())
         << "\", \"name_select_id\": " << id << "}";
      first = false;
    }
    ss << "]";
  } else if (auto* sel = std::any_cast<gui::Selected>(&prop.value)) {
    if (*sel) {
      int id = storeSelectable(selectables, *sel);
      ss << ", \"value\": \"" << json_escape(sel->getName())
         << "\", \"value_select_id\": " << id;
    }
  } else {
    std::string val_str = prop.toString();
    ss << ", \"value\": \"" << json_escape(val_str) << "\"";
  }

  ss << "}";
}

//------------------------------------------------------------------------------
// dispatch_request — handles BOUNDS, LAYERS, TILE, INFO
//------------------------------------------------------------------------------

WsResponse dispatch_request(
    const WsRequest& req,
    const std::shared_ptr<TileGenerator>& gen,
    const std::vector<odb::Rect>& highlight_rects,
    const std::vector<ColoredRect>& colored_rects,
    const std::vector<FlightLine>& flight_lines)
{
  WsResponse resp;
  resp.id = req.id;

  switch (req.type) {
    case WsRequest::BOUNDS: {
      resp.type = 0;
      const odb::Rect bounds = gen->getBounds();
      std::stringstream ss;
      ss << "{\"bounds\": [[" << bounds.yMin() << ", " << bounds.xMin()
         << "], [" << bounds.yMax() << ", " << bounds.xMax() << "]]}";
      const std::string json = ss.str();
      resp.payload.assign(json.begin(), json.end());
      break;
    }
    case WsRequest::LAYERS: {
      resp.type = 0;
      std::stringstream ss;
      ss << "{\"layers\": [";
      bool first = true;
      for (const auto& name : gen->getLayers()) {
        if (!first) {
          ss << ", ";
        }
        ss << "\"" << name << "\"";
        first = false;
      }
      ss << "]}";
      const std::string json = ss.str();
      resp.payload.assign(json.begin(), json.end());
      break;
    }
    case WsRequest::TILE: {
      resp.type = 1;
      resp.payload = gen->generateTile(req.layer,
                                       req.z,
                                       req.x,
                                       req.y,
                                       req.vis,
                                       highlight_rects,
                                       colored_rects,
                                       flight_lines);
      break;
    }
    case WsRequest::INFO: {
      resp.type = 0;
      const std::string json = gen->hasSta() ? "{\"has_liberty\": true}"
                                             : "{\"has_liberty\": false}";
      resp.payload.assign(json.begin(), json.end());
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

SelectHandler::SelectHandler(std::shared_ptr<TileGenerator> gen)
    : gen_(std::move(gen))
{
}

WsResponse SelectHandler::handleSelect(const WsRequest& req,
                                       SessionState& state)
{
  WsResponse resp;
  resp.id = req.id;
  try {
    auto results
        = gen_->selectAt(req.select_x, req.select_y, req.select_zoom, req.vis);

    // Collect highlight shapes from the first selected instance
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.highlight_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();
      if (!results.empty()) {
        auto* registry = gui::DescriptorRegistry::instance();
        gui::Selected sel
            = registry->makeSelected(std::any(results[0].inst));
        if (sel) {
          ShapeCollector collector;
          sel.highlight(collector);
          state.highlight_rects = std::move(collector.rects);
        }
      }
    }

    // Build JSON response with selection and properties
    resp.type = 0;
    std::stringstream ss;
    ss << "{\"selected\": [";
    bool first = true;
    for (const auto& r : results) {
      if (!first) {
        ss << ", ";
      }
      ss << "{\"name\": \"" << json_escape(r.name) << "\", \"master\": \""
         << json_escape(r.master) << "\", \"bbox\": [" << r.bbox.xMin()
         << ", " << r.bbox.yMin() << ", " << r.bbox.xMax() << ", "
         << r.bbox.yMax() << "]}";
      first = false;
    }
    ss << "]";

    // Add properties for the first selected instance
    std::vector<gui::Selected> new_selectables;
    if (!results.empty()) {
      const auto& r0 = results[0];
      ss << ", \"bbox\": [" << r0.bbox.xMin() << ", " << r0.bbox.yMin()
         << ", " << r0.bbox.xMax() << ", " << r0.bbox.yMax() << "]";

      auto* registry = gui::DescriptorRegistry::instance();
      gui::Selected sel = registry->makeSelected(std::any(r0.inst));
      if (sel) {
        auto props = sel.getProperties();
        ss << ", \"properties\": [";
        bool pfirst = true;
        for (const auto& prop : props) {
          if (!pfirst) {
            ss << ", ";
          }
          serializeProperty(ss, prop, new_selectables);
          pfirst = false;
        }
        ss << "]";
      }
    }
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      state.selectables = std::move(new_selectables);
    }

    ss << "}";
    const std::string json = ss.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WsResponse SelectHandler::handleInspect(const WsRequest& req,
                                        SessionState& state)
{
  WsResponse resp;
  resp.id = req.id;
  try {
    gui::Selected sel;
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      if (req.select_id >= 0
          && req.select_id
                 < static_cast<int>(state.selectables.size())) {
        sel = state.selectables[req.select_id];
      }
    }

    // Collect highlight shapes from the inspected object
    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.highlight_rects.clear();
      state.timing_rects.clear();
      state.timing_lines.clear();
      if (sel) {
        ShapeCollector collector;
        sel.highlight(collector);
        state.highlight_rects = std::move(collector.rects);
      }
    }

    resp.type = 0;
    std::stringstream ss;
    if (sel) {
      auto props = sel.getProperties();
      std::vector<gui::Selected> new_selectables;
      ss << "{\"name\": \"" << json_escape(sel.getName())
         << "\", \"type\": \"" << json_escape(sel.getTypeName())
         << "\", \"properties\": [";
      bool first = true;
      for (const auto& prop : props) {
        if (!first) {
          ss << ", ";
        }
        serializeProperty(ss, prop, new_selectables);
        first = false;
      }
      ss << "]";
      odb::Rect bbox;
      if (sel.getBBox(bbox)) {
        ss << ", \"bbox\": [" << bbox.xMin() << ", " << bbox.yMin() << ", "
           << bbox.xMax() << ", " << bbox.yMax() << "]";
      }
      ss << "}";
      {
        std::lock_guard<std::mutex> lock(state.selectables_mutex);
        state.selectables = std::move(new_selectables);
      }
    } else {
      ss << "{\"error\": \"invalid select_id\"}";
    }
    const std::string json = ss.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WsResponse SelectHandler::handleHover(const WsRequest& req,
                                      SessionState& state)
{
  WsResponse resp;
  resp.id = req.id;
  try {
    gui::Selected sel;
    {
      std::lock_guard<std::mutex> lock(state.selectables_mutex);
      if (req.select_id >= 0
          && req.select_id
                 < static_cast<int>(state.selectables.size())) {
        sel = state.selectables[req.select_id];
      }
    }

    resp.type = 0;
    std::stringstream ss;
    if (sel) {
      ShapeCollector collector;
      sel.highlight(collector);
      if (!collector.rects.empty()) {
        ss << "{\"rects\": [";
        bool first = true;
        for (const auto& r : collector.rects) {
          if (!first) {
            ss << ", ";
          }
          ss << "[" << r.xMin() << ", " << r.yMin() << ", " << r.xMax()
             << ", " << r.yMax() << "]";
          first = false;
        }
        ss << "]}";
      } else {
        odb::Rect bbox;
        if (sel.getBBox(bbox)) {
          ss << "{\"rects\": [[" << bbox.xMin() << ", " << bbox.yMin()
             << ", " << bbox.xMax() << ", " << bbox.yMax() << "]]}";
        } else {
          ss << "{\"error\": \"no bbox\"}";
        }
      }
    } else {
      ss << "{\"error\": \"invalid select_id\"}";
    }
    const std::string json = ss.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    std::string err = std::string("server error: ") + e.what();
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

WsResponse TclHandler::handleTclEval(const WsRequest& req)
{
  WsResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    auto result = tcl_eval_->eval(req.tcl_cmd);
    std::stringstream ss;
    ss << "{\"output\": \"" << json_escape(result.output)
       << "\", \"result\": \"" << json_escape(result.result)
       << "\", \"is_error\": " << (result.is_error ? "true" : "false")
       << "}";
    const std::string json = ss.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    std::string err = std::string("server error: ") + e.what();
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

WsResponse TimingHandler::handleTimingReport(const WsRequest& req)
{
  WsResponse resp;
  resp.id = req.id;
  resp.type = 0;
  try {
    std::lock_guard<std::mutex> lock(tcl_eval_->mutex);
    auto paths
        = timing_report_->getReport(req.timing_is_setup, req.timing_max_paths);
    std::stringstream ss;
    ss << "{\"paths\": [";
    bool first_path = true;
    for (const auto& p : paths) {
      if (!first_path) {
        ss << ", ";
      }
      first_path = false;
      ss << "{\"start_clk\": \"" << json_escape(p.start_clk)
         << "\", \"end_clk\": \"" << json_escape(p.end_clk)
         << "\", \"required\": " << p.required
         << ", \"arrival\": " << p.arrival << ", \"slack\": " << p.slack
         << ", \"skew\": " << p.skew << ", \"path_delay\": " << p.path_delay
         << ", \"logic_depth\": " << p.logic_depth
         << ", \"fanout\": " << p.fanout << ", \"start_pin\": \""
         << json_escape(p.start_pin) << "\", \"end_pin\": \""
         << json_escape(p.end_pin) << "\", \"data_nodes\": [";
      bool first_node = true;
      for (const auto& n : p.data_nodes) {
        if (!first_node) {
          ss << ", ";
        }
        first_node = false;
        ss << "{\"pin\": \"" << json_escape(n.pin_name) << "\""
           << ", \"fanout\": " << n.fanout
           << ", \"rise\": " << (n.is_rising ? "true" : "false")
           << ", \"clk\": " << (n.is_clock ? "true" : "false")
           << ", \"time\": " << n.time << ", \"delay\": " << n.delay
           << ", \"slew\": " << n.slew << ", \"load\": " << n.load << "}";
      }
      ss << "], \"capture_nodes\": [";
      first_node = true;
      for (const auto& n : p.capture_nodes) {
        if (!first_node) {
          ss << ", ";
        }
        first_node = false;
        ss << "{\"pin\": \"" << json_escape(n.pin_name) << "\""
           << ", \"fanout\": " << n.fanout
           << ", \"rise\": " << (n.is_rising ? "true" : "false")
           << ", \"clk\": " << (n.is_clock ? "true" : "false")
           << ", \"time\": " << n.time << ", \"delay\": " << n.delay
           << ", \"slew\": " << n.slew << ", \"load\": " << n.load << "}";
      }
      ss << "]}";
    }
    ss << "]}";
    const std::string json = ss.str();
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    std::string err = std::string("server error: ") + e.what();
    resp.payload.assign(err.begin(), err.end());
  }
  return resp;
}

WsResponse TimingHandler::handleTimingHighlight(const WsRequest& req,
                                                SessionState& state)
{
  WsResponse resp;
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
          static const Color kStageColor{255, 255, 0, 180};
          auto [iterm, bterm] = resolvePin(block, req.timing_pin_name);
          odb::dbNet* net = iterm   ? iterm->getNet()
                            : bterm ? bterm->getNet()
                                    : nullptr;
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

    std::cerr << "TIMING_HIGHLIGHT: " << new_rects.size() << " rects, "
              << new_lines.size() << " lines"
              << " pin_name='" << req.timing_pin_name << "'\n";

    {
      std::lock_guard<std::mutex> lock(state.selection_mutex);
      state.timing_rects = std::move(new_rects);
      state.timing_lines = std::move(new_lines);
      state.highlight_rects.clear();
    }

    const std::string json = "{\"ok\": true}";
    resp.payload.assign(json.begin(), json.end());
  } catch (const std::exception& e) {
    resp.type = 2;
    std::string err = std::string("server error: ") + e.what();
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

WsResponse TileHandler::handleTile(const WsRequest& req, SessionState& state)
{
  // Snapshot current highlight state
  std::vector<odb::Rect> rects;
  std::vector<ColoredRect> colored;
  std::vector<FlightLine> lines;
  {
    std::lock_guard<std::mutex> lock(state.selection_mutex);
    rects = state.highlight_rects;
    colored = state.timing_rects;
    lines = state.timing_lines;
  }

  return dispatch_request(req, gen_, rects, colored, lines);
}

}  // namespace web
