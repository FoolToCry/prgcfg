//-----------------------------------------------------------------------------
//  Description    : Command Line Configuration source file
//                   Functions for reading program configurations from command-
//                   line and files
//------------------------------------------------------------------------------
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "../LIB/mystrings.h"
#include "../LIB/prgcfg.h"

//#define debug_options              // debug command line

//-----------------------------------------------------------------------------
// defines, variables and functions for command line evaluation
//-----------------------------------------------------------------------------
#define PrgCfgOptSwChar "-"                       // optiion switch sign: default: '-'

char PrgCfgCommentSign[5] = "//";		  // comment sign for reading configuration from files
char PrgCfgSeparatorSign = '=';			  // separation sign between option amd value when reading configuration from file
char PrgCfgPrgName[PrgCfgOptMaxNameLength]   =""; // name of program
char PrgCfgPrgCall[PrgCfgOptMaxNameLength]   =""; // commandline
char PrgCfgUnassigned[PrgCfgOptMaxNameLength]=""; // input value (everything left beside otions and their arguments)
char PrgCfgUsage[PrgCfgUsageTextLength]      =""; // first text displayed in function prgcfghelp
int  PrgCfgAutoLineBreak = 80;			  // insert line break in help display if set to value > 0
int  PrgCfgLogHdCommentOn = 0;			  // print comment sign in each log header line

//------------------------------------------------------------------------------
// truncate line after string comment
//------------------------------------------------------------------------------
void trunc_comment( char *line ,char *comment) {
  int debug = 0;
  int linelength;
  int opentick  = 0;
  int opendtick = 0;
  int i;
  int k;
  int nocomment;
  linelength = strlen( line);

  if( line[linelength-1] == '\n') {
    line[linelength-1] = '\0'; // remove newline command at end of line
    --linelength;
  }
  if( line[linelength-1] == '\r') {
    line[linelength-1] = '\0'; // remove return command at end of line
    --linelength;
  }

  if( debug) {
    printf("\n\n  dbg: debug_line function");
    printf("\n  dbg: comment : '%s', length: %d", comment, (int) strlen(comment));
    printf("\n  dbg: length  : %d",  linelength);
    printf("\n  dbg: lastchar:0x%x", line[linelength-1]);
    printf("\n  dbg: position: ");
    for( i=0; i< linelength; i++)
      if( (i%10) ==0)
	printf("%d", i/10);
      else
	printf(" ");
    printf("\n  dbg:           ");
    for( i=0; i< linelength; i++)
      printf("%d", i%10);
    printf("\n  dbg: line    :'%s'", line);
  }

  if(debug) for(i=0; i< linelength; i++) printf("\n line[%2d] = %3d :'%c'", i, line[i], line[i]);
  for(i=0; i< linelength; i++) {
    if( (line[i] == '\'') && (((i==0) || (line[i-1] != '\\')) || ((i>1) && (line[i-1] == '\\') && (line[i-2] == '\\'))) ) {
      if(opentick == 0)
	++opentick;
      else if( (i>0) && ((line[i-1] != '\\') || ((i>1) && (line[i-1] == '\\') && (line[i-2] == '\\'))))
	--opentick;
      if(debug) printf("\n  dbg: at %2d, openticks=%d", i, opentick);
    }

    if(( line[i] == '"') && ((i==0)  || (line[i-1] != '\\')) ) {
      if(opendtick == 0)
	++opendtick;
      else if( (i>0) && (line[i-1] != '\\'))
	--opendtick;
    }

    nocomment = 0;
    if( (line[i] == comment[0]) && (opentick==0) && (opendtick==0)) {
      for( k=1; k<strlen(comment); k++)
	if( line[i+k] != comment[k]) {
	  if( debug) printf("\n  dbg: no comment at %d", i);
	  nocomment = 1;
	  break;
	}
      if( nocomment == 0) {
	if( debug) printf("\n  dbg: found comment at %d", i);
	line[i] = '\0';
	break;
      }
    }
  }
} // trunc_comment() -----------------------------------------------------------

//------------------------------------------------------------------------------
// remove marks from beginning and end of strings
//------------------------------------------------------------------------------
void remove_marks( char *line , char *mark) {
  int debug = 0;
  char tmps[PrgCfgOptMaxNameLength] = "";
  char tmps2[PrgCfgOptMaxNameLength] = "";
  int linelength;
  int i, k, rme;
  linelength = strlen( line);
  rme = 0;

  if( debug) {
    printf("\n\n  dbg: debug remove marks function");
    printf("\n  dbg: mark : '%s', length: %d", mark, (int) strlen(mark));
    printf("\n  dbg: length  : %d",  linelength);
    printf("\n  dbg: lastchar:0x%x", line[linelength-1]);
    printf("\n  dbg: position: ");
    for( i=0; i< linelength; i++)
      if( (i%10) ==0)
	printf("%d", i/10);
      else
	printf(" ");
    printf("\n  dbg:           ");
    for( i=0; i< linelength; i++)
      printf("%d", i%10);
    printf("\n  dbg: line    :'%s'", line);
  }

  if( line[0] == mark[0]) {
    if(debug) printf("\n  dbg: found mark at 0");
    for( i = linelength; i>0 ; i--)
      if( (line[i] == mark[0] ) && ((line[i-1] != '\\') || ((line[i-1] == '\\') && (line[i-2] == '\\')))) {
	if(debug) printf("\n  dbg: at %2d, end mark", i);
	sprintf( tmps, "%s", substr( line, 1, i-1));
	strcpy( line, tmps);
	if(debug) printf("\n  dbg: string ='%s'", line);
	rme = 1;
	break;
    }
  }

  linelength = strlen(line);
  // remove \ before mark from string
  if( rme == 1) {
    for(k=0; k<linelength; k++) {
      if( line[k] == '\\') {
	if( debug) {
	  printf("\n  dbg: position: ");
	  for( i=0; i< linelength; i++)
	    if( (i%10) ==0)
	      printf("%d", i/10);
	    else
	      printf(" ");
	  printf("\n  dbg:           ");
	  for( i=0; i< linelength; i++)
	    printf("%d", i%10);
	  printf("\n  dbg: line    :'%s'", line);
	  printf("\n  dbg: at %2d, found escape", k);
	}
	if(k==0) {
	  sprintf( tmps,  "%s", substr( line, 1, linelength));
	  if(debug) printf("\n  dbg: tmps    :'%s'", tmps);
	}
	else {
	  sprintf( tmps,  "%s", substr( line, 0, k));
	  if(debug) printf("\n  dbg: tmps    :'%s'", tmps);
	  sprintf( tmps2, "%s", substr( line, k+1, linelength));
	  if(debug) printf("\n  dbg: tmps2   :'%s'", tmps2);
	  strcat( tmps, tmps2);
	  if(debug) printf("\n  dbg: tmps    :'%s'", tmps);
	}
	strcpy( line, tmps);
	linelength = strlen(line);
      }
    }
  }

} // remove_marks() ------------------------------------------------------------

//------------------------------------------------------------------------------
// print debug information
// used only for development
//------------------------------------------------------------------------------
void prgcfgdbg( PrgCfgOpt *cfgval)
{
  int i;
  bool *bval;
  int *ival;
  double *dval;
  char *cval;
  int tstnew =0;
  static char tmpstr[100];
  static int change=1;

  int opt_cnt;
  opt_cnt=0;
  while( cfgval[opt_cnt].name) {
    opt_cnt++;
  }

  printf("\n number of options  = %d\n", opt_cnt);

  for( i=0; i<( opt_cnt); i++) {
    printf("\ncfgval[%d]  name : %s \n\t   type : %d", i, cfgval[i].name, cfgval[i].type);
    printf("\n\t   value: ");

    if( cfgval[i].type == stringt) {
      printf("%s", (char *)cfgval[i].value);
      if( tstnew) {
	sprintf( tmpstr, "%s %d.", "this string has changed", change++);
	//*(char **)cfgval[i].value = tmpstr;
	//sprintf( (char *)cfgval[i].value, "%s", tmpstr);
	//strcpy( (char *)cfgval[i].value, tmpstr);
	strcpy( (char *)cfgval[i].value, tmpstr);
	printf("\n\t   --> new value of %s = ", cfgval[i].name);
	printf("'%s'", (char *)cfgval[i].value);
      }
    }
    else if( cfgval[i].type == boolt){
      bval = cfgval[i].value;
      printf("%d", *bval);
      if( tstnew) {
	if( *bval)
	  *bval = 0;
	else
	  *bval = 1;
	cfgval[i].value = bval;
	bval = cfgval[i].value;
	printf(" --> new value = %d", *bval);
      }
    }
    else if( cfgval[i].type == intt){
      ival = cfgval[i].value;
      printf("%d", *ival);
      if( tstnew) {
	*ival = *ival +20;
	cfgval[i].value = ival;
	ival = cfgval[i].value;
	printf(" --> new value = %d", *ival);
      }
    }
    else if( cfgval[i].type == doublet){
      dval = cfgval[i].value;
      printf("%f", *dval);
      if( tstnew) {
	*dval = *dval +1.345;
	cfgval[i].value = dval;
	dval = cfgval[i].value;
	printf(" --> new value = %f", *dval);
      }
    }
    else if( cfgval[i].type == chart){
      cval = cfgval[i].value;
      printf("%c", *cval);
      if( tstnew) {
	*cval = *cval +1;
	cfgval[i].value = cval;
	cval = cfgval[i].value;
	printf(" --> new value = %c", *cval);
      }
    }
  }
}// prgcfgdbg()-----------------------------------------------------------------



//------------------------------------------------------------------------------
// print config information
//------------------------------------------------------------------------------
void prgcfgprint( PrgCfgOpt *cfgval)
{
  int i, k;
  bool *bval;
  int *ival;
  double *dval;
  char *cval;
  int opt_cnt;
  int olength = 1;
  int vlength = 1;

  char *tmps[100]; //PrgCfgOptMaxNameLength];
  char **tmpsa;
  opt_cnt=0;
  while( cfgval[opt_cnt].name) {
    opt_cnt++;
  }

  printf("\n number of configuration options = %d", opt_cnt);

  for( i=0; i< opt_cnt; i++) {
    tmps[i] = (char *) malloc( (PrgCfgOptMaxNameLength+1) * sizeof( char));
  }

  //----------------------------------------------------------------------------
  // write values to strings
  //----------------------------------------------------------------------------
  for( i=0; i< opt_cnt; ++i) {
    if( cfgval[i].type == stringt) {
      sprintf( tmps[i], "%s", (char *)cfgval[i].value);
    }
    else if( cfgval[i].type == stringat) {
      tmpsa = cfgval[i].value;
      sprintf( tmps[i], "%s", *tmpsa);
    }
    else if( cfgval[i].type == boolt){
      bval = cfgval[i].value;
      sprintf( tmps[i], "%d", *bval);
    }
    else if( cfgval[i].type == intt){
      ival = cfgval[i].value;
      sprintf( tmps[i], "%d", *ival);
    }
    else if( cfgval[i].type == intat){
      ival = cfgval[i].value;
      sprintf( tmps[i], "%d", *ival);
    }
    else if( cfgval[i].type == doublet){
      dval = cfgval[i].value;
      sprintf( tmps[i], "%f", *dval);
    }
    else if( cfgval[i].type == chart){
      cval = cfgval[i].value;
      sprintf( tmps[i], "%c", *cval);
    }
  }

  //----------------------------------------------------------------------------
  // find max string length of option name and value
  //----------------------------------------------------------------------------
  for( i=0; i< opt_cnt; ++i) {
    if( strlen( cfgval[i].name) > olength)
      olength = strlen( cfgval[i].name);
    if( strlen( tmps[i]) > vlength)
      vlength = strlen( tmps[i]);
  }

  //----------------------------------------------------------------------------
  // print headline
  //----------------------------------------------------------------------------
  printf("\n\n option");
  for( k=strlen("option"); k<olength; k++)
    printf(" ");
  printf("   value");
  for( k=strlen("value"); k<vlength; k++)
    printf(" ");
  printf(" mod. comment");
  printf("\n");
  for( k=0; k<80; k++)
    printf("-");

  //----------------------------------------------------------------------------
  // print option name, value, used and comment
  //----------------------------------------------------------------------------
  for( i=0; i< opt_cnt; ++i) {
    printf("\n %s", cfgval[i].name);
    for( k=strlen( cfgval[i].name); k<olength; k++)
      printf(" ");
    printf(" = %s", tmps[i]);
    for( k=strlen( tmps[i]); k<vlength; k++)
      printf(" ");
    printf(" %2d", cfgval[i].used);
    printf("   %s", cfgval[i].help);
  }

  printf("\n\nvariable modified if column mod.>0");

  if( strlen( PrgCfgUnassigned))
  printf("\n\n not assigned: %s", PrgCfgUnassigned);
  for( i=0; i< opt_cnt; i++) {
    free(tmps[i]);
  }

  printf("\n");
}// prgcfgprint()------------------------------------------------------------------


//-----------------------------------------------------------------------------
// display help screen
//-----------------------------------------------------------------------------
void prgcfghelp( PrgCfgOpt *cfgval, int switch_on)
{
  int i, j, k;
  int length  = 1;
  int opt_cnt;
  int cpos;
  int debug = 0;
  //FILE *tmpfile;
  char bold[]    = "\033[1m";
  char boldoff[] = "\033[0m";

  opt_cnt = 0;
  while( cfgval[opt_cnt].name) {
    opt_cnt++;
  }

  // find max string length
  for( i=0; i< opt_cnt; ++i) {
    if( strlen( cfgval[i].name) > length)
      length = strlen( cfgval[i].name);
  }

  //tmpfile = fopen( "_PrgCfgTmpFile_", "w");

  // check if user defined help text should be used and print help intro
  if( strlen( PrgCfgUsage))
    printf("%s\n", PrgCfgUsage);
  else
    printf("\nusage: %s [%s<option> [<value>]]\n", PrgCfgPrgName, PrgCfgOptSwChar);

  // print help text for each option
  for( i=0; i< opt_cnt; ++i) {
    if( debug) printf("\noption: %d, name: %s, used: %d, help: %s\n", i, cfgval[i].name, cfgval[i].used, cfgval[i].help);
    // dummy option generates line without "-name:" but prints help text
    if( strcmp(cfgval[i].name, "_noprint_") == 0)
      printf("%s\n", cfgval[i].help);

    else if( cfgval[i].used >= 0) { // set used to negative value for hidden switches
      // print "-"
      if( switch_on)
	printf(" %s", PrgCfgOptSwChar);
      else
	printf(" ");
      // print "name "
      printf("%s%s%s", bold, cfgval[i].name, boldoff);
      for( k=strlen( cfgval[i].name); k<length; k++)
	printf(" ");
      printf(" ");

      cpos = length;
      // print help text, if string contains '\n' insert line break and indent
      for( j=0; j<strlen( cfgval[i].help); j++) {
	// auto line break
	if( (PrgCfgAutoLineBreak > 0) && ( cpos >= PrgCfgAutoLineBreak) && (cfgval[i].help[j] == ' ')) {
	  printf("\n  ");
	  // indent
	  for( k=0; k<length; k++)
	    printf(" ");
	  if( switch_on)
	    printf(" ");
	  cpos = length;
	}
	else {
	  // print each character of help string
	  printf("%c", cfgval[i].help[j]);
	  cpos++;
	}

	// check for '\n' and insert indent
	if( cfgval[i].help[j] == '\n') {
	  printf("  ");
	  // indent
	  for( k=0; k<length; k++)
	    printf(" ");
	  if( switch_on)
	    printf(" ");
	  cpos = length;
	}
      }
      printf("\n");
    }
  }
  printf("\n");
  //i = system( "more  _PrgCfgTmpFile_");
  //printf("\nsystem() returned %d\n", i);
  //fclose( tmpfile);

} // prgcfghelp() --------------------------------------------------------------


//-----------------------------------------------------------------------------
// assign values to variables
// int index    : index of cfgval to be modified
// char *value  : value to store in cfgval[index]
// int arind    : gives the array index if cfgval[index] is array type
// int ovwrfile : if 1 cfgval[index] won't be marked as used
//-----------------------------------------------------------------------------
int prgcfgvarassign( PrgCfgOpt *cfgval, int index, char *value, int *arind, int ovwrfile)
{
  //bool b = 0;
  int *bval, *ival, i = 0;
  int **ia2val;
  double *dval, **da2val;
  double d;
  char *cval;
  char c;
  char *tmps;     // string
  char **tmpsa;   // 1 dim. string array
  char ***tmpsa2; // 2 dim. string array
  int success = 1;
  int debug = 0;
#ifdef debug_options
  debug = 1;
#endif
  c = ' ';
  dval = &d;
  ival = &i;
  if(debug) printf("\ndbg: \t\t%d: prgcfgvarassign for %s, value:%s, index: %d %d %d",
		   __LINE__, cfgval[index].name, value, arind[0], arind[1], arind[2]);

  //----------------------------------------------------------------
  // boolean option
  //----------------------------------------------------------------
  if( cfgval[index].type == boolt){
    bval = cfgval[index].value;
    if(debug) printf("\ndbg: \t\t%d: new bool value: %s, old value:'%d', new:'%s'", __LINE__, cfgval[index].name, *bval, value);
    if( match( value, "toggle")) { // call from command line
      if( *bval == true)          // toggle boolean value
	*bval= false;
      else
	*bval = true;
    }
    else {
      if( match( touppers( value), "TRUE") || match( value, "1")) {
	if(debug) printf("\ndbg: \t\t%d: FILE:new bool value: %s, old value:'%d', new:'%s'", __LINE__, cfgval[index].name, *bval, value);
	*bval = true;
      }
      else
	*bval= false;
    }
  }

  //----------------------------------------------------------------
  // integer option
  // int size        : 4
  // long size       : 4
  // long long size  : 8
  // float size      : 4
  // double size     : 8
  // long double size: 12
  //----------------------------------------------------------------
  else if( cfgval[index].type == intt){
    ival = cfgval[index].value;
    if( debug) printf("\ndbg: \t\t\t\told %s = %d",  cfgval[index].name, *ival);
    i = (int)strtod( value, 0); // convert input value to decimal
    *ival = i;
    if( ( strlen( value)>1) && (i == 0) && (match( value, "0X") == 0) && (match( value, "0x") == 0) && (match( value, "0;") == 0)) {
      printf("\n*** ERROR: expected integer: '%s'", value);
      success = 0;
      }
    if( debug) printf("\ndbg: \t\t\t\tinteger value = %d", *ival);
  }

  //----------------------------------------------------------------
  // integer 1. dim array
  //----------------------------------------------------------------
  else if( cfgval[index].type == intat){
    ival = cfgval[index].value;
    if( debug) printf("\ndbg: \t\t\t\told %s[%d] = %d",  cfgval[index].name, arind[0], *ival);
    i = (int)strtod( value, 0); // convert input value to decimal
    ival += arind[0];
    *ival = i;
    if( ( strlen( value)>1) && (i == 0) && (match( value, "0X") == 0) && (match( value, "0x") == 0) && (match( value, "0;") == 0)) {
      printf("\n*** ERROR: expected integer: '%s'", value);
      success = 0;
      }
    if( debug) printf("\ndbg: \t\t\t\tinteger value in array[%d]= %d", arind[0], *ival);
  }

  //----------------------------------------------------------------
  // integer 2 dim. array
  //----------------------------------------------------------------
  else if( cfgval[index].type == inta2t){
    //**ia2val
    if( debug) printf("\ndbg: \t\t\t\t array[%d][%d]", arind[0], arind[1]);
    ia2val = cfgval[index].value;
    if( debug) printf("\ndbg: \t\t\t\told %s = %d",  cfgval[index].name, ia2val[arind[0]][arind[1]]);
    i = (int)strtod( value, 0); // convert input value to decimal
    ia2val[arind[0]][arind[1]] = i;
    if( ( strlen( value)>1) && (i == 0) && (match( value, "0X") == 0) && (match( value, "0x") == 0) && (match( value, "0;") == 0)) {
      printf("\n*** ERROR: expected integer: %s", value);
      success = 0;
      }
    if( debug) printf("\ndbg: \t\t\t\tinteger value in array[%d][%d]= %d", arind[0], arind[1], ia2val[arind[0]][arind[1]]);
  }

  //----------------------------------------------------------------
  // double       strtod( const char *nptr, char **endptr);
  // float        strtof( const char *nptr, char **endptr);
  // long double strtold( const char *nptr, char **endptr);
  //----------------------------------------------------------------
  else if( cfgval[index].type == doublet){
    dval = cfgval[index].value;
    if( debug) printf("\ndbg: \t\t\t\told %s = %f",  cfgval[index].name, *dval);
    d = (double)strtod( value, 0); // convert input value to double
    *dval = d;
    if( ( strlen( value)>1) && (d == 0) && (match( value, "0X") == 0) && (match( value, "0x") == 0) && (match( value, "0;") == 0)) {
      printf("\n*** ERROR: expected double: %s", value);
      success = 0;
      }
    if( debug) printf("\ndbg: \t\t\t\tdouble value = %f", *dval);
    dval = cfgval[index].value;
    if( debug) printf("\ndbg: \t\t\t\t%s = %f",  cfgval[index].name, *dval);
  }

  //----------------------------------------------------------------
  // double 1. dim array
  //----------------------------------------------------------------
  else if( cfgval[index].type == doubleat){
    dval = cfgval[index].value;
    if( debug) printf("\ndbg: \t\t\t\told %s = %f",  cfgval[index].name, *dval);
    d = (double)strtod( value, 0); // convert input value to double
    dval += arind[0];
    *dval = d;
    if( ( strlen( value)>1) && (d == 0) && (match( value, "0X") == 0) && (match( value, "0x") == 0) && (match( value, "0;") == 0)) {
      printf("\n*** ERROR: expected double: %s", value);
      success = 0;
      }
    if( debug) printf("\ndbg: \t\t\t\tdouble value = %f", *dval);
  }

  //----------------------------------------------------------------
  // double 2. dim array
  //----------------------------------------------------------------
  else if( cfgval[index].type == doublea2t){
    // **da2val
    da2val = cfgval[index].value;
    if( debug) printf("\ndbg: \t\t\t\told %s = %f",  cfgval[index].name, *dval);
    d = (double)strtod( value, 0); // convert input value to double
    da2val[arind[0]][arind[1]] = d;
    if( ( strlen( value)>1) && (d == 0) && (match( value, "0X") == 0) && (match( value, "0x") == 0) && (match( value, "0;") == 0)) {
      printf("\n*** ERROR: expected double: %s", value);
      success = 0;
      }
    if( debug) printf("\ndbg: \t\t\t\tdouble value in array[%d][%d]= %f", arind[0], arind[1], da2val[arind[0]][arind[1]]);
  }


  //----------------------------------------------------------------
  // character option
  //
  //   Oct   Dec   Hex   Char
  //   --------------------------
  //   000   0     00    NUL '\0'
  //   007   7     07    BEL '\a'
  //   010   8     08    BS  '\b'
  //   011   9     09    HT  '\t'
  //   012   10    0A    LF  '\n'
  //   013   11    0B    VT  '\v'
  //   014   12    0C    FF  '\f'
  //   015   13    0D    CR  '\r'
  //----------------------------------------------------------------
  else if( cfgval[index].type == chart) {
    cval = cfgval[index].value;
    // remove trailing "'"
    if( strlen( value) > 1) {
      if(debug) printf("\ndbg: value = '%s'", value);
      if( ((value[0] == '\'') && (value[strlen(value)-1] == '\'')) ||
	  ((value[0] == '\\') && (strlen( value) == 2))) {
	if( (strlen(value) == 4) && (value[1] == '\\'))
	  i = 2;
	else if( (strlen(value) == 2) && (value[0] == '\\'))
	  i = 1;
	else if((value[0] == '\'') && (value[2] == '\''))
	  i = 1;
	else {
	  printf("\n*** ERROR: expected character, no string: '%s'", value);
	  success = 0;
	}
	// case char = '\''
	if(debug) printf("\ndbg: \t\t\tvalue = '%s', character = '%c'", value, value[2]);
	if(      value[i] == '0') c = 0x00;      //    NUL '\0'
	else if( value[i] == 'a') c = 0x07;      //    BEL '\a'
	else if( value[i] == 'b') c = 0x08;      //    BS  '\b'
	else if( value[i] == 't') c = 0x09;      //    HT  '\t'
	else if( value[i] == 'n') c = 0x0A;      //    LF  '\n'
	else if( value[i] == 'v') c = 0x0B;      //    VT  '\v'
	else if( value[i] == 'f') c = 0x0C;      //    FF  '\f'
	else if( value[i] == 'r') c = 0x0D;      //    CR  '\r'
	else 	                  c = value[i];
	if(debug) printf("\ndbg: \t\t\tvalue = '%s', character = '%c'", value, value[i]);
      }
    }
    else {
      if(debug) printf("\ndbg: \t\t\tvalue = '%s', character = '%c'", value, value[0]);
      c = value[0];
    }
    *cval = c;
    cfgval[index].value = cval;
    if( debug) printf("\ndbg: \t\t\t\tcharacter value = %c", *cval);
  }

  //----------------------------------------------------------------
  // character 1 dim. array
  // string option
  //----------------------------------------------------------------
  else if( cfgval[index].type == stringt) {
    // remove leading and trailing" from string
    if( value[0] == '"')
      remove_marks( value, "\"");
    else if( value[0] == '\'')
      remove_marks( value, "'");

    tmps = cfgval[index].value;
    strcpy( tmps, value);
    if( debug) printf("\n\t\t\t\tstring value = %s", value);
  }

  //----------------------------------------------------------------
  // character 2 dim. array
  // string 1 dim. array option
  //----------------------------------------------------------------
  else if( cfgval[index].type == stringat) {
    // remove leading and trailing" from string
    if( value[0] == '"')
      remove_marks( value, "\"");
    else if( value[0] == '\'')
      remove_marks( value, "'");

    tmpsa = cfgval[index].value;
    tmpsa += arind[0];
    strcpy( *tmpsa, value);
    if( debug) printf("\n\t\t\t\tstring [%d] value = %s", arind[0], value);
  }

  //----------------------------------------------------------------
  // character 3 dim. array
  // string 2 dim. array option
  //----------------------------------------------------------------
  else if( cfgval[index].type == stringa2t) {
    // remove leading and trailing" from string
    if( value[0] == '"')
      remove_marks( value, "\"");
    else if( value[0] == '\'')
      remove_marks( value, "'");

    tmpsa2 = cfgval[index].value;
    strcpy( (char *)tmpsa2[arind[0]][arind[1]], value);
    if( debug) printf("\n\t\t\t\tstring [%d][%d] value = '%s'", arind[0], arind[1], value);
  }

  else {
    success = 0;
    if(debug) printf("\ndbg: \t\t%d: *** ERROR: unknown type for %s",  __LINE__, cfgval[index].name);
  }

  if( ovwrfile == 0)
    ++cfgval[index].used;               // mark switch used
  if(debug) printf("\ndbg: \t\t%d: mark %s as used\n",  __LINE__, cfgval[index].name);

  return( success);
}// prgcfgvarassign() ---------------------------------------------------------------


//-----------------------------------------------------------------------------
// extract array index from strings
//-----------------------------------------------------------------------------
int prgcfgetarryindex( char *s, int *arind)
{
  int i, index, open, close;
  int debug = 0;
  char tmps[1000];
  char tmps2[500];
  char tmps3[500];
  index = 0;
  open = 0;
  close = 0;
  if( debug) printf("\n\n prgcfgetarryindex: \n s: '%s'", s);
  if( debug) printf("\n ind[0] = %d", arind[0]);
  if( debug) printf("\n ind[0] = %d", arind[1]);
  if( debug) printf("\n ind[0] = %d", arind[2]);

  strcpy( tmps, s); // make a copy
  for( i=0; i<3; i++) {
    open = match( tmps,"[");
    close = match( tmps,"]");

    if( debug) printf("\n [ at %d, ] at %d", open, close);
    if( close == 0)
      break;

    sprintf( tmps2,  substr( tmps, open, close - open -1 ));
    if( debug) printf("\n tmps2 = %s", tmps2);
    arind[i] = atoi(tmps2);
    ++index;
    if( debug) printf("\n index= %d", arind[i]);
    //remove index from string
    sprintf( tmps3, "%s", substr( tmps, close, 0));
    if( debug) printf("\n tmps3 = %s", tmps3);
    //sprintf( tmps, "%s%s", substr( tmps, 0, open-1), tmps3);
    strcpy( tmps, tmps3);
    if( debug) printf("\n tmps = %s", tmps);
  }
  if( debug) printf("\n ind[0] = %d", arind[0]);
  if( debug) printf("\n ind[0] = %d", arind[1]);
  if( debug) printf("\n ind[0] = %d", arind[2]);
  if( debug) printf("\n index# = %d", index);

  return( index);
} // prgcfgetarryindex() ------------------------------------------------------



//-----------------------------------------------------------------------------
// read command line and check assignment to defined options
//-----------------------------------------------------------------------------
int prgcfg_read_cmdline( int argc, char*argv[], PrgCfgOpt *cfgval, int ovwrfile)
{
  int i,k,l, pos=0;
  int arglength=0;
  int error=0;
  int success=0;
  char value[1000]; // string for values
  char tmps[1000];
  char origargv[1000];
  int debug = 0;
#ifdef debug_options
  debug = 1;
#endif
  int *ind;	     // sorted option index
  int *ol;	     // sorted option length
  int onl;           // option name length;
  char *argvcpy[100];// local copy of argv
  int cfgmax = 0;
  int arrayindex[3];

  arrayindex[2] = arrayindex[1] = arrayindex[0] = 0;
  cfgmax = 0;
  while( cfgval[cfgmax].name) {
    cfgmax++;
  }

  // store commandline
  PrgCfgPrgCall[0] = '\0';	    // clear command line string
  for( i=0; i<argc; i++) {
    argvcpy[i] = (char *) malloc( 1000 * sizeof( char));
    strcpy( argvcpy[i], argv[i]);   // make local copy of argv
    sprintf( tmps, "%s ", argv[i]);
    strcat( PrgCfgPrgCall, tmps);   // store commandline
  }

  if( strlen( PrgCfgPrgName) == 0)        // set program name
    sprintf( PrgCfgPrgName, "%s", argv[0]);

  // sort options, longest name first
  ind = malloc( (cfgmax+1) *sizeof(int)); // sorted option index
  ol  = malloc( (cfgmax+1) *sizeof(int)); // sorted option length

  for( i=0; i<cfgmax; i++) {              // init array
    ol[i] = strlen(cfgval[i].name);
    ind[i] = i;
  }

  for( i=1; i < cfgmax; ++i) {            // sort index of options according option lenghts
    if( ol[i] > ol[i-1]) {
      onl = ol[i];
      pos = ind[i];
      k = i-1;
      while( ol[k] < onl) {
	ind[k+1] = ind[k];
        ol[k+1]  = ol[k--];
        if(k<0) break;
      }
      ol[k+1] = onl;
      ind[k+1] = pos;
    }
  }
  if( debug) for( i=0; i<cfgmax; i++) printf("\ndbg: ind[%d] = %d, length:%2d, name: %s", i, ind[i], ol[i], cfgval[ind[i]].name);
  pos = 0;

  // check if command line should overwrite configuration from file
  if( ovwrfile>0)
    ovwrfile = 2;
  else
    ovwrfile = 0;

  do{
    //----------------------------------------------------------------------------
    // read options from command line
    //----------------------------------------------------------------------------
    for( i =1; i<argc; i++) {
      if( match( argv[i], PrgCfgOptSwChar)==1) { // option switch sign in 1st.position
						 // of argv?
	if(debug) printf("\ndbg: %d: prgcfg_read_cmdline:  check argv[%i]: %s", __LINE__, i, argv[i]);
	strcpy( origargv, argv[i]);
	for( k=0; k< cfgmax; k++) {
	  if(debug) printf("\ndbg: \t\t%d: search %s in %s", __LINE__, cfgval[ind[k]].name, argv[i]);
	  if(  (pos=match( argv[i], cfgval[ind[k]].name))) {
	    if(debug) printf("\ndbg: \t\t%d: found %s type %d in '%s' at pos. %d", __LINE__, cfgval[ind[k]].name, cfgval[ind[k]].type, argv[i], pos);
	    if( cfgval[ind[k]].type != boolt ) {
	      //------------------------------------------------------------------
	      // option allowed and argument needed
	      //------------------------------------------------------------------
	      if(debug) printf("\ndbg: \t\t%d: type of %s is not boolean",  __LINE__, cfgval[ind[k]].name);
	      gsub( cfgval[ind[k]].name, "", argv[i], 0); // remove allowed option from argv
	      arglength = strlen( argv[i]);
	      if(debug) printf("\ndbg: \t\t%d: check argv[%d], option %d: %s", __LINE__, i, k, cfgval[ind[k]].name);

	      // check for array type
	      if(( cfgval[ind[k]].type == intat)     ||
		 ( cfgval[ind[k]].type == inta2t)    ||
		 ( cfgval[ind[k]].type == doubleat)  ||
		 ( cfgval[ind[k]].type == doublea2t) ||
		 ( cfgval[ind[k]].type == stringat)  ||
		 ( cfgval[ind[k]].type == stringa2t)) {// get index
		if( match(argv[i],"[")) {
		  // store array index
		  prgcfgetarryindex( argv[i], arrayindex);
		  if( debug) printf("\ndbg: \t\t%d: found array type, -->index = %d%d%d", __LINE__, arrayindex[0], arrayindex[1], arrayindex[2]);
		  gsub( substr(argv[i], match(argv[i],"[")-1, 0), "", argv[i], 0); // remove array index from argv
		  if( debug) printf("\ndbg: \t\t%d: new argv: %s", __LINE__, argv[i]);
		  arglength = strlen( argv[i]);
		}
	      }

	      if( pos > arglength ) {           // argument in next argv[]
		if( pos > 1)                    // any alowed options in argv with
		  for( l=0; l< cfgmax; ++l) {  // no required argument before (boolt)
		    if(debug) printf("\n\ndbg: \t\t%d: check option %d: search %s in %s, match=%d", __LINE__, l, cfgval[ind[l]].name, argv[i], match(argv[i], cfgval[ind[l]].name));
		    if((pos = match(argv[i], cfgval[ind[l]].name))) {
		      gsub( cfgval[ind[l]].name,"", argv[i], 0);         // remove allowed option from argv
		      if(debug) printf("\ndbg: \t\t%d: prgcfgvarassign call for %s, value:%s", __LINE__, cfgval[ind[l]].name, "toggle");
		      prgcfgvarassign( cfgval, ind[l], "toggle", arrayindex, ovwrfile);// assign value to cfgval
		      arrayindex[2] = arrayindex[1] = arrayindex[0] = 0;
		    }
		  }
		++i;
		if( argc < (i+1)) {             // required argument does not exist
		  error = 1;
		  if(debug) printf("\ndbg: \t\t%d: check argument argv[%d], option %d: %s\n", __LINE__, i, k, cfgval[ind[k]].name);
		  printf("*** ERROR: switch %s%s requires argument",PrgCfgOptSwChar,
			 cfgval[ind[k]].name);
		  exit(1); //break;
		}

		//------------------------------------------------------------------
		// get argument from next argv[]
		//------------------------------------------------------------------
		else {
		  sprintf( value, "%s", argv[i]);
		  gsub( value, "", argv[i], 0);     // remove argument from next argv
		  if(debug) printf("\ndbg: \t\t%d: prgcfgvarassign call for %s, value:%s", __LINE__, cfgval[ind[k]].name, value);
		  success = prgcfgvarassign( cfgval, ind[k], value, arrayindex, ovwrfile);
		  arrayindex[2] = arrayindex[1] = arrayindex[0] = 0;
		  if( success == 0) {  // assign value to cfgval
		    printf("\n*** ERROR: command line: %s%s %s", PrgCfgOptSwChar, cfgval[ind[k]].name, value);
		    //exit(1); //break;
		    error = 1;
		  }
		}
	      }

	      //--------------------------------------------------------------------
	      // argument in same argv[]
	      //--------------------------------------------------------------------
	      else {
		sprintf( value,"%s", substr( argv[i], pos-1, 0));
		if(debug) printf("\ndbg: \t\t%d: prgcfgvarassign call for %s, value:%s", __LINE__, cfgval[ind[k]].name, value);
		prgcfgvarassign( cfgval, ind[k], value, arrayindex, ovwrfile);     // assign value to cfgval
		arrayindex[2] = arrayindex[1] = arrayindex[0] = 0;
		gsub( value, "", argv[i], 0);        // remove argument from next argv
	      }

	      gsub( cfgval[ind[k]].name,"",argv[i], 0); // remove argument name from argv for next loop
	    }

	    else if( cfgval[ind[k]].type == boolt ) {
	      //---------------------------------------------------------------------
	      // check for options without required argument
	      //---------------------------------------------------------------------
	      if(debug) printf("\n\t\t%d: type of %s is boolean",  __LINE__, cfgval[ind[k]].name);
	      //for(k=0; k< cfgmax; k++) {
	      //  if( (pos=match(argv[i],cfgval[ind[k]].name)) ) {
	      gsub( cfgval[ind[k]].name,"",argv[i], 0); // remove allowed option from argv
	      if(debug) printf("\ndbg: \t\t%d: check argv[%d], option %d: %s",  __LINE__, i, k, cfgval[ind[k]].name);
	      if(debug) printf("\ndbg: \t\t%d: prgcfgvarassign call for %s, value:%s", __LINE__, cfgval[ind[k]].name, "");
	      prgcfgvarassign( cfgval, ind[k], "toggle", arrayindex, ovwrfile);     // assign value to cfgval
	      arrayindex[2] = arrayindex[1] = arrayindex[0] = 0;
	      // }
	      //}
	    }
	  }
	  gsub( PrgCfgOptSwChar, "", argv[i], 0);     // delete option swith sign in argv
	}
	if( strlen(argv[i])!=0) {             // anything left in argv?
          // check if value is a negative number and an intended input value
          if( match( PrgCfgOptSwChar,"-")) {
            if(debug) printf("\ndbg: \t\tline %d, check for a negative value = %s",  __LINE__, argv[i]);
            if( atof(argv[i]) != 0.0) {
              sprintf( tmps, "-%s ", argv[i]); // add '-' because PrgCfgOptSwChar has been removed in previous for( k=0; k< cfgmax; k++) ... loop
              if(debug) printf("\ndbg: \t\tline %d, tmps = '%s', argv[%d]=%s, strlen(argv[%d]=%lu)",  __LINE__, tmps, i, argv[i], i, strlen( argv[i]));
              if( ovwrfile == 0)
                strncat( PrgCfgUnassigned, tmps, strlen( tmps));
              if(debug) printf("\ndbg: \t\tline %d, PrgCfgUnassigned = '%s'",  __LINE__, PrgCfgUnassigned);
            }
          }
          else {
            printf("\n*** ERROR: unknown switch: %s", origargv);
            error = 1;
            break;
          }
	}
      }
      else {
	// everything else is concatenated to the string input val
	sprintf( tmps, "%s ", argv[i]);
	if( ovwrfile == 0)
	  strncat( PrgCfgUnassigned, tmps, strlen( argv[i])+1);
	if(debug) printf("\ndbg: \t\tline %d, PrgCfgUnassigned = '%s'",  __LINE__, PrgCfgUnassigned);//argv[i]);
      }
    } // for( i =1; i<argc; i++) ....

    for( i=0; i<argc; i++)         // copy stored argv back to argv
      strcpy( argv[i], argvcpy[i]);

    if( ovwrfile > 0)
      --ovwrfile;
  } while( ovwrfile != 0);


  for( i=0; i<argc; i++)
    free( argvcpy[i]);
  // */
  free( ind);
  free( ol);

  if( error)
    return 0;
  else
    return 1;
} // prgcfg_read_cmdline() -----------------------------------------------------




//------------------------------------------------------------------------------
// read parameter from configuration file
//------------------------------------------------------------------------------
int prgcfg_read_file( char *PrgCfgFileName, PrgCfgOpt *cfgval)
{
  FILE *PrgCfgFile; // pointer configuration file
  int i, k;
  int cnt=0;
  int eqsign=0;
  char line[PrgCfgFileLineMaxLength];
  char arg[PrgCfgOptMaxNameLength] = "";
  char val[PrgCfgOptMaxNameLength] = "";
  char tmps[PrgCfgOptMaxNameLength] = "";
  char seperationsign[3];
  int cfgmax=0;
  int *ind;	    // sorted option index
  int *ol;	    // sorted option length
  int onl;          // option name length;
  int pos;
  int matched =0;
  int debug = 0;
  int arrayindex[3];

  arrayindex[2] = arrayindex[1] = arrayindex[0] = 0;

#ifdef debug_options
  debug = 1;
#endif

  sprintf( seperationsign, "%c", PrgCfgSeparatorSign);

  if( (PrgCfgFile = fopen( PrgCfgFileName, "r")) ==0) {
    printf("\n*** failed to open file %s\n", PrgCfgFileName);
    return(0);
  }
  else {
    cfgmax = 0;
    while( cfgval[cfgmax].name) {
      if( debug) printf("\ndbg: in loop line : %d, cfgmax: %d", __LINE__, cfgmax);
      if( debug) printf("\ndbg: in loop line : %d, cfgmax: %d, %s", __LINE__, cfgmax, cfgval[cfgmax].name);
      cfgmax++;
    }

    // sort options, longest name first
    ind = malloc( (cfgmax+1) *sizeof(int)); // sorted option index
    ol  = malloc( (cfgmax+1) *sizeof(int)); // sorted option length

    if( debug) printf("\ndbg: line : %d, cfgmax: %d", __LINE__, cfgmax);

    for( i=0; i<cfgmax; i++) {              // init array
      ol[i] = strlen(cfgval[i].name);
      ind[i] = i;
    }

    for( i=1; i < cfgmax; ++i) {            // sort
      if( ol[i] > ol[i-1]) {
	onl = ol[i];
	pos = ind[i];
	k = i-1;
	while( ol[k] < onl) {
	  ind[k+1] = ind[k];
	  ol[k+1]  = ol[k--];
	  if(k<0) break;
	}
	ol[k+1] = onl;
	ind[k+1] = pos;
      }
    }

    if( debug) for( i=0; i<cfgmax; i++) printf("\ndbg: ind[%d] = %d, length:%2d, name: %s", i, ind[i], ol[i], cfgval[ind[i]].name);


    while( fgets( line, PrgCfgFileLineMaxLength, PrgCfgFile) ) { // read each line from config file
      ++cnt;
      trunc_comment( line , PrgCfgCommentSign);                  // stop reading after comment sign

      if( debug) printf("\n\ndbg: scanned cfg line: %s", line);
      i = match( line, seperationsign);
      //eqsign = gsub( seperationsign, " ", line, 0);  // remove '=' from line
      eqsign = gsub( seperationsign, " ", line, 1);  // remove first '=' from line
      sprintf( arg, "%s", substr( line, 0, i));	  // read variable name
      //i = sscanf( line, "%s", arg);		  // read variable name, old version
      if( strlen(arg))
	i=1;
      else
	i=0;

      if( debug) printf("\ndbg: line: '%s', arg: '%s', i=%d", line, arg, i);
      i = i + gsub( arg, "", line, 0);		  // remove arg from line
      if( debug) printf("\ndbg: line: '%s', i=%d, strlen(line)=%d", line, i, (int)strlen(line));
      if( strlen(line) == 0)                      // skip empty lines
	i = 0;

      k = 0;					  // remove leading spaces from value
      strcpy( val, line);
      while( (val[0] == ' ') || (val[0] == '\t')) {
	sprintf( val, "%s", substr( line, k++, 0));
      }
      if( debug) printf("\ndbg: line: '%s', val: '%s'", line, val);
      k = strlen( val) -1;        		  // remove trailing spaces from value
      strcpy( line, val);
      if( debug) printf("\ndbg: line: '%s', val: '%s'", line, val);
      while( (val[k] == ' ') || (val[k] == '\t')) {
	sprintf( val, "%s", substr( line, 0, k--));
      }
      if( debug) printf("\ndbg: line: '%s', val: '%s'", line, val);
      if( debug) printf("\ndbg: variable: '%s', vlaue: '%s'", arg, val);
      matched =0;
      if( i>1 && eqsign ==1) {                                // if read of "variable = value"
	if( debug) printf("\ndbg: %s = %s", arg, val);        // successful, evaluate expression
	for( i =0; i<cfgmax; i++) {
	  if( debug) printf("\ndbg: check option %d %s = %s", ind[i], cfgval[ind[i]].name, arg);
	  if( match( arg, cfgval[ind[i]].name)) {
	    matched =1;
	    if( debug) printf("\ndbg: match found: %s = cfgval[%d] --> %s = %s", arg, ind[i], cfgval[ind[i]].name, val);
	    // check for array
	    if( match(arg,"[")) {
	      prgcfgetarryindex( arg, arrayindex);
	      if( debug) printf("\ndbg: found array type, -->index = %d", arrayindex[0]);
	    }
	    if( prgcfgvarassign( cfgval, ind[i], val, arrayindex, 0) == 0) {  // assign value to cfgval
	      printf("\n*** ERROR: in file: '%s' line %d", PrgCfgFileName, cnt);
	      free( ind);
	      free( ol);
	      return(0);
	    }
	    break;
	  }
	}
	// WARNING for unknown variable
	if( matched == 0) {
	  k = strlen( arg) -1;        		  // remove trailing spaces from arg
	  strcpy( tmps, arg);
	  while( (tmps[k] == ' ') || (tmps[k] == '\t')) {
	    sprintf( tmps, "%s", substr( arg, 0, k--));
	  }
	  printf("\n*** WARNING: unknown variable '%s' in file '%s' line %d, will be ignored.\n", tmps, PrgCfgFileName, cnt);
	}
      }
      else if( i>0) {                       // error if read only one expression
	printf("\n*** ERROR: invalid expression in file '%s' line %d, missing '%s'", PrgCfgFileName, cnt, seperationsign);
	free( ind);
	free( ol);
	return(0);
      }
    }// while()
    free( ind);
    free( ol);
    fclose( PrgCfgFile);
  }

  return(1);
}// prgcfg_read_file() ---------------------------------------------------------


//------------------------------------------------------------------------------
// prgcfg_cmdl + prgcfg_file in one function
//------------------------------------------------------------------------------
int prgcfg_read_cmdl( int argc, char *argv[], PrgCfgOpt *cmdl_cfgval)
{
  int commandlineOK =1;

  commandlineOK = prgcfg_read_cmdline( argc, argv, cmdl_cfgval, 0);
  if( !commandlineOK) {
    printf("\n*** ERROR %d, error in command line\n", __LINE__);
  }

  return( commandlineOK);
}// prgcfg_read_cmdl() ----------------------------------------------------------


///------------------------------------------------------------------------------
// prgcfg_cmdl + prgcfg_file in one function
//------------------------------------------------------------------------------
int prgcfg_read( int argc, char *argv[], PrgCfgOpt *cmdl_cfgval, char *PrgCfgFileName, PrgCfgOpt *file_cfgval)
{
  int commandlineOK =1;
  int fileconfigOK  =1;

  commandlineOK = prgcfg_read_cmdline( argc, argv, cmdl_cfgval, 0);

  if( commandlineOK ) {
    // read config files
    if( strlen( PrgCfgFileName))           // config filename required
      fileconfigOK = commandlineOK & prgcfg_read_file( PrgCfgFileName, file_cfgval);

    // read command line again to eventually overwrite configuration from file
    if( fileconfigOK) {
      commandlineOK = prgcfg_read_cmdline( argc, argv, cmdl_cfgval, 1);
    }
  }

  if( !commandlineOK) {
    printf("\n*** ERROR %d, error in command line\n", __LINE__);
  }
  if( !fileconfigOK) {
    printf("\n*** ERROR %d, error in configuration file\n", __LINE__);
  }

  return( commandlineOK & fileconfigOK);
}// prgcfg_read() --------------------------------------------------------------


//------------------------------------------------------------------------------
// get modification status for variable
//------------------------------------------------------------------------------
int prgcfgvstat( PrgCfgOpt *cfgval, void *var)
{
  int i, k;
  int stat;
  int length  = 1;
  int opt_cnt;
  int debug = 0;
  long var2;
#ifdef debug_options
  debug = 1;
#endif
  stat = 0;
  opt_cnt = 0;

  var2 = (long)var;
  // count number of options
  while( cfgval[opt_cnt].name) {
    opt_cnt++;
  }

  if( debug) printf("\naddr: %ld\n", var2);

  // find max string length
  for( i=0; i< opt_cnt; ++i) {
    if( strlen( cfgval[i].name) > length)
      length = strlen( cfgval[i].name);
  }

  // count number of modifications vor var
  for( i=0; i< opt_cnt; ++i) {
    if( var2 == (long)cfgval[i].value)
      stat += (int)cfgval[i].used;

    if( debug) {
      printf(" %s%s", PrgCfgOptSwChar, cfgval[i].name);
      for( k=strlen( cfgval[i].name); k<length; k++)
	printf(" ");
        printf(" : addr: %ld, %s\n",  (long)cfgval[i].value, cfgval[i].help);
    }
  }
  if( debug)  printf("\n");

  return( stat);
}// prgcfgvstat() --------------------------------------------------------------


//------------------------------------------------------------------------------
// print a log message to to file and to stderr if verbose=1
//------------------------------------------------------------------------------
void prglogmsg( bool verbose, FILE *prglgfp, const char *logmsg, ...)
{
  va_list ap;
  char logstring[PrgCfgUsageTextLength];

  // extern char *program_invocation_short_name;
  // fprintf( stderr, "%s: ", program_invocation_short_name);
  va_start( ap, logmsg);
  vsprintf( logstring, logmsg, ap);
  va_end( ap);
  if( verbose)
    fprintf( stderr, logstring);
  fprintf( prglgfp, logstring);

}// prglogmsg() ----------------------------------------------------------------

//------------------------------------------------------------------------------
// print a log message header to file and to stderr if verbose=1
//------------------------------------------------------------------------------
void prglogheader( bool verbose, FILE *prglgfp, const char *logmsg, ...)
{
  char logstring[PrgCfgUsageTextLength];
  struct tm *ptr;
  time_t tm;
  tm = time( NULL);
  ptr = localtime( &tm);

  va_list ap;
  va_start( ap, logmsg);
  vsprintf( logstring, logmsg, ap);
  va_end( ap);

  if( verbose) {
    fprintf( stderr, "\n------------------------------------------------------------------------------");
    fprintf( stderr, "\n Date    : %s", asctime( ptr));
    fprintf( stderr, " Command : %s", PrgCfgPrgCall);
    fprintf( stderr, logstring);
    fprintf( stderr, "\n------------------------------------------------------------------------------\n");
  }

  fprintf( prglgfp, "\n%s------------------------------------------------------------------------------", (PrgCfgLogHdCommentOn==1)? PrgCfgCommentSign : "");
  fprintf( prglgfp, "\n%s Date    : %s", (PrgCfgLogHdCommentOn==1)? PrgCfgCommentSign : "", asctime( ptr));
  fprintf( prglgfp, "%s Command : %s", (PrgCfgLogHdCommentOn==1)? PrgCfgCommentSign : "", PrgCfgPrgCall);
  fprintf( prglgfp, logstring);
  //prgcfgprint( cfgval); // does not work anymore :-(((
  fprintf( prglgfp, "\n%s------------------------------------------------------------------------------\n", (PrgCfgLogHdCommentOn==1)? PrgCfgCommentSign : "");

}// prglogmsg() ----------------------------------------------------------------

// EOF
