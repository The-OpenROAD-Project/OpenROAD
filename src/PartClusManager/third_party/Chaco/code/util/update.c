/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

/* update - fills double vec1 with vec2 + alpha*vec3 over range*/
void      update(vec1, beg, end, vec2, fac, vec3)
double   *vec1;
int       beg, end;
double    fac;
double   *vec2;
double   *vec3;
{
    int       i;

    vec1 += beg;
    vec2 += beg;
    vec3 += beg;
    for (i = end - beg + 1; i; i--) {
	(*vec1++) = (*vec2++) + fac * (*vec3++);
    }
}

/* update - fills float vec1 with vec2 + alpha*vec3 over range*/
void      update_float(vec1, beg, end, vec2, fac, vec3)
float    *vec1;
int       beg, end;
float     fac;
float    *vec2;
float    *vec3;
{
    int       i;

    vec1 += beg;
    vec2 += beg;
    vec3 += beg;
    for (i = end - beg + 1; i; i--) {
	(*vec1++) = (*vec2++) + fac * (*vec3++);
    }
}
