/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

/* Normalizes a double n-vector over range. */
double    normalize(vec, beg, end)
double   *vec;
int       beg, end;
{
    int       i;
    double    scale;
    double    norm();

    scale = norm(vec, beg, end);
    vec = vec + beg;
    for (i = end - beg + 1; i; i--) {
	*vec = *vec / scale;
	vec++;
    }
    return (scale);
}

/* Normalizes such that element k is positive */
double    sign_normalize(vec, beg, end, k)
double   *vec;
int       beg, end, k;
{
    int       i;
    double    scale, scale2;
    double    norm();

    scale = norm(vec, beg, end);
    if (vec[k] < 0) {
	scale2 = -scale;
    }
    else {
	scale2 = scale;
    }
    vec = vec + beg;
    for (i = end - beg + 1; i; i--) {
	*vec = *vec / scale2;
	vec++;
    }
    return (scale);
}

/* Normalizes a float n-vector over range. */
double    normalize_float(vec, beg, end)
float    *vec;
int       beg, end;
{
    int       i;
    float     scale;
    double    norm_float();

    scale = norm_float(vec, beg, end);
    vec = vec + beg;
    for (i = end - beg + 1; i; i--) {
	*vec = *vec / scale;
	vec++;
    }
    return ((double) scale);
}
