/*
 * fixnastran.c
 *
 *  Created on: May 24, 2019
 *      Author: dad
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "fixnastran.h"

void removecr(char *str);
int loadcard (char *str);
int unloadcards(int idelete);
int savecommon( char * substr, int substrlen);
int unloadcommons();
int DeleteAllSaves();
int parsecards();
int saveline (char * linein , int icontinue);
void unloadlines();
void sqzline ( char * linein );
int getcardtype( char * x, int j);
int FindOurCommonBlock();
int DoSanityCheck( int ourguy );
int FixOurCommon( int ourguy );
int DeleteOurInclude();
int AddOurInclude();
int AddOurSaves();
char * substrparse( char *lines, unsigned int c, int *substrlen);
int isblankcard(char * x);
int deletecards( int cardnumber, int numbertodelete );
int addcardbefore( char * ourline, int cardnumber );
int addonecard( char*z , int tempcardnum );
int writedeck();
extern int parsecommon(void);
extern int dumpparsetree(PVARTREE vartree);
extern int printParseNode(PVARTREE mynode, int verbose);
extern int NumChildBranch( PVARTREE thetree );
extern int DeleteTree(PVARTREE vartree, int verbose);
extern char * GetBranchName(PVARTREE mytree);
extern int DeleteVarData( PVARDATA mydata);
extern int PrintVarData( PVARDATA mydata);
extern PVARTREE GetBranch( PVARTREE vartree, int index);
int strnrchrjk(char * tmpstr, int istart , unsigned char c );
int setimplicits(int mylinenum);
int paresme( char * parsethis , int i , char * mytopnodelabel, int cardtype);
int loaddatatypes ();
int setvardata(int mylinenum, int cardtp );

VARDEF vardatadef = {NULL,NULL,NULL,0,0,4,0,0,0,0};		// default values for a newly found variable (befor being
                                                        // filled in by correct data) note that we are assuming that
														// NULL == 0 here (not always true on all systems, cf, msvc)

PVARDATA vardata;				// pointer to the variable data structure
int vardatasize = 0;			// current number of elements in the vardata array 0-. not defined yet

VARTREEDEF vartreedef = {NULL,NULL,0,0};

PVARTREE vartree;


FILE * inputlist;
FILE * fortinfile;
FILE * badboys;
char filename[BUFFLEN];


int cardnumber[MAXROUTINESIZE];
unsigned char continues[MAXROUTINESIZE];
unsigned char cardtype[MAXROUTINESIZE];
char *cards[MAXROUTINESIZE];
char *lines[MAXROUTINESIZE];
int numcards;
int numlines;
int firstexe = 0;
int hasinclude = 0;
char ourcb[] = "ZZZZZZ";
char ourstring[] = "ICORESZ";
char ourinclude[] = "      INCLUDE '../params.inc'";
//char prefix[] ="/mnt/DataDisk2/dad/repositories/linux/nasworking/NASTRAN-95/";
char prefix[] ="/mnt/DataDisk2/dad/repositories/linux/nasworking/NASTRAN-95/";
char outprefix[] ="/mnt/DataDisk2/dad/repositories/linux/nasworking/";
char filestr[BUFFLEN];
unsigned char printflag = '\000';
char inputfile[] = "filelist.txt";
char Badboys[] = "badboys.txt";
int savebadboys = 1;
int firstnondeclare;
char *commonblocknames[BUFFLEN];
int numcommons;
char oursave[] = "      SAVE ";
int nasty;
char * parsethis;
char * nextparsechar;
char testvect[] = "COMMON/SEM/MASK,MASK2,MASK3,LINKNM(15)/SYSTEM/SYSBUF,XX(20),LINKNO,XXX(16,1492,Isiz),NBPC,NBPW,NCPW,XXXX(1)";
char mytopnodelabel[] = "9t9o9p9";
char * topnodelabel;
char teststr[] = "my test string";

//  to do : need to generat a dimensioned variable list - find all definitions to the length of the common block variable
// THEY MUST BE SET CONSISTENTLY!!



int numkeywords = 33;
char *statementkeywords[] = { "OTHER", "COMMENT", "PARAMETER", "COMMON", "IMPLICIT", "INCLUDE",
		      "EQUIVALENCE", "SUBROUTINE", "FUNCTION", "USE", "EXTERNAL",
			  "INTRINSIC", "SAVE", "BLOCKDATA", "PROGRAM", "ENTRY",
			  "FORMAT", "DOUBLEPRECISIONFUNCTION", "LOGICALFUNCTION", "REALFUNCTION", "INTEGERFUNCTION",
			  "CHARACTERFUNCTION", "COMPLEXFUNCTION", "END", "DOUBLEPRECISION", "LOGICAL",
			  "REAL", "INTEGER", "CHARACTER", "COMPLEX", "DIMENSION",
			  "POINTER", "DATA"
               };
#define BAD 99
#define OTHER 0
#define COMMENT 1
#define PARAMETER 2
#define COMMON 3
#define IMPLICIT 4
#define INCLUDE 5
#define EQUIVALENCE 6
#define SUBROUTINE 7
#define FUNCTION 8
#define USE 9
#define EXTERNAL 10
#define INTRINSIC 11
#define SAVE 12
#define BLOCKDATA 13
#define PROGRAM 14
#define ENTRY 15
#define FORMAT 16
#define DOUBLEPRECISIONFUNCTION 17
#define LOGICALFUNCTION 18
#define REALFUNCTION 19
#define INTEGERFUNCTION 20
#define CHARACTERFUNCTION 21
#define COMPLEXFUNCTION 22
#define END 23
#define DOUBLEPRECISION 24
#define LOGICAL 25
#define REAL 26
#define INTEGER 27
#define CHARACTER 28
#define COMPLEX 29
#define DIMENSION 30
#define POINTER 31
#define DATA 32



unsigned const char defimplicits[] = {[0 ... 7] = REAL, [8 ... 13] = INTEGER, [14 ... 25] = REAL};
unsigned char myimplicits[26];


int main()
{
    int i, j, n=100000, ourguy; // iret;
    char linein[BUFFLEN];
    char *szret;
/*
    topnodelabel = mytopnodelabel;
    nextparsechar = parsethis = testvect;
    while ( *(nextparsechar) != '\000' ) {
    	printf(" at character # %u\n", (unsigned int)(nextparsechar-testvect));
    	if ( (i = parsecommon()) ) {
    		printf("parsecommon failed, returned %i\n",i);
    	}
    	dumpparsetree(vartree);
    	DeleteTree(vartree,0);
    	parsethis = nextparsechar;
    } */
// COMMON/SEM/MASK,MASK2,MASK3,LINKNM(15)/SYSTEM/SYSBUF,XX(20),LINKNO,XXX(16,1492,Isiz),NBPC,NBPW,NCPW,XXXX(1)/
// 012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567
//           1         2         3         4         5         6         7         8         9         0


    inputlist = fopen(inputfile, "r");
    if (inputlist == NULL)
    {
        printf("Could not open input file\n");
        return 1;
    }

    badboys = fopen(Badboys, "w");
    if (badboys == NULL)
    {
        printf("Could not open badboys file\n");
        return 2;
    }

    for (i=0; i<n; i++)
    {
//        iret = fscanf(inputlist,"%[^\n]%*c", filestr);
    	szret = fgets(filestr, BUFFLEN, inputlist);
//        printf(" iret = %i\n",iret);
        if ( szret == NULL ) {
        	printf(" EOF\n");
        	return 3;
        }
// output from grep:
        szret = strchr ( filestr, ':' );
// output from ls:
        szret = strchr ( filestr, '\n' );
        if ( szret == NULL ) {
        	printf(" %i [%li] : not found %s\n",i+1,strlen(filestr),filestr);
        }else{
        	if (filestr[0] == '#' ){
        	   	printf(" %i [%li] : commented out... skipping %s\n",i+1,strlen(filestr),filestr);
        	   	continue;
        	}
        	printf(" =====================================================================================\n");
        	*szret = '\000';
 //       	printf(" %i [%li] file name found : %s\n",i+1,strlen(filestr),filestr);
        	filename[0] = '\000';
        	strcat(filename,prefix);
        	strcat(filename,filestr);
        	printf(" opening %s\n",filename);
        	fortinfile = fopen(filename, "r");
            if (fortinfile == NULL)
            {
				printf(" Error : Could not open fortinfile file %s , errno = %i\n",filename,errno);
				printf(" %s\n",strerror(errno));
                return 4;
            }
//            printf(" file %s opened\n",filename);
            numlines = 0;
            numcards = 0;
            hasinclude = 0;
            firstexe = 0;
            firstnondeclare = 0;
            nasty = 0;
            memcpy(myimplicits,defimplicits,sizeof(defimplicits[0])*26);
            if ( (vardata = malloc(sizeof(vardatadef)*1000) ) == NULL ){
            	printf("unable to malloc vardata struct in main\n");
            	return 5;
            }
            for (j=0;j<1000;j++) {
            	vardata[j] = vardatadef;
            }
            vardatasize = 1000;
            for (j=0; j<MAXROUTINESIZE; j++)
            {
            	szret = fgets(linein, BUFFLEN, fortinfile);
            	if ( szret == NULL ) {
//            		printf(" EOF\n");
            		break;
            	}
            	removecr(linein);
//            	printf("         ->%s\n",linein);
           		if ( loadcard(linein) ) return 6;
            }
        }
        printf(" %i cards loaded\n",numcards);
//    	unloadcards(0);										//
        if ( parsecards() ) return 7;
        if ( loaddatatypes () ) return 11;
        ourguy = 0;
        if ( ( ourguy = FindOurCommonBlock() ) ) {
        	int sancheck = DoSanityCheck( ourguy );
        	if ( sancheck != 0 && sancheck != 7 ) return 8;
        	if ( FixOurCommon ( ourguy ) ) return 9;
//        	unloadcards(0);									//
        	if ( hasinclude ) {
        		if ( DeleteOurInclude() ) return(10);
        	}
        	AddOurInclude();
        }
        DeleteAllSaves();
    	AddOurSaves();
        writedeck();
        if ( unloadcards(1) ) return(12);
        unloadlines();
        unloadcommons();
        if (ourguy) printf(" our guy found at line %i (card %i)\n",ourguy,cardnumber[ourguy]);
        printf(" the first non-declarative statement was card number %i\n",firstnondeclare);
        free(vardata);
        fclose(fortinfile);
        fflush(stdout);
    }
    fclose(inputlist);
    return 0;
}

void removecr(char *strtmp) {
  if (strtmp == NULL) return;
  int length = strlen(strtmp);
  if (strtmp[length-1] == '\n') {
    strtmp[length-1]  = '\000';
  }
  length = strlen(strtmp);
  if (strtmp[length-1] == '\012') {
     strtmp[length-1]  = '\000';
   }
  length = strlen(strtmp);
  if (strtmp[length-1] == '\015') {
     strtmp[length-1]  = '\000';
   }
  length = strlen(strtmp);
  int i;
  int last = 0;
  for (i=0;i<length;i++) {
	  if ( strtmp[i] != ' ' ) last =i;
  }
  strtmp[last+1] = '\000';
}

int loadcard(char *linein ) {
	char *x;
	x = (char*)malloc(strlen(linein)+1);
	if ( x == NULL ) {
		printf(" malloc failed at card %i\n",numcards);
		return (1);
	}
	strcpy(x,linein);
	cards[numcards] = x;
	numcards++;
	return (0);
}

int unloadcards(int idelete) {
	int j = 0;
	char *x;
	for (int i=0; i<numcards;i++) {
		x = cards[i];
		if ( x == NULL ) {
			printf(" null pointer found in unloadcards at %i\n",i);
			return(1);
		} else {
			if ( printflag ) {
				printf("unloading card %6.6i",i);
				if ( i == firstexe ) { printf("*"); } else { printf(" "); }
				printf(": %s\n",x);
			}
			if ( idelete ) free(x);
			j++;
		}
	}
	if ( idelete ) {
		printf(" %i cards freed\n",j);
		numcards = 0;
	} else {
		printf(" %i cards printed\n",j);
	}
	return(0);
}

int parsecards() {
	int i;
	char * x;
	for (i=0;i<numcards;i++) {
		x = cards[i];
		if ( (x[0] == 'c') || (x[0] == 'C') || (x[0] == '#') || (x[0] == '*') || (x[0] == '!') ||
				                                    (isblankcard(x)) ) {  // comment or pre-compiler directive card or blank card
			continue;
		}
		if ( x[0] == '\t' ) {  // tab in first collumn
			int lxx = strlen(x);
			x = (char*)realloc(cards[i],lxx+9);
			if ( x == NULL ) {
				printf(" realloc failed at card %i when expanding tab\n",i);
				return (1);
			}
			cards[i]=x;
			for (int j=lxx;j>0;j--) {
				x[j+6] = x[j];
			}
			for (int k=0;k<8;k++) {
				x[k] = ' ' ;
			}
		}
		if ( (strncmp( (const char *)x, "     ", 5 ) == 0) && (x[5] != ' ') ) {  // continuation card
			if ( saveline(x,1) ) return (1);
			continue;
		}
		if ( numlines ) sqzline(lines[numlines-1]);
		switch ( getcardtype(x,i) ) {
			case BAD:
				return (1);
				break;
			case IMPLICIT:
			case PARAMETER:
			case SUBROUTINE:
			case FUNCTION:
			case REALFUNCTION:;
			case INTEGERFUNCTION:
			case DOUBLEPRECISIONFUNCTION:
			case LOGICALFUNCTION:
			case CHARACTERFUNCTION:
			case COMPLEXFUNCTION:
			case BLOCKDATA:
			case PROGRAM:
			case FORMAT:
				break;
			case DATA:
				if ( firstexe == 0 ) firstexe = i;
				break;
			case EXTERNAL:			// hopefully we don't need to worry about these - the more typical use can occure anywhere - this
				                    // will find externals outside the program unit (ie.e., first statement in file
				fprintf(stderr," Warning, manual inspection required: an \"EXTERNAL\" statement was found ... see card %i in %s\n",
						i,filename);
				break;
			case INTRINSIC:
				break;
			case SAVE:				// should delete all save cards and create our own...
				break;
			case USE:
				fprintf(stderr," Warning, manual inspection required: an \"USE\" statement was found ... see card %i in %s\n",i,filename);
				break;
			case END:
				break;
			case INCLUDE:
				if ( firstexe == 0 ) firstexe = i;
				if ( (strncmp( (const char *)(x+15), "../params.inc", 13 ) == 0)) {
					hasinclude = numlines;
				}
				break;
			case POINTER:
				fprintf(stderr," Holy cow, Batman! There be Cray Pointers here! ... see card %i in %s\n",i,filename);
				return(POINTER);
			case OTHER:
				if ( firstnondeclare == 0 ) firstnondeclare = i;
			default:
				if ( firstexe == 0 ) firstexe = i;
		}
	}
	sqzline(lines[numlines-1]);
	return (0);
}

int getcardtype( char * x , int j) {
	int i;
	char ctem[5000];

	strcpy(ctem,x);
	sqzline(ctem);
	for (i=1;i<numkeywords;i++) {
		if ( (strncmp( (const char *)(ctem+6), statementkeywords[i], strlen(statementkeywords[i]) ) == 0)) {
			if ( i == USE ){
//				printf(" extra USE test on %s\n",ctem);
				if ( strchr( ctem, '=' ) != NULL ) continue;
			}
			cardtype[numlines] = i;
			continues[numlines] = 0;
			cardnumber[numlines] = j;
			if ( saveline(x,0) ) return (BAD);
			return (i);
		}
	}
	cardtype[numlines] = OTHER;
	continues[numlines] = 0;
	cardnumber[numlines] = j;
	if ( saveline(x,0) ) return (BAD);
	return (OTHER);
}

int saveline(char * linein , int icontinue) {
	char *x,*y;
	size_t isize, addsize;
	if ( icontinue ) {
		if ( numlines == 0 ) {
			printf("bad saveline call : %s\n",linein);
			return 1;
		}
		continues[numlines-1]++;
		y = lines[numlines-1];
		isize = strlen( y );
		addsize = strlen( linein+6 );
		x = (char*)realloc(y,isize+addsize+1);
		if ( x == NULL ) {
			printf(" realloc failed at line %i\n",numlines);
			return (1);
		}
		strcat(x,(char*)(linein+6));
		lines[numlines-1] = x;
	} else {
		x = (char*)malloc(strlen(linein)+1);
		if ( x == NULL ) {
			printf(" malloc failed at line %i\n",numlines);
			return (1);
		}
		strcpy(x,linein);
		lines[numlines] = x;
		numlines++;
	}
	return (0);
}

void unloadlines() {
	int j = 0;
	char *x;
	for (int i=0; i<numlines;i++) {
		x = lines[i];
		if ( x == NULL ) {
			printf(" null pointer found in unloadlines at %i\n",i);
		} else {
			if ( printflag ) {
				printf("unloading line %6.6i : %-100.100s... type %s",i,x,statementkeywords[cardtype[i]]);
				if ( continues[i] != 0 ) printf ("+%i", continues[i]);
				if ( cardnumber[i] < firstexe ) printf( "x");
				if ( strlen(statementkeywords[cardtype[i]]) < 8 ) printf("\t");
				printf("\t-> card %i",cardnumber[i]);
				printf("\n");
			}
			free(x);
			j++;
		}
	}
	printf(" %i of %i lines freed, first exe statement was at card %i\n",j,numlines,firstexe);
	if ( hasinclude ) printf (" our INCLUDE is located at line %i (card %i)\n",hasinclude-1,cardnumber[hasinclude-1]);
	numlines = 0;
	firstexe = 0;
	hasinclude = 0;
	return;
}

int savecommon( char * substr, int substrlen) {
	char *x;
	x = (char*)malloc(substrlen+1);
	if ( x == NULL ) {
		printf(" malloc failed to load common %s in %s\n",substr,filename);
		return (1);
	}
	strncpy(x,substr,substrlen);
	x[substrlen] = '\000';
	commonblocknames[numcommons] = x;
	numcommons++;
	return (0);
}

int unloadcommons() {
	int j = 0;
	char *x;

	if ( numcommons == 0 ) return 0;

	for (int i=0; i<numcommons;i++) {
		x = commonblocknames[i];
		if ( x == NULL ) {
			printf(" null pointer found in unloadcommons at %i\n",i);
		} else {
			if ( printflag ) {
				printf("unloading common  %6.6i : %-20.20s\n",i,x);
			}
			free(x);
			j++;
		}
	}
	printf(" %i of %i common block names freed\n",j,numcommons);
	numcommons = 0;
	return 0;
}

void sqzline ( char * linein ) {
/*
 * set everything after column 6 to upper case and squeeze out all blanks
 * this will include things inside paretheses and single and double quotes - thus, these
 * might get messed up.
 * However, since we don't plan to output auything that our squeeze might mess up, that should be ok.
 * e.g., EQUIVALENCE and COMMON are ok to squeeze inside parens
 * we do not care about catching hollerith constants.. the lines we need to change shouldn;t
 * have any
 * we will also make assumtions regarding the use of certain f99 contructs.
 * one of the goals of the overall program is to identify where to insert our INCLUDE statement
 * (if neccessary). Thus, we want to try to identify the start of executable statements. We
 * will assume that any INCLUDE other than our own might include executable statements - thus
 * saving the need to open them up and check... i.e., we will just put our include before any others.
 * we will assume all pre-compiler directives (e.i., #include) do not include executable statements
 * in any case, even if we get it wrong, the compiler should catch our mistakes!!
 *
*/

	char *s,*d;

	if ( strlen(linein) < 7 ) return;
	s = linein+6;
	d = linein+6;
	do while(isspace(*s)) s++; while( (*d++ = *s++) );
	s = linein+6;
	while(*s) {*s = toupper((unsigned char) *s); s++;}

	return;
}

int FindOurCommonBlock() {

	int i;
	int ourcblen;
	int linelen;
	char * substr;
	int substrlen;
	int found = 0;
	ourcblen = strlen(ourcb);
	for (i=0;i<numlines;i++) {
		if ( cardtype[i] != COMMON ) continue;
		linelen = strlen(lines[i]);
		substrlen = 0;
		substr = lines[i];
		int munonline = 0;
		while ( ( substr = substrparse(substr+substrlen+2, '/', &substrlen) ) ) {	// we gotta check for nasty guys who put more
			                                                                        // than one named common in a statemnt
			savecommon(substr,substrlen);
			munonline++;
			if ( substrlen == ourcblen ) {
				if ( ( strncmp (ourcb, (const char *)substr, (unsigned int)substrlen ) ) == 0 ) found = i;
			}
			if ( !( (substr+substrlen+2) < lines[i]+linelen ) ) break;
		}
		if ( (munonline > 1) && found ) nasty++;
	}

	return (found);
}

char * substrparse( char *lines, unsigned int c, int *substrlen) {
	char *start, *end;
	if ( ( start = strchr( lines, c ) ) == NULL ) return (NULL);
	if ( strlen(start) < 2 ) return(NULL);
	if ( ( end = strchr( (start+1), c ) ) == NULL ) return (NULL);
	*substrlen = (int)(end-start-1);
	return (start+1);
}

int DoSanityCheck( int ourguy ) {
/*
 * do some basic sanity checks...
 * 1) are there any staements besides the common block?
 * 2) is there a first executable statement found?
 * 3) is the first statement a recognizable program, subroutine, or function statement?
 * 4) were any USE statements found? (we can't handle f99 stuff)
 * 5) is there anything declared in the common block at all?
 * 6) is there an END statement?
 * 7) is the variable in the common block not dimensioned? (can't go hunting in equivalence statements, yet)
 * 8) are there multiple named commons defined on a sigle statement?
 */

	char *ourline;
	char *start, *end;

	if ( numlines < 2 ) {
		printf(" error : common block found in empty deck. file=%s\n",filename);
		return(1);	// case 1)
	}
	if ( firstexe == 0 ) {
		printf(" error : common block found in deck with no executable statements. file=%s\n",filename);
		return(2);		// case 2)
	}
	if ( (cardtype[0] != SUBROUTINE) && (cardtype[0] != FUNCTION) && (cardtype[0] != BLOCKDATA) &&
		     (cardtype[0] != PROGRAM) && (cardtype[0] != DOUBLEPRECISIONFUNCTION) && (cardtype[0] != LOGICALFUNCTION) &&
		     (cardtype[0] != REALFUNCTION) && (cardtype[0] != INTEGERFUNCTION) && (cardtype[0] != CHARACTERFUNCTION) &&
			 (cardtype[0] != COMPLEXFUNCTION) ) {
		printf(" error : common block found in deck with no routine declaration (program/subroutine/function/etc..) statement. file=%s\n",
				filename);
		return(3);	// case 3)
	}
	for (int i=0;i<numlines;i++) if ( cardtype[i] == USE ) {
		printf(" error : common block found in deck with a USE statement (possible f77+ deck). file=%s\n",filename);
		return(4);	// case 4)
	}
	ourline = lines[ourguy];
	start = strchr( ourline, '/' );
	end = strchr( (start+1), '/' );
	if ( strlen(end) < 2 ) {
		printf(" error : common block found but it\'s empty. file=%s\n",filename);
		return(5);	// case 5)
	}
//	printf("ourguy = \"%s\"\n",(end+1));
	if ( (cardtype[numlines-1] != END) ) {
		printf(" error : common block found in deck with no END statement. file=%s\n",filename);
		return(6);	// case 6)
	}
	if ( strchr( (end+1), '(' ) == NULL ) {
		printf(" error : common block found but no dimensioned variables on line. file=%s\n",filename);
		if ( savebadboys ) {
			fprintf(badboys," undimensioned common in %s\n",filename);
		} else {
			return(7);	// case 7)
		}
	}
	if ( nasty ) {
		if ( savebadboys ) {
			fprintf(badboys," multiple named commons defined on a sigle statement in %s\n",filename);
		} else {
			return(8);	// case 8)
		}
	}

	return(0);  // everythin ok
}

int FixOurCommon( int ourguy ) {

	char *ourline,*ournewline;
	char *start, *end, *newstart;
	ourline = lines[ourguy];
	start = ourline;
	if ( nasty ) {
		PVARTREE commonnode;
		int NumCardsAdded = 0;
		if ( ( start = strstr( start, ourcb ) ) == NULL ) {
			printf(" mixup in FixOurCommon... ourguy not found in %s in file %s\n",ourline,filename);
			return(5);
		}

		if ( ( parsethis = (char*)malloc(strlen(ourline)+200) ) == NULL ) {
			printf(" error in malloc of parseme for line : %s in file %s\n",ourline,filename);
			return(10);
		}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		strcpy(parsethis,ourline);

		topnodelabel = mytopnodelabel;

		if ( paresme( parsethis , ourguy , topnodelabel, COMMON) ) return (11);
/*
		int iret;
		if ( (iret = parsecommon()) ) {
			printf("parsecommon failed, returned %i\n",iret);
			if ( savebadboys ) {
				fprintf(badboys," nasty COMMON block parse failed in %s\n",filename);
				DeleteTree(vartree, 1);
				free(parsethis);
				return(0);
			} else {
				return(9);
			}
		}
		//dumpparsetree(vartree);
		// do a bunch of checks... just in case we have some undetected bugs...
		if ( strcmp ( GetBranchName(vartree), mytopnodelabel) != 0 ) {
			printf("error 1 in nasty COMMON block parse in file %s\n",filename);
			if ( savebadboys ) {
				fprintf(badboys," nasty COMMON block parse failed in %s\n",filename);
				DeleteTree(vartree, 1);
				free(parsethis);
				return(0);
			} else {
				return(10);
			}
		}
		if ( ( commonnode = GetBranch( vartree, 0 ) ) == NULL ) {
			printf("error 2 in nasty COMMON block parse in file %s\n",filename);
			if ( savebadboys ) {
				fprintf(badboys," nasty COMMON block parse failed in %s\n",filename);
				DeleteTree(vartree, 1);
				free(parsethis);
				return(0);
			} else {
				return(11);
			}
		}
		if ( strcmp ( GetBranchName(commonnode), statementkeywords[COMMON] ) != 0 ) {
			printf("error 3 in nasty COMMON block parse in file %s\n",filename);
			if ( savebadboys ) {
				fprintf(badboys," nasty COMMON block parse failed in %s\n",filename);
				DeleteTree(vartree, 1);
				free(parsethis);
				return(0);
			} else {
				return(12);
			}
		}
		int numcommons = commonnode->numbranches;
		if ( numcommons < 1 ) {
			printf("error 4 in nasty COMMON block parse in file %s\n",filename);
			if ( savebadboys ) {
				fprintf(badboys," nasty COMMON block parse failed in %s\n",filename);
				DeleteTree(vartree, 1);
				free(parsethis);
				return(0);
			} else {
				return(13);
			}
		}
*/
		commonnode = GetBranch( vartree, 0 ); 	// (exsistance check was made in parseme)
		int numcommons = commonnode->numbranches;
		// ok - so here goes... lets delete the entire old common card and all of its continuation cards
		// delete the card(s) containg the old common block declaration
		int ourguycardnumber;		// save the card number where he used to start
		ourguycardnumber = cardnumber[ourguy];
		if ( deletecards(cardnumber[ourguy],(int)(continues[ourguy])+1) ) return(14);
		// reuse the parsethis array to assemble our new cards
		PVARTREE CommonBlock;
		PVARTREE Variable;
		int thisishim = 0;
		for ( int i=0;i<numcommons;i++) {	// for each COMMON block found
			CommonBlock = GetBranch( commonnode, i );
			if ( strcmp ( GetBranchName(CommonBlock) , ourcb ) == 0 ) thisishim = 1;	// see if this is the guy we need to change
			strcpy ( parsethis , "      COMMON /" );
			strcat( parsethis, GetBranchName(CommonBlock) );
			strcat( parsethis, "/ " );
//			int istart;
//			istart = strlen(parsethis);
			if ( CommonBlock->numbranches < 1 ) {
				printf("error 5 in nasty COMMON block parse in file %s\n",filename);
				if ( savebadboys ) {
					fprintf(badboys," nasty COMMON block parse failed in %s\n",filename);
					DeleteTree(vartree, 0);
					free(parsethis);
					return(0);
				} else {
					return(15);
				}
			}
			if ( thisishim && ( ( CommonBlock->numbranches > 1 ) || ( NumChildBranch( GetBranch( CommonBlock , 0 ) ) > 1 ) ) ) {
				fprintf(stderr," WARNING, WARNING, Will Robinson!! more nasty stuff on the common line: %s in file: %s\n",
						ourline,filename);
				if ( savebadboys ) {
					fprintf(badboys," more nasty stuff on the common line: %s\n",filename);
					DeleteTree(vartree, 0);
					free(parsethis);
					return(0);
				} else {
					return(16);
				}
			}  // so now we know there is only one singlely (or non) dimensioned variable on our common block card
			if ( thisishim ){
				Variable = GetBranch( CommonBlock , 0 );
				strcat ( parsethis , GetBranchName( GetBranch( CommonBlock , 0 ) ) );
				strcat ( parsethis , "(" );
				strcat ( parsethis , ourstring );
				strcat ( parsethis , ")" );
				// add the card back in, pretty printing as we go...
				if ( addcardbefore( parsethis, ourguycardnumber+NumCardsAdded ) ) return(6);
				NumCardsAdded++;
				thisishim = 0;
			} else {
				int numvars = CommonBlock->numbranches;
				for ( int j=0;j<numvars;j++) {	// for each variable found
					Variable = GetBranch( CommonBlock, j );
					strcat ( parsethis , GetBranchName( Variable ) );
					if ( NumChildBranch(Variable) > 0  ) {
						strcat ( parsethis ,"(" );
						for ( int k=0;k<NumChildBranch(Variable) ; k++ ) {
							strcat ( parsethis , GetBranchName( GetBranch(Variable,k) ) );
							if ( k+1 != NumChildBranch(Variable) ) strcat ( parsethis , "," );
						}
						strcat ( parsethis ,")" );
					}
					if ( j+1 != numvars ) strcat ( parsethis , "," );
				}  // whole line is complete... now see if its too long..
				if ( strlen(parsethis) < 72 ) {
					printf(" adding \"%s\" to location %i\n",parsethis,cardnumber[ourguy]+NumCardsAdded);
					if ( addcardbefore( parsethis, ourguycardnumber+NumCardsAdded ) ) return(6);
					NumCardsAdded++;
				} else {  // its too long, find a good place to break it up
					int mylast;
					char *tmpstr;
					char onecard[80];
					tmpstr = parsethis;
					int crdstart = 0;
					onecard[0] = '\000';
					while ( strlen(tmpstr) > 66) {
						mylast = strnrchrjk(tmpstr, 72-crdstart , ',');
						strncat(onecard, tmpstr, mylast);
						printf(" adding \"%s\" to location %i\n",onecard,cardnumber[ourguy]+NumCardsAdded);
						if ( addcardbefore( onecard, ourguycardnumber+NumCardsAdded ) ) return(6);
						NumCardsAdded++;
						tmpstr+=mylast;
						crdstart = 6;
						strcpy ( onecard, "     +");
					}
					if ( strlen(tmpstr) > 0 ) {
						strncat(onecard, tmpstr, mylast);
						printf(" adding \"%s\" to location %i\n",onecard,cardnumber[ourguy]+NumCardsAdded);
						if ( addcardbefore( onecard, ourguycardnumber+NumCardsAdded ) ) return(6);
						NumCardsAdded++;
					}
/*					mylast = strnrchrjk(tmpstr, 72 , ',');
					int needed;
					needed = ( strlen(parsethis) - 6) / 66;
					if ( needed*66+6 < strlen(parsethis) ) needed++;
					char onecard[73];
					char * breakpt;
					int iend;
					while ( strlen(parsethis-istart) > 66 ) {  // istart was set to the length of the
					                                           // string "      COMMON/XXX/", we will try to keep it on one line
						iend = strlen(parsethis);
						breakpt = strchr( parsethis+iend-65, ',' );
						strcpy ( onecard, "     +");
						strcat( onecard, breakpt+1);
						if ( addcardbefore( onecard, cardnumber[ourguy]+NumCardsAdded ) ) return(6);
						NumCardsAdded++;
						*(breakpt+1) = '\000';
					} // write out the last card
					if ( addcardbefore( parsethis, cardnumber[ourguy]+NumCardsAdded ) ) return(6);
					NumCardsAdded++;	*/
				}
			}
		}
		DeleteTree(vartree, 0);
		free(parsethis);
	} else {
		if ( (start = strchr( start, '(' ) ) == NULL ) {
			if ( savebadboys ) return(0);
			printf(" no open paren found on %s in file %s\n",ourline,filename);
			return(1);
		}
		if ( (end = strchr( (start+1), ')' ) ) == NULL ) {
			printf(" no close paren found on %s in file %s\n",ourline,filename);
			return(2);
		}
		if ( end < (start+1) ) {
			printf(" nothing found within parens on %s in file %s\n",ourline,filename);
			return(3);
		}
		ournewline = (char*)malloc(strlen(ourline)+strlen(ourstring)+1);
		strcpy ( ournewline, ourline );
		if ( ( newstart = strstr( ournewline, ourcb ) ) == NULL ) {		//	get ready to check our guy for extras
			printf(" impossible error #1 in FixOurCommon : ourline = %s in file %s\n",ournewline,filename);
			return(7);
		}
		newstart = strchr( newstart, '(' );
		strcpy ( (newstart+1), ourstring );
		strcat ( ournewline, end );
		if ( ( start = strstr( ournewline, ourcb ) ) == NULL ) {		//	get ready to check our guy for extras
			printf(" impossible error #2 in FixOurCommon : ourline = %s in file %s\n",ournewline,filename);
			return(6);
		}
		start = start + strlen(ourcb) + 1;
		if ( (strchr( start, ',' ) ) != NULL ) {
			fprintf(stderr," WARNING, WARNING, Will Robinson!! more stuff on the common line: %s in file: %s\n",ourline,filename);
			free(ournewline);
			if ( savebadboys ) {
				fprintf(badboys," more stuff on the common line: %s\n",filename);
				return(0);
			} else {
				return(4);
			}
		}
		free(ourline);
		lines[ourguy] = ournewline;
		ourline = ournewline;
		// now fix the card(s)
		// delete the card(s) containg the old common block declaration
		if ( deletecards(cardnumber[ourguy],(int)(continues[ourguy])+1) ) return(5);
		// add the card back in, pretty printing as we go...
		if ( addcardbefore( ourline, cardnumber[ourguy] ) ) return(6);
	}

	return(0);  // everything ok
}

int deletecards( int cardtodelete, int numbertodelete ) {

	char * x;
	int i;
	if ( numcards == 0 ) {
		printf( "deletecards called with numcards = 0");
		return(1);
	}
	if ( cardtodelete == numcards ) {
		printf(" warning attempting to delete EOF, card number = %i, file %s\n",cardtodelete,filename);
		return(0);
	}
	if ( cardtodelete < 0 || cardtodelete > (numcards - 1) ) {
		printf(" error attempting to delete non-exsistant card (number %i), file %s\n",cardtodelete,filename);
		return(2);
	}
	for ( i = cardtodelete; i<(cardtodelete+numbertodelete); i++) {
		x = cards[i];
		if ( x == NULL ) {
			printf(" error : attempt to delete non-existant card number %i in %s\n",cardtodelete,filename);
			return (3);
		}
		free(x);
	}
	for ( i = cardtodelete; i<(numcards-1); i++) cards[i] = cards[i+numbertodelete];
	numcards-=numbertodelete;
// repair indecies in lines
	for ( i = 0; i<numlines; i++) {
		if ( cardnumber[i] > (cardtodelete-1) && cardnumber[i] < (cardtodelete+numbertodelete) ) cardnumber[i] = -1;
		if ( cardnumber[i] > cardtodelete ) cardnumber[i]-=numbertodelete;
	}
	if ( firstexe > cardtodelete ) firstexe-=numbertodelete;
	return(0);
}

int addcardbefore( char * ourline, int cardtoadd ) {

//	char *x, *y, *z;
	char *y;
//	int in = 0;
//	int iout = 1;
//	int numcom = 0;
	int cardout = 1;
//	int cardout = 0;
	int tempcardnum = cardtoadd;
	y = ourline;
/*	x = (char*)malloc(strlen(ourline)*2+2);
	if ( x == NULL ) {
		printf(" malloc failed to add card at %i in file %s\n",cardtoadd,filename);
		return (1);
	}
	int mylen = strlen(ourline);
	z = x;
	while ( *y ) {
		if ( *y == '/' ) {
			if ( in ) {
				*z = *y;
				z++;
				*z = ' ';
				numcom++;
				if ( numcom > 1 ) {
					printf("warning! more than one common statement on a line at %i in %s\n",cardtoadd,filename);
				}
				in = 0;
			} else {
				*z = ' ';
				z++;
				*z = *y;
				in = 1;
			}
			iout+=2;
		} else {
			*z = *y;
			iout++;
		}
		z++;
		y++;
		if ( iout > 73 ) {
			*z = '\000';
			// out card -> tempcardnum
			if ( addonecard( z , tempcardnum ) ) return(2);
			cardout++;
			z = x;
			tempcardnum++;
			strcpy(z,"      ");
			z+=6;
			iout = 7;
		}
	}

// output final card
	*z = '\000';
	// out card -> tempcardnum
	if ( addonecard( x , tempcardnum ) ) return(2);
	cardout++;
	free(x);													*/
// adding the following line in place of the above stuff...
	if ( addonecard( y , tempcardnum ) ) return(2);
// repair indecies in lines
	for ( int i = 0; i<numlines; i++) {
		if ( cardnumber[i] > (cardtoadd-1) ) cardnumber[i]+=cardout;
	}
/*
	for ( int i = 0; i<numlines; i++) {
		if ( cardnumber[i] == cardtoadd+1 ) {
			cardnumber[i] = cardtoadd;
			continues[i] = cardout-1;
			break;
		}
	}
*/
	if ( firstexe > cardtoadd ) firstexe+=cardout; // should bever be true!

	return(0);
}

int addonecard( char*z , int tempcardnum ){
// onserts card at card number tempcardnum
	char *x;
	x = (char*)malloc(strlen(z)+1);
	if ( x == NULL ) {
		printf(" malloc failed to add one card at %i in file %s\n",tempcardnum,filename);
		return (1);
	}
	strcpy(x,z);
	if ( numcards > 0 ) {
		for ( int i=numcards; i>(tempcardnum-1); i--) {
			cards[i] = cards[i-1];
		}
	}
	cards[tempcardnum] = x;
	numcards++;

	return(0);
}

int DeleteOurInclude() {
	int cardtodelete;
	int numbertodelete = 1;

	cardtodelete = cardnumber[hasinclude-1];
	if ( cardtodelete < (numcards - 1) ) {
		if ( isblankcard( cards[cardtodelete+1] ) ) {
			numbertodelete++;
		}
	}
	printf(" deleting our include (card %i), number to delete = %i\n",cardtodelete,numbertodelete);
	if ( deletecards( cardtodelete, numbertodelete ) ) {
		printf("Error : failed to delete our include (card %i), number to delete = %i\n",cardtodelete,numbertodelete);
		return(1);
	}

	return(0);  // everything ok
}

int AddOurInclude() {
	char *z;
//	int cardout = 1;

	z = ourinclude;
	if ( addcardbefore( z , firstexe ) ) return(2);
/*
	// repair indecies in lines
	for ( int i = 0; i<numlines; i++) {
		if ( cardnumber[i] > (firstexe-1) ) cardnumber[i]+=cardout;
	}
	for ( int i = 0; i<numlines; i++) {
		if ( cardnumber[i] == firstexe+1 ) {
			cardnumber[i] = firstexe;
			continues[i] = 0;
			break;
		}
	}
	*/
		// firstexe+=cardout; // should bever be true!

	return(0);  // everything ok
}

int DeleteAllSaves() {
	int i;
	int cardtodelete;
	int numbertodelete = 1;

	for (i=0;i<numlines;i++) {
		if ( cardtype[i] != SAVE ) continue;
		cardtodelete = cardnumber[i];
		if ( deletecards( cardtodelete, numbertodelete ) ) {
			printf("Error : failed to delete a SAVE (card %i)\n",cardtodelete);
			return(1);
		}
	}

	return(0);  // everything ok
}

int AddOurSaves(){
	char *z;
	char xx[80];
	char slash[] = "/\0";
	int cardout = 0;

	int fixline = 0;

	if ( cardtype[0] == BLOCKDATA ) return (0);

	if ( addcardbefore( oursave , firstexe+fixline ) ) return(1);		// the SAVE ALL for local variables
	fixline++;
	cardout++;

	if (numcommons != 0) {
		z = xx;
		for (int i=0;i<numcommons;i++) {
			strcpy(z,oursave);
			strcat(z,slash);
			strcat(z,commonblocknames[i]);
			strcat(z,slash);
			if ( addcardbefore( z , firstexe+fixline ) ) return(2);	// add a SAVE for each named common block (per strict F90)
			fixline++;
			cardout++;
		}
	}
/*
	// repair indecies in lines
	for ( int i = 0; i<numlines; i++) {
		if ( cardnumber[i] > (firstexe-1) ) cardnumber[i]+=cardout;
	}
	for ( int i = 0; i<numlines; i++) {
		if ( cardnumber[i] == firstexe+1 ) {
			cardnumber[i] = firstexe;
			continues[i] = 0;
			break;;
		}
	}
*/
	return(0);  // everything ok

}

int isblankcard( char * x ) {
	char *s;
	int i;
	if ( x == NULL ) return(1);
	i = strlen(x);
	if ( i == 0 ) return(2);
	s = x;
	while ( isspace(*s) ) s++;
	if ( s == (x+i) ) return (3);
	return (0);
}

int writedeck() {

	FILE *fortoutfile;
	int i;
	int iret;

	filename[0] = '\000';
	strcat(filename,outprefix);
	strcat(filename,filestr);
	printf(" opening %s\n",filename);
	fortoutfile = fopen(filename, "w");
    if (fortoutfile == NULL)
    {
        printf(" Error : Could not open fortoutfile file %s , errno = %i\n",filename,errno);
        printf(" %s\n",strerror(errno));
        return (1);
    }
    for ( i=0;i<numcards;i++) {
    	fprintf(fortoutfile, "%s\n",cards[i]);
    }
    iret = fclose(fortoutfile);
    if ( iret ) {
    	printf(" ERROR : unable to clode file %s, error number = %i\n",filename,iret);
    	printf(" %s\n",strerror(iret));
    	return(2);
    }
	return(0);
}

int strnrchrjk(char * tmpstr, int istart , unsigned char c ) {
	int i;
	for ( i=istart; i>-1;i--) {
		if ( *(tmpstr+i) == c ) return (i);
	}

	return (istart);
}

int paresme( char * parsethisin , int i , char * mytopnodelabel, int cardtype) {
	PVARTREE commonnode;
	char * ourline;
	char * nextparsechar;
	int createps = 0;
	int skipcardtype = 0;

//	if ( ( ourline = (char*)malloc(strlen(lines[i]+2) ) == NULL ) {
//		printf("malloc error in ourline for line : %s in file %s\n",parsethis,filename);
//		return(10);
//	}
//	if ( parsethis != NULL ) free(parsethis);
	ourline = lines[i];
	if ( parsethisin  == NULL ) {
		createps = 1;
		if ( ( parsethis = (char*)malloc(strlen(ourline)+200) ) == NULL ) {
			printf(" error in malloc of parseme for line : %s in file %s\n",ourline,filename);
			return(8);
		}
		strcpy(parsethis, ourline);
	} else {
		parsethis = parsethisin;
	}
	nextparsechar = parsethis;
	if ( createps && cardtype != COMMON ) {
		parsethis+= ( strlen( statementkeywords[cardtype] ) + 6  );
		skipcardtype = 1;
	}
//	printf(" parsethis = %s\n",parsethis);

	topnodelabel = mytopnodelabel;
	int iret;
	if ( (iret = parsecommon()) ) {													// parse takes place here
		printf("parsecommon failed, returned %i\n",iret);
		if ( savebadboys ) {
			fprintf(badboys," %s parse failed with iret = %i in %s\n",mytopnodelabel,iret,filename);
			DeleteTree(vartree, 1);
			if ( createps ) free(nextparsechar);
			return(0);
		} else {
			fprintf(stderr," %s parse failed with iret = %i in %s\n",mytopnodelabel,iret,filename);
			return(9);
		}
	}
	//dumpparsetree(vartree);
	// do a bunch of checks... just in case we have some undetected bugs...
	if ( strcmp ( GetBranchName(vartree), mytopnodelabel) != 0 ) {
		printf("error 1 %s parse in file %s\n",mytopnodelabel,filename);
		if ( savebadboys ) {
			fprintf(badboys," %s node not found in %s\n",mytopnodelabel,filename);
			DeleteTree(vartree, 1);
			if ( createps ) free(nextparsechar);
			return(0);
		} else {
			fprintf(stderr," %s node not found in %s\n",mytopnodelabel,filename);
			return(10);
		}
	}
	if ( ( commonnode = GetBranch( vartree, 0 ) ) == NULL ) {
		printf("error 2 in %s parse in file %s\n",mytopnodelabel,filename);
		if ( savebadboys ) {
			fprintf(badboys," %s parse failed with error 2 in %s\n",mytopnodelabel,filename);
			DeleteTree(vartree, 1);
			if ( createps ) free(nextparsechar);
			return(0);
		} else {
			fprintf(stderr," %s parse failed with error 2 in %s\n",mytopnodelabel,filename);
			return(11);
		}
	}
	if ( cardtype != -1 && ( ! skipcardtype ) ) {
		int temp;
		temp = cardtype;
		if ( temp < 0 ) temp = -2 - temp;
		if ( strcmp ( GetBranchName(commonnode), statementkeywords[temp] ) != 0 ) {
			printf("error 3 in %s parse in file %s\n",mytopnodelabel,filename);
			if ( savebadboys ) {
				fprintf(badboys," %s parse failed with error 3 : %s card not found in %s\n file = %s\n",
						mytopnodelabel,statementkeywords[temp],parsethis,filename);
				DeleteTree(vartree, 1);
				if ( createps ) free(nextparsechar);
				return(0);
			} else {
				fprintf(stderr," %s parse failed with error 3 : %s card not found in %s\n file = %s\n",
						mytopnodelabel,statementkeywords[temp],parsethis,filename);
				return(12);
			}
		}
	}
//	if ( skipcardtype ) commonnode = vartree;
	if ( cardtype < -1 && ( ! skipcardtype ) ) {
		int  temp = -2 - cardtype;
		int numcommons = commonnode->numbranches;
		if ( numcommons < 1 ) {
			printf("error 4 in %s parse in file %s\n",mytopnodelabel,filename);
			if ( savebadboys ) {
				fprintf(badboys," %s parse failed with error 4 : nothing parsed after keyword \"%s\" line \"%s\"\n in file %s\n",
						mytopnodelabel,statementkeywords[temp],parsethis,filename);
				DeleteTree(vartree, 1);
				if ( createps ) free(nextparsechar);
				return(0);
			} else {
				fprintf(stderr," %s parse failed with error 4 : nothing parsed after keyword \"%s\" line \"%s\"\n in file %s\n",
						mytopnodelabel,statementkeywords[temp],parsethis,filename);
				return(13);
			}
		}
	}
	if ( createps ) free(nextparsechar);
	return(0);
}

int loaddatatypes () {

	for (int i = 0; i<numlines;i++) {
		switch ( cardtype[i] ) {
			case IMPLICIT:
				if ( setimplicits(i) ) return (1);
				break;
			case PARAMETER:
			case SUBROUTINE:
			case FUNCTION:
			case REALFUNCTION:;
			case INTEGERFUNCTION:
			case DOUBLEPRECISIONFUNCTION:
			case LOGICALFUNCTION:
			case CHARACTERFUNCTION:
			case COMPLEXFUNCTION:
			case PROGRAM:
			case DOUBLEPRECISION:
			case LOGICAL:
			case REAL:
			case INTEGER:
			case CHARACTER:
			case COMPLEX:
			case DIMENSION:
				if ( setvardata(i , cardtype[i]) ) return (2);
				break;
			case EQUIVALENCE:
				break;

			default:
				break;
		}
	}

	return 0;
}

int setimplicits(int mylinenum) {
//	unsigned const char defimplicits[] = {[0 ... 7] = REAL, [8 ... 13] = INTEGER, [14 ... 25] = REAL};
//	unsigned char myimplicits[26];
	PVARTREE commonnode;
	PVARTREE charnode;

	topnodelabel = mytopnodelabel;

	if ( paresme( NULL , mylinenum , topnodelabel, IMPLICIT) ) return (1);
	if ( printflag > 1 ) dumpparsetree(vartree);

	int numtype = vartree->numbranches;
	int numbranch;
	int type;
	int k;
	char jchar;
	char * ctype;
	char * chartype;
	unsigned char firstalpha;
	firstalpha = 'A';
	for (int i=0;i<numtype;i++) {
		commonnode = GetBranch( vartree, i );
		ctype = GetBranchName(commonnode);
		type = -1;
		for ( k=0;k<numkeywords;k++) {
			if ( strcmp(ctype,statementkeywords[k]) == 0 ) {
				type = k;
				break;
			}
		}
		if ( type == -1 ) {
			printf(" error in implicit parse: %s is not a recognized type\n",ctype);
			return(1);
		}
		if ( ( numbranch = NumChildBranch( commonnode ) ) == 0 ) {
			printf(" error in implicit parse: list for type %s is empty\n",ctype);
			return(2);
		}
		char lastchar;
		char nextchar;
		int isrange = 0;
		for ( k=0;k<numbranch;k++) {
			charnode = GetBranch( commonnode, k );
			chartype = GetBranchName( charnode );
			if ( strlen(chartype) != 1 ) {
				printf(" error in implicit parse: expected a single character, but found \"%s\"  (%i characters)\n",
						chartype,(int)strlen(chartype) );
				return(3);
			}
			if ( chartype[0] == '-' ) {
				isrange = 1;
				continue;
			}
			if ( ! isalpha( chartype[0] ) ) {
				printf(" error in implicit parse: expected an alpha character, but found \"%s\"\n",chartype);
				return(4);
			}
			nextchar = toupper(chartype[0]);
			if ( isrange ) {
				for ( jchar=lastchar;jchar<nextchar+1;jchar++ ) {
					myimplicits[jchar-firstalpha] = type;
				}
				isrange = 0;
			} else {
				myimplicits[nextchar-firstalpha] = type;
				lastchar = nextchar;
			}
		}
		if ( isrange ) {
			printf(" error in implicit parse: expected another alpha character to indicate the end of a range "
					"for tyep %s found nothing.\n",chartype);
			return(5);
		}
	}

	DeleteTree(vartree,0);

	if ( printflag > 1 ) {
		printf("implicit array = \n");
		for ( k=0;k<26;k++) {
			type = myimplicits[k];
			printf(" %c    %s\n",firstalpha+k,statementkeywords[type]);
		}
	}

	return(0);
}

int setvardata(int mylinenum, int cardtp ) {
//	unsigned const char defimplicits[] = {[0 ... 7] = REAL, [8 ... 13] = INTEGER, [14 ... 25] = REAL};
//	unsigned char myimplicits[26];
	PVARTREE commonnode;
	PVARTREE charnode;

	topnodelabel = mytopnodelabel;

	if ( paresme( NULL , mylinenum , topnodelabel, cardtp) ) return (1);
	if ( printflag > 1 ) {
		printf(" TYPE STATEMENT %s\n",statementkeywords[cardtp]);
		dumpparsetree(vartree);
	}


	DeleteTree(vartree,0);


	return (0);
}

