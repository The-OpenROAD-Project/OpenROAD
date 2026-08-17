// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Structural conformance: for every netlist in the corpus, prove that the
// emitted netlist has the same *structure* as the input netlist -- the same
// modules, the same port lists in the same order, the same declared nets, the
// same instances bound to the same masters, and no illegal or invented names.
//
// This complements TestHierConformance.cpp, which proves logical equivalence
// with a LEC. Equivalence is blind to structural fidelity: a netlist whose top
// port list has been reordered, whose modules have been cloned per instance,
// whose dangling nets have been erased, or which declares one name twice, is
// still provably equivalent to its input. Every one of those breaks a
// downstream flow (positional instantiation, SDC/UPF/DFT name matching,
// third-party readers) and no LEC will ever say so.
//
// Both suites compare against the *input* netlist, never flat_out vs hier_out:
// a reader bug upstream of the hierarchy split corrupts both outputs
// identically, and the input is the only ground truth available.
//
// This suite runs a superset of the LEC suite's corpus: it also loads
// hier_cases/structural/, the cases parked out of the LEC suite because a LEC
// cannot adjudicate them (the netlist is refused outright, or the defect is a
// naming or shape property that leaves the logic equivalent). See the corpus
// section below.
//
// METHOD, and a deliberate deviation from the original plan
// ---------------------------------------------------------
// The plan was to re-link the emitted netlist into a second odb and diff the
// two databases module by module. That is implemented here only as the
// round-trip check (the emitted netlist must read and link again). The
// structural comparison itself is done on the *text* of the two netlists, with
// a Verilog scanner in this file, because an odb-vs-odb diff is provably blind
// to three of the defects this suite exists to catch:
//
//   * Module cloning. dbLinkDesign -hier calls dbModule::makeUniqueDbModule,
//     so the clones (`sub`, `sub_u2`) already exist in the database built from
//     the INPUT. Both sides of an odb diff show them and the diff is green
//     while the emitted netlist has twice the modules of the input.
//   * Top port order. Verilog2db::makeDbNets creates dbBTerms while walking
//     nets, not while walking the port list, so the input database's bterm
//     order is not the input's declaration order. Neither side of an odb diff
//     knows what order the ports were declared in.
//   * Erased dead objects. VerilogReader::makeModuleInstBody creates no net
//     for a `wire n;` that nothing references, so a dropped dangling net is
//     absent from the input database too.
//
// A second reason: comparing two databases built by the same reader hides
// every reader-side normalization, which is exactly where the escaping and
// bus-shape bugs live. Comparing the two files instead means the check sees
// what a downstream tool sees. The cost is a hand-written scanner (below); it
// is deliberately a scanner and not a parser -- it understands module headers,
// declarations, instantiations and continuous assigns, which is all this
// corpus contains, and it reports what it could not make sense of instead of
// guessing.
//
// CANONICALIZATION (deliberate weakenings, so the suite does not cry wolf)
// -----------------------------------------------------------------------
//   * Escaped vs plain identifiers are the same name: `\a$b ` == `a$b`. The
//     writer re-escapes every identifier containing '$'; that is a lexical
//     form change, not a rename, and is not reported.
//   * Connection expression shape is not compared at all. The hier writer
//     bit-blasts every vector port connection (`.a(x)` becomes
//     `.a({x[3],x[2],x[1],x[0]})`) and explodes vector assigns into per-bit
//     assigns. Both are equivalence- and connectivity-preserving, so this
//     suite ignores them: assigns are compared by the *base names* they drive,
//     which per-bit explosion does not change.
//   * Added declarations are only reported when the added name appears nowhere
//     in the input netlist. Declaring a net the input left implicit is not a
//     defect; inventing `_NC3` or `\u1/n ` is.
//   * A name that is a '/'-join of input identifiers is accepted wherever the
//     writer legitimately has to synthesize hierarchical names: everywhere in a
//     flat netlist, and in the top module of a hier netlist, where the boundary
//     policy materializes a child-side name as <path>/<net> when the parent has
//     no alias for it. Inside a hier submodule such a name is still reported --
//     that is the module-local-net renaming defect.
//
// KNOWN LIMITATIONS
// -----------------
//   * Canonicalizing escapes makes the escaped scalar `\x[3] ` and the bus bit
//     `x[3]` the same string. They are different objects, so a defect that
//     turned one into the other inside a single module would be missed. It
//     cannot produce a false positive.
//   * The scanner reads declarations, instantiations and continuous assigns.
//     Connection expressions are skipped, so per-bit boundary connectivity is
//     not compared here -- that is what the LEC suite proves.
//   * `wire dead;` that nothing references is compared as text, so its loss is
//     caught; a dead *implicit* net cannot be, because nothing in either file
//     names it.

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "tst/db_fixture.h"
#include "tst/loaded_design.h"

namespace tst {
namespace {

enum class Path
{
  kHier,
  kFlat,
};

const char* toString(Path path)
{
  return path == Path::kHier ? "hier" : "flat";
}

// One structural aspect. Failures are keyed on (netlist, path, check) so that a
// systemic defect in one aspect -- top port order is currently reordered in
// nearly every case -- does not mask the other aspects of the same netlist.
enum class Check
{
  kRoundTrip,
  kModuleSet,
  kTopPorts,
  kSubmodulePorts,
  kDeclaredNets,
  kInstances,
  kNameIdentity,
  kCellCensus,
  kAssigns,
  kNamespace,
};

const char* toString(Check check)
{
  switch (check) {
    case Check::kRoundTrip:
      return "round_trip";
    case Check::kModuleSet:
      return "module_set";
    case Check::kTopPorts:
      return "top_ports";
    case Check::kSubmodulePorts:
      return "submodule_ports";
    case Check::kDeclaredNets:
      return "declared_nets";
    case Check::kInstances:
      return "instances";
    case Check::kNameIdentity:
      return "name_identity";
    case Check::kCellCensus:
      return "cell_census";
    case Check::kAssigns:
      return "assigns";
    case Check::kNamespace:
      return "namespace";
  }
  return "unknown";
}

////////////////////////////////////////////////////////////////////////////
// Verilog structural scanner
////////////////////////////////////////////////////////////////////////////

struct Token
{
  enum class Kind
  {
    kIdent,
    kNumber,
    kPunct,
    kEof,
  };
  Kind kind{Kind::kEof};
  // Identifiers are stored canonically: an escaped identifier keeps its
  // payload without the leading '\' or the terminating space, so `\a$b ` and
  // `a$b` compare equal. `escaped` is retained only so that `\wire ` is not
  // mistaken for the keyword.
  std::string text;
  bool escaped{false};
};

// ASCII-only character classification. The <cctype> functions are
// locale-dependent, and Verilog identifiers are ASCII by definition, so under
// a locale that classifies a byte differently a netlist would tokenize
// differently -- a difference that would show up as a corpus case mysteriously
// changing verdict on one machine.
bool isAsciiDigit(char c)
{
  return c >= '0' && c <= '9';
}

bool isAsciiAlpha(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isAsciiAlnum(char c)
{
  return isAsciiAlpha(c) || isAsciiDigit(c);
}

bool isIdentStart(char c)
{
  return isAsciiAlpha(c) || c == '_' || c == '$';
}

bool isIdentChar(char c)
{
  return isAsciiAlnum(c) || c == '_' || c == '$';
}

std::vector<Token> tokenize(const std::string& src)
{
  std::vector<Token> tokens;
  const std::size_t n = src.size();
  std::size_t i = 0;
  while (i < n) {
    const char c = src[i];
    if (std::isspace(static_cast<unsigned char>(c)) != 0) {
      ++i;
      continue;
    }
    if (c == '/' && i + 1 < n && src[i + 1] == '/') {
      while (i < n && src[i] != '\n') {
        ++i;
      }
      continue;
    }
    if (c == '/' && i + 1 < n && src[i + 1] == '*') {
      i += 2;
      while (i + 1 < n && !(src[i] == '*' && src[i + 1] == '/')) {
        ++i;
      }
      i = std::min(n, i + 2);
      continue;
    }
    if (c == '`') {
      // Compiler directive. No corpus case uses one; skipping the line is
      // better than mis-tokenizing it.
      while (i < n && src[i] != '\n') {
        ++i;
      }
      continue;
    }
    if (c == '(') {
      // An attribute instance `(* dont_touch = 1 *)`. Without this the
      // attribute's name lexes as an identifier at the head of a statement and
      // is counted as an instantiation of a cell called `dont_touch`.
      std::size_t at = i + 1;
      while (at < n && std::isspace(static_cast<unsigned char>(src[at])) != 0) {
        ++at;
      }
      if (at < n && src[at] == '*') {
        i = at + 1;
        while (i + 1 < n && !(src[i] == '*' && src[i + 1] == ')')) {
          ++i;
        }
        i = std::min(n, i + 2);
        continue;
      }
    }
    if (c == '\\') {
      // An escaped identifier runs from the backslash to the next whitespace.
      // This is the one lexical rule a naive regex-based checker gets wrong:
      // `\li/y[1] [0]` is a bit select of the escaped identifier `li/y[1]`,
      // not an identifier ending in `[0]`.
      const std::size_t begin = ++i;
      while (i < n && std::isspace(static_cast<unsigned char>(src[i])) == 0) {
        ++i;
      }
      tokens.push_back({Token::Kind::kIdent,
                        src.substr(begin, i - begin),
                        /*escaped=*/true});
      continue;
    }
    if (isIdentStart(c)) {
      const std::size_t begin = i;
      while (i < n && isIdentChar(src[i])) {
        ++i;
      }
      tokens.push_back({Token::Kind::kIdent, src.substr(begin, i - begin)});
      continue;
    }
    if (isAsciiDigit(c) || c == '\'') {
      const std::size_t begin = i;
      ++i;
      while (i < n
             && (isAsciiAlnum(src[i]) || src[i] == '_' || src[i] == '\'')) {
        ++i;
      }
      tokens.push_back({Token::Kind::kNumber, src.substr(begin, i - begin)});
      continue;
    }
    tokens.push_back({Token::Kind::kPunct, std::string(1, c)});
    ++i;
  }
  return tokens;
}

// A declared object: a port or a net. `range` is the declared bus range as
// written (normalized to "[msb:lsb]"), empty for a scalar. Ranges are compared
// textually rather than as bit sets so that a bus re-emitted as exploded
// scalars, or with renormalized bounds, is a difference.
struct Decl
{
  std::string name;
  std::string range;

  bool operator<(const Decl& other) const
  {
    return std::tie(name, range) < std::tie(other.name, other.range);
  }
  bool operator==(const Decl& other) const
  {
    return name == other.name && range == other.range;
  }
};

struct PortDecl
{
  std::string name;
  std::string dir;
  std::string range;

  bool operator==(const PortDecl& other) const
  {
    return name == other.name && dir == other.dir && range == other.range;
  }
};

std::string toString(const PortDecl& port)
{
  std::string out = port.dir.empty() ? "<no-dcl>" : port.dir;
  if (!port.range.empty()) {
    out += port.range;
  }
  out += " ";
  out += port.name;
  return out;
}

using InstBinding = std::pair<std::string, std::string>;  // (instance, master)

struct ModuleView
{
  std::string name;
  // Ports in header declaration order, with the direction and range picked up
  // from the matching declaration.
  std::vector<PortDecl> ports;
  std::set<std::string> port_names;
  // Every declared port and net, deduplicated.
  std::set<Decl> objects;
  std::multiset<InstBinding> insts;
  // Base names driven by a continuous assign. Base names, not full
  // expressions, so exploding `assign z[1:0] = a[1:0];` into two per-bit
  // assigns is not a difference.
  std::set<std::string> assign_lhs;
  // Names declared twice in the same namespace. Verilog puts nets, ports and
  // instances in one module namespace, so any of these is illegal.
  std::vector<std::string> duplicate_ports;
  std::vector<std::string> duplicate_nets;
  std::vector<std::string> inst_name_collisions;
  // Names in a declaration or instance-name position that are not legal
  // unescaped Verilog identifiers -- in practice, a name starting with a digit
  // written without its escape, which the writer emits and no reader can read
  // back.
  std::vector<std::string> illegal_names;
};

struct FileView
{
  std::map<std::string, ModuleView> modules;
  // Every identifier appearing anywhere in the file. Used to tell an invented
  // name from a name the input already knew.
  std::set<std::string> identifiers;
  std::vector<std::string> duplicate_modules;
  std::string error;
};

const std::set<std::string>& dirKeywords()
{
  static const std::set<std::string> kSet{"input", "output", "inout"};
  return kSet;
}

const std::set<std::string>& netKeywords()
{
  static const std::set<std::string> kSet{"wire",
                                          "tri",
                                          "tri0",
                                          "tri1",
                                          "triand",
                                          "trior",
                                          "trireg",
                                          "wand",
                                          "wor",
                                          "reg",
                                          "logic",
                                          "supply0",
                                          "supply1"};
  return kSet;
}

const std::set<std::string>& declModifiers()
{
  static const std::set<std::string> kSet{
      "signed", "unsigned", "scalared", "vectored", "small", "medium", "large"};
  return kSet;
}

class Scanner
{
 public:
  explicit Scanner(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

  FileView scan()
  {
    FileView view;
    for (const Token& token : tokens_) {
      if (token.kind == Token::Kind::kIdent) {
        view.identifiers.insert(token.text);
      }
    }
    while (!atEnd()) {
      if (isKeyword("module") || isKeyword("macromodule")) {
        advance();
        parseModule(view);
        continue;
      }
      advance();
    }
    return view;
  }

 private:
  bool atEnd() const { return index_ >= tokens_.size(); }

  const Token& peek(std::size_t ahead = 0) const
  {
    static const Token kEof;
    const std::size_t at = index_ + ahead;
    return at < tokens_.size() ? tokens_[at] : kEof;
  }

  void advance(std::size_t count = 1) { index_ += count; }

  bool isKeyword(const char* text, std::size_t ahead = 0) const
  {
    const Token& token = peek(ahead);
    return token.kind == Token::Kind::kIdent && !token.escaped
           && token.text == text;
  }

  bool isPunct(char c, std::size_t ahead = 0) const
  {
    const Token& token = peek(ahead);
    return token.kind == Token::Kind::kPunct && token.text[0] == c;
  }

  bool isIdentToken(std::size_t ahead = 0) const
  {
    return peek(ahead).kind == Token::Kind::kIdent;
  }

  // True for an identifier that is a declaration keyword rather than a name.
  bool isDeclKeyword() const
  {
    const Token& token = peek();
    if (token.kind != Token::Kind::kIdent || token.escaped) {
      return false;
    }
    return dirKeywords().count(token.text) != 0
           || netKeywords().count(token.text) != 0
           || declModifiers().count(token.text) != 0;
  }

  void skipBalanced(char open, char close)
  {
    if (!isPunct(open)) {
      return;
    }
    int depth = 0;
    while (!atEnd()) {
      if (isPunct(open)) {
        ++depth;
      } else if (isPunct(close)) {
        --depth;
        if (depth == 0) {
          advance();
          return;
        }
      }
      advance();
    }
  }

  void skipToSemi()
  {
    while (!atEnd()) {
      const bool semi = isPunct(';');
      advance();
      if (semi) {
        return;
      }
    }
  }

  // Consumes "[ msb : lsb ]" (or "[ index ]") and returns it normalized.
  // Bounds are kept as text so negative and non-literal bounds survive.
  std::string parseRange()
  {
    std::string out;
    int depth = 0;
    while (!atEnd()) {
      const Token& token = peek();
      if (token.kind == Token::Kind::kPunct && token.text == "[") {
        ++depth;
      } else if (token.kind == Token::Kind::kPunct && token.text == "]") {
        --depth;
      }
      out += token.text;
      advance();
      if (depth == 0) {
        break;
      }
    }
    return out;
  }

  void parseModule(FileView& view)
  {
    if (!isIdentToken()) {
      view.error = "expected a module name";
      return;
    }
    ModuleView module;
    module.name = peek().text;
    advance();
    if (isPunct('#')) {
      advance();
      skipBalanced('(', ')');
    }

    // Header port list. Non-ANSI headers name the ports only; ANSI headers
    // carry the direction and range too, so both are picked up here and the
    // body declarations below refine them.
    std::vector<std::string> header_order;
    std::map<std::string, PortDecl> header_decls;
    if (isPunct('(')) {
      advance();
      int depth = 1;
      std::string dir;
      std::string range;
      while (!atEnd() && depth > 0) {
        if (isPunct('[')) {
          range = parseRange();
          continue;
        }
        if (isPunct('(')) {
          ++depth;
          advance();
          continue;
        }
        if (isPunct(')')) {
          --depth;
          advance();
          continue;
        }
        if (isDeclKeyword()) {
          if (dirKeywords().count(peek().text) != 0) {
            dir = peek().text;
          }
          // A new direction or net keyword starts a new declaration, so the
          // previous one's range stops applying: in
          // `(input [7:0] bus, input scalar)` only `bus` is a vector, while in
          // `(input [7:0] a, b)` both are.
          if (dirKeywords().count(peek().text) != 0
              || netKeywords().count(peek().text) != 0) {
            range.clear();
          }
          advance();
          continue;
        }
        if (isIdentToken()) {
          header_order.push_back(peek().text);
          if (!dir.empty() || !range.empty()) {
            header_decls[peek().text] = PortDecl{peek().text, dir, range};
          }
          advance();
          continue;
        }
        advance();
      }
    }
    if (isPunct(';')) {
      advance();
    }

    std::map<std::string, PortDecl> port_decls;
    std::vector<std::string> port_decl_order;
    parseBody(module, port_decls, port_decl_order);

    // Assemble the ordered port list. The header order is authoritative; a
    // port declared but absent from the header (or the other way round) is
    // reported by appending it, so the difference shows up rather than being
    // silently dropped.
    for (const std::string& name : header_order) {
      auto found = port_decls.find(name);
      if (found != port_decls.end()) {
        module.ports.push_back(found->second);
      } else {
        auto in_header = header_decls.find(name);
        module.ports.push_back(in_header != header_decls.end()
                                   ? in_header->second
                                   : PortDecl{name, "", ""});
      }
      module.port_names.insert(name);
    }
    for (const std::string& name : port_decl_order) {
      if (module.port_names.count(name) == 0) {
        module.ports.push_back(port_decls[name]);
        module.port_names.insert(name);
      }
    }

    // Verilog puts nets, ports and instances in one module namespace, so an
    // instance sharing a name with a declared object is illegal however legal
    // each half looks on its own.
    for (const InstBinding& inst : module.insts) {
      if (module.port_names.count(inst.first) != 0
          || std::any_of(
              module.objects.begin(),
              module.objects.end(),
              [&](const Decl& decl) { return decl.name == inst.first; })) {
        module.inst_name_collisions.push_back(inst.first);
      }
    }
    std::sort(module.inst_name_collisions.begin(),
              module.inst_name_collisions.end());
    module.inst_name_collisions.erase(
        std::unique(module.inst_name_collisions.begin(),
                    module.inst_name_collisions.end()),
        module.inst_name_collisions.end());

    if (view.modules.count(module.name) != 0) {
      view.duplicate_modules.push_back(module.name);
    }
    view.modules[module.name] = std::move(module);
  }

  void parseBody(ModuleView& module,
                 std::map<std::string, PortDecl>& port_decls,
                 std::vector<std::string>& port_decl_order)
  {
    std::set<std::string> seen_ports;
    std::set<std::string> seen_nets;
    while (!atEnd()) {
      if (isKeyword("endmodule")) {
        advance();
        return;
      }
      if (peek().kind != Token::Kind::kIdent) {
        advance();
        continue;
      }
      if (isDeclKeyword()) {
        parseDecl(module, port_decls, port_decl_order, seen_ports, seen_nets);
        continue;
      }
      if (isKeyword("assign")) {
        parseAssign(module);
        continue;
      }
      if (isKeyword("defparam") || isKeyword("parameter")
          || isKeyword("localparam") || isKeyword("genvar")) {
        skipToSemi();
        continue;
      }
      parseInstance(module);
    }
  }

  void parseDecl(ModuleView& module,
                 std::map<std::string, PortDecl>& port_decls,
                 std::vector<std::string>& port_decl_order,
                 std::set<std::string>& seen_ports,
                 std::set<std::string>& seen_nets)
  {
    std::string dir;
    std::string last_keyword;
    bool is_net = false;
    while (isDeclKeyword()) {
      if (dirKeywords().count(peek().text) != 0) {
        dir = peek().text;
      } else if (netKeywords().count(peek().text) != 0) {
        is_net = true;
      }
      last_keyword = peek().text;
      advance();
    }
    std::string range;
    if (isPunct('[')) {
      range = parseRange();
    }
    int names = 0;
    while (!atEnd()) {
      if (isPunct(';')) {
        if (names == 0) {
          // `output output;` or `wire logic;`: a name that is a keyword was
          // emitted without the escape that made it a name in the input, so the
          // declaration has no name left in it.
          module.illegal_names.push_back(last_keyword);
        }
        advance();
        return;
      }
      if (isPunct(',')) {
        advance();
        continue;
      }
      if (peek().kind == Token::Kind::kNumber) {
        // A declared name that lexes as a number is a name that needed an
        // escape and did not get one: `wire 1n;` is not readable Verilog.
        module.illegal_names.push_back(peek().text);
      } else if (!isIdentToken()) {
        advance();
        continue;
      }
      const std::string name = peek().text;
      advance();
      // `wire x = expr;` declares and drives in one statement.
      if (isPunct('=')) {
        module.assign_lhs.insert(name);
        int depth = 0;
        while (!atEnd()) {
          if (isPunct('(') || isPunct('{') || isPunct('[')) {
            ++depth;
          } else if (isPunct(')') || isPunct('}') || isPunct(']')) {
            --depth;
          } else if (depth == 0 && (isPunct(',') || isPunct(';'))) {
            break;
          }
          advance();
        }
      }
      module.objects.insert(Decl{name, range});
      ++names;
      if (!dir.empty()) {
        if (!seen_ports.insert(name).second) {
          module.duplicate_ports.push_back(name);
        }
        if (port_decls.count(name) == 0) {
          port_decl_order.push_back(name);
        }
        port_decls[name] = PortDecl{name, dir, range};
      } else if (is_net) {
        // A port may legally be redeclared as a net; two net declarations of
        // the same name may not.
        if (!seen_nets.insert(name).second) {
          module.duplicate_nets.push_back(name);
        }
      }
    }
  }

  void parseAssign(ModuleView& module)
  {
    advance();  // assign
    bool collecting_lhs = true;
    int depth = 0;
    while (!atEnd()) {
      const Token& token = peek();
      if (token.kind == Token::Kind::kPunct) {
        const char c = token.text[0];
        if (c == '(' || c == '{' || c == '[') {
          ++depth;
        } else if (c == ')' || c == '}' || c == ']') {
          --depth;
        } else if (c == '=' && depth == 0) {
          collecting_lhs = false;
        } else if (c == ',' && depth == 0 && !collecting_lhs) {
          collecting_lhs = true;
        } else if (c == ';') {
          advance();
          return;
        }
        advance();
        continue;
      }
      if (token.kind == Token::Kind::kIdent && collecting_lhs && depth <= 1) {
        module.assign_lhs.insert(token.text);
      }
      advance();
    }
  }

  void parseInstance(ModuleView& module)
  {
    const std::string master = peek().text;
    advance();
    if (isPunct('#')) {
      advance();
      skipBalanced('(', ')');
    }
    while (!atEnd()) {
      if (isPunct(';')) {
        advance();
        return;
      }
      if (isPunct(',')) {
        advance();
        continue;
      }
      if (isIdentToken() || peek().kind == Token::Kind::kNumber) {
        if (peek().kind == Token::Kind::kNumber) {
          module.illegal_names.push_back(peek().text);
        }
        const std::string inst_name = peek().text;
        advance();
        if (isPunct('[')) {
          parseRange();
        }
        skipBalanced('(', ')');
        module.insts.emplace(inst_name, master);
        continue;
      }
      if (isPunct('(')) {
        // Gate primitive with no instance name: `buf (o, i);`
        skipBalanced('(', ')');
        module.insts.emplace("", master);
        continue;
      }
      advance();
    }
  }

  std::vector<Token> tokens_;
  std::size_t index_{0};
};

FileView scanVerilogFile(const std::string& path)
{
  FileView view;
  std::ifstream in(path);
  if (!in) {
    view.error = "could not open " + path;
    return view;
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  Scanner scanner(tokenize(buffer.str()));
  FileView scanned = scanner.scan();
  if (scanned.modules.empty() && scanned.error.empty()) {
    scanned.error = "no module definition found in " + path;
  }
  return scanned;
}

// Elaborated leaf-cell census: every leaf instance reachable from the top,
// counted with multiplicity. A module instantiated twice contributes its gates
// twice, which is what flattening produces, so this one census is comparable
// between an input netlist, its hierarchical output and its flat output.
void censusOf(const FileView& view,
              const std::string& module_name,
              std::map<std::string, int>& census,
              std::set<std::string>& on_stack,
              int depth)
{
  if (depth > 64 || !on_stack.insert(module_name).second) {
    return;  // malformed input: recursive instantiation
  }
  const auto found = view.modules.find(module_name);
  if (found != view.modules.end()) {
    for (const InstBinding& inst : found->second.insts) {
      if (view.modules.count(inst.second) != 0) {
        censusOf(view, inst.second, census, on_stack, depth + 1);
      } else {
        ++census[inst.second];
      }
    }
  }
  on_stack.erase(module_name);
}

std::map<std::string, int> cellCensus(const FileView& view,
                                      const std::string& top)
{
  std::map<std::string, int> census;
  std::set<std::string> on_stack;
  censusOf(view, top, census, on_stack, 0);
  return census;
}

////////////////////////////////////////////////////////////////////////////
// Corpus
//
// Three folders, all scanned:
//   * hier_cases/*.v and hier_cases/inherited/*.v -- the corpus
//     TestHierConformance.cpp also runs. inherited/ holds symlinks to fixtures
//     owned by other suites, so membership is a folder listing rather than a
//     manifest, and a case's top module comes from HIER_TOP_OVERRIDES in
//     src/dbSta/test/BUILD when it is not the default "top".
//   * hier_cases/structural/*.v -- cases the LEC suite deliberately does not
//     load, because a LEC cannot adjudicate them: the netlist is rejected
//     outright, or the defect is a naming/shape property that leaves the
//     logic equivalent. This suite is the tool that can, so it loads them.
//     Their names carry the `structural/` prefix, which keeps the manifest key
//     and the gtest name unambiguous and makes the origin visible in a failure
//     message.
//
// hier_cases/crash/ is deliberately NOT scanned by either suite: those five
// netlists kill the process, so loading them would take a whole shard down
// instead of reporting a failure.
////////////////////////////////////////////////////////////////////////////

struct CorpusEntry
{
  std::string path;
  // Corpus-relative name: "case.v", or "structural/case.v" for a case from the
  // structural-only subdirectory. This is the XFAIL manifest key.
  std::string name;
  std::string top;
  Technology tech{Technology::kNangate45};
  std::string load_error;
};

void PrintTo(const CorpusEntry& entry, std::ostream* os)
{
  if (!entry.load_error.empty()) {
    *os << "<corpus load error: " << entry.load_error << ">";
    return;
  }
  *os << entry.name << " (top " << entry.top << ")";
}

std::string entryName(const ::testing::TestParamInfo<CorpusEntry>& info)
{
  std::string name = info.param.name;
  for (char& c : name) {
    if (!isAsciiAlnum(c)) {
      c = '_';
    }
  }
  return name;
}

std::filesystem::path workDir()
{
  const char* tmp = std::getenv("TEST_TMPDIR");
  return tmp != nullptr ? tmp : ".";
}

// A file name derived from a corpus name. The corpus name of a subdirectory
// case contains a '/', and the emitted netlist must not be written into a
// directory that does not exist.
std::string fileStem(const std::string& name)
{
  std::string stem = name;
  for (char& c : stem) {
    if (!isAsciiAlnum(c) && c != '.' && c != '-' && c != '_') {
      c = '_';
    }
  }
  return stem;
}

const char* kCasesDir = "_main/src/dbSta/test/cpp/hier_cases/";

// The corpus subdirectory holding the cases only this suite runs.
const char* kStructuralSubdir = "structural";

std::vector<std::string> splitFields(const std::string& line)
{
  std::vector<std::string> fields;
  std::istringstream in(line);
  std::string field;
  while (std::getline(in, field, ':')) {
    const std::string::size_type begin = field.find_first_not_of(" \t\r");
    const std::string::size_type end = field.find_last_not_of(" \t\r");
    fields.push_back(begin == std::string::npos
                         ? std::string()
                         : field.substr(begin, end - begin + 1));
  }
  return fields;
}

bool isComment(const std::string& line)
{
  const std::string::size_type first = line.find_first_not_of(" \t\r");
  return first == std::string::npos || line[first] == '#';
}

std::vector<CorpusEntry> corpusLoadError(const std::string& message)
{
  CorpusEntry entry;
  entry.name = "corpus_load_error";
  entry.load_error = message;
  return {entry};
}

// The netlists whose top module is not "top", as `<file>=<top>,...`. The build
// rule supplies it (HIER_TOP_OVERRIDES in src/dbSta/test/BUILD), keeping the
// corpus metadata with the build rules instead of inside each netlist.
std::map<std::string, std::string> topOverrides()
{
  std::map<std::string, std::string> overrides;
  const char* env = std::getenv("HIER_TOP_OVERRIDES");
  if (env == nullptr) {
    return overrides;
  }
  std::istringstream entries(env);
  std::string entry;
  while (std::getline(entries, entry, ',')) {
    const std::string::size_type eq = entry.find('=');
    if (eq != std::string::npos) {
      overrides.emplace(entry.substr(0, eq), entry.substr(eq + 1));
    }
  }
  return overrides;
}

// Appends every .v file directly in `dir`, naming each `name_prefix` + its file
// name. Top defaults to "top" unless the build rule names an exception;
// Nangate45 is the only technology the corpus uses. Returns the number
// appended, so a directory that silently came back empty -- a broken data
// dependency -- can be reported rather than losing coverage. Not recursive:
// hier_cases/crash/ must never be loaded.
std::size_t scanCaseDirectory(const std::filesystem::path& dir,
                              const std::string& name_prefix,
                              const std::map<std::string, std::string>& tops,
                              std::vector<CorpusEntry>& corpus)
{
  if (!std::filesystem::is_directory(dir)) {
    return 0;
  }
  std::size_t found = 0;
  for (const auto& item : std::filesystem::directory_iterator(dir)) {
    if (item.path().extension() != ".v") {
      continue;
    }
    CorpusEntry entry;
    entry.path = item.path().string();
    entry.name = name_prefix + item.path().filename().string();
    entry.top = "top";
    entry.tech = Technology::kNangate45;
    if (const auto it = tops.find(item.path().filename().string());
        it != tops.end()) {
      entry.top = it->second;
    }
    corpus.push_back(entry);
    ++found;
  }
  return found;
}

// The corpus named explicitly, as corpus-relative names ("case.v",
// "inherited/case.v", "structural/case.v"). A per-case test target names its
// one case here and carries only that netlist in its runfiles, so bazel caches
// and invalidates the corpus one case at a time; the whole-corpus target sets
// nothing and gets the directory scan below.
std::vector<CorpusEntry> corpusFromNames(
    const std::string& names,
    const std::map<std::string, std::string>& tops)
{
  std::vector<CorpusEntry> corpus;
  std::istringstream fields(names);
  std::string name;
  while (std::getline(fields, name, ',')) {
    if (name.empty()) {
      continue;
    }
    CorpusEntry entry;
    entry.name = name;
    entry.path = getRunfilePath(std::string(kCasesDir) + name);
    entry.top = "top";
    entry.tech = Technology::kNangate45;
    // Keyed on the file name, as the directory scan is: an override names a
    // netlist, not the subdirectory it happens to sit in.
    const std::string file_name
        = std::filesystem::path(name).filename().string();
    if (const auto it = tops.find(file_name); it != tops.end()) {
      entry.top = it->second;
    }
    corpus.push_back(entry);
  }
  return corpus;
}

std::vector<CorpusEntry> loadCorpus()
{
  std::vector<CorpusEntry> corpus;
  try {
    if (const char* names = std::getenv("HIER_CASES"); names != nullptr) {
      corpus = corpusFromNames(names, topOverrides());
      if (corpus.empty()) {
        return corpusLoadError("HIER_CASES is set but names no cases");
      }
      return corpus;
    }
    // Located through the XFAIL manifest, the one file in hier_cases/ this
    // suite is guaranteed to have a runfile for. It is generated rather than
    // checked in, but runfiles merge a rule's outputs with the package's source
    // files, so its parent is the directory holding the netlists.
    const std::filesystem::path cases_dir
        = std::filesystem::path(
              getRunfilePath(std::string(kCasesDir)
                             + "structural_expected_fail.txt"))
              .parent_path();
    const std::map<std::string, std::string> tops = topOverrides();

    scanCaseDirectory(cases_dir, "", tops, corpus);

    // Fixtures owned by other suites, symlinked in so the corpus is a set of
    // folders rather than a manifest.
    // Missing or empty means the data dependency broke -- which is exactly
    // how these cases went unrun once already, since nothing else in the
    // suite notices a directory that simply is not there.
    if (scanCaseDirectory(cases_dir / "inherited", "inherited/", tops, corpus)
        == 0) {
      return corpusLoadError(
          "no .v cases found in the inherited corpus subdirectory "
          + (cases_dir / "inherited").string()
          + "; check the cpp/hier_cases/inherited/*.v data dependency");
    }

    // The structural-only subdirectory. Missing or empty means the data
    // dependency broke, which would silently drop 80 cases, so it is an error
    // rather than a quiet zero.
    const std::filesystem::path structural_dir = cases_dir / kStructuralSubdir;
    if (scanCaseDirectory(
            structural_dir, std::string(kStructuralSubdir) + "/", tops, corpus)
        == 0) {
      return corpusLoadError(
          "no .v cases found in the structural corpus subdirectory "
          + structural_dir.string()
          + "; check the cpp/hier_cases/structural/*.v data dependency");
    }
  } catch (const std::exception& e) {
    return corpusLoadError(std::string("loading corpus: ") + e.what());
  }

  if (corpus.empty()) {
    return corpusLoadError("corpus is empty");
  }
  std::sort(corpus.begin(),
            corpus.end(),
            [](const CorpusEntry& a, const CorpusEntry& b) {
              return a.name < b.name;
            });
  return corpus;
}

const std::vector<CorpusEntry>& corpus()
{
  static const std::vector<CorpusEntry> loaded = loadCorpus();
  return loaded;
}

////////////////////////////////////////////////////////////////////////////
// XFAIL manifest
////////////////////////////////////////////////////////////////////////////

struct ExpectedFailure
{
  std::string netlist;
  Path path;
  std::string check;
  // The OpenROAD issue, when one has been filed. Empty otherwise -- see
  // hier_expected_fail.bzl for why that is allowed to be empty rather than
  // carrying a placeholder.
  std::optional<std::string> issue;
  std::string symptom;
  // The entry as authored, which is what a message must name for the reader to
  // find it: one entry can name a run of netlists with a '*', and the row above
  // holds the netlist it expanded to, not the text in the .bzl file.
  std::string as_authored;
};

// "issue 1234, " when one is recorded, "" otherwise, so a message about an
// unfiled defect does not read as a formatting bug.
std::string issuePrefix(const std::optional<std::string>& issue)
{
  return !issue.has_value() ? std::string() : "issue " + *issue + ", ";
}

// Parses the XFAIL manifest, which the build rule generates from
// STRUCTURAL_EXPECTED_FAIL in src/dbSta/test/hier_expected_fail.bzl -- that is
// where entries are edited. Grouping the netlists under one entry per failure
// mode keeps 1100 rows readable as the ~50 defects they actually are, and
// Starlark rejects an unknown check or path when the package loads instead of
// leaving a typo to be silently dropped here.
const std::vector<ExpectedFailure>& expectedFailures()
{
  static const std::vector<ExpectedFailure> all = []() {
    std::vector<ExpectedFailure> parsed;
    // A per-case target is handed its own rows in HIER_EXPECTED_FAIL, so it
    // depends on the netlist it runs and not on every other case's XFAIL
    // entries. The corpus-wide target leaves it unset and reads the manifest,
    // which is the whole list -- including any row naming no case at all.
    const char* inline_rows = std::getenv("HIER_EXPECTED_FAIL");
    std::ifstream file;
    std::istringstream rows;
    if (inline_rows != nullptr) {
      rows.str(inline_rows);
    } else {
      file.open(getRunfilePath(std::string(kCasesDir)
                               + "structural_expected_fail.txt"));
    }
    std::istream& in
        = inline_rows != nullptr ? static_cast<std::istream&>(rows) : file;
    std::string line;
    while (std::getline(in, line)) {
      if (isComment(line)) {
        continue;
      }
      const std::vector<std::string> f = splitFields(line);
      if (f.size() < 4) {
        continue;
      }
      parsed.push_back(
          ExpectedFailure{f[0],
                          f[1] == "hier" ? Path::kHier : Path::kFlat,
                          f[2],
                          f[3],
                          f.size() > 4 ? f[4] : "",
                          f.size() > 5 ? f[5] : f[0]});
    }
    return parsed;
  }();
  return all;
}

// Rows name one netlist exactly: hier_expected_fail.bzl expands a '*' entry
// against the corpus when the package loads, so there is no pattern left here.
const ExpectedFailure* expectedFailure(const std::string& netlist,
                                       Path path,
                                       Check check)
{
  for (const ExpectedFailure& failure : expectedFailures()) {
    if (failure.path == path && failure.check == toString(check)
        && failure.netlist == netlist) {
      return &failure;
    }
  }
  return nullptr;
}

// Inverts the expectation for a known failure, so an accidental fix turns the
// suite red with an actionable message rather than silently losing coverage.
void expectOrXfail(const CorpusEntry& entry,
                   Path path,
                   Check check,
                   const std::vector<std::string>& problems)
{
  const ExpectedFailure* failure = expectedFailure(entry.name, path, check);
  if (failure != nullptr) {
    EXPECT_FALSE(problems.empty())
        << entry.name << " [" << toString(path) << "/" << toString(check)
        << "] is a known failure (" << issuePrefix(failure->issue)
        << failure->symptom << "). It now PASSES -- delete '"
        << failure->as_authored
        << "' from STRUCTURAL_EXPECTED_FAIL in "
           "src/dbSta/test/hier_expected_fail.bzl.";
    return;
  }
  std::string detail;
  for (const std::string& problem : problems) {
    detail += "\n  " + problem;
  }
  EXPECT_TRUE(problems.empty()) << entry.name << " [" << toString(path) << "/"
                                << toString(check) << "]" << detail;
}

////////////////////////////////////////////////////////////////////////////
// The checks
////////////////////////////////////////////////////////////////////////////

std::string join(const std::vector<std::string>& items)
{
  std::string out;
  for (const std::string& item : items) {
    if (!out.empty()) {
      out += ", ";
    }
    out += item;
  }
  return out;
}

std::string formatPorts(const std::vector<PortDecl>& ports)
{
  std::vector<std::string> items;
  items.reserve(ports.size());
  for (const PortDecl& port : ports) {
    items.push_back(toString(port));
  }
  return "(" + join(items) + ")";
}

std::string formatDecls(const std::vector<Decl>& decls)
{
  std::vector<std::string> items;
  items.reserve(decls.size());
  for (const Decl& decl : decls) {
    items.push_back(decl.name + decl.range);
  }
  return join(items);
}

std::vector<std::string> checkModuleSet(const FileView& in,
                                        const FileView& out,
                                        Path path,
                                        const std::string& top)
{
  std::vector<std::string> problems;
  for (const std::string& name : out.duplicate_modules) {
    problems.push_back("module '" + name
                       + "' is defined more than once in the output");
  }
  if (path == Path::kFlat) {
    // Flattening is allowed to collapse the hierarchy, but it must produce
    // exactly the top module and nothing else.
    for (const auto& [name, module] : out.modules) {
      if (name != top) {
        problems.push_back("flat output defines an extra module '" + name
                           + "'");
      }
    }
    if (out.modules.count(top) == 0) {
      problems.push_back("flat output does not define the top module '" + top
                         + "'");
    }
    return problems;
  }
  std::vector<std::string> dropped;
  std::vector<std::string> added;
  for (const auto& [name, module] : in.modules) {
    if (out.modules.count(name) == 0) {
      dropped.push_back(name);
    }
  }
  for (const auto& [name, module] : out.modules) {
    if (in.modules.count(name) == 0) {
      added.push_back(name);
    }
  }
  if (!dropped.empty()) {
    problems.push_back("modules defined in the input but not in the output: "
                       + join(dropped));
  }
  if (!added.empty()) {
    problems.push_back("modules defined in the output but not in the input: "
                       + join(added));
  }
  return problems;
}

std::vector<std::string> checkPortList(const FileView& in,
                                       const FileView& out,
                                       const std::string& module_name)
{
  std::vector<std::string> problems;
  const auto in_module = in.modules.find(module_name);
  const auto out_module = out.modules.find(module_name);
  if (in_module == in.modules.end() || out_module == out.modules.end()) {
    return problems;  // module set differences are checkModuleSet's business
  }
  if (in_module->second.ports != out_module->second.ports) {
    problems.push_back("module '" + module_name
                       + "' port list changed:\n    in  "
                       + formatPorts(in_module->second.ports) + "\n    out "
                       + formatPorts(out_module->second.ports));
  }
  return problems;
}

// True if `name` is a '/'-join of identifiers the input netlist uses, i.e. a
// hierarchical path the writer legitimately synthesized while flattening. The
// split is searched for rather than assumed, because an input identifier may
// itself contain '/' (an escaped name like `\net/with/slash `), so
// `u/inst/net/with/slash` has to be recognized as `u/inst` + `net/with/slash`.
bool isSynthesizedPath(const std::string& name,
                       const std::set<std::string>& identifiers)
{
  if (name.find('/') == std::string::npos) {
    return false;
  }
  std::vector<bool> reachable(name.size() + 1, false);
  reachable[0] = true;
  for (std::size_t begin = 0; begin < name.size(); ++begin) {
    if (!reachable[begin]) {
      continue;
    }
    for (std::size_t end = begin + 1; end <= name.size(); ++end) {
      const bool at_separator = end == name.size() || name[end] == '/';
      if (at_separator
          && identifiers.count(name.substr(begin, end - begin)) != 0) {
        reachable[std::min(name.size(), end + 1)] = true;
      }
    }
  }
  return reachable[name.size()];
}

std::vector<std::string> checkDeclaredNets(
    const FileView& in,
    const FileView& out,
    const std::vector<std::string>& module_names,
    // Modules whose contents the writer is allowed to name after instance
    // paths: every module in the flat output (flattening has to synthesize
    // names), and the top module of a hier output (the documented boundary-net
    // policy materializes a child-side name as <path>/<net> when the parent has
    // no alias for it). Inside a hier submodule a path name is the
    // module-local-net renaming defect and stays reportable.
    const std::set<std::string>& paths_allowed_in)
{
  std::vector<std::string> problems;
  for (const std::string& name : module_names) {
    const ModuleView& in_module = in.modules.at(name);
    const ModuleView& out_module = out.modules.at(name);
    const bool paths_allowed = paths_allowed_in.count(name) != 0;
    std::vector<Decl> dropped;
    std::vector<Decl> invented;
    for (const Decl& decl : in_module.objects) {
      if (out_module.objects.count(decl) == 0) {
        dropped.push_back(decl);
      }
    }
    for (const Decl& decl : out_module.objects) {
      if (in_module.objects.count(decl) == 0
          // Declaring a net the input left implicit is not a defect; only a
          // name the input never mentions at all is invented.
          && in.identifiers.count(decl.name) == 0
          && !(paths_allowed && isSynthesizedPath(decl.name, in.identifiers))) {
        invented.push_back(decl);
      }
    }
    if (!dropped.empty()) {
      problems.push_back("module '" + name
                         + "': declared in the input, missing from the output: "
                         + formatDecls(dropped));
    }
    if (!invented.empty()) {
      problems.push_back("module '" + name
                         + "': output declares names the input never uses: "
                         + formatDecls(invented));
    }
  }
  return problems;
}

std::vector<std::string> checkInstances(
    const FileView& in,
    const FileView& out,
    const std::vector<std::string>& module_names)
{
  std::vector<std::string> problems;
  for (const std::string& name : module_names) {
    const std::multiset<InstBinding>& in_insts = in.modules.at(name).insts;
    const std::multiset<InstBinding>& out_insts = out.modules.at(name).insts;
    std::vector<InstBinding> dropped;
    std::vector<InstBinding> added;
    std::set_difference(in_insts.begin(),
                        in_insts.end(),
                        out_insts.begin(),
                        out_insts.end(),
                        std::back_inserter(dropped));
    std::set_difference(out_insts.begin(),
                        out_insts.end(),
                        in_insts.begin(),
                        in_insts.end(),
                        std::back_inserter(added));
    auto format = [](const std::vector<InstBinding>& insts) {
      std::vector<std::string> items;
      items.reserve(insts.size());
      for (const InstBinding& inst : insts) {
        items.push_back(inst.second + " " + inst.first);
      }
      return join(items);
    };
    if (!dropped.empty()) {
      problems.push_back("module '" + name + "': instances in the input but not"
                         " in the output: " + format(dropped));
    }
    if (!added.empty()) {
      problems.push_back("module '" + name + "': instances in the output but"
                         " not in the input: " + format(added));
    }
  }
  return problems;
}

// Resolves every module in the emitted netlist to the input module it is a copy
// of, using STRUCTURE alone and never the name: an emitted module is reached
// through some instance, and that same instance in the input named its master.
//
// The answer is a set on purpose. If one emitted module name resolves to two
// different input modules, that single name has been made to denote two
// modules, and that is the finding rather than an inconvenience.
class SourceResolver
{
 public:
  SourceResolver(const FileView& in, const FileView& out, std::string top)
      : in_(in), out_(out), top_(std::move(top))
  {
    for (const auto& [parent, module] : out_.modules) {
      for (const InstBinding& inst : module.insts) {
        if (out_.modules.count(inst.second) != 0) {
          sites_[inst.second].emplace_back(parent, inst.first);
        }
      }
    }
  }

  const std::set<std::string>& sourcesOf(const std::string& module_name)
  {
    const auto memo = memo_.find(module_name);
    if (memo != memo_.end()) {
      return memo->second;
    }
    // Inserted before recursing, so a recursive instantiation terminates with
    // an empty answer instead of running away. std::map nodes are stable, so
    // this reference survives the nested inserts below.
    std::set<std::string>& result = memo_[module_name];
    if (module_name == top_ && in_.modules.count(top_) != 0) {
      result.insert(top_);  // the top module is the one thing never cloned
    }
    const auto sites = sites_.find(module_name);
    if (sites == sites_.end()) {
      return result;
    }
    for (const auto& [parent, inst_name] : sites->second) {
      const std::set<std::string> parent_sources = sourcesOf(parent);
      for (const std::string& parent_source : parent_sources) {
        const auto in_parent = in_.modules.find(parent_source);
        if (in_parent == in_.modules.end()) {
          continue;
        }
        for (const InstBinding& in_inst : in_parent->second.insts) {
          if (in_inst.first == inst_name
              && in_.modules.count(in_inst.second) != 0) {
            result.insert(in_inst.second);
          }
        }
      }
    }
    return result;
  }

 private:
  const FileView& in_;
  const FileView& out_;
  std::string top_;
  // Emitted module name -> the (parent module, instance name) sites that
  // instantiate it.
  std::map<std::string, std::vector<std::pair<std::string, std::string>>>
      sites_;
  std::map<std::string, std::set<std::string>> memo_;
};

// True if `emitted` is a name dbModule::makeUniqueDbModule could have produced
// for `module_name` cloned at instance `inst_name`: <module>_<inst>, optionally
// with the numeric suffix it appends when even that name is taken.
bool decodesToClone(const std::string& emitted,
                    const std::string& module_name,
                    const std::string& inst_name)
{
  const std::string base = module_name + "_" + inst_name;
  if (emitted == base) {
    return true;
  }
  if (emitted.size() <= base.size() + 1
      || emitted.compare(0, base.size(), base) != 0
      || emitted[base.size()] != '_') {
    return false;
  }
  const std::string suffix = emitted.substr(base.size() + 1);
  return std::all_of(
      suffix.begin(), suffix.end(), [](char c) { return isAsciiDigit(c); });
}

// Name identity: every module name in the emitted netlist must map back to the
// module it came from, and to exactly one.
//
// This is the assertion the generic module_set and instances rows cannot make.
// Those two fire on *any* uniquification, so they fire identically on a benign
// clone (`sub` instantiated twice becomes `sub`, `sub_i2`) and on a netlist
// where uniquification has actually destroyed the mapping from name to module.
// A manifest row keyed on them therefore proves nothing about the two defects
// below, which is why this check exists:
//
//   * Clone-name ambiguity. `<module>_<inst>` is not injective: module `a_b`
//     instantiated as `c` and module `a` instantiated as `b_c` both request
//     `a_b_c`. Whichever wins, the emitted name no longer says which module was
//     cloned, and the loser is renamed by a numeric suffix that says even less.
//   * A clone name implying the wrong source. When the requested name is
//     already a real module's, the clone takes it and the real module is pushed
//     onto a suffixed name, so a module named `sub_i2` is a copy of `sub` while
//     the input's own `sub_i2` -- a different module -- is emitted as
//     `sub_i2_u3`. Every reference is rewritten consistently, so no LEC and no
//     count-based check sees it, but every name-driven downstream flow
//     (SDC/UPF paths, DFT, `-hier` re-reads, library caches) now resolves the
//     name to the wrong module.
//
// Hier only: the flat writer emits one module, and its internals are
// legitimately renamed to instance paths.
std::vector<std::string> checkNameIdentity(const FileView& in,
                                           const FileView& out,
                                           const std::string& top)
{
  std::vector<std::string> problems;

  // The (module, instance name) pairs the input actually contains. These are
  // the only pairs uniquification can ever be called with, so they are also the
  // only decodings of a clone name that a reader of the two netlists could
  // reasonably make.
  std::set<std::pair<std::string, std::string>> clone_pairs;
  for (const auto& [parent, module] : in.modules) {
    for (const InstBinding& inst : module.insts) {
      if (in.modules.count(inst.second) != 0) {
        clone_pairs.emplace(inst.second, inst.first);
      }
    }
  }

  SourceResolver resolver(in, out, top);
  for (const auto& [name, module] : out.modules) {
    const std::set<std::string> sources = resolver.sourcesOf(name);
    if (sources.size() > 1) {
      problems.push_back(
          "emitted module name '" + name
          + "' denotes more than one input module: "
          + join(std::vector<std::string>(sources.begin(), sources.end())));
      continue;
    }
    if (in.modules.count(name) != 0) {
      // A name the input already defines must still be that module.
      if (sources.size() == 1 && *sources.begin() != name) {
        problems.push_back("emitted module '" + name
                           + "' is a copy of input module '" + *sources.begin()
                           + "', not of the input module of that name");
      }
      continue;
    }
    // A synthesized name. It must decode to exactly one source module, and to
    // the one it is actually a copy of.
    std::vector<std::string> decodings;
    std::set<std::string> decoded_sources;
    for (const auto& [module_name, inst_name] : clone_pairs) {
      if (decodesToClone(name, module_name, inst_name)) {
        decodings.push_back("module '" + module_name + "' instantiated as '"
                            + inst_name + "'");
        decoded_sources.insert(module_name);
      }
    }
    if (decodings.empty()) {
      problems.push_back(
          "emitted module name '" + name
          + "' decodes to no <input module>_<instance name> pair, so nothing in"
            " it identifies the module it was cloned from");
      continue;
    }
    if (decoded_sources.size() > 1) {
      problems.push_back("emitted module name '" + name
                         + "' is the uniquification name of " + join(decodings)
                         + " at once, so it does not identify which module was"
                           " cloned");
      continue;
    }
    if (sources.size() == 1 && *decoded_sources.begin() != *sources.begin()) {
      problems.push_back("emitted module '" + name
                         + "' is a copy of input module '" + *sources.begin()
                         + "' but its name implies it was cloned from '"
                         + *decoded_sources.begin() + "'");
    }
  }
  return problems;
}

std::vector<std::string> checkCellCensus(const FileView& in,
                                         const FileView& out,
                                         const std::string& top)
{
  std::vector<std::string> problems;
  const std::map<std::string, int> in_census = cellCensus(in, top);
  const std::map<std::string, int> out_census = cellCensus(out, top);
  std::set<std::string> masters;
  for (const auto& [master, count] : in_census) {
    masters.insert(master);
  }
  for (const auto& [master, count] : out_census) {
    masters.insert(master);
  }
  for (const std::string& master : masters) {
    const int in_count
        = in_census.count(master) != 0 ? in_census.at(master) : 0;
    const int out_count
        = out_census.count(master) != 0 ? out_census.at(master) : 0;
    if (in_count != out_count) {
      problems.push_back("elaborated instance count of '" + master
                         + "' changed: " + std::to_string(in_count) + " -> "
                         + std::to_string(out_count));
    }
  }
  return problems;
}

std::vector<std::string> checkAssigns(
    const FileView& in,
    const FileView& out,
    Path path,
    const std::vector<std::string>& module_names)
{
  std::vector<std::string> problems;
  for (const std::string& name : module_names) {
    const ModuleView& in_module = in.modules.at(name);
    const ModuleView& out_module = out.modules.at(name);
    std::vector<std::string> dropped;
    std::vector<std::string> added;
    for (const std::string& lhs : in_module.assign_lhs) {
      if (out_module.assign_lhs.count(lhs) == 0) {
        dropped.push_back(lhs);
      }
    }
    for (const std::string& lhs : out_module.assign_lhs) {
      if (in_module.assign_lhs.count(lhs) == 0) {
        added.push_back(lhs);
      }
    }
    if (!dropped.empty()) {
      problems.push_back("module '" + name
                         + "': names driven by an assign in the input but not"
                           " in the output: "
                         + join(dropped));
    }
    // Flattening legitimately rewrites alias chains, moving assigns between
    // scopes, so a new assign in the flat top is not reportable there.
    if (!added.empty() && path == Path::kHier) {
      problems.push_back("module '" + name
                         + "': names driven by an assign in the output but not"
                           " in the input: "
                         + join(added));
    }
  }
  return problems;
}

std::vector<std::string> checkNamespace(const FileView& out)
{
  std::vector<std::string> problems;
  for (const auto& [name, module] : out.modules) {
    if (!module.duplicate_ports.empty()) {
      problems.push_back("module '" + name + "' declares a port twice: "
                         + join(module.duplicate_ports));
    }
    if (!module.duplicate_nets.empty()) {
      problems.push_back("module '" + name + "' declares a net twice: "
                         + join(module.duplicate_nets));
    }
    if (!module.inst_name_collisions.empty()) {
      problems.push_back(
          "module '" + name
          + "' uses one name for both an instance and a net or port: "
          + join(module.inst_name_collisions));
    }
    if (!module.illegal_names.empty()) {
      problems.push_back("module '" + name
                         + "' names an object with something that is not a"
                           " legal unescaped identifier: "
                         + join(module.illegal_names));
    }
  }
  return problems;
}

////////////////////////////////////////////////////////////////////////////
// Test body
////////////////////////////////////////////////////////////////////////////

void checkStructure(const CorpusEntry& entry, Path path)
{
  ASSERT_TRUE(entry.load_error.empty()) << entry.load_error;

  const bool hierarchy = path == Path::kHier;
  const std::filesystem::path out_v
      = workDir() / (fileStem(entry.name) + "." + toString(path) + ".struct.v");

  // Only one design is kept live at a time: LoadedDesign owns a dbSta, and
  // sta::Sta keeps a global pointer to the first one constructed.
  //
  // A netlist the reader refuses is a finding, not a reason to stop: several
  // corpus cases exist precisely because OpenROAD cannot link them. It is
  // reported as a round_trip problem and routed through the XFAIL manifest like
  // any other, and the remaining checks are skipped rather than reported as
  // passing. utl::Logger::error throws, so a refusal arrives here as an
  // exception; the catch-all is for the ones that throw something else.
  std::vector<std::string> round_trip;
  bool wrote_output = false;
  {
    std::optional<LoadedDesign> design;
    try {
      design.emplace(entry.tech, entry.path, entry.top.c_str(), hierarchy);
    } catch (const std::exception& e) {
      round_trip.push_back(std::string("read_verilog/link_design")
                           + (hierarchy ? " -hier" : "")
                           + " rejected the input netlist: " + e.what());
    } catch (...) {
      round_trip.push_back(std::string("read_verilog/link_design")
                           + (hierarchy ? " -hier" : "")
                           + " rejected the input netlist by throwing a"
                             " non-std::exception");
    }
    if (design.has_value()) {
      try {
        design->writeVerilog(out_v);
        wrote_output = true;
      } catch (const std::exception& e) {
        round_trip.push_back(std::string("write_verilog threw: ") + e.what());
      } catch (...) {
        round_trip.emplace_back("write_verilog threw a non-std::exception");
      }
    }
  }

  if (wrote_output) {
    // The emitted netlist must be readable and linkable again. This is the
    // only part of the original odb-vs-odb plan that survives: see the header
    // comment for why the comparison itself is done on the netlists.
    try {
      LoadedDesign relinked(entry.tech, out_v, entry.top.c_str(), hierarchy);
    } catch (const std::exception& e) {
      round_trip.push_back(std::string("the emitted netlist could not be"
                                       " re-linked")
                           + (hierarchy ? " -hier" : "") + ": " + e.what());
    } catch (...) {
      round_trip.push_back(std::string("the emitted netlist could not be"
                                       " re-linked")
                           + (hierarchy ? " -hier" : "")
                           + ": a non-std::exception was thrown");
    }
  }

  FileView in_view;
  FileView out_view;
  if (wrote_output) {
    in_view = scanVerilogFile(entry.path);
    out_view = scanVerilogFile(out_v);
    if (!in_view.error.empty()) {
      round_trip.push_back("could not scan the input netlist: "
                           + in_view.error);
    }
    if (!out_view.error.empty()) {
      round_trip.push_back("could not scan the emitted netlist: "
                           + out_view.error);
    }
  }

  expectOrXfail(entry, path, Check::kRoundTrip, round_trip);

  if (!wrote_output || !in_view.error.empty() || !out_view.error.empty()) {
    // Nothing to compare. Every other check is skipped rather than reported as
    // passing.
    return;
  }

  // Modules to compare per-module aspects over. In flat mode only the top
  // survives, and its internals are legitimately renamed to instance paths, so
  // the per-module aspects are compared for the top module alone.
  std::vector<std::string> common_modules;
  for (const auto& [name, module] : in_view.modules) {
    if (out_view.modules.count(name) == 0) {
      continue;
    }
    if (path == Path::kFlat && name != entry.top) {
      continue;
    }
    common_modules.push_back(name);
  }

  std::vector<std::string> submodules;
  for (const std::string& name : common_modules) {
    if (name != entry.top) {
      submodules.push_back(name);
    }
  }

  expectOrXfail(entry,
                path,
                Check::kModuleSet,
                checkModuleSet(in_view, out_view, path, entry.top));
  expectOrXfail(entry,
                path,
                Check::kTopPorts,
                checkPortList(in_view, out_view, entry.top));

  std::vector<std::string> submodule_ports;
  for (const std::string& name : submodules) {
    const std::vector<std::string> problems
        = checkPortList(in_view, out_view, name);
    submodule_ports.insert(
        submodule_ports.end(), problems.begin(), problems.end());
  }
  expectOrXfail(entry, path, Check::kSubmodulePorts, submodule_ports);

  std::set<std::string> paths_allowed_in;
  if (path == Path::kFlat) {
    paths_allowed_in.insert(common_modules.begin(), common_modules.end());
  } else {
    paths_allowed_in.insert(entry.top);
  }
  expectOrXfail(
      entry,
      path,
      Check::kDeclaredNets,
      checkDeclaredNets(in_view, out_view, common_modules, paths_allowed_in));
  // Instance names in the flat output are synthesized hierarchical paths, so
  // the per-module binding multiset is a hier-mode aspect; kCellCensus covers
  // the flat path.
  expectOrXfail(entry,
                path,
                Check::kInstances,
                path == Path::kHier
                    ? checkInstances(in_view, out_view, common_modules)
                    : std::vector<std::string>{});
  // Name identity is a hier-mode aspect: the flat writer emits one module.
  expectOrXfail(entry,
                path,
                Check::kNameIdentity,
                path == Path::kHier
                    ? checkNameIdentity(in_view, out_view, entry.top)
                    : std::vector<std::string>{});
  expectOrXfail(entry,
                path,
                Check::kCellCensus,
                checkCellCensus(in_view, out_view, entry.top));
  expectOrXfail(entry,
                path,
                Check::kAssigns,
                checkAssigns(in_view, out_view, path, common_modules));
  expectOrXfail(entry, path, Check::kNamespace, checkNamespace(out_view));
}

class TestStructuralHier : public ::testing::TestWithParam<CorpusEntry>
{
};

class TestStructuralFlat : public ::testing::TestWithParam<CorpusEntry>
{
};

TEST_P(TestStructuralHier, MatchesInput)
{
  checkStructure(GetParam(), Path::kHier);
}

TEST_P(TestStructuralFlat, MatchesInput)
{
  checkStructure(GetParam(), Path::kFlat);
}

INSTANTIATE_TEST_SUITE_P(HierCases,
                         TestStructuralHier,
                         ::testing::ValuesIn(corpus()),
                         entryName);

INSTANTIATE_TEST_SUITE_P(HierCases,
                         TestStructuralFlat,
                         ::testing::ValuesIn(corpus()),
                         entryName);

// Guards against the failure mode this whole suite exists to avoid: a corpus
// that loaded as zero cases would make both suites above vacuously green.
TEST(TestHierStructuralCorpus, IsLoaded)
{
  ASSERT_FALSE(corpus().empty());
  for (const CorpusEntry& entry : corpus()) {
    ASSERT_TRUE(entry.load_error.empty()) << entry.load_error;
  }
  EXPECT_GT(corpus().size(), 1U)
      << "only one corpus case resolved; the manifest is probably not being "
         "read";
  for (const CorpusEntry& entry : corpus()) {
    EXPECT_TRUE(std::filesystem::exists(entry.path))
        << entry.name << " listed in the manifest does not exist at "
        << entry.path;
  }

  // The structural-only subdirectory is this suite's alone, so nothing else
  // would notice if its data dependency stopped resolving.
  const std::string prefix = std::string(kStructuralSubdir) + "/";
  std::size_t structural_cases = 0;
  for (const CorpusEntry& entry : corpus()) {
    if (entry.name.rfind(prefix, 0) == 0) {
      ++structural_cases;
    }
  }
  EXPECT_GT(structural_cases, 1U)
      << "the hier_cases/" << kStructuralSubdir
      << " corpus subdirectory contributed " << structural_cases
      << " cases; only this suite loads it, so a broken data dependency there "
         "is invisible everywhere else";
}

// The scanner is the measuring instrument: if it silently fails to understand a
// netlist, every check on that netlist is vacuous. Every corpus netlist must
// scan, must define its declared top module, and that module must have at
// least one instance -- an empty top would mean the body was skipped.
TEST(TestHierStructuralCorpus, ScannerUnderstandsEveryNetlist)
{
  for (const CorpusEntry& entry : corpus()) {
    if (!entry.load_error.empty()) {
      continue;
    }
    const FileView view = scanVerilogFile(entry.path);
    EXPECT_TRUE(view.error.empty()) << entry.name << ": " << view.error;
    EXPECT_EQ(view.modules.count(entry.top), 1U)
        << entry.name << ": scanner did not find the top module '" << entry.top
        << "'; it found: " << [&]() {
             std::vector<std::string> names;
             for (const auto& [name, module] : view.modules) {
               names.push_back(name);
             }
             return join(names);
           }();
  }
}

// The scanner's lexical rules, on the two forms this corpus is full of: an
// escaped identifier that contains a '/' or a bracket, and a bit select
// applied to one.
TEST(TestHierStructuralScanner, HandlesEscapedIdentifiers)
{
  const std::string src = R"(
module top (a, \b/c[1] );
   input a;
   output [3:0] \b/c[1] ;
   wire \net[3] ;
   \mod/slash  \u/inst  (.i(a), .o(\net[3] ));
   BUF_X1 g0 (.A(\net[3] ), .Z(\b/c[1] [0]));
   assign \b/c[1] [1] = a;
endmodule
)";
  Scanner scanner(tokenize(src));
  const FileView view = scanner.scan();
  ASSERT_EQ(view.modules.count("top"), 1U);
  const ModuleView& top = view.modules.at("top");

  ASSERT_EQ(top.ports.size(), 2U);
  EXPECT_EQ(top.ports[0].name, "a");
  EXPECT_EQ(top.ports[0].dir, "input");
  EXPECT_EQ(top.ports[1].name, "b/c[1]");
  EXPECT_EQ(top.ports[1].dir, "output");
  EXPECT_EQ(top.ports[1].range, "[3:0]");

  EXPECT_EQ(top.objects.count(Decl{"net[3]", ""}), 1U);
  EXPECT_EQ(top.insts.count(InstBinding{"u/inst", "mod/slash"}), 1U);
  EXPECT_EQ(top.insts.count(InstBinding{"g0", "BUF_X1"}), 1U);
  EXPECT_EQ(top.assign_lhs.count("b/c[1]"), 1U);
  EXPECT_TRUE(top.duplicate_nets.empty());
}

// An escaped identifier and the plain identifier it escapes are the same name.
TEST(TestHierStructuralScanner, CanonicalizesEscapedForm)
{
  const std::string escaped = R"(
module top (a);
   input a;
   wire \n$1 ;
endmodule
)";
  const std::string plain = R"(
module top (a);
   input a;
   wire n$1;
endmodule
)";
  Scanner escaped_scanner(tokenize(escaped));
  Scanner plain_scanner(tokenize(plain));
  const FileView escaped_view = escaped_scanner.scan();
  const FileView plain_view = plain_scanner.scan();
  EXPECT_EQ(escaped_view.modules.at("top").objects,
            plain_view.modules.at("top").objects);
}

// Duplicate declarations and a name used for both an instance and a net are
// illegal in one module namespace, and are what the writer's synthesized flat
// names collide into.
TEST(TestHierStructuralScanner, FindsNamespaceCollisions)
{
  const std::string src = R"(
module top (a, y);
   input a;
   output y;
   wire dup;
   wire dup;
   wire shared;
   BUF_X1 shared (.A(a), .Z(y));
endmodule
)";
  Scanner scanner(tokenize(src));
  const FileView view = scanner.scan();
  const ModuleView& top = view.modules.at("top");
  EXPECT_EQ(top.duplicate_nets, std::vector<std::string>{"dup"});
  EXPECT_EQ(top.inst_name_collisions, std::vector<std::string>{"shared"});
}

// The elaborated census must count a twice-instantiated module's gates twice,
// or a flat netlist could never be compared with its hierarchical input.
TEST(TestHierStructuralScanner, CountsElaboratedCells)
{
  const std::string src = R"(
module top (a, y0, y1);
   input a;
   output y0, y1;
   sub u0 (.i(a), .o(y0));
   sub u1 (.i(a), .o(y1));
endmodule

module sub (i, o);
   input i;
   output o;
   BUF_X1 b0 (.A(i), .Z(o));
   INV_X1 v0 (.A(i), .ZN(o));
endmodule
)";
  Scanner scanner(tokenize(src));
  const FileView view = scanner.scan();
  const std::map<std::string, int> census = cellCensus(view, "top");
  EXPECT_EQ(census.size(), 2U);
  EXPECT_EQ(census.at("BUF_X1"), 2);
  EXPECT_EQ(census.at("INV_X1"), 2);
}

// A non-ANSI header takes its ranges from the body declarations; an ANSI header
// carries them itself, and there a new direction keyword ends the previous
// declaration's range while a comma continues it.
TEST(TestHierStructuralScanner, ScopesAnsiHeaderRanges)
{
  const std::string src = R"(
module top (
  input [7:0] in_bus,
  input       in_scalar,
  output [1:0] out_a, out_b
);
endmodule
)";
  Scanner scanner(tokenize(src));
  const FileView view = scanner.scan();
  const std::vector<PortDecl>& ports = view.modules.at("top").ports;
  ASSERT_EQ(ports.size(), 4U);
  EXPECT_EQ(ports[0].range, "[7:0]");
  EXPECT_EQ(ports[1].range, "");
  EXPECT_EQ(ports[2].range, "[1:0]");
  EXPECT_EQ(ports[3].range, "[1:0]");
}

// An attribute instance is not an instantiation of a cell named after the
// attribute.
TEST(TestHierStructuralScanner, SkipsAttributes)
{
  const std::string src = R"(
(* my_module_attr = "kept" *)
module top (a, y);
   input a;
   output y;
   (* dont_touch = 1 *)
   (* src = "gen.v:12.3-12.9" *)
   BUF_X1 g0 (.A(a), .Z(y));
endmodule
)";
  Scanner scanner(tokenize(src));
  const FileView view = scanner.scan();
  ASSERT_EQ(view.modules.count("top"), 1U);
  const ModuleView& top = view.modules.at("top");
  EXPECT_EQ(top.insts.size(), 1U);
  EXPECT_EQ(top.insts.count(InstBinding{"g0", "BUF_X1"}), 1U);
}

// A name that needed an escape and did not get one is not readable Verilog, and
// is what the writer emits for a digit-leading identifier.
TEST(TestHierStructuralScanner, FindsUnescapedIllegalNames)
{
  const std::string legal = R"(
module top (a, y);
   input a;
   output y;
   wire \1n ;
   BUF_X1 \1g  (.A(a), .Z(\1n ));
   BUF_X1 g2 (.A(\1n ), .Z(y));
endmodule
)";
  const std::string illegal = R"(
module top (a, y);
   input a;
   output y;
   wire 1n;
   BUF_X1 1g (.A(1n), .Z(1n));
   BUF_X1 g2 (.A(1n), .Z(y));
endmodule
)";
  Scanner legal_scanner(tokenize(legal));
  Scanner illegal_scanner(tokenize(illegal));
  EXPECT_TRUE(checkNamespace(legal_scanner.scan()).empty());
  EXPECT_FALSE(checkNamespace(illegal_scanner.scan()).empty());

  // An escaped name that happens to be a keyword loses more than its escape:
  // `output output;` declares nothing at all.
  const std::string keyword = R"(
module top (a, output);
   input a;
   output output;
   INV_X1 g (.A(a), .ZN(output));
endmodule
)";
  Scanner keyword_scanner(tokenize(keyword));
  EXPECT_FALSE(checkNamespace(keyword_scanner.scan()).empty());
}

////////////////////////////////////////////////////////////////////////////
// The detectors themselves, on synthesized (input, output) pairs. Each of the
// positive cases below is a defect that is known to be present today and that
// no LEC reports; each of the negative cases is a change the writer makes that
// preserves structure and must not be reported. Without these, a checker that
// quietly stopped detecting anything would still look green.
////////////////////////////////////////////////////////////////////////////

FileView viewOf(const std::string& src)
{
  Scanner scanner(tokenize(src));
  return scanner.scan();
}

TEST(TestHierStructuralDetector, ReportsReorderedTopPorts)
{
  const FileView in = viewOf(
      "module top (d, ck, q); input d, ck; output q;"
      " DFF_X1 r (.D(d), .CK(ck), .Q(q)); endmodule");
  const FileView out = viewOf(
      "module top (ck, d, q); input ck; input d;"
      " output q;"
      " DFF_X1 r (.D(d), .CK(ck), .Q(q)); endmodule");
  EXPECT_FALSE(checkPortList(in, out, "top").empty());
}

TEST(TestHierStructuralDetector, ReportsPerInstanceModuleClones)
{
  const FileView in = viewOf(
      "module top (a, y0, y1); input a; output y0, y1;"
      " sub u0 (.i(a), .o(y0)); sub u1 (.i(a), .o(y1)); endmodule"
      " module sub (i, o); input i; output o;"
      " BUF_X1 b (.A(i), .Z(o)); endmodule");
  const FileView out = viewOf(
      "module top (a, y0, y1); input a; output y0, y1;"
      " sub u0 (.i(a), .o(y0)); sub_u1 u1 (.i(a), .o(y1)); endmodule"
      " module sub (i, o); input i; output o;"
      " BUF_X1 b (.A(i), .Z(o)); endmodule"
      " module sub_u1 (i, o); input i; output o;"
      " BUF_X1 b (.A(i), .Z(o)); endmodule");
  EXPECT_FALSE(checkModuleSet(in, out, Path::kHier, "top").empty());
  EXPECT_FALSE(checkInstances(in, out, {"top"}).empty());
}

// The two defects the generic module_set/instances rows cannot distinguish from
// a benign clone. Both of these netlists are shapes the corpus contains
// (bx_collisions_uniq_cross_prefix, bx_collisions_uniq_vs_module_collide), and
// in both the emitted netlist is equivalent to its input.
TEST(TestHierStructuralDetector, ReportsAmbiguousCloneName)
{
  // `psub` instantiated as `x_c2` and `psub_x` instantiated as `c2` both
  // request the clone name `psub_x_c2`.
  const FileView in = viewOf(
      "module psub (a, z); input a; output z;"
      " BUF_X1 u1 (.A(a), .Z(z)); endmodule"
      " module psub_x (a, z); input a; output z;"
      " INV_X1 u1 (.A(a), .ZN(z)); endmodule"
      " module top (i1, i2, i3, i4, o1, o2, o3, o4);"
      " input i1, i2, i3, i4; output o1, o2, o3, o4;"
      " psub a1 (.a(i1), .z(o1)); psub x_c2 (.a(i2), .z(o2));"
      " psub_x c1 (.a(i3), .z(o3)); psub_x c2 (.a(i4), .z(o4)); endmodule");
  const FileView out = viewOf(
      "module psub (a, z); input a; output z;"
      " BUF_X1 u1 (.A(a), .Z(z)); endmodule"
      " module psub_x (a, z); input a; output z;"
      " INV_X1 u1 (.A(a), .ZN(z)); endmodule"
      " module psub_x_c2 (a, z); input a; output z;"
      " BUF_X1 u1 (.A(a), .Z(z)); endmodule"
      " module psub_x_c2_1 (a, z); input a; output z;"
      " INV_X1 u1 (.A(a), .ZN(z)); endmodule"
      " module top (i1, i2, i3, i4, o1, o2, o3, o4);"
      " input i1, i2, i3, i4; output o1, o2, o3, o4;"
      " psub a1 (.a(i1), .z(o1)); psub_x_c2 x_c2 (.a(i2), .z(o2));"
      " psub_x c1 (.a(i3), .z(o3));"
      " psub_x_c2_1 c2 (.a(i4), .z(o4)); endmodule");
  const std::vector<std::string> problems = checkNameIdentity(in, out, "top");
  EXPECT_FALSE(problems.empty());
  EXPECT_NE(join(problems).find("does not identify which module was cloned"),
            std::string::npos)
      << join(problems);
}

TEST(TestHierStructuralDetector, ReportsCloneNameImplyingTheWrongSourceModule)
{
  // The clone of `sub` for instance `i2` takes the name of the input's own
  // module `sub_i2`, which is pushed onto `sub_i2_u3`. Nothing is lost and
  // nothing is added -- the name `sub_i2` now just means a different module.
  const FileView in = viewOf(
      "module sub (a, z); input a; output z;"
      " BUF_X1 u1 (.A(a), .Z(z)); endmodule"
      " module sub_i2 (a, z); input a; output z;"
      " INV_X1 u1 (.A(a), .ZN(z)); endmodule"
      " module top (i1, i2, i3, o1, o2, o3);"
      " input i1, i2, i3; output o1, o2, o3;"
      " sub i1 (.a(i1), .z(o1)); sub i2 (.a(i2), .z(o2));"
      " sub_i2 u3 (.a(i3), .z(o3)); endmodule");
  const FileView out = viewOf(
      "module sub (a, z); input a; output z;"
      " BUF_X1 u1 (.A(a), .Z(z)); endmodule"
      " module sub_i2 (a, z); input a; output z;"
      " BUF_X1 u1 (.A(a), .Z(z)); endmodule"
      " module sub_i2_u3 (a, z); input a; output z;"
      " INV_X1 u1 (.A(a), .ZN(z)); endmodule"
      " module top (i1, i2, i3, o1, o2, o3);"
      " input i1, i2, i3; output o1, o2, o3;"
      " sub i1 (.a(i1), .z(o1)); sub_i2 i2 (.a(i2), .z(o2));"
      " sub_i2_u3 u3 (.a(i3), .z(o3)); endmodule");
  const std::vector<std::string> problems = checkNameIdentity(in, out, "top");
  EXPECT_FALSE(problems.empty());
  EXPECT_NE(join(problems).find("not of the input module of that name"),
            std::string::npos)
      << join(problems);
  // The displaced module is emitted under a name that does decode to it, so it
  // is not reported a second time.
  EXPECT_EQ(problems.size(), 1U) << join(problems);
}

// Uniquification per se is not a name-identity defect: as long as the clone
// name decodes to exactly one module, and to the module it is a copy of, the
// mapping from name to module survives. Prefix-related module names (psub /
// psub_x) are the interesting negative: they are only ambiguous when an
// instance name lines up with the prefix difference, which here it does not.
TEST(TestHierStructuralDetector, AcceptsUnambiguousClones)
{
  const FileView in = viewOf(
      "module sub (a, z); input a; output z;"
      " BUF_X1 u1 (.A(a), .Z(z)); endmodule"
      " module top (i1, i2, o1, o2); input i1, i2; output o1, o2;"
      " sub i1 (.a(i1), .z(o1)); sub i2 (.a(i2), .z(o2)); endmodule");
  const FileView out = viewOf(
      "module sub (a, z); input a; output z;"
      " BUF_X1 u1 (.A(a), .Z(z)); endmodule"
      " module sub_i2 (a, z); input a; output z;"
      " BUF_X1 u1 (.A(a), .Z(z)); endmodule"
      " module top (i1, i2, o1, o2); input i1, i2; output o1, o2;"
      " sub i1 (.a(i1), .z(o1)); sub_i2 i2 (.a(i2), .z(o2)); endmodule");
  const std::vector<std::string> clone_problems
      = checkNameIdentity(in, out, "top");
  EXPECT_TRUE(clone_problems.empty()) << join(clone_problems);

  const FileView prefix_in = viewOf(
      "module psub (a, z); input a; output z;"
      " BUF_X1 u1 (.A(a), .Z(z)); endmodule"
      " module psub_x (a, z); input a; output z;"
      " INV_X1 u1 (.A(a), .ZN(z)); endmodule"
      " module top (i1, i2, i3, i4, o1, o2, o3, o4);"
      " input i1, i2, i3, i4; output o1, o2, o3, o4;"
      " psub a1 (.a(i1), .z(o1)); psub a2 (.a(i2), .z(o2));"
      " psub_x c1 (.a(i3), .z(o3)); psub_x c2 (.a(i4), .z(o4)); endmodule");
  const FileView prefix_out = viewOf(
      "module psub (a, z); input a; output z;"
      " BUF_X1 u1 (.A(a), .Z(z)); endmodule"
      " module psub_a2 (a, z); input a; output z;"
      " BUF_X1 u1 (.A(a), .Z(z)); endmodule"
      " module psub_x (a, z); input a; output z;"
      " INV_X1 u1 (.A(a), .ZN(z)); endmodule"
      " module psub_x_c2 (a, z); input a; output z;"
      " INV_X1 u1 (.A(a), .ZN(z)); endmodule"
      " module top (i1, i2, i3, i4, o1, o2, o3, o4);"
      " input i1, i2, i3, i4; output o1, o2, o3, o4;"
      " psub a1 (.a(i1), .z(o1)); psub_a2 a2 (.a(i2), .z(o2));"
      " psub_x c1 (.a(i3), .z(o3));"
      " psub_x_c2 c2 (.a(i4), .z(o4)); endmodule");
  const std::vector<std::string> prefix_problems
      = checkNameIdentity(prefix_in, prefix_out, "top");
  EXPECT_TRUE(prefix_problems.empty()) << join(prefix_problems);

  // The clone of a module instantiated under the same name in two parents is
  // renamed by uniquification's numeric fallback. That still decodes.
  const FileView deep_in = viewOf(
      "module leaf (a, z); input a; output z;"
      " BUF_X1 g (.A(a), .Z(z)); endmodule"
      " module mid1 (a, z); input a; output z;"
      " leaf u (.a(a), .z(z)); endmodule"
      " module mid2 (a, z); input a; output z;"
      " leaf u (.a(a), .z(z)); endmodule"
      " module mid3 (a, z); input a; output z;"
      " leaf u (.a(a), .z(z)); endmodule"
      " module top (i1, i2, i3, o1, o2, o3);"
      " input i1, i2, i3; output o1, o2, o3;"
      " mid1 m1 (.a(i1), .z(o1)); mid2 m2 (.a(i2), .z(o2));"
      " mid3 m3 (.a(i3), .z(o3)); endmodule");
  const FileView deep_out = viewOf(
      "module leaf (a, z); input a; output z;"
      " BUF_X1 g (.A(a), .Z(z)); endmodule"
      " module leaf_u (a, z); input a; output z;"
      " BUF_X1 g (.A(a), .Z(z)); endmodule"
      " module leaf_u_1 (a, z); input a; output z;"
      " BUF_X1 g (.A(a), .Z(z)); endmodule"
      " module mid1 (a, z); input a; output z;"
      " leaf u (.a(a), .z(z)); endmodule"
      " module mid2 (a, z); input a; output z;"
      " leaf_u u (.a(a), .z(z)); endmodule"
      " module mid3 (a, z); input a; output z;"
      " leaf_u_1 u (.a(a), .z(z)); endmodule"
      " module top (i1, i2, i3, o1, o2, o3);"
      " input i1, i2, i3; output o1, o2, o3;"
      " mid1 m1 (.a(i1), .z(o1)); mid2 m2 (.a(i2), .z(o2));"
      " mid3 m3 (.a(i3), .z(o3)); endmodule");
  const std::vector<std::string> deep_problems
      = checkNameIdentity(deep_in, deep_out, "top");
  EXPECT_TRUE(deep_problems.empty()) << join(deep_problems);
}

TEST(TestHierStructuralDetector, ReportsUninstantiatedModuleDropped)
{
  const FileView in = viewOf(
      "module top (a, y); input a; output y;"
      " BUF_X1 b (.A(a), .Z(y)); endmodule"
      " module spare (i, o); input i; output o;"
      " BUF_X1 b (.A(i), .Z(o)); endmodule");
  const FileView out = viewOf(
      "module top (a, y); input a; output y;"
      " BUF_X1 b (.A(a), .Z(y)); endmodule");
  EXPECT_FALSE(checkModuleSet(in, out, Path::kHier, "top").empty());
}

TEST(TestHierStructuralDetector, ReportsErasedDanglingObjects)
{
  const FileView in = viewOf(
      "module top (a, y); input a; output y;"
      " wire dead; wire [3:0] partly_used;"
      " assign dead = a;"
      " BUF_X1 b (.A(a), .Z(y)); endmodule");
  const FileView out = viewOf(
      "module top (a, y); input a; output y;"
      " BUF_X1 b (.A(a), .Z(y)); endmodule");
  EXPECT_FALSE(
      checkDeclaredNets(in, out, {"top"}, /*paths_allowed_in=*/{}).empty());
  EXPECT_FALSE(checkAssigns(in, out, Path::kHier, {"top"}).empty());
}

TEST(TestHierStructuralDetector, ReportsInventedFillerWires)
{
  const FileView in = viewOf(
      "module top (a, y); input a; output y;"
      " BUF_X1 b (.A(a), .Z(y)); endmodule");
  const FileView out = viewOf(
      "module top (a, y); input a; output y;"
      " wire _NC1; wire _NC2;"
      " BUF_X1 b (.A(a), .Z(y)); endmodule");
  EXPECT_FALSE(
      checkDeclaredNets(in, out, {"top"}, /*paths_allowed_in=*/{}).empty());
  // A filler wire invented twice under one name is illegal, not merely ugly.
  const FileView collided = viewOf(
      "module top (a, y); input a; output y;"
      " wire _NC1; wire _NC1;"
      " BUF_X1 b (.A(a), .Z(y)); endmodule");
  EXPECT_FALSE(checkNamespace(collided).empty());
}

TEST(TestHierStructuralDetector, ReportsModuleLocalNetRenamedToInstancePath)
{
  const FileView in = viewOf(
      "module top (a, y); input a; output y;"
      " sub u1 (.i(a), .o(y)); endmodule"
      " module sub (i, o); input i; output o;"
      " wire n; BUF_X1 b (.A(i), .Z(n));"
      " BUF_X1 c (.A(n), .Z(o)); endmodule");
  const FileView out = viewOf(
      "module top (a, y); input a; output y;"
      " sub u1 (.i(a), .o(y)); endmodule"
      " module sub (i, o); input i; output o;"
      " wire \\u1/n ; BUF_X1 b (.A(i), .Z(\\u1/n ));"
      " BUF_X1 c (.A(\\u1/n ), .Z(o)); endmodule");
  // The path is allowed in the top module only; inside `sub` it is the defect.
  EXPECT_FALSE(
      checkDeclaredNets(in, out, {"top", "sub"}, /*paths_allowed_in=*/{"top"})
          .empty());
}

TEST(TestHierStructuralDetector, ReportsDroppedGates)
{
  const FileView in = viewOf(
      "module top (a, y); input a; output y;"
      " wire n; INV_X1 v (.A(a), .ZN(n));"
      " INV_X1 w (.A(n), .ZN(y)); endmodule");
  const FileView out = viewOf(
      "module top (a, y); input a; output y;"
      " INV_X1 v (.A(a), .ZN(y)); endmodule");
  EXPECT_FALSE(checkCellCensus(in, out, "top").empty());
}

TEST(TestHierStructuralDetector, AcceptsFlattenedPathNames)
{
  const FileView in = viewOf(
      "module top (a, y); input a; output y;"
      " sub u1 (.i(a), .o(y)); endmodule"
      " module sub (i, o); input i; output o;"
      " wire n; BUF_X1 b (.A(i), .Z(n));"
      " BUF_X1 c (.A(n), .Z(o)); endmodule");
  const FileView out = viewOf(
      "module top (a, y); input a; output y;"
      " wire \\u1/n ;"
      " BUF_X1 \\u1/b  (.A(a), .Z(\\u1/n ));"
      " BUF_X1 \\u1/c  (.A(\\u1/n ), .Z(y)); endmodule");
  EXPECT_TRUE(checkDeclaredNets(in, out, {"top"}, /*paths_allowed_in=*/{"top"})
                  .empty());
  EXPECT_TRUE(checkModuleSet(in, out, Path::kFlat, "top").empty());
  EXPECT_TRUE(checkCellCensus(in, out, "top").empty());
}

// An escaped name that already contains '/' must not make the flattened-path
// rule accept an arbitrary invented name.
TEST(TestHierStructuralDetector, AcceptsFlattenedPathsThroughEscapedNames)
{
  const FileView in = viewOf(
      "module top (a, y); input a; output y;"
      " \\u/inst  m (.i(a), .o(y)); endmodule"
      " module \\u/inst  (i, o); input i; output o;"
      " wire \\net/with/slash ; BUF_X1 b (.A(i), .Z(\\net/with/slash ));"
      " BUF_X1 c (.A(\\net/with/slash ), .Z(o)); endmodule");
  const FileView out = viewOf(
      "module top (a, y); input a; output y;"
      " wire \\m/net/with/slash ;"
      " BUF_X1 \\m/b  (.A(a), .Z(\\m/net/with/slash ));"
      " BUF_X1 \\m/c  (.A(\\m/net/with/slash ), .Z(y)); endmodule");
  EXPECT_TRUE(checkDeclaredNets(in, out, {"top"}, /*paths_allowed_in=*/{"top"})
                  .empty());
  const FileView invented = viewOf(
      "module top (a, y); input a; output y;"
      " wire \\m/not_a_name ;"
      " BUF_X1 \\m/b  (.A(a), .Z(\\m/not_a_name ));"
      " BUF_X1 \\m/c  (.A(\\m/not_a_name ), .Z(y)); endmodule");
  EXPECT_FALSE(
      checkDeclaredNets(in, invented, {"top"}, /*paths_allowed_in=*/{"top"})
          .empty());
}

TEST(TestHierStructuralDetector, AcceptsReEscapedDollarIdentifiers)
{
  const FileView in = viewOf(
      "module top (a$b, y); input a$b; output y;"
      " wire n$1; BUF_X1 g$0 (.A(a$b), .Z(n$1));"
      " BUF_X1 g$1 (.A(n$1), .Z(y)); endmodule");
  const FileView out = viewOf(
      "module top (\\a$b , y); input \\a$b ; output y;"
      " wire \\n$1 ; BUF_X1 \\g$0  (.A(\\a$b ), .Z(\\n$1 ));"
      " BUF_X1 \\g$1  (.A(\\n$1 ), .Z(y)); endmodule");
  EXPECT_TRUE(checkPortList(in, out, "top").empty());
  EXPECT_TRUE(
      checkDeclaredNets(in, out, {"top"}, /*paths_allowed_in=*/{}).empty());
  EXPECT_TRUE(checkInstances(in, out, {"top"}).empty());
  EXPECT_TRUE(checkNamespace(out).empty());
}

// Bit-blasting a vector connection and exploding a vector assign into per-bit
// assigns are equivalence- and connectivity-preserving shape changes. Reporting
// them would bury the real findings.
TEST(TestHierStructuralDetector, AcceptsPerBitExplosion)
{
  const FileView in = viewOf(
      "module top (i, z); input [1:0] i; output [1:0] z;"
      " wire [1:0] n; sub u (.a(i), .y(n)); assign z[1:0] = n[1:0];"
      " endmodule"
      " module sub (a, y); input [1:0] a; output [1:0] y;"
      " BUF_X1 b0 (.A(a[0]), .Z(y[0]));"
      " BUF_X1 b1 (.A(a[1]), .Z(y[1])); endmodule");
  const FileView out = viewOf(
      "module top (i, z); input [1:0] i; output [1:0] z;"
      " wire [1:0] n; sub u (.a({i[1],i[0]}), .y({n[1],n[0]}));"
      " assign z[0] = n[0]; assign z[1] = n[1]; endmodule"
      " module sub (a, y); input [1:0] a; output [1:0] y;"
      " BUF_X1 b0 (.A(a[0]), .Z(y[0]));"
      " BUF_X1 b1 (.A(a[1]), .Z(y[1])); endmodule");
  EXPECT_TRUE(checkPortList(in, out, "top").empty());
  EXPECT_TRUE(checkPortList(in, out, "sub").empty());
  EXPECT_TRUE(
      checkDeclaredNets(in, out, {"top", "sub"}, /*paths_allowed_in=*/{})
          .empty());
  EXPECT_TRUE(checkInstances(in, out, {"top", "sub"}).empty());
  EXPECT_TRUE(checkAssigns(in, out, Path::kHier, {"top", "sub"}).empty());
  EXPECT_TRUE(checkCellCensus(in, out, "top").empty());
}

// A bus that comes back with a different declared range, or exploded into
// escaped scalars, is not a shape change the suite tolerates.
TEST(TestHierStructuralDetector, ReportsBusShapeChanges)
{
  const FileView in = viewOf(
      "module top (i, z); input [3:0] i; output [3:0] z;"
      " wire [3:0] n; endmodule");
  const FileView narrowed = viewOf(
      "module top (i, z); input [3:0] i; output [3:0] z;"
      " wire [1:0] n; endmodule");
  const FileView exploded = viewOf(
      "module top (i, z); input [3:0] i; output [3:0] z;"
      " wire \\n[0] ; wire \\n[1] ; wire \\n[2] ; wire \\n[3] ;"
      " endmodule");
  EXPECT_FALSE(checkDeclaredNets(in, narrowed, {"top"}, /*paths_allowed_in=*/{})
                   .empty());
  EXPECT_FALSE(checkDeclaredNets(in, exploded, {"top"}, /*paths_allowed_in=*/{})
                   .empty());
}

}  // namespace
}  // namespace tst
