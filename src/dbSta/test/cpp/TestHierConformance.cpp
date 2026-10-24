// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Hierarchy conformance: for every netlist in the corpus, prove that each
// emitted netlist is logically equivalent to the *input* netlist, as two
// independent checks:
//
//   A)  input.v  ==  read_verilog -> link_design       -> write_verilog
//   B)  input.v  ==  read_verilog -> link_design -hier -> write_verilog
//
// Deliberately NOT flat_out == hier_out. Comparing the two outputs only proves
// the two paths agree, and the flat dbNet view is built by mode-independent
// code (`hierarchy_` appears nowhere in dbReadVerilog.cc's makeDbNets), so a
// reader bug upstream of the hierarchy split corrupts both outputs identically
// and the comparison passes while both are wrong. The input netlist is the only
// ground truth available.
//
// Running A and B separately also gives attribution: a hier-vs-flat mismatch
// says the two disagree but not which is wrong, while A and B independently
// name the broken path.
//
// Both checks use SEC with the `binary` encoding -- see tst/lec.h. LEC mode
// cannot serve check A at all: flat write_verilog renames every instance
// (`b1.r1` becomes `\b1/r1 `), which a name-matching boundary comparison
// rejects as a mismatch even though nothing is wrong.

#include <algorithm>
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
#include "tst/lec.h"
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

struct CorpusEntry
{
  // Absolute path to the netlist, already resolved through runfiles.
  std::string path;
  // File name, used both as the gtest parameter name and as the XFAIL
  // manifest key.
  std::string name;
  std::string top;
  Technology tech{Technology::kNangate45};
  // Authored for this suite (lives in hier_cases/) rather than inherited from
  // another test via the manifest.
  bool authored{false};
  // Construct tags from the `// TARGETS:` header, sorted. Authored cases only.
  std::vector<std::string> targets;
  // For a reclaimed case, the `// ORIGIN:` path of the netlist it was copied
  // and repaired from. Reclaimed cases inherit their shape rather than
  // targeting a construct, so they are exempt from the TARGETS rules.
  std::string origin;
  // Set when the corpus could not be loaded, so the suite reports that rather
  // than silently instantiating zero cases.
  std::optional<std::string> load_error;
};

// Without this gtest hex-dumps the struct into every failure message. Must be
// found by ADL, so it lives in the same namespace as CorpusEntry.
void PrintTo(const CorpusEntry& entry, std::ostream* os)
{
  if (entry.load_error.has_value()) {
    *os << "<corpus load error: " << *entry.load_error << ">";
    return;
  }
  *os << entry.name << " (top " << entry.top << ")";
}

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

// Makes gtest report the netlist name instead of GetParam(0).
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

std::vector<std::filesystem::path> libertyFor(Technology tech)
{
  switch (tech) {
    case Technology::kNangate45:
      return {getRunfilePath("_main/test/Nangate45/Nangate45_typ.lib")};
    case Technology::kSky130hd:
      return {getRunfilePath("_main/test/sky130hd/sky130_fd_sc_hd_tt.lib")};
  }
  return {};
}

// The mode settled by the Stage 0 spike. The same for both paths, so this is a
// constant rather than a function of `path` -- kept named so the reason stays
// attached to the choice.
LecMode modeFor(Path /* path */)
{
  return LecMode::kSequential;
}

const char* kCasesDir = "_main/src/dbSta/test/cpp/hier_cases/";

// Splits on ':' and trims surrounding whitespace. Trailing empty fields are
// preserved so a manifest line may end with an empty reason.
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

// A one-entry corpus that reports why the real one could not be loaded.
std::vector<CorpusEntry> corpusLoadError(const std::string& message)
{
  CorpusEntry entry;
  entry.name = "corpus_load_error";
  entry.load_error = message;
  return {entry};
}

// How a case fails, as recorded in the manifest and as observed by a run. An
// XFAIL that accepts *any* failure is barely stronger than a disabled test: a
// case recorded as producing a counterexample would keep passing after it
// starts crashing the linker instead, and the regression would be invisible
// because the case was already expected to be red. Recording the mode makes the
// manifest say what the defect is, so a case that changes how it fails is a
// finding rather than a silent one.
//
// The two stage tokens name where in the round trip the case died; the rest are
// LEC verdicts. Kept as strings because they cross a file boundary -- these are
// the same tokens hier_expected_fail.bzl validates against.
constexpr const char* kModeOrError = "or-error";
constexpr const char* kModeWriteError = "write-error";

const char* modeOf(LecResult result)
{
  switch (result) {
    case LecResult::kCounterexample:
      return "counterexample";
    case LecResult::kBoundaryMismatch:
      return "boundary-mismatch";
    case LecResult::kPartial:
      return "partial";
    case LecResult::kInconclusive:
      return "inconclusive";
    case LecResult::kToolError:
      return "tool-error";
    case LecResult::kProved:
    case LecResult::kNotInstalled:
      break;
  }
  // Neither is a failure mode: kProved is not a failure at all, and
  // kNotInstalled is caught by an ASSERT before any of this runs.
  return "unexpected";
}

struct ExpectedFailure
{
  // One of the tokens above: which way this case is known to fail.
  std::string mode;
  // The OpenROAD issue, when one has been filed. Empty otherwise -- see
  // hier_expected_fail.bzl for why that is allowed to be empty rather than
  // carrying a placeholder.
  std::string issue;
  std::string symptom;
};

// "issue 1234, " when one is recorded, "" otherwise, so a message about an
// unfiled defect does not read as a formatting bug.
std::string issuePrefix(const std::string& issue)
{
  return issue.empty() ? std::string() : "issue " + issue + ", ";
}

struct ParsedManifest
{
  std::vector<std::tuple<std::string, Path, ExpectedFailure>> entries;
  // Rows this parser could not read. Reported by ManifestIsWellFormed rather
  // than dropped: an unreadable row silently removes an XFAIL, and the case
  // then reads as one that is expected to pass.
  std::vector<std::string> malformed;
};

// Parses the XFAIL manifest into a (netlist, path) -> reason map. The manifest
// is generated by the build rule from CONFORMANCE_EXPECTED_FAIL in
// src/dbSta/test/hier_expected_fail.bzl, which is where entries are edited:
// grouping the netlists under one entry per failure mode keeps the list
// readable, and Starlark rejects a malformed entry when the package loads
// rather than leaving it to be dropped here.
const ParsedManifest& expectedFailures()
{
  static const ParsedManifest all = []() {
    ParsedManifest parsed;
    // A per-case target is handed its own rows in HIER_EXPECTED_FAIL, so it
    // depends on the netlist it runs and not on every other case's XFAIL
    // entries. The corpus-wide target leaves it unset and reads the manifest,
    // which is the whole list -- including any row naming no case at all, which
    // is what ManifestIsWellFormed exists to report.
    const char* inline_rows = std::getenv("HIER_EXPECTED_FAIL");
    std::ifstream file;
    std::istringstream rows;
    if (inline_rows != nullptr) {
      rows.str(inline_rows);
    } else {
      file.open(getRunfilePath(std::string(kCasesDir) + "expected_fail.txt"));
    }
    std::istream& in
        = inline_rows != nullptr ? static_cast<std::istream&>(rows) : file;
    std::string line;
    while (std::getline(in, line)) {
      if (isComment(line)) {
        continue;
      }
      // netlist : path : mode : issue : symptom. The generator always emits
      // all five fields, though `issue` is empty until one is filed.
      const std::vector<std::string> f = splitFields(line);
      if (f.size() < 5) {
        parsed.malformed.push_back(line);
        continue;
      }
      const Path path = f[1] == "hier" ? Path::kHier : Path::kFlat;
      parsed.entries.emplace_back(
          f[0], path, ExpectedFailure{f[2], f[3], f[4]});
    }
    return parsed;
  }();
  return all;
}

const ExpectedFailure* expectedFailure(const std::string& netlist, Path path)
{
  for (const auto& [name, entry_path, failure] : expectedFailures().entries) {
    if (name == netlist && entry_path == path) {
      return &failure;
    }
  }
  return nullptr;
}

// The netlists whose top module is not "top", as `<file>=<top>,...`. The build
// rule supplies it (HIER_TOP_OVERRIDES in src/dbSta/test/BUILD) so the corpus
// metadata sits with the build rules instead of inside each netlist.
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

// One corpus entry for `file`. `authored` is false for the inherited/
// subdirectory, whose netlists are symlinks to other suites' fixtures: they
// carry no header of ours, so the TARGETS lint below must not demand one.
CorpusEntry makeCorpusEntry(const std::filesystem::path& file,
                            bool authored,
                            const std::map<std::string, std::string>& tops)
{
  CorpusEntry entry;
  entry.path = file.string();
  entry.name = file.filename().string();
  // Defaults cover all but a handful of cases; the build rule names the
  // exceptions. Nangate45 is the only technology the corpus uses.
  entry.top = "top";
  entry.tech = Technology::kNangate45;
  if (const auto it = tops.find(entry.name); it != tops.end()) {
    entry.top = it->second;
  }
  if (authored) {
    std::ifstream case_file(file);
    std::string case_line;
    while (std::getline(case_file, case_line)) {
      if (case_line.rfind("// TARGETS:", 0) == 0) {
        // Comma-separated construct tags, normalized to a sorted set so
        // TargetsAreUnique below compares them order-independently.
        std::istringstream tags(splitFields(case_line).back());
        std::string tag;
        while (std::getline(tags, tag, ',')) {
          const std::string::size_type b = tag.find_first_not_of(" \t");
          const std::string::size_type e = tag.find_last_not_of(" \t");
          if (b != std::string::npos) {
            entry.targets.push_back(tag.substr(b, e - b + 1));
          }
        }
        std::sort(entry.targets.begin(), entry.targets.end());
      } else if (case_line.rfind("// ORIGIN:", 0) == 0) {
        entry.origin = splitFields(case_line).back();
      } else if (case_line.rfind("//", 0) != 0) {
        break;  // header comments only
      }
    }
  }
  entry.authored = authored;
  return entry;
}

// Adds every .v in `dir` to `corpus`.
void scanCaseDirectory(const std::filesystem::path& dir,
                       bool authored,
                       const std::map<std::string, std::string>& tops,
                       std::vector<CorpusEntry>& corpus)
{
  if (!std::filesystem::is_directory(dir)) {
    return;
  }
  for (const auto& item : std::filesystem::directory_iterator(dir)) {
    if (item.path().extension() != ".v") {
      continue;
    }
    corpus.push_back(makeCorpusEntry(item.path(), authored, tops));
  }
}

// The corpus named explicitly, as corpus-relative paths ("case.v",
// "inherited/case.v"). A per-case test target names its one case here and
// carries only that netlist in its runfiles, so bazel caches and invalidates
// the corpus one case at a time; the corpus-wide target names nothing and gets
// the directory scan. A case under inherited/ is unauthored, exactly as the
// scan records it -- the name it is keyed by stays the bare file name.
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
    const bool authored = name.find('/') == std::string::npos;
    corpus.push_back(makeCorpusEntry(
        getRunfilePath(std::string(kCasesDir) + name), authored, tops));
  }
  return corpus;
}

// The corpus is every .v file under hier_cases/, plus the symlinks in
// hier_cases/inherited/ that point at other suites' fixtures. There is no
// manifest: a folder holds a category, and adding a case means adding a file.
// hier_cases/structural/ (cases LEC cannot adjudicate) and hier_cases/crash/
// (cases that kill the process) are deliberately NOT scanned here.
//
// Runs during static initialization (INSTANTIATE_TEST_SUITE_P), so it must not
// throw: a failure to find the corpus comes back as a single entry carrying
// load_error, which the test body then reports.
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
              getRunfilePath(std::string(kCasesDir) + "expected_fail.txt"))
              .parent_path();
    const std::map<std::string, std::string> tops = topOverrides();
    scanCaseDirectory(cases_dir, /*authored=*/true, tops, corpus);
    scanCaseDirectory(
        cases_dir / "inherited", /*authored=*/false, tops, corpus);
  } catch (const std::exception& e) {
    return corpusLoadError(std::string("loading corpus: ") + e.what());
  }

  if (corpus.empty()) {
    return corpusLoadError("corpus is empty");
  }
  return corpus;
}

const std::vector<CorpusEntry>& corpus()
{
  static const std::vector<CorpusEntry> loaded = loadCorpus();
  return loaded;
}

// Inverts the expectation for a known failure, so an accidental fix turns the
// suite red with an actionable message rather than silently losing coverage.
//
// `mode` is how this run failed, and is compared against the recorded mode: a
// known failure is only "as expected" if it still fails the recorded way. It is
// ignored when the case passed, since there is no failure to classify.
void expectOrXfail(const CorpusEntry& entry,
                   Path path,
                   bool proved,
                   const char* mode,
                   const std::string& detail)
{
  const ExpectedFailure* failure = expectedFailure(entry.name, path);
  if (failure == nullptr) {
    EXPECT_TRUE(proved) << detail;
    return;
  }

  if (proved) {
    ADD_FAILURE() << entry.name << " [" << toString(path)
                  << "] is a known failure (" << issuePrefix(failure->issue)
                  << failure->mode << ": " << failure->symptom
                  << "). It now PASSES -- delete it from "
                  << "CONFORMANCE_EXPECTED_FAIL in "
                     "src/dbSta/test/hier_expected_fail.bzl.";
    return;
  }

  // Still failing, but not the recorded way. Worth a failure of its own: a
  // counterexample degrading into a crash, or a crash into a tool error, is a
  // change in behavior that an XFAIL keyed only on "fails somehow" would hide
  // for as long as the entry survives.
  EXPECT_EQ(failure->mode, mode)
      << entry.name << " [" << toString(path)
      << "] is a known failure, but it now fails a different way. Recorded '"
      << failure->mode << "' (" << failure->symptom << "), observed '" << mode
      << "':\n"
      << detail << "\nIf the new mode is expected, update the entry in "
      << "CONFORMANCE_EXPECTED_FAIL in src/dbSta/test/hier_expected_fail.bzl.";
}

void checkPathAgainstInput(const CorpusEntry& entry, Path path)
{
  ASSERT_FALSE(entry.load_error.has_value()) << *entry.load_error;
  TST_REQUIRE_LEC();

  const bool hierarchy = path == Path::kHier;

  // link_design reports failure by throwing from utl::Logger::error(), so a
  // legal netlist that a link mode refuses surfaces as an exception rather than
  // a return code. A legal netlist that -hier refuses while flat accepts is a
  // conformance bug too: a divergence in what the modes *accept* matters as
  // much as one in what they emit. Rejections route through expectOrXfail so a
  // known reader/linker defect can be tracked in the XFAIL manifest like any
  // other conformance failure.
  std::optional<LoadedDesign> design;
  try {
    design.emplace(entry.tech, entry.path, entry.top.c_str(), hierarchy);
  } catch (const std::exception& e) {
    std::string detail = entry.name;
    detail += " [";
    detail += toString(path);
    detail += "]: read_verilog/link_design";
    detail += hierarchy ? " -hier" : "";
    detail += " rejected the netlist: ";
    detail += e.what();
    expectOrXfail(entry, path, /*proved=*/false, kModeOrError, detail);
    return;
  }

  const std::string out_v
      = workDir() / (entry.name + "." + toString(path) + ".v");
  try {
    design->writeVerilog(out_v);
  } catch (const std::exception& e) {
    std::string detail = entry.name;
    detail += " [";
    detail += toString(path);
    detail += "]: write_verilog threw: ";
    detail += e.what();
    expectOrXfail(entry, path, /*proved=*/false, kModeWriteError, detail);
    return;
  }

  // Gold is the input netlist itself, never the other path's output.
  const LecOutcome outcome = runLec(/*gold_v=*/entry.path,
                                    /*gate_v=*/out_v,
                                    libertyFor(entry.tech),
                                    modeFor(path),
                                    workDir());

  ASSERT_NE(outcome.result, LecResult::kNotInstalled) << outcome.detail;

  std::string detail = entry.name;
  detail += " [";
  detail += toString(path);
  detail += "] is not provably equivalent to its input: ";
  detail += tst::toString(outcome.result);
  detail += "\n";
  detail += outcome.detail;
  detail += "\n  emitted: " + out_v;

  expectOrXfail(entry,
                path,
                outcome.result == LecResult::kProved,
                modeOf(outcome.result),
                detail);
}

class TestHierPath : public ::testing::TestWithParam<CorpusEntry>
{
};

class TestFlatPath : public ::testing::TestWithParam<CorpusEntry>
{
};

TEST_P(TestHierPath, MatchesInput)
{
  checkPathAgainstInput(GetParam(), Path::kHier);
}

TEST_P(TestFlatPath, MatchesInput)
{
  checkPathAgainstInput(GetParam(), Path::kFlat);
}

INSTANTIATE_TEST_SUITE_P(HierCases,
                         TestHierPath,
                         ::testing::ValuesIn(corpus()),
                         entryName);

INSTANTIATE_TEST_SUITE_P(HierCases,
                         TestFlatPath,
                         ::testing::ValuesIn(corpus()),
                         entryName);

// Every authored case must declare what construct it targets. Two cases may
// share a TARGETS set when they bracket one hazard with different structure
// (e.g. the same shape one level shallower), so only byte-identical bodies
// count as redundant.
TEST(TestHierConformanceCorpus, TargetsAreUnique)
{
  auto body = [](const std::string& path) {
    std::ifstream in(path);
    std::string text;
    std::string line;
    while (std::getline(in, line)) {
      if (line.rfind("//", 0) == 0) {
        continue;  // headers may differ while the netlist is a duplicate
      }
      text += line;
      text += '\n';
    }
    return text;
  };
  std::map<std::vector<std::string>, std::vector<const CorpusEntry*>> by_tags;
  for (const CorpusEntry& entry : corpus()) {
    if (!entry.authored || !entry.origin.empty()) {
      // Inherited netlists are owned by their original tests; reclaimed copies
      // are repaired versions of those, not new constructs.
      continue;
    }
    EXPECT_FALSE(entry.targets.empty())
        << entry.name << " has no '// TARGETS: ...' header. Authored cases must"
        << " say which construct they exercise.";
    by_tags[entry.targets].push_back(&entry);
  }
  for (const auto& [targets, entries] : by_tags) {
    if (entries.size() < 2) {
      continue;
    }
    for (size_t i = 0; i < entries.size(); ++i) {
      for (size_t j = i + 1; j < entries.size(); ++j) {
        EXPECT_NE(body(entries[i]->path), body(entries[j]->path))
            << entries[i]->name << " and " << entries[j]->name
            << " declare identical TARGETS and have identical bodies;"
            << " one of them is redundant.";
      }
    }
  }
}

// Two ways the manifest can be wrong that no individual case would report
// clearly.
//
// An unreadable row drops an XFAIL, which does turn the suite red -- but as a
// pile of cases failing for no stated reason, several steps from the typo that
// caused it. One message naming the row is worth the check.
//
// A row naming a netlist that is not in the corpus reports nothing at all: it
// is dead text that still reads as coverage of a known defect, which is exactly
// what an XFAIL list must never accumulate. It is how a case gets deleted or
// renamed while its entry stays behind, describing a defect nothing tests.
TEST(TestHierConformanceCorpus, ManifestIsWellFormed)
{
  for (const std::string& line : expectedFailures().malformed) {
    ADD_FAILURE() << "unreadable XFAIL row, expected "
                  << "`netlist : path : mode : issue : symptom`: " << line;
  }
  EXPECT_FALSE(expectedFailures().entries.empty())
      << "the XFAIL manifest parsed as empty; every known failure would be "
         "reported as a new one";

  std::set<std::string> names;
  for (const CorpusEntry& entry : corpus()) {
    names.insert(entry.name);
  }
  for (const auto& [netlist, path, failure] : expectedFailures().entries) {
    EXPECT_TRUE(names.count(netlist) > 0)
        << netlist << " [" << toString(path)
        << "] is listed in CONFORMANCE_EXPECTED_FAIL, but no such netlist is "
           "in the corpus. Remove the entry, or restore the case it names.";
  }
}

// Guards against the failure mode this whole suite exists to avoid: a corpus
// that loaded as zero cases would make both suites above vacuously green.
TEST(TestHierConformanceCorpus, IsLoaded)
{
  ASSERT_FALSE(corpus().empty());
  for (const CorpusEntry& entry : corpus()) {
    ASSERT_FALSE(entry.load_error.has_value()) << *entry.load_error;
  }
  EXPECT_GT(corpus().size(), 1U)
      << "only one corpus case resolved; the manifest is probably not being "
         "read";

  // Every netlist must exist, or a manifest typo would surface as a confusing
  // link failure per case instead of one clear message.
  for (const CorpusEntry& entry : corpus()) {
    EXPECT_TRUE(std::filesystem::exists(entry.path))
        << entry.name << " listed in the manifest does not exist at "
        << entry.path;
  }
}

}  // namespace
}  // namespace tst
