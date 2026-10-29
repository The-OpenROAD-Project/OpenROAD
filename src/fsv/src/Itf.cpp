// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/Itf.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <functional>
#include <istream>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fsv/Stack.h"

namespace fsv {

namespace {

struct Token
{
  std::string text;
  int line;
};

// Heights closer than this (µm) are treated as equal.
constexpr double kHeightTolerance = 1e-9;
// The via endpoint that names the substrate, i.e. the ground plane.
constexpr std::string_view kSubstrate = "SUBSTRATE";

[[noreturn]] void fail(const int line, const std::string_view message)
{
  throw std::runtime_error("ITF line " + std::to_string(line) + ": "
                           + std::string(message));
}

// For errors about the file as a whole, which have no single line.
[[noreturn]] void failFile(const std::string_view message)
{
  throw std::runtime_error("ITF: " + std::string(message));
}

// Parses a token as a finite number. The stream uses the classic locale, so
// a decimal point parses the same whatever the C locale is (std::strtod
// does not). Floating-point std::from_chars would too, but it is unavailable
// before macOS 26.
double parseNumber(const Token& token, const std::string_view key)
{
  std::istringstream in{std::string(token.text)};
  in.imbue(std::locale::classic());
  double v = 0;
  in >> v;
  const bool whole = !in.fail() && in.peek() == std::char_traits<char>::eof();
  if (!whole || !std::isfinite(v)) {
    fail(token.line,
         std::string(key) + " expects a finite number, got '" + token.text
             + "'");
  }
  return v;
}

// Splits the file into words and the punctuation { } = ( ) ,. A '$' starts a
// comment that runs to the end of the line (layer names cannot contain '$').
std::vector<Token> tokenize(std::istream& in)
{
  constexpr std::string_view kPunctuation = "{}=(),";
  constexpr std::string_view kWordEnd = " \t\r{}=(),";
  std::vector<Token> tokens;
  std::string text;
  for (int line = 1; std::getline(in, text); line++) {
    const size_t comment = text.find('$');
    if (comment != std::string::npos) {
      text.erase(comment);
    }
    size_t i = 0;
    while (i < text.size()) {
      const char c = text[i];
      if (std::isspace(static_cast<unsigned char>(c))) {
        i++;
      } else if (kPunctuation.find(c) != std::string_view::npos) {
        tokens.push_back({std::string(1, c), line});
        i++;
      } else {
        const size_t end = text.find_first_of(kWordEnd, i);
        tokens.push_back({text.substr(i, end - i), line});
        i = end == std::string::npos ? text.size() : end;
      }
    }
  }
  return tokens;
}

// One DIELECTRIC, CONDUCTOR or VIA block: KEY = value pairs and bare flags.
struct Block
{
  std::string kind;
  std::string name;
  int line;
  // Transparent comparators allow lookups by std::string_view.
  std::map<std::string, Token, std::less<>> values;
  std::map<std::string, int, std::less<>> flags;  // flag -> line

  bool has(const std::string_view key) const
  {
    return values.find(key) != values.end();
  }

  double number(const std::string_view key, const double fallback) const
  {
    const auto it = values.find(key);
    return it == values.end() ? fallback : parseNumber(it->second, key);
  }

  // Line of `key`'s value, or of the block when it is absent.
  int lineOf(const std::string_view key) const
  {
    const auto it = values.find(key);
    return it == values.end() ? line : it->second.line;
  }

  double required(const std::string_view key) const
  {
    if (!has(key)) {
      fail(line, kind + " " + name + " is missing " + std::string(key));
    }
    return number(key, 0);
  }

  // The value of `key`, or an empty string when it is absent.
  const std::string& text(const std::string_view key) const
  {
    static const std::string kNone;
    const auto it = values.find(key);
    return it == values.end() ? kNone : it->second.text;
  }
};

class Parser
{
 public:
  explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

  Stack parse();

 private:
  const Token& peek() const;
  Token next();
  void expect(std::string_view text);
  Block parseBlock(std::string_view kind);
  void checkKeys(const Block& block,
                 const std::set<std::string>& values,
                 const std::set<std::string>& flags) const;
  // The layers placed by height, before the ground shift.
  struct Placed
  {
    std::vector<Stack::Dielectric> dielectrics;
    std::vector<Stack::Conductor> conductors;
  };
  Placed place(const std::vector<Block>& layers);
  void addVias(const std::vector<Block>& vias, Stack& stack) const;

  std::vector<Token> tokens_;
  size_t pos_ = 0;
  // Line of each conductor's block, for errors found after placement.
  std::map<std::string, int, std::less<>> conductor_lines_;
};

const Token& Parser::peek() const
{
  static const Token kEnd{"", -1};
  return pos_ < tokens_.size() ? tokens_[pos_] : kEnd;
}

Token Parser::next()
{
  if (pos_ >= tokens_.size()) {
    const int line = tokens_.empty() ? 0 : tokens_.back().line;
    fail(line, "unexpected end of file");
  }
  return tokens_[pos_++];
}

void Parser::expect(const std::string_view text)
{
  const Token t = next();
  if (t.text != text) {
    fail(t.line, "expected '" + std::string(text) + "', got '" + t.text + "'");
  }
}

Block Parser::parseBlock(const std::string_view kind)
{
  Block block;
  block.kind = kind;
  const Token name = next();
  block.name = name.text;
  block.line = name.line;
  expect("{");
  while (peek().text != "}") {
    const Token key = next();
    if (peek().text == "=") {
      next();
      const Token value = next();
      if (value.text == "{" || value.text == "}" || value.text == "=") {
        fail(value.line, "missing value for " + key.text);
      }
      if (block.values.count(key.text)) {
        fail(key.line, key.text + " given twice in " + block.name);
      }
      block.values.emplace(key.text, value);
    } else if (peek().text == "{" || peek().text == "(") {
      fail(key.line,
           "unsupported table " + key.text + " in " + block.kind + " "
               + block.name);
    } else {
      block.flags[key.text] = key.line;
    }
  }
  expect("}");
  return block;
}

void Parser::checkKeys(const Block& block,
                       const std::set<std::string>& values,
                       const std::set<std::string>& flags) const
{
  for (const auto& [key, token] : block.values) {
    if (!values.count(key)) {
      fail(token.line,
           "unsupported keyword " + key + " in " + block.kind + " "
               + block.name);
    }
  }
  for (const auto& [flag, line] : block.flags) {
    if (!flags.count(flag)) {
      fail(line,
           "unsupported keyword " + flag + " in " + block.kind + " "
               + block.name);
    }
  }
}

// Places the layers bottom-up. A layer's bottom is the top of the planar
// dielectric below it. A planar dielectric's top is the top of its
// MEASURED_FROM layer (default: the layer below) plus THICKNESS. A conductor
// sits on the layer below it and is embedded in the dielectrics above. A
// conformal dielectric wraps its ASSOCIATED_CONDUCTOR or, without one, every
// conductor that rises above the current planar top.
Parser::Placed Parser::place(const std::vector<Block>& layers)
{
  Placed placed;
  std::map<std::string, double> tops_by_name;
  std::set<std::string> names;
  double planar_top = 0;
  for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
    const Block& b = *it;
    if (!names.insert(b.name).second) {
      fail(b.line, "layer " + b.name + " is defined twice");
    }
    if (b.kind == "CONDUCTOR") {
      const double thickness = b.required("THICKNESS");
      if (!(thickness > 0)) {
        fail(b.lineOf("THICKNESS"),
             "CONDUCTOR " + b.name + " needs a positive THICKNESS");
      }
      conductor_lines_[b.name] = b.line;
      Stack::Conductor conductor;
      conductor.name = b.name;
      conductor.bottom = planar_top;
      conductor.top = planar_top + thickness;
      conductor.min_width = b.number("WMIN", 0);
      conductor.min_spacing = b.number("SMIN", 0);
      placed.conductors.push_back(std::move(conductor));
      continue;
    }
    const double eps = b.required("ER");
    if (!(eps > 0)) {
      fail(b.lineOf("ER"), "ER of " + b.name + " must be positive");
    }
    if (b.flags.count("IS_CONFORMAL")) {
      if (b.number("THICKNESS", 0) != 0) {
        fail(b.line,
             "IS_CONFORMAL dielectric " + b.name
                 + " with a non-zero THICKNESS is not supported");
      }
      if (b.has("MEASURED_FROM") && b.text("MEASURED_FROM") != "TOP_OF_CHIP") {
        fail(b.line,
             "IS_CONFORMAL dielectric " + b.name
                 + " cannot use MEASURED_FROM a layer");
      }
      const Stack::ConformalDielectric conformal{
          .name = b.name,
          .side_thickness = b.number("SW_T", 0),
          .top_thickness = b.number("TW_T", 0),
          .eps = eps};
      if (!(conformal.side_thickness >= 0 && conformal.top_thickness >= 0)) {
        fail(b.line, "negative SW_T or TW_T in " + b.name);
      }
      const bool has_associated = b.has("ASSOCIATED_CONDUCTOR");
      const std::string& associated = b.text("ASSOCIATED_CONDUCTOR");
      bool wrapped = false;
      for (Stack::Conductor& c : placed.conductors) {
        double outer_top = c.top;
        for (const Stack::ConformalDielectric& inner : c.conformals) {
          outer_top += inner.top_thickness;
        }
        const bool target = has_associated
                                ? c.name == associated
                                : outer_top > planar_top + kHeightTolerance;
        if (target) {
          c.conformals.push_back(conformal);
          wrapped = true;
        }
      }
      if (!wrapped) {
        fail(b.line,
             has_associated ? "ASSOCIATED_CONDUCTOR " + associated + " of "
                                  + b.name + " is not a conductor below it"
                            : "IS_CONFORMAL dielectric " + b.name
                                  + " has no conductor to wrap");
      }
      continue;
    }
    if (b.has("SW_T") || b.has("TW_T") || b.has("ASSOCIATED_CONDUCTOR")) {
      fail(
          b.line,
          "SW_T, TW_T and ASSOCIATED_CONDUCTOR need IS_CONFORMAL in " + b.name);
    }
    double base = planar_top;
    if (b.has("MEASURED_FROM")) {
      const std::string& from = b.text("MEASURED_FROM");
      if (from == "TOP_OF_CHIP") {
        fail(b.values.at("MEASURED_FROM").line,
             "MEASURED_FROM = TOP_OF_CHIP without IS_CONFORMAL is not "
             "supported ("
                 + b.name + ")");
      }
      const auto ref = tops_by_name.find(from);
      if (ref == tops_by_name.end()) {
        fail(b.values.at("MEASURED_FROM").line,
             "MEASURED_FROM " + from + " of " + b.name
                 + " is not a planar dielectric below it");
      }
      base = ref->second;
    }
    const double top = base + b.required("THICKNESS");
    if (!(top > planar_top)) {
      fail(b.line,
           "dielectric " + b.name
               + " has no thickness above the layer below it");
    }
    placed.dielectrics.push_back({b.name, planar_top, top, eps});
    tops_by_name[b.name] = top;
    planar_top = top;
  }
  return placed;
}

Stack Parser::parse()
{
  Stack stack;
  std::vector<Block> layers;  // in file order, top to bottom
  std::vector<Block> vias;
  std::set<std::string, std::less<>> globals_seen;
  // Keywords that only affect resistance or temperature.
  const std::set<std::string> kResistive
      = {"RPSQ", "RHO", "CRT1", "CRT2", "T0", "RESISTIVE_ONLY_ETCH"};
  while (pos_ < tokens_.size()) {
    const Token t = next();
    const bool global = t.text == "TECHNOLOGY" || t.text == "GLOBAL_TEMPERATURE"
                        || t.text == "BACKGROUND_ER";
    if (global && !globals_seen.insert(t.text).second) {
      fail(t.line, t.text + " is given twice");
    }
    if (t.text == "TECHNOLOGY") {
      expect("=");
      stack.setTechnology(next().text);
    } else if (t.text == "GLOBAL_TEMPERATURE") {
      expect("=");
      parseNumber(next(), t.text);
    } else if (t.text == "BACKGROUND_ER") {
      expect("=");
      const double eps = parseNumber(next(), t.text);
      if (!(eps > 0)) {
        fail(t.line, "BACKGROUND_ER must be positive");
      }
      stack.setBackgroundEps(eps);
    } else if (t.text == "DIELECTRIC") {
      Block b = parseBlock(t.text);
      checkKeys(b,
                {"ER",
                 "THICKNESS",
                 "MEASURED_FROM",
                 "SW_T",
                 "TW_T",
                 "ASSOCIATED_CONDUCTOR"},
                {"IS_CONFORMAL"});
      layers.push_back(std::move(b));
    } else if (t.text == "CONDUCTOR") {
      Block b = parseBlock(t.text);
      std::set<std::string> values = {"THICKNESS",
                                      "WMIN",
                                      "SMIN",
                                      "LAYER_TYPE",
                                      "ETCH",
                                      "CAPACITIVE_ONLY_ETCH"};
      values.insert(kResistive.begin(), kResistive.end());
      checkKeys(b, values, {"IS_DIFF"});
      for (const char* etch : {"ETCH", "CAPACITIVE_ONLY_ETCH"}) {
        if (b.number(etch, 0) != 0) {
          fail(b.values.at(etch).line,
               std::string("non-zero ") + etch + " in " + b.name
                   + " is not supported");
        }
      }
      layers.push_back(std::move(b));
    } else if (t.text == "VIA") {
      Block b = parseBlock(t.text);
      checkKeys(
          b, {"FROM", "TO", "AREA", "RPV", "RHO", "CRT1", "CRT2", "T0"}, {});
      if (!b.has("FROM") || !b.has("TO")) {
        fail(b.line, "VIA " + b.name + " needs FROM and TO");
      }
      for (const Block& other : vias) {
        if (other.name == b.name) {
          fail(b.line, "VIA " + b.name + " is defined twice");
        }
      }
      vias.push_back(std::move(b));
    } else {
      fail(t.line, "unsupported ITF statement " + t.text);
    }
  }

  if (stack.technology().empty()) {
    failFile("missing TECHNOLOGY");
  }
  Placed placed = place(layers);
  if (placed.dielectrics.empty()) {
    failFile("no planar dielectric layers");
  }

  // The ground plane is the top of the lowest dielectric, which is dropped.
  const double ground = placed.dielectrics.front().top;
  for (size_t i = 1; i < placed.dielectrics.size(); i++) {
    Stack::Dielectric d = placed.dielectrics[i];
    d.bottom -= ground;
    d.top -= ground;
    stack.addDielectric(d);
  }
  for (Stack::Conductor& c : placed.conductors) {
    c.bottom -= ground;
    c.top -= ground;
    if (c.bottom < -kHeightTolerance) {
      fail(conductor_lines_.at(c.name),
           "conductor " + c.name + " lies below the ground plane");
    }
    c.bottom = std::max(c.bottom, 0.0);
    stack.addConductor(c);
  }
  addVias(vias, stack);
  return stack;
}

// Resolves each via's layers. FROM and TO may come in either order, so the
// upper layer is decided by height; SUBSTRATE means the ground plane.
void Parser::addVias(const std::vector<Block>& vias, Stack& stack) const
{
  for (const Block& b : vias) {
    const std::string& from = b.text("FROM");
    const std::string& to = b.text("TO");
    const Stack::Layer a = stack.layer(from);
    const Stack::Layer c = stack.layer(to);
    if (from != kSubstrate && !stack.isConductor(a)) {
      fail(b.lineOf("FROM"), "VIA " + b.name + " names unknown layer " + from);
    }
    if (to != kSubstrate && !stack.isConductor(c)) {
      fail(b.lineOf("TO"), "VIA " + b.name + " names unknown layer " + to);
    }
    if (from == to) {
      fail(b.line, "VIA " + b.name + " connects " + from + " to itself");
    }
    if (from == kSubstrate || to == kSubstrate) {
      stack.addVia({b.name, from == kSubstrate ? c : a, {}});
      continue;
    }
    const Stack::Conductor& from_conductor = stack.conductor(a);
    const Stack::Conductor& to_conductor = stack.conductor(c);
    if (from_conductor.bottom >= to_conductor.top - kHeightTolerance) {
      stack.addVia({b.name, a, c});
    } else if (to_conductor.bottom >= from_conductor.top - kHeightTolerance) {
      stack.addVia({b.name, c, a});
    } else {
      fail(b.line,
           "VIA " + b.name + " connects " + from + " and " + to
               + ", which overlap in height");
    }
  }
}

}  // namespace

Stack readItf(std::istream& in)
{
  try {
    return Parser(tokenize(in)).parse();
  } catch (const std::invalid_argument& e) {
    // A layer the stack refuses that the checks above did not catch.
    throw std::runtime_error(std::string("ITF: ") + e.what());
  }
}

Stack readItfFile(const std::string& path)
{
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("ITF: cannot open " + path);
  }
  return readItf(in);
}

}  // namespace fsv
