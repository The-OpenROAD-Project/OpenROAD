// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Linenoise-backed implementation of the legacy tcl_readline_setup
// interface.  The file, header, and function name (SetupTclReadlineLibrary)
// are preserved so that OpenSTA's standalone `sta` binary
// (src/sta/app/Main.cc) — which depends on this target — continues to
// build unchanged.
//
// What this used to do: load the real `tclreadline` Tcl package, which
// wraps GNU readline (GPL).  The combined binary picked up GPL contagion.
//
// What it now does: register a small `::tclreadline` Tcl namespace
// implemented in C++ on top of linenoise (BSD-2-Clause).  OpenSTA's
// Init.tcl gates on `[info exists tclreadline::version]` and calls
// `::tclreadline::Loop`; OpenROAD's Main.cc calls `::tclreadline::Loop`
// directly.  Both paths reach the same linenoise REPL below.

#include "tcl_readline_setup.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

#include "cli_completer.h"
#include "linenoise.h"
#include "tcl.h"
#include "tclDecls.h"

namespace ord {

namespace {

// linenoise's completion callback receives the line buffer but no interp,
// so we stash the interp set up by SetupTclReadlineLibrary here.  The CLI
// is single-threaded so a file-scope pointer is fine.
Tcl_Interp* g_interp = nullptr;

std::string historyFilePath()
{
  const char* home = std::getenv("HOME");
  std::filesystem::path p = home ? home : ".";
  p /= ".openroad_history";
  return p.string();
}

void completionCallback(const char* buf, linenoiseCompletions* lc)
{
  if (!g_interp) {
    return;
  }
  const int len = static_cast<int>(std::strlen(buf));
  const TclCompletion r = completeTcl(g_interp, buf, len);
  // Tell linenoise where the current word starts; each candidate replaces
  // buf[r.replace_start .. cursor].
  lc->replace_start = static_cast<size_t>(r.replace_start);
  for (const auto& c : r.completions) {
    linenoiseAddCompletion(lc, c.c_str());
  }
}

int loopCmd(ClientData /*cd*/,
            Tcl_Interp* interp,
            int /*objc*/,
            Tcl_Obj* const /*objv*/[])
{
  g_interp = interp;
  linenoiseSetCompletionCallback(&completionCallback);
  linenoiseHistorySetMaxLen(1000);
  const std::string histfile = historyFilePath();
  linenoiseHistoryLoad(histfile.c_str());

  std::string buf;
  while (true) {
    const char* prompt = buf.empty() ? "openroad> " : "...> ";
    char* line = linenoise(prompt);
    if (line == nullptr) {
      // EOF (Ctrl-D) or read error — exit cleanly.
      std::printf("\n");
      std::exit(EXIT_SUCCESS);
    }
    if (*line != '\0') {
      linenoiseHistoryAdd(line);
      linenoiseHistorySave(histfile.c_str());
    }
    if (!buf.empty()) {
      buf += '\n';
    }
    buf += line;
    linenoiseFree(line);

    if (!Tcl_CommandComplete(buf.c_str())) {
      continue;
    }

    const int rc = Tcl_RecordAndEval(interp, buf.c_str(), 0);
    const char* result = Tcl_GetStringResult(interp);
    if (rc == TCL_OK) {
      if (result && *result) {
        std::printf("%s\n", result);
      }
    } else {
      std::fprintf(stderr, "%s\n", result ? result : "");
    }
    buf.clear();
  }
}

// No-op stub: real tclreadline::readline accepts many subcommands
// (`customcompleter`, `builtincompleter`, `read`, `eof`, …).  We absorb
// any args so legacy `setup_tclreadline`-style Tcl scripts don't break,
// but configuration happens in C++ at registration time.
int readlineCmd(ClientData /*cd*/,
                Tcl_Interp* /*interp*/,
                int /*objc*/,
                Tcl_Obj* const /*objv*/[])
{
  return TCL_OK;
}

// `::tclreadline::complete <line> <cursor_pos>` — invoke the shared
// completer and return its candidates as a Tcl list.  Used by the
// `ord_tclsh_completion` regression test; also handy when debugging
// completion behavior from the REPL.
int completeCmd(ClientData /*cd*/,
                Tcl_Interp* interp,
                int objc,
                Tcl_Obj* const objv[])
{
  if (objc != 3) {
    Tcl_WrongNumArgs(interp, 1, objv, "line cursor_pos");
    return TCL_ERROR;
  }
  const std::string line = Tcl_GetString(objv[1]);
  int cursor = 0;
  if (Tcl_GetIntFromObj(interp, objv[2], &cursor) != TCL_OK) {
    return TCL_ERROR;
  }
  const TclCompletion r = completeTcl(interp, line, cursor);
  Tcl_Obj* list = Tcl_NewListObj(0, nullptr);
  for (const auto& c : r.completions) {
    Tcl_ListObjAppendElement(
        interp, list, Tcl_NewStringObj(c.data(), static_cast<int>(c.size())));
  }
  Tcl_SetObjResult(interp, list);
  return TCL_OK;
}

}  // namespace

int SetupTclReadlineLibrary(Tcl_Interp* interp)
{
  // Create the namespace and version variable that downstream Tcl code
  // (e.g. src/sta/tcl/Init.tcl) probes to decide whether interactive
  // line editing is available.
  if (Tcl_Eval(interp,
               "namespace eval ::tclreadline { variable version "
               "{linenoise-shim} }")
      != TCL_OK) {
    return TCL_ERROR;
  }

  Tcl_CreateObjCommand(
      interp, "::tclreadline::Loop", &loopCmd, nullptr, nullptr);
  Tcl_CreateObjCommand(
      interp, "::tclreadline::readline", &readlineCmd, nullptr, nullptr);
  Tcl_CreateObjCommand(
      interp, "::tclreadline::complete", &completeCmd, nullptr, nullptr);

  // Default prompts; OpenROAD's Main.cc invokes ::tclreadline::Loop after
  // initOpenRoad so these strings only matter if a downstream script
  // overrides them (OpenSTA's Init.tcl does — that's fine, last write wins
  // because the proc is plain Tcl, not a C-implemented command).
  Tcl_Eval(interp,
           "proc ::tclreadline::prompt1 {} { return {openroad> } }\n"
           "proc ::tclreadline::prompt2 {} { return {...> } }\n");

  return TCL_OK;
}

}  // namespace ord
