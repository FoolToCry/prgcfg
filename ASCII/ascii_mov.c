//------------------------------------------------------------------------------
// movie player for ASCII pics
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../LIB/prgcfg.h"
#include "../LIB/mystrings.h"

#define cfgstrlen 120         /* string length for configuration file        */

//-----------------------------------------------------------------------------
// variables set from command line
//-----------------------------------------------------------------------------
bool   help		= 0;    // commandline options help
char   FileName[1000]   = "";

#define n_lines  100
#define n_frames 100
char  *frame[ n_frames][n_lines];  // storage for movie frames with n_lines each
char **framep[n_frames];           // pointer to frames

//------------------------------------------------------------------------------
// global variables configurable via command line or file
//------------------------------------------------------------------------------
char   moviefile[1000]  = "";      // file with frame pics
int    nframes = 100;              // actual number of used frames
int    nlines  = 100;              // actual number lines per frame
int    delay   = 150000;
int    loops   = 1;
bool   cls     = 0;
bool   debug   = 0;
bool   displ_frame_nr = 0;

//------------------------------------------------------------------------------
// global variables but not configurable via command line or file
//------------------------------------------------------------------------------
char comment[5]       = "//";  // comment sign for reading configuration from files
char separator	      = '=';   // separator between option and value when reading config file
char boldon[]         = "\033[1m"; // bold print on
char boldoff[]        = "\033[0m"; // bold print off

#include <sys/timeb.h>
int ftime(struct timeb *tp);

//------------------------------------------------------------------------------
// function declaration
//------------------------------------------------------------------------------
void call_help( int select);       // put out some special text for and call help function
void variableinit( int set);       // init variables (set=1), free allocated memory (set=0)

//------------------------------------------------------------------------------
// Configuration Options
// line break for help messages set via PrgCfgAutoLineBreak = 75;
//------------------------------------------------------------------------------
bool todo = 0;
bool dummy = 0;
char nlines_help[cfgstrlen];

// config via command line
static PrgCfgOpt cfg_opt[]={
  // name         type       variable dest.     used help
  { "_noprint_",  boolt,     &dummy,            0,   "\n\033[1mControl switches from command line\033[0m"},
  { "h",          boolt,     &help,             0,   "Display this help."},
  { "f"  ,        stringt,   &moviefile,        0,   "-f <filename> ascii movie file (required)"},
  { "nframes",    intt,      &nframes,          0,   "number of frames"},
  { "nlines",     intt,      &nlines,           0,   nlines_help},
  { "delay",      intt,      &delay,            0,   "delay between two frames, default: 150000"},
  { "loops",      intt,      &loops,            0,   "number of loops for all frames"},
  { "cls",        boolt,     &cls,              0,   "clear screen at program end"},
  { "step",       boolt,     &debug,            0,   "step through frame sequence via keypress"},
  { "dfn",        boolt,     &displ_frame_nr,   0,   "display frame numbers"},
  { NULL}   // delimiter for automatic size detection of cfg_opt[]
};

// config via file
static PrgCfgOpt fcfg_opt[]={
  // name         type       variable dest.     used help
  { "_noprint_",  boolt,     &dummy,            0,   "\n\033[1mVariables for config file:\033[0m"},
  { "frame",      stringa2t, &framep,           0,   "frame[n][m] = \"string\", frame number n, line m"},
  { "nframes",    intt,      &nframes,          0,   "number of frames"},
  { "nlines",     intt,      &nlines,           0,   nlines_help},
  { "cls",        boolt,     &cls,              0,   "clear screen at program end"},
  { "step",       boolt,     &debug,            0,   "step through frame sequence via keypress"},
  { "dfn",        boolt,     &displ_frame_nr,   0,   "display frame numbers"},
  { NULL}   // delimiter for automatic size detection of cfg_opt[]
};

//------------------------------------------------------------------------------
// MAIN
//  - init variables
//  - read commandline and config file
//  - show movie
//  - exit
//------------------------------------------------------------------------------
int main( int argc, char **argv)
{
  int configOK = 1;
  int i,k,l;
  struct timeb tp;
  variableinit(1);
  sprintf( nlines_help, "number of lines per frame (max %d characters per line)", cfgstrlen);

  strcpy( PrgCfgCommentSign, comment);   // comment sign for reading configuration from files
  PrgCfgSeparatorSign = separator;	 // separator between option and value when reading config file

  //----------------------------------------------------------------------------
  // read program configuration from ...
  // command line and file         : cfgOK = prgcfg_read(      argc, argv, <PrgCfgOpt for commandline>, <cfg.file name>, <PrgCfgOpt for config file>);
  // command line only, no cfg file: cfgOK = prgcfg_read_cmdl( argc, argv, <PrgCfgOpt for commandline>);
  // config file only              : cfgOK = prgcfg_read_file( <cfg.file name>, <PrgCfgOpt for config file>);
  //----------------------------------------------------------------------------
  configOK = prgcfg_read( argc, argv, cfg_opt, moviefile, fcfg_opt);

  // display program help
  if( (help == true) || (strlen(moviefile) == 0)) {
    call_help( 0); // command line
    call_help( 1); // file
  }


  //--------------------------------------------------------------------------
  // play movie
  //--------------------------------------------------------------------------

  // prgcfgvstat() checks if a program option has been modified (return > 0)
  else if( configOK && prgcfgvstat( cfg_opt, moviefile)) {
    cursor_off();
    ftime( &tp);

    if( debug)
      set_keypress();

    for( l = 0; l<loops; l++) {
      for( k = 0; k<nframes; k++) {
	for( i = 0; i<nlines; i++) {
	  printf("%s\n", frame[k][i]);
	}

        if( displ_frame_nr)
          printf ("[ %2d ]", k);
	usleep( delay);

        if( debug)
          getkey();

	ftime(&tp);
        printf("\033[%dA\n", 1);
	// - Move the cursor up
	printf("\033[%dA", nlines);
      }
    }
    if( debug)
      reset_keypress();

    if( cls == true) {
      // clear output
      for( l = 0; l<= nlines; l++) {
	printf("\033[K\n");
      }
      // - Move the cursor up
      printf("\033[%dA", nlines +1);
    }
    cursor_on();
  }

  variableinit( 0); // free allocated mem

  if( cls == false) {
    // - Move the cursor down
    printf("\033[%dB", nlines +1);
    printf("                                                                  ");
    printf("\n");
  }
 return 0;
}// main() ---------------------------------------------------------------------



//------------------------------------------------------------------------------
// init variables (set=1), free allocated memory (set=0)
//------------------------------------------------------------------------------
void variableinit( int set)
{
  int i, k;

  // malloc amd init strings
  if( set == 1) {
    for( i=0; i<n_frames; i++) {
      for( k=0; k<n_lines; k++) {
	frame[i][k] = (char *) malloc( cfgstrlen * sizeof( char));
	strcpy( frame[i][k], " ");   // init lines
      }
      framep[i] = &frame[i][0];      // init pointer to list of frames
    }
  }

  // free
  else {
    for( i=0; i<n_frames; i++) {
      for( k=0; k<n_lines; k++){
	free( frame[i][k]);
      }
    }
  }

}// variableinit() -------------------------------------------------------------


//------------------------------------------------------------------------------
// Help display
//------------------------------------------------------------------------------
void call_help( int select)
{
  PrgCfgAutoLineBreak = 75;

  // command line help
  if( select == 0) {
    prgcfghelp( cfg_opt, 1);   // 1 switches on printing "-" before option name

  }

  // file config help
  else if( select == 1) {
    sprintf( PrgCfgUsage, " "); // overwrite string "usage: programname [-option [<value>]]"
    prgcfghelp( fcfg_opt, 0);

  }


}// set_help_text() ------------------------------------------------------------


// EOF
