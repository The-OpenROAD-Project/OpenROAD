/* An array of these stores all the data for the graph/matrix. */
struct vtx_data {
	int vwgt;		/* weight of vertex */
	int nedges;		/* number of neighbors of vertex in subgraph */
				/* Note: above always includes self-edge first */
	int *edges;		/* neighbor list in subgraph numbering scheme */
	float *ewgts;		/* weights of all the edges */
				/* Note: above 2 fields have self-edge first */
};


/* An Array of lists made of these stores scheduler's message table. */
struct msg_data{
	int dest;		/* destination of the message */
	double dur;		/* duration of message */
	double beg;		/* time at which message begins */
	double end;		/* time at which message end */
	struct list *route;	/* linked list of ints stores message route */
	struct msg_data *pntr;	/* pointer to next outgoing message from this set */
};


/* A linked list of these stores the selective orthogonalization set */
struct orthlink {
	int depth;		/* bottom of list is 0, previous is 1 etc */
	int index;		/* position in list of ritz vals (i index) */
	double ritzval;		/* good ritz value */		
	double betaji;		/* residual bound on good ritz pair */ 
	double tau;		/* from orthogonality recursion */
	double prevtau;		/* from orthogonality recursion */
	double *vec;		/* vector to orthogonalize against */
	struct orthlink *pntr;	/* pointer to next link */
};

/* A linked list of these stores the selective orthogonalization set */
struct orthlink_float {
	int depth;		/* bottom of list is 0, previous is 1 etc */
	int index;		/* position in list of ritz vals (i index) */
	double ritzval;		/* good ritz value */		
	double betaji;		/* residual bound on good ritz pair */ 
	double tau;		/* from orthogonality recursion */
	double prevtau;		/* from orthogonality recursion */
	float *vec;		/* vector to orthogonalize against */
	struct orthlink_float *pntr;	/* pointer to next link */
};



/* A linked list of these stores the minimum elements of a vector */
struct scanlink {
	double val;		/* value of vector entry */
	int indx;		/* index of corresponding entry */
	struct scanlink *pntr;	/* pointer to next link */
};

/* These store the phantom edges needed to keep a subgraph connected */
struct edgeslist {
	 int vtx1;               /* first vertex in edge */
	 int vtx2;               /* second vertex in edge */
	 struct edgeslist *next; /* pointer to next element in list */
 };

/* These store all the data needed to modify edges for connectivity. */
struct connect_data {
	struct ilists *old_edges;	/* overwritten old edges */
	struct flists *old_ewgts;	/* overwritten old weights */
	struct edgeslist *new_edges;	/* list of new edges */
	int old_nedges;		/* original number of edges in graph */
};

/* Information about subsets of processors is needed in recurse. */
struct set_info {
	short setnum;		/* assignment value for this set */
	short ndims;		/* log of # processors if hypercube */
	short low[3];		/* low limit for grid dimensions if mesh */
	short span[3];		/* size of grid dimensions if mesh */
	struct set_info *next;	/* pointer to next element in linked list */
};

/* Linked list stuff for various uses */
struct list {				/* linked list of integers */
	int num;			/* element number */
	struct list *next;		/* ptr to next element in list */
};

struct lists {				/* linked list of lists */
	struct list *begin;		/* pointer to list */
	struct lists *nextlist;		/* next list header */
};

struct bilist {				/* bidirectional list */
	struct bilist *prev;		/* pointer to previous element */
	struct bilist *next;		/* ptr to next element in list */
};

struct ipairs {				/* stores pairs of integers */
	int val1;
	int val2;
};

struct ilists {				/* linked list of integer lists */
	int *list;			
	struct ilists *next;
};

struct flists {				/* linked list of floating lists */
	float *list;			
	struct flists *next;
};
