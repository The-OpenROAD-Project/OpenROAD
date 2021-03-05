/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#pragma once

//
// Assigns cluster labels (0, ..., nclust-1) to the n points such
// that the cluster result is split into nclust clusters.
//
// Input arguments:
//   n      = number of observables
//   merge  = clustering result in R format
//   nclust = number of clusters
// Output arguments:
//   labels = allocated integer array of size n for result
//
void cutreeK(int n, const int* merge, int nclust, int* labels);

//
// Assigns cluster labels (0, ..., nclust-1) to the n points such
// that the hierarchical clsutering is stopped at cluster distance cdist
//
// Input arguments:
//   n      = number of observables
//   merge  = clustering result in R format
//   height = cluster distance at each merge step
//   cdist  = cutoff cluster distance
// Output arguments:
//   labels = allocated integer array of size n for result
//
void cutreeCdist(int n, const int* merge, double* height, double cdist, int* labels);

//
// Hierarchical clustering with one of Daniel Muellner's fast algorithms
//
// Input arguments:
//   n       = number of observables
//   distmat = condensed distance matrix, i.e. an n*(n-1)/2 array representing
//             the upper triangle (without diagonal elements) of the distance
//             matrix, e.g. for n=4:
//               d00 d01 d02 d03
//               d10 d11 d12 d13   ->  d01 d02 d03 d12 d13 d23
//               d20 d21 d22 d23
//               d30 d31 d32 d33
//   method  = cluster metric (see enum hclust_fast_methods)
// Output arguments:
//   merge   = allocated (n-1)x2 matrix (2*(n-1) array) for storing result.
//             Result follows R hclust convention:
//              - observabe indices start with one
//              - merge[i][] contains the merged nodes in step i
//              - merge[i][j] is negative when the node is an atom
//   height  = allocated (n-1) array with distances at each merge step
// Return code:
//   0 = ok
//   1 = invalid method
//
int hclustFast(int n, double* distmat, int method, int* merge, double* height);
enum hclustFastMethods {
  // complete link with the nearest-neighbor-chain algorithm (Murtagh, 1984)
  HCLUSTMETHODCOMPLETE = 1,
};
