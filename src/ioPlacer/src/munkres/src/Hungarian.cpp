///////////////////////////////////////////////////////////////////////////////
// Hungarian.cpp: Implementation file for Class HungarianAlgorithm.
//
// This is a C++ wrapper with slight modification of a hungarian algorithm
// implementation by Markus Buehren. The original implementation is a few
// mex-functions for use in MATLAB, found here:
// http://www.mathworks.com/matlabcentral/fileexchange/6543-functions-for-the-rectangular-assignment-problem
//
// Both this code and the orignal code are published under the BSD license.
// by Cong Ma, 2016
//

#include "Hungarian.h"

#include <stdlib.h>

#include <cfloat>  // for DBL_MAX
#include <cmath>   // for fabs()
#include <limits>

//********************************************************//
// A single function wrapper for solving assignment problem.
//********************************************************//
int HungarianAlgorithm::solve(vector<vector<int>>& dist_matrix,
                              vector<int>& assignment)
{
  int n_rows = dist_matrix.size();
  int n_cols = dist_matrix[0].size();

  int* dist_matrix_in = new int[n_rows * n_cols];
  int cost = 0;

  // Fill in the distMatrixIn. Mind the index is "i + nRows * j".
  // Here the cost matrix of size MxN is defined as a int precision array of N*M
  // elements. In the solving functions matrices are seen to be saved
  // MATLAB-internally in row-order. (i.e. the matrix [1 2; 3 4] will be stored
  // as a vector [1 3 2 4], NOT [1 2 3 4]).
  for (int i = 0; i < n_rows; i++)
    for (int j = 0; j < n_cols; j++)
      dist_matrix_in[i + n_rows * j] = dist_matrix[i][j];

  // call solving function
  int* tmp_assignment = new int[n_rows];
  assignmentoptimal(tmp_assignment, &cost, dist_matrix_in, n_rows, n_cols);

  assignment.clear();
  for (int r = 0; r < n_rows; r++)
    assignment.push_back(tmp_assignment[r]);

  delete[] dist_matrix_in;
  delete[] tmp_assignment;
  return cost;
}

//********************************************************//
// Solve optimal solution for assignment problem using Munkres algorithm, also
// known as Hungarian Algorithm.
//********************************************************//
void HungarianAlgorithm::assignmentoptimal(int* assignment,
                                           int* cost,
                                           int* dist_matrix_in,
                                           int n_of_rows,
                                           int n_of_columns)
{
  /* initialization */
  *cost = 0;
  for (int row = 0; row < n_of_rows; row++)
    assignment[row] = -1;

  /* generate working copy of distance Matrix */
  /* check if all matrix elements are positive */
  int n_of_elements = n_of_rows * n_of_columns;
  int* dist_matrix = (int*) malloc(n_of_elements * sizeof(int));
  int* dist_matrix_end = dist_matrix + n_of_elements;

  for (int row = 0; row < n_of_elements; row++) {
    int value = dist_matrix_in[row];
    if (value < 0)
      cerr << "All matrix elements have to be non-negative." << endl;
    dist_matrix[row] = value;
  }

  /* memory allocation */
  bool* covered_columns = (bool*) calloc(n_of_columns, sizeof(bool));
  bool* covered_rows = (bool*) calloc(n_of_rows, sizeof(bool));
  bool* star_matrix = (bool*) calloc(n_of_elements, sizeof(bool));
  bool* prime_matrix = (bool*) calloc(n_of_elements, sizeof(bool));
  bool* new_star_matrix
      = (bool*) calloc(n_of_elements, sizeof(bool)); /* used in step4 */

  /* preliminary steps */
  int min_dim;
  if (n_of_rows <= n_of_columns) {
    min_dim = n_of_rows;

    for (int row = 0; row < n_of_rows; row++) {
      /* find the smallest element in the row */
      int* dist_matrix_temp = dist_matrix + row;
      int min_value = *dist_matrix_temp;
      dist_matrix_temp += n_of_rows;
      while (dist_matrix_temp < dist_matrix_end) {
        int value = *dist_matrix_temp;
        if (value < min_value)
          min_value = value;
        dist_matrix_temp += n_of_rows;
      }

      /* subtract the smallest element from each element of the row */
      dist_matrix_temp = dist_matrix + row;
      while (dist_matrix_temp < dist_matrix_end) {
        *dist_matrix_temp -= min_value;
        dist_matrix_temp += n_of_rows;
      }
    }

    /* Steps 1 and 2a */
    for (int row = 0; row < n_of_rows; row++)
      for (int col = 0; col < n_of_columns; col++)
        if (fabs(dist_matrix[row + n_of_rows * col]) < DBL_EPSILON)
          if (!covered_columns[col]) {
            star_matrix[row + n_of_rows * col] = true;
            covered_columns[col] = true;
            break;
          }
  } else /* if(nOfRows > nOfColumns) */
  {
    min_dim = n_of_columns;

    for (int col = 0; col < n_of_columns; col++) {
      /* find the smallest element in the column */
      int* dist_matrix_temp = dist_matrix + n_of_rows * col;
      int* column_end = dist_matrix_temp + n_of_rows;

      int min_value = *dist_matrix_temp++;
      while (dist_matrix_temp < column_end) {
        int value = *dist_matrix_temp++;
        if (value < min_value)
          min_value = value;
      }

      /* subtract the smallest element from each element of the column */
      dist_matrix_temp = dist_matrix + n_of_rows * col;
      while (dist_matrix_temp < column_end)
        *dist_matrix_temp++ -= min_value;
    }

    /* Steps 1 and 2a */
    for (int col = 0; col < n_of_columns; col++)
      for (int row = 0; row < n_of_rows; row++)
        if (fabs(dist_matrix[row + n_of_rows * col]) < DBL_EPSILON)
          if (!covered_rows[row]) {
            star_matrix[row + n_of_rows * col] = true;
            covered_columns[col] = true;
            covered_rows[row] = true;
            break;
          }
    for (int row = 0; row < n_of_rows; row++)
      covered_rows[row] = false;
  }

  /* move to step 2b */
  step2b(assignment,
         dist_matrix,
         star_matrix,
         new_star_matrix,
         prime_matrix,
         covered_columns,
         covered_rows,
         n_of_rows,
         n_of_columns,
         min_dim);

  /* compute cost and remove invalid assignments */
  computeassignmentcost(assignment, cost, dist_matrix_in, n_of_rows);

  /* free allocated memory */
  free(dist_matrix);
  free(covered_columns);
  free(covered_rows);
  free(star_matrix);
  free(prime_matrix);
  free(new_star_matrix);

  return;
}

/********************************************************/
void HungarianAlgorithm::buildassignmentvector(int* assignment,
                                               bool* star_matrix,
                                               int n_of_rows,
                                               int n_of_columns)
{
  for (int row = 0; row < n_of_rows; row++)
    for (int col = 0; col < n_of_columns; col++)
      if (star_matrix[row + n_of_rows * col]) {
#ifdef ONE_INDEXING
        assignment[row] = col + 1; /* MATLAB-Indexing */
#else
        assignment[row] = col;
#endif
        break;
      }
}

/********************************************************/
void HungarianAlgorithm::computeassignmentcost(int* assignment,
                                               int* cost,
                                               int* dist_matrix,
                                               int n_of_rows)
{
  for (int row = 0; row < n_of_rows; row++) {
    int col = assignment[row];
    if (col >= 0)
      *cost += dist_matrix[row + n_of_rows * col];
  }
}

/********************************************************/
void HungarianAlgorithm::step2a(int* assignment,
                                int* dist_matrix,
                                bool* star_matrix,
                                bool* new_star_matrix,
                                bool* prime_matrix,
                                bool* covered_columns,
                                bool* covered_rows,
                                int n_of_rows,
                                int n_of_columns,
                                int min_dim)
{
  /* cover every column containing a starred zero */
  for (int col = 0; col < n_of_columns; col++) {
    bool* star_matrix_temp = star_matrix + n_of_rows * col;
    bool* column_end = star_matrix_temp + n_of_rows;
    while (star_matrix_temp < column_end) {
      if (*star_matrix_temp++) {
        covered_columns[col] = true;
        break;
      }
    }
  }

  /* move to step 3 */
  step2b(assignment,
         dist_matrix,
         star_matrix,
         new_star_matrix,
         prime_matrix,
         covered_columns,
         covered_rows,
         n_of_rows,
         n_of_columns,
         min_dim);
}

/********************************************************/
void HungarianAlgorithm::step2b(int* assignment,
                                int* dist_matrix,
                                bool* star_matrix,
                                bool* new_star_matrix,
                                bool* prime_matrix,
                                bool* covered_columns,
                                bool* covered_rows,
                                int n_of_rows,
                                int n_of_columns,
                                int min_dim)
{
  /* count covered columns */
  int n_of_covered_columns = 0;
  for (int col = 0; col < n_of_columns; col++)
    if (covered_columns[col])
      n_of_covered_columns++;

  if (n_of_covered_columns == min_dim) {
    /* algorithm finished */
    buildassignmentvector(assignment, star_matrix, n_of_rows, n_of_columns);
  } else {
    /* move to step 3 */
    step3(assignment,
          dist_matrix,
          star_matrix,
          new_star_matrix,
          prime_matrix,
          covered_columns,
          covered_rows,
          n_of_rows,
          n_of_columns,
          min_dim);
  }
}

/********************************************************/
void HungarianAlgorithm::step3(int* assignment,
                               int* dist_matrix,
                               bool* star_matrix,
                               bool* new_star_matrix,
                               bool* prime_matrix,
                               bool* covered_columns,
                               bool* covered_rows,
                               int n_of_rows,
                               int n_of_columns,
                               int min_dim)
{
  bool zeros_found = true;
  while (zeros_found) {
    zeros_found = false;
    for (int col = 0; col < n_of_columns; col++)
      if (!covered_columns[col])
        for (int row = 0; row < n_of_rows; row++)
          if ((!covered_rows[row])
              && (fabs(dist_matrix[row + n_of_rows * col]) < DBL_EPSILON)) {
            /* prime zero */
            prime_matrix[row + n_of_rows * col] = true;

            /* find starred zero in current row */
            int star_col;
            for (star_col = 0; star_col < n_of_columns; star_col++)
              if (star_matrix[row + n_of_rows * star_col])
                break;

            if (star_col == n_of_columns) /* no starred zero found */
            {
              /* move to step 4 */
              step4(assignment,
                    dist_matrix,
                    star_matrix,
                    new_star_matrix,
                    prime_matrix,
                    covered_columns,
                    covered_rows,
                    n_of_rows,
                    n_of_columns,
                    min_dim,
                    row,
                    col);
              return;
            } else {
              covered_rows[row] = true;
              covered_columns[star_col] = false;
              zeros_found = true;
              break;
            }
          }
  }

  /* move to step 5 */
  step5(assignment,
        dist_matrix,
        star_matrix,
        new_star_matrix,
        prime_matrix,
        covered_columns,
        covered_rows,
        n_of_rows,
        n_of_columns,
        min_dim);
}

/********************************************************/
void HungarianAlgorithm::step4(int* assignment,
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
                               int col)
{
  int n_of_elements = n_of_rows * n_of_columns;

  /* generate temporary copy of starMatrix */
  for (int n = 0; n < n_of_elements; n++)
    new_star_matrix[n] = star_matrix[n];

  /* star current zero */
  new_star_matrix[row + n_of_rows * col] = true;

  /* find starred zero in current column */
  int star_col = col;
  int star_row;
  for (star_row = 0; star_row < n_of_rows; star_row++)
    if (star_matrix[star_row + n_of_rows * star_col])
      break;

  while (star_row < n_of_rows) {
    /* unstar the starred zero */
    new_star_matrix[star_row + n_of_rows * star_col] = false;

    /* find primed zero in current row */
    int prime_row = star_row;
    int prime_col;
    for (prime_col = 0; prime_col < n_of_columns; prime_col++)
      if (prime_matrix[prime_row + n_of_rows * prime_col])
        break;

    /* star the primed zero */
    new_star_matrix[prime_row + n_of_rows * prime_col] = true;

    /* find starred zero in current column */
    star_col = prime_col;
    for (star_row = 0; star_row < n_of_rows; star_row++)
      if (star_matrix[star_row + n_of_rows * star_col])
        break;
  }

  /* use temporary copy as new starMatrix */
  /* delete all primes, uncover all rows */
  for (int n = 0; n < n_of_elements; n++) {
    prime_matrix[n] = false;
    star_matrix[n] = new_star_matrix[n];
  }
  for (int n = 0; n < n_of_rows; n++)
    covered_rows[n] = false;

  /* move to step 2a */
  step2a(assignment,
         dist_matrix,
         star_matrix,
         new_star_matrix,
         prime_matrix,
         covered_columns,
         covered_rows,
         n_of_rows,
         n_of_columns,
         min_dim);
}

/********************************************************/
void HungarianAlgorithm::step5(int* assignment,
                               int* dist_matrix,
                               bool* star_matrix,
                               bool* new_star_matrix,
                               bool* prime_matrix,
                               bool* covered_columns,
                               bool* covered_rows,
                               int n_of_rows,
                               int n_of_columns,
                               int min_dim)
{
  /* find smallest uncovered element h */
  int h = std::numeric_limits<int>::max();
  for (int row = 0; row < n_of_rows; row++)
    if (!covered_rows[row])
      for (int col = 0; col < n_of_columns; col++)
        if (!covered_columns[col]) {
          int value = dist_matrix[row + n_of_rows * col];
          if (value < h)
            h = value;
        }

  /* add h to each covered row */
  for (int row = 0; row < n_of_rows; row++)
    if (covered_rows[row])
      for (int col = 0; col < n_of_columns; col++)
        dist_matrix[row + n_of_rows * col] += h;

  /* subtract h from each uncovered column */
  for (int col = 0; col < n_of_columns; col++)
    if (!covered_columns[col])
      for (int row = 0; row < n_of_rows; row++)
        dist_matrix[row + n_of_rows * col] -= h;

  /* move to step 3 */
  step3(assignment,
        dist_matrix,
        star_matrix,
        new_star_matrix,
        prime_matrix,
        covered_columns,
        covered_rows,
        n_of_rows,
        n_of_columns,
        min_dim);
}
