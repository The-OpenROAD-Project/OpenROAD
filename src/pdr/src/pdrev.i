/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
// All rights reserved.
//
// BSD 3-Clause License
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

#include <vector>
#include "pdr/pdrev.h"
#include "gui/gui.h"

namespace ord {
utl::Logger *
getLogger();
}

%}

%include "../../Exception.i"

%import <std_vector.i>
namespace std {
%template(pdrev_xy) vector<int>;
}

%inline %{

void
report_pd_tree(const std::vector<int> &x,
               const std::vector<int> &y,
               int drvr_index,
               float alpha)
{
  utl::Logger *logger = ord::getLogger();
  stt::Tree tree = pdr::primDijkstra(x, y, drvr_index, alpha, logger);
  pdr::reportSteinerTree(tree, logger);
}

void
highlight_pd_tree(const std::vector<int> &x,
                  const std::vector<int> &y,
                  int drvr_index,
                  float alpha)
{
  utl::Logger *logger = ord::getLogger();
  gui::Gui *gui = gui::Gui::get();
  stt::Tree tree = pdr::primDijkstra(x, y, drvr_index, alpha, logger);
  pdr::highlightSteinerTree(tree, gui);
}

void
report_pdII_tree(const std::vector<int> &x,
                 const std::vector<int> &y,
                 int drvr_index,
                 float alpha)
{
  utl::Logger *logger = ord::getLogger();
  stt::Tree tree = pdr::primDijkstraRevII(x, y, drvr_index, alpha, logger);
  pdr::reportSteinerTree(tree, logger);
}

%} // inline
