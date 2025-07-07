//------------------------------------------------------------------------------
/*
 base_conv prints out input values in decimal, hexadecimal, binary and fixed-point format.

 command line syntax:
        %> base_conv [-option] [value]

 Options:
 -h     display short help
 -help  display full help text

 -x     base of input value is hexadecimal
 -d     base of input value is decimal
 -b     base of input value is binary
 -f     input in floating point format

 -X     print only hexadecimal value, default is to print dec, hex and bin
 -D     print only decimal value
 -B     print only binary value
 -F     print only fixed-point value (requires format specification via -fp)

 -fp    <format>, enable fixed-point output (default: disabled),
        format: <sgn>:<wl>:<fl> or <wl>:<fl>
        sgn: signed (0 or 1), signed is always on for specification via -fp <wl>:<fl>
        wl = word length
        fl = fractional length, negative fractional length is not supported
        il = integer length il=wl-fl, printed only in verbose mode, il inlcludes sign-bit
        to disable fixed-point output set: -fp 0:0

 -s     separate output digits, grouping 3 digits (decimal) or 4 digits (hex and bin)
 -c     <c>, set character for digit separation, default: '_'
 -z     show leading zeros in hex- and binary output
 -sat   saturate result for real to fixed-point conversion in case of overflow

 -u     treat values as unsigned integer, default: signed
 -v     verbose mode for fixed-point display and ring shift operations
 -w     bit width for ring shift, -w <n> forces n bit binary output
 -i     print out current settings
 -C     <calculation> call basic calculator system command (echo <commandline> | bc))
 -o     <n>, set output color (0-19) for displaying bits out of specified fixpoint range,
        default color: 7
 -q     set quiet mode, disable all outputs except converted values
 -nocol colored output disable, use -nocol if you don't want escape sequences in output
        data, default: colored output enabled

*/
//------------------------------------------------------------------------------
// define _GNU_SOURCE is set in Makefile to solve compile error: base_conv.c:(.text+0x1f8): undefined reference to `__isoc99_scanf' ...
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "../LIB/mystrings.h"
#include "../LIB/prgcfg.h"
#include "../LIB/keys.h"
#include <locale.h>
#include <unistd.h> // required for checking piped data

#define cfgstrlen       50  /* string length for configuration file */

//-----------------------------------------------------------------------------
// variables set from command line
//-----------------------------------------------------------------------------
bool help              = 0; // commandline options help
bool verbose_help      = 0;
bool hex_base_in       = 0; // base of input value is hexadecimal
bool dec_base_in       = 0; // base of input value is decimal
bool bin_base_in       = 0; // base of input value is binary
bool float_in          = 0; // input in floating point format
bool hex_base_out      = 0; // base of output value is hexadecimal
bool dec_base_out      = 0; // base of output value is decimal
bool bin_base_out      = 0; // base of output value is binary
bool fixpoint_out      = 0; // print only fixed-point value (requires format specification via -fp)"},
bool digit_separate    = 0; // separate nibbles in binary output with spaces
char separator_sign    = '_'; // separtor sign for -s option
bool leading_zero      = 0; // show leading zeros in binary output
bool piped             = 0; // no prompt, not interactive, use this for call in piped command
bool todo = 0;
bool usigned           = 0; // treat values as unsigned integer, default: signed
bool verbose           = 0; // verbose mode for ring shift operations
bool quiet	       = 0; // quiet mode, disable all outputs except converted values
int  width             = 0; // bit width for ring shift
bool info              = 0; // print out current settings
bool no_colored_output = 0;
int  drkprnt_color     = 7; // color 0-19 (+30) for out of range bits
bool drkprnt_dbg       = 0; // print color examples
bool dbg_enable        = 0; // global enable for debug outputs
char fp_format[100]    = "0:0:0"; // fixed-point format
char system_command[1000] = "";  // system command
//char real2fixpoint[100]   = "";  // real to fixed-point conversion
bool saturation       = 0; // if 1, saturate bits in case of overflow in real to fixp. conversion

//-----------------------------------------------------------------------------
// initernal variables
//-----------------------------------------------------------------------------
bool interactive_mode = 1; // default, start in interactive mode
bool fp_display       = 0; // toggle fixed-point display on/off
bool fp_s             = 0; // 1: msb of fixed-point is signed bit
int  fpn_w            = 0; // set width of fixed-point bits fixed-point_i+fixed-point_w+fixed-point_s
int  fpn_f            = 0; // set number of fixed-point fractional bits
int  fpn_i            = 0; // set number of fixed-point integer bits
int  fpv_i            = 0; // fixed-point integer val
int  fpv_f            = 0; // fixed-point fractional bit value
int  fp_range_ovflw   = 0; // flags range overflow during real to fixed-point conversion

int  set_width        = 0; // set width for circular shift
int  set_fp_w         = 0; // set width of fixed-point bits fixed-point_i+fixed-point_w+fixed-point_s
int  set_fp_f         = 0; // set number of fixed-point fractional bits
int  set_fp_format    = 0; // set fixed-point format
int  set_dp_format    = 0;
int  set_syscmd       = 0; // system command call
int  real2fp          = 0; // convert real value to fixed-point format
int  float_in_tmp_after_calc = 0;
bool saturated        = 0;

int  set_separator    = 0;
int  display_info     = 0;

int  fp_conv_fr_real  = 0; // flags that current fixed-point number has been converted from real value
float conv_real_fp = 0.0; // old
long long conv_real_fp_i   = 0; // integer bits of the origianl real value which has been converted to fixed-point
long long conv_real_fp_f   = 0; // fraction bits of the origianl real value
int       conv_real_fp_fne = 0; // negative exponent for fraction, eg. 0.001 = 1E-3, conv_real_fp_fne = 3
long long conv_real_fp_s   = 0; // sign of the origianl real value -1 or 1
char conv_real_val[100]    = "";
long double conv_granularity;

#define DATA_STREAM_LENGTH 10000        // max data stream length 10k byte
#define LOGFILE_NAME "baseconv.log"
char boldon[]    = "\033[1m"; // bold print on
char normprint[] = "\033[0m"; // normal print on
char flash[]     = "\033[5m"; // flashing print on
char prog_name[80] ="";       // name of programm
char input_val[100] ="";      // input value to convert
char f_input_val_str[100] = "";

#define IS_HISTORY_SIZE 100
#define DBG_PRINT_MAIN  dbg_print( debug, __func__, __LINE__,"input_val: '%s', dec_in:%d, hex_in:%d, bin_in:%d, float_in:%d, dec_out:%d, hex_out:%d, bin_out:%d, fixp_out:%d", input_val, dec_base_in, hex_base_in, bin_base_in, float_in, dec_base_out, hex_base_out, bin_base_out, fixpoint_out)

//------------------------------------------------------------------------------
// command line configuration options
//------------------------------------------------------------------------------
char fp_help_txt[] = "\
<format>, enable fixed-point output (default: disabled)\n\
format: <sgn>:<wl>:<fl> or <wl>:<fl>\n\
sgn = signed (0 or 1), signed is always on for specification via -fp <wl>:<fl>\n\
l   = word length\n\
fl  = fractional length, negative fractional length is not supported\n\
il  = integer length, il=wl-fl, printed only in verbose mode, il inlcludes sign-bit\n\
to disable fixed-point output set: -fp 0:0\n                            \
";

static PrgCfgOpt cfg_opt[]={
  // name         type       variable dest.     used,  help
  { "h",          boolt,     &help,              0,    "display short help"},
  { "help",       boolt,     &verbose_help,      0,    "display full help text\n"},

  { "x",          boolt,     &hex_base_in,       0,    "base of input value is hexadecimal"},
  { "d",          boolt,     &dec_base_in,       0,    "base of input value is decimal"},
  { "b",          boolt,     &bin_base_in,       0,    "base of input value is binary"},
  { "f",          boolt,     &float_in,          0,    "input in floating point format\n"},

  { "X",          boolt,     &hex_base_out,      0,    "print only hexadecimal value, default is to print dec, hex and bin"},
  { "D",          boolt,     &dec_base_out,      0,    "print only decimal value"},
  { "B",          boolt,     &bin_base_out,      0,    "print only binary value"},
  { "F",          boolt,     &fixpoint_out,      0,    "print only fixed-point value (requires format specification via -fp)\n"},

  { "fp",         stringt,   &fp_format,         0,    fp_help_txt},
  { "s",          boolt,     &digit_separate,    0,    "separate output digits, grouping 3 digits (decimal) or 4 digits (hex and bin)"},
  { "c",          chart,     &separator_sign,    0,    "<c>, set character for digit separation, default: '_'"},
  { "z",          boolt,     &leading_zero,      0,    "show leading zeros in hex- and binary output"},
  { "sat",        boolt,     &saturation,        0,    "saturate result for real to fixed-point conversion to given wl in case of overflow\n"},

  { "u",          boolt,     &usigned,           0,    "treat values as unsigned integer, default: signed"},
  { "v",          boolt,     &verbose,           0,    "verbose mode for fixed-point display and ring shift operations"},
  { "w",          intt,      &width,             0,    "bit width for ring shift, -w <n> forces n bit binary output"},
  { "i",          boolt,     &info,              0,    "print out current settings"},
  { "C",          stringt,   &system_command,    0,    "<calculation> call basic calculator system command (echo <commandline> | bc))"},
  { "oc",         intt,      &drkprnt_color,     0,    "<n>, set output color (0-19) for displaying bits out of specified fixpoint range, default color: 7"},
  { "q",          boolt,     &quiet,             0,    "set quiet mode, disable all outputs except converted values"},
  { "nocol",      boolt,     &no_colored_output, 0,    "colored output disable, use -nocol if you don't want escape sequences in output data, default: colored output enabled"},
  { "odbg",       boolt,     &drkprnt_dbg,      -1,    "hidden switch: diplay color 0-19 examples"},
  { "dbg",        boolt,     &dbg_enable,       -1,    "hidden switch: enable debug outputs"},
  //{ "todo",       boolt,     &todo,            -10,  "hidden switch: diplay list of to dos for this program."},
  { NULL}   // delimiter for automatic size detection of cfg_opt[]
};


char bin_val_glb[100];
//------------------------------------------------------------------------------
// function declarations
//------------------------------------------------------------------------------
int first_aid ( int uargc, char *uargv[]); // help
void call_help( int select);       // put out some special text for and call help function
void variableinit( int set);       // init variables (set=1), free allocated memory (set=0)
int post_set_options ( int progstart);
long long hex2dec( char* hex_val);
long long bin2dec( char* bin_value);
void binout( int print, long long dec_val);
void hexout( int print, long long dec_val);
double fixp_out ( int print, long long dec_val);
long long inv( long long data);
void ctrl_out( int newline);
void calculate( int minus, int plus, int mult, int div, int and, int or, int xor, int not, int shl, int shr, int rsl, int rsr);
int send_values_to_control( char* command);
int control( int newline);
long long ring_shift( int shift_right, long long value, int shift);
char *ins_thousand_sep( long long n, int start);
char *ins_thousand_sep_unsigned( unsigned long long n, int start);
void print_config( int opt_change);
int print_system_call_result( char* syscmd);
void prep_conv_real_to_fixpoint( char* real_val);
unsigned long long conv_real_to_fixpoint( char* real_val, int frac);
void init_fp( char* fixp_format);
void darkprint( int onoff);
void init_dp( char* input_val);
void dbg_print ( int debug, const char* function, int line_nr, const char* message, ...);
void dbg_msg ( int debug, int line_nr, const char* message, ...);

char *sformat( const char *msg, ...);
void print_colors( void);

static UserCommands my_cmd[] = {
  // name    parameters         function    help
  { (char *) "help",       0,   first_aid,  (char *) "display this help"         },
  { NULL}   // delimiter for automatic size detection
};

int first_aid( int uargc, char *uargv[]) {
  call_help(0);
  return 0;
}

//-----------------------------------------------------------------------------
// put out base converted input value in hex, dec and bin
//-----------------------------------------------------------------------------
int main (int argc, char *argv[])
{
  //int i=1;
  int commandlineOK;
  int imax =0;
  char *is[ReadCommand_StringLength];     // array of input strings
  char *buffer[ReadCommand_StringLength]; // input buffer
  char command[ReadCommand_StringLength]; // current command input string
  int exit_prog = 0;
  char prompt[20] = "> ";                 // input prompt definition
  char historylogfile[ReadCommand_StringLength];
  int debug = 1;

  sprintf( historylogfile, "/home/%s/.base_conv.log", getenv("USER"));

  //------------------------------------------------------------------------------
  // evaluate options from command line
  //------------------------------------------------------------------------------
  variableinit( 1);
  commandlineOK = prgcfg_read_cmdline( argc, argv, cfg_opt, 0);
  debug = debug && dbg_enable;
  DBG_PRINT_MAIN ;
  if( commandlineOK == 0) {
    return 1;
  }

  //------------------------------------------------------------------------------
  // real to fixed-point conversion from command line
  //------------------------------------------------------------------------------
  if( prgcfgvstat( cfg_opt, &fp_format) ) {
    init_fp( fp_format);
    DBG_PRINT_MAIN ;
  }

  //------------------------------------------------------------------------------
  // basic calculator call from command line - ok this is not very useful ;-)
  //------------------------------------------------------------------------------
  if( prgcfgvstat( cfg_opt, &system_command) ) {
    set_syscmd = 1;
    strcpy( input_val, system_command);
    ctrl_out( 1);
    return 0;
  }
  DBG_PRINT_MAIN ;

  // aynthing left on the command line is a value to be converted
  strcpy( input_val, PrgCfgUnassigned);
  dbg_print( debug, __func__, __LINE__,"PrgCfgUnassigned: '%s'", PrgCfgUnassigned);
  DBG_PRINT_MAIN ;

  post_set_options( 1);
  DBG_PRINT_MAIN ;

  //------------------------------------------------------------------------------
  // color debug
  //------------------------------------------------------------------------------
  if( drkprnt_dbg)  {
    print_colors();
  }

  //------------------------------------------------------------------------------
  // info request from command line,
  // ...well you should remember what you have typed one second ago, but if not...
  //------------------------------------------------------------------------------
  if( info) {
    print_config( 0x1ff);
  }
  // TODO command for printing history

  if( (help == true) ||( verbose_help == true)) {
    call_help( 0); // 0: help for command line options, 1: help for configuration options via file
  }
  else {
    // no help requested:

    //------------------------------------------------------------------------------
    // convert value from command line
    //------------------------------------------------------------------------------
    if( strlen( input_val)) {
      //control( 1);
      interactive_mode = 0;
      DBG_PRINT_MAIN ;
      strcpy( command, input_val);
      exit_prog = send_values_to_control( command);
      printf("\n");
    }

    //------------------------------------------------------------------------------
    // values piped into program
    //------------------------------------------------------------------------------
    DBG_PRINT_MAIN ;
    imax = init_commandlist_history( is, buffer, IS_HISTORY_SIZE, historylogfile);

    fd_set readfds;
    struct timeval timeout;
    FD_ZERO( &readfds);
    timeout.tv_sec  = 0;
    timeout.tv_usec = 0;
    FD_SET( STDIN_FILENO, &readfds);

    if ( select( 1, &readfds, NULL, NULL, &timeout)) {
      DBG_PRINT_MAIN ;
      piped = 1;
      interactive_mode = 0;
      while( fgets( command, ReadCommand_StringLength, stdin)) {
	dbg_print( debug, __func__, __LINE__, "dbg piped, command: %s", command);
	exit_prog = send_values_to_control( command);
      }
    }

    //------------------------------------------------------------------------------
    // interactive mode
    //------------------------------------------------------------------------------
    if ( interactive_mode == 1) {
      dbg_msg( debug, __LINE__, "start interactive mode");
      printf("# enter number or command, q to quit, for help type -h");
      do{
	strcpy( command, readcommand( is, buffer, IS_HISTORY_SIZE, &imax, prompt, my_cmd));
	exit_prog = send_values_to_control( command);
      } while( !exit_prog);

      // clean up, write history log
      free_commandlist_history( is, buffer, IS_HISTORY_SIZE, imax, historylogfile);
    }
    else
      free_commandlist( is, buffer, IS_HISTORY_SIZE);

    if( piped == 0)
      printf("\n");

  }
  variableinit(0);
  return 0;
} // main() //-----------------------------------------------------------------


//-----------------------------------------------------------------------------
// get chunks from command line and pass it to control output function
//-----------------------------------------------------------------------------
int send_values_to_control( char* command) {
  int exit_prog = 0;
  char tmp[ReadCommand_StringLength];
  int scanfreturn = 0;
  int input_val_length = 0;
  int pos = 0;
  int debug = 0 && dbg_enable;
  int rval = 0;

  if( match( command, "#")) {
    dbg_msg( debug, __LINE__, "send_values_to_control: detected comment");
    if( piped ==1)
      printf("%s", command);      // print comment and clear command
    sprintf( command, "%s", "");  // remove comment, clear command
  }

  while( (scanfreturn = sscanf( command, "%s", input_val)) == 1) {
    dbg_print( debug, __func__, __LINE__, "command: \"%s\", input_val: \"%s\", scanf returned: %d", command, input_val, scanfreturn);
    input_val_length = strlen( input_val); // store length

    if( (input_val[0] == 0x0027) || (input_val[0] == '"') ) { // input in quotes?
      dbg_print( debug, __func__, __LINE__, "found quotes");
      if(input_val[0] == 0x0027)
        pos =  match( command, "'");
      else
        pos =  match( command, "\"");

      strcat( input_val, substr( command, 0, pos));         // get rest from quote into input val
      strcpy( tmp, command);                                // remove input_val from command for next loop
      sprintf( command, "%s", substr( tmp, pos, 0));
      dbg_print( debug, __func__, __LINE__, "rest of command: %s", command);
    }

    // call command control and conversion, if piped from command line a new line char is included in input_val
    rval = control( piped);
    if( rval == -1) {
      exit_prog = 1;
      break;
    }

    if( rval == -2) {
      dbg_print( debug, __func__, __LINE__, "clr command %s", command);
      sprintf( command, ""); // wrong command, continue without evaluation rest of command
    }

    // remove current command from input string
    strcpy( tmp, command);
    while( tmp[0] == ' ') { // remove leading spaces
      sprintf( command, "%s", substr( tmp, 1, 0));
      strcpy( tmp, command);
    }
    sprintf( tmp, "%s", substr( command, input_val_length, 0));
    strcpy( command, tmp);
  }
  return( exit_prog);
}// send_values_to_control() //------------------------------------------------



//-----------------------------------------------------------------------------
// check for option change or calculation
//-----------------------------------------------------------------------------
int control( int newline)
{
  int debug = 1 && dbg_enable;
  int opt_change;
  char wrongopt[100];
  opt_change = 0;
  dbg_print( debug, __func__, __LINE__, "control newline: %d, input_val:'%s'", newline, input_val);
  dbg_print( debug, __func__, __LINE__, "input: '%s', dec_in:%d, hex_in:%d, bin_in:%d, dec_out:%d, hex_out:%d, bin_out:%d, fixp_out:%d", input_val, dec_base_in, hex_base_in, bin_base_in, dec_base_out, hex_base_out, bin_base_out, fixpoint_out);

  int minus, plus, mult, div, and, or, xor, not, shl, shr, rsl, rsr;
  minus =  plus =  mult =  div = and = or = xor = not = shl = shr = rsl = rsr = 0;
  if( match( input_val,"#")) {
    dbg_print( debug, __func__, __LINE__, "detected comment");
    input_val[0] = '\0';
    return( 1);
  }
  else if( (match( input_val, "-") ==1) && (match( input_val, "pf") || match( input_val, "co"))) {
    if (match( input_val, "pf"))
      sprintf( wrongopt ,"-pf");
    else
      sprintf( wrongopt ,"-co");
    printf( "\n*** ERROR: unknown command: %s\n", wrongopt);
    input_val[0] = '\0';
    return( -2);
  }
  else if( (match( input_val, "-") ==1) &&
           (match( input_val, "q")  ||
	    match( input_val, "x")  || match( input_val, "X") ||
            match( input_val, "d")  || match( input_val, "D") ||
            match( input_val, "b")  || match( input_val, "B") ||
            match( input_val, "f")  || match( input_val, "F") || match( input_val, "sat") ||
            match( input_val, "s")  || match( input_val, "c") || match( input_val, "z")   ||
            match( input_val, "h")  || match( input_val, "v") || match( input_val, "u")   ||
            match( input_val, "w")  || match( input_val, "i") || match( input_val, "C")   ||
            match( input_val, "fp") || match( input_val, "oc")
            )) {
    dbg_print( debug, __func__, __LINE__, "call post_set_options()");
    opt_change = post_set_options( 0);
    print_config( opt_change);
    dbg_print( debug, __func__, __LINE__, "result post_set_options()= 0x%02x", opt_change);
    dbg_print( debug, __func__, __LINE__, "input: '%s', dec_in:%d, hex_in:%d, bin_in:%d, dec_out:%d, hex_out:%d, bin_out:%d, fixp_out:%d", input_val, dec_base_in, hex_base_in, bin_base_in, dec_base_out, hex_base_out, bin_base_out, fixpoint_out);
  }
  else if( set_syscmd) {
    dbg_print( debug, __func__, __LINE__, "call ctrl_out(%d), input_val = '%s'", newline, input_val);
    ctrl_out( newline);
  }
  else if( match( input_val,"q") || match( input_val,"Q")) {
    return( -1);
  }
  else if( ( (minus = match( input_val,"-")) && (minus >1) && (match( input_val, ":-")==0)) ||
           ( plus = match( input_val,"+"))     ||
           ( mult = match( input_val,"*"))     ||
           ( div  = match( input_val,"/"))     ||
           ( and  = match( input_val,"&"))     ||
           ( or   = match( input_val,"|"))     ||
           ( xor  = match( input_val,"^"))     ||
           ( not  = match( input_val,"~"))     ||
           ( shl  = match( input_val,"<<"))    ||
           ( shr  = match( input_val,">>"))    ||
           ( rsl  = match( input_val,"rs<"))   ||
           ( rsr  = match( input_val,"rs>")) ) {
    dbg_print( debug, __func__, __LINE__, "calculation, minus:%d, plus:%d, mult:%d, div:%d, and:%d, or:%d, xor:%d, not:%d, shl:%d, shr:%d, rsl:%d, rsr:%d", minus, plus, mult, div, and, or, xor, not, shl, shr, rsl, rsr);
    calculate( minus, plus, mult, div, and, or, xor, not, shl, shr, rsl, rsr); // calculation result is printed to string input_val
    dbg_print( debug, __func__, __LINE__, "input: '%s', dec_in:%d, hex_in:%d, bin_in:%d, dec_out:%d, hex_out:%d, bin_out:%d, fixp_out:%d", input_val, dec_base_in, hex_base_in, bin_base_in, dec_base_out, hex_base_out, bin_base_out, fixpoint_out);
    dbg_print( debug, __func__, __LINE__, "call ctrl_out(%d), input_val = '%s'", newline, input_val);
    ctrl_out( newline);
  }
  else if( (match( input_val, "options") ==1)) {
    dbg_print( debug, __func__, __LINE__, "option match input_val:'%s'", input_val);
    dbg_print( debug, __func__, __LINE__, "input: '%s', dec_in:%d, hex_in:%d, bin_in:%d, dec_out:%d, hex_out:%d, bin_out:%d, fixp_out:%d", input_val, dec_base_in, hex_base_in, bin_base_in, dec_base_out, hex_base_out, bin_base_out, fixpoint_out);
    prgcfgprint( cfg_opt);
  }
  else {
    dbg_print( debug, __func__, __LINE__, "input: '%s', dec_in:%d, hex_in:%d, bin_in:%d, dec_out:%d, hex_out:%d, bin_out:%d, fixp_out:%d", input_val, dec_base_in, hex_base_in, bin_base_in, dec_base_out, hex_base_out, bin_base_out, fixpoint_out);
    dbg_print( debug, __func__, __LINE__, "call ctrl_out(%d), input_val = '%s'", newline, input_val);
    ctrl_out( newline);
  }
  return( 1);
} // control() ----------------------------------------------------------------


//------------------------------------------------------------------------------
// set color for darker print
//------------------------------------------------------------------------------
void init_dp( char* input_val)
{
    int debug = 0;
    int c;
    dbg_print( debug, __func__, __LINE__, "0: input_val =\"%s\"\n", input_val);
    c = atoi( input_val);
    if( c > 19 || c < 0) {
      printf("\n color %d out of range, 0 <= color < 20 ", c);
      for ( c=0; c<20; c++)
        printf("\n\033[%dm color %2d", 30+c, c);
      return;
    }
    drkprnt_color = c;
    printf("\nset color for out of range bits to %d\n", drkprnt_color);
}


//------------------------------------------------------------------------------
// init fixed-point output format
//------------------------------------------------------------------------------
void init_fp( char* fixp_format)
{
    char s[5];
    char m[10];
    char f[100];
    char format[100];
    char tmps[100];
    int i = 0;
    int count = 0;
    int debug = 0;

    sprintf( format, "%s", fixp_format);
    dbg_print( debug, __func__, __LINE__, "0: input_val =\"%s\"\n", format);

    // count : in format
    for ( i = 0; i < strlen( fixp_format); i++)
      if (fixp_format[i] == ':') {
        count++;
        dbg_print( debug, __func__, __LINE__, "1: count =\"%d\"\n", count);
      }

    if( (count <1) || (count>2)) {
      printf("\n Wrong format, either s:m:f or w:f");
      return;
    }

    dbg_print( debug, __func__, __LINE__, "2: format: %s\n", (count ==3)? "s:m:f" : "w:f");

    if( count == 2) {
      // format 1:2:3, set signed or unsigned
      sprintf( s, "%s", substr( format, 0, match( format, ":") -1));
      fp_s = atoi( substr( format, 0, match( format, ":") -1));
      if(fp_s>1)
        fp_s = 1;

      // remove <signed/unsigend>: from format string
      sprintf( tmps, "%s", substr( format, match( format, ":"), 0));
      strcpy ( format, tmps);
    }
    else {
      // format 2:3, set signed = 1
      sprintf( s,"1");
      fp_s = 1;
    }

    dbg_print( debug, __func__, __LINE__, "2: s=\"%s\"", s);
    dbg_print( debug, __func__, __LINE__, "3: input_val =\"%s\"\n", format);

    // get first remaining parameter from format string
    sprintf( m, "%s", substr( format, 0, match( format, ":")-1));
    fpn_w = atoi( substr( format, 0, match( format, ":") -1));

    // remove remaing first parameter from format string
    sprintf( tmps, "%s", substr( format, match( format, ":"), 0));
    strcpy ( format, tmps);

    dbg_print( debug, __func__, __LINE__, "4: i=\"%s\"", m);
    dbg_print( debug, __func__, __LINE__, "5: input_val =\"%s\"\n", format);

    // get second remaining parameter from format string
    sprintf( f, "%s", format);
    fpn_f = atoi( format);
    dbg_print( debug, __func__, __LINE__, "6: f =\"%s\"\n", f);

    fpn_i = fpn_w -fpn_f;
    dbg_print( debug, __func__, __LINE__, "set fixed-point display to %s:%s:%s", s, m, f);

    if( (fpn_w == 0) && (fpn_i == 0) && (fpn_f == 0)) {
      fp_display = 0;
      }
    else {
      fp_display = 1;
    }
} //init_fp()-------------------------------------------------------------------


//------------------------------------------------------------------------------
// init variables (set=1), free allocated memory (set=0)
//------------------------------------------------------------------------------
void variableinit( int set)
{

}// variableinit() -------------------------------------------------------------

void print_colors()
{
  int c;
  for ( c=0; c<20; c++)
    printf("\n\033[%dm color %2d \033[0m", 30+c, c);
  return;
}


//------------------------------------------------------------------------------
// insert separator_sign as thousand separator
//------------------------------------------------------------------------------
char *ins_thousand_sep( long long n, int start) {
  static char res[504];
  char tmps[500] = "";
  int debug = 0 && dbg_enable;
  long long one     = 1;
  long long max_val_p = 0;
  long long max_val_n = 0;
  register int i;
  dbg_print( debug, __func__, __LINE__, "n: %lld, start:%d", n, start);

  if( start) {
    sprintf( res, "%s", "");
  }

  max_val_n = 0 | one << (sizeof(long long)*8 -1);

  for( i= 0; i<sizeof(long long)*8-1; i++) {
    max_val_p = max_val_p | (one << i);
  }
  dbg_print( debug, __func__, __LINE__, "max_val_p: %llx", max_val_p);
  dbg_print( debug, __func__, __LINE__, "max_val_n: %llx", max_val_n);

  dbg_print( debug, __func__, __LINE__, "         n: %22lld, tmps: \"%s\", res: \"%s\"\n", n, tmps, res);
  if (n < 0) {
    dbg_print( debug, __func__, __LINE__, "n<0");
    dbg_print( debug, __func__, __LINE__, "1n<0,    n: %22lld, tmps: \"%s\", res: \"%s\"\n", n, tmps, res);
    if( n == max_val_n) {
      n = max_val_p;
      sprintf( tmps, "-%s", ins_thousand_sep( n, 0));
    }
    else
      sprintf( tmps, "-%s", ins_thousand_sep( -n, 0));
    dbg_print( debug, __func__, __LINE__, "2n<0,    n: %22lld, tmps: \"%s\", res: \"%s\"\n", n, tmps, res);
    strcpy( res, tmps);
    dbg_print( debug, __func__, __LINE__, "3n<0,    n: %22lld, tmps: \"%s\", res: \"%s\"\n", n, tmps, res);
    return (res);
  }
  if (n < 1000) {
    dbg_print( debug, __func__, __LINE__, "an<1000, n: %22lld, tmps: \"%s\", res: \"%s\"\n", n, tmps, res);
    sprintf( tmps, "%lld", n);
    strcat( res, tmps);
    dbg_print( debug, __func__, __LINE__, "bn<1000, n: %22lld, tmps: \"%s\", res: \"%s\"\n", n, tmps, res);
    return( res);
  }
  dbg_print( debug, __func__, __LINE__, "last1   n: %22lld, tmps: \"%s\", res: \"%s\"\n", n, tmps, res);
  sprintf( tmps, "%s", ins_thousand_sep( n/1000, 0));
  dbg_print( debug, __func__, __LINE__, "last2   n: %22lld, tmps: \"%s\", res: \"%s\"\n", n, tmps, res);
  sprintf( res, "%s%c%03lld", tmps, separator_sign, n%1000);
  dbg_print( debug, __func__, __LINE__, "last4   n: %22lld, tmps: \"%s\", res: \"%s\"\n", n, tmps, res);
  return( res);
}

//------------------------------------------------------------------------------
// insert separator_sign as thousand separator (version for unsigned values)
//------------------------------------------------------------------------------
char *ins_thousand_sep_unsigned( unsigned long long n, int start) {
  static char res[500];
  char tmps[500] = "";

  if( start) {
    sprintf( res, "%s", "");
  }

  if (n < 1000) {
    sprintf( tmps, "%lld", n);
    strcat( res, tmps);
    return( res);
  }
  sprintf( tmps, "%s", ins_thousand_sep_unsigned( n/1000, 0));
  sprintf( res, "%s%c%03lld", tmps, separator_sign, n%1000);
  return( res);
}

//------------------------------------------------------------------------------
// same as sprintf but string is returned
//------------------------------------------------------------------------------
char *sformat( const char *msg, ...) {
  va_list ap;
  static char my_sprintf_string[1000];

  va_start( ap, msg);
  vsprintf( my_sprintf_string, msg, ap);
  va_end( ap);
  return( my_sprintf_string);
}

//-----------------------------------------------------------------------------
// print some debug output
//-----------------------------------------------------------------------------
void dbg_msg ( int debug, int line_nr, const char* message, ...) {
  if( debug) {
    va_list ap;
    static char msg_string[1000];

    // extern char *program_invocation_short_name;
    // fprintf( stderr, "%s: ", program_invocation_short_name);
    va_start( ap, message);
    vsprintf( msg_string, message, ap);
    va_end( ap);

    printf("\n%d: %s", line_nr, msg_string);
    printf("\ninput_val: '%s', dec_in:%d, hex_in:%d, bin_in:%d, float_in:%d, dec_out:%d, hex_out:%d, bin_out:%d, fixp_out:%d\n",
	   input_val, dec_base_in, hex_base_in, bin_base_in, float_in, dec_base_out, hex_base_out, bin_base_out, fixpoint_out);
  }
}

//-----------------------------------------------------------------------------
// another debug print  print some debug output
//-----------------------------------------------------------------------------
void dbg_print ( int debug, const char* function, int line_nr, const char* message, ...) {
  if( debug) {
    va_list ap;
    static char msg_string[1000];

    va_start( ap, message);
    vsprintf( msg_string, message, ap);
    va_end( ap);

    printf("\ndbg %s %d: %s ", function, line_nr, msg_string);
  }
}


//-----------------------------------------------------------------------------
// set selected options during execution of programme
//-----------------------------------------------------------------------------
int post_set_options ( int progstart)
{
  int debug =  0 && dbg_enable;
  int initial_call = 0;
  char new_base[100];
  int opt_change;
  opt_change = 0;
  dbg_print( debug, __func__, __LINE__, "post_set_options, progstart=%d", progstart);
  // set default after command line evaluation
  int inbase = dec_base_in +hex_base_in + bin_base_in + float_in;
  dbg_print( debug, __func__, __LINE__, "inbase count = %d", inbase);
  if( inbase >1) {
    printf( "\n*** ERROR: only one input base specification allowed\n");
    exit( 1);
  }

  if( dec_base_in == 0 &&
      hex_base_in == 0 &&
      bin_base_in == 0 &&
      float_in    == 0) {              dec_base_in = 1;  hex_base_in = 0;  bin_base_in = 0; float_in = 0; initial_call = 1; }
  else if( dec_base_in) {              dec_base_in = 1;  hex_base_in = 0;  bin_base_in = 0; float_in = 0; initial_call = 0; }
  else if( hex_base_in) {              dec_base_in = 0;  hex_base_in = 1;  bin_base_in = 0; float_in = 0; initial_call = 0; }
  else if( bin_base_in) {              dec_base_in = 0;  hex_base_in = 0;  bin_base_in = 1; float_in = 0; initial_call = 0; }
  else if( float_in)    {              dec_base_in = 0;  hex_base_in = 0;  bin_base_in = 0; float_in = 1; initial_call = 0; }
  dbg_print( debug, __func__, __LINE__, "after initial check, initial_call=%d, input_val:%s", initial_call, input_val);

  if( dec_base_out == 0 &&
      hex_base_out == 0 &&
      bin_base_out == 0 &&
      fixpoint_out == 0) {             dec_base_out = 1; hex_base_out = 1;  bin_base_out = 1; fixpoint_out = 1; initial_call = 1;}
  dbg_print( debug, __func__, __LINE__, "dec_base_out:%d, hex_base_out:%d,  bin_base_out:%d, fixpoint_out:%d, initial_call:%d", dec_base_out, hex_base_out,bin_base_out, fixpoint_out, initial_call);

  if( initial_call == 0) {
    if( match( input_val, "q")) {
      quiet = !quiet;
    }
    if(match( input_val, "odbg")) {
      gsub( "odbg", "", input_val, 1);
      print_colors();
    }
    // set during interactive execution
    if( match( input_val,"-")) {
      if(  match( input_val,"d")) {        dec_base_in = 1; hex_base_in = 0; bin_base_in = 0; float_in = 0; opt_change = opt_change | 0x1; sprintf( new_base, " set input base dec");}
      else if( match( input_val,"x")) {    dec_base_in = 0; hex_base_in = 1; bin_base_in = 0; float_in = 0; opt_change = opt_change | 0x1; sprintf( new_base, " set input base hex");}
      else if( match( input_val,"b")) {    dec_base_in = 0; hex_base_in = 0; bin_base_in = 1; float_in = 0; opt_change = opt_change | 0x1; sprintf( new_base, " set input base bin");}
      else if( match( input_val,"f") &&
               match( input_val,"fp")==0) {dec_base_in = 0; hex_base_in = 0; bin_base_in = 0; float_in = 1; opt_change = opt_change | 0x1; sprintf( new_base, " set input format floating point");}
    }

    dbg_print( debug, __func__, __LINE__, "check output base");
    int obases = 0;
    if( match( input_val,"-")) {
        if( match( input_val,"D")) obases += 8;
        if( match( input_val,"X")) obases += 4;
        if( match( input_val,"B")) obases += 2;
        if( match( input_val,"F")) obases += 1;
        dbg_print( debug, __func__, __LINE__, "obases: %d", obases);
        if( obases > 0) {
          opt_change = opt_change | 0x2;
          switch( obases) {                                                                                                                 // D X B F
          case  0: dec_base_out = 1; hex_base_out = 1; bin_base_out = 1; fixpoint_out = 1; fp_display = 1;                           break; // 0 0 0 0
          case  1: dec_base_out = 0; hex_base_out = 0; bin_base_out = 0; fixpoint_out = 1; fp_display = 1;                           break; // 0 0 0 1
          case  2: dec_base_out = 0; hex_base_out = 0; bin_base_out = 1; fixpoint_out = 0; fp_display = 0; fpn_w=0; fpn_f=0,fpn_i=0; break; // 0 0 1 0
          case  3: dec_base_out = 0; hex_base_out = 0; bin_base_out = 1; fixpoint_out = 1; fp_display = 1;                           break; // 0 0 1 1
          case  4: dec_base_out = 0; hex_base_out = 1; bin_base_out = 0; fixpoint_out = 0; fp_display = 0; fpn_w=0; fpn_f=0,fpn_i=0; break; // 0 1 0 0
          case  5: dec_base_out = 0; hex_base_out = 1; bin_base_out = 0; fixpoint_out = 1; fp_display = 1;                           break; // 0 1 0 1
          case  6: dec_base_out = 0; hex_base_out = 1; bin_base_out = 1; fixpoint_out = 0; fp_display = 0; fpn_w=0; fpn_f=0,fpn_i=0; break; // 0 1 1 0
          case  7: dec_base_out = 0; hex_base_out = 1; bin_base_out = 1; fixpoint_out = 1; fp_display = 1;                           break; // 0 1 1 1
          case  8: dec_base_out = 1; hex_base_out = 0; bin_base_out = 0; fixpoint_out = 0; fp_display = 0; fpn_w=0; fpn_f=0,fpn_i=0; break; // 1 0 0 0
          case  9: dec_base_out = 1; hex_base_out = 0; bin_base_out = 0; fixpoint_out = 1; fp_display = 1;                           break; // 1 0 0 1
          case 10: dec_base_out = 1; hex_base_out = 0; bin_base_out = 1; fixpoint_out = 0; fp_display = 0; fpn_w=0; fpn_f=0,fpn_i=0; break; // 1 0 1 0
          case 11: dec_base_out = 1; hex_base_out = 0; bin_base_out = 1; fixpoint_out = 1; fp_display = 1;                           break; // 1 0 1 1
          case 12: dec_base_out = 1; hex_base_out = 1; bin_base_out = 0; fixpoint_out = 0; fp_display = 0; fpn_w=0; fpn_f=0,fpn_i=0; break; // 1 1 0 0
          case 13: dec_base_out = 1; hex_base_out = 1; bin_base_out = 0; fixpoint_out = 1; fp_display = 1;                           break; // 1 1 0 1
          case 14: dec_base_out = 1; hex_base_out = 1; bin_base_out = 1; fixpoint_out = 0; fp_display = 0; fpn_w=0; fpn_f=0,fpn_i=0; break; // 1 1 1 0
          case 15: dec_base_out = 1; hex_base_out = 1; bin_base_out = 1; fixpoint_out = 1; fp_display = 1;                           break; // 1 1 1 1
          }
        }
        if( (fp_display ==1) && (fpn_w == 0) && (fpn_i == 0) && (fpn_f == 0)) {
          fp_display = 0;
          printf("\n*** ERROR: No fixed point format defined (wl=0, fl=0). Set fixed point format via -fp <sgn:wl:fl> to enable fixed point output");
        }
      }
  }

  dbg_print( debug, __func__, __LINE__, "after set output base type");

  // execute this only in interactive mode, not directly at program start
  if( progstart == 0) {
    if(match( input_val,"h") && (match( input_val,"help") == 0)) {
      verbose_help = 0;
      call_help( 0);
    }

    if(match( input_val,"help")) {
      verbose_help = 1;
      call_help( 0);
    }

    if(match( input_val, "u")) {
      opt_change = opt_change | 0x4;
      usigned = !usigned;
    }

    if(match( input_val, "sat")) {    // saturation on off
      opt_change = opt_change | 0x8;
     saturation  = !saturation;
     }
    else if(match( input_val, "s")) { // set digit separation on, off
      opt_change = opt_change | 0x10;
      digit_separate = !digit_separate;
    }

    if(match( input_val, "z")) {
      opt_change = opt_change | 0x20;
      leading_zero = !leading_zero;
    }

    if(match( input_val, "c") && (match( input_val,"oc")==0)) {
      set_separator = 1;
      dbg_print( debug, __func__, __LINE__, "set_seperaor =1, input_val = \"%s\"", input_val);
    }
    else
      set_separator = 0;

    if(match( input_val, "v")) {
      verbose = !verbose;
      opt_change = opt_change | 0x80;
    }

    if(match( input_val, "w")) {
      set_width = 1;
    }
    else
      set_width = 0;

    // settings for fixed-point width
    if(match( input_val, "fp")) {
      //opt_change = opt_change | 0x2;
      set_fp_format = 1;
    }
    else
      set_fp_format = 0;


    if(match( input_val, "oc")) // dark print color
      set_dp_format = 1;
    else
      set_dp_format = 0;


    if(match( input_val, "C"))
      set_syscmd = 1;
    else
      set_syscmd = 0;
  }

  if(match( input_val, "i")) {
    print_config( 0x1ff);
    opt_change = 0;
  }

  return opt_change;
} // post_set_options() -------------------------------------------------------


//-----------------------------------------------------------------------------
// print info
//-----------------------------------------------------------------------------
void print_config( int opt_change)
{
  char fixpstr [100];
  char sep_str[10];
  char newline [10];
  int  firstp;

  if(opt_change > 0) {
    firstp = 0;
    if( piped )
      newline[0] = '\0';
    else
      sprintf( newline, "\n");

    sprintf( fixpstr, "fixp (sgn:%d, wl:%d, fl:%d, il:%d)", fp_s, fpn_w, fpn_f, (fpn_i>0)? fpn_i:0);
    sprintf( sep_str, "'%c'", separator_sign);
    if( opt_change & 0x01) { if( firstp) sprintf( newline, "\n"); firstp =1; printf("%s  input value          : %s", newline, (dec_base_in)? "dec": ((hex_base_in)? "hex": ((bin_base_in)? "bin": ((float_in && fp_display)? "floating point": ((float_in)? "floating point, WARNING fixed point conversion parameter not set -fp" : "")) ))); }
    if( opt_change & 0x02) { if( firstp) sprintf( newline, "\n"); firstp =1; printf("%s  output format        : %s%s%s%s", newline,
                                                                                    (dec_base_out)? "dec, "  : "",
                                                                                    (hex_base_out)? "hex, "  : "",
                                                                                    (bin_base_out)? "bin, "  : "",
                                                                                    (fp_display)?   fixpstr  : ""); }
    if( opt_change & 0x100) { if( firstp) sprintf( newline, "\n"); firstp =1; printf("%s  fixed point format   : %d:%d:%d", newline, fp_s, fpn_w, fpn_f); }
    if( opt_change & 0x004) { if( firstp) sprintf( newline, "\n"); firstp =1; printf("%s  unsigned integer     : %s", newline, (usigned)? "on": "off"); }
    if( opt_change & 0x008) { if( firstp) sprintf( newline, "\n"); firstp =1; printf("%s  saturation           : %s", newline, (saturation)? "on" : "off"); }
    if( opt_change & 0x010) { if( firstp) sprintf( newline, "\n"); firstp =1; printf("%s  digit separation     : %s", newline, (digit_separate)? sep_str : "off"); }
    if( opt_change & 0x020) { if( firstp) sprintf( newline, "\n"); firstp =1; printf("%s  show leading zeros   : %s", newline, (leading_zero)? "on" : "off"); }
    if( opt_change & 0x040) { if( firstp) sprintf( newline, "\n"); firstp =1; printf("%s  ring shift bit width : %d", newline, width); }
    if( opt_change & 0x080) { if( firstp) sprintf( newline, "\n"); firstp =1; printf("%s  verbose message      : %s", newline, (verbose)? "on": "off"); }
    if( piped && (quiet == 0))
      printf("\n");
  }
} //print_config() ------------------------------------------------------------


//-----------------------------------------------------------------------------
// call system command
//-----------------------------------------------------------------------------
int print_system_call_result( char* syscmd)
{
  char tmps[1000]  = "";
  char cbopt[1000] = "";
  FILE *fp;
  char rv[1035];   // command return value
  int debug = 0 && dbg_enable;
  int flp   = 0;

  if( match( syscmd, ".")) {
    if (debug) { printf("\n detected floating point operation: %s, position:%d", syscmd, match( syscmd, "."));}
    flp = 1;
    sprintf( cbopt, "-lq");
  }
  sprintf( tmps, "echo \"%s\" | bc %s", syscmd, cbopt);
  if (debug) { printf("\n call system command: %s\n", tmps);}
  //system( tmps);
  //sprintf( input_val, " ");

  // get result from system call:
  fp = popen( tmps, "r"); // open the command for reading.
  if (fp == NULL) { printf("failed to run command: %s\n", tmps);}

  // read the output, a line at a time and print return value
  while ( fgets( rv, sizeof( rv), fp) != NULL) {
    printf("%s", rv);
  }
  pclose( fp); // close
  sprintf( input_val, "%s", rv);
  dbg_print( debug, __func__, __LINE__, "input value after system call = %s, flp:%d", input_val, flp);
  return( flp);
} // print_system_call_result() ------------------------------------------------


//------------------------------------------------------------------------------
// convert fraction value into bin fixed-point representation
//------------------------------------------------------------------------------
unsigned long long llfrac_to_fixpoint( long long f_val, int decexp)
{
  int debug = 0;
  char dbgstring[100];
  int i;
  double f_val_int_d = f_val;
  long long bit_pattern = 0x0000000000000000;
  float  calc_resultf;
  float  diff; // difference between given f_val and fixed-point bit_pattern
  float  diff_store;
  char tmps[100] = "";
  diff = 0;

  if (debug) { printf("\n%d: llfrac_to_fixpoint %lld, %d: ", __LINE__, f_val, decexp); }
  for( i=1; i<= fpn_f; i++) {
    calc_resultf = f_val_int_d/(pow( 2, -i) *pow( 10, decexp));

    if( (calc_resultf >=1) ||
        ((i>12) &&( (calc_resultf +0.001) >=1) ) ||
        ((i>24) &&( (calc_resultf +0.01) >=1) )
        ) {
      strcat( tmps, "1");
      f_val_int_d = f_val_int_d - (pow( 2, -i) *pow( 10, decexp));
      if (debug) { sprintf( dbgstring, ", set bit_pattern[%d] (line %d)", (fpn_f-i),  __LINE__); }
    }
    else {
        strcat( tmps, "0");
        if (debug) { dbgstring[0] = '\0'; }
    }
    diff = f_val_int_d * pow( 10, -decexp);
    if (debug) { printf("\n%d: llfrac_to_fixpoint i=%2d, f_val=%lld, f_val_int: %f, calc. result = %20.20f, bit_pattern = %s = 0x%llx diff: %20.20f %s", __LINE__, i, f_val, f_val_int_d, calc_resultf, tmps,  bin2dec(tmps), diff, dbgstring);}
  }

  calc_resultf = f_val_int_d/(pow( 2, -i) *pow( 10, decexp));
  //diff = f_val_int_d * pow( 10, -decexp);
  if (debug) { printf("\n%d: llfrac_to_fixpoint i=%2d, f_val=%lld, f_val_int: %f, calc. result = %20.20f, bit_pattern = 0x%llx, diff: %f*10^(-%d)=%20.20f", __LINE__, i, f_val, f_val_int_d, calc_resultf, bin2dec(tmps), f_val_int_d, decexp, diff); }

  bit_pattern = bin2dec(tmps);
  diff_store =  diff;

  if( calc_resultf >= 0.5) {
    bit_pattern++;

    double cmp_v = 0;
    for( i=0; i<fpn_f; ++i) {
      cmp_v += ((bit_pattern >>(fpn_f-i)) & 0x1)? pow( 2, -i): 0;
    }

    diff = cmp_v -f_val* pow( 10, -decexp);
    if (debug) { printf("\n%d: llfrac_to_fixpoint i=%2d, f_val=%lld, f_val_int: %f, calc. result = %20.20f, bit_pattern = 0x%llx diff: %f*10^(-%d)=%20.20f (rounded up)", __LINE__, i, f_val, f_val_int_d, calc_resultf, bit_pattern, f_val_int_d, decexp, diff); }

    if( diff <0)
      diff = -1* diff;

    if( diff_store <0)
      diff_store = -1* diff_store;

    if( diff_store < diff) {
      bit_pattern--;
      if (debug) { printf("\n%d: llfrac_to_fixpoint i=%2d, f_val=%lld, f_val_int: %f, calc. result = %20.20f, bit_pattern = 0x%llx diff: %20.20f (revert rounded up)", __LINE__, i, f_val, f_val_int_d, calc_resultf, bit_pattern, diff_store); }
    }

  }
  if (debug) { printf("\n%d: llfrac_to_fixpoint bit_pattern = 0x%llx, diff: %20.20f", __LINE__, bit_pattern, diff); }

  return bit_pattern;

}// llfrac_to_fixpoint() ------------------------------------------------------

//-----------------------------------------------------------------------------
// real to fixpoint conversion, separated in integer an fractional part
// This is a mess. Someone should clan this up ;-)
//-----------------------------------------------------------------------------
void prep_conv_real_to_fixpoint( char* real_val) {
  register int i;
  int debug = 0 && dbg_enable;
  char int_str[100];
  char frac_str[100];
  char input_val_save[100];
  char input_val_int[100];
  char input_val_frac[100];
  char sign_str[10];
  long long int_value;
  long long frac_value;
  long long res_int_value;
  long long min_int_value = 0;
  long long max_int_value = 0;
  int fp_int_ovflw = 0;
  int fp_frac_ovflw = 0;
  unsigned long long fixp2real_max_val_int  =0;
  unsigned long long fixp2real_max_val_frac =0;

  sprintf( input_val_save, "%s", real_val);
  sign_str[0] = '\0';

  sprintf( int_str,  "0.0");
  sprintf( frac_str, "0.0");
  if( match( real_val, "-"))
    sprintf( sign_str, "-");

  if( match( real_val, ".")>1) {
    sprintf( int_str,  "%s.0", substr( real_val, 0, match( real_val, ".")-1));
    sprintf( frac_str, "%s0.%s", sign_str, substr( real_val, match( real_val, "."), 0));
  }
  else if (match( real_val, ".") == 1)
    sprintf( frac_str, "%s0.%s", sign_str, substr( real_val, match( real_val, "."), 0));
  else
    sprintf( int_str,  "%s.0", substr( real_val, match( real_val, "."), 0));

  sprintf( f_input_val_str, "%s", input_val);
  dbg_print( debug, __func__, __LINE__, "save original input value: %s, real_val: %s", f_input_val_str, real_val);
  dbg_print( debug, __func__, __LINE__, "int string : %s", int_str);
  dbg_print( debug, __func__, __LINE__, "frac string: %s", frac_str);

  // integer part
  saturated = 0;
  fixp2real_max_val_int = conv_real_to_fixpoint( int_str, 0);
  int_value = atoll( input_val);
  sprintf( input_val_int, "%s", input_val);
  fp_int_ovflw = fp_range_ovflw;
  dbg_print( debug, __func__, __LINE__, "converted integer part   : %lld, overlofw: %d", int_value, fp_int_ovflw);

  // fractional part
  if( (saturation ==0) || (saturation && (fp_int_ovflw == 0)))
    fixp2real_max_val_frac = conv_real_to_fixpoint( frac_str, 1);
  sprintf( input_val_frac, "%s", input_val);
  frac_value = atoll( input_val);
  fp_frac_ovflw = fp_range_ovflw;
  dbg_print( debug, __func__, __LINE__, "converted fractional part: %lld, overlofw: %d, max. val. fryc: %lld", frac_value, fp_frac_ovflw, fixp2real_max_val_frac);

  if( saturation && fp_int_ovflw)
    res_int_value = int_value;
  else if ( saturation && fp_frac_ovflw)
    res_int_value = frac_value;
  else
    res_int_value = int_value + frac_value;

  fp_range_ovflw = fp_int_ovflw || fp_frac_ovflw;

  //----------------------------------------------------------------------------
  // new
  //----------------------------------------------------------------------------
  if( saturation && (saturated==0) && (fp_range_ovflw==0) && (res_int_value > fixp2real_max_val_int)) {
    if( fp_s) {
      for( i=63; i>=fpn_w-1; i--) {
        min_int_value = min_int_value | ((unsigned long long)0x1)<<i;
      }
      for( i=0; i<fpn_w-1; i++)
        max_int_value = max_int_value | ((unsigned long long)0x1)<<i;
    }
    else {
      min_int_value = 0;
      for( i=0; i<fpn_w; i++)
        max_int_value = max_int_value | ((unsigned long long)0x1)<<i;
    }
    if( res_int_value < min_int_value)  dbg_print( debug, __func__, __LINE__, "DEBUG: res < min, 0x%llx < 0x%llx", res_int_value,  min_int_value);
    if( res_int_value > min_int_value)  dbg_print( debug, __func__, __LINE__, "DEBUG: res > min, 0x%llx > 0x%llx", res_int_value,  min_int_value);
    if( res_int_value < max_int_value)  dbg_print( debug, __func__, __LINE__, "DEBUG: res < max, 0x%llx < 0x%llx", res_int_value,  max_int_value);
    if( res_int_value > max_int_value)  dbg_print( debug, __func__, __LINE__, "DEBUG: res > max, 0x%llx < 0x%llx", res_int_value,  max_int_value);

    if( ( res_int_value < min_int_value) ||  res_int_value > max_int_value) {
      // check sign
      if((res_int_value>>63)&& 0x1) {
        if( fp_s )  { // signed bit set
          res_int_value = (unsigned long long)0x1<< (fpn_w -1);
          dbg_print( debug, __func__, __LINE__, "DEBUG: sest to min. signed value:  %lld", res_int_value);
        }
        else {
          res_int_value = 0;
          dbg_print( debug, __func__, __LINE__, "DEBUG: sest to min. unsigned value: %lld", res_int_value);
        }
      }
      else {
        dbg_print( debug, __func__, __LINE__, "DEBUG: fix positive saturation, set value to max. value: %llx", fixp2real_max_val_int);
        res_int_value = fixp2real_max_val_int;
      }
      fp_range_ovflw = 1;
      saturated      = 1;
    }
  }

  sprintf( input_val, "%lld", res_int_value);
  dbg_print( debug, __func__, __LINE__, "int value : %lld", int_value);
  dbg_print( debug, __func__, __LINE__, "frac value: %lld", frac_value);
  dbg_print( debug, __func__, __LINE__, "int + frac: %lld", res_int_value);
  dbg_print( debug, __func__, __LINE__, "range ovfw: %d", fp_range_ovflw);
  dbg_print( debug, __func__, __LINE__, "saturated : %d\n", saturated);
}// prep_conv_real_to_fixpoint()-----------------------------------------------

//-----------------------------------------------------------------------------
// convert real value to fixed-point format
// Calculate realb = realv * 2^^F and round to nearest integer,
// where F is the fractional length of the variable.
// max granularity:
// 1*2^-64 = 0.00000000000000000005
// max value:
// 1*2^-64 = 18446744073709551615
// // This is a mess. Someone should clan this up ;-)
//-----------------------------------------------------------------------------
unsigned long long conv_real_to_fixpoint( char* real_val, int frac)
{
  int debug = 0 && dbg_enable;
  int correct_overflow = 1;
  register int i;
  long long realv;      // the real value as long long
  long double realv_ld; // the real value as long double
  long long realb;      // the real value converted to bin representation in fixed-point format
  long long sign;
  unsigned long long fixp2real_max_val=0;
  long long fixp2real_min_val=0;
  char int_str[100];
  char frac_str[100];
  realv = 0;
  sprintf( int_str,  "0");
  sprintf( frac_str, "0");


  if (debug) { printf("\n\ndbg %s %d: conv real value %s to fixed-point format ", __func__, __LINE__, real_val); }

  if( (fpn_i == 0) && fpn_f == 0 ) {
    printf("\n *** ERROR fixed-point format not set correctly: %d:%d:%d", fp_s, fpn_i, fpn_f);
    printf("\n           set fixed-point format via -fp <s:i:f>\n");
    sprintf( input_val, " ");
  }
  else {
    // set min and max value
    fixp2real_min_val = 0;
    fixp2real_max_val = 0x1;

    if( fp_s ) { // signed bit set
      fixp2real_min_val = (unsigned long long)0x1<< (fpn_w -1);
      dbg_print( debug, __func__, __LINE__, "set min_val: %16llx", fixp2real_min_val);
      for( i=0; i< fpn_w-1; i++)
        fixp2real_max_val = fixp2real_max_val | (unsigned long long)0x1<<i;
      dbg_print( debug, __func__, __LINE__, "set max_val: %16llx", fixp2real_max_val);
    }
    else
      for( i=0; i< fpn_w; i++)
        fixp2real_max_val = fixp2real_max_val | (unsigned long long)0x1<<i;
    dbg_print( debug, __func__, __LINE__, "set fp max. val : %16llx = %6lld  ", fixp2real_max_val, fixp2real_max_val);
    dbg_print( debug, __func__, __LINE__, "set fp min. val : %16llx = %6lld   (valid bits: [%d:0])", (unsigned long long) fixp2real_min_val, fixp2real_min_val, fpn_w-1);

    conv_granularity = (long double)1/pow( 2, fpn_f);
    dbg_print( debug, __func__, __LINE__, "conversion granularity = %.32Lf", conv_granularity);

    if(  match( real_val, ".")>1) {
      sprintf( int_str,  "%s", substr( real_val, 0, match( real_val, ".")-1));
      sprintf( frac_str, "%s", substr( real_val, match( real_val, "."), 0));
    }
    else if (match( real_val, ".") == 1)
      sprintf( frac_str, "%s", substr( real_val, match( real_val, "."), 0));
    else
      sprintf( int_str,  "%s", substr( real_val, match( real_val, "."), 0));

    dbg_print( debug, __func__, __LINE__, "decimal p. pos : %d", match( real_val, "."));
    dbg_print( debug, __func__, __LINE__, "integer part   : %s", int_str);
    dbg_print( debug, __func__, __LINE__, "fractional part: %s", frac_str);

    // detect sign
    // integer part
    realv = atoll( real_val); // this gives just the integer, no fraction
    dbg_print( debug, __func__, __LINE__, "realv: %lld", realv);

    // detect sign
    sign = 1;
    if( ((long double) atof( real_val)) < 0)
      sign = -1;
    dbg_print( debug, __func__, __LINE__, "sign : %d", sign);

    realb    = (long double)atof(real_val)*(long double)pow( 2, fpn_f);
    realv_ld = (long double)atof(real_val)*(long double)pow( 2, fpn_f);
    dbg_print( debug, __func__, __LINE__, "fixpnt = %.32Lf/%.32Lf = %.32Lf --> realb = %lld = 0x%llx", (long double)atof(real_val), conv_granularity, realv_ld, realb, realb);

    // round ?
    if ( fmodl(realv_ld,(long double)1.0)) {
      if (sign ==1) {
        if(fmodl(realv_ld,(long double)1.0)>(long double) 0.5)
          realb++;
      }
      else {
        if(fmodl(realv_ld,(long double)1.0)<(long double) -0.5)
          realb--;
      }
      dbg_print( debug, __func__, __LINE__, "round = %Lf mod 1 = %Lf --> realb =%ld", realv_ld, fmodl(realv_ld,(long double)1.0), realb);
    }

    //----------------------
    // first overflow check
    fp_range_ovflw = 0;  // clear conversion overflow flag
    dbg_print( debug, __func__, __LINE__, "realb  : %16llx", realb);
    dbg_print( debug, __func__, __LINE__, "min_val: %16llx", fixp2real_min_val);
    dbg_print( debug, __func__, __LINE__, "max_val: %16llx", fixp2real_max_val);
    // correct overflow
    if( (realb > fixp2real_max_val) && (sign == 1)) {
      fp_range_ovflw = 1; // flags range overflow during real to fixed-point conversion
      dbg_print( debug, __func__, __LINE__, "set fp_range_ovflw: realb > fixp2real_max_val, %16llx > %16llx", realb, fixp2real_max_val);
      if( saturation) {
        if( correct_overflow == 1) {
          realb =  fixp2real_max_val;
          //realb =  fixp2real_min_val;
          saturated = 1;
        }
        dbg_print( debug, __func__, __LINE__, "correct pos. overflow,  max:  %16llx, realb  %16llx", fixp2real_max_val, realb);
        //dbg_print( debug, __func__, __LINE__, "correct pos. overflow,  min:  %16llx, realb  %16llx", fixp2real_min_val, realb);
      }
    }
    // ------------------------------------------------------------------------------>
    //if( (realb > fixp2real_min_val) && (sign == -1)) {
    if( ( (realb < -fixp2real_min_val) || (realb > fixp2real_min_val)) && (sign == -1)) {
      fp_range_ovflw = 1; // flags range overflow during real to fixed-point conversion
      dbg_print( debug, __func__, __LINE__, "set fp_range_ovflw: realb < fixp2real_min_val, 0x%16llx < 0x%16llx,  %lld <  %lld", realb, fixp2real_min_val, realb, fixp2real_min_val);
      if( saturation) {
        if( correct_overflow == 1) {
          realb =  fixp2real_min_val;
        }
        dbg_print( debug, __func__, __LINE__, "correct neg. overflow,  min:  %16llx, realb  0x%16llx", fixp2real_min_val, realb);
      }
    }

    // check for rounding if not already at limit
    if( ((fp_range_ovflw == 0) || ((fp_range_ovflw == 1) && (saturation==0))) && (realb != fixp2real_max_val) && (realb != fixp2real_min_val) ) {
      // round up
      if( ((long long)atof(real_val)*(long long)pow( 2, fpn_f) -realb >= 0.5) && ((long long)atof(real_val) *(long long)pow( 2, fpn_f) -realb < 1.0)) {
        dbg_print( debug, __func__, __LINE__, "round up: %f <= %f <%f", 0.5,     (long long)atof(real_val) *(long long)pow( 2, fpn_f) -realb, 1.0);
        ++realb;
        dbg_print( debug, __func__, __LINE__, "round up:  0x%16llx", realb);
      }
      // round down
      else if( atof(real_val)*pow( 2, fpn_f) -realb <= -0.5) {
        --realb;
        dbg_print( debug, __func__, __LINE__, "round down:  0x%16llx", realb);
      }

      //----------------------
      // 2nd overflow check
      // correct overflow
      if( (realb > fixp2real_max_val) && (sign == 1)) {
        fp_range_ovflw = 1; // flags range overflow during real to fixed-point conversion
        dbg_print( debug, __func__, __LINE__, "set fp_range_ovflw: realb > fixp2real_max_val, %16llx <  %16llx", realb, fixp2real_max_val);
        if( saturation) {
          dbg_print( debug, __func__, __LINE__, "correct pos. overflow,  max:  %16llx, realb  %16llx", fixp2real_max_val, realb);
          if( correct_overflow == 1) {
            realb =  fixp2real_max_val;
          }
        }
      }
      // ------------------------------------------------------------------------------>
      if( (realb > fixp2real_min_val) && (sign == -1)) {
      //if( (realb < fixp2real_min_val) && (sign == -1)) {
        fp_range_ovflw = 1; // flags range overflow during real to fixed-point conversion
        dbg_print( debug, __func__, __LINE__, "set fp_range_ovflw: realb < fixp2real_min_val, 0x%16llx < 0x%16llx", realb, fixp2real_min_val);
        if( saturation) {
          dbg_print( debug, __func__, __LINE__, "correct neg. overflow,  min:  %16llx, realb  %16llx", fixp2real_min_val, realb);
          if( correct_overflow == 1) {
            realb =  fixp2real_min_val;
          }
        }
      }
    }

    conv_real_fp_s  = sign;
    conv_real_fp    = realv;
    sprintf( conv_real_val, "%s", real_val);
    fp_conv_fr_real = 1;
    dbg_print( debug, __func__, __LINE__, "real val string: %s", conv_real_val);

    if( dec_base_in || float_in || float_in_tmp_after_calc) {       // input is dec
      if( usigned && realb<0)
        sprintf( input_val, "%lld", -realb);
      else
        sprintf( input_val, "%lld", realb);
      dbg_print( debug, __func__, __LINE__, "set input_val: %s", input_val);
    }
    else if( hex_base_in ) {             // input is hex
      sprintf( input_val, "%llx", realb);
      dbg_print( debug, __func__, __LINE__, "set input_val: %s", input_val);
    }
    else if( bin_base_in) {             // input is bin
      printf("\n*** ERROR, function currently not supported,  set input format to dec (-d) or hex (-x)\n");
      sprintf( input_val, " ");
    }
  }

  dbg_print( debug, __func__, __LINE__, "fp_range_ovflw    : %d", fp_range_ovflw);
  dbg_print( debug, __func__, __LINE__, "end, set input_val: %s", input_val);

  return( fixp2real_max_val);
} //conv_real_to_fixpoint() ---------------------------------------------------


//-----------------------------------------------------------------------------
// hexadecimal to decimal conversion, argument is string
//-----------------------------------------------------------------------------
long long hex2dec(char* hex_val)
{
  int i, digits;
  long long dec_val = 0;
  digits = strlen(hex_val);
  for (i =0; i < digits ; i++)  {
    if( hex_val[i] != ' ')
      dec_val = dec_val*16;
    if( hex_val[i] == '0') dec_val +=0;
    else if( hex_val[i] == '1') dec_val +=1;
    else if( hex_val[i] == '2') dec_val +=2;
    else if( hex_val[i] == '3') dec_val +=3;
    else if( hex_val[i] == '4') dec_val +=4;
    else if( hex_val[i] == '5') dec_val +=5;
    else if( hex_val[i] == '6') dec_val +=6;
    else if( hex_val[i] == '7') dec_val +=7;
    else if( hex_val[i] == '8') dec_val +=8;
    else if( hex_val[i] == '9') dec_val +=9;
    else if( hex_val[i] == 'a') dec_val +=10;
    else if( hex_val[i] == 'A') dec_val +=10;
    else if( hex_val[i] == 'b') dec_val +=11;
    else if( hex_val[i] == 'B') dec_val +=11;
    else if( hex_val[i] == 'c') dec_val +=12;
    else if( hex_val[i] == 'C') dec_val +=12;
    else if( hex_val[i] == 'd') dec_val +=13;
    else if( hex_val[i] == 'D') dec_val +=13;
    else if( hex_val[i] == 'e') dec_val +=14;
    else if( hex_val[i] == 'E') dec_val +=14;
    else if( hex_val[i] == 'f') dec_val +=15;
    else if( hex_val[i] == 'F') dec_val +=15;
    else if( hex_val[i] ==  separator_sign) {dec_val = dec_val/16;}
    else if(hex_val[i] != ' ') {
      printf ("\ninvalid argument for hex to dec conversion: %s --> %c\n",
              hex_val, hex_val[i]);
      return -1;
    }
  }
return (dec_val);
} // hex2dec() -----------------------------------------------------------------



//------------------------------------------------------------------------------
// binary to decimal conversion, argument is string, returns integer
//-----------------------------------------------------------------------------
long long bin2dec(char* bin_value)
{
  int i, temp, digits;
  long long  dec_val =0;
  int debug = 0;
  dbg_print( debug, __func__, __LINE__, "bin2dec: '%s'", bin_value);

  // remove trailing spaces
  digits = strlen(bin_value);
  while( bin_value[digits-1] == ' ') { // remove leading spaces
    digits --;
  }

  for( i =0; i < digits ; i++) {
    dec_val = dec_val*2;
    temp=0;
    if( bin_value[i] == '0')      temp=0;
    else if( bin_value[i] == '1') temp=1;
    else if( bin_value[i] == separator_sign) {dec_val = dec_val/2;}
    else {
      printf ("\ninvalid argument for bin to hex conversion: \"%s\" -->\"%c\"\n",
              bin_value, bin_value[i]);
      return -1;
    }
    dec_val = dec_val + temp;
    //printf("\n b2d: bin_value[%d]=%d, dec_val: %d", i, temp, dec_val);
  }

  return( dec_val);
} //bin2dec() -----------------------------------------------------------------



//------------------------------------------------------------------------------
// decimal to hexadecimal conversion and  output, argument is integer
//-----------------------------------------------------------------------------
void hexout( int print, long long dec_val)
{
  register int i;
  int k;
  int print_length = 20;
  char formatstr [20]     = "\0";
  char space_str [20]     = "\0";
  char space_str_cpy [20] = "\0";
  char res[100]     = "\0";
  char res_cpy[100] = "\0";
  char tmps[100]    = "\0";
  int oorbits = 0;
  int oornibbles = 0;
  char dp[100] = "\0";
  int pl =0; // real print length
  int nibbles_req = 0;
  int first_one;
  int print_fpn_w;
  long long one     = 1;
  long long max_val = 0;
  unsigned long long u_max_val = 0;
  int debug = 0 && dbg_enable;

  print_fpn_w = fpn_w;
  if( fpn_f > fpn_w)
    print_fpn_w = fpn_f;

  if( no_colored_output ==0)
    sprintf( dp, "\033[%dm", 30+drkprnt_color);

  if( fpn_w > 0) {
    oorbits    = sizeof(long long)*8 -fpn_w;
    oornibbles = oorbits/4;

    // corner case
    if( fpn_w == sizeof(long long)*8) {
      for( i= 0; i<sizeof(long long)*8; i++) {
        max_val = max_val | (one << i);
      }
    }
    else
      max_val = (one<<fpn_w)-1;
    u_max_val = max_val;

    if (dec_val<0) {
      nibbles_req = sizeof(long long)*2;
      dbg_print( debug, __func__, __LINE__, "value: %lld, max.val: %lld (unsigned: %llu), fpn_w: %d, fpn_f: %d, nibbles_req: %d", dec_val, max_val, u_max_val, fpn_w, fpn_f, nibbles_req);
    }
    else if( (print_fpn_w == sizeof(long long)*8) || (dec_val == u_max_val)) {
      nibbles_req = sizeof(long long)*2; // size in bytes, one byte = 2 nibbles
      dbg_print( debug, __func__, __LINE__, "value: %lld, max.val: %lld (unsigned: %llu), fpn_w: %d, fpn_f: %d, nibbles_req: %d", dec_val, max_val, u_max_val, fpn_w, fpn_f, nibbles_req);
    }
    else if (dec_val==0) {
      if( print_fpn_w > 0)
        nibbles_req = ceil(print_fpn_w/4.0);
      else
        nibbles_req = 1;
      dbg_print( debug, __func__, __LINE__, "value: %lld, max.val: %lld (unsigned: %llu), fpn_w: %d, fpn_f: %d, nibbles_req: %d", dec_val, max_val, u_max_val, fpn_w, fpn_f, nibbles_req);
    }
    else {
      for( first_one= sizeof(long long)*8-1 ; !((dec_val>>first_one) && 0x1); first_one--) ;
      nibbles_req = (int)ceil((first_one+1)/4.0);
      if( print_fpn_w > 0 && (ceil(print_fpn_w/4.0)>nibbles_req))
        nibbles_req = ceil(print_fpn_w/4.0);
      dbg_print( debug, __func__, __LINE__, "value: %lld, max.val: %lld (unsigned: %llu), fpn_w: %d, fpn_f: %d, fist_one: %d nibbles_req: %d", dec_val, max_val, u_max_val, fpn_w, fpn_f, first_one, nibbles_req);
    }
  }

  // result string with fixed length
  if( leading_zero) {
    sprintf( res, "%016llX", dec_val);
    dbg_print( debug, __func__, __LINE__, "res: %s", res);
  }
  else if ( fpn_w>0) {
    sprintf( formatstr, "%%s%%0%dllX", nibbles_req);
    for( k=0; k<16-nibbles_req; k++) {
      sprintf( space_str_cpy, "%s%c", space_str, ' ');
      strcpy( space_str, space_str_cpy);
  }
    sprintf( res, formatstr, space_str, dec_val);
    dbg_print( debug, __func__, __LINE__, "res: %s, nibbles_req: %d", res, nibbles_req );
  }
  else {
    sprintf( res, "%16llX", dec_val);
    dbg_print( debug, __func__, __LINE__, "res: %s", res);
  }

  dbg_print( debug, __func__, __LINE__, "res: %s, max_val: %lld, requ.nibble: %d, oorbits: %d, oornibbles: %d",res, max_val, nibbles_req, oorbits, oornibbles);

  if( digit_separate) {
    // 0000_0000_0000_000A
    print_length = 4*4 +3; // 64 bit max. 4 chunks of 4 hex digit + 3 spaces between chunks
    strcpy( tmps, res);
    res[0] = '\0';
    for( k=0; k<4; k++) {
      for( i=k*4; i<k*4+4; i++) {   // print 4 hexdigits
        // check value range 2^(64-k*4*4) <= dec_val <2^(64-k*4*4-4)
        dbg_print( debug, __func__, __LINE__, "k:%d, i:2%d, oorb:%2d,  %s",k, i, oorbits, res);
        if(  oorbits > 3)
           strcat( res, dp);
        else
          if( no_colored_output ==0)
            strcat( res, "\033[0m");
        sprintf( res_cpy, "%s%c", res, tmps[i]);
        strcpy( res, res_cpy);
        ++pl;
        oorbits -= 4;
        dbg_print( debug, __func__, __LINE__, "k:%d, i:2%d, oorb:%2d,  %s", k, i, oorbits, res);
      }
      if( (k<3) && (tmps[ i-1] != ' ')) {
        sprintf( res_cpy, "%s%c", res, separator_sign);
        strcpy( res, res_cpy);
        ++pl;
      }
    }

    // string with spaces to match complete length for hex output
    tmps[0] = '\0';
    for( i=0; i< print_length-pl; i++) {
      strcat( tmps, " ");
    }

    // print hex value
    printf("%s%s%s   ", (debug)? "\n":"", tmps, res);
  }
  else {
    if( no_colored_output==0)
      printf( "%s", dp);
    for( i=0; i<strlen(res); i++) {
      dbg_print( debug, __func__, __LINE__, "i:2%d, oornibbles:%2d, hexout nibble: ", i, oornibbles);
      if( no_colored_output==0)
        if( oornibbles ==0 )
          printf("\033[0m");
       printf("%c", res[i]);
       --oornibbles;
     }
     printf("  ");
  }
} // hexout() ------------------------------------------------------------------


//------------------------------------------------------------------------------
// decimal to binary conversion and output, argument is integer
//------------------------------------------------------------------------------
void binout( int print, long long dec_val)
{
  register int i;
  int first_one;
  char tmps[100];
  char tmps_cpy[100];
  bin_val_glb[0]    = '\0';
  int oorbits       = 0; // out of range bits
  long long max_val = 0;
  unsigned long long u_max_val = 0;
  long long one     = 1;
  int print_fpn_w;
  int bits_req      = fpn_w;
  int bit_printed   = 0;
  int debug         = 0 && dbg_enable;

  long long ll_max = 0;
  long long ll_min = 0;
  for( i= 0; i<sizeof(long long)*8-1; i++) {
      ll_max = ll_max | (one << i);
  }
  ll_min = 0 | (one << (sizeof(long long)*8-1));
  dbg_print ( debug, __func__, __LINE__, "max ll :%llx, min ll: %llx", ll_max, ll_min);

  // corner case
  if( fpn_w == sizeof(long long)*8) {
    for( i= 0; i<sizeof(long long)*8; i++) {
      max_val = max_val | (one << i);
    }
  }
  else
    max_val = (one<<fpn_w)-1;
  u_max_val = max_val;

  print_fpn_w = fpn_w;
  if( fpn_f > fpn_w)
    print_fpn_w = fpn_f;

  // find first one in dec_val
  if(dec_val != 0)
    for( first_one= sizeof(long long)*8-1 ; !((dec_val>>first_one) && 0x1); first_one--) ;
  else
    first_one = 0;
  dbg_print( debug, __func__, __LINE__, "value %llx, first one: %2d", dec_val, first_one);

  if(dec_val == 0)
    bits_req = (fpn_w>0)? fpn_w: 1;
  else
    bits_req = (fpn_w>0)? fpn_w: (first_one+1);

  dbg_print( debug, __func__, __LINE__, "wl:%d, fl:%d, max_val: %lld =0x%llX (unsigned: %llu), dec_val: 0x%llx bits_req: %d", fpn_w, fpn_f, max_val, max_val, u_max_val, dec_val, bits_req);

  // check if value exceeds range of fixed-point value
  if( fpn_w > 0) {
    if( leading_zero || dec_val <0 )
      oorbits = sizeof(long long)*8 -fpn_w;

    else if( fpn_f > fpn_w)
      oorbits = ( log2( dec_val)+1 > print_fpn_w)? log2( dec_val)+1 -fpn_w : print_fpn_w -fpn_w;
    else
      oorbits = ( log2( dec_val)+1 > fpn_w)? log2( dec_val)+1 -fpn_w : 0;

    if(debug && print) printf("\ndbg %s %4d: fixed-point max bits = %d, out of range %d, value %llx", __func__, __LINE__, fpn_w, oorbits, dec_val);
  }


  if( dec_val == 0) {
    sprintf( bin_val_glb, "0");
  }
  else {
    for( i= sizeof(long long)*8-1; i> first_one; i--);
    for( ; i>= 0; i--) {
      sprintf( tmps, "%lld", (dec_val>>i) & 0x1);
      strcat( bin_val_glb, tmps);
    }
    strcat( bin_val_glb, "\0");
  }

  if( print) {
    tmps[0] = '\0';;
    if(debug && print) printf("\ndbg %s %4d: start bin value print", __func__, __LINE__);

    //--------------------------------------------------------------------------
    // print bits
    // z off, s off
    //--------------------------------------------------------------------------
    if( (leading_zero ==0) && (digit_separate ==0)) {
      if(debug && print) printf("\ndbg %s %4d: start case a, z off, s off", __func__, __LINE__);
      if( fpn_w > 0) {
        darkprint( 1);
        dbg_print ( debug, __func__, __LINE__, "loop i:%d; i>=%d; i--; print spaces", sizeof(long long)*8-1,  bits_req+oorbits);
        for( i= sizeof(long long)*8-1; i>= bits_req +oorbits; i--) {
          printf(" ");
          strcat( tmps, " ");
        }
        dbg_print ( debug, __func__, __LINE__, "loop i:%d; i>=0; i--; print bits", i);
        for( ; i>= 0; i--) {
          if(i == fpn_w-1)
            darkprint( 0);
          printf("%lld", (dec_val>>i) & 0x1);
          sprintf( tmps_cpy, "%s%lld", tmps, (dec_val>>i) & 0x1);
          strcpy( tmps, tmps_cpy);
        }
      }
      else {
        for( i= sizeof(long long)*8-1; i> first_one; i--) {
          printf(" ");
          strcat( tmps, " ");
        }
        for( ; i>= 0; i--) {
          printf("%lld", (dec_val>>i) & 0x1);
          sprintf( tmps_cpy, "%s%lld", tmps, (dec_val>>i) & 0x1);
          strcpy( tmps, tmps_cpy);
        }
        dbg_print ( debug, __func__, __LINE__, "end case a");
      }
    }

    //--------------------------------------------------------------------------
    // z off, s on
    //--------------------------------------------------------------------------
    else if( (leading_zero ==0) && (digit_separate ==1)) {
      dbg_print( debug, __func__, __LINE__, "start case b z off, s on");
      // fpn_w > 0 -> fixed point output, always print fpn_w bits
      //*
      if( fpn_w > 0) {
        darkprint( 1);
        for( i= sizeof(long long)*8-1; i>= bits_req +oorbits; i--) {
          if( (i<sizeof(long long)*8-1) && ((i+1)%4==0)) {
            dbg_print( debug, __func__, __LINE__, "i=%2d. print seperator       space oor", i);
            printf(" ");
            strcat( tmps, " ");
          }
          dbg_print( debug, __func__, __LINE__, "i=%2d. print bit             oor", i);
          printf(" ");
          strcat( tmps, " ");
        }
        for( ; i>= 0; i--) {
          if( ((i+1)%4==0) && (i<sizeof(long long)*8-1) ) {
            if( bit_printed==1) {
              if ( (i< fpn_w-1 +oorbits) || (dec_val> (1<<(i+1))) ) {
                dbg_print( debug, __func__, __LINE__, "i=%2d. print seperator       ", i);
                printf("%c", separator_sign);
                sprintf( tmps_cpy, "%s%c", tmps, separator_sign);
                strcpy( tmps, tmps_cpy);
              }
              else {
                dbg_print( debug, __func__, __LINE__, "i=%2d. print bit             space", i);
                printf(" ");
                strcat( tmps, " ");
              }
            }
            else {
              dbg_print( debug, __func__, __LINE__, "i=%2d. print bit             space", i);
              printf(" ");
              strcat( tmps, " ");
            }
          }
          if(i == fpn_w-1)
            darkprint( 0);

          bit_printed = 1;
          dbg_print( debug, __func__, __LINE__, "i=%2d. print bit             ", i);
          printf("%lld", (dec_val>>i) & 0x1);
          sprintf( tmps_cpy, "%s%lld", tmps, (dec_val>>i) & 0x1);
          strcpy( tmps, tmps_cpy);
        }
      }
      // fpn_w = 0 -> no fixed point output printing starts at msb with value 1
      else // */
      {
        for( i= sizeof(long long)*8-1; i> first_one; i--) {
          printf(" ");
          strcat( tmps, " ");
          if( (i<sizeof(long long)*8-1) && ((i+1)%4==0)) {
            printf(" ");
            strcat( tmps, " ");
          }
        }
        for( ; i>= 0; i--) {
          if( ((i+1)%4==0) && (i<sizeof(long long)*8-1)) {
            if ( i< first_one) {
              printf("%c", separator_sign);
              sprintf( tmps_cpy, "%s%c", tmps, separator_sign);
              strcpy( tmps, tmps_cpy);
            }
            else {
              printf(" ");
              strcat( tmps, " ");
            }
          }
          printf("%lld", (dec_val>>i) & 0x1);
          sprintf( tmps_cpy, "%s%lld", tmps, (dec_val>>i) & 0x1);
          strcpy( tmps, tmps_cpy);
        }
      }
      dbg_print ( debug, __func__, __LINE__, "end case b");
    }

    //--------------------------------------------------------------------------
    // z on, s off
    //--------------------------------------------------------------------------
    else if( (leading_zero ==1) && (digit_separate ==0)) {
      dbg_print ( debug, __func__, __LINE__, "start case c\n");
      if( fpn_w == 0) {
        for( i= sizeof(long long)*8-1; i>= 0; i--) {
          printf("%lld", (dec_val>>i) & 0x1);
          sprintf( tmps_cpy, "%s%lld", tmps, (dec_val>>i) & 0x1);
          strcpy( tmps, tmps_cpy);
        }
      }
      else {
        darkprint( 1);
        for( i= sizeof(long long)*8-1; i>= 0; i--) {
          if( i == fpn_w -1)
            darkprint( 0);
          printf("%lld", (dec_val>>i) & 0x1);
          sprintf( tmps_cpy, "%s%lld", tmps, (dec_val>>i) & 0x1);
          strcpy( tmps, tmps_cpy);
        }
      }
      dbg_print ( debug, __func__, __LINE__, "end case c\n");
    }

    //--------------------------------------------------------------------------
    // z on, s on
    //--------------------------------------------------------------------------
    else if( (leading_zero ==1) && (digit_separate ==1)) {
      dbg_print ( debug, __func__, __LINE__, "start case d\n");
      if( fpn_w == 0) {
        for( i= sizeof(long long)*8-1; i>= 0; i--) {
          if( ((i+1)%4==0) && (i<sizeof(long long)*8-1)) {
              printf("%c", separator_sign);
              sprintf( tmps_cpy, "%s%c", tmps, separator_sign);
              strcpy( tmps, tmps_cpy);
          }
          printf("%lld", (dec_val>>i) & 0x1);
          sprintf( tmps_cpy, "%s%lld", tmps, (dec_val>>i) & 0x1);
          strcpy( tmps, tmps_cpy);
        }
      }
      else {
        darkprint( 1);
        for( i= sizeof(long long)*8-1; i>= 0; i--) {
          if( ((i+1)%4==0) && (i<sizeof(long long)*8-1)) {
              printf("%c", separator_sign);
              sprintf( tmps_cpy, "%s%c", tmps, separator_sign);
              strcpy( tmps, tmps_cpy);
          }
          if( i == fpn_w -1)
            darkprint( 0);
          printf("%lld", (dec_val>>i) & 0x1);
          sprintf( tmps_cpy, "%s%lld", tmps, (dec_val>>i) & 0x1);
          strcpy( tmps, tmps_cpy);
        }
      }
      dbg_print ( debug, __func__, __LINE__, "end case d\n");
    }
    dbg_print ( debug, __func__, __LINE__, "end bin value print");
  }

  dbg_print( debug, __func__, __LINE__, "bin value: \"%s\"", tmps);
  dbg_print( debug, __func__, __LINE__, "end function, bin_val_glb: %s\n", bin_val_glb);

} //binout() ------------------------------------------------------------------


//------------------------------------------------------------------------------
// convert string to unsigned long long, atoll does note work for higher
// numbers like 18446744073709551615 = 0xFFFFFFFFFFFFFFFF
//------------------------------------------------------------------------------
unsigned long long string2ull( char *input) {
  unsigned long long result = 0;
  unsigned long long potenz = 1;
  int i, k;
  int digit;
  char tmps[10];
  int debug = 0;
  for( k=0, i= strlen(input)-1; i>=0; i--, k++) {
    sprintf( tmps, "%c", input[i]);
    if( tmps[0] != ' ') {
      digit = atoi( tmps);
      if( k> 0)
        potenz *= 10;
      result += digit* potenz;
    }
    else{
      k--;
    }
    dbg_print( debug, __func__, __LINE__, "i=%2d, k=%2d, digit=\"%s\"=%d, res=%llu", i, k, tmps, digit, result);
  }
  dbg_print( debug, __func__, __LINE__, "");
  return result;
}


//------------------------------------------------------------------------------
// print fixed-point bin value
//-----------------------------------------------------------------------------
void fp_binout( int fp_s, int fpn_w, int fpn_i, int fpn_f, long long dec_val)
{
  register int i;
  char sign_bit[10];
  sprintf( sign_bit, "%1d", ((int) ((dec_val>>(fpn_w-1)) & 0x1)) );
  printf( "  s:%s i:", (fp_s==1)? sign_bit : "-");

  // give out binary int value
  if(fpn_i>0)
    for( i=fpn_w-1 ;i>fpn_f-1; i--) {
      if( ((i+1)%4 == 0) && ((i+1)<(fpn_w-1)) && (digit_separate==1))
        printf("%c", separator_sign);
      printf("%d", (int)((dec_val>>i) & 0x1));
    }
  else
    printf("-");

  // give out fractional value
  printf( " f:");
  if( (fpn_f>fpn_w) && (fpn_f>0))
    darkprint( 1);

  if(fpn_f >0)
    for( i=fpn_f-1 ;i>=0; i--) {
      if( ((i+1)%4 == 0) && ((i+1)<(fpn_f-1)) && (digit_separate==1))
        printf("%c", separator_sign);
      if( i== fpn_w-1)
        darkprint( 0);
      printf("%d", (int)((dec_val>>i) & 0x1));
    }
  else
    printf("-");

} //fp_binout() ---------------------------------------------------------------


//------------------------------------------------------------------------------
// fixed-point display
// This is a mess. Someone should clan this up ;-)
//-----------------------------------------------------------------------------
double fixp_out ( int print, long long dec_val)
{
  register int i;
  long long dec_val_calc;
  int k;
  int w;
  int s;
  int fb;
  int error_oor;
  int debug = 0 && dbg_enable;

  error_oor = 0;
  if( usigned && (dec_base_in == 0) )
    dec_val = (unsigned long long) dec_val;

  dbg_print( debug, __func__, __LINE__, "sizeof(double)=%d, sizeof(long double)=%d", sizeof(double), sizeof(long double));

  int ib = fpn_i; // integer bits
  long long   fixp2real_integer  = 0.0;
  long double fixp2real_fraction = 0.0;
  int         fraction_overflow = 0;
  int         negative_input    = 0;
  char      tmps_result[100];
  double    err;

  unsigned long long fixp2real_max_val;
  unsigned long long fixp2real_min_val;
  unsigned long long msb_extension;
  unsigned long long valid_mask = 0;

  fpn_w = fpn_i +  fpn_f;
  w  = fpn_w;
  s  = fp_s;
  fb = fpn_f;

  if( s) {
    fixp2real_max_val = 0xFFFFFFFFFFFFFFFF >> (64-fpn_w+1);
    fixp2real_min_val = 0xFFFFFFFFFFFFFFFF << (fpn_w-1);
  }
  else {
    fixp2real_max_val = (0x1<<fpn_w) -1;
    fixp2real_min_val = 0;
  }

  if( (dec_val < 0) && (s==0))
    error_oor =1;

  dbg_print( debug, __func__, __LINE__, "dec_val: %lld, s:%d, w:%d, fb:%d, max. value: %llx, min. value: %llx, integer bits: %d, oor error:%d", dec_val, s, w, fb, fixp2real_max_val, fixp2real_min_val, fpn_i, error_oor);
  dbg_print( debug, __func__, __LINE__, "raw dec_val     : 0x%llX", dec_val);

  // set valid bit mask
  for( i= 0; i<w; i++) {
    valid_mask = valid_mask | ((unsigned long long)1) << i;
  }
  msb_extension = 0;
  int                      msb = 0;
  unsigned long long msb4shift = 0;
  if( fpn_i <0) {
    msb = ( dec_val >> (w-1)) & s;
    msb4shift = (unsigned long long)msb;
    for( i= w; i<=w-fpn_i; i++) {
      msb_extension = msb_extension | msb4shift << i;
      dbg_print( debug, __func__, __LINE__, "calc msb extensn: msb= 0x%16llX, i:%2d msbext: 0x%16llX", msb4shift, i, msb_extension);
    }
  }
  // this is required to display the oor fractional bits correctly but calculate only the valid bits
  dec_val_calc = (dec_val & valid_mask)| msb_extension;
  negative_input = ((dec_val_calc>>(fpn_w-1) && 1) ==1);

  dbg_print( debug, __func__, __LINE__, "negative input  : %d", negative_input);
  dbg_print( debug, __func__, __LINE__, "msb             : %d", msb);
  dbg_print( debug, __func__, __LINE__, "msb_extension   : 0x%16llX", msb_extension);
  dbg_print( debug, __func__, __LINE__, "valid_mask      : 0x%16llX", valid_mask);

  if( w <= fb) {
    ib = 1;     // set fake number of integer bits
  }
  else
    ib = w -fb; // set actual number of integer bits

  if( debug) {
    printf( "\ndbg fixp_out %d: input value     : 0x%16llX ", __LINE__, dec_val);
    printf( "\ndbg fixp_out %d: value to convert: 0x%16llX, bin: ", __LINE__, dec_val_calc);
    for( i=63; i>=0; i--) {
      k = (dec_val_calc >> i) & 0x0000000000000001;
      printf( "%d", k);
    }
  }

  // calc integer bits
  dbg_print( debug, __func__, __LINE__, "calc integer bits");
  dbg_print( debug, __func__, __LINE__, "integer bits    : %d", ib);
  dbg_print( debug, __func__, __LINE__, "if bit i = 1, add 2^i to integer value (value>>i & 0x1");
  for( i=0; i<ib-s; i++) {
    if( (dec_val_calc >>(i+fb)) & 0x1) {
      // corner case
      if( (dec_val_calc == 1) && (fb == 64)) {
        dbg_print( debug, __func__, __LINE__, "calc integer bits, dec_val: 0x%llX, i=%d, fb=%d, corner case dec_val: 0x%16llX, fb=%d (do not add))", dec_val_calc, i, fb, dec_val_calc,fb);
      }
      else {
        fixp2real_integer = fixp2real_integer + pow( 2, i);
        dbg_print( debug, __func__, __LINE__, "calc integer bits, dec_val: 0x%llX, i=%d, fb=%d, i=%2d, adding 2^^%d = %lld: fixp2real = %16.16f", dec_val_calc, i, fb, i, i, (long long) pow(2,i), (double)fixp2real_integer);
      }
    }
  }
  dbg_print( debug, __func__, __LINE__, "fixp2real_integer variable = %16.16f", (double)fixp2real_integer);


  if( s == 1) {
    dbg_print( debug, __func__, __LINE__, "subtract 2^^%d if bit %d is set", ib-1, w-1);
    if( (dec_val_calc >> (w-1)) & 0x1) {
      fixp2real_integer = fixp2real_integer -pow( 2, ib-1);
      dbg_print( debug, __func__, __LINE__, "subtracting 2^^%d = %lld: fixp2real = %16.16f", ib-1, (long long) pow(2,ib-1), (double)fixp2real_integer);
    }
    dbg_print( debug, __func__, __LINE__, "fixp2real = %16.16f", (double)fixp2real_integer);
  }


  // calc fractional bits
  dbg_print( debug, __func__, __LINE__, "calc fractional bits: dec_val_calc: %lld =0x%llx, fb:%d", dec_val_calc, dec_val_calc, fb);
  fraction_overflow = 0;
  for( i=1; i<fb+1; i++) {
    if( (dec_val_calc >> (fb-i)) & 0x1) {
      fixp2real_fraction = fixp2real_fraction + (long double)pow( 2, -i);
      if( fixp2real_fraction >= 1.0) {
        fraction_overflow = 1;
      }
      dbg_print( debug, __func__, __LINE__, "adding 2^^(-%2d) = %16.24f,(dec_val_calc >> %d & 0x1), fixp2real = %16.40Lf", i, pow(2,-i), i, fixp2real_fraction);
    }
  }
  dbg_print( debug, __func__, __LINE__, "fixp2real fractional bits= %16.16f", (double)fixp2real_fraction);

  int fixp2real_int_is_neg_but_zero = 0;
  int fixp2real_int_is_neg          = 0;
  if( fraction_overflow) {
    if( fixp2real_integer <0) {
      ;//--fixp2real_integer;
    }
    else {
      ++fixp2real_integer;
    }
    dbg_print( debug, __func__, __LINE__, "detected fraction overflow. int: %lld , frac:  %16.24Lf", fixp2real_integer, fixp2real_fraction);
  }

  if( fixp2real_integer <0){
    fixp2real_int_is_neg = 1;
    if( fixp2real_fraction >0) {
      ++fixp2real_integer;
      fixp2real_fraction = 1-fixp2real_fraction;
      if( fixp2real_integer ==0) {
        fixp2real_int_is_neg_but_zero = 1;
      }
      dbg_print( debug, __func__, __LINE__, "integer <0: %s%lld: fraction bits: %16.24Lf\n", (fixp2real_int_is_neg_but_zero)? "-": "", fixp2real_integer, fixp2real_fraction);
    }
  }
  dbg_print( debug, __func__, __LINE__, "fixp2real_integer variable = %16.16f", (double)fixp2real_integer);

  dbg_print( debug, __func__, __LINE__, "integer: %s%lld: fraction bits: %16.40Lf (fixp2real_int_is_neg_but_zero: %d)", (fixp2real_int_is_neg_but_zero)? "-": "", fixp2real_integer, fixp2real_fraction, fixp2real_int_is_neg_but_zero);
  char tmpsf[100];
  char tmpsi[100];
  char tmpspc[100] = "";
  char tmpspc_cpy[100] = "";
  char satovflw_str[100] = "";
  char convgran_str[100] = "";
  char format_string[100];
  int frac_width;
  int spaces;
  int total_width;
  if (     fb< 4) frac_width =  3;
  else if (fb< 7) frac_width =  6;
  else if (fb<10) frac_width =  9;
  else if (fb<13) frac_width = 12;
  else if (fb<16) frac_width = 15;
  else if (fb<19) frac_width = 18;
  else            frac_width = 21;


  if ( fpn_i<0)
    total_width = (int)(log(pow(2,1))/log(10)) +1 +frac_width +2;
  else
    total_width = (int)(log(pow(2,fpn_i))/log(10)) +1 +frac_width +2;
  int int_width = total_width -frac_width;

  spaces = 25 -frac_width -int_width;
  for( i =0; i< spaces; ++i) {
    sprintf( tmpspc_cpy, "%s%c", tmpspc, ' ');
    strcpy( tmpspc, tmpspc_cpy);
  }
  dbg_print( debug, __func__, __LINE__, "total width = (int)(log(pow(2,%d))/log(10)) +1 +%d +2 = %d", fpn_i, frac_width, total_width);
  dbg_print( debug, __func__, __LINE__, "total width: %d, frac. width:%d\n", total_width, frac_width);

  sprintf( format_string, "%%%d.%dLf", 3, frac_width);
  sprintf( tmpsf, format_string, fixp2real_fraction);

  if( (usigned && (dec_base_in == 0)) || (s ==0))
    sprintf( format_string, "%%%du", int_width);
  else
    sprintf( format_string, "%%%dlld", int_width);

  sprintf( tmpsi, format_string, fixp2real_integer);
  dbg_print( debug, __func__, __LINE__, "integer part       = %lld", fixp2real_integer);
  dbg_print( debug, __func__, __LINE__, "set integer string = %s", tmpsi );

  if( fixp2real_int_is_neg_but_zero) {
    //int gsub( char *search_pattern, char *substitution, char *target, int n)
    gsub( " 0", "-0", tmpsi, 1);
    dbg_print( debug, __func__, __LINE__, "set integer string = %s", tmpsi );
  }

  dbg_print( debug, __func__, __LINE__, "integer string: %s",  tmpsi);
  dbg_print( debug, __func__, __LINE__, "fraction string: %s", tmpsf);
  dbg_print( debug, __func__, __LINE__, "negative value: %d", negative_input);
  if( print) {
    dbg_print( debug, __func__, __LINE__, "print fixed point \n");
    printf("    %s.%s%s", tmpsi, substr( tmpsf, 2, 0), tmpspc);
    if( (quiet == 0) && (verbose==1)) {
      //printf(" (s=%1d,i=%d,w=%d,f=%d) ", fp_s, (fpn_w>fpn_f)? ib:0, fpn_w, fpn_f);
      fp_binout( fp_s, fpn_w, fpn_i, fpn_f, dec_val);
      printf("  wl:%d fl:%d ", fpn_w, fpn_f);
    }
  }

  //debug = 1;
  sprintf( tmps_result, "%s.%s", tmpsi, substr( tmpsf, 2, 0));

  // print conversion error
  if( float_in ==1) {
    err = atof(tmps_result) - atof(f_input_val_str);
    dbg_print( debug, __func__, __LINE__, "calc error: %s -(%s) = %5.3e", tmps_result, f_input_val_str, err);
  }
  else {
    err = atof(tmps_result) - atof(conv_real_val);
    dbg_print( debug, __func__, __LINE__, "calc error: %s -(%s) = %5.3e", tmps_result, conv_real_val, err);
  }

  fp_range_ovflw = 0;
  long double abs_err;
  abs_err = (long double) err;
  if( err<0)
    abs_err = -err;

  if( abs_err > conv_granularity) {
      fp_range_ovflw = 1;
      dbg_print( debug, __func__, __LINE__, "abs(err)>abs(granularity): %5.3e > %5.3e", tmps_result, conv_real_val, abs_err, (double)conv_granularity);
      if (verbose==1) {
        sprintf( convgran_str, ", bit granularity = %5.3e", (double)conv_granularity);
      }
  }


  if( (fp_conv_fr_real == 1)|| error_oor) {
    fp_conv_fr_real = 0;
    if( print &&(quiet ==0)) {
      if( (err != 0.0) || error_oor) {
        if( saturated || (saturation && (fp_range_ovflw == 1)))
          sprintf( satovflw_str, ", saturated");
        if( (saturation ==0) && (fp_range_ovflw == 1))
          sprintf( satovflw_str, ", overflow, wrap around");
        printf(" (error = %5.3e%s%s", err, satovflw_str, convgran_str);
        printf(")");
      }
    }
  }
  err = (fixp2real_int_is_neg)? (-1)*err : err;
  return err;
} //fixp_out() ----------------------------------------------------------------


//-----------------------------------------------------------------------------
// control the base conversion and output
//-----------------------------------------------------------------------------
void ctrl_out( int newline)
{
  long long input_int = 0;
  unsigned long long input_uint =0;
  int i;
  char sepstr[10] = "\0"; // separator sign as string
  char res[100]    = "\0";
  char tmps[100]   = "\0";
  int print_length = 20;
  int dec_base_tmp = 0;
  int debug = 1 && dbg_enable;
  float_in_tmp_after_calc = 0;
  dbg_print( debug, __func__, __LINE__, "input: '%s', dec_in:%d, hex_in:%d, bin_in:%d, floatin:%d, dec_out:%d, hex_out:%d, bin_out:%d, fixp_out:%d\n", input_val, dec_base_in, hex_base_in, bin_base_in, float_in, dec_base_out, hex_base_out, bin_base_out, fixpoint_out);

  if( set_width) {
    width = atoi( input_val);
    dbg_print( debug, __func__, __LINE__, "set width");
    if( quiet == 0) print_config( 0x40);
    set_width = 0;
    return;
  }

  if( set_fp_format) {
    dbg_print( debug, __func__, __LINE__, "set fixed-point format: %s", input_val);
    init_fp( input_val);
    if( quiet == 0) print_config( 0x02);
    set_fp_format = 0;
    return;
  }

  if( newline == 0) {
    printf("\n");
  }

  if( set_dp_format) {
    dbg_print( debug, __func__, __LINE__, "set dark print value");
    init_dp( input_val);
    set_dp_format = 0;
    return;
  }

  if( set_syscmd) {
    dbg_print( debug, __func__, __LINE__, "set syscmd ", __LINE__);
    // system call
    int flp;
    flp = print_system_call_result( input_val);
    set_syscmd = 0;
    // enable real to fixed-point conversion if result is float
    if( fpn_w != 0 && flp) {
      real2fp = 1;
      float_in_tmp_after_calc = 1;
      dbg_print( debug, __func__, __LINE__, "set flag real2fp = %d", real2fp);
    }
    else
      dec_base_tmp = 1; // input_val is dec based after bc system call
  }

  if( real2fp || float_in || float_in_tmp_after_calc) {
    dbg_print( debug, __func__, __LINE__, "conv real to fixed-point");
    // convert real value to fixed-point format
    if( fp_display)
      prep_conv_real_to_fixpoint( input_val);
    real2fp = 0;
  }

  if(set_separator) {
    dbg_print( debug, __func__, __LINE__, "set separator");
    if( ((input_val[0] == 0x0027) && (input_val[2] == 0x0027)) || /* character quoted ' ' */
        ((input_val[0] == '"') && (input_val[2] == '"'))) {
      separator_sign = input_val[1];
    }
    else
      separator_sign = input_val[0];
    if( quiet == 0) printf("set digit separator to '%c'", separator_sign);
    set_separator = 0;
  }
  else {
    if( dec_base_in || dec_base_tmp || float_in || float_in_tmp_after_calc) {      // input is dec
      dbg_print( debug, __func__, __LINE__, "input_val %s, dec_base_in %d, float_in: %d, dec_base_tmp %d", input_val, dec_base_in, float_in, dec_base_tmp);
      // remove digit separation
      if( digit_separate) {
        sprintf( sepstr, "%c", separator_sign);
        gsub( sepstr, "", input_val, 0);
        dbg_print( debug, __func__, __LINE__, "input_val %s ", input_val);
      }
      dec_base_tmp = 0;
      //float_in_tmp_after_calc = 0;
      if( usigned) {
        input_uint = string2ull( input_val);
        input_int  = (long long) input_uint;
        dbg_print( debug, __func__, __LINE__, "input_uint %llu, input_uint %lld", input_uint, input_int);
        /*
        printf("\ninput_uint %22llu \n", input_uint);
        printf("input_uint %22lld \n", input_uint);
        printf("input_int  %22llu \n", (unsigned long long) input_int);
        printf("input_int  %22lld \n", input_int);
        printf("string2ull: %22llu\n", string2ull( input_val)); //*/
      }
      else {
        input_int = atoll( input_val);
        dbg_print( debug, __func__, __LINE__, "input_int atoll(%s) =%lld, ", input_val, input_int);
      }
      dbg_print( debug, __func__, __LINE__, "input_int %lld, ", input_int);
    }
    else if( hex_base_in ) {  // input is hex
      dbg_print( debug, __func__, __LINE__, "hex_base_in %s, ", input_val);
      input_int = hex2dec( input_val);
      dbg_print( debug, __func__, __LINE__, "hex_base_in, dec: %lld, ", input_int);
    }
    else if( bin_base_in)  { // input is bin
      dbg_print( debug, __func__, __LINE__, "bin_base_in %s, ", input_val);
      input_int = bin2dec(input_val);
      dbg_print( debug, __func__, __LINE__, "bin_base_in, dec: %lld, ", input_int);
    }
    else {
      dbg_print( debug, __func__, __LINE__, "WHAT ??? !!! How did I get here???");
    }

    if( usigned && (dec_base_in == 0) )
      input_uint = (unsigned long long) input_int;
    dbg_print( debug, __func__, __LINE__, "input_int: %lld, input_uint: %llu ", input_int ,input_uint);

    //--------------------------------------------------------------------------
    // print value decimal
    //--------------------------------------------------------------------------
    if( dec_base_out) {
      dbg_print( debug, __func__, __LINE__, "print dec ");
      if( digit_separate) {
        print_length += 6;
        if(usigned) {
          dbg_print( debug, __func__, __LINE__, "unsigned");
          sprintf( res, "%s", ins_thousand_sep_unsigned( input_uint, 1));
          for( i=strlen( res); i< print_length; i++) {
            strcat( tmps, " ");
          }
        }
        else {
          dbg_print( debug, __func__, __LINE__, "signed");
          sprintf( res, "%s", ins_thousand_sep( input_int, 1));
          dbg_print( debug, __func__, __LINE__, "inserted seperators");
          for( i=strlen( res); i< print_length; i++) {
            strcat( tmps, " ");
          }
        }
        strcat( tmps, res);
        dbg_print( debug, __func__, __LINE__, "dec. out\n");
        printf(" %s  ", tmps);
      }
      else  {
        dbg_print( debug, __func__, __LINE__, "dec. out\n");
        if( usigned)
          printf("%22llu  ", input_uint);
        else
          printf("%22lld  ", input_int);
      }
      dbg_print( debug, __func__, __LINE__, "print dec end");
    }

    //--------------------------------------------------------------------------
    // print value hex
    //--------------------------------------------------------------------------
    if( hex_base_out) {
      dbg_print( debug, __func__, __LINE__, "print hex \n");
      hexout( 1, input_int);
      dbg_print( debug, __func__, __LINE__, "print hex end");
    }

    //--------------------------------------------------------------------------
    // print value bin
    //--------------------------------------------------------------------------
    if( bin_base_out) {
      dbg_print( debug, __func__, __LINE__, "print bin \n");
      binout( 1, input_int);
      dbg_print( debug, __func__, __LINE__, "print bin end ");
    }
    //--------------------------------------------------------------------------
    // print fixed-point value
    //--------------------------------------------------------------------------
    if( fp_display && fixpoint_out) {
      dbg_print( debug, __func__, __LINE__, "print fixed point");
      fixp_out( 1, input_int);
      dbg_print( debug, __func__, __LINE__, "print fixed point end ");
    }
    if( newline) {
      printf("\n");
    }
  }
  dbg_print( debug, __func__, __LINE__, "end ");
} //ctrl_out() ------------------------------------------------------------------


//------------------------------------------------------------------------------
// invert value
//------------------------------------------------------------------------------
long long inv( long long data)
{
  data = data ^ (0xFFFFFFFFFFFFFFFFULL);
  return data;
}

//------------------------------------------------------------------------------
// control the base conversion and output
//------------------------------------------------------------------------------
#define MINUS_OP 0
#define PLUS_OP  1
#define MULT_OP  2
#define DIV_OP   3
#define AND_OP   4
#define OR_OP    5
#define XOR_OP   6
#define NOT_OP   7
#define SHL_OP   8
#define SHR_OP   9
#define CSHL_OP  10
#define CSHR_OP  11
void calculate( int minus, int plus, int mult, int div, int and, int or, int xor, int not, int shl, int shr, int rsl, int rsr)
{
  char val1[80];
  char val2[80];
  int operatorpos;
  int operation;
  long long val1_int = 0, val2_int =0;
  int debug = 0 && dbg_enable;
  dbg_print( debug, __func__, __LINE__, "calculation, minus:%d, plus:%d, mult:%d, div:%d, and:%d, or:%d, xor:%d, not:%d, shl:%d, shr:%d, rsl:%d, rsr:%d", minus, plus, mult, div, and, or, xor, not, shl, shr, rsl, rsr);

  operatorpos = minus;
  operation = MINUS_OP;

  if( plus > minus) {
    operatorpos = plus;
    operation = PLUS_OP;
  }
  if( mult > minus) {
    operatorpos = mult;
    operation = MULT_OP;
  }
  if( div > minus) {
    operatorpos = div;
    operation = DIV_OP;
  }
  if( and > minus) {
    operatorpos = and;
    operation = AND_OP;
  }
  if( or > minus) {
    operatorpos = or;
    operation = OR_OP;
  }
  if( xor > minus) {
    operatorpos = xor;
    operation = XOR_OP;
  }
  if( not > minus) {
    operatorpos = not;
    operation = NOT_OP;
  }
  if( shl > minus) {
    operatorpos = shl;
    operation = SHL_OP;
  }
  if( shr > minus) {
    operatorpos = shr;
    operation = SHR_OP;
  }
  if( rsl > minus) {
    operatorpos = rsl;
    operation = CSHL_OP;
  }
  if( rsr > minus) {
    operatorpos = rsr;
    operation = CSHR_OP;
  }

  dbg_print( debug, __func__, __LINE__, "operation: %d, at pos. %d, input string :%s\n", operation, operatorpos, input_val);
  sprintf( val2, "default");
  if( operation == NOT_OP) {
    sprintf( val1,"%s", substr( input_val, 1, 0));
    //dbg_print( debug, __func__, __LINE__, "substring:%s, substr:%s\n", substring( input_val, 2, 0), substr( input_val, 1, 0)); // difference between sugstring() and substr() functions?
  }
  else {
    sprintf( val1,"%s", substr( input_val, 0, operatorpos-1));
    //dbg_print( debug, __func__, __LINE__, "substring:%s, substr:%s\n", substring( input_val, 0, operatorpos-1), substr( input_val, 0, operatorpos-1));
  }
  if( (operation == SHL_OP) || (operation == SHR_OP)) {
    sprintf( val2,"%s", substr( input_val, operatorpos+1, 0));
    //dbg_print( debug, __func__, __LINE__, "substring:%s, substr:%s\n", substring( input_val, operatorpos+2, 0), substr( input_val, operatorpos+1, 0));
  }
  else if( (operation == CSHL_OP) || (operation == CSHR_OP)) {
    sprintf( val2,"%s", substr( input_val, operatorpos+2, 0));
    if( width == 0)
      printf("\n WARNING: width for ring shift set to 0, use -w <n> to set width");
    //dbg_print( debug, __func__, __LINE__, "substring:%s, substr:%s\n", substring( input_val, operatorpos+3, 0), substr( input_val, operatorpos+2, 0));
  }
  else {
    sprintf( val2,"%s", substr( input_val, operatorpos, 0));
    //dbg_print( debug, __func__, __LINE__, "substring:%s, substr:%s\n", substring( input_val, operatorpos+1, 0), substr( input_val, operatorpos, 0));
  }

  if( dec_base_in ) {
    val1_int = atoll( val1);
    val2_int = atoll( val2);
    dbg_print( debug, __func__, __LINE__, "case dec_base_in val1_int = %lld, val2_int = %lld:\n", val1_int, val2_int);
  }
  else if( hex_base_in) {  // input is hex
    val1_int  = hex2dec( val1);
    if( (operation == SHL_OP) || (operation == SHR_OP) || (operation == CSHL_OP) || (operation == CSHR_OP))
      val2_int = atoll( val2);
    else
      val2_int  = hex2dec( val2);
    dbg_print( debug, __func__, __LINE__, "case hex_base_in val1_int = %lld, val2_int = %lld:\n", val1_int, val2_int);
  }
  else if( bin_base_in ) {  // input is bin
    val1_int  = bin2dec( val1);
    if( (operation == SHL_OP) || (operation == SHR_OP) || (operation == CSHL_OP) || (operation == CSHR_OP))
      val2_int = atoll( val2);
    else
      val2_int  = bin2dec( val2);
    dbg_print( debug, __func__, __LINE__, "case bin_base_in val1_int = %lld, val2_int = %lld:\n", val1_int, val2_int);
  }
  if( (operation == SHL_OP) || (operation == SHR_OP) || (operation == CSHL_OP) || (operation == CSHR_OP)) {// second operand for shift always decimal
    //val2_int = atoi( val2);
    dbg_print( debug, __func__, __LINE__, "shift operation: val1:%lld shift left or right by %s = %lld", val1_int, val2, val2_int);
  }
  //printf("\nval1= %s, val2 = %s\n", val1, val2);
  long long result=0;
  if( operation == MINUS_OP)
    result = val1_int - val2_int;
  else if( operation == PLUS_OP)
    result = val1_int + val2_int;
  else if( operation == MULT_OP)
    result = val1_int * val2_int;
  else if( operation == DIV_OP)
    result = val1_int / val2_int;
  else if( operation == AND_OP)
    result = val1_int & val2_int;
  else if( operation == OR_OP)
    result = val1_int | val2_int;
  else if( operation == XOR_OP)
    result = val1_int ^ val2_int;
  else if( operation == NOT_OP)
    result = inv(val1_int);
  else if( operation == SHL_OP)
    result = val1_int << val2_int;
  else if( operation == SHR_OP)
    if(val2_int == 0)
      result = val1_int;
    else
      result = ((val1_int >>1) & 0x7FFFFFFFFFFFFFFF)>>(val2_int-1);
  else if( operation == CSHL_OP)
    result = ring_shift( 0, val1_int, val2_int);
  else if( operation == CSHR_OP)
    result = ring_shift( 1, val1_int, val2_int);

  dbg_print( debug, __func__, __LINE__, "result: %lld, base hex: %d, base bin: %d, base dec: %d", result, hex_base_in, bin_base_in, dec_base_in);

  // convert back
  if( hex_base_in) {    // dec2hex
    if( leading_zero)
      sprintf( input_val, "%0llx", result);
    else
      sprintf( input_val, "%llx", result);
  }
  else if( bin_base_in ) {  // input is bin
    binout( 0, result);
    sprintf( input_val, "%s", bin_val_glb);
  }
  else
    sprintf( input_val, "%lld", result);
} // calculate() ------------------------------------------------------------------


//------------------------------------------------------------------------------
// ring shift
//------------------------------------------------------------------------------
long long ring_shift( int shift_right, long long value, int shift)
{
  long long result =0;
  int i,k;
  int bit_val[128];
  int bit_val_cpy[128];
  char tmps[128];
  //printf("\n circ shift %d bits: shift %d %s by %d\n", width, value, ((shift_right)? "right": "left"), shift);
  for(i=0; i<width; i++) {
    bit_val[i]     = (value>>i) &0x1;
    bit_val_cpy[i] = bit_val[i];
  }

  if( shift_right) {
    if(verbose) {
      printf("\n\nshift right:\n   0: ");
      for( i=width-1; i>=0; i--)
        printf( "%d", bit_val[i]);
    }
    for( k=0; k<shift; k++) {
      if(verbose) printf("\n %3d: ", k+1);
      for( i=width-1; i>=0; i--) {
        if( i==width-1)
          bit_val[i] = bit_val_cpy[0];
        else
          bit_val[i] = bit_val_cpy[i+1];
      }
      for( i=width-1; i>=0; i--)
        if(verbose) printf( "%d", bit_val[i]);
      for(i=0; i<width; i++) {
        bit_val_cpy[i] = bit_val[i];
      }
    }
  }
  else {
    if(verbose) {
      printf("\n\nshift left\n   0: ");
      for( i=width-1; i>=0; i--)
        printf( "%d", bit_val[i]);
    }
    for( k=0; k<shift; k++) {
      if(verbose) printf("\n %3d: ", k+1);
      for( i=0; i<width; i++) {
        if( i==0)
          bit_val[i] = bit_val_cpy[width-1];
        else
          bit_val[i] = bit_val_cpy[i-1];
      }
      for( i=width-1; i>=0; i--)
        if(verbose) printf( "%d", bit_val[i]);
      for(i=0; i<width; i++) {
        bit_val_cpy[i] = bit_val[i];
      }
    }
  }
  for( i=0; i<width; i++)
    tmps[i] = bit_val[width-1-i] +48;
  tmps[width] = '\0';
  result =  bin2dec( tmps);

  if(verbose) {
    printf(" = result dec.: %lld\n", result);
  }
  return( result);
}

//-----------------------------------------------------------------------------
// set dark print on () or off
//-----------------------------------------------------------------------------
void darkprint( int onoff) {
  if( no_colored_output==0) {
    if( onoff>0)
      printf("\033[%dm", 30+drkprnt_color);
    else
      printf("\033[0m");
  }
}


//-----------------------------------------------------------------------------
// display help screen
// some special text definition for help display
//------------------------------------------------------------------------------
void call_help( int select)
{
  PrgCfgAutoLineBreak = 80;

  // command line help
  if( select == 0) {
    printf("\n %s%s%s prints out input values in decimal, hexadecimal, binary and fixed-point format.\n", boldon, PrgCfgPrgName, normprint);

    sprintf( PrgCfgUsage, "\n command line syntax:\n \t%%> %s [-option] [value]\n\n \033[1mOptions:\033[0m", PrgCfgPrgName);
    prgcfghelp( cfg_opt, 1);   // 1 switches on printing "-" before option name
    if( verbose_help) {
      char usage_string [] = " \
\n In default mode the input format is decimal and the output of decimal, hexadecimal\
\n and binary values are enabled (options -d -DXB).\
\n If there is no value detected on the command line or piped into the program, the\
\n program starts in interactive mode. The program options could be changed by \
\n entering the desired option with a preceding '-'. 'Q' or 'q' quits the program.\n \
\n It is possible to evaluate simple calculations and logical operations with two \
\n operands:  +, -, *, /,  & , |, ^, ~, <<, >>, rs>, rs<, rs = ring shift (requires -w <n>),\
\n ~: only one operand). For operations +, -, *, /,  & , |, ^ both operands have to \
\n belong to the same base. For <<, >>, rs>, rs< the second operand is decimal. Spaces \
\n are not allowed between operands and operator.\
\n \
\n All Input values are treated as 64-bit values. \
\n \
\n The format for fixed-point number output is set via -fp <sgn:wl:fl> or -fp <wl:fl>. \
\n Example for unsigned (a) or signed (b) 8-bit fixed-point output with 3 integer\
\n and 5 fractional bits:\
\n   (a) -fp 0:8:5 \
\n   (b) -fp 1:8:5 or -fp 8:5 \
\n";
      printf("%s", usage_string);

      printf("\n\033[1m \
Examples:\033[0m \
\n   %%> %s -x 2A \
\n                    42                2A                   101010 \
\n   The hexadecimal value 2A is converted into decimal and binary, where \
\n   42 is the decimal, 2A the hexadecimal and 101010 the binary value. \n\
\n   The following three examples will return the same result. \
\n   1. value piped into program \
\n   %%> echo \"2A\" | %s -x  \
\n                    42                2A                   101010 \
\n   2. add operation entered on command line\
\n   %%> %s -x 20+A \
\n                    42                2A                   101010 \
\n   3. value entered interactively \
\n   %%> %s\
\n   > -q -x 2A q \
\n                    42                2A                   101010 \
\n \
\n   Max. unsigned value: \
\n   %%> %s -u 18446744073709551615 \
\n   18446744073709551615  FFFFFFFFFFFFFFFF  1111 ... 1111111111111 \n\
\n   Note the difference for signed and unsigned: \
\n   %%> echo \"-42\" | %s \
\n                    -42  FFFFFFFFFFFFFFD6  1111 ... 1111111010110 \
\n   %%> echo \"-42\" | %s -u \
\n                     42                2A                  101010 \
\n \
\n   Conversion of hex to fixed-point and vice versa: \
\n   %%> %s -xv \
\n   > -fp 10:6 \
\n    output format        : dec, hex, bin, fixp (sgn:1, wl:10, fl:6, il:4) \
\n   %%> 2A \
\n                     42               02A              0000101010       0.656250  s:0 i:0000 f:101010  wl:10 fl:6 \
\n   %%> -f -0.656250 \
\n    input value          : floating point \
\n                    -42  FFFFFFFFFFFFFFD6  1111 ... 1111111010110      -0.656250  s:1 i:1111 f:010110  wl:10 fl:6\
\n\n", PrgCfgPrgName, PrgCfgPrgName, PrgCfgPrgName, PrgCfgPrgName, PrgCfgPrgName, PrgCfgPrgName, PrgCfgPrgName, PrgCfgPrgName);
      // print command line options with their current values, status and help text
      // prgcfgprint( cfg_opt);
    }
  }
  // print help for configuration
  else if( select == 1) {
    // config file is not required for base_conv.c
    // sprintf( PrgCfgUsage," "); // overwrite text "usage: programname [-option [<value>]]"
    // prgcfghelp( fcfg_opt, 0);

    // print command line options with their current values, status and help text
    // prgcfgprint( fcfg_opt);
  }

}// call_help() ----------------------------------------------------------------

//EOF
