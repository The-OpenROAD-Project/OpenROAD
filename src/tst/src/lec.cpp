// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "tst/lec.h"

#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "gtest/gtest.h"
#ifdef BAZEL_BUILD
#include "tools/cpp/runfiles/runfiles.h"
#endif

namespace tst {

namespace {

const char* kInstallHint
    = "kepler-formal was not found. Under bazel it comes from the test's\n"
      "runfiles (//src/tst:kepler_formal_bin, a data dep of //src/tst:lec);\n"
      "otherwise it is resolved from $KEPLER_FORMAL (an explicit path to the\n"
      "binary) or from `kepler-formal` on $PATH.\n"
      "Build it from https://github.com/keplertech/kepler-formal and put it "
      "on\n"
      "$PATH, or set KEPLER_FORMAL=/path/to/kepler-formal.";

// Runfiles path of the binary, keyed by *apparent* repository name -- the
// runfiles library maps it through the main repo's repository mapping, so the
// bzlmod canonical name ("kepler-formal+") never appears here.
const char* kKeplerRunfile = "kepler-formal/src/bin/kepler-formal";

bool isExecutableFile(const std::filesystem::path& path)
{
  std::error_code ec;
  if (!std::filesystem::is_regular_file(path, ec)) {
    return false;
  }
  return ::access(path.c_str(), X_OK) == 0;
}

std::string readAll(const std::filesystem::path& path)
{
  std::ifstream in(path);
  if (!in) {
    return {};
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

// kepler-formal links naja's shared libraries, which a --prefix install puts in
// a lib/ directory alongside bin/ rather than anywhere the loader searches by
// default. Point the loader at both so callers do not have to set
// LD_LIBRARY_PATH by hand. An existing LD_LIBRARY_PATH is preserved, appended
// after the derived entries.
std::string loaderEnvPrefix(const std::filesystem::path& binary)
{
  const std::filesystem::path bin_dir
      = std::filesystem::absolute(binary).parent_path();
  std::string paths = bin_dir.string();
  paths += ":" + (bin_dir.parent_path() / "lib").string();
  if (const char* existing = std::getenv("LD_LIBRARY_PATH");
      existing != nullptr && *existing != '\0') {
    paths += ":";
    paths += existing;
  }
  return "LD_LIBRARY_PATH='" + paths + "' ";
}

// Runs `command` capturing stdout and stderr together. Sets `exit_code` to the
// process exit status, or -1 if it could not be run or did not exit normally.
std::string capture(const std::string& command, int* exit_code)
{
  *exit_code = -1;
  FILE* pipe = popen((command + " 2>&1").c_str(), "r");
  if (pipe == nullptr) {
    return {};
  }
  std::string output;
  std::array<char, 4096> buffer{};
  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
    output += buffer.data();
  }
  const int status = pclose(pipe);
  if (status != -1 && WIFEXITED(status)) {
    *exit_code = WEXITSTATUS(status);
  }
  return output;
}

// Extracts the SEC coverage percentage. Returns -1 when the line is absent,
// which is normal for kCombinational mode and a defect anywhere else -- see the
// call site, which must not read the sentinel as full coverage.
double coveragePercent(const std::string& text)
{
  static const std::regex re(R"(checked-output coverage: ([0-9.]+)%)");
  std::smatch m;
  if (std::regex_search(text, m, re)) {
    return std::stod(m[1].str());
  }
  return -1.0;
}

// Returns the first line matching `pattern`, trimmed, or "".
std::string firstLineContaining(const std::string& text, const char* pattern)
{
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) {
    if (line.find(pattern) != std::string::npos) {
      return line;
    }
  }
  return {};
}

}  // namespace

const char* toString(LecResult result)
{
  switch (result) {
    case LecResult::kProved:
      return "proved";
    case LecResult::kPartial:
      return "partial (incomplete output coverage)";
    case LecResult::kInconclusive:
      return "inconclusive (no recognized verdict)";
    case LecResult::kCounterexample:
      return "counterexample";
    case LecResult::kBoundaryMismatch:
      return "boundary mismatch";
    case LecResult::kToolError:
      return "tool error";
    case LecResult::kNotInstalled:
      return "kepler-formal not installed";
  }
  return "unknown";
}

// The binary as a bazel runfile. Empty outside a bazel test, or when the test
// did not declare //src/tst:kepler_formal_bin in its data.
std::filesystem::path keplerFormalRunfile()
{
#ifdef BAZEL_BUILD
  using bazel::tools::cpp::runfiles::Runfiles;
  // Created once per process; resolution depends only on the environment
  // bazel sets up for the test.
  static const std::unique_ptr<Runfiles> runfiles = [] {
    std::string error;
    // Reads $TEST_SRCDIR / $RUNFILES_MANIFEST_FILE. Outside a bazel test those
    // are unset and this returns null -- not an error here, just "no runfile".
    return std::unique_ptr<Runfiles>(Runfiles::CreateForTest(&error));
  }();
  if (runfiles == nullptr) {
    return {};
  }
  const std::string path = runfiles->Rlocation(kKeplerRunfile);
  if (!path.empty() && isExecutableFile(path)) {
    return path;
  }
#endif
  return {};
}

std::filesystem::path findKeplerFormal()
{
  if (const char* env = std::getenv("KEPLER_FORMAL");
      env != nullptr && *env != '\0') {
    // An explicit override that does not resolve is a configuration error the
    // caller wants to hear about, so do not silently fall through to the
    // runfile or $PATH.
    return isExecutableFile(env) ? std::filesystem::path(env) : "";
  }

  // Prefer the runfile over $PATH: a bazel test must use the binary its own
  // dependency graph built, not whatever happens to be installed.
  if (const std::filesystem::path runfile = keplerFormalRunfile();
      !runfile.empty()) {
    return runfile;
  }

  if (const char* path = std::getenv("PATH"); path != nullptr) {
    std::istringstream dirs(path);
    std::string dir;
    while (std::getline(dirs, dir, ':')) {
      if (dir.empty()) {
        continue;
      }
      const std::filesystem::path candidate
          = std::filesystem::path(dir) / "kepler-formal";
      if (isExecutableFile(candidate)) {
        return candidate;
      }
    }
  }
  return {};
}

bool isLecAvailable()
{
  return !findKeplerFormal().empty();
}

void assertLecAvailable()
{
  if (isLecAvailable()) {
    return;
  }
  const char* env = std::getenv("KEPLER_FORMAL");
  if (env != nullptr && *env != '\0') {
    FAIL() << "KEPLER_FORMAL is set to '" << env
           << "' but that is not an executable file.\n"
           << kInstallHint;
  }
  FAIL() << kInstallHint;
}

LecOutcome runLec(const std::filesystem::path& gold_v,
                  const std::filesystem::path& gate_v,
                  const std::vector<std::filesystem::path>& liberty,
                  LecMode mode,
                  const std::filesystem::path& work_dir)
{
  LecOutcome outcome;

  const std::filesystem::path binary = findKeplerFormal();
  if (binary.empty()) {
    outcome.result = LecResult::kNotInstalled;
    outcome.detail = kInstallHint;
    return outcome;
  }

  // Name the artifacts after the gate netlist so several runs in one test do
  // not overwrite each other's evidence.
  const std::string stem = std::filesystem::path(gate_v).stem().string()
                           + (mode == LecMode::kSequential ? "_sec" : "_lec");
  outcome.config_path = work_dir / (stem + ".yaml");
  outcome.log_path = work_dir / (stem + ".log");

  {
    std::ofstream cfg(outcome.config_path);
    cfg << "format: verilog\n";
    cfg << "verification: " << (mode == LecMode::kSequential ? "sec" : "lec")
        << "\n";
    if (mode == LecMode::kSequential) {
      // Encoding choice, and it has flipped once already -- do not change it
      // without re-running a mutation battery, because both options have been
      // unsound at some point:
      //
      //   binary            was required through kepler PR #167:
      //   dual_rail_steady
      //                     proved equivalence for a netlist with a flip-flop's
      //                     D and CK swapped, which binary caught.
      //   dual_rail_steady  is required from kepler PR #168 on. That PR changed
      //                     the binary path's output filter to trigger on "some
      //                     flop lacks an initial value", and nothing in the
      //                     tree assigns initial values any more, so binary now
      //                     refuses every sequential design outright --
      //                     returning a verdict identical on equivalent and
      //                     inequivalent inputs, i.e. no information.
      //                     dual_rail_steady in turn gained the D/CK case, and
      //                     catches every mutant in the battery at 100%
      //                     coverage.
      cfg << "sec_encoding: dual_rail_steady\n";
    }
    // Produces the per-output reasons an incomplete-coverage failure needs to
    // be actionable.
    cfg << "report_skipped_pos: true\n";
    // Flat form: exactly two paths, one file per design. (The nested form
    // groups multiple files per design; not needed here.)
    cfg << "input_paths:\n";
    cfg << "  - " << gold_v << "\n";
    cfg << "  - " << gate_v << "\n";
    cfg << "liberty_files:\n";
    for (const std::filesystem::path& lib : liberty) {
      cfg << "  - " << lib << "\n";
    }
    cfg << "log_file: " << outcome.log_path << "\n";
  }

  // Truncate any log from a previous run so a tool that dies before writing
  // cannot leave us reading stale evidence.
  {
    std::ofstream truncate(outcome.log_path);
  }

  // Run from work_dir: with report_skipped_pos on, kepler-formal drops
  // boundary_terms.txt and skipped_*_pos.txt into the current directory. Under
  // bazel that would be the sandbox, but anyone running the test binary
  // directly would otherwise find them littering their working tree.
  int exit_code = 0;
  const std::string stdout_text
      = capture("cd '" + work_dir.string() + "' && " + loaderEnvPrefix(binary)
                    + "'" + binary.string() + "' --config '"
                    + outcome.config_path.string() + "'",
                &exit_code);
  const std::string log_text = readAll(outcome.log_path);
  const std::string all = stdout_text + log_text;

  auto detail = [&](const std::string& head) {
    std::string text = head;
    text += "\n  config: " + outcome.config_path.string();
    text += "\n  log:    " + outcome.log_path.string();
    return text;
  };

  // Classify on text, never on exit code -- the verdict is read first and the
  // exit status only decides what an ABSENT verdict means. kepler-formal used
  // to exit 0 for both "proved" and "found a difference", and now exits 3 on a
  // difference, so an exit-code-first classifier reports every genuine
  // inequivalence as a tool error. That is the worst possible direction to be
  // wrong in: a real defect and a broken fixture become indistinguishable.
  if (all.find("Difference was found") != std::string::npos
      || all.find("Circuits are DIFFERENT") != std::string::npos) {
    std::string cex = firstLineContaining(all, "counterexample");
    if (cex.empty()) {
      cex = firstLineContaining(all, "Difference was found");
    }
    outcome.result = LecResult::kCounterexample;
    outcome.detail = detail(cex);
    return outcome;
  }

  // The two designs' boundary sets differed, so the tool stopped before
  // solving. Read before the partial verdict below: a refusal to compare is not
  // a partial comparison.
  const std::string mismatch = firstLineContaining(all, "Mismatched");
  if (exit_code != 0 && !mismatch.empty()) {
    outcome.result = LecResult::kBoundaryMismatch;
    outcome.detail = detail(mismatch);
    return outcome;
  }

  // "SEC partially proved equivalence at k = 0: 1/2 outputs proved" -- a
  // verdict, and one kepler-formal delivers with a nonzero exit and none of the
  // success wording below, so it has to be recognized before the exit code is
  // read as a tool failure. Older builds reported the same situation as a plain
  // proof carrying a sub-100 coverage figure, which the coverage test further
  // down still catches; both are handled because the difference is only a
  // kepler version apart.
  //
  // Getting this wrong is not cosmetic. A partial proof is the exact signature
  // of a dropped connection -- the outputs in the orphaned cone become
  // unprovable and are skipped -- so filing it as "the checker broke" hides the
  // class of defect this harness exists to find, and hides it as tooling noise
  // that nobody re-examines.
  if (all.find("partially proved equivalence") != std::string::npos) {
    outcome.result = LecResult::kPartial;
    std::string why = firstLineContaining(all, "checked-output coverage");
    if (why.empty()) {
      why = firstLineContaining(all, "partially proved equivalence");
    }
    outcome.detail = detail(why);
    return outcome;
  }

  // No verdict in the output, so a nonzero exit is a tool-level failure: a bad
  // config key or an unparseable netlist.
  if (exit_code != 0) {
    std::string why = firstLineContaining(all, "critical");
    if (why.empty()) {
      why = firstLineContaining(all, "error");
    }
    outcome.result = LecResult::kToolError;
    outcome.detail = detail("kepler-formal exited " + std::to_string(exit_code)
                            + (why.empty() ? "" : ": " + why));
    return outcome;
  }

  // The success wording is encoding-specific: `binary` says "No difference was
  // found", `dual_rail_steady` says "No binary-defined difference was found ...
  // under the dual-rail steady-state abstraction". Matching only the first
  // scores every dual-rail proof as inconclusive. Both are checked so the
  // encoding can be changed without silently failing the whole corpus. Neither
  // is a substring of the other, and the counterexample test above uses a
  // capital "Difference", so the dual-rail success line cannot match it.
  static constexpr const char* kProvedMarkers[]
      = {"No difference was found", "No binary-defined difference was found"};
  const char* proved_marker = nullptr;
  for (const char* marker : kProvedMarkers) {
    if (all.find(marker) != std::string::npos) {
      proved_marker = marker;
      break;
    }
  }
  if (proved_marker == nullptr) {
    outcome.result = LecResult::kInconclusive;
    outcome.detail = detail(
        "kepler-formal produced no recognized verdict; treating as failure");
    return outcome;
  }

  // "No difference was found" alone is not enough. A dropped internal
  // connection still reports it, while dropping coverage to a fraction of the
  // outputs -- the skipped outputs are listed in the log.
  const double coverage = coveragePercent(all);

  // In SEC the coverage line is unconditional, so its absence is not "this run
  // covered everything", it is "this output does not have the shape we parse".
  // Reading a missing number as full coverage would turn every future change to
  // kepler's reporting into a silently passing corpus -- the proof would stop
  // saying anything about how many outputs were compared, and nothing would
  // report that. Absent means unproved. (Combinational LEC never emits the
  // line, hence the mode test rather than a blanket requirement.)
  if (mode == LecMode::kSequential && coverage < 0.0) {
    outcome.result = LecResult::kInconclusive;
    outcome.detail = detail(
        "SEC reported equivalence but no checked-output coverage, so the "
        "verdict does not say how many outputs were compared");
    return outcome;
  }

  if (coverage >= 0.0 && coverage < 100.0) {
    outcome.result = LecResult::kPartial;
    outcome.detail
        = detail(firstLineContaining(all, "checked-output coverage"));
    return outcome;
  }

  // kCombinational only: the boundary-point counts live in the log, not on
  // stdout. An older kepler-formal silently compares just the intersection and
  // calls that IDENTICAL, so check the counts rather than trusting the verdict.
  // (Newer builds stop before solving instead, which lands in
  // kBoundaryMismatch.)
  static const std::regex diff_re(
      R"(size of diff[01] (?:in|out)puts: ([0-9]+))");
  for (auto it
       = std::sregex_iterator(log_text.begin(), log_text.end(), diff_re);
       it != std::sregex_iterator();
       ++it) {
    if (std::stoi((*it)[1].str()) != 0) {
      outcome.result = LecResult::kBoundaryMismatch;
      outcome.detail = detail(
          "verdict was 'no difference', but the designs' boundary sets differ: "
          + (*it).str());
      return outcome;
    }
  }

  outcome.result = LecResult::kProved;
  outcome.detail = detail(firstLineContaining(all, proved_marker));
  return outcome;
}

}  // namespace tst
