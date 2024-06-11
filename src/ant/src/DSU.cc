// BSD 3-Clause License
//
// Copyright (c) 2020, MICL, DD-Lab, University of Michigan
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

#include "DSU.hh"

namespace ant {

DSU::DSU() = default;
DSU::~DSU() = default;

void DSU::initDsu(const int node_count)
{
  dsu_parent_.resize(node_count);
  dsu_size_.resize(node_count);
  for (int i = 0; i < node_count; i++) {
    dsu_size_[i] = 1;
    dsu_parent_[i] = i;
  }
}

int DSU::findSet(int u)
{
  if (u == dsu_parent_[u]) {
    return u;
  }
  return dsu_parent_[u] = findSet(dsu_parent_[u]);
}

bool DSU::dsuSame(int u, int v)
{
  return findSet(u) == findSet(v);
}

void DSU::unionSet(int u, int v)
{
  u = findSet(u);
  v = findSet(v);
  // union the smaller set to bigger set
  if (dsu_size_[u] < dsu_size_[v]) {
    std::swap(u, v);
  }
  dsu_parent_[v] = u;
  dsu_size_[u] += dsu_size_[v];
}

}  // namespace ant