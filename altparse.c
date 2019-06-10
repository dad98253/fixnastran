/*
 * altparse.c
 *
 *  Created on: Jun 10, 2019
 *      Author: dad
 */



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "fixnastran.h"

#define COMMON 3
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

extern int labelleaf( PVARTREE vartree, char * label);
extern PVARTREE AddTreeBranch( PVARTREE vartree);
/* arith0.c: simple parser -- no output
 * grammar:
 *   P ---> E '#'				/
 *   E ---> T {('+'|'-') T}		,
 *   T ---> S {('*'|'/') S}		V(
 *   S ---> F '^' S | F
 *   F ---> char | '(' E ')'
 */

extern char next;
void Comma(void);
void Fdim(void);
void Dimxp(void);
void Numxp(void);
int isFname(void);
void Dnamexp(void);
void Namexp(void);
void Express(void);
void Txp(void);
void Sxp(void);
void Fxp(void);

extern void error(int);
extern void scan(void);
extern void enter(char);
extern void leave(char);
extern void spaces(int);
extern void clearstr();
extern void pushchr( char mychar);
extern void printvar();
extern PVARTREE getparentof(PVARTREE currentbranch);
extern void pushbranch();
extern void popbranch();
extern PVARTREE GetBranch( PVARTREE vartree, int index);
extern int isprogtype( int cardtype );


extern int level;

extern char EOL;
extern int iprint;
extern char datastr[BUFFLEN];
extern int datastrloc;
extern PVARTREE currentbranch;
extern PVARTREE parent;
extern PVARTREE topbranch;
extern PVARTREE newbranch;
extern int slashlevel;
extern int parsethislen;
extern int requiredigit;

int cardisprogtype;;

int parsedeclare(int cardtype)
{

   cardisprogtype = isprogtype(cardtype);
   newbranch = parent = NULL;
   slashlevel = 0;
   requiredigit = 0;
   if ( ( currentbranch = topbranch = vartree = AddTreeBranch(parent) ) == NULL ) return 1;
   if ( labelleaf( currentbranch, topnodelabel ) ) return 2;
   if ( cardtype == COMMON ) return 3;
   if ( ( currentbranch = newbranch = AddTreeBranch(topbranch) ) == NULL ) return 3;
   if ( labelleaf( currentbranch, statementkeywords[cardtype] ) ) return 4;
   nextparsechar = parsethis;
   parsethislen = strlen(parsethis);
   clearstr();
   level = 0;
   scan();
   if ( next == '*' ) {
	   printvar();
	   scan();
	   Express();
	   printvar();
	   pushbranch();
	   if ( next == ',' ) scan();
   }
   if ( next == '(' ) {
//	   printvar();
//	   scan();
	   Express();
	   printvar();
	   pushbranch();
	   if ( next == ',' ) scan();
   }
   Comma();
   if (next != EOL) {
	   fprintf(stderr,"Unsuccessful parse, next = %c\n",next);
	   return 5;
   }
   return 0;
}

void Express(void)
{
   enter('E');
   Txp();
   while (next == '+' || next == '-') {
	  printvar();
      scan();
      Txp();
   }
   leave('E');
}

void Txp(void)
{
   enter('T');
   Sxp();
   while (next == '*' || next == '/') {
	  printvar();
	  pushchr(next);
	  printvar();
	  scan();
      Sxp();
   }
   leave('T');
}
void Sxp(void)
{
   enter('S');
   Fxp();
   if (next == '^') {
	  printvar();
	  pushchr(next);
	  printvar();
      scan();
      Sxp();
   }
   leave('S');
}
void Fxp(void)
{
   enter('F');
//   if (isalpha(next) || isdigit(next) || next == '_' ) {
//   while (isalpha(next) || isdigit(next) || next == '_' ) {
   if (isdigit(next)) {
	   while (isdigit(next)) {
		  pushchr(next);
		  scan();
	   }
   }
   else if ( next == '*' ) {
		  pushchr(next);
		  scan();
   }
   else if (next == '(') {
	  printvar();
	  pushbranch();
	   if ( strncmp ( nextparsechar, "LEN=", 4 ) == 0 ) {
		   for ( int i = 0; i<4; i++ ){
			   scan();
		   }
	   }
	   if ( strncmp ( nextparsechar, "KIND=", 5 ) == 0 ) {
		   for ( int i = 0; i<5; i++ ){
			   scan();
		   }
	   }
      scan();
      Express();
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

void Comma(void)
{
   enter('C');
   Fdim();
   while (next == ',') {
      printvar();
      scan();
      Fdim();
   }
   leave('C');
}

void Fdim(void)
{
   enter('P');
   Dimxp();
   while (next == '*') {
      printvar();
      scan();
      Numxp();
   }
   leave('P');
}

void Numxp(void)
{
   enter('N');
   while (isdigit(next)) {
	  pushchr(next);
	  scan();
   }
   printvar();
   leave('N');
}

void Dimxp(void)
{
   enter('D');
   if (isFname()) {
      printvar();
   }
   if (next == '(') {
	  printvar();
	  pushbranch();
      scan();
      Dnamexp();
      if (next == ')') {
    	  printvar();
    	  popbranch();
    	  scan();
      }
      else error(2);
   }
   leave('D');
}

int isFname(void)
{
   enter('I');
   if ( ! isalpha(next) ) return 0;
   pushchr(next);
   scan();
   while (isalpha(next)  || isdigit(next) || next == '_' ) {
	  pushchr(next);
	  scan();
   }
   leave('I');
   return 1;
}

void Dnamexp(void)
{
   enter('A');
   Namexp();
   while (next == ',') {
      printvar();
      scan();
      Namexp();
   }
   leave('A');
}

void Namexp(void)
{
   enter('X');
   //   if ( cardisprogtype && next == '*' ) {
   if ( next == '*' ) {
	   pushchr(next);
	   scan();
   } else {
	   while (isalpha(next)  || isdigit(next) || next == '_' ) {
		  pushchr(next);
		  scan();
	   }
   }
   printvar();
   leave('X');
}
