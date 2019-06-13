
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include <unistd.h>
#include "y.tab.hh"
#include <sys/types.h>
#include <sys/wait.h>
#include "command.hh"

#include <string.h>

////////////  Start added code ///////////

extern char * read_line();

int mygetc(FILE * f) {
 static char *p;
 char ch;

 if (!isatty(0)) {
 // stdin is not a tty. Call real getc
   return getc(f);
 }

 // stdin is a tty. Call our read_line.
 if (p==NULL || *p == 0) {
   char * s = read_line();
   p = s;
 }

 ch = *p;
 p++;

 return ch;
}

#undef getc
#define getc(f) mygetc(f)

/////////// End added code ///////////

static  void yyunput (int c,char *buf_ptr  );
static char * shell_last_arg;
bool source_found = false;

void myunputc(int c) {
  unput(c);
}

%}

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">>&" {
  return GREATGREATAMPERSAND;
}

">>" {
  return GREATGREAT;
}

"2>" {
  return TWOGREAT;
}

">&" {
  return GREATAMPERSAND;
}

">" {
  return GREAT;
}

"<" {
  return LESS;
}

"|" {
  return PIPE;
}

"&" {
  return AMPERSAND;
}

(\$\().*(\)) {
  char * command = strdup(yytext + 2);
  
  if(command[yyleng - 3] == ')') {
    command[yyleng - 3] = '\0';
  }

  int default_in = dup(0);
  int default_out = dup(1);

  int fdpipein[2];
  int fdpipeout[2];

  pipe(fdpipein);
  pipe(fdpipeout);

  write(fdpipein[1], command, strlen(command));
  write(fdpipein[1], "\n", 1);
  write(fdpipein[1], "exit2", 5);
  write(fdpipein[1], "\n", 1);

  close(fdpipein[1]);

  dup2(fdpipein[0], 0);
  close(fdpipein[0]);
  dup2(fdpipeout[1], 1);
  close(fdpipeout[1]);

  int pid = fork();
  if (pid == 0) {
    char * const * argc = NULL;
    execvp("/proc/self/exe", argc);
    _exit(1);
  }
  else if (pid < 0) {
    perror("fork");
    exit(1);
  }

  dup2(default_in, 0);
  dup2(default_out, 1);
  close(default_in);
  close(default_out);

  char ch;
  char * buffer = (char *) malloc(4096);
  int i = 0;

  waitpid(pid, 0, 0);

  while (read(fdpipeout[0], &ch, 1)) {
    if (ch == '\n') {
      buffer[i++] = ' ';
    }
    else {
      buffer[i++] = ch;
    }
  }

  buffer[i] = '\0';
  for (i = (strlen(buffer) - 1); i >= 0; i--) {
    unput(buffer[i]);
  }
}


["][^\n\"]*["] {
  char * command = strdup(yytext + 1);

  if (command[yyleng - 2] == '"') {
    command[yyleng - 2] = 0;
  }
  yylval.cpp_string = new std::string(command);
  return WORD;
}                                                       

[^ \t\n|><&]*\\[^ \t\n]* {
  int a = 0;
  char * esc = (char *)malloc(200);
  char * t = yytext;

  while(*t) {
    if (*t == '\\') {
      if(*(t + 1) == '\\') {
        t = t + 2;
        esc[a++] = '\\';
      }
    }
    else {
      esc[a++] = *t;
    }
    t++;
  }

  esc[a] = '\0';
  char * c = strdup(esc);
  yylval.cpp_string = new std::string(c);
  return WORD;
}


[^ \t\n|><][^ \t\n|><]*  {
  /* Assume that file names have only alpha chars */

  if(strcmp("source", yytext) == 0) {
    source_found = true;
  }
  else {
    
    if(source_found) {
      std::string source_word(yytext);
      FILE * fd = fopen(source_word.c_str(), "r");

      char * buffer = (char *) malloc(4096);

      read(fileno(fd), buffer, 1024);

      for (int i = (strlen(buffer) - 1); i >= 0; i--) {
        unput(buffer[i]);
      }
      unput('\n');
      source_found = false;
    }
    else {
      shell_last_arg = Command::command_last_arg;

      if(Command::command_counter == 1) {
        SimpleCommand::_arg = strdup(shell_last_arg);
        Command::command_counter = 0;
      }

      yylval.cpp_string = new std::string(yytext);
      return WORD;
    }
  }
}
