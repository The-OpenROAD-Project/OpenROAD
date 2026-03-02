// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <memory>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace web {

struct Color;
class Search;

class TileGenerator;

// A layout web server

class WebServer
{
 public:
  WebServer(odb::dbDatabase* db, utl::Logger* logger);
  ~WebServer();

  void serve(const std::string& doc_root = "");

 private:

  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;
  std::shared_ptr<TileGenerator> generator_;
};

}  // namespace web
