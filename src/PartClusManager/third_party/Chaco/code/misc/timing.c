/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include   <stdio.h>

/* Timing parameters. */

double    start_time = -1;
double    total_time = 0;
double    input_time = 0;
double    partition_time = 0;
double 	  sequence_time = 0;
double    kernel_time = 0;
double    reformat_time = 0;
double    check_input_time = 0;
double    count_time = 0;
double    print_assign_time = 0;

double    coarsen_time = 0;
double    match_time = 0;
double    make_cgraph_time = 0;

double    lanczos_time = 0;
double    splarax_time = 0;
double    orthog_time = 0;
double    ql_time = 0;
double    tevec_time = 0;
double    ritz_time = 0;
double    evec_time = 0;
double    blas_time = 0;
double    init_time = 0;
double    scan_time = 0;
double    debug_time = 0;
double    pause_time = 0;

double    rqi_symmlq_time = 0;
double    refine_time = 0;

double    kl_total_time = 0;
double    kl_init_time = 0;
double    nway_kl_time = 0;
double    kl_bucket_time = 0;

double    inertial_time = 0;
double    inertial_axis_time = 0;
double    median_time = 0;

double    sim_time = 0;


void      time_out(outfile)
FILE     *outfile;		/* file to print output to */
{
    FILE     *tempfile;		/* file name for two passes */
    extern int ECHO;		/* parameter for different output styles */
    extern int OUTPUT_TIME;	/* how much timing output should I print? */
    double    time_tol;		/* tolerance for ignoring time */
    double    other_time;	/* time not accounted for */
    int       i;		/* loop counter */

    time_tol = 0.005;

    for (i = 0; i < 2; i++) {
	if (i == 1) {		/* Print to file? */
	    if (ECHO < 0)
		tempfile = outfile;
	    else
		break;
	}
	else {			/* Print to stdout. */
	    tempfile = stdout;
	}


	if (OUTPUT_TIME > 0) {
	    if (total_time != 0) {
		fprintf(tempfile, "\nTotal time: %g sec.\n", total_time);
		if (input_time != 0)
		    fprintf(tempfile, "  input %g\n", input_time);
		if (reformat_time != 0)
		    fprintf(tempfile, "  reformatting %g\n", reformat_time);
		if (check_input_time != 0)
		    fprintf(tempfile, "  checking input %g\n", check_input_time);
		if (partition_time != 0)
		    fprintf(tempfile, "  partitioning %g\n", partition_time);
		if (sequence_time != 0)
		    fprintf(tempfile, "  sequencing %g\n", sequence_time);
		if (kernel_time != 0)
		    fprintf(tempfile, "  kernel benchmarking %g\n", kernel_time);
		if (count_time != 0)
		    fprintf(tempfile, "  evaluation %g\n", count_time);
		if (print_assign_time != 0)
		    fprintf(tempfile, "  printing assignment file %g\n", print_assign_time);

		if (sim_time != 0)
		    fprintf(tempfile, "  simulating %g\n", sim_time);
		other_time = total_time - input_time - reformat_time -
		   check_input_time - partition_time - count_time -
		   print_assign_time - sim_time - sequence_time - kernel_time;
		if (other_time > time_tol)
		    fprintf(tempfile, "  other %g\n", other_time);
	    }
	}

	if (OUTPUT_TIME > 1) {
	    if (inertial_time != 0) {
		if (inertial_time != 0)
		    fprintf(tempfile, "\nInertial time: %g sec.\n", inertial_time);
		if (inertial_axis_time != 0)
		    fprintf(tempfile, "  inertial axis %g\n", inertial_axis_time);
		if (median_time != 0)
		    fprintf(tempfile, "  median finding %g\n", median_time);
		other_time = inertial_time - inertial_axis_time - median_time;
		if (other_time > time_tol)
		    fprintf(tempfile, "  other %g\n", other_time);
	    }

	    if (kl_total_time != 0) {
		if (kl_total_time != 0)
		    fprintf(tempfile, "\nKL time: %g sec.\n", kl_total_time);
		if (kl_init_time != 0)
		    fprintf(tempfile, "  initialization %g\n", kl_init_time);
		if (nway_kl_time != 0)
		    fprintf(tempfile, "  nway refinement %g\n", nway_kl_time);
		if (kl_bucket_time != 0)
		    fprintf(tempfile, "    bucket sorting %g\n", kl_bucket_time);
		other_time = kl_total_time - kl_init_time - nway_kl_time;
		if (other_time > time_tol)
		    fprintf(tempfile, "  other %g\n", other_time);
	    }

	    if (coarsen_time != 0 && rqi_symmlq_time == 0) {
		fprintf(tempfile, "\nCoarsening %g sec.\n", coarsen_time);
		if (match_time != 0)
		    fprintf(tempfile, "  maxmatch %g\n", match_time);
		if (make_cgraph_time != 0)
		    fprintf(tempfile, "  makecgraph %g\n", make_cgraph_time);
	    }

	    if (lanczos_time != 0) {
		fprintf(tempfile, "\nLanczos time: %g sec.\n", lanczos_time);
		if (splarax_time != 0)
		    fprintf(tempfile, "  matvec/solve %g\n", splarax_time);
		if (blas_time != 0)
		    fprintf(tempfile, "  vector ops %g\n", blas_time);
		if (evec_time != 0)
		    fprintf(tempfile, "  assemble evec %g\n", evec_time);
		if (init_time != 0)
		    fprintf(tempfile, "  malloc/init/free %g\n", init_time);
		if (orthog_time != 0)
		    fprintf(tempfile, "  maintain orthog %g\n", orthog_time);
		if (scan_time != 0)
		    fprintf(tempfile, "  scan %g\n", scan_time);
		if (debug_time != 0)
		    fprintf(tempfile, "  debug/warning/check %g\n", debug_time);
		if (ql_time != 0)
		    fprintf(tempfile, "  ql/bisection %g\n", ql_time);
		if (tevec_time != 0)
		    fprintf(tempfile, "  tevec %g\n", tevec_time);
		if (ritz_time != 0)
		    fprintf(tempfile, "  compute ritz %g\n", ritz_time);
		if (pause_time != 0)
		    fprintf(tempfile, "  pause %g\n", pause_time);
		other_time = lanczos_time - splarax_time - orthog_time
		   - ql_time - tevec_time - ritz_time - evec_time
		   - blas_time - init_time - scan_time - debug_time - pause_time;
		if (other_time > time_tol && other_time != lanczos_time) {
		    fprintf(tempfile, "  other %g\n", other_time);
		}
	    }

	    if (rqi_symmlq_time != 0) {
		fprintf(tempfile, "\nRQI/Symmlq time: %g sec.\n", rqi_symmlq_time);
		if (coarsen_time != 0)
		    fprintf(tempfile, "  coarsening %g\n", coarsen_time);
		if (match_time != 0)
		    fprintf(tempfile, "    maxmatch %g\n", match_time);
		if (make_cgraph_time != 0)
		    fprintf(tempfile, "    makecgraph %g\n", make_cgraph_time);
		if (refine_time != 0)
		    fprintf(tempfile, "  refinement %g\n", refine_time);
		if (lanczos_time != 0)
		    fprintf(tempfile, "  lanczos %g\n", lanczos_time);
		other_time = rqi_symmlq_time - coarsen_time - refine_time - lanczos_time;
		if (other_time > time_tol)
		    fprintf(tempfile, "  other %g\n", other_time);
	    }
	}
    }
}


void      clear_timing()
{
    start_time = -1;
    total_time = 0;
    input_time = 0;
    partition_time = 0;
    sequence_time = 0;
    kernel_time = 0;
    reformat_time = 0;
    check_input_time = 0;
    count_time = 0;
    print_assign_time = 0;

    coarsen_time = 0;
    match_time = 0;
    make_cgraph_time = 0;

    lanczos_time = 0;
    splarax_time = 0;
    orthog_time = 0;
    ql_time = 0;
    tevec_time = 0;
    ritz_time = 0;
    evec_time = 0;
    blas_time = 0;
    init_time = 0;
    scan_time = 0;
    debug_time = 0;
    pause_time = 0;

    rqi_symmlq_time = 0;
    refine_time = 0;

    kl_total_time = 0;
    kl_init_time = 0;
    nway_kl_time = 0;
    kl_bucket_time = 0;

    inertial_time = 0;
    inertial_axis_time = 0;
    median_time = 0;

    sim_time = 0;
}
