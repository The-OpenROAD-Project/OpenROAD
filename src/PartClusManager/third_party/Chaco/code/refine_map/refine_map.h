struct refine_vdata {
	float above;		/* sum of edge weights pulling me higher */
	float below;		/* sum of edge weights pulling me lower */
	float same;		/* sum of edge weights keeping me here */
};

struct refine_edata {
	short node1, node2;	/* nodes in mesh connected by this edge */
	short dim;		/* which dimension of mesh does wire span? */
	float swap_desire;	/* reduction in hops if edge is flipped */
	struct refine_edata *prev;	/* pointer to previous guy in list */
	struct refine_edata *next;	/* pointer to next guy in list */
};
