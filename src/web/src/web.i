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

void
web_server_wait_cmd()
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  server->waitForStop();
  // If `exit` was typed in the browser tcl widget, do the real process
  // exit here on the main thread (workers are already joined).
  if (server->exitRequested()) {
    std::exit(EXIT_SUCCESS);
  }
}

void
web_server_stop_cmd()
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
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
