#include <cstdio>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "shell.hh"
#include "signal.h"
#include <sys/types.h>
#include <sys/wait.h>

int yyparse(void);
int yy_scan_string(const char * str);
int yyrestart(FILE * file);
extern FILE * yyin;
bool prompt_shown = false;
char * last_arg;

extern "C" void control_c (int sig) {
  fprintf( stderr, "\nsig:%d      Ouch!\n", sig);
  Shell::prompt();
}

extern "C" void zombie_process(int sig) {  
  int pid = wait3(0, 0, NULL);
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

void Shell::prompt() {
  if (isatty(0)) {
    if(getenv("PROMPT") == NULL) {
    printf("myshell>");
    }
    else {
    printf("%s", getenv("PROMPT"));
    }
    fflush(stdout);
  }
}

int main(int argc, char *argv[]) {

  SimpleCommand::relative_path = argv[0];

  char * test = (char *)malloc(100);
  for(int i = 1; i < argc; i++) {
    if(i == 1) {
      strcpy(test, argv[i]);
    }
    else {
      strcat(test, " ");
      strcat(test, argv[i]);
    }
  }
  strcat(test, "\n");


  // Control C
  struct sigaction sig;
  sig.sa_handler = control_c;
  sigemptyset(&sig.sa_mask);
  sig.sa_flags = SA_RESTART;
  int err = sigaction(SIGINT, &sig, NULL);
  if(err) {
    perror("sigaction");
    exit(-1);
  }

  // Zombie Processes
  struct sigaction sig2;
  sig2.sa_handler = zombie_process;
  sigemptyset(&sig2.sa_mask);
  sig2.sa_flags = SA_RESTART;
  err = sigaction(SIGCHLD, &sig2, NULL);
  if (err) {
    perror("sigaction");
    exit(-1);
  }

  if(argc > 1) {
    if(strcmp(argv[1], "source") == 0) {
      yyin = fopen(argv[2], "r");
    }
    else {
      yy_scan_string(test);
    }
  }

  Shell::prompt();
  yyparse();
}

Command Shell::_currentCommand;
