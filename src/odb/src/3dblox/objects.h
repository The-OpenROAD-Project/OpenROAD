// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <string>
#include <vector>

namespace odb {

struct Coordinate
{
  double x, y;
  Coordinate(double x_val = 0.0, double y_val = 0.0) : x(x_val), y(y_val) {}
};

struct ChipletRegion
{
  std::string name;
  std::string bmap;
  std::string pmap;
  std::string side;
  std::string layer;
  std::string gds_layer;
  std::vector<Coordinate> coords;
};

struct ChipletExternal
{
  std::vector<std::string> lef_files;
  std::string def_file;
};

struct ChipletDef
{
  std::string name;
  std::string type;

  // Design area components
  double design_width = -1.0;
  double design_height = -1.0;
  Coordinate offset;

  double seal_ring_left = -1.0;
  double seal_ring_bottom = -1.0;
  double seal_ring_right = -1.0;
  double seal_ring_top = -1.0;

  double scribe_line_left = -1.0;
  double scribe_line_bottom = -1.0;
  double scribe_line_right = -1.0;
  double scribe_line_top = -1.0;

  double thickness = -1.0;
  double shrink = -1.0;
  bool tsv = false;

  std::map<std::string, ChipletRegion> regions;
  ChipletExternal external;
};

struct Header
{
  std::string version;
  std::string unit;
  int precision = 1;
  std::vector<std::string> includes;
};

struct DbvData
{
  std::vector<std::string> includes;
  Header header;
  std::map<std::string, ChipletDef> chiplet_defs;
};

struct DesignExternal
{
  std::string verilog_file;
};

struct DesignDef
{
  std::string name;
  DesignExternal external;
};

struct ChipletInstExternal
{
  std::string verilog_file;
  std::string sdc_file;
  std::string def_file;
};

struct ChipletInst
{
  std::string name;
  std::string reference;
  ChipletInstExternal external;

  // Stack information
  Coordinate loc;
  double z = 0.0;
  std::string orient;
};

struct Connection
{
  std::string name;
  std::string top;
  std::string bot;
  double thickness = 0.0;
};

struct DbxData
{
  std::vector<std::string> includes;
  Header header;
  DesignDef design;
  std::map<std::string, ChipletInst> chiplet_instances;
  std::map<std::string, Connection> connections;
};

}  // namespace odb