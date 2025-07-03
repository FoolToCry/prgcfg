//-----------------------------------------------------------------------------
//  Description       : definition of some string modifying functions
//----------------------------------------------------------------------------*/
#ifndef __mystrings_h__
#define __mystrings_h__
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define ReadCommand_StringLength 500
#define CONST_ST(s) ((char*)s)
#define CMD_CHECK(s) ((strlen(cmdargv[0]) == strlen( CONST_ST(s))) && (match( cmdargv[0], CONST_ST(s)) == 1))
#define CMD_ERROR(i) (printf("\n*** ERROR, command: %s, wrong number of parameters: %d, required: %d", cmdargv[0], cmdargc, i))
#define CMD_ARGC(i) if(cmdargc!=i){CMD_ERROR(i);}else

char *substr( char *source, int start, int count);
char *touppers( char *source);
char *tolowers( char *source);
int match( char *source, char *search_pattern);
int gsub( char *search_pattern, char *substitution, char *target, int n);
int no_esc_strlen( char *string);
int getkey( void);
int getstring( char *is, int offset);
void init_commandlist( char *history[], char *buffer[], int hsize);
void free_commandlist( char *history[], char *buffer[], int hsize);
int  init_commandlist_history( char *history[], char *buffer[], int hsize, char *hlogfile);
void free_commandlist_history( char *history[], char *buffer[], int hsize, int h_entries, char *hlogfile);
void set_keypress( void);
void reset_keypress( void);
void cursor_off( void);
void cursor_on( void);

// list of user defined commands
typedef struct interactive_command_list
{
  char *name;                                     // option name in command line or file
  int  parameters;                                // number of parameters
  int (*userfunction)( int uargc, char *uargv[]); // pointer to user defined function
  char *help;                                     // help description of configuration option
} UserCommands;

char *readcommand( char *history[], char *buffer[], int hsize, int *indexmax, char *prompt, UserCommands *cmd_list);

extern char command[ReadCommand_StringLength];      // command line as read from stdio
extern char cmdargv[ReadCommand_StringLength][100]; // argv
extern int  cmdargc;                                // argc

#include <termios.h>
extern struct termios new_settings;
extern struct termios stored_settings;
extern struct winsize terminal_size;
#endif
