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
  std::vector<std::string> tech_lef_files;
  std::vector<std::string> lib_files;
  std::string def_file;
  std::string verilog_file;
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
  Header header;
  DesignDef design;
  std::map<std::string, ChipletInst> chiplet_instances;
  std::map<std::string, Connection> connections;
};

struct BumpMapEntry
{
  const std::string bump_inst_name;
  const std::string bump_cell_type;
  const double x{0.0};
  const double y{0.0};
  const std::string port_name;
  const std::string net_name;

  BumpMapEntry() = default;
  BumpMapEntry(const std::string& inst_name,
               const std::string& cell_type,
               double x_coord,
               double y_coord,
               const std::string& port,
               const std::string& net)
      : bump_inst_name(inst_name),
        bump_cell_type(cell_type),
        x(x_coord),
        y(y_coord),
        port_name(port),
        net_name(net)
  {
  }
};

struct BumpMapData
{
  std::vector<BumpMapEntry> entries;
};

}  // namespace odb
