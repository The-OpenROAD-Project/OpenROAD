// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "yaml2json.h"

#include <charconv>
#include <cstdio>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include "yaml-cpp/yaml.h"

namespace yaml2json {

namespace {

const std::string kTagPrefix = "tag:yaml.org,2002:";

std::string where(const YAML::Node& node)
{
  const YAML::Mark mark = node.Mark();
  if (mark.is_null()) {
    return "";
  }
  return "line " + std::to_string(mark.line + 1) + ", column "
         + std::to_string(mark.column + 1) + ": ";
}

[[noreturn]] void fail(const YAML::Node& node, const std::string& message)
{
  throw std::runtime_error(where(node) + message);
}

std::string quote(const std::string& s)
{
  std::string out = "\"";
  for (const char c : s) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
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
          std::snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else {
          out += c;
        }
    }
  }
  return out + "\"";
}

// A float printed the way Python's repr prints it, so that a reader doing
// str(value) sees what it saw when PyYAML loaded the same text.
std::string formatFloat(const YAML::Node& node, const std::string& text)
{
  std::string digits = text;
  if (!digits.empty() && digits[0] == '+') {
    digits.erase(0, 1);
  }
  double value = 0;
  const char* end = digits.data() + digits.size();
  const auto parsed = std::from_chars(digits.data(), end, value);
  if (parsed.ec != std::errc() || parsed.ptr != end) {
    fail(node, "cannot convert float '" + text + "'");
  }
  char buf[64];
  const auto printed = std::to_chars(buf, buf + sizeof(buf), value);
  std::string out(buf, printed.ptr);
  if (out.find_first_of(".e") == std::string::npos) {
    out += ".0";
  }
  return out;
}

std::string scalar(const YAML::Node& node)
{
  const std::string& text = node.Scalar();
  std::string tag = node.Tag();
  if (tag == "!" || tag == kTagPrefix + "str") {
    return quote(text);
  }
  if (tag != "?" && tag.rfind(kTagPrefix, 0) != 0) {
    fail(node, "unsupported tag '" + tag + "'");
  }

  static const std::regex null_re("~|null|Null|NULL|");
  static const std::regex true_re("yes|Yes|YES|true|True|TRUE|on|On|ON");
  static const std::regex false_re("no|No|NO|false|False|FALSE|off|Off|OFF");
  static const std::regex int_re("[-+]?(0|[1-9][0-9]*)");
  static const std::regex float_re(
      "[-+]?([0-9][0-9]*)?\\.[0-9]*([eE][-+][0-9]+)?");
  // Numbers PyYAML reads in YAML 1.1 forms that are easy to get wrong:
  // octal (010 is 8), hexadecimal, binary, sexagesimal (1:30 is 90),
  // underscores and the IEEE specials, which JSON cannot hold.
  static const std::regex other_number_re(
      "[-+]?(0[0-7_]+|0x[0-9a-fA-F_]+|0b[01_]+|[0-9][0-9_]*(:[0-5]?[0-9])+"
      "|[0-9][0-9_]*|[0-9][0-9_]*(:[0-5]?[0-9])+\\.[0-9_]*"
      "|([0-9][0-9_]*\\.[0-9_]*|\\.[0-9_]+)([eE][-+][0-9]+)?)"
      "|[-+]?\\.(inf|Inf|INF)|\\.(nan|NaN|NAN)");

  const auto is = [&](const std::string& name, const std::regex& re) {
    return (tag == "?" || tag == kTagPrefix + name)
           && std::regex_match(text, re);
  };
  if (is("null", null_re)) {
    return "null";
  }
  if (is("bool", true_re)) {
    return "true";
  }
  if (is("bool", false_re)) {
    return "false";
  }
  if (is("int", int_re)) {
    return text[0] == '+' ? text.substr(1) : text;
  }
  // "." alone matches float_re's shape but is a string to PyYAML.
  if (text != "." && is("float", float_re)) {
    return formatFloat(node, text);
  }
  if (tag == "?" && std::regex_match(text, other_number_re)) {
    fail(node,
         "'" + text
             + "' is a YAML 1.1 number in a form yaml2json does not convert;"
               " quote it or write it as a decimal");
  }
  if (tag != "?") {
    fail(node, "'" + text + "' is not a valid " + tag);
  }
  return quote(text);
}

void write(const YAML::Node& node, const std::string& indent, std::string& out)
{
  const std::string inner = indent + "  ";
  switch (node.Type()) {
    case YAML::NodeType::Null:
      out += "null";
      return;
    case YAML::NodeType::Scalar:
      out += scalar(node);
      return;
    case YAML::NodeType::Sequence: {
      if (node.size() == 0) {
        out += "[]";
        return;
      }
      out += "[\n";
      bool first = true;
      for (const YAML::Node& item : node) {
        out += first ? "" : ",\n";
        first = false;
        out += inner;
        write(item, inner, out);
      }
      out += "\n" + indent + "]";
      return;
    }
    case YAML::NodeType::Map: {
      if (node.size() == 0) {
        out += "{}";
        return;
      }
      out += "{\n";
      std::set<std::string> keys;
      bool first = true;
      for (const auto& entry : node) {
        const YAML::Node& key = entry.first;
        if (!key.IsScalar()) {
          fail(key, "a key must be a scalar");
        }
        if (key.Tag() == "?" && key.Scalar() == "<<") {
          fail(key, "merge keys are not supported");
        }
        if (!keys.insert(key.Scalar()).second) {
          fail(key, "duplicate key '" + key.Scalar() + "'");
        }
        out += first ? "" : ",\n";
        first = false;
        out += inner + quote(key.Scalar()) + ": ";
        write(entry.second, inner, out);
      }
      out += "\n" + indent + "}";
      return;
    }
    case YAML::NodeType::Undefined:
      break;
  }
  fail(node, "undefined node");
}

}  // namespace

std::string toJson(const YAML::Node& node)
{
  std::string out;
  write(node, "", out);
  return out + "\n";
}

std::string yamlToJson(const std::string& text)
{
  const std::vector<YAML::Node> documents = YAML::LoadAll(text);
  if (documents.size() != 1) {
    throw std::runtime_error("expected one YAML document, found "
                             + std::to_string(documents.size()));
  }
  return toJson(documents[0]);
}

}  // namespace yaml2json
