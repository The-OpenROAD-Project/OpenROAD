/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */


/* Sort a double array using Shell's method. Modified algorithm
   from p. 245, Numerical Recipies (changed float to double). */

void      shell_sort(n, arr)
double    arr[];
int       n;
{
    int       nn, m, j, i;
    double    t;

    m = n;
    for (nn = 1; nn <= n; nn <<= 1) {
	m >>= 1;
	for (j = m + 1; j <= n; j++) {
	    i = j - m;
	    t = arr[j];
	    while (i >= 1 && arr[i] > t) {
		arr[i + m] = arr[i];
		i -= m;
	    }
	    arr[i + m] = t;
	}
    }
}
