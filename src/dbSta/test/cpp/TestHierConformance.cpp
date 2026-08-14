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
#include <sstream>
#include <string>
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
  std::string load_error;
};

// Without this gtest hex-dumps the struct into every failure message. Must be
// found by ADL, so it lives in the same namespace as CorpusEntry.
void PrintTo(const CorpusEntry& entry, std::ostream* os)
{
  if (!entry.load_error.empty()) {
    *os << "<corpus load error: " << entry.load_error << ">";
    return;
  }
  *os << entry.name << " (top " << entry.top << ")";
}

// Makes gtest report the netlist name instead of GetParam(0).
std::string entryName(const ::testing::TestParamInfo<CorpusEntry>& info)
{
  std::string name = info.param.name;
  for (char& c : name) {
    if (!std::isalnum(static_cast<unsigned char>(c))) {
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

struct ExpectedFailure
{
  std::string issue;
  std::string symptom;
};

// Parses the XFAIL manifest into a (netlist, path) -> reason map. The manifest
// is generated by the build rule from CONFORMANCE_EXPECTED_FAIL in
// src/dbSta/test/hier_expected_fail.bzl, which is where entries are edited:
// grouping the netlists under one entry per failure mode keeps the list
// readable, and Starlark rejects a malformed entry when the package loads
// rather than leaving it to be dropped here.
const std::vector<std::tuple<std::string, Path, ExpectedFailure>>&
expectedFailures()
{
  static const std::vector<std::tuple<std::string, Path, ExpectedFailure>> all
      = []() {
          std::vector<std::tuple<std::string, Path, ExpectedFailure>> parsed;
          std::ifstream in(
              getRunfilePath(std::string(kCasesDir) + "expected_fail.txt"));
          std::string line;
          while (std::getline(in, line)) {
            if (isComment(line)) {
              continue;
            }
            const std::vector<std::string> f = splitFields(line);
            if (f.size() < 3) {
              continue;
            }
            const Path path = f[1] == "hier" ? Path::kHier : Path::kFlat;
            parsed.emplace_back(
                f[0], path, ExpectedFailure{f[2], f.size() > 3 ? f[3] : ""});
          }
          return parsed;
        }();
  return all;
}

const ExpectedFailure* expectedFailure(const std::string& netlist, Path path)
{
  for (const auto& [name, entry_path, failure] : expectedFailures()) {
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

// Adds every .v in `dir` to `corpus`. `authored` is false for the inherited/
// subdirectory, whose netlists are symlinks to other suites' fixtures: they
// carry no header of ours, so the TARGETS lint below must not demand one.
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
    CorpusEntry entry;
    entry.path = item.path().string();
    entry.name = item.path().filename().string();
    // Defaults cover all but a handful of cases; the build rule names the
    // exceptions. Nangate45 is the only technology the corpus uses.
    entry.top = "top";
    entry.tech = Technology::kNangate45;
    if (const auto it = tops.find(entry.name); it != tops.end()) {
      entry.top = it->second;
    }
    if (authored) {
      std::ifstream case_file(item.path());
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
    corpus.push_back(entry);
  }
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
void expectOrXfail(const CorpusEntry& entry,
                   Path path,
                   bool proved,
                   const std::string& detail)
{
  if (const ExpectedFailure* failure = expectedFailure(entry.name, path)) {
    EXPECT_FALSE(proved) << entry.name << " [" << toString(path)
                         << "] is a known failure (issue " << failure->issue
                         << ": " << failure->symptom
                         << "). It now PASSES -- delete it from "
                         << "CONFORMANCE_EXPECTED_FAIL in "
                            "src/dbSta/test/hier_expected_fail.bzl.";
  } else {
    EXPECT_TRUE(proved) << detail;
  }
}

void checkPathAgainstInput(const CorpusEntry& entry, Path path)
{
  ASSERT_TRUE(entry.load_error.empty()) << entry.load_error;
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
    expectOrXfail(entry, path, /*proved=*/false, detail);
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
    expectOrXfail(entry, path, /*proved=*/false, detail);
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

  expectOrXfail(entry, path, outcome.result == LecResult::kProved, detail);
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

// Guards against the failure mode this whole suite exists to avoid: a corpus
// that loaded as zero cases would make both suites above vacuously green.
TEST(TestHierConformanceCorpus, IsLoaded)
{
  ASSERT_FALSE(corpus().empty());
  for (const CorpusEntry& entry : corpus()) {
    ASSERT_TRUE(entry.load_error.empty()) << entry.load_error;
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
