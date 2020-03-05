/* symmlq.f -- translated by f2c (version of 16 May 1991  13:06:06).
   You must link the resulting object file with the libraries:
	-link <S|C|M|L>f2c.lib   (in that order)
*/

#include "f2c.h"

/* Table of constant values */

static integer c__1 = 1;
static doublereal c_b4 = 1.;
static doublereal c_b18 = .33333;

/* Subroutine */ int symmlq_(n, b, r1, r2, v, w, x, y, work, checka, goodb, 
	precon, shift, nout, itnlim, rtol, istop, itn, anorm, acond, rnorm, 
	ynorm, a, vwsqrt, orthlist, macheps, normxlim, itnmin)
integer *n;
doublereal *b, *r1, *r2, *v, *w, *x, *y, *work;
logical *checka, *goodb, *precon;
doublereal *shift;
integer *nout, *itnlim;
doublereal *rtol;
integer *istop, *itn;
doublereal *anorm, *acond, *rnorm, *ynorm, *a, *vwsqrt, *orthlist, *macheps, *
	normxlim;
integer *itnmin;
{

    /* System generated locals */
    integer i__1;
    doublereal d__1, d__2;

    /* Builtin functions */
    double pow_dd(), sqrt();

    /* Local variables */
    static doublereal alfa, diag, dbar, beta, gbar, oldb, epsa;
    extern doublereal ddot_();
    static doublereal gmin, gmax, zbar, epsr, epsx, beta1;
    extern doublereal dnrm2_();
    static integer i;
    static doublereal gamma, s, t, delta, z, denom;
    extern /* Subroutine */ int aprod_();
    static doublereal bstep;
    extern /* Subroutine */ int dcopy_();
    static doublereal epsln;
    extern /* Subroutine */ int daxpy_();
    static doublereal tnorm, cs, ynorm2, sn, cgnorm;
    extern /* Subroutine */ int msolve_();
    static doublereal snprod, lqnorm, qrnorm, eps, rhs1, rhs2;

/*     ------------------------------------------------------------------ 
*/

/*     SYMMLQ  is designed to solve the system of linear equations */

/*                Ax = b */

/*     where A is an n by n symmetric matrix and b is a given vector. */
/*     The matrix A is not required to be positive definite. */
/*     (If A is known to be definite, the method of conjugate gradients */

/*     might be preferred, since it will require about the same number of 
*/
/*     iterations as SYMMLQ but slightly less work per iteration.) */


/*     The matrix A is intended to be large and sparse.  It is accessed */

/*     by means of a subroutine call of the form */

/*     old        call aprod ( n, x, y ) */
/*     new:       call aprod ( n, x, y, A, vwsqrt, work, orthlist ) -rwl 
*/

/*     which must return the product y = Ax for any given vector x. */


/*     More generally, SYMMLQ is designed to solve the system */

/*                (A - shift*I) x = b */

/*     where  shift  is a specified scalar value.  If  shift  and  b */
/*     are suitably chosen, the computed vector x may approximate an */
/*     (unnormalized) eigenvector of A, as in the methods of */
/*     inverse iteration and/or Rayleigh-quotient iteration. */
/*     Again, the matrix (A - shift*I) need not be positive definite. */
/*     The work per iteration is very slightly less if  shift = 0. */


/*     A further option is that of preconditioning, which may reduce */
/*     the number of iterations required.  If M = C C' is a positive */
/*     definite matrix that is known to approximate  (A - shift*I) */
/*     in some sense, and if systems of the form  My = x  can be */
/*     solved efficiently, the parameters precon and msolve may be */
/*     used (see below).  When  precon = .true., SYMMLQ will */
/*     implicitly solve the system of equations */

/*             P (A - shift*I) P' xbar  =  P b, */

/*     i.e.                  Abar xbar  =  bbar */
/*     where                         P  =  C**(-1), */
/*                                Abar  =  P (A - shift*I) P', */
/*                                bbar  =  P b, */

/*     and return the solution       x  =  P' xbar. */
/*     The associated residual is rbar  =  bbar - Abar xbar */
/*                                      =  P (b - (A - shift*I)x) */
/*                                      =  P r. */

/*     In the discussion below, eps refers to the machine precision. */
/*     eps is computed by SYMMLQ.  A typical value is eps = 2.22e-16 */
/*     for IBM mainframes and IEEE double-precision arithmetic. */

/*     Parameters */
/*     ---------- */

/*     n       input      The dimension of the matrix A. */

/*     b(n)    input      The rhs vector b. */

/*     r1(n)   workspace */
/*     r2(n)   workspace */
/*     v(n)    workspace */
/*     w(n)    workspace */

/*     x(n)    output     Returns the computed solution  x. */

/*     y(n)    workspace */

/*     aprod   external   A subroutine defining the matrix A. */
/*                        For a given vector x, the statement */

/*                        old: call aprod ( n, x, y, ) */
/*                       new: call aprod ( n, x, y, A, vwsqrt, work, orthl
ist ) -rwl*/

/*                        must return the product y = Ax */
/*                        without altering the vector x. */

/*     msolve  external   An optional subroutine defining a */
/*                        preconditioning matrix M, which should */
/*                        approximate (A - shift*I) in some sense. */
/*                        M must be positive definite. */
/*                        For a given vector x, the statement */

/*                        old:      call msolve( n, x, y ) */
/*                        new:      call msolve( n, x, y )  -rwl */

/*                        must solve the linear system My = x */
/*                        without altering the vector x. */

/*                        In general, M should be chosen so that Abar has 
*/
/*                        clustered eigenvalues.  For example, */
/*                        if A is positive definite, Abar would ideally */

/*                        be close to a multiple of I. */
/*                        If A or A - shift*I is indefinite, Abar might */

/*                        be close to a multiple of diag( I  -I ). */

/*                        NOTE.  The program calling SYMMLQ must declare 
*/
/*                        aprod and msolve to be external. */

/*     checka  input      If checka = .true., an extra call of aprod will 
*/
/*                        be used to check if A is symmetric.  Also, */
/*                        if precon = .true., an extra call of msolve */
/*                        will be used to check if M is symmetric. */

/*     goodb   input      Usually, goodb should be .false. */
/*                        If x is expected to contain a large multiple of 
*/
/*                        b (as in Rayleigh-quotient iteration), */
/*                        better precision may result if goodb = .true. */

/*                        See Lewis (1977) below. */
/*                        When goodb = .true., an extra call to msolve */
/*                        is required. */

/*     precon  input      If precon = .true., preconditioning will */
/*                        be invoked.  Otherwise, subroutine msolve */
/*                        will not be referenced; in this case the */
/*                        actual parameter corresponding to msolve may */
/*                        be the same as that corresponding to aprod. */

/*     shift   input      Should be zero if the system Ax = b is to be */
/*                        solved.  Otherwise, it could be an */
/*                        approximation to an eigenvalue of A, such as */
/*                        the Rayleigh quotient b'Ab / (b'b) */
/*                        corresponding to the vector b. */
/*                        If b is sufficiently like an eigenvector */
/*                        corresponding to an eigenvalue near shift, */
/*                        then the computed x may have very large */
/*                        components.  When normalized, x may be */
/*                        closer to an eigenvector than b. */

/*     nout    input      A file number. */
/*                        If nout .gt. 0, a summary of the iterations */
/*                        will be printed on unit nout. */

/*     itnlim  input      An upper limit on the number of iterations. */

/*     rtol    input      A user-specified tolerance.  SYMMLQ terminates 
*/
/*                        if it appears that norm(rbar) is smaller than */

/*                              rtol * norm(Abar) * norm(xbar), */
/*                        where rbar is the transformed residual vector, 
*/
/*                              rbar = bbar - Abar xbar. */

/*                        If shift = 0 and precon = .false., SYMMLQ */
/*                        terminates if norm(b - A*x) is smaller than */
/*                              rtol * norm(A) * norm(x). */

/*     istop   output     An integer giving the reason for termination... 
*/

/*              -1        beta2 = 0 in the Lanczos iteration; i.e. the */
/*                        second Lanczos vector is zero.  This means the 
*/
/*                        rhs is very special. */
/*                        If there is no preconditioner, b is an */
/*                        eigenvector of A. */
/*                        Otherwise (if precon is true), let My = b. */
/*                        If shift is zero, y is a solution of the */
/*                        generalized eigenvalue problem Ay = lambda My, 
*/
/*                        with lambda = alpha1 from the Lanczos vectors. 
*/

/*                        In general, (A - shift*I)x = b */
/*                        has the solution         x = (1/alpha1) y */
/*                        where My = b. */

/*               0        b = 0, so the exact solution is x = 0. */
/*                        No iterations were performed. */

/*               1        Norm(rbar) appears to be less than */
/*                        the value  rtol * norm(Abar) * norm(xbar). */
/*                        The solution in  x  should be acceptable. */

/*               2        Norm(rbar) appears to be less than */
/*                        the value  eps * norm(Abar) * norm(xbar). */
/*                        This means that the residual is as small as */
/*                        seems reasonable on this machine. */

/*               3        Norm(Abar) * norm(xbar) exceeds norm(b)/eps, */
/*                        which should indicate that x has essentially */
/*                        converged to an eigenvector of A */
/*                        corresponding to the eigenvalue shift. */

/*               4        acond (see below) has exceeded 0.1/eps, so */
/*                        the matrix Abar must be very ill-conditioned. */

/*                        x may not contain an acceptable solution. */

/*               5        The iteration limit was reached before any of */

/*                        the previous criteria were satisfied. */

/*               6        The matrix defined by aprod does not appear */
/*                        to be symmetric. */
/*                        For certain vectors y = Av and r = Ay, the */
/*                        products y'y and r'v differ significantly. */

/*               7        The matrix defined by msolve does not appear */
/*                        to be symmetric. */
/*                        For vectors satisfying My = v and Mr = y, the */

/*                        products y'y and r'v differ significantly. */

/*               8        An inner product of the form  x' M**(-1) x */
/*                        was not positive, so the preconditioning matrix 
*/
/*                        M does not appear to be positive definite. */

/*                        If istop .ge. 5, the final x may not be an */
/*                        acceptable solution. */

/* 		9	 The norm of the iterate is > than normxlim. */
/* 			 Termination enforced on presumption that inverse */
/* 			 iteration is being performed -rwl. */

/*     itn     output     The number of iterations performed. */

/*     anorm   output     An estimate of the norm of the matrix operator 
*/
/*                        Abar = P (A - shift*I) P',   where P = C**(-1). 
*/

/*     acond   output     An estimate of the condition of Abar above. */
/*                        This will usually be a substantial */
/*                        under-estimate of the true condition. */

/*     rnorm   output     An estimate of the norm of the final */
/*                        transformed residual vector, */
/*                           P (b  -  (A - shift*I) x). */

/*     ynorm   output     An estimate of the norm of xbar. */
/*                        This is sqrt( x'Mx ).  If precon is false, */
/*                        ynorm is an estimate of norm(x). */

/*     A	      input      A pointer variable to the matrix data. Passed */

/* 	                 in to use in revised call to aprod and msolve */
/* 			 added 14 Dec 92 by rwl. */

/*    macheps  output     Used to return the calculated machine precision.
 */
/*                        Added 10 Feb 93 by rwl. */

/*   normxlim input	Used as possible termination criterion. 10 Feb 93 rwl.
*/

/*   itnmin input	Used to enforce a minimum number of itns. 10 Feb 93 rwl.
*/

/*     To change precision */
/*     ------------------- */

/*     Alter the words */
/*            double precision, */
/*            daxpy, dcopy, ddot, dnrm2 */
/*     to their single or double equivalents. */
/*     ------------------------------------------------------------------ 
*/


/*     This routine is an implementation of the algorithm described in */
/*     the following references: */

/*     C.C. Paige and M.A. Saunders,  Solution of Sparse Indefinite */
/*          Systems of Linear Equations, */
/*          SIAM J. Numer. Anal. 12, 4, September 1975, pp. 617-629. */

/*     J.G. Lewis,  Algorithms for Sparse Matrix Eigenvalue Problems, */
/*          Report STAN-CS-77-595, Computer Science Department, */
/*          Stanford University, Stanford, California, March 1977. */

/*     Applications of SYMMLQ and the theory of preconditioning */
/*     are described in the following references: */

/*     D.B. Szyld and O.B. Widlund,  Applications of Conjugate Gradient */

/*          Type Methods to Eigenvalue Calculations, */
/*          in R. Vichnevetsky and R.S. Steplman (editors), */
/*          Advances in Computer Methods for Partial Differential */
/*          Equations -- III, IMACS, 1979, 167-173. */

/*     D.B. Szyld,  A Two-level Iterative Method for Large Sparse */
/*          Generalized Eigenvalue Calculations, */
/*          Ph. D. dissertation, Department of Mathematics, */
/*          New York University, New York, October 1983. */

/*     P.E. Gill, W. Murray, D.B. Ponceleon and M.A. Saunders, */
/*          Preconditioners for indefinite systems arising in */
/*          optimization, SIMAX 13, 1, 292--311, January 1992. */
/*          (SIAM J. on Matrix Analysis and Applications) */
/*     ------------------------------------------------------------------ 
*/


/*     SYMMLQ development: */
/*            1972: First version. */
/*            1975: John Lewis recommended modifications to help with */
/*                  inverse iteration: */
/*                  1. Reorthogonalize v1 and v2. */
/*                  2. Regard the solution as x = x1  +  bstep * b, */
/*                     with x1 and bstep accumulated separately */
/*                     and bstep * b added at the end. */
/*                     (In inverse iteration, b might be close to the */
/*                     required x already, so x1 may be a lot smaller */
/*                     than the multiple of b.) */
/*            1978: Daniel Szyld and Olof Widlund implemented the first */

/*                  form of preconditioning. */
/*                  This required both a solve and a multiply with M. */
/*            1979: Implemented present method for preconditioning. */
/*                  This requires only a solve with M. */
/*            1984: Sven Hammarling noted corrections to tnorm and x1lq. 
*/
/*                  SYMMLQ added to NAG Fortran Library. */
/*     15 Sep 1985: Final F66 version.  SYMMLQ sent to "misc" in netlib. 
*/
/*     16 Feb 1989: First F77 version. */

/*     22 Feb 1989: Hans Mittelmann observed beta2 = 0 (hence failure) */
/*                  if Abar = const*I.  istop = -1 added for this case. */


/*     01 Mar 1989: Hans Mittelmann observed premature termination on */
/*                  ( 1  1  1 )     (   )                   ( 1  1    ) */

/*                  ( 1  1    ) x = ( 1 ),  for which  T3 = ( 1  1  1 ). 
*/
/*                  ( 1     1 )     (   )                   (    1  1 ) */

/*                  T2 is exactly singular, so estimating cond(A) from */
/*                  the diagonals of Lbar is unsafe.  We now use */
/*                  L       or  Lbar         depending on whether */
/*                  lqnorm  or  cgnorm       is least. */

/*     03 Mar 1989: eps computed internally instead of coming in as a */
/*                  parameter. */
/*     07 Jun 1989: ncheck added as a parameter to say if A and M */
/*                  should be checked for symmetry. */
/*                  Later changed to checka (see below). */
/*     20 Nov 1990: goodb added as a parameter to make Lewis's changes */
/*                  an option.  Usually b is NOT much like x.  Setting */
/*                  goodb = .false. saves a call to msolve at the end. */
/*     20 Nov 1990: Residual not computed exactly at end, to save time */
/*                  when only one or two iterations are required */
/*                  (e.g. if the preconditioner is very good). */
/*                  Beware, if precon is true, rnorm estimates the */
/*                  residual of the preconditioned system, not Ax = b. */
/*     04 Sep 1991: Parameter list changed and reordered. */
/*                  integer ncheck is now logical checka. */
/*     22 Jul 1992: Example from Lothar Reichel and Daniela Calvetti */
/*                  showed that beta2 = 0 (istop = -1) means that */
/*                  b is an eigenvector when M = I. */
/*                  More complicated if there is a preconditioner; */
/*                  not clear yet how to describe it. */
/*   14 Dec 1992:  Modified by Robert Leland, Sandia National Laboratories
*/
/* 		   to integrate with a C application code. The matrix */
/*                  data is now passed by reference through symmlq to */
/*                  aprod and msolve. These are now just Fortran wrappers 
*/
/*                  for C codes consistent with the matrix data passed */
/*                  via the pointers "A", "vwsqrt", "work" and "orthlist" 
*/
/*    10 Feb 1993:  Modified by Robert Leland to return calculate machine 
*/
/* 		   precision and terminate if the norm of the iterate gets */
/* 		   above the limit normxlim. Relevant for inverse iteration. */
/* 		   Also incorporated itnmin to enforce minimum number itns. */
/*    17 Aug 1993:  Observed that the Fortran i/o in this routine won't */

/* 		   work because there is no main fortran program to open */
/*                 the standard i/o files. So for this (and other reasons)
*/
/*                  converted the Fortran to C, necessitating inclusion of
 */
/*                  the file f2c.h. To avoid a problem with maintaining */

/* 		   Symmlq, I commented out the i/o within it and instead */
/* 		   report its performance based only on the return value */
/* 	   	   of various parameters. That means we can modifiy the */
/* 		   Fortran source, run f2c and recompile without losing or */
/* 		   re-writing any functionality. */

/*     Michael A. Saunders                    na.saunders@na-net.ornl.gov 
*/
/*     Department of Operations Research    mike@sol-michael.stanford.edu 
*/
/*     Stanford University */
/*     Stanford, CA 94305-4022                             (415) 723-1875 
*/
/*     ------------------------------------------------------------------ 
*/


/*     Subroutines and functions */

/*     USER       aprod, msolve */
/*     BLAS       daxpy, dcopy, ddot , dnrm2 */


/*     Intrinsics and local variables */
    /* Parameter adjustments */
    --vwsqrt;
    --work;
    --y;
    --x;
    --w;
    --v;
    --r2;
    --r1;
    --b;

    /* Function Body */
/*     ------------------------------------------------------------------ 
*/
/*     Compute eps, the machine precision.  The call to daxpy is */
/*     intended to fool compilers that use extra-length registers. */
    eps = .0625;
L10:
    eps /= 2.;
    x[1] = eps;
    y[1] = 1.;
    daxpy_(&c__1, &c_b4, &x[1], &c__1, &y[1], &c__1);
    if (y[1] > 1.) {
	goto L10;
    }
    eps *= 2.;
/*     Return the calculated machine precision - rwl */
    *macheps = eps;
/*     Print heading and initialize. */
/*   This i/o won't work - see note in preamble.. */
/*     if (nout .gt. 0) then */
/*        write(nout, 1000) enter, n, checka, goodb, precon, */
/*     $                     itnlim, rtol, shift */
/*     end if */
    *istop = 0;
    *itn = 0;
    *anorm = 0.;
    *acond = 0.;
    *rnorm = 0.;
    *ynorm = 0.;
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
	x[i] = 0.;
/* L50: */
    }
/*     Set up y for the first Lanczos vector v1. */
/*     y is really beta1 * P * v1  where  P = C**(-1). */
/*     y and beta1 will be zero if b = 0. */
    dcopy_(n, &b[1], &c__1, &y[1], &c__1);
    dcopy_(n, &b[1], &c__1, &r1[1], &c__1);
    if (*precon) {
	msolve_(n, &r1[1], &y[1], a, &vwsqrt[1], &work[1]);
    }
/*     if ( goodb  ) then */
/*        b1  = y(1) */
/*     else */
/*        b1  = zero */
/*     end if */
    beta1 = ddot_(n, &r1[1], &c__1, &y[1], &c__1);
/*     See if msolve is symmetric. */
    if (*checka && *precon) {
	msolve_(n, &y[1], &r2[1], a, &vwsqrt[1], &work[1]);
	s = ddot_(n, &y[1], &c__1, &y[1], &c__1);
	t = ddot_(n, &r1[1], &c__1, &r2[1], &c__1);
	z = (d__1 = s - t, abs(d__1));
	epsa = (s + eps) * pow_dd(&eps, &c_b18);
	if (z > epsa) {
	    *istop = 7;
	    goto L900;
	}
    }
/*     Test for an indefinite preconditioner. */
    if (beta1 < 0.) {
	*istop = 8;
	goto L900;
    }
/*     If b = 0 exactly, stop with x = 0. */
    if (beta1 == 0.) {
	goto L900;
    }
/*     Here and later, v is really P * (the Lanczos v). */
    beta1 = sqrt(beta1);
    s = 1. / beta1;
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
	v[i] = s * y[i];
/* L100: */
    }
/*     See if aprod  is symmetric. */
    aprod_(n, &v[1], &y[1], a, &vwsqrt[1], &work[1], orthlist);
    if (*checka) {
	aprod_(n, &y[1], &r2[1], a, &vwsqrt[1], &work[1], orthlist);
	s = ddot_(n, &y[1], &c__1, &y[1], &c__1);
	t = ddot_(n, &v[1], &c__1, &r2[1], &c__1);
	z = (d__1 = s - t, abs(d__1));
	epsa = (s + eps) * pow_dd(&eps, &c_b18);
	if (z > epsa) {
	    *istop = 6;
	    goto L900;
	}
    }
/*     Set up y for the second Lanczos vector. */
/*     Again, y is beta * P * v2  where  P = C**(-1). */
/*     y and beta will be zero or very small if b is an eigenvector. */
    d__1 = -(*shift);
    daxpy_(n, &d__1, &v[1], &c__1, &y[1], &c__1);
    alfa = ddot_(n, &v[1], &c__1, &y[1], &c__1);
    d__1 = -alfa / beta1;
    daxpy_(n, &d__1, &r1[1], &c__1, &y[1], &c__1);
/*     Make sure  r2  will be orthogonal to the first  v. */
    z = ddot_(n, &v[1], &c__1, &y[1], &c__1);
    s = ddot_(n, &v[1], &c__1, &v[1], &c__1);
    d__1 = -z / s;
    daxpy_(n, &d__1, &v[1], &c__1, &y[1], &c__1);
    dcopy_(n, &y[1], &c__1, &r2[1], &c__1);
    if (*precon) {
	msolve_(n, &r2[1], &y[1], a, &vwsqrt[1], &work[1]);
    }
    oldb = beta1;
    beta = ddot_(n, &r2[1], &c__1, &y[1], &c__1);
    if (beta < 0.) {
	*istop = 8;
	goto L900;
    }
/*     Cause termination (later) if beta is essentially zero. */
    beta = sqrt(beta);
    if (beta <= eps) {
	*istop = -1;
    }
/*     See if the local reorthogonalization achieved anything. */
    denom = sqrt(s) * dnrm2_(n, &r2[1], &c__1) + eps;
    s = z / denom;
    t = ddot_(n, &v[1], &c__1, &r2[1], &c__1) / denom;
/*     if (nout .gt. 0  .and.  goodb) then */
/*        write(nout, 1100) beta1, alfa, s, t */
/*     end if */
/*     Initialize other quantities. */
    cgnorm = beta1;
    gbar = alfa;
    dbar = beta;
    rhs1 = beta1;
    rhs2 = 0.;
    bstep = 0.;
    snprod = 1.;
/* Computing 2nd power */
    d__1 = alfa;
    tnorm = d__1 * d__1;
    ynorm2 = 0.;
    gmax = abs(alfa);
    gmin = gmax;
    if (*goodb) {
	i__1 = *n;
	for (i = 1; i <= i__1; ++i) {
	    w[i] = 0.;
/* L200: */
	}
    } else {
	dcopy_(n, &v[1], &c__1, &w[1], &c__1);
    }
/*     ------------------------------------------------------------------ 
*/
/*     Main iteration loop. */
/*     ------------------------------------------------------------------ 
*/
/*     Estimate various norms and test for convergence. */
L300:
    *anorm = sqrt(tnorm);
    *ynorm = sqrt(ynorm2);
    epsa = *anorm * eps;
    epsx = *anorm * *ynorm * eps;
    epsr = *anorm * *ynorm * *rtol;
    diag = gbar;
    if (diag == 0.) {
	diag = epsa;
    }
/* Computing 2nd power */
    d__1 = rhs1;
/* Computing 2nd power */
    d__2 = rhs2;
    lqnorm = sqrt(d__1 * d__1 + d__2 * d__2);
    qrnorm = snprod * beta1;
    cgnorm = qrnorm * beta / abs(diag);
/*     Estimate  cond(A). */
/*     In this version we look at the diagonals of  L  in the */
/*     factorization of the tridiagonal matrix,  T = L*Q. */
/*     Sometimes, T(k) can be misleadingly ill-conditioned when */
/*     T(k+1) is not, so we must be careful not to overestimate acond. */
    if (lqnorm <= cgnorm) {
	*acond = gmax / gmin;
    } else {
/* Computing MIN */
	d__1 = gmin, d__2 = abs(diag);
	denom = min(d__1,d__2);
	*acond = gmax / denom;
    }
/*     See if any of the stopping criteria are satisfied. */
/*     In rare cases, istop is already -1 from above (Abar = const * I). 
*/
    if (*istop == 0) {
	if (*itn >= *itnlim) {
	    *istop = 5;
	}
/*         if (acond  .ge. 0.1/eps) istop = 4 */
	if (epsx >= beta1) {
	    *istop = 3;
	}
	if (cgnorm <= epsx) {
	    *istop = 2;
	}
	if (cgnorm <= epsr) {
	    *istop = 1;
	}
	if (*istop != 4 && *ynorm >= *normxlim && *normxlim != 0.) {
	    *istop = 9;
	}
    }
    if (*itn < *itnmin) {
	*istop = 0;
    }
/*     ================================================================== 
*/
/*     See if it is time to print something. */
    if (*nout <= 0) {
	goto L600;
    }
    if (*n <= 40) {
	goto L400;
    }
    if (*itn <= 10) {
	goto L400;
    }
    if (*itn >= *itnlim - 10) {
	goto L400;
    }
    if (*itn % 10 == 0) {
	goto L400;
    }
    if (cgnorm <= epsx * (float)10.) {
	goto L400;
    }
    if (cgnorm <= epsr * (float)10.) {
	goto L400;
    }
    if (*acond >= (float).01 / eps) {
	goto L400;
    }
    if (*istop != 0) {
	goto L400;
    }
    goto L600;
/*     Print a line for this iteration. */
L400:
    zbar = rhs1 / diag;
    z = (snprod * zbar + bstep) / beta1;
/*     x1lq   = x(1)  +  b1 * bstep / beta1 */
/*     x1cg   = x(1)  +  w(1) * zbar  +  b1 * z */
/*     if (    itn     .eq. 0) write(nout, 1200) */
/*     write(nout, 1300) itn, x1cg, cgnorm, bstep/beta1, anorm, acond */
/*     if (mod(itn,10) .eq. 0) write(nout, 1500) */
/*     ================================================================== 
*/
/*     Obtain the current Lanczos vector  v = (1 / beta)*y */
/*     and set up  y  for the next iteration. */
L600:
    if (*istop != 0) {
	goto L800;
    }
    s = 1. / beta;
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
	v[i] = s * y[i];
/* L620: */
    }
    aprod_(n, &v[1], &y[1], a, &vwsqrt[1], &work[1], orthlist);
    d__1 = -(*shift);
    daxpy_(n, &d__1, &v[1], &c__1, &y[1], &c__1);
    d__1 = -beta / oldb;
    daxpy_(n, &d__1, &r1[1], &c__1, &y[1], &c__1);
    alfa = ddot_(n, &v[1], &c__1, &y[1], &c__1);
/* Computing 2nd power */
    d__1 = alfa;
/* Computing 2nd power */
    d__2 = beta;
    tnorm = tnorm + d__1 * d__1 + d__2 * d__2 * 2.;
    d__1 = -alfa / beta;
    daxpy_(n, &d__1, &r2[1], &c__1, &y[1], &c__1);
    dcopy_(n, &r2[1], &c__1, &r1[1], &c__1);
    dcopy_(n, &y[1], &c__1, &r2[1], &c__1);
    if (*precon) {
	msolve_(n, &r2[1], &y[1], a, &vwsqrt[1], &work[1]);
    }
    oldb = beta;
    beta = ddot_(n, &r2[1], &c__1, &y[1], &c__1);
    if (beta < 0.) {
	*istop = 6;
	goto L800;
    }
    beta = sqrt(beta);
/*     Compute the next plane rotation for  Q. */
/* Computing 2nd power */
    d__1 = gbar;
/* Computing 2nd power */
    d__2 = oldb;
    gamma = sqrt(d__1 * d__1 + d__2 * d__2);
    cs = gbar / gamma;
    sn = oldb / gamma;
    delta = cs * dbar + sn * alfa;
    gbar = sn * dbar - cs * alfa;
    epsln = sn * beta;
    dbar = -cs * beta;
/*     Update  x. */
    z = rhs1 / gamma;
    s = z * cs;
    t = z * sn;
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
	x[i] = w[i] * s + v[i] * t + x[i];
	w[i] = w[i] * sn - v[i] * cs;
/* L700: */
    }
/*     Accumulate the step along the direction  b, */
/*     and go round again. */
    bstep = snprod * cs * z + bstep;
    snprod *= sn;
    gmax = max(gmax,gamma);
    gmin = min(gmin,gamma);
/* Computing 2nd power */
    d__1 = z;
    ynorm2 = d__1 * d__1 + ynorm2;
    rhs1 = rhs2 - delta * z;
    rhs2 = -epsln * z;
    ++(*itn);
    goto L300;
/*     ------------------------------------------------------------------ 
*/
/*     End of main iteration loop. */
/*     ------------------------------------------------------------------ 
*/
/*     Move to the CG point if it seems better. */
/*     In this version of SYMMLQ, the convergence tests involve */
/*     only cgnorm, so we're unlikely to stop at an LQ point, */
/*     EXCEPT if the iteration limit interferes. */
L800:
    if (cgnorm <= lqnorm) {
	zbar = rhs1 / diag;
	bstep = snprod * zbar + bstep;
/* Computing 2nd power */
	d__1 = zbar;
	*ynorm = sqrt(ynorm2 + d__1 * d__1);
	*rnorm = cgnorm;
	daxpy_(n, &zbar, &w[1], &c__1, &x[1], &c__1);
    } else {
	*rnorm = lqnorm;
    }
    if (*goodb) {
/*        Add the step along  b. */
	bstep /= beta1;
	dcopy_(n, &b[1], &c__1, &y[1], &c__1);
	if (*precon) {
	    msolve_(n, &b[1], &y[1], a, &vwsqrt[1], &work[1]);
	}
	daxpy_(n, &bstep, &y[1], &c__1, &x[1], &c__1);
    }
/*     ================================================================== 
*/
/*     Display final status. */
/*     ================================================================== 
*/
L900:
/*     if (nout  .gt. 0) then */
/*        write(nout, 2000) exit, istop, itn, */
/*    $                     exit, anorm, acond, */
/*    $                     exit, rnorm, ynorm */
/*        write(nout, 3000) exit, msg(istop) */
/*     end if */
    return 0;
/*     ------------------------------------------------------------------ 
*/
/* 1000 format(// 1p,    a, 5x, 'Solution of symmetric   Ax = b' */
/*     $       / ' n      =', i7, 5x, 'checka =', l4, 12x, */
/*     $          'goodb  =', l4, 7x, 'precon =', l4 */
/*     $       / ' itnlim =', i7, 5x, 'rtol   =', e11.2, 5x, */
/*     $          'shift  =', e23.14) */
/* 1100 format(/ 1p, ' beta1  =', e10.2, 3x, 'alpha1 =', e10.2 */
/*     $       / ' (v1,v2) before and after ', e14.2 */
/*     $       / ' local reorthogonalization', e14.2) */
/* 1200 format(// 5x, 'itn', 7x, 'x1(cg)', 10x, */
/*     $         'norm(r)', 5x, 'bstep', 7x, 'norm(A)', 3X, 'cond(A)') */
/* 1300 format(1p, i8, e19.10, e11.2, e14.5, 2e10.2) */
/* 1500 format(1x) */
/* 2000 format(/ 1p, a, 6x, 'istop =', i3,   15x, 'itn   =', i8 */
/*     $       /     a, 6x, 'anorm =', e12.4, 6x, 'acond =', e12.4 */
/*     $       /     a, 6x, 'rnorm =', e12.4, 6x, 'ynorm =', e12.4) */
/* 3000 format(      a, 6x, a ) */
/*     ------------------------------------------------------------------ 
*/
/*     end of SYMMLQ */
} /* symmlq_ */

