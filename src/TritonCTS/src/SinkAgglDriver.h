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

namespace cts {


#define FILL_N std::fill_n
typedef int_fast32_t tIndex;

// self-destructing array pointer
template <typename type>
class autoArrayPtr
{
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

class clusterResult
{
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

enum hclustFastMethods {
  // complete link with the nearest-neighbor-chain algorithm (Murtagh, 1984)
  HCLUSTMETHODCOMPLETE = 1,
};

class SinkAgglClustering
{
public:
  SinkAgglClustering() {}
  ~SinkAgglClustering() {}
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

  void fComplete( double * const b, const double a );
  enum methodCodes {
    // non-Euclidean methods
    METHODMETRCOMPLETE         = 1,
  };

  template <const bool sorted>
  void generateRDendrogram(int * const merge, double * const height,
                          int * const order, clusterResult & Z2, const int N);

  template <methodCodes method, typename Members>
  void NNChainCore(const tIndex N, double * const D, 
                    Members * const members, clusterResult & Z2);
  void orderNodes(const int N, const int * const merge,
                         const tIndex * const nodeSize,int * const order);

  struct posNode {
  tIndex pos;
  int node;
  };

};
}