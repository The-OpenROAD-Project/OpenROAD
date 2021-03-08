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


#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath> // for std::pow, std::sqrt
#include <math.h> // for std::pow, std::sqrt
#include <cstddef> // for std::ptrdiff_t
#include <limits> // for std::numeric_limits<...>::infinity()
#include <algorithm> // for std::fill_n
#include <stdexcept> // for std::runtime_error
#include <string> // for std::string

#include <cfloat> // also for DBL_MAX, DBL_MIN

#include "SinkAgglDriver.h"

namespace cts {

//#include "SinkAgglEngine.cpp"
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
void SinkAgglClustering::cutreeK(int n, const int* merge, int nclust, int* labels) {

  int k,m1,m2,j,l;

  if (nclust > n || nclust < 2) {
    for (j=0; j<n; j++) labels[j] = 0;
    return;
  }

  // assign to each observable the number of its last merge step
  // beware: indices of observables in merge start at 1 (R convention)
  std::vector<int> lastMerge(n, 0);
  for (k=1; k<=(n-nclust); k++) {
    // (m1,m2) = merge[k,]
    m1 = merge[k-1];
    m2 = merge[n-1+k-1];
    if (m1 < 0 && m2 < 0) { // both single observables
      lastMerge[-m1-1] = lastMerge[-m2-1] = k;
	}
	else if (m1 < 0 || m2 < 0) { // one is a cluster
      int temp = -m1;
	    if (m1 < 0) 
        { 
          temp = -m1; 
          m1 = m2; 
        } 
      else 
        {
          temp = -m2;
        }

	    // merging single observable and cluster
	    for(l = 0; l < n; l++){
		    if (lastMerge[l] == m1){
          lastMerge[l] = k;
        }
      }
	    lastMerge[temp-1] = k;
	}
	else { // both cluster
	    for(l=0; l < n; l++) {
		if( lastMerge[l] == m1 || lastMerge[l] == m2 )
		    lastMerge[l] = k;
	    }
    }
  }


  // assign cluster labels
  int label = 0;
  std::vector<int> z(n,-1);
  for (j=0; j<n; j++) {
    if (lastMerge[j] == 0) { // still singleton
      labels[j] = label++;
    } else {
      if (z[lastMerge[j]] < 0) {
        z[lastMerge[j]] = label++;
      }
      labels[j] = z[lastMerge[j]];
    }
  }


}



//
// Assigns cluster labels (0, ..., nclust-1) to the n points such
// that the hierarchical clustering is stopped when cluster distance >= cdist
//
// Input arguments:
//   n      = number of observables
//   merge  = clustering result in R format
//   height = cluster distance at each merge step
//   cdist  = cutoff cluster distance
// Output arguments:
//   labels = allocated integer array of size n for result
//
void SinkAgglClustering::cutreeCdist(int n, const int* merge, double* height, double cdist, int* labels) {

  int k;

  for (k=0; k<(n-1); k++) {
    if (height[k] >= cdist) {
      break;
    }
  }

  cutreeK(n, merge, n-k, labels);
}


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
int SinkAgglClustering::hclustFast(int n, double* distmat, int method,
                                  int* merge, double* height)
{
  
  // call appropriate culstering function
  clusterResult Z2(n-1);
  if (method == HCLUSTMETHODCOMPLETE) {
    // complete link
    NNChainCore<METHODMETRCOMPLETE, double>(n, distmat, NULL, Z2);
  }
  else {
    return 1;
  }

  int* order = new int[n];
  generateRDendrogram<false>(merge, height, order, Z2, n);
  

  delete[] order; // only needed for visualization
  
  
  return 0;
}

// Indexing functions
// D is the upper triangular part of a symmetric (NxN)-matrix
// We require r_ < c_ !
#define D_(r_,c_) ( D[(static_cast<std::ptrdiff_t>(2*N-3-(r_))*(r_)>>1)+(c_)-1] )
// Z is an ((N-1)x4)-array
#define Z_(_r, _c) (Z[(_r)*4 + (_c)])



/* Functions for the update of the dissimilarity array */

void SinkAgglClustering::fComplete( double * const b, const double a )
{
  if (*b < a) *b = a;
}


template <SinkAgglClustering::methodCodes method, typename Members>
void SinkAgglClustering::NNChainCore(const tIndex N, double * const D, 
                                     Members * const members, clusterResult & Z2) {
/*
    N: integer
    D: condensed distance matrix N*(N-1)/2
    Z2: output data structure

    This is the NN-chain algorithm, described on page 86 in the following book:

    Fionn Murtagh, Multidimensional Clustering Algorithms,
    Vienna, Würzburg: Physica-Verlag, 1985.
*/
  tIndex i;

  autoArrayPtr<tIndex> NNChain(N);
  tIndex NNChainTip = 0;

  tIndex idx1, idx2;

  double size1, size2;
  doublyLinkedList activeNodes(N);

  double min;

  for (double const * DD=D; DD!=D+(static_cast<std::ptrdiff_t>(N)*(N-1)>>1);
       ++DD) {
#if HAVE_DIAGNOSTIC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#if HAVE_DIAGNOSTIC
#pragma GCC diagnostic pop
#endif
  }

  for (tIndex j=0; j<N-1; ++j) {
    if (NNChainTip <= 3) {
      NNChain[0] = idx1 = activeNodes.start;
      NNChainTip = 1;

      idx2 = activeNodes.succ[idx1];
      min = D_(idx1,idx2);
      for (i=activeNodes.succ[idx2]; i<N; i=activeNodes.succ[i]) {
        if (D_(idx1,i) < min) {
          min = D_(idx1,i);
          idx2 = i;
        }
      }
    }  // a: idx1   b: idx2
    else {
      NNChainTip -= 3;
      idx1 = NNChain[NNChainTip-1];
      idx2 = NNChain[NNChainTip];
      min = idx1<idx2 ? D_(idx1,idx2) : D_(idx2,idx1);
    }  // a: idx1   b: idx2

    do {
      NNChain[NNChainTip] = idx2;

      for (i=activeNodes.start; i<idx2; i=activeNodes.succ[i]) {
        if (D_(i,idx2) < min) {
          min = D_(i,idx2);
          idx1 = i;
        }
      }
      for (i=activeNodes.succ[idx2]; i<N; i=activeNodes.succ[i]) {
        if (D_(idx2,i) < min) {
          min = D_(idx2,i);
          idx1 = i;
        }
      }

      idx2 = idx1;
      idx1 = NNChain[NNChainTip++];

    } while (idx2 != NNChain[NNChainTip-2]);

    Z2.append(idx1, idx2, min);

    if (idx1>idx2) {
      tIndex tmp = idx1;
      idx1 = idx2;
      idx2 = tmp;
    }

    // Remove the smaller index from the valid indices (activeNodes).
    activeNodes.remove(idx1);

    switch (method) {
    case METHODMETRCOMPLETE:
      /*
      Complete linkage.

      Characteristic: new distances are never shorter than the old distances.
      */
      // Update the distance matrix in the range [start, idx1).
      for (i=activeNodes.start; i<idx1; i=activeNodes.succ[i])
        fComplete(&D_(i, idx2), D_(i, idx1) );
      // Update the distance matrix in the range (idx1, idx2).
      for (; i<idx2; i=activeNodes.succ[i])
        fComplete(&D_(i, idx2), D_(idx1, i) );
      // Update the distance matrix in the range (idx2, N).
      for (i=activeNodes.succ[idx2]; i<N; i=activeNodes.succ[i])
        fComplete(&D_(idx2, i), D_(idx1, i) );
      break;

    default:
      throw std::runtime_error(std::string("Invalid method."));
    }
  }
}

//
// Excerpt from fastcluster_R.cpp
//
// Copyright: Daniel Müllner, 2011 <http://danifold.net>
//



void SinkAgglClustering::orderNodes(const int N, const int * const merge,
                         const tIndex * const nodeSize,int * const order)
{
  /* Parameters:
     N         : number of data points
     merge     : (N-1)×2 array which specifies the node indices which are
                 merged in each step of the clustering procedure.
                 Negative entries -1...-N point to singleton nodes, while
                 positive entries 1...(N-1) point to nodes which are themselves
                 parents of other nodes.
     nodeSize : array of node sizes - makes it easier
     order     : output array of size N

     Runtime: ?(N)
  */
  autoArrayPtr<posNode> queue(N/2);

  int parent;
  int child;
  tIndex pos = 0;

  queue[0].pos = 0;
  queue[0].node = N-2;
  tIndex idx = 1;

  do {
    --idx;
    pos = queue[idx].pos;
    parent = queue[idx].node;

    // First child
    child = merge[parent];
    if (child<0) { // singleton node, write this into the 'order' array.
      order[pos] = -child;
      ++pos;
    }
    else { /* compound node: put it on top of the queue and decompose it
              in a later iteration. */
      queue[idx].pos = pos;
      queue[idx].node = child-1; // convert index-1 based to index-0 based
      ++idx;
      pos += nodeSize[child-1];
    }
    // Second child
    child = merge[parent+N-1];
    if (child<0) {
      order[pos] = -child;
    }
    else {
      queue[idx].pos = pos;
      queue[idx].node = child-1;
      ++idx;
    }
  } while (idx>0);
}

#define size_(r_) ( ((r_<N) ? 1 : nodeSize[r_-N]) )

template <const bool sorted>
void SinkAgglClustering::generateRDendrogram(int * const merge, double * const height,
                         int * const order, clusterResult & Z2, const int N)
{
  // The array "nodes" is a union-find data structure for the cluster
  // identites (only needed for unsorted clusterResult input).
  unionFind nodes(sorted ? 0 : N);
  if (!sorted) {
    std::stable_sort(Z2[0], Z2[N-1]);
  }

  tIndex node1, node2;
  autoArrayPtr<tIndex> nodeSize(N-1);

  for (tIndex i=0; i<N-1; ++i) {
    // Get two data points whose clusters are merged in step i.
    // Find the cluster identifiers for these points.
    if (sorted) {
      node1 = Z2[i]->node1;
      node2 = Z2[i]->node2;
    }
    else {
      node1 = nodes.Find(Z2[i]->node1);
      node2 = nodes.Find(Z2[i]->node2);
      // Merge the nodes in the union-find data structure by making them
      // children of a new node.
      nodes.Union(node1, node2);
    }
    // Sort the nodes in the output array.
    if (node1>node2) {
      tIndex tmp = node1;
      node1 = node2;
      node2 = tmp;
    }
    /* Conversion between labeling conventions.
       Input:  singleton nodes 0,...,N-1
               compound nodes  N,...,2N-2
       Output: singleton nodes -1,...,-N
               compound nodes  1,...,N
    */
    merge[i]     = (node1<N) ? -static_cast<int>(node1)-1
                              : static_cast<int>(node1)-N+1;
    merge[i+N-1] = (node2<N) ? -static_cast<int>(node2)-1
                              : static_cast<int>(node2)-N+1;
    height[i] = Z2[i]->dist;
    nodeSize[i] = size_(node1) + size_(node2);
  }

  orderNodes(N, merge, nodeSize, order);
}
}