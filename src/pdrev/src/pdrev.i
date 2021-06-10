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
#include "pdrev/pdrev.h"

namespace ord {
utl::Logger *
getLogger();
}

void
reportPdrevTree(bool use_pd,
                const std::vector<int> &x,
                const std::vector<int> &y,
                int drvr_index,
                float alpha)
{
  PD::PdRev pd(ord::getLogger());
  pd.setAlphaPDII(alpha);
  std::vector<int> x1(x);
  std::vector<int> y1(y);
  // Move driver to pole position.
  std::swap(x1[0], x1[drvr_index]);
  std::swap(y1[0], y1[drvr_index]);
  pd.addNet(x1, y1);
  if (use_pd)
    pd.runPD(alpha);
  else
    pd.runPDII();
  PD::Tree tree = pd.translateTree();
  printf("WL = %d\n", tree.length);
  for (int i = 0; i < 2 * tree.deg - 2; i++) {
    int x1 = tree.branch[i].x;
    int y1 = tree.branch[i].y;
    int parent = tree.branch[i].n;
    int x2 = tree.branch[parent].x;
    int y2 = tree.branch[parent].y;
    int length = abs(x1-x2)+abs(y1-y2);
    printf("%d (%d %d) parent %d length %d\n",
           i, x1, y1, parent, length);
  }
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
  reportPdrevTree(true, x, y, drvr_index, alpha);
}

void
report_pdII_tree(const std::vector<int> &x,
                 const std::vector<int> &y,
                 int drvr_index,
                 float alpha)
{
  reportPdrevTree(false, x, y, drvr_index, alpha);
}

%} // inline
