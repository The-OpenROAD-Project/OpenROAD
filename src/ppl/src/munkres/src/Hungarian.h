///////////////////////////////////////////////////////////////////////////////
// Hungarian.h: Header file for Class HungarianAlgorithm.
//
// This is a C++ wrapper with slight modification of a hungarian algorithm
// implementation by Markus Buehren. The original implementation is a few
// mex-functions for use in MATLAB, found here:
// http://www.mathworks.com/matlabcentral/fileexchange/6543-functions-for-the-rectangular-assignment-problem
//
// Both this code and the orignal code are published under the BSD license.
// by Cong Ma, 2016
//

#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

using int64 = std::int64_t;

class HungarianAlgorithm
{
 public:
  int solve(std::vector<std::vector<int>>& dist_matrix,
            std::vector<int>& assignment);

 private:
  void assignmentoptimal(int* assignment,
                         int64* cost,
                         int* dist_matrix,
                         int n_of_rows,
                         int n_of_columns);
  void buildassignmentvector(int* assignment,
                             const bool* star_matrix,
                             int n_of_rows,
                             int n_of_columns);
  void computeassignmentcost(const int* assignment,
                             int64* cost,
                             const int* dist_matrix,
                             int n_of_rows);
  void step2a(int* assignment,
              int* dist_matrix,
              bool* star_matrix,
              bool* new_star_matrix,
              bool* prime_matrix,
              bool* covered_columns,
              bool* covered_rows,
              int n_of_rows,
              int n_of_columns,
              int min_dim);
  void step2b(int* assignment,
              int* dist_matrix,
              bool* star_matrix,
              bool* new_star_matrix,
              bool* prime_matrix,
              bool* covered_columns,
              bool* covered_rows,
              int n_of_rows,
              int n_of_columns,
              int min_dim);
  void step3(int* assignment,
             int* dist_matrix,
             bool* star_matrix,
             bool* new_star_matrix,
             bool* prime_matrix,
             bool* covered_columns,
             bool* covered_rows,
             int n_of_rows,
             int n_of_columns,
             int min_dim);
  void step4(int* assignment,
             int* dist_matrix,
             bool* star_matrix,
             bool* new_star_matrix,
             bool* prime_matrix,
             bool* covered_columns,
             bool* covered_rows,
             int n_of_rows,
             int n_of_columns,
             int min_dim,
             int row,
             int col);
  void step5(int* assignment,
             int* dist_matrix,
             bool* star_matrix,
             bool* new_star_matrix,
             bool* prime_matrix,
             bool* covered_columns,
             bool* covered_rows,
             int n_of_rows,
             int n_of_columns,
             int min_dim);
};
