/*
  fastcluster: Fast hierarchical clustering routines for R and Python

  Copyright © 2011 Daniel Müllner
  <http://danifold.net>

  This library implements various fast algorithms for hierarchical,
  agglomerative clustering methods:

  (1) Algorithms for the "stored matrix approach": the input is the array of
      pairwise dissimilarities.

      MST_linkage_core: single linkage clustering with the "minimum spanning
      tree algorithm (Rohlfs)

      NN_chain_core: nearest-neighbor-chain algorithm, suitable for single,
      complete, average, weighted and Ward linkage (Murtagh)

      generic_linkage: generic algorithm, suitable for all distance update
      formulas (Müllner)

  (2) Algorithms for the "stored data approach": the input are points in a
      vector space.

      MST_linkage_core_vector: single linkage clustering for vector data

      generic_linkage_vector: generic algorithm for vector data, suitable for
      the Ward, centroid and median methods.

      generic_linkage_vector_alternative: alternative scheme for updating the
      nearest neighbors. This method seems faster than "generic_linkage_vector"
      for the centroid and median methods but slower for the Ward method.

  All these implementation treat infinity values correctly. They throw an
  exception if a NaN distance value occurs.
*/

#include <cmath> // for std::pow, std::sqrt
#include <math.h> // for std::pow, std::sqrt
#include <cstddef> // for std::ptrdiff_t
#include <limits> // for std::numeric_limits<...>::infinity()
#include <algorithm> // for std::fill_n
#include <stdexcept> // for std::runtime_error
#include <string> // for std::string

#include <cfloat> // also for DBL_MAX, DBL_MIN
#ifndef DBL_MANT_DIG
#error The constant DBL_MANT_DIG could not be defined.
#endif
#define T_FLOAT_MANT_DIG DBL_MANT_DIG

#ifndef LONG_MAX
#include <climits>
#endif
#ifndef LONG_MAX
#error The constant LONG_MAX could not be defined.
#endif
#ifndef INT_MAX
#error The constant INT_MAX could not be defined.
#endif

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

typedef int_fast32_t t_index;
#ifndef INT32_MAX
#define MAX_INDEX 0x7fffffffL
#else
#define MAX_INDEX INT32_MAX
#endif
#if (LONG_MAX < MAX_INDEX)
#error The integer format "t_index" must not have a greater range than "long int".
#endif
#if (INT_MAX > MAX_INDEX)
#error The integer format "int" must not have a greater range than "t_index".
#endif
typedef double t_float;


// Started Changes

// Started Changes
enum method_codes {
  // non-Euclidean methods
  METHOD_METR_COMPLETE         = 1,
};

// self-destructing array pointer
template <typename type>
class auto_array_ptr{
private:
  type * ptr;
  auto_array_ptr(auto_array_ptr const &); // non construction-copyable
  auto_array_ptr& operator=(auto_array_ptr const &); // non copyable
public:
  auto_array_ptr()
    : ptr(NULL)
  { }
  template <typename index>
  auto_array_ptr(index const size)
    : ptr(new type[size])
  { }
  template <typename index, typename value>
  auto_array_ptr(index const size, value const val)
    : ptr(new type[size])
  {
    FILL_N(ptr, size, val);
  }
  ~auto_array_ptr() {
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
  t_index node1, node2;
  t_float dist;
};

inline bool operator< (const node a, const node b) {
  return (a.dist < b.dist);
}

class cluster_result {
private:
  auto_array_ptr<node> Z;
  t_index pos;

public:
  cluster_result(const t_index size)
    : Z(size)
    , pos(0)
  {}

  void append(const t_index node1, const t_index node2, const t_float dist) {
    Z[pos].node1 = node1;
    Z[pos].node2 = node2;
    Z[pos].dist  = dist;
    ++pos;
  }

  node * operator[] (const t_index idx) const { return Z + idx; }

  /* Define several methods to postprocess the distances. All these functions
     are monotone, so they do not change the sorted order of distances. */

  void sqrt() const {
    for (node * ZZ=Z; ZZ!=Z+pos; ++ZZ) {
      ZZ->dist = std::sqrt(ZZ->dist);
    }
  }

  void sqrt(const t_float) const { // ignore the argument
    sqrt();
  }

  void sqrtdouble(const t_float) const { // ignore the argument
    for (node * ZZ=Z; ZZ!=Z+pos; ++ZZ) {
      ZZ->dist = std::sqrt(2*ZZ->dist);
    }
  }

  #ifdef R_pow
  #define my_pow R_pow
  #else
  #define my_pow std::pow
  #endif

  void power(const t_float p) const {
    t_float const q = 1/p;
    for (node * ZZ=Z; ZZ!=Z+pos; ++ZZ) {
      ZZ->dist = my_pow(ZZ->dist,q);
    }
  }

  void plusone(const t_float) const { // ignore the argument
    for (node * ZZ=Z; ZZ!=Z+pos; ++ZZ) {
      ZZ->dist += 1;
    }
  }

  void divide(const t_float denom) const {
    for (node * ZZ=Z; ZZ!=Z+pos; ++ZZ) {
      ZZ->dist /= denom;
    }
  }
};

class doubly_linked_list {
  /*
    Class for a doubly linked list. Initially, the list is the integer range
    [0, size]. We provide a forward iterator and a method to delete an index
    from the list.

    Typical use: for (i=L.start; L<size; i=L.succ[I])
    or
    for (i=somevalue; L<size; i=L.succ[I])
  */
public:
  t_index start;
  auto_array_ptr<t_index> succ;

private:
  auto_array_ptr<t_index> pred;
  // Not necessarily private, we just do not need it in this instance.

public:
  doubly_linked_list(const t_index size)
    // Initialize to the given size.
    : start(0)
    , succ(size+1)
    , pred(size+1)
  {
    for (t_index i=0; i<size; ++i) {
      pred[i+1] = i;
      succ[i] = i+1;
    }
    // pred[0] is never accessed!
    //succ[size] is never accessed!
  }

  ~doubly_linked_list() {}

  void remove(const t_index idx) {
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

  bool is_inactive(t_index idx) const {
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
class union_find {
private:
  auto_array_ptr<t_index> parent;
  t_index nextparent;

public:
  union_find(const t_index size)
    : parent(size>0 ? 2*size-1 : 0, 0)
    , nextparent(size)
  { }

  t_index Find (t_index idx) const {
    if (parent[idx] != 0 ) { // a ? b
      t_index p = idx;
      idx = parent[idx];
      if (parent[idx] != 0 ) { // a ? b ? c
        do {
          idx = parent[idx];
        } while (parent[idx] != 0);
        do {
          t_index tmp = parent[p];
          parent[p] = idx;
          p = tmp;
        } while (parent[p] != idx);
      }
    }
    return idx;
  }

  void Union (const t_index node1, const t_index node2) {
    parent[node1] = parent[node2] = nextparent++;
  }
};

/* Functions for the update of the dissimilarity array */

inline static void f_complete( t_float * const b, const t_float a ) {
  if (*b < a) *b = a;
}


template <method_codes method, typename t_members>
static void NN_chain_core(const t_index N, t_float * const D, t_members * const members, cluster_result & Z2) {
/*
    N: integer
    D: condensed distance matrix N*(N-1)/2
    Z2: output data structure

    This is the NN-chain algorithm, described on page 86 in the following book:

    Fionn Murtagh, Multidimensional Clustering Algorithms,
    Vienna, Würzburg: Physica-Verlag, 1985.
*/
  t_index i;

  auto_array_ptr<t_index> NN_chain(N);
  t_index NN_chain_tip = 0;

  t_index idx1, idx2;

  t_float size1, size2;
  doubly_linked_list active_nodes(N);

  t_float min;

  for (t_float const * DD=D; DD!=D+(static_cast<std::ptrdiff_t>(N)*(N-1)>>1);
       ++DD) {
#if HAVE_DIAGNOSTIC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#if HAVE_DIAGNOSTIC
#pragma GCC diagnostic pop
#endif
  }

  for (t_index j=0; j<N-1; ++j) {
    if (NN_chain_tip <= 3) {
      NN_chain[0] = idx1 = active_nodes.start;
      NN_chain_tip = 1;

      idx2 = active_nodes.succ[idx1];
      min = D_(idx1,idx2);
      for (i=active_nodes.succ[idx2]; i<N; i=active_nodes.succ[i]) {
        if (D_(idx1,i) < min) {
          min = D_(idx1,i);
          idx2 = i;
        }
      }
    }  // a: idx1   b: idx2
    else {
      NN_chain_tip -= 3;
      idx1 = NN_chain[NN_chain_tip-1];
      idx2 = NN_chain[NN_chain_tip];
      min = idx1<idx2 ? D_(idx1,idx2) : D_(idx2,idx1);
    }  // a: idx1   b: idx2

    do {
      NN_chain[NN_chain_tip] = idx2;

      for (i=active_nodes.start; i<idx2; i=active_nodes.succ[i]) {
        if (D_(i,idx2) < min) {
          min = D_(i,idx2);
          idx1 = i;
        }
      }
      for (i=active_nodes.succ[idx2]; i<N; i=active_nodes.succ[i]) {
        if (D_(idx2,i) < min) {
          min = D_(idx2,i);
          idx1 = i;
        }
      }

      idx2 = idx1;
      idx1 = NN_chain[NN_chain_tip++];

    } while (idx2 != NN_chain[NN_chain_tip-2]);

    Z2.append(idx1, idx2, min);

    if (idx1>idx2) {
      t_index tmp = idx1;
      idx1 = idx2;
      idx2 = tmp;
    }

    // Remove the smaller index from the valid indices (active_nodes).
    active_nodes.remove(idx1);

    switch (method) {
    case METHOD_METR_COMPLETE:
      /*
      Complete linkage.

      Characteristic: new distances are never shorter than the old distances.
      */
      // Update the distance matrix in the range [start, idx1).
      for (i=active_nodes.start; i<idx1; i=active_nodes.succ[i])
        f_complete(&D_(i, idx2), D_(i, idx1) );
      // Update the distance matrix in the range (idx1, idx2).
      for (; i<idx2; i=active_nodes.succ[i])
        f_complete(&D_(i, idx2), D_(idx1, i) );
      // Update the distance matrix in the range (idx2, N).
      for (i=active_nodes.succ[idx2]; i<N; i=active_nodes.succ[i])
        f_complete(&D_(idx2, i), D_(idx1, i) );
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

struct pos_node {
  t_index pos;
  int node;
};

void order_nodes(const int N, const int * const merge, const t_index * const node_size, int * const order) {
  /* Parameters:
     N         : number of data points
     merge     : (N-1)×2 array which specifies the node indices which are
                 merged in each step of the clustering procedure.
                 Negative entries -1...-N point to singleton nodes, while
                 positive entries 1...(N-1) point to nodes which are themselves
                 parents of other nodes.
     node_size : array of node sizes - makes it easier
     order     : output array of size N

     Runtime: ?(N)
  */
  auto_array_ptr<pos_node> queue(N/2);

  int parent;
  int child;
  t_index pos = 0;

  queue[0].pos = 0;
  queue[0].node = N-2;
  t_index idx = 1;

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
      pos += node_size[child-1];
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

#define size_(r_) ( ((r_<N) ? 1 : node_size[r_-N]) )

template <const bool sorted>
void generate_R_dendrogram(int * const merge, double * const height, int * const order, cluster_result & Z2, const int N) {
  // The array "nodes" is a union-find data structure for the cluster
  // identites (only needed for unsorted cluster_result input).
  union_find nodes(sorted ? 0 : N);
  if (!sorted) {
    std::stable_sort(Z2[0], Z2[N-1]);
  }

  t_index node1, node2;
  auto_array_ptr<t_index> node_size(N-1);

  for (t_index i=0; i<N-1; ++i) {
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
      t_index tmp = node1;
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
    node_size[i] = size_(node1) + size_(node2);
  }

  order_nodes(N, merge, node_size, order);
}



