// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

%{
#include <cstdint>
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
  server->serve((uint16_t) port, doc_root);
}

} // namespace web

%} // inline
