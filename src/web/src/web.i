// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

%{
#include <fstream>
#include "ord/OpenRoad.hh"
#include "odb/db.h"
#include "web/web.h"
#include "timing_report.h"
#include "json_builder.h"
%}

%include "../../Exception.i"

%inline %{

namespace web {

void
web_server_cmd(int port, const char* doc_root)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  server->serve(port, doc_root);
}

static void serializePaths(web::JsonBuilder& builder,
                           const std::vector<web::TimingPathSummary>& paths)
{
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
    builder.endObject();
  }
  builder.endArray();
}

void
web_export_json_cmd(const char* output_file,
                    const char* design_name,
                    const char* stage_name,
                    const char* variant_name)
{
  ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
  odb::dbDatabase* db = openroad->getDb();
  sta::dbSta* sta = openroad->getSta();
  web::TimingReport report(sta);

  web::JsonBuilder builder;
  builder.beginObject();

  // Metadata
  builder.beginObject("metadata");
  if (design_name[0] != '\0') {
    builder.field("design", design_name);
  }
  if (stage_name[0] != '\0') {
    builder.field("stage", stage_name);
  }
  if (variant_name[0] != '\0') {
    builder.field("variant", variant_name);
  }
  builder.endObject();

  // Tech data (matches WebSocket 'tech' response)
  odb::dbBlock* block = db->getChip() ? db->getChip()->getBlock() : nullptr;
  builder.beginObject("tech");
  builder.beginArray("layers");
  if (db->getTech()) {
    for (auto* layer : db->getTech()->getLayers()) {
      builder.value(layer->getName());
    }
  }
  builder.endArray();
  builder.beginArray("sites");
  builder.endArray();
  builder.field("has_liberty", sta != nullptr);
  if (block) {
    builder.field("dbu_per_micron", block->getDbUnitsPerMicron());
  }
  builder.endObject();

  // Bounds (matches WebSocket 'bounds' response)
  builder.beginObject("bounds");
  if (block) {
    odb::Rect die = block->getDieArea();
    builder.beginArray("bounds");
    builder.beginArray();
    builder.value(die.yMin());
    builder.value(die.xMin());
    builder.endArray();
    builder.beginArray();
    builder.value(die.yMax());
    builder.value(die.xMax());
    builder.endArray();
    builder.endArray();
    builder.field("shapes_ready", true);
  }
  builder.endObject();

  // Chart filters (path groups + clocks)
  auto filters = report.getChartFilters();
  builder.beginObject("chart_filters");
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

  // Slack histogram (setup)
  auto histogram = report.getSlackHistogram(true);
  builder.beginObject("slack_histogram");
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

  // Timing paths — setup (top 50)
  builder.beginObject("timing_report_setup");
  serializePaths(builder, report.getReport(true, 50));
  builder.endObject();

  // Timing paths — hold (top 50)
  builder.beginObject("timing_report_hold");
  serializePaths(builder, report.getReport(false, 50));
  builder.endObject();

  builder.endObject();

  std::ofstream out(output_file);
  if (!out.is_open()) {
    throw std::runtime_error(
        std::string("Cannot open output file: ") + output_file);
  }
  out << builder.str();
}

} // namespace web

%} // inline
