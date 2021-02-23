//
// C++ standalone verion of fastcluster by Daniel Müllner
//
// Copyright: Christoph Dalitz, 2020
//            Daniel Müllner, 2011
// License:   BSD style license
//            (see the file LICENSE for details)
//


#include <vector>
#include <algorithm>
#include <iostream>

#include "fastcluster.h"

// Code by Daniel Müllner
// workaround to make it usable as a standalone version (without R)
#include "fastcluster_dm.cpp"

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
void cutree_k(int n, const int* merge, int nclust, int* labels) {

  int k,m1,m2,j,l;

  if (nclust > n || nclust < 2) {
    for (j=0; j<n; j++) labels[j] = 0;
    return;
  }

  std::cout << "TODO: 1 step" << std::endl;

  // assign to each observable the number of its last merge step
  // beware: indices of observables in merge start at 1 (R convention)
  std::vector<int> last_merge(n, 0);
  for (k=1; k<=(n-nclust); k++) {
    // (m1,m2) = merge[k,]
    std::cout << "TODO: " << k << std::endl; 
    m1 = merge[k-1];
    m2 = merge[n-1+k-1];
    std::cout << "TODO: MERGE : " << m1 << " " << m2 << std::endl;
    if (m1 < 0 && m2 < 0) { // both single observables
      std::cout << "TODO: " << "01" << std::endl; 

      last_merge[-m1-1] = last_merge[-m2-1] = k;
	}
	else if (m1 < 0 || m2 < 0) { // one is a cluster
      std::cout << "TODO: " << "02" << std::endl; 
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
      std::cout << "TODO: " << "02" << std::endl; 

	    // merging single observable and cluster
	    for(l = 0; l < n; l++){
		    if (last_merge[l] == m1){
          last_merge[l] = k;
        }
      }
      std::cout << "TODO: " << "02" << std::endl; 

	    last_merge[0] = k;
      std::cout << "TODO: " << "02 " << temp-1 << std::endl; 

	}
	else { // both cluster
    std::cout << "TODO: " << "03" << std::endl; 

	    for(l=0; l < n; l++) {
		if( last_merge[l] == m1 || last_merge[l] == m2 )
		    last_merge[l] = k;
	    }
    }
  }

  std::cout << "TODO: 2 step" << std::endl;


  // assign cluster labels
  int label = 0;
  std::vector<int> z(n,-1);
  for (j=0; j<n; j++) {
    if (last_merge[j] == 0) { // still singleton
      labels[j] = label++;
    } else {
      if (z[last_merge[j]] < 0) {
        z[last_merge[j]] = label++;
      }
      labels[j] = z[last_merge[j]];
    }
  }

  std::cout << "TODO: 3 step" << std::endl;

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
void cutree_cdist(int n, const int* merge, double* height, double cdist, int* labels) {

  int k;

  for (k=0; k<(n-1); k++) {
    if (height[k] >= cdist) {
      break;
    }
  }

  std::cout << "TODO: 0 step :" << n-k << " " << n << std::endl;

  cutree_k(n, merge, n-k, labels);
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
int hclust_fast(int n, double* distmat, int method, int* merge, double* height) {
  
  // call appropriate culstering function
  cluster_result Z2(n-1);
  if (method == HCLUST_METHOD_SINGLE) {
    // single link
    MST_linkage_core(n, distmat, Z2);
  }
  else if (method == HCLUST_METHOD_COMPLETE) {
    // complete link
    NN_chain_core<METHOD_METR_COMPLETE, t_float>(n, distmat, NULL, Z2);
  }
  else if (method == HCLUST_METHOD_AVERAGE) {
    // best average distance
    double* members = new double[n];
    for (int i=0; i<n; i++) members[i] = 1;
    NN_chain_core<METHOD_METR_AVERAGE, t_float>(n, distmat, members, Z2);
    delete[] members;
  }
  else if (method == HCLUST_METHOD_MEDIAN) {
    // best median distance (beware: O(n^3))
    generic_linkage<METHOD_METR_MEDIAN, t_float>(n, distmat, NULL, Z2);
  }
  else {
    return 1;
  }
  
  
  return 0;
}
