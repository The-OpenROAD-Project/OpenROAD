/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include <malloc.h>

static int nmalloc = 0;		/* number of calls to malloc */
static int nfree = 0;		/* number of calls to free */
static int bytes_used = 0;	/* current dynamic memory usage */
static int bytes_max = 0;	/* largest dynamic memory usage */

static struct smalloc_debug_data {
    int       order;			/* which smalloc call is it? */
    unsigned  size;			/* size of malloc invocation */
    double   *ptr;			/* memory location returned */
    struct smalloc_debug_data *next;	/* pointer to next element */
}        *top = NULL;


/* Safe version of malloc.  Does not initialize memory .*/
double   *smalloc(n)
unsigned int n;				/* number of bytes to be allocated */
{
    extern FILE *Output_File;		/* output file or null */
    extern int DEBUG_MEMORY;		/* use debug memory allocator? */
    double   *ptr;			/* return value */
    struct smalloc_debug_data *new;	/* data structure for malloc data */
    void      bail();

    nmalloc++;
    if (n == 0) {
	printf("ERROR: Non-positive argument to smalloc (%u).\n", n);
	if (Output_File != NULL) {
	    fprintf(Output_File, 
		"ERROR: Non-positive argument to smalloc (%u).\n", n);
	}
	bail((char *) NULL, 1);
    }

    ptr = (double *) malloc(n);

    if (ptr == NULL) {
	printf("Program out of space while attempting to allocate %u.  Sorry!\n", n);
	if (Output_File != NULL) {
	    fprintf(Output_File, 
	        "Program out of space while attempting to allocate %u.  Sorry!\n", n);
	}
	bail((char *) NULL, 1);
    }

    if (DEBUG_MEMORY > 1) {
	new = (struct smalloc_debug_data *)
	   malloc(sizeof(struct smalloc_debug_data));

	if (new == NULL) {
	    printf("WARNING: No space for malloc_debug %u.\n", n);
	    if (Output_File != NULL) {
	        fprintf(Output_File,
		    "WARNING: No space for malloc_debug %u.\n", n);
	    }
	    return(ptr);
	}

	new->order = nmalloc;
	new->size = n;
	new->ptr = ptr;
	new->next = top;
	top = new;
	bytes_used += n;
	if (bytes_used > bytes_max) {
	    bytes_max = bytes_used;
	}
    }

    if (DEBUG_MEMORY > 2) {
        printf(" order=%d, size=%u, location=0x%lx\n", nmalloc, n, (long) ptr);
    }

    return (ptr);
}


/* Safe version of malloc.  Does not initialize memory .*/
/* Returns instead of dying if it fails. */
double   *smalloc_ret(n)
unsigned int n;				/* number of bytes to be allocated */
{
    extern FILE *Output_File;		/* output file or null */
    extern int DEBUG_MEMORY;		/* use debug memory allocator? */
    double   *ptr;			/* return value */
    struct smalloc_debug_data *new;	/* data structure for malloc data */

    ptr = NULL;
    if (n == 0) {
	printf("ERROR: Non-positive argument to smalloc_ret (%u).\n", n);
	if (Output_File != NULL) {
	    fprintf(Output_File,
		"ERROR: Non-positive argument to smalloc_ret (%u).\n", n);
	}
    }

    else {
        nmalloc++;
        ptr = (double *) malloc(n);

        if (ptr == NULL) {
	    nmalloc--;
            if (DEBUG_MEMORY > 0) {
	        printf("WARNING: No space in smalloc_ret while allocating %u.\n", n);
	        if (Output_File != NULL) {
	            fprintf(Output_File,
		        "WARNING: No space in smalloc_ret while allocating %u.\n", n);
	        }
	    }
        }

	else {

	    if (DEBUG_MEMORY > 1) {
	        new = (struct smalloc_debug_data *)
	           malloc(sizeof(struct smalloc_debug_data));

	        if (new == NULL) {
	            printf("WARNING: No space for malloc_debug %u.\n", n);
	            if (Output_File != NULL) {
	                fprintf(Output_File,
			    "WARNING: No space for malloc_debug %u.\n", n);
		    }
		    return(ptr);
	        }

	        new->order = nmalloc;
	        new->size = n;
	        new->ptr = ptr;
	        new->next = top;
	        top = new;
		bytes_used += n;
		if (bytes_used > bytes_max) {
		    bytes_max = bytes_used;
		}
	    }

	    if (DEBUG_MEMORY > 2) {
        	printf(" order=%d, size=%u, location=0x%lx\n", nmalloc, n, (long) ptr);
	    }

	}
    }

    return (ptr);
}


/* Safe version of realloc */
double   *srealloc(ptr, n)
char     *ptr;			/* pointer to current location */
unsigned int n;			/* desired size of new allocation */
{
    extern FILE *Output_File;	/* output file or null */
    double   *p;		/* returned pointer */
    extern int DEBUG_MEMORY;	/* use debug memory allocator? */
    struct smalloc_debug_data *dbptr;	/* loops through debug list */
    int       sfree();
    void      bail();

    if (ptr == NULL) {
	if (n == 0) {
	    return ((double *) NULL);
	}
	else {
	    p = smalloc(n);
	}
    }
    else {
	if (n == 0) {
	    sfree((char *) ptr);
	    return ((double *) NULL);
	}
	else {
	    p = (double *) realloc((char *) ptr, n);
	    if (DEBUG_MEMORY > 1) {
		for (dbptr = top; dbptr != NULL && (char *) dbptr->ptr != ptr; dbptr = dbptr->next) ;
		if (dbptr == NULL) {
		    printf("Memory error: In srealloc, address not found in debug list (0x%lx)\n",
			   (long) ptr);
		}
		else {
		    dbptr->size = n;
		    dbptr->ptr = p;
		    bytes_used += n;
		    if (bytes_used > bytes_max) {
		        bytes_max = bytes_used;
		    }
		}
	    }
	}
    }

    if (p == NULL) {
	printf("Program out of space while attempting to reallocate %u.\n", n);
	if (Output_File != NULL) {
	    fprintf(Output_File, 
	        "Program out of space while attempting to reallocate %u.\n", n);
	}
	bail((char *) NULL, 1);
    }
    return (p);
}


/* Safe version of realloc */
/* Returns instead of dying if it fails. */
double   *srealloc_ret(ptr, n)
char     *ptr;			/* pointer to current location */
unsigned int n;			/* desired size of new allocation */
{
    extern FILE *Output_File;	/* output file or null */
    double   *p;		/* returned pointer */
    extern int DEBUG_MEMORY;	/* use debug memory allocator? */
    struct smalloc_debug_data *dbptr;	/* loops through debug list */
    int       sfree();

    if (ptr == NULL) {
	if (n == 0) {
	    return ((double *) NULL);
	}
	else {
	    p = smalloc(n);
	}
    }
    else {
	if (n == 0) {
	    sfree((char *) ptr);
	    return ((double *) NULL);
	}
	else {
	    p = (double *) realloc((char *) ptr, n);
	    if (DEBUG_MEMORY > 1) {
		for (dbptr = top; dbptr != NULL && (char *) dbptr->ptr != ptr; dbptr = dbptr->next) ;
		if (dbptr == NULL) {
		    printf("Memory error: In srealloc_ret, address not found in debug list (0x%lx)\n",
			   (long) ptr);
		}
		else {
		    dbptr->size = n;
		    dbptr->ptr = p;
		    bytes_used += n;
		    if (bytes_used > bytes_max) {
		        bytes_max = bytes_used;
		    }
		}
	    }
	}
    }

    if (p == NULL && DEBUG_MEMORY > 0) {
	printf("WARNING: Program out of space while attempting to reallocate %u.\n", n);
	if (Output_File != NULL) {
	    fprintf(Output_File, 
	        "WARNING: Program out of space while attempting to reallocate %u.\n", n);
	}
    }
    return (p);
}


/* Safe version of free. */
int       sfree(ptr)
char     *ptr;
{
    extern FILE *Output_File;	/* output file or null */
    extern int DEBUG_MEMORY;	/* use debug memory allocator? */
    struct smalloc_debug_data *dbptr;	/* loops through debug list */
    struct smalloc_debug_data **prev;	/* holds previous pointer */

    if (DEBUG_MEMORY > 1) {
	if (ptr != NULL) {	/* search through debug list for it */
	    prev = &top;
	    for (dbptr = top; dbptr != NULL && (char *) dbptr->ptr != ptr; dbptr = dbptr->next) {
		prev = &(dbptr->next);
	    }
	    if (dbptr == NULL) {
		printf("Memory error: In sfree, address not found in debug list (0x%lx)\n",
		    (long) ptr);
		if (Output_File != NULL) {
		    printf("Memory error: In sfree, address not found in debug list (0x%lx)\n",
		        (long) ptr);
		}
	    }
	    else {
		*prev = dbptr->next;
		bytes_used =- dbptr->size;
		free((char *) dbptr);
	    }
	}
    }


    if (ptr != NULL) {
	nfree++;
	free(ptr);
	ptr = NULL;
    }

    return (0);
}


void      smalloc_stats()
{
    extern int DEBUG_MEMORY;	/* use debug memory allocator? */
    struct smalloc_debug_data *dbptr;	/* loops through debug list */

    if (DEBUG_MEMORY == 1) {
	printf("Calls to smalloc = %d,  Calls to sfree = %d\n", nmalloc, nfree);
    }
    if (DEBUG_MEMORY > 1) {
	printf("Calls to smalloc = %d,  Calls to sfree = %d, maximum bytes = %d\n",
	       nmalloc, nfree, bytes_max);
	if (top != NULL) {
	    printf("Remaining allocations:\n");
	    for (dbptr = top; dbptr != NULL; dbptr = dbptr->next) {
		printf(" order=%d, size=%u, location=0x%lx\n", dbptr->order,
		       dbptr->size, (long) dbptr->ptr);
	    }
	}
    }
}
