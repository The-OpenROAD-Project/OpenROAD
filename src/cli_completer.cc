// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "cli_completer.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "tcl.h"
#include "tclDecls.h"

namespace ord {

namespace {

constexpr const char* kBoundary = " \t\n\r[]{}";

int findWordStart(const std::string& line, int cursor_pos)
{
  int pos = cursor_pos - 1;
  while (pos >= 0
         && std::string(kBoundary).find(line[pos]) == std::string::npos) {
    --pos;
  }
  return pos + 1;
}

// Find the enclosing Tcl command for the word at word_start.  In Tcl the
// command is always the *first* non-flag word in the current segment; the
// remaining positions are positional arguments, flags, or flag values —
// none of which are the command name.  A '[' resets context so anything
// before it belongs to an outer command and is ignored here.
std::string findEnclosingCommand(const std::string& line, int word_start)
{
  const std::string boundary = kBoundary;
  std::vector<std::string> words;
  int pos = 0;
  while (pos < word_start) {
    while (pos < word_start && boundary.find(line[pos]) != std::string::npos) {
      if (line[pos] == '[') {
        words.clear();
      }
      ++pos;
    }
    if (pos >= word_start) {
      break;
    }
    const int start = pos;
    while (pos < word_start && boundary.find(line[pos]) == std::string::npos) {
      ++pos;
    }
    words.push_back(line.substr(start, pos - start));
  }
  for (const auto& w : words) {
    if (!w.empty() && w[0] != '-') {
      return w;
    }
  }
  return {};
}

// Evaluate a Tcl command that returns a list; sort and split it.
// Returns empty on Tcl error.
std::vector<std::string> getTclList(Tcl_Interp* interp,
                                    const std::string& tcl_cmd)
{
  const std::string wrapped = "join [lsort [" + tcl_cmd + "]] \\n";
  std::vector<std::string> items;
  if (Tcl_Eval(interp, wrapped.c_str()) != TCL_OK) {
    Tcl_ResetResult(interp);
    return items;
  }
  std::istringstream stream(Tcl_GetStringResult(interp));
  std::string item;
  while (std::getline(stream, item)) {
    if (!item.empty()) {
      items.push_back(std::move(item));
    }
  }
  Tcl_ResetResult(interp);
  return items;
}

// Returns true if this token position is the start of a Tcl command
// (i.e. nothing meaningful precedes word_start, or only an opening `[`).
bool isCommandPosition(const std::string& line, int word_start)
{
  return findEnclosingCommand(line, word_start).empty();
}

void addVariableCompletions(Tcl_Interp* interp,
                            const std::string& prefix,
                            std::vector<std::string>& out)
{
  const std::string var_prefix = prefix.substr(1);  // strip leading '$'
  const bool starts_with_colon = !var_prefix.empty() && var_prefix[0] == ':';

  // Split the typed text into (namespace context, leaf).  A trailing single
  // colon counts as the start of a `::` separator.  Examples:
  //   "dpl"        -> ns_ctx="",       leaf="dpl"
  //   "sta::cmd"   -> ns_ctx="::sta",  leaf="cmd"
  //   "sta::"      -> ns_ctx="::sta",  leaf=""
  //   "::sta::"    -> ns_ctx="::sta",  leaf=""
  //   "::"         -> ns_ctx="::",     leaf=""
  std::string head = var_prefix;
  if (!head.empty() && head.back() == ':'
      && (head.size() == 1 || head[head.size() - 2] != ':')) {
    head.push_back(':');
  }
  std::string ns_ctx;
  std::string leaf;
  const auto last_sep = head.rfind("::");
  if (last_sep == std::string::npos) {
    leaf = head;
  } else {
    ns_ctx = head.substr(0, last_sep);
    leaf = head.substr(last_sep + 2);
    if (ns_ctx.empty()) {
      ns_ctx = "::";  // user typed leading "::"
    } else if (ns_ctx.size() < 2 || ns_ctx[0] != ':' || ns_ctx[1] != ':') {
      ns_ctx = "::" + ns_ctx;  // normalize to fully-qualified
    }
  }

  // 1. Variables in ns_ctx matching leaf*.  `info vars` accepts a qualified
  // pattern; an empty ns_ctx falls back to the global namespace.
  const std::string var_pattern
      = ns_ctx.empty() ? leaf + "*" : ns_ctx + "::" + leaf + "*";
  size_t var_count = 0;
  for (auto var : getTclList(interp, "info vars " + var_pattern)) {
    if (!starts_with_colon && var.size() >= 2 && var[0] == ':'
        && var[1] == ':') {
      var = var.substr(2);
    }
    std::string candidate = "$" + std::move(var);
    if (candidate.size() >= prefix.size() && candidate.starts_with(prefix)) {
      out.push_back(std::move(candidate));
      ++var_count;
    }
  }

  // 2. Child namespaces of ns_ctx matching leaf*.  Emit each with a
  // trailing `::` so the next TAB descends into it.
  const std::string ns_cmd
      = ns_ctx.empty() ? "namespace children" : "namespace children " + ns_ctx;
  size_t ns_count = 0;
  for (auto ns : getTclList(interp, ns_cmd)) {
    if (!starts_with_colon && ns.size() >= 2 && ns[0] == ':' && ns[1] == ':') {
      ns = ns.substr(2);
    }
    std::string candidate = "$" + std::move(ns) + "::";
    if (candidate.size() >= prefix.size() && candidate.starts_with(prefix)) {
      out.push_back(std::move(candidate));
      ++ns_count;
    }
  }

  // 3. Fallback when the namespace has no `$`-completable content: many
  // OpenROAD namespaces (e.g. ::drt, ::dpl, ::cts) hold only commands, so
  // `$drt::` + TAB would otherwise return nothing.  Surface the commands
  // *without* the leading `$` — accepting the candidate then replaces the
  // entire `$<prefix>` token and yields valid Tcl (e.g. the input
  // `puts $drt::rep` becomes `puts drt::report_constraints`).
  if (var_count == 0 && ns_count == 0 && !ns_ctx.empty()) {
    const std::string cmd_pattern = ns_ctx + "::" + leaf + "*";
    const std::string prefix_no_dollar = prefix.substr(1);
    for (auto cmd : getTclList(interp, "info commands " + cmd_pattern)) {
      if (!starts_with_colon && cmd.size() >= 2 && cmd[0] == ':'
          && cmd[1] == ':') {
        cmd = cmd.substr(2);
      }
      if (cmd.size() >= prefix_no_dollar.size()
          && cmd.starts_with(prefix_no_dollar)) {
        out.push_back(std::move(cmd));
      }
    }
  }
}

void addArgumentFlags(Tcl_Interp* interp,
                      const std::string& cmd_name,
                      const std::string& prefix,
                      std::vector<std::string>& out)
{
  if (cmd_name.empty()) {
    return;
  }
  const std::string tcl_cmd = "if {[info exists sta::cmd_args(" + cmd_name
                              + ")]} { set sta::cmd_args(" + cmd_name
                              + ") } else { list }";
  if (Tcl_Eval(interp, tcl_cmd.c_str()) != TCL_OK) {
    Tcl_ResetResult(interp);
    return;
  }
  const std::string args_str = Tcl_GetStringResult(interp);
  Tcl_ResetResult(interp);
  if (args_str.empty()) {
    return;
  }

  static const std::regex kArgMatcher("-[a-zA-Z0-9_]+");
  std::set<std::string> unique_args;
  for (auto it
       = std::sregex_iterator(args_str.begin(), args_str.end(), kArgMatcher);
       it != std::sregex_iterator{};
       ++it) {
    unique_args.insert(it->str());
  }
  for (const auto& arg : unique_args) {
    if (prefix.size() <= 1 || arg.substr(0, prefix.size()) == prefix) {
      out.push_back(arg);
    }
  }
}

void addCommandCompletions(Tcl_Interp* interp,
                           const std::string& prefix,
                           std::vector<std::string>& out)
{
  // Collect into a std::set to dedup across the three sources (the same
  // command often appears in more than one) and to keep results sorted.
  std::set<std::string> all;

  // 1. OpenROAD/OpenSTA commands registered via define_cmd_args.
  for (auto& cmd : getTclList(interp, "array names sta::cmd_args")) {
    all.insert(std::move(cmd));
  }

  // 2. Every command visible at the current (global) scope: Tcl builtins
  //    (`puts`, `set`, `if`, `foreach`, …) plus user-defined procs.
  for (auto& cmd : getTclList(interp, "info commands")) {
    all.insert(std::move(cmd));
  }

  // 3. Fully-qualified commands inside child namespaces (e.g. sta::, odb::).
  for (const auto& ns : getTclList(interp, "namespace children")) {
    for (auto ns_cmd : getTclList(interp, "info commands " + ns + "::*")) {
      if (ns_cmd.size() > 2 && ns_cmd[0] == ':' && ns_cmd[1] == ':') {
        ns_cmd = ns_cmd.substr(2);
      }
      all.insert(std::move(ns_cmd));
    }
  }

  if (prefix.empty()) {
    out.insert(out.end(), all.begin(), all.end());
    return;
  }
  const bool add_colons = prefix[0] == ':';
  for (const auto& c : all) {
    std::string target = c;
    if (add_colons && !c.empty() && c[0] != ':') {
      target = "::" + c;
    }
    if (target.substr(0, prefix.size()) == prefix) {
      out.push_back(add_colons && !c.empty() && c[0] != ':' ? "::" + c : c);
    }
  }
}

// Filesystem completion at argument positions.  Glob `prefix*` relative to
// either CWD (if prefix has no path) or the parent directory (if it does).
// Directories get a trailing `/` so successive TABs descend into them.
void addFileCompletions(const std::string& prefix,
                        std::vector<std::string>& out)
{
  namespace fs = std::filesystem;
  fs::path prefix_path(prefix);
  fs::path dir;
  std::string leaf;
  if (prefix.empty()) {
    dir = fs::current_path();
    leaf = "";
  } else if (prefix.back() == '/') {
    dir = prefix;
    leaf = "";
  } else if (prefix_path.has_parent_path()) {
    dir = prefix_path.parent_path();
    leaf = prefix_path.filename().string();
  } else {
    dir = fs::current_path();
    leaf = prefix;
  }

  std::error_code ec;
  if (!fs::is_directory(dir, ec)) {
    return;
  }

  // We render results back relative to whatever the user typed: if they
  // typed `foo/ba`, completion candidates look like `foo/bar/` or
  // `foo/baz.lef`.
  std::string render_prefix;
  if (!prefix.empty() && prefix.back() == '/') {
    render_prefix = prefix;
  } else if (prefix_path.has_parent_path()) {
    render_prefix = prefix_path.parent_path().string() + "/";
  }

  std::vector<std::string> matches;
  for (const auto& entry : fs::directory_iterator(dir, ec)) {
    const std::string name = entry.path().filename().string();
    if (!leaf.empty() && !name.starts_with(leaf)) {
      continue;
    }
    if (leaf.empty() && !name.empty() && name[0] == '.'
        && (prefix.empty() || prefix.back() == '/')) {
      // Hide dotfiles unless the user explicitly asked.
      continue;
    }
    std::string candidate = render_prefix + name;
    if (entry.is_directory(ec)) {
      candidate.push_back('/');
    }
    matches.push_back(std::move(candidate));
  }
  std::ranges::sort(matches);
  out.insert(out.end(),
             std::make_move_iterator(matches.begin()),
             std::make_move_iterator(matches.end()));
}

}  // namespace

TclCompletion completeTcl(Tcl_Interp* interp,
                          const std::string& line,
                          int cursor_pos_in)
{
  TclCompletion r;
  int cursor_pos = cursor_pos_in;
  if (cursor_pos < 0) {
    cursor_pos = static_cast<int>(line.size());
  }
  cursor_pos = std::min(cursor_pos, static_cast<int>(line.size()));

  r.replace_start = findWordStart(line, cursor_pos);
  r.replace_end = cursor_pos;
  r.prefix = line.substr(r.replace_start, cursor_pos - r.replace_start);

  if (!r.prefix.empty() && r.prefix[0] == '$') {
    r.mode = "variables";
    addVariableCompletions(interp, r.prefix, r.completions);
  } else if (!r.prefix.empty() && r.prefix[0] == '-') {
    r.mode = "arguments";
    const std::string cmd = findEnclosingCommand(line, r.replace_start);
    addArgumentFlags(interp, cmd, r.prefix, r.completions);
  } else if (isCommandPosition(line, r.replace_start)) {
    r.mode = "commands";
    addCommandCompletions(interp, r.prefix, r.completions);
  } else {
    r.mode = "files";
    addFileCompletions(r.prefix, r.completions);
  }
  return r;
}

}  // namespace ord
