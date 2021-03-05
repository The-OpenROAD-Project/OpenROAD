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
void cutreeK(int n, const int* merge, int nclust, int* labels) {

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
void cutreeCdist(int n, const int* merge, double* height, double cdist, int* labels) {

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
int hclustFast(int n, double* distmat, int method, int* merge, double* height) {
  
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
