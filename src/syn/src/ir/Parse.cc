// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PortDirection.hh"
#include "syn/ir/Bundle.h"
#include "syn/ir/Const.h"
#include "syn/ir/ControlNet.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"

namespace syn {

namespace {

class Parser
{
 public:
  Parser(const std::string& text, sta::Network* network = nullptr)
      : src_(text), network_(network)
  {
  }

  std::unique_ptr<Graph> run()
  {
    auto g = std::make_unique<Graph>();
    while (pos_ < src_.size()) {
      skipBlanks();
      if (pos_ >= src_.size()) {
        break;
      }
      if (src_[pos_] == '\n') {
        advance();
        continue;
      }
      parseLine(g.get());
    }
    return g;
  }

 private:
  // -- Lexing primitives --

  char peek() const
  {
    if (pos_ >= src_.size()) {
      return '\0';
    }
    return src_[pos_];
  }

  char advance()
  {
    char c = src_[pos_++];
    if (c == '\n') {
      line_++;
    }
    return c;
  }

  bool atEnd() const { return pos_ >= src_.size(); }

  void skipSpaces()
  {
    while (pos_ < src_.size() && (src_[pos_] == ' ' || src_[pos_] == '\t')) {
      pos_++;
    }
  }

  void skipComment()
  {
    if (pos_ < src_.size() && src_[pos_] == ';') {
      while (pos_ < src_.size() && src_[pos_] != '\n') {
        pos_++;
      }
    }
  }

  void skipBlanks()
  {
    skipSpaces();
    skipComment();
  }

  bool match(char c)
  {
    if (pos_ < src_.size() && src_[pos_] == c) {
      advance();
      return true;
    }
    return false;
  }

  void expect(char c)
  {
    if (!match(c)) {
      error(std::string("expected '") + c + "'");
    }
  }

  void expectNewline()
  {
    skipBlanks();
    if (atEnd()) {
      return;
    }
    if (src_[pos_] == '\n') {
      advance();
      return;
    }
    error("expected newline");
  }

  [[noreturn]] void error(const std::string& msg)
  {
    throw std::runtime_error("Parse error at line " + std::to_string(line_)
                             + ": " + msg);
  }

  // -- Token parsers --

  uint32_t parseUint()
  {
    if (pos_ >= src_.size() || !isdigit(src_[pos_])) {
      error("expected integer");
    }
    uint32_t val = 0;
    while (pos_ < src_.size() && isdigit(src_[pos_])) {
      val = val * 10 + (src_[pos_] - '0');
      pos_++;
    }
    return val;
  }

  uint32_t parseHashInt()
  {
    skipBlanks();
    expect('#');
    return parseUint();
  }

  std::string parseString()
  {
    skipBlanks();
    expect('"');
    std::string result;
    while (pos_ < src_.size() && src_[pos_] != '"') {
      if (src_[pos_] == '\\') {
        pos_++;
        if (pos_ + 1 >= src_.size()) {
          error("incomplete escape sequence");
        }
        char hi = src_[pos_++];
        char lo = src_[pos_++];
        auto hexVal = [&](char c) -> uint8_t {
          if (c >= '0' && c <= '9') {
            return c - '0';
          }
          if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
          }
          if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
          }
          error("invalid hex digit in escape");
          return 0;
        };
        result += (char) ((hexVal(hi) << 4) | hexVal(lo));
      } else {
        result += src_[pos_++];
      }
    }
    expect('"');
    return result;
  }

  std::string parseKeyword()
  {
    std::string kw;
    while (pos_ < src_.size()
           && (isalnum(src_[pos_]) || src_[pos_] == '_' || src_[pos_] == '/')) {
      kw += src_[pos_++];
    }
    if (kw.empty()) {
      error("expected keyword");
    }
    return kw;
  }

  // -- IR element parsers --

  // Parse a single net reference: 0, 1, X, %id, %id+offset
  Net parseNetRef()
  {
    skipBlanks();
    if (match('%')) {
      uint32_t id = parseUint();
      if (match('+')) {
        uint32_t off = parseUint();
        return Graph::makeNet(id + off);
      }
      return Graph::makeNet(id);
    }
    if (pos_ < src_.size()) {
      if (src_[pos_] == '0') {
        pos_++;
        return Net::zero();
      }
      if (src_[pos_] == '1') {
        pos_++;
        return Net::one();
      }
      if (src_[pos_] == 'X') {
        pos_++;
        return Net::undef();
      }
    }
    error("expected net reference");
  }

  // Parse a constant string (01X...) MSB-first into a Bundle (LSB at index 0).
  Bundle parseConstBits()
  {
    std::vector<Net> nets;
    while (pos_ < src_.size()
           && (src_[pos_] == '0' || src_[pos_] == '1' || src_[pos_] == 'X')) {
      // NOLINTNEXTLINE(bugprone-switch-missing-default-case)
      switch (src_[pos_++]) {
        case '0':
          nets.push_back(Net::zero());
          break;
        case '1':
          nets.push_back(Net::one());
          break;
        case 'X':
          nets.push_back(Net::undef());
          break;
      }
    }
    if (nets.empty()) {
      error("expected constant bits");
    }
    // Reverse: input is MSB-first, Bundle is LSB-first.
    std::ranges::reverse(nets);
    return Bundle::fromVec(std::move(nets));
  }

  // Parse a value part: %id, %id:w, %id+off, %id+off:w, or constant bits.
  Bundle parseValuePart()
  {
    skipBlanks();
    char c = peek();
    if (c == '%') {
      advance();
      uint32_t id = parseUint();
      uint32_t off = 0;
      if (match('+')) {
        off = parseUint();
      }
      if (match(':')) {
        uint32_t w = parseUint();
        return Bundle(Graph::makeNet(id + off), w);
      }
      return Bundle(Graph::makeNet(id + off));
    }
    if (c == '0' || c == '1' || c == 'X') {
      return parseConstBits();
    }
    error("expected value part");
  }

  // Parse a full value: a single part, a concatenation [ ... ], or [].
  Bundle parseValue()
  {
    skipBlanks();
    if (peek() == '[') {
      advance();
      skipBlanks();
      if (peek() == ']') {
        advance();
        return Bundle();
      }
      // Collect parts in MSB-first order.
      std::vector<Bundle> parts;
      while (peek() != ']') {
        parts.push_back(parseValuePart());
        skipBlanks();
      }
      expect(']');
      // Reverse to LSB-first and concatenate.
      Bundle result;
      for (int i = (int) parts.size() - 1; i >= 0; i--) {
        result = result.concat(parts[i]);
      }
      return result;
    }
    return parseValuePart();
  }

  // Parse "keyword=net" pattern for DFF control signals.
  bool tryParseKeywordNet(const char* keyword, Net& out)
  {
    skipBlanks();
    size_t saved = pos_;
    int saved_line = line_;
    std::string kw;
    while (pos_ < src_.size() && (isalnum(src_[pos_]) || src_[pos_] == '_')) {
      kw += src_[pos_++];
    }
    if (kw == keyword && match('=')) {
      out = parseNetRef();
      return true;
    }
    pos_ = saved;
    line_ = saved_line;
    return false;
  }

  bool tryParseKeywordControlNet(const char* keyword, ControlNet& out)
  {
    skipBlanks();
    size_t saved = pos_;
    int saved_line = line_;
    std::string kw;
    while (pos_ < src_.size() && (isalnum(src_[pos_]) || src_[pos_] == '_')) {
      kw += src_[pos_++];
    }
    if (kw == keyword && match('=')) {
      skipBlanks();
      bool negated = match('!');
      Net net = parseNetRef();
      out = negated ? ControlNet::neg(net) : ControlNet::pos(net);
      return true;
    }
    pos_ = saved;
    line_ = saved_line;
    return false;
  }

  // -- Line parser --

  void parseLine(Graph* g)
  {
    skipBlanks();
    if (atEnd() || peek() == '\n') {
      if (!atEnd()) {
        advance();
      }
      return;
    }

    // Parse %id:width = keyword ...
    expect('%');
    uint32_t id = parseUint();
    expect(':');
    uint32_t width = parseUint();
    skipBlanks();
    expect('=');
    skipBlanks();

    // Honor the declared %id: pad the table with kVoid slots so the next
    // instance's first slot lands at `id`. Dumps with gaps in the ID space
    // (e.g. after DCE) otherwise resolve net references to wrong slots.
    g->padTableTo(id);

    std::string keyword = parseKeyword();

    if (keyword == "buf") {
      Bundle a = parseValue();
      g->add<Buffer>(std::move(a));
    } else if (keyword == "not") {
      Bundle a = parseValue();
      g->add<Not>(std::move(a));
    } else if (keyword == "and") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<And>(std::move(a), std::move(b));
    } else if (keyword == "or") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<Or>(std::move(a), std::move(b));
    } else if (keyword == "andnot") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<Andnot>(std::move(a), std::move(b));
    } else if (keyword == "xor") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<Xor>(std::move(a), std::move(b));
    } else if (keyword == "mux") {
      Net sel = parseNetRef();
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<Mux>(sel, std::move(a), std::move(b));
    } else if (keyword == "adc") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      Net cin = parseNetRef();
      g->add<Adc>(std::move(a), std::move(b), cin);
    } else if (keyword == "eq") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<Eq>(std::move(a), std::move(b));
    } else if (keyword == "ult") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<ULt>(std::move(a), std::move(b));
    } else if (keyword == "slt") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<SLt>(std::move(a), std::move(b));
    } else if (keyword == "shl") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      uint32_t stride = parseHashInt();
      g->add<Shl>(std::move(a), std::move(b), stride);
    } else if (keyword == "ushr") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      uint32_t stride = parseHashInt();
      g->add<UShr>(std::move(a), std::move(b), stride);
    } else if (keyword == "sshr") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      uint32_t stride = parseHashInt();
      g->add<SShr>(std::move(a), std::move(b), stride);
    } else if (keyword == "xshr") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      uint32_t stride = parseHashInt();
      g->add<XShr>(std::move(a), std::move(b), stride);
    } else if (keyword == "mul") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<Mul>(std::move(a), std::move(b));
    } else if (keyword == "udiv") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<UDiv>(std::move(a), std::move(b));
    } else if (keyword == "umod") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<UMod>(std::move(a), std::move(b));
    } else if (keyword == "sdiv_trunc") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<SDivTrunc>(std::move(a), std::move(b));
    } else if (keyword == "sdiv_floor") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<SDivFloor>(std::move(a), std::move(b));
    } else if (keyword == "smod_trunc") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<SModTrunc>(std::move(a), std::move(b));
    } else if (keyword == "smod_floor") {
      Bundle a = parseValue();
      Bundle b = parseValue();
      g->add<SModFloor>(std::move(a), std::move(b));
    } else if (keyword == "dff") {
      Bundle data = parseValue();
      ControlNet clk;
      ControlNet clr;
      ControlNet rst;
      ControlNet en = ControlNet::one();
      tryParseKeywordControlNet("clk", clk);
      tryParseKeywordControlNet("clr", clr);
      tryParseKeywordControlNet("rst", rst);
      tryParseKeywordControlNet("en", en);
      g->add<Dff>(std::move(data),
                  clk,
                  clr,
                  rst,
                  en,
                  Const::undef(width),
                  Const::undef(width),
                  Const::undef(width));
    } else if (keyword == "loop_breaker") {
      Bundle a = parseValue();
      g->add<LoopBreaker>(std::move(a));
    } else if (keyword == "input") {
      std::string name = parseString();
      g->add<Input>(std::move(name), width);
    } else if (keyword == "dangling") {
      g->add<Dangling>(width);
    } else if (keyword == "output") {
      std::string name = parseString();
      Bundle value = parseValue();
      g->add<Output>(std::move(name), std::move(value));
    } else if (keyword == "name") {
      std::string name = parseString();
      skipBlanks();
      uint32_t from = parseUint();
      skipBlanks();
      uint32_t to = parseUint();
      skipBlanks();
      bool tentative = false;
      bool is_vector = false;
      while (pos_ < src_.size() && (src_[pos_] == 't' || src_[pos_] == 'v')) {
        size_t saved = pos_;
        std::string kw = parseKeyword();
        if (kw == "tentative") {
          tentative = true;
        } else if (kw == "vector") {
          is_vector = true;
        } else {
          pos_ = saved;
          break;
        }
        skipBlanks();
      }
      Bundle value = parseValue();
      g->add<Name>(
          std::move(name), std::move(value), from, to, tentative, is_vector);
    } else if (keyword == "target") {
      if (network_) {
        parseTargetCell(g, id, width);
      } else {
        parseOtherCell(g, id, width);
      }
    } else if (keyword == "other") {
      parseOtherCell(g, id, width);
    } else {
      error("unknown cell keyword: " + keyword);
    }

    const uint32_t expected_slots = std::max(width, 1u);
    if (g->tableSize() != id + expected_slots) {
      error("'" + keyword + "' declared output width " + std::to_string(width)
            + " but produced " + std::to_string(g->tableSize() - id) + " slot"
            + (g->tableSize() - id == 1 ? "" : "s"));
    }

    expectNewline();
  }

  void parseTargetCell(Graph* g, uint32_t base_id, uint32_t width)
  {
    skipBlanks();
    std::string cell_name = parseString();
    skipBlanks();
    expect('{');
    expectNewline();

    sta::LibertyCell* cell = network_->findLibertyCell(cell_name);
    if (!cell) {
      error("unknown liberty cell: " + cell_name);
    }

    // Collect input values indexed by port name.
    std::map<std::string, Bundle> input_values;

    while (true) {
      skipBlanks();
      if (peek() == '}') {
        break;
      }

      if (peek() == '%') {
        // Output line: %off:w = output "name" — skip.
        advance();
        parseUint();
        expect(':');
        parseUint();
        skipBlanks();
        expect('=');
        skipBlanks();
        std::string dir_kw = parseKeyword();
        if (dir_kw != "output") {
          error("expected 'output' in target cell block");
        }
        skipBlanks();
        parseString();  // port name, skip
      } else {
        // Input line: input "name" = value
        std::string kw = parseKeyword();
        if (kw != "input") {
          error("expected 'input' in target cell block");
        }
        skipBlanks();
        std::string name = parseString();
        skipBlanks();
        expect('=');
        Bundle value = parseValue();
        input_values[name] = std::move(value);
      }
      expectNewline();
    }
    expect('}');

    // Build the input Bundle in LibertyCellPortIterator order.
    std::vector<Net> input_nets;
    sta::LibertyCellPortIterator port_iter(cell);
    while (port_iter.hasNext()) {
      sta::LibertyPort* port = port_iter.next();
      if (port->isPwrGnd() || !port->direction()->isInput()) {
        continue;
      }
      auto it = input_values.find(port->name());
      if (it == input_values.end()) {
        error("missing input port '" + std::string(port->name()) + "' for cell "
              + cell_name);
      }
      for (uint32_t i = 0; i < it->second.width(); i++) {
        input_nets.push_back(it->second[i]);
      }
    }

    Bundle inputs = Bundle::fromVec(std::move(input_nets));
    g->add<Target>(cell, std::move(inputs));
    (void) base_id;
    (void) width;
  }

  void parseOtherCell(Graph* g, uint32_t base_id, uint32_t width)
  {
    skipBlanks();
    std::string cell_type = parseString();
    skipBlanks();
    expect('{');
    expectNewline();

    std::vector<Other::Port> ports;

    while (true) {
      skipBlanks();
      if (peek() == '}') {
        break;
      }

      if (peek() == '%') {
        // Output or IO port: %off:w = output "name" or %off:w = io "name" =
        // value
        advance();
        uint32_t port_off = parseUint();
        expect(':');
        uint32_t port_width = parseUint();
        skipBlanks();
        expect('=');
        skipBlanks();
        std::string dir_kw = parseKeyword();
        if (dir_kw == "output") {
          skipBlanks();
          std::string name = parseString();
          ports.push_back(
              {std::move(name), Other::Port::kOutput, port_width, Bundle()});
        } else if (dir_kw == "io") {
          skipBlanks();
          std::string name = parseString();
          skipBlanks();
          expect('=');
          Bundle value = parseValue();
          ports.push_back({std::move(name),
                           Other::Port::kInOut,
                           port_width,
                           std::move(value)});
        } else {
          error("expected 'output' or 'io' in other cell block");
        }
        (void) port_off;
      } else {
        // Input port: input "name" = value
        std::string kw = parseKeyword();
        if (kw != "input") {
          error("expected 'input' in other cell block");
        }
        skipBlanks();
        std::string name = parseString();
        skipBlanks();
        expect('=');
        Bundle value = parseValue();
        ports.push_back({std::move(name),
                         Other::Port::kInput,
                         value.width(),
                         std::move(value)});
      }
      expectNewline();
    }
    expect('}');
    (void) base_id;
    g->add<Other>(std::move(cell_type), std::move(ports));
  }

  std::string src_;
  size_t pos_{0};
  int line_{1};
  sta::Network* network_;
};

}  // namespace

std::unique_ptr<Graph> Graph::parse(std::istream& is, sta::Network* network)
{
  std::ostringstream buf;
  buf << is.rdbuf();
  Parser parser(buf.str(), network);
  return parser.run();
}

}  // namespace syn
