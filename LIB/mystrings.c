//-----------------------------------------------------------------------------
//  Description       : definition of some string modifying functions
//----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../LIB/keys.h"
#include "../LIB/mystrings.h"

struct termios new_settings;
struct termios stored_settings;
struct winsize terminal_size;

void cursor_off( void) { printf("\e[?25l");}
void cursor_on(  void) { printf("\e[?25h");}

//-----------------------------------------------------------------------------
// touppers( s)  returns s converted to upper case letter
//-----------------------------------------------------------------------------
char *touppers( char *source)
{
  int i;
  static char dest[1000]="";
  for( i = 0; i< strlen(source); i++)
    dest[i] = toupper( source[i]);

  dest[i] = '\0';
  return( dest);
} // toupper() ----------------------------------------------------------------

//-----------------------------------------------------------------------------
// tolowers( s)  returns s converted to lower case letter
//-----------------------------------------------------------------------------
char *tolowers( char *source)
{
  int i;
  static char dest[1000]="";
  for( i = 0; i< strlen(source); i++)
    dest[i] = tolower( source[i]);

  dest[i] = '\0';
  return( dest);
} // tolowers() ---------------------------------------------------------------

//-----------------------------------------------------------------------------
// substr(s, i , n)  returns the at most n-character substring  of s starting
// at i.  If n is 0, the rest of s is used.
//-----------------------------------------------------------------------------
char *substr( char *source, int start, int count)
{
  static char dest[500]="";
  int k,i;

  dest[0] = '\0';

  if( count == 0)
    count = strlen(source) - start;

  for( k=0, i= start; k<count; ++i, ++k)
    dest[k] = source[i];

  dest[k]= '\0';
  return( dest);
} // substr() -----------------------------------------------------------------


//-----------------------------------------------------------------------------
// find matching pattern in a string and returns position of first occurrence
// returns 0 if pattern not found in source
//-----------------------------------------------------------------------------
int match( char *source, char *search_pattern)
{
  int debug = 0;
  unsigned int k=0, i=0,j=0, match=0;
  if( debug) printf("\ndbg: match: source:'%s', search_pattern:'%s'",source,search_pattern);

  // step through source string as there are elements != 0 in string
  for( i=0; source[i]; i++) {
    j=i;
    if( source[i] == search_pattern[0]) {
      // first character match of source and search pattern
      match = i+1;
      if( debug) printf("\ndbg: match: source[%d]:'%c', search_pattern:'%c'",
			i, source[i],search_pattern[0]);

      for( k=0 ; search_pattern[k]; k++, i++) {
	// check following characters in string
	if( debug) printf("\ndbg: match: compare source[%d]:'%c' <-> search_pattern[%d]:'%c'",
			  i, source[i],k,search_pattern[k]);
	if( source[i] != search_pattern[k]) {
	  // missmatch in one of the following characters
	  if( debug) printf("\ndbg: match: source[%d]:'%c'(%d) != search_pattern[%d]:'%c' (%d)",
			    i, source[i], source[i], k, search_pattern[k], search_pattern[k]);
	  k = 0;
	  match = 0;
	  // end search if end of source string reached
	  if( source[i] == 0)
	    return( 0);
	  break;
	}
	if( k == (strlen(search_pattern)-1)) {
	  if( debug) printf("\ndbg: End match in loop: %d", match);
	  return (match);
	}
      }
    }
    if( match == 0)
      i=j; // next loop, shift start search one character in source
  }
  if( debug) printf("\ndbg: END match: %d", match);
  return( match);
} // match() ------------------------------------------------------------------

//-----------------------------------------------------------------------------
// gsub(r, s , t, n)  for each substring matching the string r in the string t,
// substitute the string s, until the number of substitions equals n. If n=0
// all matching substrings will be replaced. The actual number of substitutions
// is returned.
//-----------------------------------------------------------------------------
int gsub( char *search_pattern, char *substitution, char *target, int n)
{
  char tmpstr[500];
  unsigned int i=0, k=0, pos=0, matchpos=0, patternlength=0;
  int subst=0;
  patternlength = strlen(search_pattern);
  while( (matchpos = match(target,search_pattern)) ) {
    for( i=pos,k=0; k<matchpos-1; ++i,++k,++pos)
      tmpstr[i] = target[k];
    tmpstr[i] = '\0';

    strcat( tmpstr,substitution);
    pos = strlen( tmpstr);

    for( i=0; i<strlen( target); ++i)
      target[i] = target[ matchpos-1+i+patternlength];
    target[i]='\0';

    ++subst;

    // stop substitution if n>0 and number of substitiontions
    // matches n
    if( (n!=0) && (subst >= n))
      break;
  }

  if( subst)
    strcpy( target, (char *)strcat(tmpstr,target));

  return( subst);
} // gsub() --------------------------------------------------------------------

//------------------------------------------------------------------------------
// same as strlen but do not count escape sequences
//------------------------------------------------------------------------------
int no_esc_strlen( char *string)
{
  char *tmps  = malloc(strlen(string));
  int length = 0;
  strcpy( tmps, string);

  gsub( "\033[0m",  "", tmps, 0); //   0: all off
  gsub( "\033[1m",  "", tmps, 0); //   1: bold
  gsub( "\033[4m",  "", tmps, 0); //   4: underline
  gsub( "\033[5m",  "", tmps, 0); //   5: flash
  gsub( "\033[7m",  "", tmps, 0); //   7: invert
  gsub( "\033[8m",  "", tmps, 0); //   8: clear till EOL
  gsub( "\033[30m", "", tmps, 0); //  30: FG black
  gsub( "\033[31m", "", tmps, 0); //  31: FG red
  gsub( "\033[32m", "", tmps, 0); //  32: FG green
  gsub( "\033[33m", "", tmps, 0); //  33: FG yellow
  gsub( "\033[34m", "", tmps, 0); //  34: FG blue
  gsub( "\033[35m", "", tmps, 0); //  35: FG purple
  gsub( "\033[36m", "", tmps, 0); //  36: FG turquoise
  gsub( "\033[37m", "", tmps, 0); //  37: FG lght grey
  gsub( "\033[40m", "", tmps, 0); //  40: BG black
  gsub( "\033[41m", "", tmps, 0); //  41: BG red
  gsub( "\033[42m", "", tmps, 0); //  42: BG green
  gsub( "\033[43m", "", tmps, 0); //  43: BG yellow
  gsub( "\033[44m", "", tmps, 0); //  44: BG blue
  gsub( "\033[45m", "", tmps, 0); //  45: BG purple
  gsub( "\033[46m", "", tmps, 0); //  46: BG turquoise
  gsub( "\033[47m", "", tmps, 0); //  47: BG grey

  length = strlen(tmps);
  free(tmps);
  return( length);
} // no_esc_strlen() ----------------------------------------------------------

//-----------------------------------------------------------------------------
/*
This takes away the buffering in the Linux terminals so that you don't have to
press the Enter key afterwards. You have to include the header file TERMIO.H.
And after you run this code you can use the getchar() function and it works just
like getch() does in Windows/DOS programs.
*/
//-----------------------------------------------------------------------------
void set_keypress( void) {
  tcgetattr( 0, &stored_settings);
  new_settings = stored_settings;
  //printf("\n        new_settings.c_lflag: 0x%04X", new_settings.c_lflag);
  new_settings.c_lflag &= (~ICANON);
  //printf("\n~ICANON new_settings.c_lflag: 0x%04X", new_settings.c_lflag);
  new_settings.c_lflag &= (~ECHO);
  //printf("\n~ECHO   new_settings.c_lflag: 0x%04X\n", new_settings.c_lflag);
  new_settings.c_cc[VTIME] = 0;
  //tcgetattr( 0, &stored_settings);
  new_settings.c_cc[VMIN] = 1;
  tcsetattr( 0, TCSANOW, &new_settings);
}
/*
To reset you do as follows:
*/
void reset_keypress(void) {
  tcsetattr( 0, TCSANOW, &stored_settings);
}

//------------------------------------------------------------------------------
// same as perl chomp, cut off everything after new line and new line itself
// from a string
//------------------------------------------------------------------------------
void chomp( char *s) {
    while( *s && *s != '\n' && *s != '\r')
      s++;
    *s = 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int count_lines_in_file(FILE* file)
{
  int counter = 0;
  char line[1000];

  while( fgets( line, 1000, file) ) { // read each line from config file
    counter++;
  }
  rewind( file);
  return counter;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void init_commandlist( char *history[], char *buffer[], int hsize) {
  int i;

  for( i=0; i<hsize; i++) {
    history[i] = (char *) malloc( (ReadCommand_StringLength+1) * sizeof( char));
    buffer[i] = (char *) malloc( (ReadCommand_StringLength+1) * sizeof( char));
  }

  //Init History
  for( i=0; i<hsize; ++i) {
    sprintf( history[i], "");
    strcpy( buffer[i], history[i]);
  }

  set_keypress(); // switch off keyboard echo, enable key read without enter
}


//------------------------------------------------------------------------------
// same as init_commandlist but restore command history from file
//------------------------------------------------------------------------------
int init_commandlist_history( char *history[], char *buffer[], int hsize, char *hlogfile) {
  int i;
  char line[1000];
  FILE *cmd_history;
  int debug = 0;

  for( i=0; i<hsize; i++) {
    history[i] = (char *) malloc( (ReadCommand_StringLength+1) * sizeof( char));
    buffer[i]  = (char *) malloc( (ReadCommand_StringLength+1) * sizeof( char));
  }

  // Init History
  for( i=0; i<hsize; ++i) {
    sprintf( history[i], "");
    strcpy( buffer[i], history[i]);
  }

  set_keypress(); // switch off keyboard echo, enable key read without enter

  int cnt;
  i = 0;
  if( (cmd_history = fopen( hlogfile, "r")) ==0) {
    if( debug) printf("\n*** failed to open command history file %s\n", hlogfile);
  }
  else {
    cnt = count_lines_in_file( cmd_history);
    while( fgets( line, 1000, cmd_history) ) { // read each line from config file
      chomp( line);
      sprintf( history[cnt-i], "%s", line);
      strcpy( buffer[cnt-i], history[cnt-i]);
      if( debug) printf( "\n read history %d = %s", i, line);
      i++;
    }
    fclose( cmd_history);
  }
  return i; // return number of command entries in history
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void free_commandlist( char *history[], char *buffer[], int hsize) {
  int i;

  for( i=0; i<hsize; i++) {
    free( history[i]);
    free( buffer[i]);
  }

  reset_keypress(); // restore defaults: switch on keyboard echo,
		    // disable key read without enter
}

//------------------------------------------------------------------------------
// same as free_commandlist but save command history to file
//------------------------------------------------------------------------------
void free_commandlist_history( char *history[], char *buffer[], int hsize, int h_entries, char *hlogfile) {
  FILE *cmd_history;
  int i;
  int debug = 0;

  if( (cmd_history = fopen( hlogfile, "w")) ==0) {
    if( debug) printf("\n*** failed to open file %s\n", hlogfile);
  }
  else {
    // save history
    for( i=h_entries; i>1; i--) {
      if( debug) printf("\nsave command %d: %s", i, history[i]);
      fprintf( cmd_history, "%s\n", history[i]);
    }
    if( debug) printf("\n");
    fclose( cmd_history);
  }

  for( i=0; i<hsize; i++) {
    free( history[i]);
    free( buffer[i]);
  }

  reset_keypress(); // restore defaults: switch on keyboard echo,
		    // disable key read without enter
}

//------------------------------------------------------------------------------
// read key from stdio, returns key value (alnums >0, control keys <0)
//------------------------------------------------------------------------------
int getkey( void) {
  int c;
  int esc[4];

  c = getchar();
  if( c == K_ESC) { // Escape sequence, cursor movements
    esc[0] = c;
    esc[1] = getchar();

    if( esc[1] == 91) { // '['
      esc[2] = getchar();
      if( ((esc[2] >64) && (esc[2] <69)) ||
	  ((esc[2] >48) && (esc[2] <55))) {
          switch( esc[2]) {
          case K_POS1  : esc[3] = getchar(); break;
          case K_INS   : esc[3] = getchar(); break;
          case K_DEL   : esc[3] = getchar(); break;
          case K_END   : esc[3] = getchar(); break;
          case K_PGUP  : esc[3] = getchar(); break;
          case K_PGDOWN: esc[3] = getchar(); break;
          case K_UP    : break;
          case K_DOWN  : break;
          case K_RIGHT : break;
          case K_LEFT  : break;
          default: break;
          }
        }
      return ( -esc[2]);
      }
      else
        return( C_ESC);
    }
    else {
      if     ( c == K_BCKSP ) return ( C_BCKSP );
      else if( c == K_CTRLH ) return ( C_BCKSP );
      else if( c == K_ENTER ) return ( C_ENTER );
      else if( c == K_D     ) return ( C_D   );
      else if( c == K_SOL   ) return ( C_SOL   );
      else if( c == K_EOL   ) return ( C_EOL   );
      else if( c == K_CLREOL) return ( C_CLREOL);
      else if( c == K_TAB   ) return ( C_TAB   );
      else if( c == K_EOF   ) return ( C_EOF   );
      else if( c == EOF   )   return ( C_EOF   );
      else                    return ( c);
    }
} // getkey --------------------------------------------------------------------

//------------------------------------------------------------------------------
//http://www.linux.com/howtos/Bash-Prompt-HOWTO/x361.shtml
//
// Cursor Movements
// - Position the Cursor:
//   \033[<L>;<C>H
//      Or
//   \033[<L>;<C>f
//   puts the cursor at line L and column C.
// - Move the cursor up N lines:
//   \033[<N>A
// - Move the cursor down N lines:
//   \033[<N>B
// - Move the cursor forward N columns:
//   \033[<N>C
// - Move the cursor backward N columns:
//   \033[<N>D
//
// - Clear the screen, move to (0,0):
//   \033[2J
// - Erase to end of line:
//   \033[K
//
// - Save cursor position:
//   \033[s
// - Restore cursor position:
//   \033[u
//------------------------------------------------------------------------------
// offset is cursor position on screen from left edge
//------------------------------------------------------------------------------
int getstring( char *is, int offset) {
  int ik;  // input key
  int cpos;// cursor position
  int spos;// character position in string
  char tmp[500];
  int debug =0;

  printf("\r");				// go to start of line

  if( offset)
    printf("\033[%dC", offset);         // cursor right till offset

  printf("%s\033[K", is);
  spos = strlen( is);
  cpos = offset + spos;
  ik = 0;
  while( (ik != C_ENTER) && (ik != C_UP) && (ik != C_DOWN) &&
	 (ik != C_PGUP) && (ik != C_PGDOWN) && (ik != C_EOF) && (ik != C_TAB)) {
    if( debug ==1) { //-----------------------------------------------------
      printf("\033[s");				      // Save cursor position
      printf("\033[1A");			      // Move the cursor up 1 line
      printf("\r");				      // go to start of line
      if( offset) printf("\033[%dC", offset);         // cursor right till offset
      printf("cpos:%3d, spos:%3d\033[K", cpos, spos); // position output clear unitl eol
      printf("\033[u");				      // Restore cursor position
    }       //--------------------------------------------------------------

    ik = getkey();  // get key input and evaluate in the following

    //echo( 1); // echo_on
    //---------------------------------------------------------------------------
    // Pos1 or Ctrl a
    //---------------------------------------------------------------------------
    if( ( ik == C_POS1) || ( ik == C_SOL)) {
      if( cpos > offset)
	printf("\033[%dD", cpos -offset);
      cpos = offset;
    }
    //---------------------------------------------------------------------------
    // End or Ctrl e
    //---------------------------------------------------------------------------
    else if( (ik == C_END) || ( ik == C_EOL)) {
      if( cpos <( offset +strlen( is)))
        printf("\033[%dC", (int) strlen( is) +offset -cpos);
      cpos = strlen( is) +offset;
    }
    //---------------------------------------------------------------------------
    // -> right arrow
    //---------------------------------------------------------------------------
    else if( ik == C_RIGHT) {
      if( cpos < ( offset +strlen( is))) {
	if( debug ==1) printf("\033[s\033[1B\rstrlen( is)=%d, offset+strlen=%d\033[K\033[u", (int) strlen( is), offset + (int) strlen( is));
	printf("\033[1C");
	++cpos;
      }
    }
    //---------------------------------------------------------------------------
    // <- left arrow
    //---------------------------------------------------------------------------
    else if( ik == C_LEFT) {
      if( cpos > offset) {
	printf("\033[1D");
	--cpos;
      }
    }
    //---------------------------------------------------------------------------
    // Delete
    //---------------------------------------------------------------------------
    else if( (ik == C_DEL) || ( ik == C_D)) {
      if( cpos < (strlen( is) +offset)) {
	if( cpos == offset) {
	  sprintf( tmp, "%s", (strlen(is) >0)? substr( is, 1, 0) : "");
	  if( debug ==1) printf("\033[s\033[1B\rtmp:%s l=%d subs:%s\033[K\033[u", tmp, (int) strlen(is), substr( is, 1, spos));
	}
	else {
	  sprintf( tmp, "%s", substr( is, 0, spos));
	  if( debug ==1) printf("\033[s\033[1B\rtmp:%s l=%d subs:%s\033[K\033[u", tmp, (int) strlen(is), substr( is, 0, spos));
	  strcat( tmp, substr( is, spos+1, 0));
	  if( debug ==1) printf("\033[s\033[2B\rtmp:%s l=%d subs:%s\033[K\033[u", tmp, (int) strlen(is), substr( is, spos+1, 0));
	}
	strcpy( is, tmp);
	printf("\033[s%s\033[K\033[u", substr( is, spos, 0));
      }
    }
    //---------------------------------------------------------------------------
    // Backspace
    //---------------------------------------------------------------------------
    else if( ik == C_BCKSP) {
      if( cpos > offset) {
	if( spos == 1) {
	  if( debug ==1) printf("\033[s\033[1B\rtmp:%s\033[K\033[u", "");
	  strcpy( tmp, substr( is, 1, 0));
	  if( debug ==1) printf("\033[s\033[2B\rtmp:%s\033[K\033[u", tmp);
	}
	else {
	  strcpy( tmp, substr( is, 0, spos-1));
	  if( debug ==1) printf("\033[s\033[1B\rtmp:%s\033[K\033[u", tmp);
	  strcat( tmp, substr( is, spos, 0));
	  if( debug ==1) printf("\033[s\033[2B\rtmp:%s\033[K\033[u", tmp);
	}
	strcpy( is, tmp);
	printf("\b\033[s%s\033[K\033[u", substr( is, spos-1, 0)); // backspace on stdio
	--cpos;
      }
    }
    //---------------------------------------------------------------------------
    // Ctrl k: clear rest of line
    //---------------------------------------------------------------------------
    else if( ik == C_CLREOL) {
      if( cpos > offset) {
	sprintf( tmp, "%s", substr( is, 0, spos));
	if( debug ==1) printf("\033[s\033[1B\rtmp:%s l=%d subs:%s\033[K\033[u", tmp, (int) strlen(is), substr( is, 0, spos));
	strcpy( is, tmp);
      }
      else
	sprintf( is,"");

      printf("\033[K");
    }
    //---------------------------------------------------------------------------
    // printable character
    //---------------------------------------------------------------------------
    else if( (ik > 31) && ( ik <127)) {
      if( cpos == offset) {
	sprintf( tmp, "%c%s", ik, (strlen(is) >0)? substr( is, 0, 0) : "");
	if( debug ==1) printf("\033[s\033[1B\rtmp:%s l=%d subs:%s\033[K\033[u", tmp, (int) strlen(is), substr( is, 0, spos));
      }
      else {
	sprintf( tmp, "%s%c", substr( is, 0, spos), ik);
	if( debug ==1) printf("\033[s\033[1B\rtmp:%s l=%d subs:%s\033[K\033[u", tmp, (int) strlen(is), substr( is, 0, spos));
	strncat( tmp, substr( is, spos, 0), strlen( substr( is, spos, 0)));
	if( debug ==1) printf("\033[s\033[2B\rtmp:%s l=%d subs:%s\033[K\033[u", tmp, (int) strlen(is), substr( is, spos, 0));
      }

      strcpy( is, tmp);
      printf("%c\033[s%s\033[K\033[u", ik, substr( is, spos+1, 0));
      ++cpos;
    }

    //echo( 0); // echo off
    spos = cpos -offset;
  }
  return( ik);

}// getstring ------------------------------------------------------------------


//------------------------------------------------------------------------------
// read input until enter key is pressed and store command line in command
// history list
//------------------------------------------------------------------------------
char *readcommand( char *history[], char *buffer[], int hsize, int *indexmax, char *prompt, UserCommands *cmd_list) // new---
{
  int index;
  int i,k;
  int debug = 0;
  int key = 0;
  int lineoffset=0;
  char cmd_ext[1000];
  char tmps[1000];
  int matches =0;
  int cmd_cnt=0;
  int matchindex[1000];
  int fail = 0;
  int cmdlength = 0;

  // get current terminal size
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
  // lines stored in:   terminal_size.ws_row
  // columns stored in: terminal_size.ws_col

  printf("\n%s", prompt);              // print prompt
  lineoffset = no_esc_strlen( prompt); // find length of prompt without counting optional escape sequences in prompt string
  static char command[ReadCommand_StringLength];

  index=0;               // empty string for new command
  key = 0;               // no key pressed

  while( (key != C_ENTER) && (key != C_EOF)) {
    key = getstring( buffer[index], lineoffset);

    //--------------------------------------------------------------------------
    // TAB command extension
    //--------------------------------------------------------------------------
    if(key == C_TAB) {
      matches   = 0;
      cmd_cnt   = 0;
      cmdlength = 0;
      while( cmd_list[cmd_cnt].name) {
	if( (match(  cmd_list[cmd_cnt].name, buffer[index]) == 1) || (strlen(buffer[index])==0)) {
	  sprintf( cmd_ext, "%s", cmd_list[cmd_cnt].name); // copy full command to cmd_ext string
	  if( cmdlength < strlen(cmd_list[cmd_cnt].name))  // set max. clommand length
	    cmdlength =  strlen(cmd_list[cmd_cnt].name);
	  matchindex[matches] = cmd_cnt;
	  matches++;
	}
	cmd_cnt++;
      }
      // only one command matching: complete string to full command
      if( matches == 1) {
	sprintf( buffer[index], "%s", cmd_ext);
      }

      // more than one command matching:
      if( matches > 1) {
	// print all matching commands
	for( i=0; i<matches; i++) {
	    printf("\n %s ", cmd_list[matchindex[i]].name);
	}

	// find largest common string which matches all currently matching commands
	fail = 0;
	while( strlen( cmd_ext) > strlen( buffer[index])) {
	  sprintf( tmps, "%s%c", buffer[index], cmd_ext[strlen( buffer[index])]);
	  //printf("\n check for matching '%s'", tmps);
	  for( i=0; i<matches; i++) {
	    if( match( cmd_list[matchindex[i]].name, tmps) == 0) {
	      //printf("\n    -> not matching '%s'", cmd_list[matchindex[i]].name);
	      fail = 1;
	      break;
	    }
	  }
	  if(fail == 0)
	    sprintf( buffer[index], "%s", tmps);
	  else
	    break;
	}
	printf("\n%s", prompt);
      }
    }
    //--------------------------------------------------------------------------
    // Cursor keys
    //--------------------------------------------------------------------------
    else if(  (key == C_UP)   && ( index<hsize) && ( index< *indexmax)) {
      ++index;
      if( debug ==1)
	printf("\033[s\033[1B\rindex=%2d, indexmax=%2d:\"%s\"\033[K\033[u",
	       index, *indexmax, buffer[index]);
    }
    else if( (key == C_DOWN) && ( index>0)) {
      --index;
      if( debug ==1)
	printf("\033[s\033[1B\rindex=%2d, indexmax=%2d:\"%s\"\033[K\033[u",
	       index, *indexmax, buffer[index]);
    }
    else if(  key == C_PGUP) {
      index= *indexmax;
      if( debug ==1)
	printf("\033[s\033[1B\rindex=%2d, indexmax=%2d:\"%s\"\033[K\033[u",
	       index, *indexmax, buffer[index]);
    }
    else if(  key == C_PGDOWN) {
      index=0;
      if( debug ==1)
	printf("\033[s\033[1B\rindex=%2d, indexmax=%2d:\"%s\"\033[K\033[u",
	       index, *indexmax, buffer[index]);
    }
  }

  if( debug ==1)
    printf("\033[s\033[1B\rreturned string %d:\"%s\" length%d\033[K\033[u",
			index, buffer[index], (int) strlen( buffer[index]));
  if( strlen( buffer[index]) > 0) {	       // shift history list if not empty string
    if( *indexmax < hsize -1)
      ++*indexmax;

    for( k=hsize-1; k>1; k--)
      sprintf( history[k], "%s", history[k-1]);
    sprintf( history[1], "%s", buffer[index]);      // store current input string
    sprintf( history[0], "");

    for( k=hsize-1; k>=0; k--)
      strcpy( buffer[k], history[k]);

    k=1;
  }
  else if( index!=0) {
    strcpy( buffer[index], history[index]);   // copy history back to empty buffer
    k=0;
  }
  else
    k=0;

  if( k== 1) {
    strcpy( command, history[1]);
  }
  else
    if( key == C_EOF)
      sprintf( command, "quit");
    else
      sprintf( command, "");
  if( debug ==1)
    printf("\nk=%d return command:\"%s\"", k, command);
  return( command);

} // readcommand() -------------------------------------------------------------

//------------------------------------------------------------------------------
// EOF
