#include "params.h"

int       interface_wrap(int nvtxs, int *start, int *adjacency, int *vwgts, float *ewgts, float *x, float *y, float *z,
		              char *outassignname, char *outfilename,
		              short *assignment,
		              int architecture, int ndims_tot, int mesh_dims[3], double *goal,
		              int global_method, int local_method, int rqi_flag, int vmax, int ndims,
		              double eigtol, long seed,
					  int tprop, double kl_inbalance, double coarsening_ratio, double cut_to_hop_cost,
					  int debug_print, int refine_part, int level);

//int       nvtxs;		/* number of vertices in full graph */
//int      *start;		/* start of edge list for each vertex */
//int      *adjacency;		/* edge list data */
//int      *vwgts;		/* weights for all vertices */
//float    *ewgts;		/* weights for all edges */
//float    *x, *y, *z;		/* coordinates for inertial method */
//char     *outassignname;	/* name of assignment output file */
//char     *outfilename;		/* output file name */
//short    *assignment;		/* set number of each vtx (length n) */
//int       architecture;		/* 0 => hypercube, d => d-dimensional mesh */
//int       ndims_tot;		/* total number of cube dimensions to divide */
//int       mesh_dims[3];		/* dimensions of mesh of processors */
//double   *goal;			/* desired set sizes for each set */
//int       global_method;	/* global partitioning algorithm */
//int       local_method;		/* local partitioning algorithm */
//int       rqi_flag;		/* should I use RQI/Symmlq eigensolver? */
//int       vmax;			/* how many vertices to coarsen down to? */
//int       ndims;		/* number of eigenvectors (2^d sets) */
//double    eigtol;		/* tolerance on eigenvectors */
//long      seed;			/* for random graph mutations */
//int 		tprop;	  /* perform terminal propagation */
//double 	kl_inbalance;	/* imbalance tolerated by KL */
//double 	coarsening_ratio; /* min vtx reduction at each coarsen stage */
//double 	cut_to_hop_cost;	/* relative cut/hop importance */
//int 		debug_print;	/* debug text for PartClusManager */
//int 		level	/* clustering level to export to db */

int*      clustering_wrap();
