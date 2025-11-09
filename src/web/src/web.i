// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

%{
#include "ord/OpenRoad.hh"
#include "web/web.h"
%}

%include "../../Exception.i"

%inline %{

namespace web {

void
web_server_cmd()
{
  web::WebServer *server = ord::OpenRoad::openRoad()->getWebServer();
  server->serve();
}

} // namespace web

%} // inline
