/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#define MAX_BUFFER_LINE 2048

extern "C" void tty_raw_mode(void);

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];
int cursor_pos;

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char * history[1000];
int length;
/*
char * history [] = {
  "ls -al | grep x", 
  "ps -e",
  "cat read-line-example.c",
  "vi hello.c",
  "make",
  "ls -al | grep xxx | grep yyy"
};
*/
//int history_length = sizeof(history)/sizeof(char *);

void read_line_print_usage()
{
  char const *usage = "\nctrl-?       Print usage\nBackspace    Deletes last character\nup arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;

  cursor_pos = 0;

  FILE * debug = fopen("debug.txt", "a");

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32) {
      // It is a printable character. 

      if (cursor_pos == line_length) {
        // Do echo
        write(1,&ch,1);

        // If max number of character reached return.
        if (line_length==MAX_BUFFER_LINE-2) break; 

        // add char to buffer.
        line_buffer[line_length]=ch;
        line_length++;
        cursor_pos++;
      }
      else {

        char * temp_buffer = strdup(line_buffer);
        int temp_pos = cursor_pos;
        int temp_length = line_length;
        write(1, &ch, 1);
        if (line_length==MAX_BUFFER_LINE-2) break;
        //line_buffer[cursor_pos] = ch;
        line_length++;
        for (int i = line_length; i > cursor_pos; i--) {
          line_buffer[i] = line_buffer[i - 1];
        }
        line_buffer[cursor_pos] = ch;
        cursor_pos++;
        int count = 0;
        //char temp_ch = ch;
        for(int i = temp_pos; i < temp_length; i++) {
          ch = temp_buffer[i];
          write(1, &ch, 1);
          count++;
        }
        ch = 8;
        for(int i = 0; i < count; i++) {
          write(1, &ch, 1);
        }
        free(temp_buffer);
        fprintf(debug, "current cursor_pos: %d\n", cursor_pos);
        fprintf(debug, "line_buffer: %s\n\n", line_buffer);
        fflush(debug);
      }
    }
    else if (ch==10) {
      // <Enter> was typed. Return line
     
      history[length] = (char *)malloc(sizeof(line_buffer));
      strncpy(history[length], line_buffer, line_length);
      history[length][line_length] = '\0';
      history_index = length;
      length++;
      write(1,&ch,1);
      cursor_pos = 0;
      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == 4) {
      // <ctrl-d> was typed. remove char at that space and bring
      // remaining chars from right to left one space
      
      if (cursor_pos != line_length) {
        int count = 0;
        for (int i = cursor_pos; i < line_length; i++) {
          line_buffer[i] = line_buffer[i + 1];
          ch = line_buffer[i];
          write(1, &ch, 1);
          count++;
        }
        ch = ' ';
        write(1, &ch, 1);
        line_length--;
        ch = 8;
        for (int i = 0; i < count; i++) {
          write(1, &ch, 1);
        }
      }
      fprintf(debug, "current cursor_pos: %d\n", cursor_pos);
      fprintf(debug, "line_buffer: %s\n\n", line_buffer);
      fflush(debug);
    }
    else if (ch == 8) {
      // <backspace> was typed. Remove previous character read.

      // Go back one character
      ch = 8;
      write(1,&ch,1);
      cursor_pos--;
      int count = 0;                                             
      for (int i = cursor_pos; i < line_length; i++) {
        line_buffer[i] = line_buffer[i + 1];
        ch = line_buffer[i];
        write(1, &ch, 1);
        count++;
      }
      ch = ' ';
      write(1, &ch, 1);
      line_length--;
      ch = 8;
      for (int i = 0; i < count; i++) {
        write(1, &ch, 1);
      }
      fprintf(debug, "current cursor_pos: %d\n", cursor_pos);
      fprintf(debug, "line_buffer: %s\n\n", line_buffer);
      fflush(debug);
    }
    else if (ch == 1) {
      // <ctrl-A> was typed. Go to start of line.

      ch = 8;
      while(cursor_pos > 0) {
        write(1, &ch, 1);
        cursor_pos--;
      }
      //write(1, &ch, 1);
      //cursor_pos--;
      fprintf(debug, "current cursor_pos: %d\n", cursor_pos);
      fprintf(debug, "line_buffer: %s\n\n", line_buffer);
      fflush(debug);
    }
    else if (ch == 5) {
      // <ctrl-E> was typed. Go to end of line.

      for(int i = cursor_pos; i < line_length; i++) {
        ch = line_buffer[i];
        write(1, &ch, 1);
        cursor_pos++;
      }

      fprintf(debug, "current cursor_pos: %d\n", cursor_pos);
      fprintf(debug, "line_buffer: %s\n\n", line_buffer);
      fflush(debug);
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
	// Up arrow. Print previous line in history.

        //if(history_index != 0) {

	// Erase old line
	// Print backspaces
	int i = 0;
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}

	// Print spaces on top
	for (i =0; i < line_length; i++) {
	  ch = ' ';
	  write(1,&ch,1);
	}

	// Print backspaces
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}	

        if(history_index < 0) {
          history_index = 0;
        }

	// Copy line from history
        strcpy(line_buffer, history[history_index]);
	line_length = strlen(line_buffer);
	history_index=(history_index - 1);
        //}
        fprintf(debug, "line_buffer: %s\n", line_buffer);
        fflush(debug);


	// echo line
	write(1, &line_buffer, line_length);
        cursor_pos = line_length;
      }
      else if (ch1 == 91 && ch2 == 66) {
        // Down arrow; Print next line in history
	// Erase old line
	// Print backspaces                       
	if(history_index < (length - 1)) {
          int i = 0;
   	  for (i =0; i < line_length; i++) {
	    ch = 8;
	    write(1,&ch,1);
   	  }

	  // Print spaces on top
	  for (i =0; i < line_length; i++) {
	    ch = ' ';
	    write(1,&ch,1);
      	  }

	  // Print backspaces
	  for (i =0; i < line_length; i++) {
	    ch = 8;
	    write(1,&ch,1);
	  }

          history_index++;
  	  // Copy line from history
          strcpy(line_buffer, history[history_index]);
	  line_length = strlen(line_buffer);

	  // echo line
	  write(1, &line_buffer, line_length);
          cursor_pos = line_length;               	
        }
      }
      else if (ch1 == 91 && ch2 == 68) {
        if (cursor_pos > 0) {
          ch = 8;
          write(1, &ch, 1);
          cursor_pos--;
        }
        fprintf(debug, "left arrow:\n");
        fprintf(debug, "current cursor_pos: %d\n\n", cursor_pos);
        fflush(debug);
      }
      else if(ch1 == 91 && ch2 == 67) {
        if (cursor_pos < line_length) {
          ch = line_buffer[cursor_pos];
          write(1, &ch, 1);
          cursor_pos++;
          fprintf(debug, "right arrow:\n");
          fprintf(debug, "current cursor_pos: %d\n", cursor_pos);
          fprintf(debug, "line_length: %d\n\n", line_length);
          fflush(debug);
        }
      }
    }
  }

  fclose(debug);

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  return line_buffer;
}

