// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "boost/json/object.hpp"
#include "boost/json/value.hpp"
#include "boost/json/value_to.hpp"
#include "color.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "tcl.h"
#include "tile_generator.h"
#include "utl/Logger.h"

namespace web {

class RequestDispatcher;
class TimingReport;
class ClockTreeReport;

// Sentinel string set as the Tcl result by WebServer::tclExitHandler
// when the browser-side Tcl `exit`/`quit` is invoked.  TclHandler
// detects this in handleTclEval and converts the response to a clean
// shutdown signal for the browser.
inline constexpr const char* kExitResultMsg = "_WEB_EXITING_";

// Thread-safe Tcl command evaluation.  Log output emitted while the
// command runs is captured by WebLogSink (registered on the logger via
// addSink) and pushed to clients as {"type":"log",...} messages - do
// NOT redirect the logger to a string here.  redirectStringBegin clears
// the entire sink list, which would unhook WebLogSink (and any other
// sink) for the duration of the command and break log streaming.  After
// each eval the optional drain_output hook is invoked so any buffered
// log output reaches clients before the eval response is sent.
struct TclEvaluator
{
  Tcl_Interp* interp;
  utl::Logger* logger;
  std::mutex mutex;
  std::function<void()> drain_output;

  struct Result
  {
    std::string result;
    bool is_error = false;
    // Defaults
    Result() = default;
    Result(const Result&) = default;
    Result& operator=(const Result&) = default;
  };

  Result eval(const std::string& cmd)
  {
    std::lock_guard<std::mutex> lock(mutex);

    // Block dangerous Tcl commands over the web interface.
    // This provides defense-in-depth alongside the 127.0.0.1 bind.
    static const std::vector<std::string> blocked = {
        "exec ", "open ", "socket ", "load ", "source "};
    for (const auto& pattern : blocked) {
      if (cmd.find(pattern) != std::string::npos) {
        Result r;
        r.is_error = true;
        r.result = "Blocked for security: '" + pattern
                   + "' is not allowed over the web interface.";
        return r;
      }
    }

    const int rc = Tcl_Eval(interp, cmd.c_str());
    Result r;
    r.result = Tcl_GetStringResult(interp);
    r.is_error = (rc != TCL_OK);
    if (drain_output) {
      drain_output();
    }
    return r;
  }
};