// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The ITF reader in two passes. The syntax pass turns the file into a tree
// of statements without knowing any keyword: a statement is a keyword
// with a value (KEY = value), a block (KEY words { ... }) holding
// statements, numbers and (a, b) tuples, or nothing (a flag). The semantic
// pass then takes the keywords it models out of each block, skips the ones
// it does not need, refuses the ones it cannot model yet, and skips
// whatever is left with a warning naming it and its line.

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
  int line = 0;
};

// Heights closer than this (µm) are treated as equal.
constexpr double kHeightTolerance = 1e-9;
// The characters that are tokens on their own.
constexpr std::string_view kPunctuation = "{}=()";
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

// Whether a word is data rather than a keyword: keywords are letters.
bool looksNumeric(const std::string_view text)
{
  if (text.empty()) {
    return false;
  }
  const char c = text.front();
  return std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == '-'
         || c == '+';
}

// Splits the file into words and the punctuation { } = ( ). A ',' is a
// separator like a space, and a '$' starts a comment that runs to the end
// of the line (layer names cannot contain '$').
std::vector<Token> tokenize(std::istream& in)
{
  // A separator between tokens; a word runs up to the next one or the
  // next punctuation. The newline never appears: getline consumes it.
  auto separator = [](const char c) {
    return std::isspace(static_cast<unsigned char>(c)) || c == ',';
  };
  auto punctuation = [](const char c) {
    return kPunctuation.find(c) != std::string_view::npos;
  };
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
      if (separator(c)) {
        i++;
      } else if (punctuation(c)) {
        tokens.push_back({std::string(1, c), line});
        i++;
      } else {
        size_t end = i;
        while (end < text.size() && !separator(text[end])
               && !punctuation(text[end])) {
          end++;
        }
        tokens.push_back({text.substr(i, end - i), line});
        i = end;
      }
    }
  }
  return tokens;
}

// The keywords the semantic pass refuses or skips inside a layer block.
const std::vector<const char*> kRefusedDielectric
    = {"ER_VS_SI_SPACING", "SPACER_ER_VS_WIDTH_AND_SPACING"};
// Effects on the geometry or the dielectrics the solver does not model.
const std::vector<const char*> kRefusedConductor
    = {"AIR_GAP_VS_SPACING",
       "DIELECTRIC_FILL_VS_SI_SPACING",
       "BOTTOM_DIELECTRIC_THICKNESS",
       "BOTTOM_DIELECTRIC_ER",
       "BOTTOM_THICKNESS_VS_SI_WIDTH",
       "DENSITY_BOX_WEIGHTING_FACTOR",
       "DROP_FACTOR",
       "FILL_RATIO",
       "FILL_WIDTH",
       "FILL_SPACING",
       "FILL_TYPE",
       "ILD_VS_WIDTH_AND_SPACING",
       "POLYNOMIAL_BASED_THICKNESS_VARIATION",
       "RHO_VS_SI_WIDTH_AND_THICKNESS",
       "SIDE_TANGENT",
       "THICKNESS_VS_DENSITY",
       "THICKNESS_VS_WIDTH_AND_SPACING",
       "TVF_FILE",
       "TVF_ADJUSTMENT_TABLES"};
// Device-level modeling, which gate-level extraction never meets.
const std::vector<const char*> kSkippedConductor
    = {"DEVICE_TYPE",
       "GATE_TO_CONTACT_SMIN",
       "GATE_TO_DIFFUSION_CAP",
       "FLOATING_GATE_TO_DIFFUSION_CONTACT_CAP",
       "IS_PLANAR",
       "IS_DIFF",
       "RAISED_DIFFUSION_ETCH",
       "RAISED_DIFFUSION_THICKNESS",
       "RAISED_DIFFUSION_TO_GATE_SMIN",
       "RAISED_DIFFUSION_GATE_SIDE_CONFORMAL_ER"};
// Contact and trench-contact forms, device-level.
const std::vector<const char*> kSkippedVia
    = {"ETCH_VS_CONTACT_AND_GATE_SPACINGS",
       "ETCH_VS_WIDTH_AND_LENGTH",
       "RPV_VS_COVERAGE",
       "RPV_VS_WIDTH_AND_LENGTH",
       "RPV_VS_SI_WIDTH_AND_LENGTH",
       "ETCH_ASSOCIATED_LAYER"};

// Every keyword the reader handles, whether it models, refuses or skips
// it. Each starts a statement of its own, so a group's options never
// hold one: an unknown flag before it ends there rather than taking it.
const std::set<std::string_view>& statementKeywords()
{
  static const std::set<std::string_view> keywords = [] {
    std::set<std::string_view> all = {
        "TECHNOLOGY",
        "GLOBAL_TEMPERATURE",
        "BACKGROUND_ER",
        "HALF_NODE_SCALE_FACTOR",
        "REFERENCE_DIRECTION",
        "USE_SI_DENSITY",
        "DROP_FACTOR_LATERAL_SPACING",
        "DIELECTRIC",
        "CONDUCTOR",
        "VIA",
        "TSV",
        "MULTIGATE",
        "VARIATION_PARAMETERS",
        "EM_INFORMATION",
        "ER",
        "THICKNESS",
        "MEASURED_FROM",
        "IS_CONFORMAL",
        "SW_T",
        "TW_T",
        "ASSOCIATED_CONDUCTOR",
        "WMIN",
        "SMIN",
        "LAYER_TYPE",
        "ETCH",
        "CAPACITIVE_ONLY_ETCH",
        "RESISTIVE_ONLY_ETCH",
        "ETCH_VS_WIDTH_AND_SPACING",
        "RPSQ",
        "RHO",
        "RPSQ_VS_SI_WIDTH",
        "RPSQ_VS_WIDTH_AND_SPACING",
        "RHO_VS_WIDTH_AND_SPACING",
        "T0",
        "CRT1",
        "CRT2",
        "CRT_VS_SI_WIDTH",
        "FROM",
        "TO",
        "RPV",
        "AREA",
        "RPV_VS_AREA",
        "CRT_VS_AREA",
        "SPACINGS",
        "WIDTHS",
        "VALUES",
    };
    for (const std::vector<const char*>* list : {&kRefusedDielectric,
                                                 &kRefusedConductor,
                                                 &kSkippedConductor,
                                                 &kSkippedVia}) {
      all.insert(list->begin(), list->end());
    }
    return all;
  }();
  return keywords;
}

// ---------------------------------------------------------------------------
// The syntax pass.

// One statement: KEY = value, a bare KEY, KEY (a, b) ..., or a group. A
// group is named by its keyword; its options are what follows the name
// before the '{': bare words, NAME = value settings and (a, b) tuples, so
// in GROUP A=1 B (1, 2) { ... } the options are A=1, B and (1, 2). Which words
// are a layer's name (DIELECTRIC D) and which a table's modifiers
// (ETCH_VS_WIDTH_AND_SPACING RESISTIVE_ONLY), the semantic pass knows. A group
// may instead give its name a value, GROUP = value { ... }, so that siblings of
// one name can sit side by side; KEY = { ... } is a group with no options.
struct Node
{
  std::string keyword;
  int line = 0;
  std::vector<Token> words;
  std::vector<std::pair<Token, Token>> settings;
  std::vector<std::vector<Token>> tuple_options;
  // KEY = value, alone or giving a group its value.
  bool has_value = false;
  Token value;
  bool has_block = false;
  std::vector<Node> children;
  std::vector<Token> numbers;              // bare words in the block
  std::vector<std::vector<Token>> tuples;  // ( ... ) groups in the block

  // The statement's name for messages: the keyword with its first word,
  // or with its value for a group given one.
  std::string label() const
  {
    if (has_value && has_block) {
      return keyword + " = " + value.text;
    }
    return words.empty() ? keyword : keyword + " " + words.front().text;
  }

  bool hasHeader() const
  {
    return !words.empty() || !settings.empty() || !tuple_options.empty();
  }
};

class SyntaxParser
{
 public:
  explicit SyntaxParser(std::vector<Token> tokens) : tokens_(std::move(tokens))
  {
  }

  std::vector<Node> parseFile();

 private:
  bool atEnd() const { return pos_ >= tokens_.size(); }
  const Token& peek() const
  {
    if (atEnd()) {
      fail(tokens_.empty() ? 1 : tokens_.back().line, "unexpected end of file");
    }
    return tokens_[pos_];
  }
  Token next()
  {
    const Token t = peek();
    pos_++;
    return t;
  }
  bool peekIs(const std::string_view text) const
  {
    return !atEnd() && tokens_[pos_].text == text;
  }
  bool isPunctuation(const Token& t) const
  {
    return t.text.size() == 1
           && kPunctuation.find(t.text[0]) != std::string_view::npos;
  }
  Token nextWord(const std::string_view what)
  {
    const Token t = next();
    if (isPunctuation(t)) {
      fail(t.line, "expected " + std::string(what) + ", got '" + t.text + "'");
    }
    return t;
  }
  void expect(const std::string_view text)
  {
    const Token t = next();
    if (t.text != text) {
      fail(t.line,
           "expected '" + std::string(text) + "', got '" + t.text + "'");
    }
  }

  Node parseStatement();
  void parseBlock(Node& node);
  void skipEmSection(int line);

  std::vector<Token> tokens_;
  size_t pos_ = 0;
};

std::vector<Node> SyntaxParser::parseFile()
{
  std::vector<Node> statements;
  while (!atEnd()) {
    if (peekIs("EM_INFORMATION")) {
      // The one span the braces do not delimit.
      skipEmSection(next().line);
      continue;
    }
    statements.push_back(parseStatement());
  }
  return statements;
}

void SyntaxParser::skipEmSection(const int line)
{
  while (!atEnd()) {
    if (next().text == "END_OF_EM_INFORMATION") {
      return;
    }
  }
  fail(line, "EM_INFORMATION without END_OF_EM_INFORMATION");
}

Node SyntaxParser::parseStatement()
{
  Node node;
  const Token keyword = nextWord("a keyword");
  node.keyword = keyword.text;
  node.line = keyword.line;
  if (peekIs("=")) {
    next();
    if (peekIs("{")) {
      parseBlock(node);
      return node;
    }
    node.has_value = true;
    node.value = nextWord("a value for " + node.keyword);
    if (peekIs("{")) {
      parseBlock(node);  // GROUP = value { ... }
    }
    return node;
  }
  // The options up to a '{' make the statement a group; without the '{'
  // the keyword is a bare flag and what follows it are the next
  // statements. Where an unknown flag ends and the next statement begins
  // is decided by these rules:
  // - A keyword the reader handles (models, refuses or skips) ends the
  //   options, so its own statement is read as usual. The first word
  //   after a handled keyword is exempt, since it may be a layer named
  //   like a keyword, as in CONDUCTOR WMIN { ... }.
  // - KEY = value directly before the '{' is always an option: no
  //   keyword the reader handles takes a group after its value, so the
  //   pair cannot be a statement of its own.
  // - Any other setting is an option only on the keyword's own line;
  //   words and tuples may continue on later lines, as a table's
  //   modifiers often do.
  // Without a '{', tuples after the keyword are its own: KEY (a, b) ...
  // The few flags the format has never open a group.
  static const std::set<std::string_view> kFlags
      = {"IS_CONFORMAL", "IS_PLANAR", "IS_DIFF"};
  auto word = [&](const size_t k) {
    return k < tokens_.size() && !isPunctuation(tokens_[k]);
  };
  std::vector<Token> words;
  std::vector<std::pair<Token, Token>> settings;
  const bool handled = statementKeywords().count(node.keyword) > 0;
  auto ends_options = [&](const Token& t) {
    const bool name = handled && words.empty() && settings.empty();
    return !name && statementKeywords().count(t.text) > 0;
  };
  size_t j = pos_;
  // The end of a ( ... ) of words starting at k, or 0 when there is none.
  auto tuple_end = [&](const size_t k) -> size_t {
    if (k >= tokens_.size() || tokens_[k].text != "(") {
      return 0;
    }
    size_t e = k + 1;
    while (word(e)) {
      e++;
    }
    return e < tokens_.size() && tokens_[e].text == ")" ? e + 1 : 0;
  };
  std::vector<std::vector<Token>> tuple_options;
  while (!kFlags.count(node.keyword) && (word(j) || tuple_end(j) > 0)) {
    if (const size_t e = tuple_end(j)) {
      tuple_options.emplace_back(tokens_.begin() + j + 1,
                                 tokens_.begin() + e - 1);
      j = e;
      continue;
    }
    const Token& t = tokens_[j];
    const bool ends_here = ends_options(t);
    if (j + 1 < tokens_.size() && tokens_[j + 1].text == "=") {
      if (!word(j + 2)) {
        break;
      }
      const bool before_group
          = j + 3 < tokens_.size() && tokens_[j + 3].text == "{";
      if (!before_group && (ends_here || t.line != keyword.line)) {
        break;
      }
      settings.emplace_back(t, tokens_[j + 2]);
      j += 3;
    } else {
      if (ends_here) {
        break;
      }
      words.push_back(tokens_[j++]);
    }
  }
  if (j < tokens_.size() && tokens_[j].text == "{") {
    node.words = std::move(words);
    node.settings = std::move(settings);
    node.tuple_options = std::move(tuple_options);
    pos_ = j;
    parseBlock(node);
  } else {
    // KEY (a, b) ..., as in MASKS(1, 2).
    while (peekIs("(")) {
      next();
      std::vector<Token> tuple;
      while (!peekIs(")")) {
        tuple.push_back(nextWord("a value in " + node.keyword + "( )"));
      }
      next();
      node.tuples.push_back(std::move(tuple));
    }
  }
  return node;
}

void SyntaxParser::parseBlock(Node& node)
{
  expect("{");
  node.has_block = true;
  while (!peekIs("}")) {
    const Token& t = peek();
    if (t.text == "(") {
      next();
      std::vector<Token> tuple;
      while (!peekIs(")")) {
        tuple.push_back(nextWord("a value in ( )"));
      }
      next();
      node.tuples.push_back(std::move(tuple));
    } else if (isPunctuation(t)) {
      fail(t.line, "unexpected '" + t.text + "' in " + node.label());
    } else if (looksNumeric(t.text)) {
      node.numbers.push_back(next());
    } else {
      node.children.push_back(parseStatement());
    }
  }
  expect("}");
}

// ---------------------------------------------------------------------------
// The semantic pass.

// Where skipped keywords are reported.
using Warn = std::function<void(int line, const std::string& message)>;

// Skips an unknown statement with a warning; a group goes whole.
void skipUnknown(const Node& n, const std::string& where, const Warn& warn)
{
  warn(n.line, "unknown keyword " + n.label() + where + ", skipped");
}

// A block's statements, handed out by keyword and checked off, so that
// whatever nobody asked for is reported as skipped.
class Fields
{
 public:
  // Throws for a block holding bare numbers or tuples among its
  // statements.
  Fields(const Node& node, Warn warn)
      : node_(node), warn_(std::move(warn)), taken_(node.children.size())
  {
    if (!node.numbers.empty()) {
      fail(node.numbers.front().line,
           "unexpected number '" + node.numbers.front().text + "' in "
               + node.label());
    }
    if (!node.tuples.empty()) {
      fail(node.tuples.front().empty() ? node.line
                                       : node.tuples.front().front().line,
           "unexpected ( ) in " + node.label());
    }
    for (size_t i = 0; i < node.children.size(); i++) {
      indices_[node.children[i].keyword].push_back(i);
    }
  }

  const Node& node() const { return node_; }
  std::string label() const { return node_.label(); }

  bool has(const std::string_view key) const
  {
    return indices_.find(key) != indices_.end();
  }

  // The statement of a keyword, or nullptr; taken either way. A keyword
  // given twice is an error.
  const Node* take(const std::string_view key)
  {
    const auto it = indices_.find(key);
    if (it == indices_.end()) {
      return nullptr;
    }
    if (it->second.size() > 1) {
      fail(node_.children[it->second[1]].line,
           std::string(key) + " given twice in " + label());
    }
    taken_[it->second.front()] = true;
    return &node_.children[it->second.front()];
  }

  // Every statement of a keyword that may repeat, in file order.
  std::vector<const Node*> takeAll(const std::string_view key)
  {
    std::vector<const Node*> out;
    const auto it = indices_.find(key);
    if (it != indices_.end()) {
      for (const size_t i : it->second) {
        taken_[i] = true;
        out.push_back(&node_.children[i]);
      }
    }
    return out;
  }

  // A KEY = number, or the fallback when absent.
  double number(const std::string_view key, const double fallback)
  {
    const Node* n = take(key);
    if (n == nullptr) {
      return fallback;
    }
    return parseNumber(valueOf(*n), key);
  }

  double required(const std::string_view key)
  {
    if (!has(key)) {
      fail(node_.line, label() + " is missing " + std::string(key));
    }
    return number(key, 0);
  }

  // A KEY = word, or "" when absent.
  std::string text(const std::string_view key)
  {
    const Node* n = take(key);
    return n == nullptr ? std::string() : valueOf(*n).text;
  }

  // A bare keyword.
  bool flag(const std::string_view key)
  {
    const Node* n = take(key);
    if (n == nullptr) {
      return false;
    }
    if (n->has_value || n->has_block || n->hasHeader() || !n->tuples.empty()) {
      fail(n->line, std::string(key) + " takes no value");
    }
    return true;
  }

  int lineOf(const std::string_view key) const
  {
    const auto it = indices_.find(key);
    return it == indices_.end() ? node_.line
                                : node_.children[it->second.front()].line;
  }

  // Keywords the reader does not need: taken without a look.
  void skip(const std::vector<const char*>& keys)
  {
    for (const char* key : keys) {
      takeAll(key);
    }
  }

  // Keywords the reader cannot model yet.
  void refuse(const std::vector<const char*>& keys) const
  {
    for (const char* key : keys) {
      if (has(key)) {
        fail(lineOf(key),
             std::string(key) + " in " + label() + " is not supported");
      }
    }
  }

  // Whatever was neither taken nor skipped is unknown: skipped with one
  // warning per statement, whatever a block among them holds.
  void finish() const
  {
    for (size_t i = 0; i < taken_.size(); i++) {
      if (!taken_[i]) {
        skipUnknown(node_.children[i], " in " + label(), warn_);
      }
    }
  }

  // The value of a KEY = value statement.
  static const Token& valueOf(const Node& n)
  {
    if (n.has_block) {
      fail(n.line, n.keyword + " takes a value, not a block");
    }
    if (!n.has_value) {
      fail(n.line, n.keyword + " needs a value (" + n.keyword + " = ...)");
    }
    return n.value;
  }

 private:
  const Node& node_;
  Warn warn_;
  std::vector<bool> taken_;
  std::map<std::string, std::vector<size_t>, std::less<>> indices_;
};

// The numbers of a block that holds nothing else, as in WIDTHS { 1 2 }.
std::vector<double> numberList(const Node& n)
{
  if (!n.has_block || n.has_value || n.hasHeader() || !n.children.empty()
      || !n.tuples.empty()) {
    fail(n.line, n.keyword + " expects { numbers }");
  }
  std::vector<double> out;
  out.reserve(n.numbers.size());
  for (const Token& t : n.numbers) {
    out.push_back(parseNumber(t, n.keyword));
  }
  return out;
}

// The tuples of a block that holds nothing else, each of `arity` numbers,
// as in RPSQ_VS_SI_WIDTH { (0.1, 0.5) (0.2, 0.4) }.
std::vector<std::vector<double>> tupleList(const Node& n, const size_t arity)
{
  if (!n.has_block || n.has_value || n.hasHeader() || !n.children.empty()
      || !n.numbers.empty() || n.tuples.empty()) {
    fail(
        n.line,
        n.keyword + " expects { (" + std::to_string(arity) + " numbers) ... }");
  }
  std::vector<std::vector<double>> out;
  for (const std::vector<Token>& tuple : n.tuples) {
    if (tuple.size() != arity) {
      fail(
          tuple.empty() ? n.line : tuple.front().line,
          n.keyword + " expects " + std::to_string(arity) + " numbers per ( )");
    }
    std::vector<double> values;
    for (const Token& t : tuple) {
      values.push_back(parseNumber(t, n.keyword));
    }
    out.push_back(std::move(values));
  }
  return out;
}

// An axis of a table: ascending, and positive when it must be.
void requireAscending(const Node& n,
                      const std::string_view what,
                      const std::vector<double>& values,
                      const bool positive)
{
  if (values.empty()) {
    fail(n.line, n.keyword + " has no " + std::string(what));
  }
  for (size_t i = 0; i < values.size(); i++) {
    if (positive && !(values[i] > 0)) {
      fail(n.line, n.keyword + " " + std::string(what) + " must be positive");
    }
    if (i > 0 && !(values[i] > values[i - 1])) {
      fail(n.line, n.keyword + " " + std::string(what) + " must be ascending");
    }
  }
}

// A curve from (x, y) tuples.
Stack::Curve curve(const Node& n, const std::string_view what)
{
  Stack::Curve out;
  for (const std::vector<double>& pair : tupleList(n, 2)) {
    out.x.push_back(pair[0]);
    out.y.push_back(pair[1]);
  }
  requireAscending(n, what, out.x, true);
  return out;
}

// Temperature coefficients from (x, crt1, crt2) tuples.
Stack::CrtCurve crtCurve(const Node& n, const std::string_view what)
{
  Stack::CrtCurve out;
  for (const std::vector<double>& triple : tupleList(n, 3)) {
    out.x.push_back(triple[0]);
    out.crt.push_back({triple[1], triple[2]});
  }
  requireAscending(n, what, out.x, true);
  return out;
}

// A { SPACINGS { } WIDTHS { } VALUES { } } table.
Stack::WidthSpacingTable widthSpacingTable(const Node& n, const Warn& warn)
{
  // Its modifiers are words, which the caller checks; settings and tuples
  // it never takes.
  if (!n.has_block || n.has_value || !n.settings.empty()
      || !n.tuple_options.empty() || !n.numbers.empty() || !n.tuples.empty()) {
    fail(n.line, n.keyword + " expects { SPACINGS WIDTHS VALUES }");
  }
  Fields fields(n, warn);
  Stack::WidthSpacingTable table;
  auto list = [&](const char* key) {
    const Node* child = fields.take(key);
    if (child == nullptr) {
      fail(n.line, n.keyword + " is missing " + key);
    }
    return numberList(*child);
  };
  table.spacings = list("SPACINGS");
  table.widths = list("WIDTHS");
  table.values = list("VALUES");
  fields.finish();
  requireAscending(n, "SPACINGS", table.spacings, true);
  requireAscending(n, "WIDTHS", table.widths, true);
  if (table.values.size() != table.widths.size() * table.spacings.size()) {
    fail(n.line,
         n.keyword + " has " + std::to_string(table.values.size())
             + " values for " + std::to_string(table.widths.size())
             + " widths by " + std::to_string(table.spacings.size())
             + " spacings");
  }
  return table;
}

// A block's leading words, which a table uses as modifiers; each must be
// one of `allowed`, and at most one of each group.
std::set<std::string> modifiers(const Node& n,
                                const std::set<std::string_view>& allowed)
{
  if (n.has_value) {
    fail(n.line, n.keyword + " takes no value");
  }
  if (!n.settings.empty()) {
    const Token& name = n.settings.front().first;
    fail(name.line, "unsupported option " + name.text + " of " + n.keyword);
  }
  if (!n.tuple_options.empty()) {
    fail(n.line, "unsupported option ( ) of " + n.keyword);
  }
  std::set<std::string> out;
  for (const Token& word : n.words) {
    if (!allowed.count(word.text)) {
      fail(word.line, "unsupported option " + word.text + " of " + n.keyword);
    }
    if (!out.insert(word.text).second) {
      fail(word.line, word.text + " given twice in " + n.keyword);
    }
  }
  return out;
}

// A block that names a layer: KIND name { ... }.
const Token& blockName(const Node& n)
{
  if (n.words.size() != 1 || !n.settings.empty() || !n.tuple_options.empty()
      || n.has_value || !n.has_block) {
    fail(n.line, n.keyword + " expects a name and { ... }");
  }
  return n.words.front();
}

class Reader
{
 public:
  explicit Reader(std::vector<ItfWarning>* warnings) : warnings_(warnings) {}

  Stack read(const std::vector<Node>& statements);

 private:
  Warn warn() const
  {
    return [this](const int line, const std::string& message) {
      if (warnings_ != nullptr) {
        warnings_->push_back({line, message});
      }
    };
  }

  // The layers placed by height, before the ground shift.
  struct Placed
  {
    std::vector<Stack::Dielectric> dielectrics;
    std::vector<Stack::Conductor> conductors;
  };
  void readGlobal(const Node& n, Stack& stack);
  Placed place(const std::vector<const Node*>& layers, const Stack& stack);
  void readConductor(Fields& fields,
                     const Stack& stack,
                     Stack::Conductor& conductor);
  void readDielectric(Fields& fields, Placed& placed, double& planar_top);
  void readVia(const Node& n, Stack& stack) const;
  void readEtchTable(const Node& n, Stack::Conductor& conductor);

  std::set<std::string> globals_seen_;
  std::map<std::string, double> tops_by_name_;  // planar dielectric tops
  std::set<std::string> layer_names_;
  std::map<std::string, int> conductor_lines_;
  int directional_etch_line_ = 0;  // a table that needs the direction
  std::vector<ItfWarning>* warnings_;
};

Stack Reader::read(const std::vector<Node>& statements)
{
  Stack stack;
  std::vector<const Node*> layers;  // in file order, top to bottom
  std::vector<const Node*> vias;
  for (const Node& n : statements) {
    const std::string& k = n.keyword;
    if (k == "DIELECTRIC" || k == "CONDUCTOR") {
      layers.push_back(&n);
    } else if (k == "VIA") {
      vias.push_back(&n);
    } else if (k == "TSV" || k == "MULTIGATE" || k == "VARIATION_PARAMETERS") {
      // Through-silicon vias, device models and variation: parsed, not
      // used.
      if (!n.has_block) {
        fail(n.line, k + " expects { ... }");
      }
    } else {
      readGlobal(n, stack);
    }
  }
  if (stack.technology().empty()) {
    failFile("missing TECHNOLOGY");
  }
  Placed placed = place(layers, stack);
  if (placed.dielectrics.empty()) {
    failFile("no planar dielectric layers");
  }
  if (directional_etch_line_ > 0
      && stack.referenceDirection() == Stack::ReferenceDirection::kNone) {
    fail(directional_etch_line_,
         "an etch table names the reference direction, which the file does "
         "not give (REFERENCE_DIRECTION)");
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
  for (const Node* n : vias) {
    readVia(*n, stack);
  }
  return stack;
}

void Reader::readGlobal(const Node& n, Stack& stack)
{
  const std::string& k = n.keyword;
  static const std::set<std::string> kGlobals = {"TECHNOLOGY",
                                                 "GLOBAL_TEMPERATURE",
                                                 "BACKGROUND_ER",
                                                 "HALF_NODE_SCALE_FACTOR",
                                                 "REFERENCE_DIRECTION",
                                                 "USE_SI_DENSITY",
                                                 "DROP_FACTOR_LATERAL_SPACING"};
  if (!kGlobals.count(k)) {
    // Unknown, block and all.
    skipUnknown(n, "", warn());
    return;
  }
  if (!globals_seen_.insert(k).second) {
    fail(n.line, k + " is given twice");
  }
  const Token& value = Fields::valueOf(n);
  if (k == "TECHNOLOGY") {
    stack.setTechnology(value.text);
  } else if (k == "GLOBAL_TEMPERATURE") {
    stack.setGlobalTemperature(parseNumber(value, k));
  } else if (k == "BACKGROUND_ER") {
    const double eps = parseNumber(value, k);
    if (!(eps > 0)) {
      fail(n.line, "BACKGROUND_ER must be positive");
    }
    stack.setBackgroundEps(eps);
  } else if (k == "HALF_NODE_SCALE_FACTOR") {
    const double factor = parseNumber(value, k);
    if (!(factor > 0)) {
      fail(n.line, "HALF_NODE_SCALE_FACTOR must be positive");
    }
    stack.setHalfNodeScaleFactor(factor);
  } else if (k == "REFERENCE_DIRECTION") {
    if (value.text == "VERTICAL") {
      stack.setReferenceDirection(Stack::ReferenceDirection::kVertical);
    } else if (value.text == "HORIZONTAL") {
      stack.setReferenceDirection(Stack::ReferenceDirection::kHorizontal);
    } else {
      fail(n.line, "REFERENCE_DIRECTION must be VERTICAL or HORIZONTAL");
    }
  }
  // USE_SI_DENSITY and DROP_FACTOR_LATERAL_SPACING only qualify effects
  // the reader refuses (density thickness variation, drop factors).
}

// Places the layers bottom-up. A layer's bottom is the top of the planar
// dielectric below it. A planar dielectric's top is the top of its
// MEASURED_FROM layer (default: the layer below) plus THICKNESS. A conductor
// sits on the layer below it and is embedded in the dielectrics above. A
// conformal dielectric wraps its ASSOCIATED_CONDUCTOR or, without one, every
// conductor that rises above the current planar top.
Reader::Placed Reader::place(const std::vector<const Node*>& layers,
                             const Stack& stack)
{
  Placed placed;
  double planar_top = 0;
  for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
    const Node& n = **it;
    const Token& name = blockName(n);
    if (!layer_names_.insert(name.text).second) {
      fail(n.line, "layer " + name.text + " is defined twice");
    }
    Fields fields(n, warn());
    if (n.keyword == "CONDUCTOR") {
      const double thickness = fields.required("THICKNESS");
      if (!(thickness > 0)) {
        fail(fields.lineOf("THICKNESS"),
             "CONDUCTOR " + name.text + " needs a positive THICKNESS");
      }
      conductor_lines_[name.text] = n.line;
      Stack::Conductor conductor;
      conductor.name = name.text;
      conductor.bottom = planar_top;
      conductor.top = planar_top + thickness;
      readConductor(fields, stack, conductor);
      placed.conductors.push_back(std::move(conductor));
    } else {
      readDielectric(fields, placed, planar_top);
    }
    fields.finish();
  }
  return placed;
}

void Reader::readDielectric(Fields& b, Placed& placed, double& planar_top)
{
  const std::string name = blockName(b.node()).text;
  const int line = b.node().line;
  b.refuse(kRefusedDielectric);
  const double eps = b.required("ER");
  if (!(eps > 0)) {
    fail(b.lineOf("ER"), "ER of " + name + " must be positive");
  }
  if (b.flag("IS_CONFORMAL")) {
    if (b.number("THICKNESS", 0) != 0) {
      fail(line,
           "IS_CONFORMAL dielectric " + name
               + " with a non-zero THICKNESS is not supported");
    }
    const bool has_from = b.has("MEASURED_FROM");
    if (has_from && b.text("MEASURED_FROM") != "TOP_OF_CHIP") {
      fail(line,
           "IS_CONFORMAL dielectric " + name
               + " cannot use MEASURED_FROM a layer");
    }
    const Stack::ConformalDielectric conformal{
        .name = name,
        .side_thickness = b.number("SW_T", 0),
        .top_thickness = b.number("TW_T", 0),
        .eps = eps};
    if (!(conformal.side_thickness >= 0 && conformal.top_thickness >= 0)) {
      fail(line, "negative SW_T or TW_T in " + name);
    }
    const bool has_associated = b.has("ASSOCIATED_CONDUCTOR");
    const std::string associated = b.text("ASSOCIATED_CONDUCTOR");
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
      fail(line,
           has_associated ? "ASSOCIATED_CONDUCTOR " + associated + " of " + name
                                + " is not a conductor below it"
                          : "IS_CONFORMAL dielectric " + name
                                + " has no conductor to wrap");
    }
    return;
  }
  if (b.has("SW_T") || b.has("TW_T") || b.has("ASSOCIATED_CONDUCTOR")) {
    fail(line,
         "SW_T, TW_T and ASSOCIATED_CONDUCTOR need IS_CONFORMAL in " + name);
  }
  double base = planar_top;
  if (b.has("MEASURED_FROM")) {
    const int from_line = b.lineOf("MEASURED_FROM");
    const std::string from = b.text("MEASURED_FROM");
    if (from == "TOP_OF_CHIP") {
      fail(from_line,
           "MEASURED_FROM = TOP_OF_CHIP without IS_CONFORMAL is not "
           "supported ("
               + name + ")");
    }
    const auto ref = tops_by_name_.find(from);
    if (ref == tops_by_name_.end()) {
      fail(from_line,
           "MEASURED_FROM " + from + " of " + name
               + " is not a planar dielectric below it");
    }
    base = ref->second;
  }
  const double top = base + b.required("THICKNESS");
  if (!(top > planar_top)) {
    fail(line,
         "dielectric " + name + " has no thickness above the layer below it");
  }
  placed.dielectrics.push_back({name, planar_top, top, eps});
  tops_by_name_[name] = top;
  planar_top = top;
}

void Reader::readConductor(Fields& b, const Stack& stack, Stack::Conductor& c)
{
  const int line = b.node().line;
  b.refuse(kRefusedConductor);
  b.skip(kSkippedConductor);
  c.min_width = b.number("WMIN", 0);
  c.min_spacing = b.number("SMIN", 0);
  c.layer_type = b.text("LAYER_TYPE");

  // Etch.
  const bool has_etch = b.has("ETCH");
  const double etch = b.number("ETCH", 0);
  const bool has_cap = b.has("CAPACITIVE_ONLY_ETCH");
  const bool has_res = b.has("RESISTIVE_ONLY_ETCH");
  if (has_etch && (has_cap || has_res)) {
    fail(b.lineOf("ETCH"),
         "ETCH of " + c.name
             + " cannot be combined with CAPACITIVE_ONLY_ETCH or "
               "RESISTIVE_ONLY_ETCH");
  }
  c.capacitive_etch = has_etch ? etch : b.number("CAPACITIVE_ONLY_ETCH", 0);
  c.resistive_etch = has_etch ? etch : b.number("RESISTIVE_ONLY_ETCH", 0);
  // One table per etch step, applied in order.
  for (const Node* table : b.takeAll("ETCH_VS_WIDTH_AND_SPACING")) {
    readEtchTable(*table, c);
  }

  // Resistance, in one form.
  int forms = 0;
  if (b.has("RPSQ")) {
    c.rpsq = b.number("RPSQ", 0);
    forms++;
  }
  if (b.has("RHO")) {
    c.rho = b.number("RHO", 0);
    forms++;
  }
  if (const Node* n = b.take("RPSQ_VS_SI_WIDTH")) {
    c.rpsq_vs_width = curve(*n, "widths");
    forms++;
  }
  if (const Node* n = b.take("RPSQ_VS_WIDTH_AND_SPACING")) {
    modifiers(*n, {});
    c.rpsq_vs_width_spacing = widthSpacingTable(*n, warn());
    forms++;
  }
  if (const Node* n = b.take("RHO_VS_WIDTH_AND_SPACING")) {
    modifiers(*n, {});
    c.rho_vs_width_spacing = widthSpacingTable(*n, warn());
    forms++;
  }
  if (forms > 1) {
    fail(line,
         "CONDUCTOR " + c.name
             + " gives its resistance in more than one form (RPSQ, RHO, "
               "RPSQ_VS_SI_WIDTH, RPSQ_VS_WIDTH_AND_SPACING, "
               "RHO_VS_WIDTH_AND_SPACING)");
  }
  bool negative = c.rpsq < 0 || c.rho < 0;
  for (const std::vector<double>* values : {&c.rpsq_vs_width.y,
                                            &c.rpsq_vs_width_spacing.values,
                                            &c.rho_vs_width_spacing.values}) {
    for (const double v : *values) {
      negative = negative || v < 0;
    }
  }
  if (negative) {
    fail(line, "CONDUCTOR " + c.name + " has a negative resistance");
  }

  // Temperature.
  c.t0 = b.number("T0", stack.globalTemperature());
  c.crt.crt1 = b.number("CRT1", 0);
  c.crt.crt2 = b.number("CRT2", 0);
  if (const Node* n = b.take("CRT_VS_SI_WIDTH")) {
    if (b.has("CRT1") || b.has("CRT2")) {
      fail(n->line,
           "CRT_VS_SI_WIDTH of " + c.name + " cannot be combined with CRT1 "
                                             "or CRT2");
    }
    c.crt_vs_width = crtCurve(*n, "widths");
  }
}

void Reader::readEtchTable(const Node& n, Stack::Conductor& c)
{
  const std::set<std::string> options = modifiers(n,
                                                  {"RESISTIVE_ONLY",
                                                   "CAPACITIVE_ONLY",
                                                   "ETCH_FROM_TOP",
                                                   "PARALLEL_TO_REFERENCE",
                                                   "PERPENDICULAR_TO_REFERENCE",
                                                   "PARALLEL_TO_GATE"});
  if (options.count("ETCH_FROM_TOP")) {
    fail(n.line,
         "ETCH_FROM_TOP (a trapezoidal cross section) of " + c.name
             + " is not supported");
  }
  if (options.count("RESISTIVE_ONLY") && options.count("CAPACITIVE_ONLY")) {
    fail(n.line, "ETCH_VS_WIDTH_AND_SPACING of " + c.name + " is both "
                                                             "RESISTIVE_ONLY "
                                                             "and "
                                                             "CAPACITIVE_ONLY");
  }
  if (options.count("PARALLEL_TO_REFERENCE")
          + options.count("PERPENDICULAR_TO_REFERENCE")
          + options.count("PARALLEL_TO_GATE")
      > 1) {
    fail(n.line,
         "ETCH_VS_WIDTH_AND_SPACING of " + c.name
             + " names more than one direction");
  }
  // A multiple-mask table (NUMBER_OF_MASKS, MASKS) is a different shape.
  for (const Node& child : n.children) {
    if (child.keyword == "NUMBER_OF_MASKS" || child.keyword == "MASKS") {
      fail(child.line,
           "multiple-mask etch tables (" + child.keyword + ") of " + c.name
               + " are not supported");
    }
  }
  if (options.count("PARALLEL_TO_GATE")) {
    // Device-level: the edges beside a gate.
    widthSpacingTable(n, warn());
    return;
  }
  Stack::EtchTable table;
  table.table = widthSpacingTable(n, warn());
  if (options.count("RESISTIVE_ONLY")) {
    table.affects = Stack::EtchTable::Affects::kResistance;
  } else if (options.count("CAPACITIVE_ONLY")) {
    table.affects = Stack::EtchTable::Affects::kCapacitance;
  }
  if (options.count("PARALLEL_TO_REFERENCE")) {
    table.direction = Stack::EtchTable::Direction::kParallelToReference;
  } else if (options.count("PERPENDICULAR_TO_REFERENCE")) {
    table.direction = Stack::EtchTable::Direction::kPerpendicularToReference;
  }
  if (table.direction != Stack::EtchTable::Direction::kAll) {
    directional_etch_line_ = n.line;
  }
  c.etch_tables.push_back(std::move(table));
}

// Resolves each via's layers. FROM and TO may come in either order, so the
// upper layer is decided by height; SUBSTRATE means the ground plane.
void Reader::readVia(const Node& n, Stack& stack) const
{
  const std::string name = blockName(n).text;
  if (stack.isVia(stack.layer(name))) {
    fail(n.line, "VIA " + name + " is defined twice");
  }
  if (stack.layer(name).valid()) {
    fail(n.line, "VIA " + name + " has the name of a conductor");
  }
  Fields b(n, warn());
  b.skip(kSkippedVia);
  if (!b.has("FROM") || !b.has("TO")) {
    fail(n.line, "VIA " + name + " needs FROM and TO");
  }
  const int from_line = b.lineOf("FROM");
  const int to_line = b.lineOf("TO");
  const std::string from = b.text("FROM");
  const std::string to = b.text("TO");
  Stack::Via via;
  via.name = name;
  via.rpv = b.number("RPV", 0);
  via.area = b.number("AREA", 0);
  via.rho = b.number("RHO", 0);
  if (const Node* curve_node = b.take("RPV_VS_AREA")) {
    via.rpv_vs_area = curve(*curve_node, "areas");
  }
  const int forms = (b.has("RPV") || b.has("AREA")) + b.has("RHO")
                    + !via.rpv_vs_area.empty();
  if (forms > 1) {
    fail(n.line,
         "VIA " + name
             + " gives its resistance in more than one form (RPV with AREA, "
               "RHO, RPV_VS_AREA)");
  }
  if (b.has("RPV") != b.has("AREA")) {
    fail(n.line, "VIA " + name + " needs both RPV and AREA");
  }
  if (via.rpv < 0 || via.rho < 0 || (b.has("AREA") && !(via.area > 0))) {
    fail(n.line,
         "VIA " + name + " needs a non-negative resistance and a positive "
                         "area");
  }
  for (const double rpv : via.rpv_vs_area.y) {
    if (rpv < 0) {
      fail(n.line, "VIA " + name + " has a negative resistance");
    }
  }
  via.t0 = b.number("T0", stack.globalTemperature());
  via.crt.crt1 = b.number("CRT1", 0);
  via.crt.crt2 = b.number("CRT2", 0);
  if (const Node* crt_node = b.take("CRT_VS_AREA")) {
    if (b.has("CRT1") || b.has("CRT2")) {
      fail(crt_node->line,
           "CRT_VS_AREA of " + name + " cannot be combined with CRT1 or CRT2");
    }
    via.crt_vs_area = crtCurve(*crt_node, "areas");
  }
  b.finish();

  const Stack::Layer a = stack.layer(from);
  const Stack::Layer c = stack.layer(to);
  if (from != kSubstrate && !stack.isConductor(a)) {
    fail(from_line, "VIA " + name + " names unknown layer " + from);
  }
  if (to != kSubstrate && !stack.isConductor(c)) {
    fail(to_line, "VIA " + name + " names unknown layer " + to);
  }
  if (from == to) {
    fail(n.line, "VIA " + name + " connects " + from + " to itself");
  }
  if (from == kSubstrate || to == kSubstrate) {
    via.upper = from == kSubstrate ? c : a;
    stack.addVia(via);
    return;
  }
  const Stack::Conductor& from_conductor = stack.conductor(a);
  const Stack::Conductor& to_conductor = stack.conductor(c);
  if (from_conductor.bottom >= to_conductor.top - kHeightTolerance) {
    via.upper = a;
    via.lower = c;
  } else if (to_conductor.bottom >= from_conductor.top - kHeightTolerance) {
    via.upper = c;
    via.lower = a;
  } else {
    fail(n.line,
         "VIA " + name + " connects " + from + " and " + to
             + ", which overlap in height");
  }
  stack.addVia(via);
}

}  // namespace

Stack readItf(std::istream& in, std::vector<ItfWarning>* warnings)
{
  try {
    Stack stack = Reader(warnings).read(SyntaxParser(tokenize(in)).parseFile());
    if (warnings != nullptr) {
      // The layers are read bottom-up; the warnings read better in file
      // order.
      std::stable_sort(warnings->begin(),
                       warnings->end(),
                       [](const ItfWarning& a, const ItfWarning& b) {
                         return a.line < b.line;
                       });
    }
    return stack;
  } catch (const std::invalid_argument& e) {
    // A layer the stack refuses that the checks above did not catch.
    throw std::runtime_error(std::string("ITF: ") + e.what());
  }
}

Stack readItfFile(const std::string& path, std::vector<ItfWarning>* warnings)
{
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("ITF: cannot open " + path);
  }
  return readItf(in, warnings);
}

}  // namespace fsv
