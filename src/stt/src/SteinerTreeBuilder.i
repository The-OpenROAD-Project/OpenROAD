// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

%{

#include "stt/SteinerTreeBuilder.h"
#include "LinesRenderer.h"
#include "stt/pd.h"
#include "stt/flute.h"
#include "gui/gui.h"
#include "ord/OpenRoad.hh"
#include "odb/db.h"
#include <vector>

namespace ord {
// Defined in OpenRoad.i
stt::SteinerTreeBuilder* getSteinerTreeBuilder();
utl::Logger* getLogger();
}  // namespace ord

using ord::getSteinerTreeBuilder;
using odb::dbNet;
%}

%include "../../Exception.i"

%import <std_vector.i>
namespace std {
%template(xy) vector<int>;
}

%inline %{

namespace stt {

void
set_routing_alpha_cmd(float alpha)
{
  getSteinerTreeBuilder()->setAlpha(alpha);
}

void
set_net_alpha(odb::dbNet* net, float alpha)
{
  getSteinerTreeBuilder()->setNetAlpha(net, alpha);
}

void
set_min_fanout_alpha(int num_pins, float alpha)
{
  getSteinerTreeBuilder()->setMinFanoutAlpha(num_pins, alpha);
}

void
set_min_hpwl_alpha(int hpwl, float alpha)
{
  getSteinerTreeBuilder()->setMinHPWLAlpha(hpwl, alpha);
}

void report_flute_tree(std::vector<int> x,
                       std::vector<int> y,
                       int drvr_index)
{
  const int flute_accuracy = 3;
  utl::Logger *logger = ord::getLogger();
  auto builder = getSteinerTreeBuilder();
  stt::Tree tree = builder->flute(x, y, flute_accuracy);
  stt::reportSteinerTree(tree, x[drvr_index], y[drvr_index], logger);
}

void
report_pd_tree(std::vector<int> x,
               std::vector<int> y,
               int drvr_index,
               float alpha)
{
  utl::Logger *logger = ord::getLogger();
  stt::Tree tree = pdr::primDijkstra(x, y, drvr_index, alpha, logger);
  stt::reportSteinerTree(tree, x[drvr_index], y[drvr_index], logger);
}

void
report_stt_tree(std::vector<int> x,
                std::vector<int> y,
                int drvr_index,
                float alpha)
{
  utl::Logger *logger = ord::getLogger();
  auto builder = getSteinerTreeBuilder();

  auto tree = builder->makeSteinerTree(x, y, drvr_index, alpha);
  stt::reportSteinerTree(tree, x[drvr_index], y[drvr_index], logger);
}

void
highlight_stt_tree(std::vector<int> x,
                   std::vector<int> y,
                   int drvr_index,
                   float alpha)
{
  auto builder = getSteinerTreeBuilder();
  auto tree = builder->makeSteinerTree(x, y, drvr_index, alpha);

  gui::Gui *gui = gui::Gui::get();
  stt::highlightSteinerTree(tree, gui);
}

void
highlight_pd_tree(std::vector<int> x,
                  std::vector<int> y,
                  int drvr_index,
                  float alpha)
{
  utl::Logger *logger = ord::getLogger();
  gui::Gui *gui = gui::Gui::get();
  stt::Tree tree = pdr::primDijkstra(x, y, drvr_index, alpha, logger);
  stt::highlightSteinerTree(tree, gui);
}

void
highlight_flute_tree(std::vector<int> x,
                     std::vector<int> y)
{
  gui::Gui *gui = gui::Gui::get();
  auto builder = getSteinerTreeBuilder();
  stt::Tree tree = builder->flute(x, y, 3);
  stt::highlightSteinerTree(tree, gui);
}

} // namespace

%} // inline
