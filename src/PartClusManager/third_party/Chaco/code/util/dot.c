/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

/* Returns scalar product of two double n-vectors. */
double    dot(vec1, beg, end, vec2)
double   *vec1;
int       beg, end;
double   *vec2;
{
    int       i;
    double    sum;

    sum = 0.0;
    vec1 = vec1 + beg;
    vec2 = vec2 + beg;
    for (i = end - beg + 1; i; i--) {
	sum += (*vec1++) * (*vec2++);
    }
    return (sum);
}

/* Returns scalar product of two float n-vectors. */
double    dot_float(vec1, beg, end, vec2)
float    *vec1;
int       beg, end;
float    *vec2;
{
    int       i;
    float     sum;

    sum = 0.0;
    vec1 = vec1 + beg;
    vec2 = vec2 + beg;
    for (i = end - beg + 1; i; i--) {
	sum += (*vec1++) * (*vec2++);
    }
    return ((double) sum);
}
