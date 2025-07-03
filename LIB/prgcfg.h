//-----------------------------------------------------------------------------
//  Description    : Command Line Configuration header file
//------------------------------------------------------------------------------
#ifndef __prgcfg_h__
#define __prgcfg_h__

#define PrgCfgOptMaxNameLength   1000 /* max. length for a command option      */
#define PrgCfgFileLineMaxLength  1000 /* max. length for a line in config file */
#define PrgCfgUsageTextLength   10000 /* max. length for help program text     */

typedef enum  { false, true} bool;

//------------------------------------------------------------------------------
// required for main function
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// C types and corresponding data types for program configuration
//------------------------------------------------------------------------------
// bool	     : boolt
// int	     : intt
// int[]     : intat
// *int[]    : inta2t
// double    : doublet
// double[]  : doubleat
// *double[] : doublea2t
// char      : chart
// char[]    : stringt
// *char[]   : stringat
// **char[]  : stringa2t
//------------------------------------------------------------------------------
// array dim. :  0        1         2         3
//------------------------------------------------------------------------------
typedef enum  { boolt,
		intt,    intat,    inta2t,
		doublet, doubleat, doublea2t,
		chart,   stringt,  stringat,  stringa2t} PrgCfgArgType;

//------------------------------------------------------------------------------
// structure for defining configuration options
//
// use the following style for declaring user defined configuration options
//
// <type> <variable> = <init value>; // varible declaration
//
//  static PrgCfgOpt <name>[]={
//    // name     type             variable dest.  used   help
//    { "<name>", <PrgCfgArgType>, &<variable>,    0,     "[help text]"},
//      ...
//    { NULL}    // delimiter for automatic size detection
//  };
//------------------------------------------------------------------------------
typedef struct configuration_option
{
  char *name;    // option name in command line or file
  int  type;     // option type
  void *value;   // address of variable to store value
  int  used;     // incremented if option was called in configuraton
  char *help;    // help description of configuration option
} PrgCfgOpt;


//------------------------------------------------------------------------------
// read configuration from command line and from configuration file
// argc:           argument count from program call
// argv:           argument values from program call
// cmdl_cfgval:    pointer to user defined configuration options (array of PrgCfgOpt)
// PrgCfgFileName: configuration file name
// file_cfgval:    pointer to user defined configuration options (array of
//                 PrgCfgOpt)
//
// Does the same as prgcfg_read_cmdl and prgcfg_read_file but in one function
// call
//------------------------------------------------------------------------------
int prgcfg_read( int argc, char *argv[], PrgCfgOpt *cmdl_cfgval, char *PrgCfgFileName, PrgCfgOpt *file_cfgval);

//------------------------------------------------------------------------------
// read configuration from command line
// argc:     argument count from program call
// argv:     argument values from program call
// cfgval:   pointer to user defined configuration options (array of PrgCfgOpt)
//------------------------------------------------------------------------------
int prgcfg_read_cmdl( int argc, char *argv[], PrgCfgOpt *cfgval);

//------------------------------------------------------------------------------
// read configuration from file
// PrgCfgFileName: configuration file name
// cfgval:         pointer to user defined configuration options (array of
//                 PrgCfgOpt)
//------------------------------------------------------------------------------
int prgcfg_read_file( char *PrgCfgFileName, PrgCfgOpt *cfgval);

//------------------------------------------------------------------------------
// some optional variables for usage in user code
//------------------------------------------------------------------------------
// input value (everything other than program name, otions and their arguments)
extern char PrgCfgUnassigned[PrgCfgOptMaxNameLength];

// name of program, (same as program_invocation_short_name)
extern char PrgCfgPrgName[PrgCfgOptMaxNameLength];

// command line
extern char PrgCfgPrgCall[PrgCfgOptMaxNameLength];

// comment sign when reading configuration from files, default: "//"
extern char PrgCfgCommentSign[5];

// separator sign between option amd value when reading configuration from file
// default: '='
extern char PrgCfgSeparatorSign;

// first text to be displayed from function prgcfghelp, default:
// usage: PrgCfgPrgName [-<option> [<value>]]", ,
extern char PrgCfgUsage[PrgCfgUsageTextLength];

// insert line break after column n in function prgcfghelp if n > 0, default: 80
extern int PrgCfgAutoLineBreak;

//------------------------------------------------------------------------------
// some optional functions for usage in user code
//------------------------------------------------------------------------------
// displays help screen
void prgcfghelp( PrgCfgOpt *cfgval, int switch_on);

// get modiefication status of variable
int prgcfgvstat( PrgCfgOpt *cfgval, void *var);

// print a log message to to file and to stderr if verbose=1
void prglogmsg( bool verbose, FILE *prglgfp, const char *logmsg, ...);

// print a log message header to file and to stderr if verbose=1
extern int PrgCfgLogHdCommentOn;
void prglogheader( bool verbose, FILE *prglgfp,  const char *logmsg, ...);


//------------------------------------------------------------------------------
// read configuration from command line, this function is used inside of
// prgcfg_read() instead of prgcfg_read_cmdl().
// argc:     argument count from program call
// argv:     argument values from program call
// cfgval:   pointer to user defined configuration options (array of PrgCfgOpt)
// ovwrfile: If set to 1 the modified state of each variable won't be changed
//           and bool values won't be modified, other values will be evaluated
//           according command line. This is useful for a 2nd. call of
//           prgcfg_cmdl() after prgcfg_file() call if the command line should
//	     overwrite configuration read from a file
//------------------------------------------------------------------------------
int prgcfg_read_cmdline( int argc, char *argv[], PrgCfgOpt *cfgval, int ovwrfile);

//------------------------------------------------------------------------------
// functions for internal use only
//------------------------------------------------------------------------------
// assign values to variables
int prgcfgvarassign( PrgCfgOpt *cfgval, int index, char *value, int *arind, int ovwrfile);

// print command line options with their current values, status and help text
// only usefull for program debugging
void prgcfgprint( PrgCfgOpt *cfgval);

// debug PrgCfgOpt
void prgcfgdbg( PrgCfgOpt *cfgval);

#endif
// EOF
