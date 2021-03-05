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

#include <cmath> // for std::pow, std::sqrt
#include <math.h> // for std::pow, std::sqrt
#include <cstddef> // for std::ptrdiff_t
#include <limits> // for std::numeric_limits<...>::infinity()
#include <algorithm> // for std::fill_n
#include <stdexcept> // for std::runtime_error
#include <string> // for std::string

#include <cfloat> // also for DBL_MAX, DBL_MIN

#ifndef INT32_MAX
#ifdef _MSC_VER
#if _MSC_VER >= 1600
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#else
typedef __int32 int_fast32_t;
typedef __int64 int64_t;
#endif
#else
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#endif
#endif

#define FILL_N std::fill_n
#ifdef _MSC_VER
#if _MSC_VER < 1600
#undef FILL_N
#define FILL_N stdext::unchecked_fill_n
#endif
#endif

// Suppress warnings about (potentially) uninitialized variables.
#ifdef _MSC_VER
	#pragma warning (disable:4700)
#endif

#ifndef HAVE_DIAGNOSTIC
#if __GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ >= 6))
#define HAVE_DIAGNOSTIC 1
#endif
#endif

#ifndef HAVE_VISIBILITY
#if __GNUC__ >= 4
#define HAVE_VISIBILITY 1
#endif
#endif

/* Since the public interface is given by the Python respectively R interface,
 * we do not want other symbols than the interface initalization routines to be
 * visible in the shared object file. The "visibility" switch is a GCC concept.
 * Hiding symbols keeps the relocation table small and decreases startup time.
 * See http://gcc.gnu.org/wiki/Visibility
 */
#if HAVE_VISIBILITY
#pragma GCC visibility push(hidden)
#endif

typedef int_fast32_t tIndex;

// Started Changes

// Started Changes
enum methodCodes {
  // non-Euclidean methods
  METHODMETRCOMPLETE         = 1,
};

// self-destructing array pointer
template <typename type>
class autoArrayPtr{
private:
  type * ptr;
  autoArrayPtr(autoArrayPtr const &); // non construction-copyable
  autoArrayPtr& operator=(autoArrayPtr const &); // non copyable
public:
  autoArrayPtr()
    : ptr(NULL)
  { }
  template <typename index>
  autoArrayPtr(index const size)
    : ptr(new type[size])
  { }
  template <typename index, typename value>
  autoArrayPtr(index const size, value const val)
    : ptr(new type[size])
  {
    FILL_N(ptr, size, val);
  }
  ~autoArrayPtr() {
    delete [] ptr; }
  void free() {
    delete [] ptr;
    ptr = NULL;
  }
  template <typename index>
  void init(index const size) {
    ptr = new type [size];
  }
  template <typename index, typename value>
  void init(index const size, value const val) {
    init(size);
    FILL_N(ptr, size, val);
  }
  inline operator type *() const { return ptr; }
};

struct node {
  tIndex node1, node2;
  double dist;
};

inline bool operator< (const node a, const node b) {
  return (a.dist < b.dist);
}

class clusterResult {
private:
  autoArrayPtr<node> Z;
  tIndex pos;

public:
  clusterResult(const tIndex size)
    : Z(size)
    , pos(0)
  {}

  void append(const tIndex node1, const tIndex node2, const double dist) {
    Z[pos].node1 = node1;
    Z[pos].node2 = node2;
    Z[pos].dist  = dist;
    ++pos;
  }

  node * operator[] (const tIndex idx) const { return Z + idx; }

  /* Define several methods to postprocess the distances. All these functions
     are monotone, so they do not change the sorted order of distances. */

  void sqrt() const {
    for (node * ZZ=Z; ZZ!=Z+pos; ++ZZ) {
      ZZ->dist = std::sqrt(ZZ->dist);
    }
  }

  void sqrt(const double) const { // ignore the argument
    sqrt();
  }

  void sqrtdouble(const double) const { // ignore the argument
    for (node * ZZ=Z; ZZ!=Z+pos; ++ZZ) {
      ZZ->dist = std::sqrt(2*ZZ->dist);
    }
  }

  
  void power(const double p) const {
    double const q = 1/p;
    for (node * ZZ=Z; ZZ!=Z+pos; ++ZZ) {
      ZZ->dist = std::pow(ZZ->dist,q);
    }
  }

  void plusone(const double) const { // ignore the argument
    for (node * ZZ=Z; ZZ!=Z+pos; ++ZZ) {
      ZZ->dist += 1;
    }
  }

  void divide(const double denom) const {
    for (node * ZZ=Z; ZZ!=Z+pos; ++ZZ) {
      ZZ->dist /= denom;
    }
  }
};

class doublyLinkedList {
  /*
    Class for a doubly linked list. Initially, the list is the integer range
    [0, size]. We provide a forward iterator and a method to delete an index
    from the list.

    Typical use: for (i=L.start; L<size; i=L.succ[I])
    or
    for (i=somevalue; L<size; i=L.succ[I])
  */
public:
  tIndex start;
  autoArrayPtr<tIndex> succ;

private:
  autoArrayPtr<tIndex> pred;
  // Not necessarily private, we just do not need it in this instance.

public:
  doublyLinkedList(const tIndex size)
    // Initialize to the given size.
    : start(0)
    , succ(size+1)
    , pred(size+1)
  {
    for (tIndex i=0; i<size; ++i) {
      pred[i+1] = i;
      succ[i] = i+1;
    }
    // pred[0] is never accessed!
    //succ[size] is never accessed!
  }

  ~doublyLinkedList() {}

  void remove(const tIndex idx) {
    // Remove an index from the list.
    if (idx==start) {
      start = succ[idx];
    }
    else {
      succ[pred[idx]] = succ[idx];
      pred[succ[idx]] = pred[idx];
    }
    succ[idx] = 0; // Mark as inactive
  }

  bool isInactive(tIndex idx) const {
    return (succ[idx]==0);
  }
};

// Indexing functions
// D is the upper triangular part of a symmetric (NxN)-matrix
// We require r_ < c_ !
#define D_(r_,c_) ( D[(static_cast<std::ptrdiff_t>(2*N-3-(r_))*(r_)>>1)+(c_)-1] )
// Z is an ((N-1)x4)-array
#define Z_(_r, _c) (Z[(_r)*4 + (_c)])

/*
  Lookup function for a union-find data structure.

  The function finds the root of idx by going iteratively through all
  parent elements until a root is found. An element i is a root if
  nodes[i] is zero. To make subsequent searches faster, the entry for
  idx and all its parents is updated with the root element.
 */
class unionFind {
private:
  autoArrayPtr<tIndex> parent;
  tIndex nextparent;

public:
  unionFind(const tIndex size)
    : parent(size>0 ? 2*size-1 : 0, 0)
    , nextparent(size)
  { }

  tIndex Find (tIndex idx) const {
    if (parent[idx] != 0 ) { // a ? b
      tIndex p = idx;
      idx = parent[idx];
      if (parent[idx] != 0 ) { // a ? b ? c
        do {
          idx = parent[idx];
        } while (parent[idx] != 0);
        do {
          tIndex tmp = parent[p];
          parent[p] = idx;
          p = tmp;
        } while (parent[p] != idx);
      }
    }
    return idx;
  }

  void Union (const tIndex node1, const tIndex node2) {
    parent[node1] = parent[node2] = nextparent++;
  }
};

/* Functions for the update of the dissimilarity array */

inline static void fComplete( double * const b, const double a ) {
  if (*b < a) *b = a;
}


template <methodCodes method, typename tMembers>
static void NNChainCore(const tIndex N, double * const D, tMembers * const members, clusterResult & Z2) {
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

struct posNode {
  tIndex pos;
  int node;
};

void orderNodes(const int N, const int * const merge, const tIndex * const nodeSize, int * const order) {
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
void generateRDendrogram(int * const merge, double * const height, int * const order, clusterResult & Z2, const int N) {
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



