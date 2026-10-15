// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "utl/OptionsArgs.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "utl/Logger.h"

namespace utl {

namespace {

std::string lastSegment(const std::string& path)
{
  const std::size_t dot = path.rfind('.');
  return dot == std::string::npos ? path : path.substr(dot + 1);
}

bool isKeyword(const std::string& token)
{
  return token.size() > 1 && token[0] == '-'
         && std::isalpha(static_cast<unsigned char>(token[1]));
}

bool isPrefixOf(const std::string& prefix, const std::string& name)
{
  return name.compare(0, prefix.size(), prefix) == 0;
}

// Merges `from` into `into`, recursing into nested dictionaries so that a
// later -options dict adds to an earlier one instead of replacing its
// groups.
void merge(boost::json::object& into, const boost::json::object& from)
{
  for (const auto& [key, item] : from) {
    boost::json::value* existing = into.if_contains(key);
    if (existing != nullptr && existing->is_object() && item.is_object()) {
      merge(existing->get_object(), item.get_object());
    } else {
      into[key] = item;
    }
  }
}

}  // namespace

std::vector<ArgSpec> argSpecs(const SchemaBase& schema)
{
  std::vector<ArgSpec> specs;
  for (const FieldInfo& info : schema.describe()) {
    const FieldMeta& meta = *info.meta;
    if (meta.kind == FieldMeta::Kind::kGroup) {
      continue;
    }
    ArgSpec spec{"-" + info.path, info.path, &meta, false, true};
    if (meta.kind == FieldMeta::Kind::kScalar && meta.type_name == "bool") {
      spec.is_flag = true;
      if (info.default_text == "true") {
        // The flag turns the option off: -no_ on the field, after its groups.
        const std::size_t dot = info.path.rfind('.');
        spec.name
            = "-" + info.path.substr(0, dot == std::string::npos ? 0 : dot + 1)
              + "no_" + lastSegment(info.path);
        spec.flag_value = false;
      }
    }
    specs.push_back(std::move(spec));
  }
  return specs;
}

std::string argSynopsis(const SchemaBase& schema)
{
  std::string result = std::string("[") + kOptionsSwitch + " dict]";
  for (const ArgSpec& spec : argSpecs(schema)) {
    if (!result.empty()) {
      result += ' ';
    }
    result += '[' + spec.name;
    if (!spec.is_flag) {
      result += ' ' + lastSegment(spec.path);
    }
    result += ']';
  }
  return result;
}

boost::json::object parseArgs(const std::vector<std::string>& argv,
                              const SchemaBase& schema,
                              const ArgAliases& aliases,
                              const UnitScaler& scaler,
                              const DictParser& dict_parser,
                              Logger* logger)
{
  const std::vector<ArgSpec> specs = argSpecs(schema);
  const ToolId tool = schema.tool();
  const int id = schema.genericId();

  // The switch a token names: -options, an alias or a generated switch, by
  // exact name or else by unique prefix.  Returns true for -options.
  auto resolve = [&](const std::string& token,
                     const ArgAlias*& alias,
                     const ArgSpec*& spec) -> bool {
    alias = nullptr;
    spec = nullptr;
    if (token == kOptionsSwitch) {
      return true;
    }
    for (const ArgAlias& candidate : aliases) {
      if (candidate.name == token) {
        alias = &candidate;
        return false;
      }
    }
    for (const ArgSpec& candidate : specs) {
      if (candidate.name == token) {
        spec = &candidate;
        return false;
      }
    }
    // A prefix: a unique alias wins, as the older names were the ones
    // scripts abbreviated; otherwise a unique generated switch or -options.
    std::vector<std::string> matches;
    for (const ArgAlias& candidate : aliases) {
      if (isPrefixOf(token, candidate.name)) {
        alias = &candidate;
        matches.push_back(candidate.name);
      }
    }
    if (matches.size() == 1) {
      return false;
    }
    alias = nullptr;
    for (const ArgSpec& candidate : specs) {
      if (isPrefixOf(token, candidate.name)) {
        spec = &candidate;
        matches.push_back(candidate.name);
      }
    }
    if (isPrefixOf(token, kOptionsSwitch)) {
      matches.push_back(kOptionsSwitch);
    }
    if (matches.empty()) {
      logger->error(tool, id, "{} is not a known keyword or flag.", token);
    }
    if (matches.size() > 1) {
      std::string names;
      for (const std::string& name : matches) {
        names += ' ' + name;
      }
      logger->error(tool, id, "{} is ambiguous:{}", token, names);
    }
    return spec == nullptr;
  };

  // A value with a unit, in the user's units, scaled to the API's.
  auto scaled = [&](const FieldMeta* meta,
                    const std::string& value) -> boost::json::value {
    double number = 0;
    if (meta != nullptr && meta->unit != Unit::kNone && scaler
        && val::parseDouble(value, number)) {
      return scaler(meta->unit, number);
    }
    return boost::json::value(value);
  };

  boost::json::object result;
  for (std::size_t i = 0; i < argv.size(); ++i) {
    const std::string& token = argv[i];
    if (!isKeyword(token)) {
      logger->error(tool, id, "unexpected argument {}.", token);
    }
    const ArgAlias* alias = nullptr;
    const ArgSpec* spec = nullptr;
    if (resolve(token, alias, spec)) {
      // -options dict: the whole struct, merged under what came before.
      if (i + 1 >= argv.size()) {
        logger->error(tool, id, "{} missing value.", token);
      }
      if (!dict_parser) {
        logger->error(tool, id, "{} is not supported here.", kOptionsSwitch);
      }
      merge(result, dict_parser(argv[++i]));
      continue;
    }

    if (alias != nullptr) {
      std::string value = alias->value;
      if (!alias->is_flag) {
        if (i + 1 >= argv.size()) {
          logger->error(tool, id, "{} missing value.", token);
        }
        value = argv[++i];
      }
      if (alias->custom) {
        alias->custom(result, value);
      } else {
        for (const std::string& path : alias->paths) {
          result[path] = scaled(schema.findPath(path), value);
        }
      }
      continue;
    }
    if (spec->is_flag) {
      result[spec->path] = spec->flag_value;
      continue;
    }
    if (i + 1 >= argv.size()) {
      logger->error(tool, id, "{} missing value.", token);
    }
    result[spec->path] = scaled(spec->meta, argv[++i]);
  }
  return result;
}

namespace {

constexpr const char* kOptionsHelp
    = "The options as a nested dict of the fields below, or dotted keys "
      "(timing.nets_percentage); a switch after it overrides it.";

std::string unitName(const Unit unit)
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

std::string joinConstraints(const FieldMeta& meta)
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

// The type column of a switch: nothing for a flag, else the type, unit and
// constraints of the value.
std::string typeText(const ArgSpec& spec)
{
  if (spec.is_flag) {
    return "flag";
  }
  std::string text = spec.meta->type_name;
  if (spec.meta->unit != Unit::kNone) {
    text += ", " + unitName(spec.meta->unit);
  }
  if (const std::string constraints = joinConstraints(*spec.meta);
      !constraints.empty()) {
    text += " (" + constraints + ")";
  }
  return text;
}

}  // namespace

std::string describeArgsText(const SchemaBase& schema)
{
  const std::vector<FieldInfo> fields = schema.describe();
  const std::vector<ArgSpec> specs = argSpecs(schema);
  std::size_t width = 0;
  for (const ArgSpec& spec : specs) {
    width = std::max(width, spec.name.size());
  }
  std::string result
      = fmt::format("{:<{}}  dict  {}\n", kOptionsSwitch, width, kOptionsHelp);
  std::size_t next = 0;
  for (const FieldInfo& info : fields) {
    if (info.meta->kind == FieldMeta::Kind::kGroup) {
      continue;
    }
    const ArgSpec& spec = specs[next++];
    std::string line
        = fmt::format("{:<{}}  {}", spec.name, width, typeText(spec));
    if (!spec.is_flag) {
      line += " = " + info.default_text;
    }
    if (!info.meta->help.empty()) {
      line += "  " + info.meta->help;
    }
    result += line + "\n";
  }
  return result;
}

std::string describeArgsMarkdown(const SchemaBase& schema)
{
  std::string result = "| Switch Name | Type | Default | Description |\n";
  result += "| ----- | ----- | ----- | ----- |\n";
  result
      += fmt::format("| `{}` | dict |  | {} |\n", kOptionsSwitch, kOptionsHelp);
  const std::vector<ArgSpec> specs = argSpecs(schema);
  std::size_t next = 0;
  for (const FieldInfo& info : schema.describe()) {
    if (info.meta->kind == FieldMeta::Kind::kGroup) {
      continue;
    }
    const ArgSpec& spec = specs[next++];
    result += fmt::format("| `{}` | {} | {} | {} |\n",
                          spec.name,
                          typeText(spec),
                          spec.is_flag ? "" : "`" + info.default_text + "`",
                          info.meta->help);
  }
  return result;
}

}  // namespace utl
