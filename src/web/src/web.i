// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

%{
#include "ord/OpenRoad.hh"
#include "web/web.h"
%}

%include "../../Exception.i"

%inline %{

namespace web {

void
web_server_cmd(int port, const char* doc_root)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  server->serve(port, doc_root);
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

} // namespace web

%} // inline
