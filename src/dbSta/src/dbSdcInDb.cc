// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "dbSdcInDb.hh"

#include <tcl.h>
#include <unistd.h>

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "sta/Clock.hh"
#include "sta/ClockGroups.hh"
#include "sta/ClockInsertion.hh"
#include "sta/ClockLatency.hh"
#include "sta/ExceptionPath.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/PortDelay.hh"
#include "sta/RiseFallMinMax.hh"
#include "sta/Sdc.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace sta {

namespace {

constexpr std::string_view kNativeHeader = "sdc-in-odb 1";

////////////////////////////////////////////////////////////////
// Shared encoding helpers.

// Names (clocks, path groups) are the only free text in the native form.
// Percent-encode the characters that would upset a whitespace tokenizer,
// and spell the empty string as a lone '%' so every field stays one token.
std::string encodeString(std::string_view s)
{
  if (s.empty()) {
    return "%";
  }
  std::string out;
  out.reserve(s.size());
  for (const unsigned char c : s) {
    if (c == '%' || c <= ' ' || c == 0x7f) {
      char buf[4];
      std::snprintf(buf, sizeof(buf), "%%%02X", c);
      out += buf;
    } else {
      out += static_cast<char>(c);
    }
  }
  return out;
}

std::string decodeString(std::string_view s)
{
  if (s == "%") {
    return {};
  }
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '%' && i + 2 < s.size()) {
      out += static_cast<char>(
          std::strtol(std::string(s.substr(i + 1, 2)).c_str(), nullptr, 16));
      i += 2;
    } else {
      out += s[i];
    }
  }
  return out;
}

// Floats travel as hex-float so the restored value is bit-identical to
// what the Sdc held; the .sdc text rounds to a few digits, this does not.
std::string encodeFloat(float value)
{
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%a", static_cast<double>(value));
  return buf;
}

char encodeRiseFallBoth(const RiseFallBoth* rf)
{
  if (rf == RiseFallBoth::rise()) {
    return 'r';
  }
  if (rf == RiseFallBoth::fall()) {
    return 'f';
  }
  return 'b';
}

const RiseFallBoth* decodeRiseFallBoth(char c)
{
  switch (c) {
    case 'r':
      return RiseFallBoth::rise();
    case 'f':
      return RiseFallBoth::fall();
    default:
      return RiseFallBoth::riseFall();
  }
}

char encodeMinMaxAll(const MinMaxAll* mm)
{
  if (mm == MinMaxAll::min()) {
    return 'n';
  }
  if (mm == MinMaxAll::max()) {
    return 'x';
  }
  return 'a';
}

const MinMaxAll* decodeMinMaxAll(char c)
{
  switch (c) {
    case 'n':
      return MinMaxAll::min();
    case 'x':
      return MinMaxAll::max();
    default:
      return MinMaxAll::all();
  }
}

////////////////////////////////////////////////////////////////
// The completeness oracle.
//
// write_sdc is the one place that enumerates everything an Sdc can hold,
// and its output is what read_sdc would reproduce. So the question "does
// the native form cover this Sdc?" is answered by scanning the command
// vocabulary of that text: every logical line must be a command the
// native encoder handles, in a form it handles. Anything else -- a
// generated clock, a derate, a disable, a comment -- sends the whole Sdc
// to the text fallback. Nothing is ever dropped.

std::vector<std::string> logicalLines(const std::string& text)
{
  std::vector<std::string> lines;
  std::string current;
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\\') {
      line.pop_back();
      current += line;
      current += ' ';
      continue;
    }
    current += line;
    lines.push_back(current);
    current.clear();
  }
  if (!current.empty()) {
    lines.push_back(current);
  }
  return lines;
}

std::vector<std::string_view> tokenize(std::string_view line)
{
  std::vector<std::string_view> tokens;
  size_t i = 0;
  while (i < line.size()) {
    while (i < line.size()
           && std::isspace(static_cast<unsigned char>(line[i]))) {
      i++;
    }
    const size_t start = i;
    while (i < line.size()
           && !std::isspace(static_cast<unsigned char>(line[i]))) {
      i++;
    }
    if (i > start) {
      tokens.push_back(line.substr(start, i - start));
    }
  }
  return tokens;
}

bool contains(std::string_view line, std::string_view needle)
{
  return line.find(needle) != std::string_view::npos;
}

// Returns true if every command in the write_sdc text is one the native
// encoder covers; otherwise names the first offender.
bool nativeCovers(const std::string& text, std::string& offender)
{
  static const std::set<std::string_view> kPlain = {
      "current_design",
      "create_clock",
      "set_clock_latency",
      "set_clock_transition",
      "set_input_delay",
      "set_output_delay",
      "set_false_path",
      "set_max_delay",
      "set_min_delay",
      "set_multicycle_path",
      "group_path",
      "set_clock_groups",
      "set_case_analysis",
      "set_logic_one",
      "set_logic_zero",
      "set_logic_dc",
  };
  for (const std::string& line : logicalLines(text)) {
    const std::vector<std::string_view> tokens = tokenize(line);
    if (tokens.empty() || tokens[0][0] == '#') {
      continue;
    }
    const std::string_view cmd = tokens[0];
    bool covered = false;
    if (contains(line, " -comment ")) {
      covered = false;
    } else if (kPlain.count(cmd) != 0) {
      covered = true;
    } else if (cmd == "set_propagated_clock") {
      covered = contains(line, "[get_clocks");
    } else if (cmd == "set_clock_uncertainty") {
      // The clock form ends in a bare clock name; the pin form and the
      // inter-clock form are not enumerable from the Sdc.
      covered = !contains(line, "-from") && !contains(line, "[get_");
    } else if (cmd == "set_max_fanout" || cmd == "set_min_fanout"
               || cmd == "set_max_transition" || cmd == "set_max_capacitance"
               || cmd == "set_min_capacitance") {
      covered = contains(line, "[current_design]");
    }
    if (!covered) {
      offender = line;
      return false;
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////
// sta::writeSdc only writes to a file. Round-trip through a temporary one
// until an ostream overload exists upstream in OpenSTA.

class TempSdcFile
{
 public:
  TempSdcFile()
  {
    path_ = std::filesystem::temp_directory_path()
            / ("openroad-sdc-" + std::to_string(::getpid()) + "-"
               + std::to_string(
                   reinterpret_cast<uintptr_t>(static_cast<void*>(this)))
               + ".sdc");
  }
  ~TempSdcFile()
  {
    std::error_code ec;
    std::filesystem::remove(path_, ec);
  }
  TempSdcFile(const TempSdcFile&) = delete;
  TempSdcFile& operator=(const TempSdcFile&) = delete;

  const std::filesystem::path& path() const { return path_; }

  std::string read() const
  {
    std::ifstream in(path_, std::ios::binary);
    if (!in) {
      return {};
    }
    std::ostringstream buf;
    buf << in.rdbuf();
    return buf.str();
  }

 private:
  std::filesystem::path path_;
};

////////////////////////////////////////////////////////////////
// Native encoder.
//
// One record per line, whitespace separated. Pins, instances and nets are
// odb object ids: b<id> dbBTerm, i<id> dbITerm, m<id> dbModITerm;
// instances are dbInst ids, nets dbNet ids. Ids are stable across
// write_db/read_db, so restoring is a table lookup per object.
//
//   C name period add wave... pins...        create_clock
//   P clk                                    set_propagated_clock (clock)
//   U clk s|h value                          set_clock_uncertainty (clock)
//   W clk rf mm value                        set_clock_transition
//   L clk|- pin|- rf mm value                set_clock_latency
//   N clk|- pin|- e|l rf mm value            set_clock_latency -source
//   I pin clk|- r|f|- ref|- sli nli rf mm v  set_input_delay
//   O ...                                    set_output_delay
//   E F mma | X mm ignore break delay | M mma end mult | G default name
//     then f rf pins clks insts / h rf pins nets insts / t rf endrf ...
//   A pin value                              set_case_analysis
//   V pin value                              set_logic_*
//   K name le pe async allow groups...       set_clock_groups
//   R f|s|c mm value                         set_max_fanout etc. on the design

class NativeEncoder
{
 public:
  NativeEncoder(dbSta* sta, odb::dbBlock* block, const Sdc* sdc)
      : sta_(sta), network_(sta->getDbNetwork()), block_(block), sdc_(sdc)
  {
  }

  // Returns the encoding, or an empty string with `offender` set if the
  // Sdc holds something the native form does not represent.
  std::string encode(std::string& offender)
  {
    out_ << kNativeHeader << '\n';
    encodeClocks();
    encodeClockLatencies();
    encodeClockInsertions();
    encodePortDelays(sdc_->inputDelays(), 'I');
    encodePortDelays(sdc_->outputDelays(), 'O');
    encodeExceptions();
    encodeLogicValues(sdc_->caseLogicValues(), 'A');
    encodeLogicValues(sdc_->logicValues(), 'V');
    encodeClockGroups();
    encodeLimits();
    if (!ok_) {
      offender = offender_;
      return {};
    }
    return out_.str();
  }

 private:
  void unsupported(std::string_view what)
  {
    if (ok_) {
      ok_ = false;
      offender_ = what;
    }
  }

  std::string pinRef(const Pin* pin)
  {
    odb::dbObject* obj = network_->staToDb(pin);
    if (obj == nullptr) {
      unsupported("pin without an odb object");
      return "-";
    }
    switch (obj->getObjectType()) {
      case odb::dbBTermObj:
        return "b" + std::to_string(obj->getId());
      case odb::dbITermObj:
        return "i" + std::to_string(obj->getId());
      case odb::dbModITermObj:
        return "m" + std::to_string(obj->getId());
      default:
        unsupported("pin of unsupported odb type");
        return "-";
    }
  }

  std::string instRef(const Instance* inst)
  {
    odb::dbInst* db_inst = nullptr;
    odb::dbModInst* mod_inst = nullptr;
    network_->staToDb(inst, db_inst, mod_inst);
    if (db_inst == nullptr) {
      unsupported("hierarchical instance in an exception");
      return "-";
    }
    return std::to_string(db_inst->getId());
  }

  std::string netRef(const Net* net)
  {
    odb::dbNet* db_net = nullptr;
    odb::dbModNet* mod_net = nullptr;
    network_->staToDb(net, db_net, mod_net);
    if (db_net == nullptr) {
      unsupported("hierarchical net in an exception");
      return "-";
    }
    return std::to_string(db_net->getId());
  }

  void writePins(const PinSet* pins)
  {
    out_ << ' ' << (pins ? pins->size() : 0);
    if (pins) {
      for (const Pin* pin : *pins) {
        out_ << ' ' << pinRef(pin);
      }
    }
  }

  void writeClocks(const ClockSet* clks)
  {
    out_ << ' ' << (clks ? clks->size() : 0);
    if (clks) {
      for (const Clock* clk : *clks) {
        out_ << ' ' << encodeString(clk->name());
      }
    }
  }

  void writeInstances(const InstanceSet* insts)
  {
    out_ << ' ' << (insts ? insts->size() : 0);
    if (insts) {
      for (const Instance* inst : *insts) {
        out_ << ' ' << instRef(inst);
      }
    }
  }

  void writeNets(const NetSet* nets)
  {
    out_ << ' ' << (nets ? nets->size() : 0);
    if (nets) {
      for (const Net* net : *nets) {
        out_ << ' ' << netRef(net);
      }
    }
  }

  // One record per existing rise/fall x min/max value, so the restore
  // side sets exactly the values that were set.
  template <typename WritePrefix>
  void writeRiseFallMinMax(const RiseFallMinMax* values, WritePrefix prefix)
  {
    for (const RiseFall* rf : RiseFall::range()) {
      for (const MinMax* mm : MinMax::range()) {
        float value;
        bool exists;
        values->value(rf, mm, value, exists);
        if (exists) {
          prefix();
          out_ << ' ' << (rf == RiseFall::rise() ? 'r' : 'f') << ' '
               << (mm == MinMax::min() ? 'n' : 'x') << ' ' << encodeFloat(value)
               << '\n';
        }
      }
    }
  }

  void encodeClocks()
  {
    for (Clock* clk : sdc_->clocks()) {
      if (clk->isGenerated()) {
        unsupported("create_generated_clock");
        return;
      }
      if (!clk->comment().empty()) {
        unsupported("create_clock -comment");
        return;
      }
      out_ << "C " << encodeString(clk->name()) << ' '
           << encodeFloat(clk->period()) << ' ' << (clk->addToPins() ? 1 : 0)
           << ' ' << clk->waveform().size();
      for (const float edge : clk->waveform()) {
        out_ << ' ' << encodeFloat(edge);
      }
      writePins(&clk->pins());
      out_ << '\n';

      for (const RiseFall* rf : RiseFall::range()) {
        for (const MinMax* mm : MinMax::range()) {
          float slew;
          bool exists;
          clk->slew(rf, mm, slew, exists);
          if (exists) {
            out_ << "W " << encodeString(clk->name()) << ' '
                 << (rf == RiseFall::rise() ? 'r' : 'f') << ' '
                 << (mm == MinMax::min() ? 'n' : 'x') << ' '
                 << encodeFloat(slew) << '\n';
          }
        }
      }
      for (const MinMax* setup_hold : MinMax::range()) {
        float value;
        bool exists;
        clk->uncertainty(setup_hold, value, exists);
        if (exists) {
          out_ << "U " << encodeString(clk->name()) << ' '
               << (setup_hold == MinMax::max() ? 's' : 'h') << ' '
               << encodeFloat(value) << '\n';
        }
      }
      if (clk->isPropagated()) {
        out_ << "P " << encodeString(clk->name()) << '\n';
      }
    }
  }

  void writeClockPinKey(const Clock* clk, const Pin* pin)
  {
    out_ << ' ' << (clk ? encodeString(clk->name()) : "-") << ' '
         << (pin ? pinRef(pin) : "-");
  }

  void encodeClockLatencies()
  {
    for (ClockLatency* latency : *sdc_->clockLatencies()) {
      writeRiseFallMinMax(latency->delays(), [&] {
        out_ << 'L';
        writeClockPinKey(latency->clock(), latency->pin());
      });
    }
  }

  void encodeClockInsertions()
  {
    for (ClockInsertion* insertion : sdc_->clockInsertions()) {
      for (const EarlyLate* early_late : EarlyLate::range()) {
        writeRiseFallMinMax(insertion->delays(early_late), [&] {
          out_ << 'N';
          writeClockPinKey(insertion->clock(), insertion->pin());
          out_ << ' ' << (early_late == EarlyLate::early() ? 'e' : 'l');
        });
      }
    }
  }

  template <typename PortDelaySet>
  void encodePortDelays(const PortDelaySet& delays, char record)
  {
    for (PortDelay* delay : delays) {
      const ClockEdge* clk_edge = delay->clkEdge();
      writeRiseFallMinMax(delay->delays(), [&] {
        out_ << record << ' ' << pinRef(delay->pin()) << ' ';
        if (clk_edge) {
          out_ << encodeString(clk_edge->clock()->name()) << ' '
               << (clk_edge->transition() == RiseFall::rise() ? 'r' : 'f');
        } else {
          out_ << "- -";
        }
        out_ << ' ' << (delay->refPin() ? pinRef(delay->refPin()) : "-") << ' '
             << (delay->sourceLatencyIncluded() ? 1 : 0) << ' '
             << (delay->networkLatencyIncluded() ? 1 : 0);
      });
    }
  }

  void encodeExceptions()
  {
    for (ExceptionPath* exception : sdc_->exceptions()) {
      if (exception->isFilter() || exception->isLoop()) {
        continue;  // write_sdc skips these too: they are search state.
      }
      if (!exception->comment().empty()) {
        unsupported("exception -comment");
        return;
      }
      out_ << "E ";
      if (exception->isFalse()) {
        out_ << "F " << encodeMinMaxAll(exception->minMax());
      } else if (exception->isPathDelay()) {
        out_ << "X " << encodeMinMaxAll(exception->minMax()) << ' '
             << (exception->ignoreClkLatency() ? 1 : 0) << ' '
             << (exception->breakPath() ? 1 : 0) << ' '
             << encodeFloat(exception->delay());
      } else if (exception->isMultiCycle()) {
        out_ << "M " << encodeMinMaxAll(exception->minMax()) << ' '
             << (exception->useEndClk() ? 1 : 0) << ' '
             << exception->pathMultiplier();
      } else if (exception->isGroupPath()) {
        out_ << "G " << (exception->isDefault() ? 1 : 0) << ' '
             << encodeString(exception->name());
      } else {
        unsupported(exception->typeString());
        return;
      }
      if (ExceptionFrom* from = exception->from()) {
        out_ << " f " << encodeRiseFallBoth(from->transition());
        writePins(from->pins());
        writeClocks(from->clks());
        writeInstances(from->instances());
      }
      if (exception->thrus()) {
        for (ExceptionThru* thru : *exception->thrus()) {
          out_ << " h " << encodeRiseFallBoth(thru->transition());
          writePins(thru->pins());
          writeNets(thru->nets());
          writeInstances(thru->instances());
        }
      }
      if (ExceptionTo* to = exception->to()) {
        out_ << " t " << encodeRiseFallBoth(to->transition()) << ' '
             << encodeRiseFallBoth(to->endTransition());
        writePins(to->pins());
        writeClocks(to->clks());
        writeInstances(to->instances());
      }
      out_ << '\n';
    }
  }

  void encodeLogicValues(const LogicValueMap& values, char record)
  {
    for (const auto& [pin, value] : values) {
      out_ << record << ' ' << pinRef(pin) << ' '
           << static_cast<unsigned>(value) << '\n';
    }
  }

  void encodeClockGroups()
  {
    for (const auto& [name, clk_groups] : sdc_->clockGroupsNameMap()) {
      if (!clk_groups->comment().empty()) {
        unsupported("set_clock_groups -comment");
        return;
      }
      out_ << "K " << encodeString(clk_groups->name()) << ' '
           << (clk_groups->logicallyExclusive() ? 1 : 0) << ' '
           << (clk_groups->physicallyExclusive() ? 1 : 0) << ' '
           << (clk_groups->asynchronous() ? 1 : 0) << ' '
           << (clk_groups->allowPaths() ? 1 : 0) << ' '
           << clk_groups->groups()->size();
      for (ClockGroup* group : *clk_groups->groups()) {
        writeClocks(group);
      }
      out_ << '\n';
    }
  }

  void encodeLimits()
  {
    Cell* top = network_->cell(network_->topInstance());
    for (const MinMax* mm : MinMax::range()) {
      float value;
      bool exists;
      sdc_->fanoutLimit(top, mm, value, exists);
      if (exists) {
        writeLimit('f', mm, value);
      }
      sdc_->slewLimit(top, mm, value, exists);
      if (exists) {
        writeLimit('s', mm, value);
      }
      sdc_->capacitanceLimit(top, mm, value, exists);
      if (exists) {
        writeLimit('c', mm, value);
      }
    }
  }

  void writeLimit(char which, const MinMax* mm, float value)
  {
    out_ << "R " << which << ' ' << (mm == MinMax::min() ? 'n' : 'x') << ' '
         << encodeFloat(value) << '\n';
  }

  dbSta* sta_;
  dbNetwork* network_;
  odb::dbBlock* block_;
  const Sdc* sdc_;
  std::ostringstream out_;
  bool ok_ = true;
  std::string offender_;
};

////////////////////////////////////////////////////////////////
// Native decoder.

class ParseError : public std::exception
{
 public:
  explicit ParseError(std::string what) : what_(std::move(what)) {}
  const char* what() const noexcept override { return what_.c_str(); }

 private:
  std::string what_;
};

class Tokens
{
 public:
  explicit Tokens(std::string_view line) : tokens_(tokenize(line)) {}

  bool done() const { return next_ >= tokens_.size(); }
  std::string_view peek() const
  {
    if (done()) {
      throw ParseError("truncated record");
    }
    return tokens_[next_];
  }
  std::string_view next()
  {
    std::string_view token = peek();
    next_++;
    return token;
  }
  char nextChar() { return next()[0]; }
  int nextInt() { return std::stoi(std::string(next())); }
  size_t nextCount() { return static_cast<size_t>(nextInt()); }
  float nextFloat()
  {
    return static_cast<float>(
        std::strtod(std::string(next()).c_str(), nullptr));
  }
  std::string nextString() { return decodeString(next()); }

 private:
  std::vector<std::string_view> tokens_;
  size_t next_ = 0;
};

class NativeDecoder
{
 public:
  NativeDecoder(dbSta* sta, odb::dbBlock* block)
      : sta_(sta),
        network_(sta->getDbNetwork()),
        block_(block),
        sdc_(sta->cmdSdc()),
        mode_(sta->cmdMode())
  {
  }

  void decode(const std::string& text)
  {
    std::istringstream in(text);
    std::string line;
    if (!std::getline(in, line) || line != kNativeHeader) {
      throw ParseError("unrecognized header");
    }
    while (std::getline(in, line)) {
      if (line.empty()) {
        continue;
      }
      Tokens tokens(line);
      const char record = tokens.nextChar();
      switch (record) {
        case 'C':
          decodeClock(tokens);
          break;
        case 'P':
          sta_->setPropagatedClock(clock(tokens.nextString()), mode_);
          break;
        case 'U': {
          Clock* clk = clock(tokens.nextString());
          const MinMaxAll* setup_hold
              = tokens.nextChar() == 's' ? MinMaxAll::max() : MinMaxAll::min();
          sta_->setClockUncertainty(clk, setup_hold, tokens.nextFloat());
          break;
        }
        case 'W': {
          Clock* clk = clock(tokens.nextString());
          const RiseFallBoth* rf = riseFallBoth(tokens.nextChar());
          const MinMaxAll* mm = minMaxAll(tokens.nextChar());
          sta_->setClockSlew(clk, rf, mm, tokens.nextFloat(), sdc_);
          break;
        }
        case 'L': {
          Clock* clk = optionalClock(tokens.next());
          Pin* pin = optionalPin(tokens.next());
          const RiseFallBoth* rf = riseFallBoth(tokens.nextChar());
          const MinMaxAll* mm = minMaxAll(tokens.nextChar());
          sta_->setClockLatency(clk, pin, rf, mm, tokens.nextFloat(), sdc_);
          break;
        }
        case 'N': {
          Clock* clk = optionalClock(tokens.next());
          Pin* pin = optionalPin(tokens.next());
          const EarlyLateAll* early_late = tokens.nextChar() == 'e'
                                               ? MinMaxAll::early()
                                               : MinMaxAll::late();
          const RiseFallBoth* rf = riseFallBoth(tokens.nextChar());
          const MinMaxAll* mm = minMaxAll(tokens.nextChar());
          sta_->setClockInsertion(
              clk, pin, rf, mm, early_late, tokens.nextFloat(), sdc_);
          break;
        }
        case 'I':
        case 'O':
          decodePortDelay(tokens, record == 'I');
          break;
        case 'E':
          decodeException(tokens);
          break;
        case 'A': {
          Pin* pin = pin_(tokens.next());
          sta_->setCaseAnalysis(
              pin, static_cast<LogicValue>(tokens.nextInt()), mode_);
          break;
        }
        case 'V': {
          Pin* pin = pin_(tokens.next());
          sta_->setLogicValue(
              pin, static_cast<LogicValue>(tokens.nextInt()), mode_);
          break;
        }
        case 'K':
          decodeClockGroups(tokens);
          break;
        case 'R':
          decodeLimit(tokens);
          break;
        default:
          throw ParseError("unknown record type");
      }
      if (!tokens.done()) {
        throw ParseError("trailing tokens in record");
      }
    }
  }

 private:
  Clock* clock(const std::string& name)
  {
    Clock* clk = sdc_->findClock(name);
    if (clk == nullptr) {
      throw ParseError("unknown clock " + name);
    }
    return clk;
  }

  Clock* optionalClock(std::string_view token)
  {
    return token == "-" ? nullptr : clock(decodeString(token));
  }

  Pin* pin_(std::string_view ref)
  {
    if (ref.size() < 2) {
      throw ParseError("malformed pin reference");
    }
    const uint32_t id = static_cast<uint32_t>(
        std::strtoul(std::string(ref.substr(1)).c_str(), nullptr, 10));
    Pin* pin = nullptr;
    switch (ref[0]) {
      case 'b':
        if (odb::dbBTerm* bterm = odb::dbBTerm::getBTerm(block_, id)) {
          pin = network_->dbToSta(bterm);
        }
        break;
      case 'i':
        if (odb::dbITerm* iterm = odb::dbITerm::getITerm(block_, id)) {
          pin = network_->dbToSta(iterm);
        }
        break;
      case 'm':
        if (odb::dbModITerm* moditerm
            = odb::dbModITerm::getModITerm(block_, id)) {
          pin = network_->dbToSta(moditerm);
        }
        break;
      default:
        break;
    }
    if (pin == nullptr) {
      throw ParseError("no pin for reference " + std::string(ref));
    }
    return pin;
  }

  Pin* optionalPin(std::string_view token)
  {
    return token == "-" ? nullptr : pin_(token);
  }

  Instance* instance(std::string_view ref)
  {
    const uint32_t id = static_cast<uint32_t>(
        std::strtoul(std::string(ref).c_str(), nullptr, 10));
    odb::dbInst* inst = odb::dbInst::getInst(block_, id);
    if (inst == nullptr) {
      throw ParseError("no instance for reference " + std::string(ref));
    }
    return network_->dbToSta(inst);
  }

  Net* net(std::string_view ref)
  {
    const uint32_t id = static_cast<uint32_t>(
        std::strtoul(std::string(ref).c_str(), nullptr, 10));
    odb::dbNet* db_net = odb::dbNet::getNet(block_, id);
    if (db_net == nullptr) {
      throw ParseError("no net for reference " + std::string(ref));
    }
    return network_->dbToSta(db_net);
  }

  static const RiseFallBoth* riseFallBoth(char c)
  {
    return decodeRiseFallBoth(c);
  }
  static const MinMaxAll* minMaxAll(char c) { return decodeMinMaxAll(c); }

  // Sets are handed to the Sta makers, which take ownership; an empty
  // set is passed as nullptr, as the Tcl layer does.
  PinSet* readPins(Tokens& tokens)
  {
    const size_t count = tokens.nextCount();
    if (count == 0) {
      return nullptr;
    }
    PinSet* pins = new PinSet(network_);
    for (size_t i = 0; i < count; i++) {
      pins->insert(pin_(tokens.next()));
    }
    return pins;
  }

  ClockSet* readClocks(Tokens& tokens)
  {
    const size_t count = tokens.nextCount();
    if (count == 0) {
      return nullptr;
    }
    ClockSet* clks = new ClockSet;
    for (size_t i = 0; i < count; i++) {
      clks->insert(clock(tokens.nextString()));
    }
    return clks;
  }

  InstanceSet* readInstances(Tokens& tokens)
  {
    const size_t count = tokens.nextCount();
    if (count == 0) {
      return nullptr;
    }
    InstanceSet* insts = new InstanceSet(network_);
    for (size_t i = 0; i < count; i++) {
      insts->insert(instance(tokens.next()));
    }
    return insts;
  }

  NetSet* readNets(Tokens& tokens)
  {
    const size_t count = tokens.nextCount();
    if (count == 0) {
      return nullptr;
    }
    NetSet* nets = new NetSet(network_);
    for (size_t i = 0; i < count; i++) {
      nets->insert(net(tokens.next()));
    }
    return nets;
  }

  void decodeClock(Tokens& tokens)
  {
    const std::string name = tokens.nextString();
    const float period = tokens.nextFloat();
    const bool add_to_pins = tokens.nextInt() != 0;
    FloatSeq waveform;
    const size_t edges = tokens.nextCount();
    for (size_t i = 0; i < edges; i++) {
      waveform.push_back(tokens.nextFloat());
    }
    PinSet pins(network_);
    const size_t count = tokens.nextCount();
    for (size_t i = 0; i < count; i++) {
      pins.insert(pin_(tokens.next()));
    }
    sta_->makeClock(name, pins, add_to_pins, period, waveform, "", mode_);
  }

  void decodePortDelay(Tokens& tokens, bool is_input)
  {
    Pin* pin = pin_(tokens.next());
    Clock* clk = optionalClock(tokens.next());
    const char clk_rf_char = tokens.nextChar();
    const RiseFall* clk_rf = clk_rf_char == 'r'   ? RiseFall::rise()
                             : clk_rf_char == 'f' ? RiseFall::fall()
                                                  : nullptr;
    Pin* ref_pin = optionalPin(tokens.next());
    const bool source_latency_included = tokens.nextInt() != 0;
    const bool network_latency_included = tokens.nextInt() != 0;
    const RiseFallBoth* rf = riseFallBoth(tokens.nextChar());
    const MinMaxAll* mm = minMaxAll(tokens.nextChar());
    const float delay = tokens.nextFloat();
    // write_sdc emits every delay with -add_delay, and read_sdc keeps them
    // all; restoring into a fresh Sdc with add=true does the same.
    if (is_input) {
      sta_->setInputDelay(pin,
                          rf,
                          clk,
                          clk_rf,
                          ref_pin,
                          source_latency_included,
                          network_latency_included,
                          mm,
                          true,
                          delay,
                          sdc_);
    } else {
      sta_->setOutputDelay(pin,
                           rf,
                           clk,
                           clk_rf,
                           ref_pin,
                           source_latency_included,
                           network_latency_included,
                           mm,
                           true,
                           delay,
                           sdc_);
    }
  }

  void decodeException(Tokens& tokens)
  {
    const char kind = tokens.nextChar();
    const MinMaxAll* min_max = MinMaxAll::all();
    bool ignore_clk_latency = false;
    bool break_path = false;
    float delay = 0.0;
    bool use_end_clk = false;
    int path_multiplier = 0;
    bool is_default = false;
    std::string name;
    switch (kind) {
      case 'F':
        min_max = minMaxAll(tokens.nextChar());
        break;
      case 'X':
        min_max = minMaxAll(tokens.nextChar());
        ignore_clk_latency = tokens.nextInt() != 0;
        break_path = tokens.nextInt() != 0;
        delay = tokens.nextFloat();
        break;
      case 'M':
        min_max = minMaxAll(tokens.nextChar());
        use_end_clk = tokens.nextInt() != 0;
        path_multiplier = tokens.nextInt();
        break;
      case 'G':
        is_default = tokens.nextInt() != 0;
        name = tokens.nextString();
        break;
      default:
        throw ParseError("unknown exception kind");
    }

    ExceptionFrom* from = nullptr;
    ExceptionThruSeq* thrus = nullptr;
    ExceptionTo* to = nullptr;
    while (!tokens.done()) {
      const char pt = tokens.nextChar();
      const RiseFallBoth* rf = riseFallBoth(tokens.nextChar());
      if (pt == 'f') {
        PinSet* pins = readPins(tokens);
        ClockSet* clks = readClocks(tokens);
        InstanceSet* insts = readInstances(tokens);
        from = sta_->makeExceptionFrom(pins, clks, insts, rf, sdc_);
      } else if (pt == 'h') {
        PinSet* pins = readPins(tokens);
        NetSet* nets = readNets(tokens);
        InstanceSet* insts = readInstances(tokens);
        if (thrus == nullptr) {
          thrus = new ExceptionThruSeq;
        }
        thrus->push_back(sta_->makeExceptionThru(pins, nets, insts, rf, sdc_));
      } else if (pt == 't') {
        const RiseFallBoth* end_rf = riseFallBoth(tokens.nextChar());
        PinSet* pins = readPins(tokens);
        ClockSet* clks = readClocks(tokens);
        InstanceSet* insts = readInstances(tokens);
        to = sta_->makeExceptionTo(pins, clks, insts, rf, end_rf, sdc_);
      } else {
        throw ParseError("unknown exception point");
      }
    }

    switch (kind) {
      case 'F':
        sta_->makeFalsePath(from, thrus, to, min_max, "", sdc_);
        break;
      case 'X':
        sta_->makePathDelay(
            from,
            thrus,
            to,
            min_max == MinMaxAll::min() ? MinMax::min() : MinMax::max(),
            ignore_clk_latency,
            break_path,
            delay,
            "",
            sdc_);
        break;
      case 'M':
        sta_->makeMulticyclePath(
            from, thrus, to, min_max, use_end_clk, path_multiplier, "", sdc_);
        break;
      case 'G':
        sta_->makeGroupPath(name, is_default, from, thrus, to, "", sdc_);
        break;
    }
  }

  void decodeClockGroups(Tokens& tokens)
  {
    const std::string name = tokens.nextString();
    const bool logically_exclusive = tokens.nextInt() != 0;
    const bool physically_exclusive = tokens.nextInt() != 0;
    const bool asynchronous = tokens.nextInt() != 0;
    const bool allow_paths = tokens.nextInt() != 0;
    ClockGroups* clk_groups = sta_->makeClockGroups(name,
                                                    logically_exclusive,
                                                    physically_exclusive,
                                                    asynchronous,
                                                    allow_paths,
                                                    "",
                                                    sdc_);
    const size_t groups = tokens.nextCount();
    for (size_t i = 0; i < groups; i++) {
      ClockSet* clks = readClocks(tokens);
      if (clks == nullptr) {
        clks = new ClockSet;
      }
      sta_->makeClockGroup(clk_groups, clks, sdc_);
    }
  }

  void decodeLimit(Tokens& tokens)
  {
    const char which = tokens.nextChar();
    const MinMax* mm = tokens.nextChar() == 'n' ? MinMax::min() : MinMax::max();
    const float value = tokens.nextFloat();
    Cell* top = network_->cell(network_->topInstance());
    switch (which) {
      case 'f':
        sta_->setFanoutLimit(top, mm, value, sdc_);
        break;
      case 's':
        sta_->setSlewLimit(top, mm, value, sdc_);
        break;
      case 'c':
        sta_->setCapacitanceLimit(top, mm, value, sdc_);
        break;
      default:
        throw ParseError("unknown design limit");
    }
  }

  dbSta* sta_;
  dbNetwork* network_;
  odb::dbBlock* block_;
  Sdc* sdc_;
  Mode* mode_;
};

void setProperty(odb::dbBlock* block,
                 const char* name,
                 const std::string& value)
{
  if (odb::dbProperty* existing = odb::dbProperty::find(block, name)) {
    odb::dbProperty::destroy(existing);
  }
  if (!value.empty()) {
    odb::dbStringProperty::create(block, name, value.c_str());
  }
}

}  // namespace

////////////////////////////////////////////////////////////////

void SdcInDb::save(dbSta* sta, odb::dbBlock* block)
{
  utl::Logger* logger = sta->getLogger();
  Network* network = sta->getDbNetwork();
  // A pure-odb flow (no liberty, no linked network) has no constraints to
  // save. Skip quietly: write_db must behave exactly as it did before for
  // the flows that never had constraints in the first place.
  if (!network->isLinked() || network->defaultLibertyLibrary() == nullptr) {
    return;
  }
  const Sdc* sdc = sta->cmdSdc();
  if (sdc == nullptr) {
    return;
  }

  // write_sdc enumerates everything the Sdc holds; its text is both the
  // completeness oracle for the native form and the fallback payload.
  std::string text;
  try {
    TempSdcFile temp;
    sta->writeSdc(sdc,
                  temp.path().string(),
                  /* leaf */ false,
                  /* native */ true,
                  /* digits */ 4,
                  /* gzip */ false,
                  /* no_timestamp */ true);
    text = temp.read();
  } catch (const std::exception& e) {
    // Capturing constraints must never break write_db. A flow that cannot
    // write its constraints still gets the same .odb it got before.
    logger->warn(utl::STA,
                 3008,
                 "could not store timing constraints in the database: {}",
                 e.what());
    return;
  }
  if (text.empty()) {
    return;
  }

  std::string native;
  std::string offender;
  if (nativeCovers(text, offender)) {
    NativeEncoder encoder(sta, block, sdc);
    native = encoder.encode(offender);
  }
  if (native.empty()) {
    debugPrint(logger,
               utl::STA,
               "sdc_in_db",
               1,
               "storing constraints as text: native form does not cover: {}",
               offender);
  }
  setProperty(block, kNativeProperty, native);
  setProperty(block, kTextProperty, native.empty() ? text : std::string());
}

SdcInDb::Kind SdcInDb::kind(odb::dbBlock* block)
{
  if (block == nullptr) {
    return Kind::kNone;
  }
  if (odb::dbStringProperty::find(block, kNativeProperty) != nullptr) {
    return Kind::kNative;
  }
  if (odb::dbStringProperty::find(block, kTextProperty) != nullptr) {
    return Kind::kText;
  }
  return Kind::kNone;
}

const char* SdcInDb::kindName(Kind kind)
{
  switch (kind) {
    case Kind::kNative:
      return "native";
    case Kind::kText:
      return "text";
    case Kind::kNone:
      break;
  }
  return "none";
}

SdcInDb::Kind SdcInDb::restore(dbSta* sta, odb::dbBlock* block)
{
  utl::Logger* logger = sta->getLogger();
  const Kind found = kind(block);
  switch (found) {
    case Kind::kNone:
      break;
    case Kind::kNative: {
      const std::string text
          = odb::dbStringProperty::find(block, kNativeProperty)->getValue();
      try {
        NativeDecoder decoder(sta, block);
        decoder.decode(text);
      } catch (const ParseError& e) {
        logger->error(utl::STA,
                      3010,
                      "the timing constraints stored in the database could "
                      "not be restored: {}",
                      e.what());
      }
      break;
    }
    case Kind::kText: {
      // read_sdc is `source`; the stored text is replayed the same way so
      // that every sdc command goes through exactly the code path it would
      // have taken had the constraints been read from a file.
      const std::string text
          = odb::dbStringProperty::find(block, kTextProperty)->getValue();
      Tcl_Interp* interp = sta->tclInterp();
      if (Tcl_Eval(interp, text.c_str()) != TCL_OK) {
        logger->error(utl::STA,
                      3009,
                      "error replaying the timing constraints stored in the "
                      "database: {}",
                      Tcl_GetStringResult(interp));
      }
      break;
    }
  }
  if (found != Kind::kNone) {
    logger->info(utl::STA,
                 3011,
                 "Restored the timing constraints stored in the database "
                 "({} form).",
                 kindName(found));
  }
  return found;
}

}  // namespace sta
