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
web_server_cmd(const char* host, int port, const char* doc_root)
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  server->serve(host, port, doc_root);
}

} // namespace web

%} // inline
