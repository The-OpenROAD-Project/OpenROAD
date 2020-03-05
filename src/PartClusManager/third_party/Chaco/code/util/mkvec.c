/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include	<stdio.h>

/* Allocates a double vector with range [nl..nh]. Dies. */
double   *mkvec(nl, nh)
int       nl, nh;
{
    double   *v;
    double   *smalloc();

    v = (double *) smalloc((unsigned) (nh - nl + 1) * sizeof(double));
    return (v - nl);
}

/* Allocates a double vector with range [nl..nh]. Returns error code. */
double   *mkvec_ret(nl, nh)
int       nl, nh;
{
    double   *v;
    double   *smalloc_ret();

    v = (double *) smalloc_ret((unsigned) (nh - nl + 1) * sizeof(double));
    if (v == NULL) 
	return(NULL);
    else 
        return (v - nl);
}

/* Free a double vector with range [nl..nh]. */
void      frvec(v, nl)
double   *v;
int       nl;
{
    int       sfree();

    sfree((char *) (v + nl));
    v = NULL;
}

/* Allocates a float vector with range [nl..nh]. Dies. */
float   *mkvec_float(nl, nh)
int       nl, nh;
{
    float   *v;
    double   *smalloc();

    v = (float *) smalloc((unsigned) (nh - nl + 1) * sizeof(float));
    return (v - nl);
}

/* Allocates a float vector with range [nl..nh]. Returns error code. */
float   *mkvec_ret_float(nl, nh)
int       nl, nh;
{
    float   *v;
    double   *smalloc_ret();

    v = (float *) smalloc_ret((unsigned) (nh - nl + 1) * sizeof(float));
    if (v == NULL) 
	return(NULL);
    else 
        return (v - nl);
}

/* Free a float vector with range [nl..nh]. */
void      frvec_float(v, nl)
float     *v;
int       nl;
{
    int       sfree();

    sfree((char *) (v + nl));
    v = NULL;
}
