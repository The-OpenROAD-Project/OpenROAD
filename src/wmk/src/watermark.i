// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

%{
#include <string>
#include "ord/OpenRoad.hh"
#include "wmk/Watermark.h"
%}

%include "../../Exception.i"
%include <std_string.i>

%{
#include <array>
#include <cstdint>
#include "HmacSha256.h"
%}

%inline %{

int
set_routing_watermark_cmd(const char* message, double fraction)
{
  auto* w = ord::OpenRoad::openRoad()->getWatermark();
  return w->selectNets(std::string(message), fraction);
}

int
set_routing_watermark_keyed_cmd(const char* key_hex, double fraction)
{
  std::array<std::uint8_t, 32> key;
  if (!wmk::parse_hex_key32(std::string(key_hex), key)) {
    auto* w = ord::OpenRoad::openRoad()->getWatermark();
    (void) w;
    return -1;
  }
  auto* w = ord::OpenRoad::openRoad()->getWatermark();
  return w->selectNetsKeyed(key, fraction);
}

double
report_routing_watermark_cmd(double p)
{
  auto* w = ord::OpenRoad::openRoad()->getWatermark();
  return w->reportWatermark(p);
}

int
clear_routing_watermark_cmd()
{
  auto* w = ord::OpenRoad::openRoad()->getWatermark();
  return w->clearWatermark();
}

%}  // inline
