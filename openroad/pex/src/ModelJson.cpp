// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/ModelJson.h"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "boost/json.hpp"
#include "pex/Model.h"

namespace pex {

namespace {

namespace json = boost::json;

// Values are written to seven significant digits: the solves behind them
// are good to a fraction of a percent, and the full digits of a double
// would more than double the file.
double rounded(const double v)
{
  char text[32];
  const auto [end, error] = std::to_chars(
      text, text + sizeof text, v, std::chars_format::general, 7);
  if (error != std::errc{}) {
    return v;
  }
  double out = v;
  std::from_chars(text, end, out);
  return out;
}

json::array numbers(const std::vector<double>& values)
{
  json::array out;
  out.reserve(values.size());
  for (const double v : values) {
    out.push_back(rounded(v));
  }
  return out;
}

std::vector<double> numbers(const json::value& value)
{
  std::vector<double> out;
  for (const json::value& v : value.as_array()) {
    out.push_back(v.to_number<double>());
  }
  return out;
}

json::object lateral(const LateralKey& key, const LateralTable& table)
{
  json::object o;
  o["layer"] = key.layer;
  o["below"] = key.below;
  o["above"] = key.above;
  o["widths"] = numbers(table.widths());
  o["spacings"] = numbers(table.spacings());
  // The table is its own mirror image, so only the points with left <=
  // right are written, flat in [width][left][right] order; each value is
  // [ground, left, right, below, above].
  json::array values;
  const int n = table.spacingCount();
  for (int w = 0; w < table.widthCount(); w++) {
    for (int l = 0; l < n; l++) {
      for (int r = l; r < n; r++) {
        const LateralCap& c = table.at(w, l, r);
        values.push_back(json::array{rounded(c.ground),
                                     rounded(c.left),
                                     rounded(c.right),
                                     rounded(c.below),
                                     rounded(c.above)});
      }
    }
  }
  o["values"] = std::move(values);
  return o;
}

void readLateral(const json::object& o, Model& model)
{
  const LateralKey key{std::string(o.at("layer").as_string()),
                       std::string(o.at("below").as_string()),
                       std::string(o.at("above").as_string())};
  LateralTable table(numbers(o.at("widths")), numbers(o.at("spacings")));
  const json::array& values = o.at("values").as_array();
  const int n = table.spacingCount();
  const size_t points = table.widthCount() * n * (n + 1) / 2;
  if (values.size() != points) {
    throw std::runtime_error("lateral table of " + key.layer
                             + " has the wrong number of values");
  }
  size_t i = 0;
  for (int w = 0; w < table.widthCount(); w++) {
    for (int l = 0; l < n; l++) {
      for (int r = l; r < n; r++) {
        const std::vector<double> v = numbers(values[i++]);
        if (v.size() != 5) {
          throw std::runtime_error("lateral table of " + key.layer
                                   + " has a malformed value");
        }
        table.set(w, l, r, {v[0], v[1], v[2], v[3], v[4]});
      }
    }
  }
  model.addLateral(key, std::move(table));
}

json::object crossing(const CrossingKey& key, const CrossingTable& table)
{
  json::object o;
  o["layer"] = key.layer;
  o["cover"] = key.cover;
  if (!key.opposite.empty()) {
    o["opposite"] = key.opposite;
  }
  o["gaps"] = numbers(table.gaps());
  o["spacings"] = numbers(table.spacings());
  // Flat, in [spacing][gap] order with no neighbor last.
  json::array total;
  json::array cross;
  for (const CrossingShare& s : table.values()) {
    total.push_back(rounded(s.total));
    cross.push_back(rounded(s.cross));
  }
  o["total"] = std::move(total);
  o["cross"] = std::move(cross);
  if (table.hasReach()) {
    const CrossingReach& r = table.reach();
    json::object reach;
    reach["width"] = rounded(r.width);
    reach["past"] = numbers(r.past);
    reach["extents"] = numbers(r.extents);
    // Flat, in [past][extent] order.
    json::array reach_total;
    json::array reach_cross;
    for (const CrossingShare& s : r.values) {
      reach_total.push_back(rounded(s.total));
      reach_cross.push_back(rounded(s.cross));
    }
    reach["total"] = std::move(reach_total);
    reach["cross"] = std::move(reach_cross);
    o["reach"] = std::move(reach);
  }
  return o;
}

void readCrossing(const json::object& o, Model& model)
{
  CrossingKey key{std::string(o.at("layer").as_string()),
                  std::string(o.at("cover").as_string()),
                  ""};
  if (const json::value* opposite = o.if_contains("opposite")) {
    key.opposite = std::string(opposite->as_string());
  }
  const std::vector<double> gaps = numbers(o.at("gaps"));
  const std::vector<double> spacings = numbers(o.at("spacings"));
  const std::vector<double> total = numbers(o.at("total"));
  const std::vector<double> cross = numbers(o.at("cross"));
  // The two are zipped into shares; the table checks the count against
  // the grids.
  if (total.size() != cross.size()) {
    throw std::runtime_error("crossing table of " + key.layer
                             + " has total and cross arrays that differ");
  }
  std::vector<CrossingShare> values;
  for (size_t i = 0; i < total.size(); i++) {
    values.push_back({total[i], cross[i]});
  }
  CrossingTable table(gaps, values, spacings);
  if (const json::value* found = o.if_contains("reach")) {
    const json::object& r = found->as_object();
    CrossingReach reach;
    reach.width = r.at("width").to_number<double>();
    reach.past = numbers(r.at("past"));
    reach.extents = numbers(r.at("extents"));
    const std::vector<double> reach_total = numbers(r.at("total"));
    const std::vector<double> reach_cross = numbers(r.at("cross"));
    if (reach_total.size() != reach_cross.size()) {
      throw std::runtime_error("crossing reach of " + key.layer
                               + " has total and cross arrays that differ");
    }
    for (size_t i = 0; i < reach_total.size(); i++) {
      reach.values.push_back({reach_total[i], reach_cross[i]});
    }
    table.setReach(std::move(reach));
  }
  model.addCrossing(key, std::move(table));
}

json::object diagonal(const DiagonalKey& key, const DiagonalTable& table)
{
  json::object o;
  o["layer"] = key.layer;
  o["other"] = key.other;
  if (!key.opposite.empty()) {
    o["opposite"] = key.opposite;
  }
  o["widths"] = numbers(table.widths());
  o["gaps"] = numbers(table.gaps());
  o["spacings"] = numbers(table.spacings());
  o["far_spacings"] = numbers(table.farSpacings());
  // Flat, in [width][gap][spacing][far] order; each value is [coupling,
  // total_delta, lateral_delta], with plane_delta after them when the
  // table has an opposite plane.
  json::array values;
  values.reserve(table.values().size());
  for (const DiagonalCap& c : table.values()) {
    json::array v{
        rounded(c.coupling), rounded(c.total_delta), rounded(c.lateral_delta)};
    if (!key.opposite.empty()) {
      v.push_back(rounded(c.plane_delta));
    }
    values.push_back(std::move(v));
  }
  o["values"] = std::move(values);
  return o;
}

void readDiagonal(const json::object& o, Model& model)
{
  DiagonalKey key{std::string(o.at("layer").as_string()),
                  std::string(o.at("other").as_string()),
                  ""};
  if (const json::value* opposite = o.if_contains("opposite")) {
    key.opposite = std::string(opposite->as_string());
  }
  DiagonalTable table(numbers(o.at("widths")),
                      numbers(o.at("gaps")),
                      numbers(o.at("spacings")),
                      numbers(o.at("far_spacings")));
  const json::array& values = o.at("values").as_array();
  if (values.size() != table.values().size()) {
    throw std::runtime_error("diagonal table of " + key.layer
                             + " has the wrong number of values");
  }
  const int nf = table.farCount();
  const int ns = table.spacingCount();
  const int ng = table.gapCount();
  for (size_t i = 0; i < values.size(); i++) {
    const std::vector<double> v = numbers(values[i]);
    if (v.size() != 3 && (v.size() != 4 || key.opposite.empty())) {
      throw std::runtime_error("diagonal table of " + key.layer
                               + " has a malformed value");
    }
    const int k = static_cast<int>(i);
    table.set(k / nf / ns / ng,
              (k / nf / ns) % ng,
              (k / nf) % ns,
              k % nf,
              {v[0], v[1], v[2], v.size() > 3 ? v[3] : 0});
  }
  model.addDiagonal(key, std::move(table));
}

json::object end(const LateralKey& key, const EndTable& table)
{
  json::object o;
  o["layer"] = key.layer;
  o["below"] = key.below;
  o["above"] = key.above;
  o["widths"] = numbers(table.widths());
  o["spacings"] = numbers(table.spacings());
  o["gaps"] = numbers(table.gaps());
  // Flat, in [width][spacing][gap] order with the open spacing and the
  // missing facing wire last; each value is [total, to the plane below,
  // to the plane above, to the neighbors, to the facing wire].
  json::array values;
  values.reserve(table.values().size());
  for (const EndValue& v : table.values()) {
    values.push_back(json::array{rounded(v.total),
                                 rounded(v.below),
                                 rounded(v.above),
                                 rounded(v.lateral),
                                 rounded(v.facing)});
  }
  o["values"] = std::move(values);
  if (table.hasSides()) {
    // Flat, in [width][left][right] order over the pairs with left <
    // right; each value is [total, to the plane below, to the plane above,
    // to the left neighbor, to the right neighbor].
    json::array sides;
    const int ns = table.spacingCount();
    for (int w = 0; w < static_cast<int>(table.widths().size()); w++) {
      for (int l = 0; l < ns; l++) {
        for (int r = l + 1; r < ns; r++) {
          const EndSides v = table.sides(w, l, r);
          sides.push_back(json::array{rounded(v.total),
                                      rounded(v.below),
                                      rounded(v.above),
                                      rounded(v.left),
                                      rounded(v.right)});
        }
      }
    }
    o["sides"] = std::move(sides);
  }
  if (table.hasCorners()) {
    // By width: [inner total, to the plane below, to the plane above, outer
    // total, below, above].
    json::array corners;
    for (int w = 0; w < static_cast<int>(table.widths().size()); w++) {
      const Corners& c = table.corners(w);
      corners.push_back(json::array{rounded(c.inner.total),
                                    rounded(c.inner.below),
                                    rounded(c.inner.above),
                                    rounded(c.outer.total),
                                    rounded(c.outer.below),
                                    rounded(c.outer.above)});
    }
    o["corners"] = std::move(corners);
  }
  return o;
}

void readEnd(const json::object& o, Model& model)
{
  const LateralKey key{std::string(o.at("layer").as_string()),
                       std::string(o.at("below").as_string()),
                       std::string(o.at("above").as_string())};
  EndTable table(numbers(o.at("widths")),
                 numbers(o.at("spacings")),
                 numbers(o.at("gaps")));
  const json::array& values = o.at("values").as_array();
  if (values.size() != table.values().size()) {
    throw std::runtime_error("end table of " + key.layer
                             + " has the wrong number of values");
  }
  const int ng = table.gapCount();
  const int ns = table.spacingCount();
  for (size_t i = 0; i < values.size(); i++) {
    const std::vector<double> v = numbers(values[i]);
    if (v.size() != 5) {
      throw std::runtime_error("end table of " + key.layer
                               + " has a malformed value");
    }
    const int k = static_cast<int>(i);
    table.set(
        k / ng / ns, (k / ng) % ns, k % ng, {v[0], v[1], v[2], v[3], v[4]});
  }
  if (const json::value* found = o.if_contains("sides")) {
    const json::array& sides = found->as_array();
    const int nw = static_cast<int>(table.widths().size());
    if (static_cast<int>(sides.size()) != nw * ns * (ns - 1) / 2) {
      throw std::runtime_error("end table of " + key.layer
                               + " has the wrong number of sides");
    }
    size_t i = 0;
    for (int w = 0; w < nw; w++) {
      for (int l = 0; l < ns; l++) {
        for (int r = l + 1; r < ns; r++) {
          const std::vector<double> v = numbers(sides[i++]);
          if (v.size() != 5) {
            throw std::runtime_error("end table of " + key.layer
                                     + " has a malformed side");
          }
          table.setSides(w, l, r, {v[0], v[1], v[2], v[3], v[4]});
        }
      }
    }
  }
  if (const json::value* found = o.if_contains("corners")) {
    const json::array& corners = found->as_array();
    if (corners.size() != table.widths().size()) {
      throw std::runtime_error("end table of " + key.layer
                               + " has the wrong number of corners");
    }
    for (size_t w = 0; w < corners.size(); w++) {
      const std::vector<double> v = numbers(corners[w]);
      if (v.size() != 6) {
        throw std::runtime_error("end table of " + key.layer
                                 + " has a malformed corner");
      }
      table.setCorners(static_cast<int>(w),
                       {{v[0], v[1], v[2]}, {v[3], v[4], v[5]}});
    }
  }
  model.addEnd(key, std::move(table));
}

}  // namespace

void writeModel(const Model& model,
                const ModelSource& source,
                std::ostream& out)
{
  json::object root;
  root["format"] = "pex model";
  root["version"] = kModelFormatVersion;
  root["technology"] = source.technology;
  root["source"] = source.file;
  root["fingerprint"] = source.fingerprint;
  json::array laterals;
  for (const auto& [key, table] : model.laterals()) {
    laterals.push_back(lateral(key, table));
  }
  root["lateral"] = std::move(laterals);
  json::array crossings;
  for (const auto& [key, table] : model.crossings()) {
    crossings.push_back(crossing(key, table));
  }
  root["crossing"] = std::move(crossings);
  json::array diagonals;
  for (const auto& [key, table] : model.diagonals()) {
    diagonals.push_back(diagonal(key, table));
  }
  root["diagonal"] = std::move(diagonals);
  json::array ends;
  for (const auto& [key, table] : model.ends()) {
    ends.push_back(end(key, table));
  }
  root["end"] = std::move(ends);
  out << json::serialize(root) << '\n';
}

void writeModelFile(const Model& model,
                    const ModelSource& source,
                    const std::string& path)
{
  const std::string temporary = path + ".tmp";
  {
    std::ofstream out(temporary);
    if (!out.good()) {
      throw std::runtime_error("cannot write " + temporary);
    }
    writeModel(model, source, out);
    if (!out.good()) {
      throw std::runtime_error("cannot write " + temporary);
    }
  }
  std::error_code error;
  std::filesystem::rename(temporary, path, error);
  if (error) {
    throw std::runtime_error("cannot replace " + path + ": " + error.message());
  }
}

Model readModel(std::istream& in, ModelSource* source)
{
  std::stringstream buffer;
  buffer << in.rdbuf();
  json::value root;
  try {
    root = json::parse(buffer.str());
  } catch (const std::exception& e) {
    throw std::runtime_error(std::string("cannot parse the model: ")
                             + e.what());
  }
  Model model;
  try {
    const json::object& o = root.as_object();
    if (o.at("format").as_string() != "pex model") {
      throw std::runtime_error("not a pex model");
    }
    const int version = o.at("version").to_number<int>();
    if (version != kModelFormatVersion) {
      throw std::runtime_error("model format version " + std::to_string(version)
                               + "; this build reads "
                               + std::to_string(kModelFormatVersion));
    }
    if (source != nullptr) {
      source->technology = std::string(o.at("technology").as_string());
      source->file = std::string(o.at("source").as_string());
      source->fingerprint = std::string(o.at("fingerprint").as_string());
    }
    // A kind of table the file does not have is simply absent.
    auto tables = [&](const char* kind) -> const json::array& {
      static const json::array none;
      const json::value* v = o.if_contains(kind);
      return v == nullptr ? none : v->as_array();
    };
    for (const json::value& v : tables("lateral")) {
      readLateral(v.as_object(), model);
    }
    for (const json::value& v : tables("crossing")) {
      readCrossing(v.as_object(), model);
    }
    for (const json::value& v : tables("diagonal")) {
      readDiagonal(v.as_object(), model);
    }
    for (const json::value& v : tables("end")) {
      readEnd(v.as_object(), model);
    }

  } catch (const std::runtime_error&) {
    throw;
  } catch (const std::exception& e) {
    // Boost.JSON's own errors included.
    throw std::runtime_error(std::string("malformed model: ") + e.what());
  }
  return model;
}

Model readModelFile(const std::string& path, ModelSource* source)
{
  std::ifstream in(path);
  if (!in.good()) {
    throw std::runtime_error("cannot read " + path);
  }
  return readModel(in, source);
}

}  // namespace pex
