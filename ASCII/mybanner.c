//------------------------------------------------------------------------------
// description:
// ------------
//      _     ____    ____  ___  ___     _
//     / \   / ___|  / ___||_ _||_ _|   | |__    __ _  _ __   _ __    ___  _ __
//    / _ \  \___ \ | |     | |  | |    | '_ \  / _` || '_ \ | '_ \  / _ \| '__|
//   / ___ \  ___) || |___  | |  | |    | |_) || (_| || | | || | | ||  __/| |
//  /_/   \_\|____/  \____||___||___|   |_.__/  \__,_||_| |_||_| |_| \___||_|
//
//
// usage: mybanner [-option [<value>]] [text]
//
// Command line options:
//  -h         Display this help.
//  -x         -x <n>, extend character width by n spaces, default: 0
//  -mc        -mc <n>, max. size of character in % after compression (for negative x values),
//             default: 60%
//  -f         -f <font file name>, set file name containing banner letter for each ascii
//             char
//  -c         -c <comment string>, if specified the given comment string will be added
//             to each line start
//  -ce        -ce <comment end string>, if specified the given string will be added to
//             each line end
//
// For generating a font template file set the following:
//  -g         -g generates a template font file (print to stdio)
//  -p         -p prints the default font to stdio
//  -l         -l <n>, number of lines reserved for each letter
//
//
//
// Variables for font file:
//  lines     lines = <l> number of lines per letter
//  let       let[n][m] = <string>; defines font for ascii char n line m, e.g. for space
//           (0x20): let[32][0]="    ";
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//#include <unistd.h>

#include "../LIB/mystrings.h"
#include "../LIB/prgcfg.h"
#include "../LIB/keys.h"

#define cfgstrlen       50                /* string length for configuration file        */
#define nFPGAmax        10                /* max. number of FPGAs on board               */
#define n_cnbr          nFPGAmax*nFPGAmax /* number of connections (bus) between FPGAs   */
#define n_busmembers    1000              /* pin count for bus connections between FPGAs */

//-----------------------------------------------------------------------------
// variables set from command line
//-----------------------------------------------------------------------------
bool   help             = 0;    // commandline options help
bool   cfgfhelp         = 0;    // config file options help
char   FileName[1000]   = "";
char   FontFile[1000]   = "";
bool   gentemplate      = 0;
bool   prnfnt           = 0;
bool   debug            = 0;

#define n_lines      20
#define n_letter    256
#define n_lines_def   6

char  *let[ n_letter][n_lines];  // array of strings for n_letter each n_lines number of lines
char **letp[n_letter];           // pointer to let
char  *dsa[ n_letter];           // decrease space between character after listed previous charcters

int    nlet      = 256;
int    nlines    = n_lines_def;
int    extend    = 0;
double maxcompr  = 60; // 50 % max. letter compr.
char cmnt[100]  = "\0";  // comment for output
char cmnte[100] = "\0";  // comment for output at end of line

//------------------------------------------------------------------------------
// global variables but not configurable via command line or file
//------------------------------------------------------------------------------
char comment[5]       = "//"; // comment sign for reading configuration from files
char separator        = '=';  // separator between option and value when reading config file
char boldon[]         = "\033[1m"; // bold print on
char grey[]           = "\033[37m"; // grey print on
char boldoff[]        = "\033[0m";  // bold print off

//------------------------------------------------------------------------------
// function declaration
//------------------------------------------------------------------------------
void variableinit( int set);       // init variables (set=1), free allocated memory (set=0)
void init_letter( void);
void call_help( int select);       // put out some special text for and call help function
void print_banner( void);          // print banner character after character
void print_template( void);
void print_font( void);
int rndup( double a);

//------------------------------------------------------------------------------
// Configuration Options
// set line break with PrgCfgAutoLineBreak = 75;
//------------------------------------------------------------------------------
bool todo = 0;
bool dummy = 0;
static PrgCfgOpt cfg_opt[]={
  // name         type       variable dest.     used help
  { "_noprint_",  boolt,     &dummy,            0,   "\n\033[1mCommand line options:\033[0m"},
  { "h",          boolt,     &help,             0,   "Display this help."},
  { "dbg",        boolt,     &debug,           -1,   "print debug info (-1 in used column prevents printing help for this option)"},
  { "x",          intt,      &extend,           0,   "-x <n>, extend character width by n spaces, default: 0"},
  { "mc",         doublet,   &maxcompr,         0,   "-mc <n>, max. size of character in % after compression (for negative x values), default: 60%"},
  { "f",          stringt,   &FontFile,         0,   "-f <font file name>, set file name containing banner letter for each ascii char"},
  { "c",          stringt,   &cmnt,             0,   "-c <comment string>, if specified the given comment string will be added to each line start"},
  { "ce",         stringt,   &cmnte,            0,   "-ce <comment end string>, if specified the given string will be added to each line end"},
  { "_noprint_",  boolt,     &dummy,            0,   "\nFor generating a font template file set the following:"},
  { "g",          boolt,     &gentemplate,      0,   "-g generates a template font file (print to stdio)"},
  { "p",          boolt,     &prnfnt,           0,   "-p prints the default font to stdio"},
  { "l",          intt,      &nlines,           0,   "-l <n>, number of lines reserved for each letter"},
  { NULL}   // delimiter for automatic size detection of cfg_opt[]
};

static PrgCfgOpt fcfg_opt[]={
  // name         type       variable dest.     used help
  { "_noprint_",  boolt,     &dummy,            0,   "\n\033[1mVariables for font file:\033[0m"},
  { "lines",      intt,      &nlines,           0,   "lines = <l> number of lines per letter"},
  { "let",        stringa2t, &letp,             0,   "let[n][m] = <string>; defines font for ascii char n, line m, e.g. for space (0x20): let[32][0]=\"    \";"},
  { "dsa",        stringat,  &dsa,              0,   "dsa[n] = <string>; decrease space after listed previous charcters for let[n][0..m];"},
  { NULL}   // delimiter for automatic size detection of cfg_opt[]
};



//------------------------------------------------------------------------------
// MAIN
//  - init variables
//  - read commandline and config file
//  - call sub functions
//  - exit
//------------------------------------------------------------------------------
int main( int argc, char **argv)
{
  int commandlineOK = 1;
  int fileconfigOK  = 1;

  variableinit(1);

  commandlineOK = prgcfg_read_cmdline( argc, argv, cfg_opt, 0);
  strcpy( PrgCfgCommentSign, comment);   // comment sign for reading configuration from files
  PrgCfgSeparatorSign = separator;       // separator between option and value when reading config file

  if( commandlineOK ) {
    //--------------------------------------------------------------------------
    // read config files
    //--------------------------------------------------------------------------
    if( strlen( FontFile)) // there has to be set a filename
      fileconfigOK = fileconfigOK & prgcfg_read_file( FontFile, fcfg_opt);

    //--------------------------------------------------------------------------
    // read command line to overwrite configuration from file
    //--------------------------------------------------------------------------
    /* if( fileconfigOK)
      commandlineOK = prgcfg_cmdl( argc, argv, cfg_opt, 1);
    */

  }

  if( !commandlineOK) {
    printf("\n*** ERROR %d, error in command line\n", __LINE__);
  }
  if( !fileconfigOK && !help) {
    printf("\n*** ERROR %d, error in font file\n", __LINE__);
  }

  //--------------------------------------------------------------------------
  // intended program execution
  //--------------------------------------------------------------------------
  // display program help
  if(help == true) {
    call_help( 0); // command line
    call_help( 1); // file
  }

  else if( fileconfigOK && commandlineOK) {
    if( gentemplate)
      print_template();
    else if( prnfnt)
      print_font();
    else
      print_banner();
  }
  //prgcfgprint( cfg_opt);

  variableinit( 0);
  return 0;
}// main() ---------------------------------------------------------------------


//------------------------------------------------------------------------------
// print character template
//------------------------------------------------------------------------------
void print_template( void) {
  int i, k;
  printf("\nlines = %d; // %d lines for each character", nlines, nlines);
  // print character
  for( i = 32; i<nlet; i++) {
    if( (i > 126) && (i<160))
      continue;
    else {
      printf("\n");

      for( k=0; k< nlines; k++) {
        printf("\nlet[%3d][%2d] = \"  \"; ", i, k);
        if( k== 0)
          printf("// ascii char '%c'", i);
      }
        printf("\ndsa[%3d]     = \"\"; // decrease space between character after listed previous charcters", i);
    }
  }
  printf("\n");
}// print_template() -----------------------------------------------------------


//------------------------------------------------------------------------------
// print current font
//------------------------------------------------------------------------------
void print_font( void) {
  int i, k;
  printf("\nlines = %d; // %d lines for each character", nlines, nlines);
  // print character
  for( i = 32; i<nlet; i++) {
    if( (i > 126) && (i<160))
      continue;
    else {
      printf("\n");

      for( k=0; k< nlines; k++) {
        printf("\nlet[%3d][%2d] = \"%s\"; ", i, k, let[i][k]);
        if( k== 0)
          printf("// ascii char '%c'", i);
      }
    }
  }
  printf("\n");
}// print_template() -----------------------------------------------------------

//------------------------------------------------------------------------------
// print banner character after character
//------------------------------------------------------------------------------
void print_banner( void) {

  int i, k, l;
  char text[300] = "test";
  char c;
  char tmps[100];
  int length;

  char ochar[100]; // old character
  int osl;         // old string length;
  int cosl;        // compressed old string length
  int ontop;       // flag for current character in case of character overlapping
  int txtlength;
  int char_compress;
  maxcompr = maxcompr/100;

  //-------------------
  // get banner text
  //-------------------
  if( strlen( PrgCfgUnassigned))        // banner text as parameter in program call
    strcpy( text, PrgCfgUnassigned);

  else {                                // banner text from pipe
    strcpy( text, "");

    set_keypress();
    getstring( text, 0);
    reset_keypress();
    txtlength = strlen( text);
    printf("\033[%dD", txtlength); // go to beginning of line
    for( i=0; i< txtlength; i++)    // clear line
      printf(" ");
    printf("\033[%dD", txtlength); // go back to beginning of line
  }


  //-------------------
  // start banner print
  //-------------------
  if( text[strlen( text)-1] == ' ')
    text[strlen( text)-1] ='\0';
  //printf("'%s'\n", text);

  // print character
  for( i = 0; i<nlines; i++) {
    if( strlen( cmnt))
      printf("%s ", cmnt);  // add comment sign at start of each line

    for( k=0; k< strlen( text); k++) {
      c = text[k];
      char_compress = extend;
      /*
      // TODO: methodology to detect sequences of characters where
      // variable extend needs to be decremented temporary by 1, see example
      // below:
      // myb -x 0 TeTvTwA
      //  _____        _____         _____               _
      // |_   _|  ___ |_   _|__   __|_   _|__      __   / \
      //   | |   / _ \  | |  \ \ / /  | |  \ \ /\ / /  / _ \
      //   | |  |  __/  | |   \ V /   | |   \ V  V /  / ___ \
      //   |_|   \___|  |_|    \_/    |_|    \_/\_/  /_/   \_\
      //
      // myb -x -1 TeTvTwA
      //  _____      _____       _____             _
      // |_   _| ___|_   ___   _|_   ___      __  / \
      //   | |  / _ \ | | \ \ / / | | \ \ /\ / / / _ \
      //   | | |  __/ | |  \ V /  | |  \ V  V / / ___ \
      //   |_|  \___| |_|   \_/   |_|   \_/\_/ /_/   \_\

      // ==> '|' has to overwrites '_'
      //  _____      _____       _____             _
      // |_   _| ___|_   _|_   _|_   _|_      __  / \
      //   | |  / _ \ | | \ \ / / | | \ \ /\ / / / _ \
      //   | | |  __/ | |  \ V /  | |  \ V  V / / ___ \
      //   |_|  \___| |_|   \_/   |_|   \_/\_/ /_/   \_\
      */

      // compress character spaces?
      if( char_compress < 0) {
        strcpy( tmps, let[(int)c][i]);
        // all characters but last one
        // compress letters max. (maxcompr*100)%
        length = strlen( tmps);

        if( k == strlen( text) -1)
          ; // print last letter uncompressed
        else if( (double)(length +char_compress) < (maxcompr * (double)length)){
          length = rndup( ((double)length) * maxcompr);
        }
        else
          length = length + char_compress;

        ontop = 0;
        // loop for each character in current letter
        for( l=0; l<length; l++) {

          if( k==0 )  // first character does not overlap
            printf("%c", tmps[l]);
          else {
            if( (tmps[l] == ' ') && (ontop == 0) ) { // if space in letter print previous letter

              // get printed length of previous characer
              strcpy( ochar, let[(int)text[k-1]][i]);
              osl = strlen( ochar);
              if( (double)(osl +char_compress) <= (maxcompr * (double)osl)){
                cosl = rndup( ((double)osl) * maxcompr);
              }
              else
                cosl  = osl +char_compress;

	      // print character from previous letter
              if( cosl+l < osl ) {
		if( debug) printf("%s%d<otop:%d, cosl=%d>%s", grey, __LINE__, ontop, cosl, boldoff);
                printf("%c", ochar[cosl+l]);
	      }
              else {
		if( debug) printf("%s%d<otop:%d, cosl=%d>%s", grey, __LINE__, ontop, cosl, boldoff);
                printf("%c", tmps[l]);
                ontop = 1;
              }
            }
	    // else if( (tmps[l] == '_') && (ontop == 0) && (ontop == 0)) {
	    // }
            else {
	      if( debug) printf("%s%d<otop:%d, cosl=%d>%s", grey, __LINE__, ontop, cosl, boldoff);
              printf("%c", tmps[l]);
              ontop = 1;
            }
          }
        }
      }
      else {
        printf("%s", let[(int)c][i]);

        // extend character width
        for( l=0; l<extend; l++)
          printf(" ");
      }
    }
    if( strlen( cmnte))
      printf("%s ", cmnte);  // add comment sign at end of each line

    printf("\n");
  }

}// print_banner() -------------------------------------------------------------



//------------------------------------------------------------------------------
// init variables (set=1), free allocated memory (set=0)
//------------------------------------------------------------------------------
void variableinit( int set)
{
  int i, k;
  if( set == 1) {
    for( i=0; i<n_letter; i++) {
      dsa[i] = (char *) malloc( cfgstrlen * sizeof( char));   // decrease space between character after listed previous charcters
      for( k=0; k<n_lines; k++) {
        let[i][k] = (char *) malloc( cfgstrlen * sizeof( char));
        strcpy( let[i][k], " "); // init lines
      }
      letp[i] = &let[i][0];     // init pointer to list of letters/characters
    }
    init_letter();
  }

  else {
    for( i=0; i<n_letter; i++) {
      free( dsa[i]);
      for( k=0; k<n_lines; k++) {
        free( let[i][k]);
      }
    }
  }

}// variableinit() -------------------------------------------------------------


//------------------------------------------------------------------------------
// some special text definition for help display
//------------------------------------------------------------------------------
void call_help( int select)
{
  PrgCfgAutoLineBreak = 80;

  // command line help
  if( select == 0) {
    sprintf( PrgCfgUsage, "usage: %s [-option [<value>]] [text]", PrgCfgPrgName); // overwrite "usage: programname [-option [<value>]]"
    prgcfghelp( cfg_opt, 1);   // 1 switches on printing "-" before option name

    // print command line options with their current values, status and help text
    // prgcfgprint( cfg_opt);
  }

  // file config help
  else if( select == 1) {
    sprintf( PrgCfgUsage," "); // overwrite "usage: programname [-option [<value>]]"
    prgcfghelp( fcfg_opt, 0);

    // print command line options with their current values, status and help text
    // prgcfgprint( fcfg_opt);
  }

}// set_help_text() ------------------------------------------------------------



//------------------------------------------------------------------------------
// roundup to integer
//------------------------------------------------------------------------------
int rndup( double a) {
  //printf("\na=%d rndup(a)=%d", (int)a, (int)ceil( a));
  if( (int)a<0)
    return(0);
  else
    return( (int)ceil( a));
}// rndup() --------------------------------------------------------------------
// EOF


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void init_letter( void) {
  int i, k;

  for( i=0; i<n_letter; i++) {
    sprintf( dsa[i], ""); // decrease space between character and listed charcters
    for( k=0; k<n_lines; k++) {
      sprintf( let[i][k], "## %3d line %2d undefined ##", i, k);
    }
  }


  strcpy( let[32][0] , "   ");
  strcpy( let[32][1] , "   ");
  strcpy( let[32][2] , "   ");
  strcpy( let[32][3] , "   ");
  strcpy( let[32][4] , "   ");
  strcpy( let[32][5] , "   ");

  strcpy( let[33][0] , " _ ");
  strcpy( let[33][1] , "| |");
  strcpy( let[33][2] , "| |");
  strcpy( let[33][3] , "|_|");
  strcpy( let[33][4] , "(_)");
  strcpy( let[33][5] , "   ");

  strcpy( let[34][0] , " _  _ ");
  strcpy( let[34][1] , "| || |");
  strcpy( let[34][2] , "|_||_|");
  strcpy( let[34][3] , "      ");
  strcpy( let[34][4] , "      ");
  strcpy( let[34][5] , "      ");

  strcpy( let[35][0] , "   _  _   ");
  strcpy( let[35][1] , " _| || |_ ");
  strcpy( let[35][2] , "|_      _|");
  strcpy( let[35][3] , " _| || |_ ");
  strcpy( let[35][4] , "|_      _|");
  strcpy( let[35][5] , "  |_||_|  ");

  // strcpy( let[36][0] , "  _+-+_  ");
  // strcpy( let[36][1] , " /    _| ");
  // strcpy( let[36][2] , "( (| |_  ");
  // strcpy( let[36][3] , " \\_    \\ ");
  // strcpy( let[36][4] , "  _| |) )");
  // strcpy( let[36][5] , " |_._._/ ");

  strcpy( let[36][0] , " _+-+_ ");
  strcpy( let[36][1] , "/ . ._|");
  strcpy( let[36][2] , "\\_. . \\");
  strcpy( let[36][3] , "|_. ._/");
  strcpy( let[36][4] , "  +-+  ");
  strcpy( let[36][5] , "       ");

  strcpy( let[37][0] , " _  /\\ ");
  strcpy( let[37][1] , "(_)/ / ");
  strcpy( let[37][2] , "  / /_ ");
  strcpy( let[37][3] , " / /(_)");
  strcpy( let[37][4] , " \\/    ");
  strcpy( let[37][5] , "       ");

  strcpy( let[38][0] , "  ___   ");
  strcpy( let[38][1] , " ( _ )  ");
  strcpy( let[38][2] , " / _ \\/\\");
  strcpy( let[38][3] , "| (_)  <");
  strcpy( let[38][4] , " \\___/\\/");
  strcpy( let[38][5] , "        ");

  strcpy( let[40][0] , "  __");
  strcpy( let[40][1] , " / /");
  strcpy( let[40][2] , "| | ");
  strcpy( let[40][3] , "| | ");
  strcpy( let[40][4] , " \\_\\");
  strcpy( let[40][5] , "    ");

  strcpy( let[41][0] , "__  ");
  strcpy( let[41][1] , "\\ \\ ");
  strcpy( let[41][2] , " | |");
  strcpy( let[41][3] , " | |");
  strcpy( let[41][4] , "/_/ ");
  strcpy( let[41][5] , "    ");

  strcpy( let[42][0] , "     ");
  strcpy( let[42][1] , "\\ | /");
  strcpy( let[42][2] , "-(_)-");
  strcpy( let[42][3] , "/ | \\");
  strcpy( let[42][4] , "     ");
  strcpy( let[42][5] , "     ");

  strcpy( let[43][0] , "       ");
  strcpy( let[43][1] , "   _   ");
  strcpy( let[43][2] , " _| |_ ");
  strcpy( let[43][3] , "|_   _|");
  strcpy( let[43][4] , "  |_|  ");
  strcpy( let[43][5] , "       ");

  // strcpy( let[44][0] , "    ");
  // strcpy( let[44][1] , "    ");
  // strcpy( let[44][2] , "    ");
  // strcpy( let[44][3] , "  _ ");
  // strcpy( let[44][4] , " ( )");
  // strcpy( let[44][5] , " |/ ");

  strcpy( let[44][0] , "    ");
  strcpy( let[44][1] , "    ");
  strcpy( let[44][2] , "    ");
  strcpy( let[44][3] , "  _ ");
  strcpy( let[44][4] , " ( )");
  strcpy( let[44][5] , "  V ");

  strcpy( let[45][0] , "       ");
  strcpy( let[45][1] , "       ");
  strcpy( let[45][2] , " _____ ");
  strcpy( let[45][3] , "|_____|");
  strcpy( let[45][4] , "       ");
  strcpy( let[45][5] , "       ");

  strcpy( let[46][0] , "   ");
  strcpy( let[46][1] , "   ");
  strcpy( let[46][2] , "   ");
  strcpy( let[46][3] , " _ ");
  strcpy( let[46][4] , "(_)");
  strcpy( let[46][5] , "   ");

  strcpy( let[47][0] , "    __");
  strcpy( let[47][1] , "   / /");
  strcpy( let[47][2] , "  / / ");
  strcpy( let[47][3] , " / /  ");
  strcpy( let[47][4] , "/_/   ");
  strcpy( let[47][5] , "      ");

  strcpy( let[48][0] , "  ___  ");
  strcpy( let[48][1] , " / _ \\ ");
  strcpy( let[48][2] , "| | | |");
  strcpy( let[48][3] , "| |_| |");
  strcpy( let[48][4] , " \\___/ ");
  strcpy( let[48][5] , "       ");

  strcpy( let[49][0] , " _ ");
  strcpy( let[49][1] , "/ |");
  strcpy( let[49][2] , "| |");
  strcpy( let[49][3] , "| |");
  strcpy( let[49][4] , "|_|");
  strcpy( let[49][5] , "   ");

  strcpy( let[50][0] , " ____  ");
  strcpy( let[50][1] , "|___ \\ ");
  strcpy( let[50][2] , "  __) |");
  strcpy( let[50][3] , " / __/ ");
  strcpy( let[50][4] , "|_____|");
  strcpy( let[50][5] , "       ");

  strcpy( let[51][0] , " _____ ");
  strcpy( let[51][1] , "|___ / ");
  strcpy( let[51][2] , "  |_ \\ ");
  strcpy( let[51][3] , " ___) |");
  strcpy( let[51][4] , "|____/ ");
  strcpy( let[51][5] , "       ");

  strcpy( let[52][0] , " _  _   ");
  strcpy( let[52][1] , "| || |  ");
  strcpy( let[52][2] , "| || |_ ");
  strcpy( let[52][3] , "|__   _|");
  strcpy( let[52][4] , "   |_|  ");
  strcpy( let[52][5] , "        ");

  strcpy( let[53][0] , " ____  ");
  strcpy( let[53][1] , "| ___| ");
  strcpy( let[53][2] , "|___ \\ ");
  strcpy( let[53][3] , " ___) |");
  strcpy( let[53][4] , "|____/ ");
  strcpy( let[53][5] , "       ");

  strcpy( let[54][0] , "  __   ");
  strcpy( let[54][1] , " / /_  ");
  strcpy( let[54][2] , "| '_ \\ ");
  strcpy( let[54][3] , "| (_) |");
  strcpy( let[54][4] , " \\___/ ");
  strcpy( let[54][5] , "       ");

  strcpy( let[55][0] , " _____ ");
  strcpy( let[55][1] , "|___  |");
  strcpy( let[55][2] , "   / / ");
  strcpy( let[55][3] , "  / /  ");
  strcpy( let[55][4] , " /_/   ");
  strcpy( let[55][5] , "       ");

  strcpy( let[56][0] , "  ___  ");
  strcpy( let[56][1] , " ( _ ) ");
  strcpy( let[56][2] , " / _ \\ ");
  strcpy( let[56][3] , "| (_) |");
  strcpy( let[56][4] , " \\___/ ");
  strcpy( let[56][5] , "       ");

  strcpy( let[57][0] , "  ___  ");
  strcpy( let[57][1] , " / _ \\ ");
  strcpy( let[57][2] , "| (_) |");
  strcpy( let[57][3] , " \\__, |");
  strcpy( let[57][4] , "   /_/ ");
  strcpy( let[57][5] , "       ");

  strcpy( let[58][0] , "   ");
  strcpy( let[58][1] , " _ ");
  strcpy( let[58][2] , "(_)");
  strcpy( let[58][3] , " _ ");
  strcpy( let[58][4] , "(_)");
  strcpy( let[58][5] , "   ");

  // strcpy( let[59][0] , "    ");
  // strcpy( let[59][1] , "  _ ");
  // strcpy( let[59][2] , " (_)");
  // strcpy( let[59][3] , "  _ ");
  // strcpy( let[59][4] , " ( )");
  // strcpy( let[59][5] , " |/ ");

  strcpy( let[59][0] , "    ");
  strcpy( let[59][1] , "  _ ");
  strcpy( let[59][2] , " (_)");
  strcpy( let[59][3] , "  _ ");
  strcpy( let[59][4] , " ( )");
  strcpy( let[59][5] , "  V ");

  strcpy( let[60][0] , "  __");
  strcpy( let[60][1] , " / /");
  strcpy( let[60][2] , "/ / ");
  strcpy( let[60][3] , "\\ \\ ");
  strcpy( let[60][4] , " \\_\\");
  strcpy( let[60][5] , "    ");

  strcpy( let[61][0] , "       ");
  strcpy( let[61][1] , " _____ ");
  strcpy( let[61][2] , "|_____|");
  strcpy( let[61][3] , "|_____|");
  strcpy( let[61][4] , "       ");
  strcpy( let[61][5] , "       ");

  strcpy( let[62][0] , "__  ");
  strcpy( let[62][1] , "\\ \\ ");
  strcpy( let[62][2] , " \\ \\");
  strcpy( let[62][3] , " / /");
  strcpy( let[62][4] , "/_/ ");
  strcpy( let[62][5] , "    ");

  strcpy( let[63][0] , " ___ ");
  strcpy( let[63][1] , "|__ \\");
  strcpy( let[63][2] , "  / /");
  strcpy( let[63][3] , " |_| ");
  strcpy( let[63][4] , " (_) ");
  strcpy( let[63][5] , "     ");

  strcpy( let[65][0] , "    _    ");
  strcpy( let[65][1] , "   / \\   ");
  strcpy( let[65][2] , "  / _ \\  ");
  strcpy( let[65][3] , " / ___ \\ ");
  strcpy( let[65][4] , "/_/   \\_\\");
  strcpy( let[65][5] , "         ");

  strcpy( let[66][0] , " ____  ");
  strcpy( let[66][1] , "| __ ) ");
  strcpy( let[66][2] , "|  _ \\ ");
  strcpy( let[66][3] , "| |_) |");
  strcpy( let[66][4] , "|____/ ");
  strcpy( let[66][5] , "       ");

  strcpy( let[67][0] , "  ____ ");
  strcpy( let[67][1] , " / ___|");
  strcpy( let[67][2] , "| |    ");
  strcpy( let[67][3] , "| |___ ");
  strcpy( let[67][4] , " \\____|");
  strcpy( let[67][5] , "       ");

  strcpy( let[68][0] , " ____  ");
  strcpy( let[68][1] , "|  _ \\ ");
  strcpy( let[68][2] , "| | | |");
  strcpy( let[68][3] , "| |_| |");
  strcpy( let[68][4] , "|____/ ");
  strcpy( let[68][5] , "       ");

  strcpy( let[69][0] , " _____ ");
  strcpy( let[69][1] , "| ____|");
  strcpy( let[69][2] , "|  _|  ");
  strcpy( let[69][3] , "| |___ ");
  strcpy( let[69][4] , "|_____|");
  strcpy( let[69][5] , "       ");

  strcpy( let[70][0] , " _____ ");
  strcpy( let[70][1] , "|  ___|");
  strcpy( let[70][2] , "| |_   ");
  strcpy( let[70][3] , "|  _|  ");
  strcpy( let[70][4] , "|_|    ");
  strcpy( let[70][5] , "       ");

  strcpy( let[71][0] , "  ____ ");
  strcpy( let[71][1] , " / ___|");
  strcpy( let[71][2] , "| |  _ ");
  strcpy( let[71][3] , "| |_| |");
  strcpy( let[71][4] , " \\____|");
  strcpy( let[71][5] , "       ");

  strcpy( let[72][0] , " _   _ ");
  strcpy( let[72][1] , "| | | |");
  strcpy( let[72][2] , "| |_| |");
  strcpy( let[72][3] , "|  _  |");
  strcpy( let[72][4] , "|_| |_|");
  strcpy( let[72][5] , "       ");

  strcpy( let[73][0] , " ___ ");
  strcpy( let[73][1] , "|_ _|");
  strcpy( let[73][2] , " | | ");
  strcpy( let[73][3] , " | | ");
  strcpy( let[73][4] , "|___|");
  strcpy( let[73][5] , "     ");

  strcpy( let[74][0] , "     _ ");
  strcpy( let[74][1] , "    | |");
  strcpy( let[74][2] , " _  | |");
  strcpy( let[74][3] , "| |_| |");
  strcpy( let[74][4] , " \\___/ ");
  strcpy( let[74][5] , "       ");

  strcpy( let[75][0] , " _  __");
  strcpy( let[75][1] , "| |/ /");
  strcpy( let[75][2] , "| ' / ");
  strcpy( let[75][3] , "| . \\ ");
  strcpy( let[75][4] , "|_|\\_\\");
  strcpy( let[75][5] , "      ");

  strcpy( let[76][0] , " _     ");
  strcpy( let[76][1] , "| |    ");
  strcpy( let[76][2] , "| |    ");
  strcpy( let[76][3] , "| |___ ");
  strcpy( let[76][4] , "|_____|");
  strcpy( let[76][5] , "       ");

  strcpy( let[77][0] , " __  __ ");
  strcpy( let[77][1] , "|  \\/  |");
  strcpy( let[77][2] , "| |\\/| |");
  strcpy( let[77][3] , "| |  | |");
  strcpy( let[77][4] , "|_|  |_|");
  strcpy( let[77][5] , "        ");

  strcpy( let[78][0] , " _   _ ");
  strcpy( let[78][1] , "| \\ | |");
  strcpy( let[78][2] , "|  \\| |");
  strcpy( let[78][3] , "| |\\  |");
  strcpy( let[78][4] , "|_| \\_|");
  strcpy( let[78][5] , "       ");

  strcpy( let[79][0] , "  ___  ");
  strcpy( let[79][1] , " / _ \\ ");
  strcpy( let[79][2] , "| | | |");
  strcpy( let[79][3] , "| |_| |");
  strcpy( let[79][4] , " \\___/ ");
  strcpy( let[79][5] , "       ");

  strcpy( let[80][0] , " ____  ");
  strcpy( let[80][1] , "|  _ \\ ");
  strcpy( let[80][2] , "| |_) |");
  strcpy( let[80][3] , "|  __/ ");
  strcpy( let[80][4] , "|_|    ");
  strcpy( let[80][5] , "       ");

  strcpy( let[81][0] , "  ___  ");
  strcpy( let[81][1] , " / _ \\ ");
  strcpy( let[81][2] , "| | | |");
  strcpy( let[81][3] , "| |_| |");
  strcpy( let[81][4] , " \\__\\_\\");
  strcpy( let[81][5] , "       ");

  strcpy( let[82][0] , " ____  ");
  strcpy( let[82][1] , "|  _ \\ ");
  strcpy( let[82][2] , "| |_) |");
  strcpy( let[82][3] , "|  _ < ");
  strcpy( let[82][4] , "|_| \\_\\");
  strcpy( let[82][5] , "       ");

  strcpy( let[83][0] , " ____  ");
  strcpy( let[83][1] , "/ ___| ");
  strcpy( let[83][2] , "\\___ \\ ");
  strcpy( let[83][3] , " ___) |");
  strcpy( let[83][4] , "|____/ ");
  strcpy( let[83][5] , "       ");

  strcpy( let[84][0] , " _____ ");
  strcpy( let[84][1] , "|_   _|");
  strcpy( let[84][2] , "  | |  ");
  strcpy( let[84][3] , "  | |  ");
  strcpy( let[84][4] , "  |_|  ");
  strcpy( let[84][5] , "       ");

  strcpy( let[85][0] , " _   _ ");
  strcpy( let[85][1] , "| | | |");
  strcpy( let[85][2] , "| | | |");
  strcpy( let[85][3] , "| |_| |");
  strcpy( let[85][4] , " \\___/ ");
  strcpy( let[85][5] , "       ");

  strcpy( let[86][0] , "__     __");
  strcpy( let[86][1] , "\\ \\   / /");
  strcpy( let[86][2] , " \\ \\ / / ");
  strcpy( let[86][3] , "  \\ V /  ");
  strcpy( let[86][4] , "   \\_/   ");
  strcpy( let[86][5] , "         ");

  strcpy( let[87][0] , "__        __");
  strcpy( let[87][1] , "\\ \\      / /");
  strcpy( let[87][2] , " \\ \\ /\\ / / ");
  strcpy( let[87][3] , "  \\ V  V /  ");
  strcpy( let[87][4] , "   \\_/\\_/   ");
  strcpy( let[87][5] , "            ");

  strcpy( let[88][0] , "__  __");
  strcpy( let[88][1] , "\\ \\/ /");
  strcpy( let[88][2] , " \\  / ");
  strcpy( let[88][3] , " /  \\ ");
  strcpy( let[88][4] , "/_/\\_\\");
  strcpy( let[88][5] , "      ");

  strcpy( let[89][0] , "__   __");
  strcpy( let[89][1] , "\\ \\ / /");
  strcpy( let[89][2] , " \\ V / ");
  strcpy( let[89][3] , "  | |  ");
  strcpy( let[89][4] , "  |_|  ");
  strcpy( let[89][5] , "       ");

  strcpy( let[90][0] , " _____");
  strcpy( let[90][1] , "|__  /");
  strcpy( let[90][2] , "  / / ");
  strcpy( let[90][3] , " / /_ ");
  strcpy( let[90][4] , "/____|");
  strcpy( let[90][5] , "      ");

  strcpy( let[95][0] , "       ");
  strcpy( let[95][1] , "       ");
  strcpy( let[95][2] , "       ");
  strcpy( let[95][3] , "       ");
  strcpy( let[95][4] , " _____ ");
  strcpy( let[95][5] , "|_____|");
  strcpy( dsa[95],     "FPTVWY");   // decrease space after listed characters

  strcpy( let[97][0] , "       ");
  strcpy( let[97][1] , "  __ _ ");
  strcpy( let[97][2] , " / _` |");
  strcpy( let[97][3] , "| (_| |");
  strcpy( let[97][4] , " \\__,_|");
  strcpy( let[97][5] , "       ");
  strcpy( dsa[97],     "FTVWY");

  strcpy( let[98][0] , " _     ");
  strcpy( let[98][1] , "| |__  ");
  strcpy( let[98][2] , "| '_ \\ ");
  strcpy( let[98][3] , "| |_) |");
  strcpy( let[98][4] , "|_.__/ ");
  strcpy( let[98][5] , "       ");

  strcpy( let[99][0] , "      ");
  strcpy( let[99][1] , "  ___ ");
  strcpy( let[99][2] , " / __|");
  strcpy( let[99][3] , "| (__ ");
  strcpy( let[99][4] , " \\___|");
  strcpy( let[99][5] , "      ");
  strcpy( dsa[98],     "FTVWY");

  strcpy( let[100][0] , "     _ ");
  strcpy( let[100][1] , "  __| |");
  strcpy( let[100][2] , " / _` |");
  strcpy( let[100][3] , "| (_| |");
  strcpy( let[100][4] , " \\__,_|");
  strcpy( let[100][5] , "       ");
  strcpy( dsa[100],     "FTVWY");

  strcpy( let[101][0] , "      ");
  strcpy( let[101][1] , "  ___ ");
  strcpy( let[101][2] , " / _ \\");
  strcpy( let[101][3] , "|  __/");
  strcpy( let[101][4] , " \\___|");
  strcpy( let[101][5] , "      ");
  strcpy( dsa[101],     "FTVWY");

  strcpy( let[102][0] , "  __ ");
  strcpy( let[102][1] , " / _|");
  strcpy( let[102][2] , "| |_ ");
  strcpy( let[102][3] , "|  _|");
  strcpy( let[102][4] , "|_|  ");
  strcpy( let[102][5] , "     ");

  strcpy( let[103][0] , "       ");
  strcpy( let[103][1] , "  __ _ ");
  strcpy( let[103][2] , " / _` |");
  strcpy( let[103][3] , "| (_| |");
  strcpy( let[103][4] , " \\__, |");
  strcpy( let[103][5] , " |___/ ");
  strcpy( dsa[103],     "FTVWY");

  strcpy( let[104][0] , " _     ");
  strcpy( let[104][1] , "| |__  ");
  strcpy( let[104][2] , "| '_ \\ ");
  strcpy( let[104][3] , "| | | |");
  strcpy( let[104][4] , "|_| |_|");
  strcpy( let[104][5] , "       ");

  strcpy( let[105][0] , " _ ");
  strcpy( let[105][1] , "(_)");
  strcpy( let[105][2] , "| |");
  strcpy( let[105][3] , "| |");
  strcpy( let[105][4] , "|_|");
  strcpy( let[105][5] , "   ");

  strcpy( let[106][0] , "   _ ");
  strcpy( let[106][1] , "  (_)");
  strcpy( let[106][2] , "  | |");
  strcpy( let[106][3] , "  | |");
  strcpy( let[106][4] , " _/ |");
  strcpy( let[106][5] , "|__/ ");
  strcpy( dsa[103],     "FTVWY");

  strcpy( let[107][0] , " _    ");
  strcpy( let[107][1] , "| | __");
  strcpy( let[107][2] , "| |/ /");
  strcpy( let[107][3] , "|   < ");
  strcpy( let[107][4] , "|_|\\_\\");
  strcpy( let[107][5] , "      ");

  strcpy( let[108][0] , " _ ");
  strcpy( let[108][1] , "| |");
  strcpy( let[108][2] , "| |");
  strcpy( let[108][3] , "| |");
  strcpy( let[108][4] , "|_|");
  strcpy( let[108][5] , "   ");

  strcpy( let[109][0] , "           ");
  strcpy( let[109][1] , " _ __ ___  ");
  strcpy( let[109][2] , "| '_ ` _ \\ ");
  strcpy( let[109][3] , "| | | | | |");
  strcpy( let[109][4] , "|_| |_| |_|");
  strcpy( let[109][5] , "           ");

  strcpy( let[110][0] , "       ");
  strcpy( let[110][1] , " _ __  ");
  strcpy( let[110][2] , "| '_ \\ ");
  strcpy( let[110][3] , "| | | |");
  strcpy( let[110][4] , "|_| |_|");
  strcpy( let[110][5] , "       ");

  strcpy( let[111][0] , "       ");
  strcpy( let[111][1] , "  ___  ");
  strcpy( let[111][2] , " / _ \\ ");
  strcpy( let[111][3] , "| (_) |");
  strcpy( let[111][4] , " \\___/ ");
  strcpy( let[111][5] , "       ");
  strcpy( dsa[111],     "FTVWY");

  strcpy( let[112][0] , "       ");
  strcpy( let[112][1] , " _ __  ");
  strcpy( let[112][2] , "| '_ \\ ");
  strcpy( let[112][3] , "| |_) |");
  strcpy( let[112][4] , "| .__/ ");
  strcpy( let[112][5] , "|_|    ");

  strcpy( let[113][0] , "       ");
  strcpy( let[113][1] , "  __ _ ");
  strcpy( let[113][2] , " / _` |");
  strcpy( let[113][3] , "| (_| |");
  strcpy( let[113][4] , " \\__, |");
  strcpy( let[113][5] , "    |_|");
  strcpy( dsa[113],     "FTVWY");

  strcpy( let[114][0] , "      ");
  strcpy( let[114][1] , " _ __ ");
  strcpy( let[114][2] , "| '__|");
  strcpy( let[114][3] , "| |   ");
  strcpy( let[114][4] , "|_|   ");
  strcpy( let[114][5] , "      ");

  strcpy( let[115][0] , "     ");
  strcpy( let[115][1] , " ___ ");
  strcpy( let[115][2] , "/ __|");
  strcpy( let[115][3] , "\\__ \\");
  strcpy( let[115][4] , "|___/");
  strcpy( let[115][5] , "     ");

  strcpy( let[116][0] , " _   ");
  strcpy( let[116][1] , "| |_ ");
  strcpy( let[116][2] , "| __|");
  strcpy( let[116][3] , "| |_ ");
  strcpy( let[116][4] , " \\__|");
  strcpy( let[116][5] , "     ");

  strcpy( let[117][0] , "       ");
  strcpy( let[117][1] , " _   _ ");
  strcpy( let[117][2] , "| | | |");
  strcpy( let[117][3] , "| |_| |");
  strcpy( let[117][4] , " \\__,_|");
  strcpy( let[117][5] , "       ");

  strcpy( let[118][0] , "       ");
  strcpy( let[118][1] , "__   __");
  strcpy( let[118][2] , "\\ \\ / /");
  strcpy( let[118][3] , " \\ V / ");
  strcpy( let[118][4] , "  \\_/  ");
  strcpy( let[118][5] , "       ");

  strcpy( let[119][0] , "          ");
  strcpy( let[119][1] , "__      __");
  strcpy( let[119][2] , "\\ \\ /\\ / /");
  strcpy( let[119][3] , " \\ V  V / ");
  strcpy( let[119][4] , "  \\_/\\_/  ");
  strcpy( let[119][5] , "          ");

  strcpy( let[120][0] , "      ");
  strcpy( let[120][1] , "__  __");
  strcpy( let[120][2] , "\\ \\/ /");
  strcpy( let[120][3] , " >  < ");
  strcpy( let[120][4] , "/_/\\_\\");
  strcpy( let[120][5] , "      ");

  strcpy( let[121][0] , "       ");
  strcpy( let[121][1] , " _   _ ");
  strcpy( let[121][2] , "| | | |");
  strcpy( let[121][3] , "| |_| |");
  strcpy( let[121][4] , " \\__, |");
  strcpy( let[121][5] , " |___/ ");

  strcpy( let[122][0] , "     ");
  strcpy( let[122][1] , " ____");
  strcpy( let[122][2] , "|_  /");
  strcpy( let[122][3] , " / / ");
  strcpy( let[122][4] , "/___|");
  strcpy( let[122][5] , "     ");

  strcpy( let[124][0] , " _ ");
  strcpy( let[124][1] , "| |");
  strcpy( let[124][2] , "| |");
  strcpy( let[124][3] , "| |");
  strcpy( let[124][4] , "| |");
  strcpy( let[124][5] , "|_|");

  strcpy( let[126][0] , "         ");
  strcpy( let[126][1] , "         ");
  strcpy( let[126][2] , "  __   _ ");
  strcpy( let[126][3] , " /  \\_/ |");
  strcpy( let[126][4] , "|_/\\__ / ");
  strcpy( let[126][5] , "         ");
  strcpy( dsa[126],     "FTVWY");

  strcpy( let[196][0] , " _   _ ");
  strcpy( let[196][1] , "(_)_(_)");
  strcpy( let[196][2] , "  /_\\  ");
  strcpy( let[196][3] , " / _ \\ ");
  strcpy( let[196][4] , "/_/ \\_\\");
  strcpy( let[196][5] , "       ");

  strcpy( let[214][0] , " _   _ ");
  strcpy( let[214][1] , "(_)_(_)");
  strcpy( let[214][2] , " / _ \\ ");
  strcpy( let[214][3] , "| |_| |");
  strcpy( let[214][4] , " \\___/ ");
  strcpy( let[214][5] , "       ");

  strcpy( let[220][0] , " _   _ ");
  strcpy( let[220][1] , "(_) (_)");
  strcpy( let[220][2] , "| | | |");
  strcpy( let[220][3] , "| |_| |");
  strcpy( let[220][4] , " \\___/ ");
  strcpy( let[220][5] , "       ");

  strcpy( let[223][0] , "  ___ ");
  strcpy( let[223][1] , " / _ \\");
  strcpy( let[223][2] , "| |/ /");
  strcpy( let[223][3] , "| |\\ \\");
  strcpy( let[223][4] , "| ||_/");
  strcpy( let[223][5] , "|_|   ");

  strcpy( let[228][0] , " _   _ ");
  strcpy( let[228][1] , "(_)_(_)");
  strcpy( let[228][2] , " / _` |");
  strcpy( let[228][3] , "| (_| |");
  strcpy( let[228][4] , " \\__,_|");
  strcpy( let[228][5] , "       ");

  strcpy( let[246][0] , " _   _ ");
  strcpy( let[246][1] , "(_)_(_)");
  strcpy( let[246][2] , " / _ \\ ");
  strcpy( let[246][3] , "| (_) |");
  strcpy( let[246][4] , " \\___/ ");
  strcpy( let[246][5] , "       ");

  strcpy( let[252][0] , " _   _ ");
  strcpy( let[252][1] , "(_) (_)");
  strcpy( let[252][2] , "| | | |");
  strcpy( let[252][3] , "| |_| |");
  strcpy( let[252][4] , " \\__,_|");
  strcpy( let[252][5] , "       ");

  strcpy( let[91][0] , " ___ ");
  strcpy( let[91][1] , "|  _|");
  strcpy( let[91][2] , "| |  ");
  strcpy( let[91][3] , "| |  ");
  strcpy( let[91][4] , "| |_ ");
  strcpy( let[91][5] , "|___|");

  strcpy( let[92][0] , "__    ");
  strcpy( let[92][1] , "\\ \\   ");
  strcpy( let[92][2] , " \\ \\  ");
  strcpy( let[92][3] , "  \\ \\ ");
  strcpy( let[92][4] , "   \\_\\");
  strcpy( let[92][5] , "      ");

  strcpy( let[93][0] , " ___ ");
  strcpy( let[93][1] , "|_  |");
  strcpy( let[93][2] , "  | |");
  strcpy( let[93][3] , "  | |");
  strcpy( let[93][4] , " _| |");
  strcpy( let[93][5] , "|___|");

} // init_strcpy( letter() -------------------------------------------------------------


//   Dec   Char            Dec   Char
//   -------------------------------------
//   0     NUL '\0'        64    @
//   1     SOH             65    A
//   2     STX             66    B
//   3     ETX             67    C
//   4     EOT             68    D
//   5     ENQ             69    E
//   6     ACK             70    F
//   7     BEL '\a'        71    G
//   8     BS  '\b'        72    H
//   9     HT  '\t'        73    I
//   10    LF  '\n'        74    J
//   11    VT  '\v'        75    K
//   12    FF  '\f'        76    L
//   13    CR  '\r'        77    M
//   14    SO              78    N
//   15    SI              79    O
//   16    DLE             80    P
//   17    DC1             81    Q
//   18    DC2             82    R
//   19    DC3             83    S
//   20    DC4             84    T
//   21    NAK             85    U
//   22    SYN             86    V
//   23    ETB             87    W
//   24    CAN             88    X
//   25    EM              89    Y
//   26    SUB             90    Z
//   27    ESC             91    [
//   28    FS              92    \   '\\'
//   29    GS              93    ]
//   30    RS              94    ^
//   31    US              95    _
//   32    SPACE           96    `
//   33    !               97    a
//   34    "               98    b
//   35    #               99    c
//   36    $               100   d
//   37    %               101   e
//   38    &               102   f
//   39    '               103   g
//   40    (               104   h
//   41    )               105   i
//   42    *               106   j
//   43    +               107   k
//   44    ,               108   l
//   45    -               109   m
//   46    .               110   n
//   47    /               111   o
//   48    0               112   p
//   49    1               113   q
//   50    2               114   r
//   51    3               115   s
//   52    4               116   t
//   53    5               117   u
//   54    6               118   v
//   55    7               119   w
//   56    8               120   x
//   57    9               121   y
//   58    :               122   z
//   59    ;               123   {
//   60    <               124   |
//   61    =               125   }
//   62    >               126   ~
//   63    ?               127   DEL

//  196    Ä
//  214    Ö
//  220    Ü
//  223    ß
//  228    ä
//  246    ö
//  252    ü
//EOF
