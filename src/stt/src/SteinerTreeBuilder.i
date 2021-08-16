/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

%{

#include "stt/SteinerTreeBuilder.h"
#include "stt/pdrev.h"
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
%template(pdrev_xy) vector<int>;
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
                       std::vector<int> y)
{
  const int flute_accuracy = 3;
  utl::Logger *logger = ord::getLogger();
  stt::Tree tree = flt::flute(x, y, flute_accuracy);
  stt::reportSteinerTree(tree, logger);
}

void
report_pd_tree(std::vector<int> x,
               std::vector<int> y,
               int drvr_index,
               float alpha)
{
  utl::Logger *logger = ord::getLogger();
  stt::Tree tree = pdr::primDijkstra(x, y, drvr_index, alpha, logger);
  stt::reportSteinerTree(tree, logger);
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
  stt::reportSteinerTree(tree, logger);
}

void
highlight_stt_tree(std::vector<int> x,
                   std::vector<int> y,
                   int drvr_index,
                   float alpha)
{
  utl::Logger *logger = ord::getLogger();
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
report_pdrev_tree(std::vector<int> x,
                  std::vector<int> y,
                  int drvr_index,
                  float alpha)
{
  utl::Logger *logger = ord::getLogger();
  stt::Tree tree = pdr::primDijkstraRevII(x, y, drvr_index, alpha, logger);
  stt::reportSteinerTree(tree, logger);
}

} // namespace

%} // inline
