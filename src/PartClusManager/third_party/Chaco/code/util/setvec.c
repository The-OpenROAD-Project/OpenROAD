/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

/* Set a double precision vector to constant over range. */
void      setvec(vec, beg, end, setval)
double    vec[];
int       beg, end;
double    setval;
{
    int       i;

    vec = vec + beg;
    for (i = end - beg + 1; i; i--) {
	(*vec++) = setval;
    }
}

/* Set a float precision vector to constant over range. */
void      setvec_float(vec, beg, end, setval)
float     vec[];
int       beg, end;
float     setval;
{
    int       i;

    vec = vec + beg;
    for (i = end - beg + 1; i; i--) {
	(*vec++) = setval;
    }
}
