// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <memory>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace web {

struct Color;

// A simple web server

class WebServer
{
 public:
  WebServer(odb::dbDatabase* db, utl::Logger* logger);

  void serve();

 private:
  odb::Rect getBounds() const;
  std::vector<unsigned char> generateTile(const std::string& layer,
                                          int z,
                                          int x,
                                          int y);
  void setPixel(std::vector<unsigned char>& image,
                int x,
                int y,
                const Color& c);

  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;
  static constexpr int tile_size_in_pixel = 256;
};

}  // namespace web
