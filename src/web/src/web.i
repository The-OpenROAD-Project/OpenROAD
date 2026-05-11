// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

%{
#include <cstdlib>

#include "ord/OpenRoad.hh"
#include "web/web.h"
%}

%include "../../Exception.i"

%inline %{

namespace web {

void
web_server_cmd(int port)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  server->serve(port);
}

// Block the calling Tcl_Eval until web_server -stop or browser-typed
// `exit`.  Used by web_server when called interactively so the
// launching terminal's prompt is suppressed while the server is
// running.  Tears the server down before returning; on exit-request,
// performs std::exit(0) here on the main thread.
void
web_server_wait_cmd()
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  const bool was_exit = server->runEventLoopUntilStop();
  server->stop();
  if (was_exit) {
    std::exit(EXIT_SUCCESS);
  }
}

void
web_server_stop_cmd()
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  // requestStop() just sets a flag and wakes the main-thread Tcl
  // event loop so it can tear the server down.  Safe from any thread:
  // the worker thread that ran this command never tries to join
  // itself.
  server->requestStop();
}

void
save_image_cmd(const char* filename,
               int x0, int y0, int x1, int y1,
               int width, double resolution,
               const char* vis_json)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  server->saveImage(filename, x0, y0, x1, y1, width, resolution,
                    vis_json ? vis_json : "");
}

void
save_report_cmd(const char* filename,
                int max_setup, int max_hold)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  server->saveReport(filename, max_setup, max_hold);
}

} // namespace web

%} // inline
