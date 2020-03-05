/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

/* Copy a range of a double vector to a double vector */
void      cpvec(copy, beg, end, vec)
double   *copy;
int       beg, end;
double   *vec;
{
    int       i;

    copy = copy + beg;
    vec = vec + beg;
    for (i = end - beg + 1; i; i--) {
	*copy++ = *vec++;
    }
}

/* Copy a range of a float vector to a double vector */
void      float_to_double(copy, beg, end, vec)
double   *copy;
int       beg, end;
float    *vec;
{
    int       i;

    copy = copy + beg;
    vec = vec + beg;
    for (i = end - beg + 1; i; i--) {
	*copy++ = *vec++;
    }
}

/* Copy a range of a double vector to a float vector */
void      double_to_float(copy, beg, end, vec)
float    *copy;
int       beg, end;
double   *vec;
{
    int       i;

    copy = copy + beg;
    vec = vec + beg;
    for (i = end - beg + 1; i; i--) {
	*copy++ = *vec++;
    }
}

