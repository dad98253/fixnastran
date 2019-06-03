/*
 * fixnastran.h
 *
 *  Created on: Jun 1, 2019
 *      Author: dad
 */

#ifndef FIXNASTRAN_H_
#define FIXNASTRAN_H_

#define MAXROUTINESIZE 50000
#define BUFFLEN 5000

typedef struct _VARDATA       /* var data struct */
{
	int *			refs;		// pointer to an array containing line number index of each reference
	char *			name;		// pointer to a null terminated string containing the name of the variable
	char **			dims;		// an array of vardim null terminated strings defining the dimensions of the variable
	int				typedefline;// the line number that we think defines the type of the variable (only valid if numrefs != 0)
	unsigned int	numrefs;	// size of the ref array
    unsigned int	elesize;	// This element could be smaller (e.g., unsigned char), but try to keep 64 bit alignment... elesize is the size
    							// (in bytes) of a single unit. e.g., INTEGER*4 would have elesize = 4.
	unsigned char	dimsdefined;// 0-> we have no clue (or simple), dims array will be undefined; 1-> we think we know, can look in dims array
    unsigned char	vardim;		// number of dimensions, 0 -> simple
    unsigned char	vartype;	// see type defs bellow
    unsigned char	paramflag;	// 0-> we have no clue, 1-> not in calling parameter list, 2-> is a formal parameter, 4-> a simple constant (name field will point to
    							// a string representation of the constant)
} VARDATA, *PVARDATA;

typedef const VARDATA   VARDEF;
typedef const VARDEF * PVARDEF;

//typedef struct PVARTREE   * VARTREET;

typedef struct VARTREET       /* var tree struct */
{
	struct VARTREET **	branch;	// link to the list of branches
    PVARDATA  	data;			// pointer to the variable data structure if appropriate, by convention, th etop node willpoint to a data struct with name=="9t9o9p9"
    struct VARTREET *	parent;	// link to the parent node;
	int numbranches;			// number of branches defined
	int numbraalloc;			// number of branch pointer space allowcated (must be >= numberbranches, else remalloc your list
} VARTREE, *PVARTREE, **PPVARTREE;

typedef const VARTREE   VARTREEDEF;
typedef const VARTREEDEF * PVARTREEDEF;



#endif /* FIXNASTRAN_H_ */
