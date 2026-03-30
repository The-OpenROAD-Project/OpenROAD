// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <string>

#include "odb/db.h"
#include "tcl.h"
#include "utl/Logger.h"

namespace sta {
class dbSta;
}

namespace web {

struct Color;
class Search;

class TileGenerator;
struct TclEvaluator;

// A layout web server

class WebServer
{
 public:
  WebServer(odb::dbDatabase* db,
            sta::dbSta* sta,
            utl::Logger* logger,
            Tcl_Interp* interp);
  ~WebServer();

  void serve(int port, const std::string& doc_root);

  void saveImage(const std::string& filename,
                 int x0,
                 int y0,
                 int x1,
                 int y1,
                 int width_px,
                 double dbu_per_pixel,
                 const std::string& vis_json);

 private:
  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  Tcl_Interp* interp_ = nullptr;
  std::shared_ptr<TileGenerator> generator_;
};

}  // namespace web
