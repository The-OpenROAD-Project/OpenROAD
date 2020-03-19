int       interface_wrap(int nvtxs, int *start, int *adjacency, int *vwgts, float *ewgts, float *x, float *y, float *z,
		              char *outassignname, char *outfilename,
		              short *assignment,
		              int architecture, int ndims_tot, int mesh_dims[3], double *goal,
		              int global_method, int local_method, int rqi_flag, int vmax, int ndims,
		              double eigtol, long seed)
{
    interface(nvtxs, start, adjacency, vwgts, ewgts, x, y, z,
		              outassignname, outfilename,
		              assignment,
		              architecture, ndims_tot, mesh_dims, goal,
		              global_method, local_method, rqi_flag, vmax, ndims,
		              eigtol, seed);
}
