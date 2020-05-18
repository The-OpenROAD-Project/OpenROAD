/* symmlqblas.f -- translated by f2c (version of 16 May 1991  13:06:06).
   You must link the resulting object file with the libraries:
	-link <S|C|M|L>f2c.lib   (in that order)
*/

#include "f2c.h"

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/*     symmlqblas  fortran */

/*     daxpy    dcopy    ddot     dnrm2 */

/* ** from netlib, Thu May 16 21:00:13 EDT 1991 *** */
/* ** Declarations of the form dx(1) changed to dx(*) */

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
 /* Subroutine */ int daxpy_(n, da, dx, incx, dy, incy)
integer  *n;
doublereal *da, *dx;
integer  *incx;
doublereal *dy;
integer  *incy;
{
    /* System generated locals */
    integer   i__1;

    /* Local variables */
    static integer i, m, ix, iy, mp1;


/*     constant times a vector plus a vector. */
/*     uses unrolled loops for increments equal to one. */
/*     jack dongarra, linpack, 3/11/78. */


    /* Parameter adjustments */
    --dy;
    --dx;

    /* Function Body */
    if (*n <= 0) {
	return 0;
    }
    if (*da == 0.) {
	return 0;
    }
    if (*incx == 1 && *incy == 1) {
	goto L20;
    }

/*        code for unequal increments or equal increments */
/*          not equal to 1 */

    ix = 1;
    iy = 1;
    if (*incx < 0) {
	ix = (-(*n) + 1) * *incx + 1;
    }
    if (*incy < 0) {
	iy = (-(*n) + 1) * *incy + 1;
    }
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
	dy[iy] += *da * dx[ix];
	ix += *incx;
	iy += *incy;
/* L10: */
    }
    return 0;

/*        code for both increments equal to 1 */


/*        clean-up loop */

L20:
    m = *n % 4;
    if (m == 0) {
	goto L40;
    }
    i__1 = m;
    for (i = 1; i <= i__1; ++i) {
	dy[i] += *da * dx[i];
/* L30: */
    }
    if (*n < 4) {
	return 0;
    }
L40:
    mp1 = m + 1;
    i__1 = *n;
    for (i = mp1; i <= i__1; i += 4) {
	dy[i] += *da * dx[i];
	dy[i + 1] += *da * dx[i + 1];
	dy[i + 2] += *da * dx[i + 2];
	dy[i + 3] += *da * dx[i + 3];
/* L50: */
    }
    return 0;
}				/* daxpy_ */

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
 /* Subroutine */ int dcopy_(n, dx, incx, dy, incy)
integer  *n;
doublereal *dx;
integer  *incx;
doublereal *dy;
integer  *incy;
{
    /* System generated locals */
    integer   i__1;

    /* Local variables */
    static integer i, m, ix, iy, mp1;


/*     copies a vector, x, to a vector, y. */
/*     uses unrolled loops for increments equal to one. */
/*     jack dongarra, linpack, 3/11/78. */


    /* Parameter adjustments */
    --dy;
    --dx;

    /* Function Body */
    if (*n <= 0) {
	return 0;
    }
    if (*incx == 1 && *incy == 1) {
	goto L20;
    }

/*        code for unequal increments or equal increments */
/*          not equal to 1 */

    ix = 1;
    iy = 1;
    if (*incx < 0) {
	ix = (-(*n) + 1) * *incx + 1;
    }
    if (*incy < 0) {
	iy = (-(*n) + 1) * *incy + 1;
    }
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
	dy[iy] = dx[ix];
	ix += *incx;
	iy += *incy;
/* L10: */
    }
    return 0;

/*        code for both increments equal to 1 */


/*        clean-up loop */

L20:
    m = *n % 7;
    if (m == 0) {
	goto L40;
    }
    i__1 = m;
    for (i = 1; i <= i__1; ++i) {
	dy[i] = dx[i];
/* L30: */
    }
    if (*n < 7) {
	return 0;
    }
L40:
    mp1 = m + 1;
    i__1 = *n;
    for (i = mp1; i <= i__1; i += 7) {
	dy[i] = dx[i];
	dy[i + 1] = dx[i + 1];
	dy[i + 2] = dx[i + 2];
	dy[i + 3] = dx[i + 3];
	dy[i + 4] = dx[i + 4];
	dy[i + 5] = dx[i + 5];
	dy[i + 6] = dx[i + 6];
/* L50: */
    }
    return 0;
}				/* dcopy_ */

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
doublereal ddot_(n, dx, incx, dy, incy)
integer  *n;
doublereal *dx;
integer  *incx;
doublereal *dy;
integer  *incy;
{
    /* System generated locals */
    integer   i__1;
    doublereal ret_val;

    /* Local variables */
    static integer i, m;
    static doublereal dtemp;
    static integer ix, iy, mp1;


/*     forms the dot product of two vectors. */
/*     uses unrolled loops for increments equal to one. */
/*     jack dongarra, linpack, 3/11/78. */


    /* Parameter adjustments */
    --dy;
    --dx;

    /* Function Body */
    ret_val = 0.;
    dtemp = 0.;
    if (*n <= 0) {
	return ret_val;
    }
    if (*incx == 1 && *incy == 1) {
	goto L20;
    }

/*        code for unequal increments or equal increments */
/*          not equal to 1 */

    ix = 1;
    iy = 1;
    if (*incx < 0) {
	ix = (-(*n) + 1) * *incx + 1;
    }
    if (*incy < 0) {
	iy = (-(*n) + 1) * *incy + 1;
    }
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
	dtemp += dx[ix] * dy[iy];
	ix += *incx;
	iy += *incy;
/* L10: */
    }
    ret_val = dtemp;
    return ret_val;

/*        code for both increments equal to 1 */


/*        clean-up loop */

L20:
    m = *n % 5;
    if (m == 0) {
	goto L40;
    }
    i__1 = m;
    for (i = 1; i <= i__1; ++i) {
	dtemp += dx[i] * dy[i];
/* L30: */
    }
    if (*n < 5) {
	goto L60;
    }
L40:
    mp1 = m + 1;
    i__1 = *n;
    for (i = mp1; i <= i__1; i += 5) {
	dtemp = dtemp + dx[i] * dy[i] + dx[i + 1] * dy[i + 1] + dx[i + 2] *
	   dy[i + 2] + dx[i + 3] * dy[i + 3] + dx[i + 4] * dy[i + 4];
/* L50: */
    }
L60:
    ret_val = dtemp;
    return ret_val;
}				/* ddot_ */

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
doublereal dnrm2_(n, dx, incx)
integer  *n;
doublereal *dx;
integer  *incx;
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal one = 1.;
    static doublereal cutlo = 8.232e-11;
    static doublereal cuthi = 1.304e19;

    /* Format strings */
    /* static char fmt_30[] = ""; static char fmt_50[] = ""; static char fmt_70[] =
       ""; static char fmt_110[] = ""; */

    /* System generated locals */
    integer   i__1, i__2;
    doublereal ret_val, d__1;

    /* Builtin functions */
    double    sqrt();

    /* Local variables */
    static doublereal xmax;
    static integer next, i, j, nn;
    static doublereal hitest, sum;

    /* Parameter adjustments */
    --dx;

    /* Function Body */

/*     euclidean norm of the n-vector stored in dx() with storage */
/*     increment incx . */
/*     if    n .le. 0 return with result = 0. */
/*     if n .ge. 1 then incx must be .ge. 1 */

/*           c.l.lawson, 1978 jan 08 */

/*     four phase method     using two built-in constants that are */
/*     hopefully applicable to all machines. */
/*         cutlo = maximum of  dsqrt(u/eps)  over all known machines. */
/*         cuthi = minimum of  dsqrt(v)      over all known machines. */
/*     where */
/*         eps = smallest no. such that eps + 1. .gt. 1. */
/*         u   = smallest positive no.   (underflow limit) */
/*         v   = largest  no.            (overflow  limit) */

/*     brief outline of algorithm.. */

/*     phase 1    scans zero components. */
/*     move to phase 2 when a component is nonzero and .le. cutlo */
/*     move to phase 3 when a component is .gt. cutlo */
/*     move to phase 4 when a component is .ge. cuthi/m */
/*     where m = n for x() real and m = 2*n for complex. */

/*     values for cutlo and cuthi.. */
/*     from the environmental parameters listed in the imsl converter */
/*     document the limiting values are as follows.. */
/*     cutlo, s.p.   u/eps = 2**(-102) for  honeywell.  close seconds are
*/
/*                   univac and dec at 2**(-103) */
/*                   thus cutlo = 2**(-51) = 4.44089e-16 */
/*     cuthi, s.p.   v = 2**127 for univac, honeywell, and dec. */
/*                   thus cuthi = 2**(63.5) = 1.30438e19 */
/*     cutlo, d.p.   u/eps = 2**(-67) for honeywell and dec. */
/*                   thus cutlo = 2**(-33.5) = 8.23181d-11 */
/*     cuthi, d.p.   same as s.p.  cuthi = 1.30438d19 */
/*     data cutlo, cuthi / 8.232d-11,  1.304d19 / */
/*     data cutlo, cuthi / 4.441e-16,  1.304e19 / */

    if (*n > 0) {
	goto L10;
    }
    ret_val = zero;
    goto L300;

L10:
    next = 0;
    sum = zero;
    nn = *n * *incx;
/*                                                 begin main loop */
    i = 1;
L20:
    switch ((int) next) {
    case 0:
	goto L30;
    case 1:
	goto L50;
    case 2:
	goto L70;
    case 3:
	goto L110;
    }
L30:
    if ((d__1 = dx[i], abs(d__1)) > cutlo) {
	goto L85;
    }
    next = 1;
    xmax = zero;

/*                        phase 1.  sum is zero */

L50:
    if (dx[i] == zero) {
	goto L200;
    }
    if ((d__1 = dx[i], abs(d__1)) > cutlo) {
	goto L85;
    }

/*                                prepare for phase 2. */
    next = 2;
    goto L105;

/*                                prepare for phase 4. */

L100:
    i = j;
    next = 3;
    sum = sum / dx[i] / dx[i];
L105:
    xmax = (d__1 = dx[i], abs(d__1));
    goto L115;

/*                   phase 2.  sum is small. */
/*                             scale to avoid destructive underflow. */

L70:
    if ((d__1 = dx[i], abs(d__1)) > cutlo) {
	goto L75;
    }

/*                     common code for phases 2 and 4. */
/*                     in phase 4 sum is large.  scale to avoid overflow.
*/

L110:
    if ((d__1 = dx[i], abs(d__1)) <= xmax) {
	goto L115;
    }
/* Computing 2nd power */
    d__1 = xmax / dx[i];
    sum = one + sum * (d__1 * d__1);
    xmax = (d__1 = dx[i], abs(d__1));
    goto L200;

L115:
/* Computing 2nd power */
    d__1 = dx[i] / xmax;
    sum += d__1 * d__1;
    goto L200;


/*                  prepare for phase 3. */

L75:
    sum = sum * xmax * xmax;


/*     for real or d.p. set hitest = cuthi/n */
/*     for complex      set hitest = cuthi/(2*n) */

L85:
    hitest = cuthi / (real) (*n);

/*                   phase 3.  sum is mid-range.  no scaling. */

    i__1 = nn;
    i__2 = *incx;
    for (j = i; i__2 < 0 ? j >= i__1 : j <= i__1; j += i__2) {
	if ((d__1 = dx[j], abs(d__1)) >= hitest) {
	    goto L100;
	}
/* L95: */
/* Computing 2nd power */
	d__1 = dx[j];
	sum += d__1 * d__1;
    }
    ret_val = sqrt(sum);
    goto L300;

L200:
    i += *incx;
    if (i <= nn) {
	goto L20;
    }

/*              end of main loop. */

/*              compute square root and adjust for scaling. */

    ret_val = xmax * sqrt(sum);
L300:
    return ret_val;
}				/* dnrm2_ */
