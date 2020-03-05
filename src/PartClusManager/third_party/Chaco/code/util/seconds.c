/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include   <sys/time.h>
#include   <sys/resource.h>

double    seconds()
{
    double    curtime;

#ifdef RUSAGE_SELF

/* This timer is faster and more robust (if it exists). */
    struct rusage rusage;
    int getrusage();
 
    getrusage(RUSAGE_SELF, &rusage);
    curtime = ((rusage.ru_utime.tv_sec + rusage.ru_stime.tv_sec) +
	    1.0e-6 * (rusage.ru_utime.tv_usec + rusage.ru_stime.tv_usec));

#else

/* ANSI timer, but lower resolution & wraps around after ~36 minutes. */

    curtime = clock()/((double) CLOCKS_PER_SEC);

#endif

    return (curtime);
}
