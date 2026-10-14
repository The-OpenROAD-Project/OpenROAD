// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "utl/Validate.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "utl/Logger.h"

namespace utl {

namespace val {

std::string formatDouble(const double value)
{
  return fmt::format("{}", value);
}

std::string formatJson(const boost::json::value& value)
{
  namespace json = boost::json;
  switch (value.kind()) {
    case json::kind::null:
      return "none";
    case json::kind::bool_:
      return value.get_bool() ? "true" : "false";
    case json::kind::int64:
      return std::to_string(value.get_int64());
    case json::kind::uint64:
      return std::to_string(value.get_uint64());
    case json::kind::double_:
      return formatDouble(value.get_double());
    case json::kind::string:
      return std::string(value.get_string());
    case json::kind::array: {
      std::string result = "{";
      for (const json::value& elem : value.get_array()) {
        if (result.size() > 1) {
          result += ' ';
        }
        result += formatJson(elem);
      }
      return result + "}";
    }
    case json::kind::object: {
      std::string result = "{";
      for (const auto& [key, item] : value.get_object()) {
        if (result.size() > 1) {
          result += ' ';
        }
        result += std::string(key) + " " + formatJson(item);
      }
      return result + "}";
    }
  }
  return {};
}

static std::string_view trim(std::string_view text)
{
  while (!text.empty()
         && std::isspace(static_cast<unsigned char>(text.front()))) {
    text.remove_prefix(1);
  }
  while (!text.empty()
         && std::isspace(static_cast<unsigned char>(text.back()))) {
    text.remove_suffix(1);
  }
  return text;
}

bool parseBool(std::string_view text, bool& out)
{
  std::string lower;
  for (const char c : trim(text)) {
    lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  static const std::map<std::string, bool> kSpellings = {{"true", true},
                                                         {"false", false},
                                                         {"1", true},
                                                         {"0", false},
                                                         {"yes", true},
                                                         {"no", false},
                                                         {"on", true},
                                                         {"off", false}};
  const auto it = kSpellings.find(lower);
  if (it == kSpellings.end()) {
    return false;
  }
  out = it->second;
  return true;
}

bool parseInt(std::string_view text, int64_t& out)
{
  text = trim(text);
  if (text.empty()) {
    return false;
  }
  const char* begin = text.data();
  const char* end = begin + text.size();
  if (*begin == '+') {
    ++begin;
  }
  const auto [ptr, ec] = std::from_chars(begin, end, out);
  return ec == std::errc() && ptr == end;
}

bool parseDouble(std::string_view text, double& out)
{
  const std::string copy(trim(text));
  if (copy.empty()) {
    return false;
  }
  char* end = nullptr;
  out = std::strtod(copy.c_str(), &end);
  return end == copy.c_str() + copy.size();
}

std::vector<std::string> splitList(std::string_view text)
{
  std::vector<std::string> result;
  std::string current;
  for (const char c : text) {
    if (std::isspace(static_cast<unsigned char>(c))) {
      if (!current.empty()) {
        result.push_back(current);
        current.clear();
      }
    } else {
      current += c;
    }
  }
  if (!current.empty()) {
    result.push_back(current);
  }
  return result;
}

}  // namespace val

void report(const Violation& violation, Logger* logger)
{
  logger->error(violation.tool, violation.id, "{}", violation.message);
}

////////////////////////////////////////////////////////////////

const FieldMeta* SchemaBase::find(const std::string_view name) const
{
  for (const FieldMeta* field : fields()) {
    if (field->name == name) {
      return field;
    }
  }
  return nullptr;
}

const FieldMeta* SchemaBase::findPath(const std::string_view path) const
{
  const std::size_t dot = path.find('.');
  const FieldMeta* field = find(path.substr(0, dot));
  if (field == nullptr || dot == std::string_view::npos) {
    return field;
  }
  if (field->kind != FieldMeta::Kind::kGroup) {
    return nullptr;
  }
  return field->group->findPath(path.substr(dot + 1));
}

static void collect(const SchemaBase& schema,
                    const boost::json::object& defaults,
                    const std::string& prefix,
                    const int depth,
                    std::vector<FieldInfo>& out)
{
  for (const FieldMeta* field : schema.fields()) {
    const std::string path
        = prefix.empty() ? field->name : prefix + "." + field->name;
    const boost::json::value* value = defaults.if_contains(field->name);
    FieldInfo info{path, "", field, depth};
    if (field->kind == FieldMeta::Kind::kGroup) {
      out.push_back(info);
      if (value != nullptr && value->is_object()) {
        collect(*field->group, value->get_object(), path, depth + 1, out);
      }
    } else {
      info.default_text = value ? val::formatJson(*value) : "";
      out.push_back(info);
    }
  }
}

std::vector<FieldInfo> SchemaBase::describe() const
{
  std::vector<FieldInfo> result;
  const boost::json::value defaults_json = defaults();
  collect(*this, defaults_json.as_object(), "", 0, result);
  return result;
}

static std::string unitName(const Unit unit)
{
  switch (unit) {
    case Unit::kNone:
      return "";
    case Unit::kDistance:
      return "distance";
    case Unit::kTime:
      return "time";
    case Unit::kCapacitance:
      return "capacitance";
    case Unit::kResistance:
      return "resistance";
  }
  return "";
}

static std::string joinConstraints(const FieldMeta& meta)
{
  std::string result;
  for (const std::string& constraint : meta.constraints) {
    if (constraint.empty()) {
      continue;
    }
    if (!result.empty()) {
      result += ", ";
    }
    result += constraint;
  }
  return result;
}

std::string describeText(const SchemaBase& schema)
{
  const std::vector<FieldInfo> fields = schema.describe();
  std::size_t width = 0;
  for (const FieldInfo& info : fields) {
    width = std::max(width, info.path.size());
  }
  std::string result;
  for (const FieldInfo& info : fields) {
    const FieldMeta& meta = *info.meta;
    std::string line = fmt::format("{:<{}}  ", info.path, width);
    if (meta.kind == FieldMeta::Kind::kGroup) {
      line += "(group)";
    } else {
      line += meta.type_name;
      if (meta.unit != Unit::kNone) {
        line += ", " + unitName(meta.unit);
      }
      line += " = " + info.default_text;
      if (const std::string constraints = joinConstraints(meta);
          !constraints.empty()) {
        line += " (" + constraints + ")";
      }
    }
    if (!meta.help.empty()) {
      line += "  " + meta.help;
    }
    result += line + "\n";
  }
  return result;
}

std::string describeMarkdown(const SchemaBase& schema)
{
  std::string result = "| Option | Type | Default | Description |\n";
  result += "| ----- | ----- | ----- | ----- |\n";
  for (const FieldInfo& info : schema.describe()) {
    const FieldMeta& meta = *info.meta;
    std::string type;
    std::string default_text;
    if (meta.kind == FieldMeta::Kind::kGroup) {
      type = "group";
    } else {
      type = meta.type_name;
      if (meta.unit != Unit::kNone) {
        type += ", " + unitName(meta.unit);
      }
      if (const std::string constraints = joinConstraints(meta);
          !constraints.empty()) {
        type += " (" + constraints + ")";
      }
      default_text = "`" + info.default_text + "`";
    }
    result += fmt::format(
        "| `{}` | {} | {} | {} |\n", info.path, type, default_text, meta.help);
  }
  return result;
}

static void reportObject(
    const boost::json::object& values,
    const boost::json::object& defaults,
    const std::string& prefix,
    std::vector<std::pair<std::string, std::string>>& lines)
{
  for (const auto& [key, item] : values) {
    const std::string path
        = prefix.empty() ? std::string(key) : prefix + "." + std::string(key);
    const boost::json::value* base = defaults.if_contains(key);
    if (item.is_object()) {
      static const boost::json::object kEmpty;
      reportObject(item.get_object(),
                   base && base->is_object() ? base->get_object() : kEmpty,
                   path,
                   lines);
    } else {
      const bool changed = base == nullptr || *base != item;
      lines.emplace_back(path, val::formatJson(item) + (changed ? " *" : ""));
    }
  }
}

std::string reportValues(const SchemaBase& schema,
                         const boost::json::value& values)
{
  std::vector<std::pair<std::string, std::string>> lines;
  const boost::json::value defaults = schema.defaults();
  reportObject(values.as_object(), defaults.as_object(), "", lines);
  std::size_t width = 0;
  for (const auto& [path, value] : lines) {
    width = std::max(width, path.size());
  }
  std::string result;
  for (const auto& [path, value] : lines) {
    result += fmt::format("{:<{}}  {}\n", path, width, value);
  }
  return result;
}

////////////////////////////////////////////////////////////////

boost::json::object legacyToJson(
    const std::map<std::string, std::string>& keys,
    const std::map<std::string, std::string>& flags,
    const LegacyArgs& table)
{
  boost::json::object result;
  for (const LegacyArg& entry : table) {
    std::string value;
    if (const auto it = keys.find(entry.arg); it != keys.end()) {
      value = it->second;
    } else if (flags.find(entry.arg) != flags.end()) {
      value = entry.value;
    } else {
      continue;
    }
    if (entry.custom) {
      entry.custom(result, value);
      continue;
    }
    for (const std::string& path : entry.paths) {
      result[path] = value;
    }
  }
  return result;
}

}  // namespace utl
