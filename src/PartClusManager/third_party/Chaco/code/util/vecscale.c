/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

/* Scale - fills vec1 with alpha*vec2 over range, double version */
void      vecscale(vec1, beg, end, alpha, vec2)
double   *vec1;
int       beg, end;
double    alpha;
double   *vec2;
{
    int       i;

    vec1 += beg;
    vec2 += beg;
    for (i = end - beg + 1; i; i--) {
	(*vec1++) = alpha * (*vec2++);
    }
}

/* Scale - fills vec1 with alpha*vec2 over range, float version */
void      vecscale_float(vec1, beg, end, alpha, vec2)
float    *vec1;
int       beg, end;
float     alpha;
float    *vec2;
{
    int       i;

    vec1 += beg;
    vec2 += beg;
    for (i = end - beg + 1; i; i--) {
	(*vec1++) = alpha * (*vec2++);
    }
}
