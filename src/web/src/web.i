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

int
gif_start_cmd(const char* filename)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  return server->gifStart(filename);
}

void
gif_add_cmd(int key, int x0, int y0, int x1, int y1,
            int width, double resolution, int delay,
            const char* vis_json)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  // key < 0 and delay <= 0 mean "use default" (most-recent GIF / default delay).
  std::optional<int> key_opt;
  if (key >= 0) {
    key_opt = key;
  }
  std::optional<int> delay_opt;
  if (delay > 0) {
    delay_opt = delay;
  }
  server->gifAddFrame(key_opt, odb::Rect(x0, y0, x1, y1), width, resolution,
                      delay_opt, vis_json ? vis_json : "");
}

void
gif_end_cmd(int key)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  std::optional<int> key_opt;
  if (key >= 0) {
    key_opt = key;
  }
  server->gifEnd(key_opt);
}

const char*
create_toolbar_button_cmd(const char* name, const char* text,
                          const char* script, const char* icon,
                          const char* tooltip, bool toggle,
                          const char* script_off, bool echo)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  // The returned std::string is copied into a static so the char* stays
  // valid after this call returns to Tcl (same idiom as gui.i).
  static std::string key;
  key = server->addToolbarButton(name, text, script, icon, tooltip, toggle,
                                 script_off, echo);
  return key.c_str();
}

void
remove_toolbar_button_cmd(const char* name)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  server->removeToolbarButton(name);
}

const char*
create_menu_item_cmd(const char* name, const char* path, const char* text,
                     const char* script, const char* shortcut, bool echo)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  static std::string key;
  key = server->addMenuItem(name, path, text, script, shortcut, echo);
  return key.c_str();
}

void
remove_menu_item_cmd(const char* name)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  server->removeMenuItem(name);
}

} // namespace web

%} // inline
