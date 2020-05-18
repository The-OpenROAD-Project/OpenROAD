/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */


void      orthogvec(vec1, beg, end, vec2)
double   *vec1;			/* vector to be orthogonalized */
int       beg, end;		/* start and stop range for vector */
double   *vec2;			/* vector to be orthogonalized against */
{
    double    alpha;
    double    dot();
    void      scadd();

    alpha = -dot(vec1, beg, end, vec2) / dot(vec2, beg, end, vec2);
    scadd(vec1, beg, end, alpha, vec2);
}

void      orthogvec_float(vec1, beg, end, vec2)
float    *vec1;			/* vector to be orthogonalized */
int       beg, end;		/* start and stop range for vector */
float    *vec2;			/* vector to be orthogonalized against */
{
    float     alpha;
    double    dot_float();
    void      scadd_float();

    alpha = -dot_float(vec1, beg, end, vec2) / dot_float(vec2, beg, end, vec2);
    scadd_float(vec1, beg, end, alpha, vec2);
}
