/*
 * parse.c
 *
 *  Created on: Jun 1, 2019
 *      Author: dad
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "fixnastran.h"

extern char *statementkeywords[];

extern VARDEF vardatadef;		// default values for a newly found variable (befor being filled in by correct data)
														// note that we are assuming that NULL == 0 here (not always true on all systems, cf, msvc)

extern PVARDATA vardata;		// pointer to the variable data structure
extern int vardatasize;			// current number of elements in the vardata array 0-. not defined yet

extern VARTREEDEF vartreedef;	// pointer to the default valuse of a parse tree node

extern PVARTREE vartree;		// pointer to the parse tree structure

extern char * parsethis;		// pointer to the string to parse
extern char * nextparsechar;

extern char * topnodelabel;		// what we will put in the name field for the top node
// char * topnodelabel;		// what we will put in the name field for the top node

int labelleaf( PVARTREE vartree, char * label);
PVARTREE AddTreeBranch( PVARTREE vartree);
/* arith0.c: simple parser -- no output
 * grammar:
 *   P ---> E '#'				/
 *   E ---> T {('+'|'-') T}		,
 *   T ---> S {('*'|'/') S}		V(
 *   S ---> F '^' S | F
 *   F ---> char | '(' E ')'
 */

char next;
void E(void);
void T(void);
void S(void);
void F(void);
void error(int);
void scan(void);
void enter(char);
void leave(char);
void spaces(int);
void clearstr();
void pushchr( char mychar);
void printvar();
PVARTREE getparentof(PVARTREE currentbranch);
void pushbranch();
void popbranch();
char * whichbranch( PVARTREE thetree );
int NumChildBranch( PVARTREE thetree );
int dumpparsetree(PVARTREE vartree);
int DeleteTree(PVARTREE vartree, int verbose);
char * GetBranchName(PVARTREE mytree);
int DeleteVarData( PVARDATA mydata);
int PrintVarData( PVARDATA mydata);
int printParseNode(PVARTREE mynode, int verbose);
PVARTREE GetBranch( PVARTREE vartree, int index);


int level = 0;

char EOL = '/';
int iprint = 1;
char datastr[BUFFLEN];
int datastrloc;
PVARTREE currentbranch;
PVARTREE parent;
PVARTREE topbranch;
PVARTREE newbranch;
int slashlevel;
int parsethislen;

int parsecommon(void)
{

   newbranch = parent = NULL;
   slashlevel = 0;
   if ( ( currentbranch = topbranch = vartree = AddTreeBranch(parent) ) == NULL ) return 1;
   if ( labelleaf( currentbranch, topnodelabel ) ) return 2;
   nextparsechar = parsethis;
   parsethislen = strlen(parsethis);
   clearstr();
   level = 0;
   scan();
   E();
   printvar();
   pushbranch();
   if (next != EOL) {
	   fprintf(stderr,"Unsuccessful parse, next = %c\n",next);
	   return 1;
   }
   else {
	   if (strcmp ( GetBranchName(currentbranch) , "COMMON" ) == 0 ) {
		   while ( nextparsechar < parsethis+parsethislen ) {
			   // get the common block name
			   	 clearstr();
			     level = 0;
			     scan();
			     E();
			     printvar();
			     pushbranch(); // should point us back to the node with the name of the common block
			     if (next != EOL) {
			  	   fprintf(stderr,"Unsuccessful parse, next = %c\n",next);
			  	   return 1;
			     }
			     // get all the common block variables
			   	 clearstr();
			     level = 0;
			     scan();
			     E();
			     printvar();
			     popbranch();	// should pop us back to the node with the common block name
//			     popbranch();	// should pop us back to the node that is labeled "COMMON"
			     if (next != EOL) {
			  	   fprintf(stderr,"Unsuccessful parse, next = %c\n",next);
			  	   return 1;
			     }
		   }
	   }
	   if ( iprint ) return 0;
	   printf("Successful parse\n");
   }
   return 0;
}

void E(void)
{
   enter('E');
   T();
   while (next == ','  || next == '(') {
	   printvar();
      scan();
      T();
   }
   leave('E');
}

void T(void)
{
   enter('T');
   S();
   while ((next) == '-' || next == '*' ) {
//	   pushbranch();
	   printvar();
	   pushchr(next);
	   printvar();
//	   pushbranch();

      scan();
      S();
   }
   leave('T');
}
void S(void)
{
   enter('S');
   F();
   if (next == '(') {
	   printvar();
	   pushbranch();
 //     scan();
 //     S();
	   E();
   }
   leave('S');
}
void F(void)
{
   enter('F');
   if (isalpha(next)  || isdigit(next)) {
      while ( isalpha(next) || isdigit(next) ) {
    	  pushchr(next);
    	  scan();
      }
   }
   else if (next == '(') {
	   printvar();
	   pushbranch();
      scan();
      E();
      if (next == ')') {
    	  printvar();
    	  popbranch();
    	  scan();
      }
      else error(2);
   }
   else {
      error(3);
   }
   leave('F');
}
void scan(void)
{
   while ( isspace( next = *(nextparsechar++) ) )
      ;
   if ( nextparsechar > parsethis+parsethislen ) next = EOL;
}
void error(int n)
{
   printf("\n*** ERROR in parser: %i, next = %c \n", n, next);
   exit(1);
}
void enter(char name)
{
   if ( iprint ) return;
   spaces(level++);
   printf("+-%c: Enter, \t", name);
   printf("Next == %c\n", next);
}
void leave(char name)
{
   if ( iprint ) return;
   spaces(--level);
   printf("+-%c: Leave, \t", name);
   printf("Next == %c\n", next);
}

void spaces(int local_level)
{
   if ( iprint ) return;
   while (local_level-- > 0)
      printf("| ");
}

int labelleaf( PVARTREE vartree, char * label){
	char * mylabel;
	int lablen = strlen(label);
	PVARDATA  mydata;
	if ( !lablen ) {
		printf("labelleaf called with zero length label\n");
		return 1;
	}
	mydata = vartree->data;
	if ( mydata == NULL ) {
		if ( ( mydata = vartree->data =  (PVARDATA)malloc(sizeof(vardatadef)) ) == NULL ){
	//		if ( ( mydata = vartree->data =  (PVARDATA)malloc(sizeof(vardatadef)) ) == NULL ){
			printf("malloc failed in labelleaf adding data struct\n");
			return 2;
		}
		*mydata = vardatadef;
	}
	if ( ( mylabel =  (char *)malloc(lablen + 1) ) == NULL ){
		printf("malloc failed in labelleaf adding label string\n");
		return 3;
	}
	strcpy(mylabel,label);
	if ( mydata->name != NULL ) free(mydata->name);
	mydata->name = mylabel;

	return 0;
}

PVARTREE AddTreeBranch( PVARTREE vartree){
	PVARTREE newbranch;
	PPVARTREE branchlist;

	if ( ( newbranch = (PVARTREE)malloc(sizeof(vartreedef)) ) == NULL ) {
		printf("malloc failed in AddTreeBranch\n");
		return(NULL);
	}
	*newbranch = vartreedef;
	newbranch->parent = vartree;
	if ( vartree != NULL ) {
		vartree->numbranches++;
		if ( vartree->numbranches > vartree->numbraalloc ) {
			vartree->branch = (PPVARTREE )realloc( vartree->branch , sizeof(PVARTREE) * (vartree->numbraalloc+100) );
			vartree->numbraalloc+=100;
		}
		branchlist = vartree->branch;
		*(branchlist+vartree->numbranches-1) = newbranch;
	}

	return(newbranch);
}

char * whichbranch( PVARTREE thetree ) {		// depricated in favor of GetBranchName... do not use in new code
	PVARTREE mytree;
	PVARDATA  mydata;
	char * mylabel;

	if ( thetree == NULL ) return (NULL);
	mytree = thetree;
	if ( mytree->data == NULL ) return (NULL);
	mydata = mytree->data;
	if ( mydata->name == NULL ) return (NULL);
	mylabel = mydata->name;

	return (mylabel);
}

char * GetBranchName(PVARTREE mytree) {
	char * mylabel;
	PVARDATA  mydata;

	if ( mytree == NULL ) {
		printf("GetBranchName called with NULL node\n");
		return (NULL);
	}
	if ( ( mydata = mytree->data ) == NULL ){
		printf("Node passed to GetBranchName has no data associated with it\n");
		return (NULL);
	}
	if ( ( mylabel =  mydata->name ) == NULL ){
		printf("Data block associated with node passed to GetBranchName has empty name field\n");
		return (NULL);
	}

	return (mylabel);

}

int NumChildBranch( PVARTREE thetree ) {
	PVARTREE mytree;

	if ( thetree == NULL ) return (-1);
	mytree = thetree;

	return (mytree->numbranches);
}

PVARTREE GetBranch( PVARTREE vartree, int index){
	PVARTREE newbranch;
	PPVARTREE branchlist;

	if ( vartree == NULL ) {
		printf("GetBranch called with NULL tree struct\n");
		return(NULL);
	}
	if ( index < 0 ) {
		printf("GetBranch called with negative branch index\n");
		return(NULL);
	}
	if ( (index+1) > vartree->numbranches ) {
		printf("GetBranch called with index = %i, but node only has %i branches\n", index, vartree->numbranches);
		return(NULL);
	}
	if ( vartree->branch == NULL ) {
		printf("GetBranch detected ill-formated VARTREE struct: branch data missing\n");
		return(NULL);
	}
	branchlist = vartree->branch;
	newbranch = *(branchlist+index);

	if ( newbranch == NULL ) {
		printf("GetBranch detected ill-formated VARTREE struct: branch(%i) is missing from %s\n",index,GetBranchName(vartree));
		return(NULL);
	}

	return(newbranch);
}

void clearstr() {
//	datastr[0] = '\000';
	datastrloc = 0;
	return;
}

void pushchr( char mychar) {
	datastr[datastrloc] = mychar;
	datastrloc++;
	return;
}

void printvar() {
	if ( ! datastrloc ) return;
	datastr[datastrloc] = '\000';
	if ( ( newbranch = AddTreeBranch(currentbranch) ) == NULL ) {
		printf("error adding tree in printvar\n");
		exit(1);
	}
	if ( labelleaf( newbranch, datastr ) ) {
		printf("error adding leaf label in printvar\n");
		exit(1);
	}
	if ( iprint == 0 ) printf(" var = %s\n",datastr);
	clearstr();
	return;
}

void pushbranch() {
	currentbranch = newbranch;
	parent = getparentof(currentbranch);
	if ( iprint ) return;
	printf("push\n");
	return;
}

void popbranch() {
	currentbranch = parent;
	parent = getparentof(currentbranch);
	if ( iprint ) return;
	printf("pop\n");
	return;
}

PVARTREE getparentof(PVARTREE currentbranch) {
	PVARTREE newparent;
	newparent = currentbranch->parent;
	return(newparent);
}


int dumpparsetree(PVARTREE vartree){
	PVARTREE mynode;
	PPVARTREE nodelist;
	int i;
	mynode = vartree;
	printParseNode(mynode,0);
	if ( mynode->numbranches == 0 ) return 0;
	nodelist = mynode->branch;
	for (i=0;i<mynode->numbranches;i++){
		dumpparsetree( *(nodelist+i) );
	}
	return 0;
}

int printParseNode(PVARTREE mynode, int verbose) {


	printf("=============================================\n node %s has %i branches\n",GetBranchName(mynode),mynode->numbranches);
	if ( strcmp ( GetBranchName(mynode), topnodelabel )  ) {
		printf(" parent = %s\n",GetBranchName( getparentof( mynode ) ) );
	}
	if ( mynode->numbranches == 0 ) {
		printf("=============================================\n");
	} else {
		printf(" branches follow\n");
	}
	printf("=============================================\n");
    if ( verbose ) PrintVarData( mynode->data );
	return 0;
}

int PrintVarData( PVARDATA thedata) {
	char * mylabel;
	char ** mydims;
	PVARDATA  mydata;
	const char * paramtylist[] = {"(we have no clue)", "not in the calling parameter list", "a formal parameter", "a simple constant"};
	mydata = thedata;
	mylabel = mydata->name;
	printf("=============================================\n vardata label = %s\n",mylabel);
	if ( mydata->dimsdefined ) {
		printf(" number of dimensions = %u (",mydata->vardim);
		mydims = mydata->dims;
		for (int i=0;i<mydata->vardim;i++){
			mylabel = *(mydims+i);
			printf("%s",mylabel);
			if ( i != mydata->vardim ) printf(",");
		}
		printf(")\n");
	}
	printf(" data type : %s",statementkeywords[mydata->vartype]);
	if ( mydata->elesize ) printf("*%u",mydata->elesize);
	printf("\n");
	printf(" the parameter type is %s\n",paramtylist[mydata->paramflag]);
	if ( mydata->numrefs ) {
		if ( mydata->typedefline ) printf(" varuable was defined on line %i\n",mydata->typedefline);
		if ( mydata->refs != NULL ) {
			printf(" references occur at these lines : ");
			for (int i = 0; i < mydata->numrefs; i++ ) {
				printf( "%i ",*((mydata->refs)+i));
			}
			printf("\n");
		}
	}
	printf(" number of references = %i\n",mydata->numrefs);
	printf("=============================================\n");

	return 0;
}

int DeleteTree(PVARTREE vartree , int verbose) {
	char * branchname;
	PVARTREE mytree;
	PPVARTREE nodelist;
	mytree = vartree;
	if ( (branchname = GetBranchName(mytree) ) == NULL ) {
		printf("problem with parse tree - branch label missing... detected in DeleteTree\n");
		return 1;
	}
	if ( ! strcmp( branchname , topnodelabel ) ) {
		if ( verbose ) printf("deleting top node\n");
	} else {
		if ( verbose ) printf("deleting %s\n", branchname);
	}
	if ( mytree->numbranches ) {
		nodelist = mytree->branch;
		for (int i=0;i<mytree->numbranches;i++) {
			if ( DeleteTree( *(nodelist+i) , verbose) ) return 2;
		}
		free(nodelist);
	}
	if ( DeleteVarData(mytree->data) ) return 3;
	free(mytree);

	return 0;
}

int DeleteVarData( PVARDATA thedata) {
	char * mylabel;
	char ** mydims;
	PVARDATA  mydata;
	char unknown[] = "(unknown)";

	mydata = thedata;
	mylabel = mydata->name;
	if ( mylabel == NULL ) mylabel = unknown;

	if ( mydata->dimsdefined ) {
		if ( (mydims = mydata->dims) == NULL ) {
			printf("error in vardata struct %s - dimsdefined = %u, but dims is NULL\n",mylabel,mydata->dimsdefined);
			return 1;
		}
		if ( mydata->vardim ) {
			for (int i=0;i<mydata->vardim;i++){
				mylabel = *(mydims+i);
				if ( mylabel != NULL ) free(mylabel);
			}
		}
		free(mydims);
	}
	if ( mydata->numrefs ) {
		if ( mydata->refs != NULL ) {
			free(mydata->refs);
		} else {
			printf("error in vardata struct %s - number of references was %i, but refs is NULL\n",mylabel,mydata->numrefs);
			return 2;
		}
	} else {
		if ( mydata->refs != NULL ) {
			printf("error in vardata struct %s - number of references was zero, but refs is not NULL\n",mylabel);
			return 3;
		}
	}
	if ( mylabel != unknown ) free(mylabel);
	free(mydata);

	return 0;
}

