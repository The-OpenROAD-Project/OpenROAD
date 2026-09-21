// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

struct Tcl_Interp;

namespace ord {

// Result of TAB completion for a Tcl input buffer.
//
//   completions     candidate replacements for line[replace_start..replace_end)
//   replace_start   inclusive start index in the input line
//   replace_end     exclusive end index in the input line
//   mode            "commands" | "arguments" | "variables" | "files"
//   prefix          line[replace_start..replace_end) — convenience for callers
struct TclCompletion
{
  std::vector<std::string> completions;
  int replace_start = 0;
  int replace_end = 0;
  std::string mode;
  std::string prefix;
};

// Compute completions for `line` at `cursor_pos` using Tcl introspection
// on `interp` (`info vars`, `namespace children`, `info commands`,
// `array names sta::cmd_args`).  At argument positions, also returns
// filesystem matches.
//
// The caller is responsible for thread safety:
//   * the web request handler holds TclEvaluator::mutex,
//   * the CLI linenoise loop runs single-threaded.
TclCompletion completeTcl(Tcl_Interp* interp,
                          const std::string& line,
                          int cursor_pos);

}  // namespace ord
