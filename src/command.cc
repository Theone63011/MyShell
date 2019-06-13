/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <iostream>
#include <errno.h>
#include "command.hh"
#include "shell.hh"

extern char **environ;
bool onError = false;
int Command::previous_background_pid;
char * Command::command_last_arg;
int Command::command_counter;

SimpleCommand * Command::_currentSimpleCommand;

Command::Command() {
  // Initialize a new vector of Simple Commands
  _simpleCommands = std::vector<SimpleCommand *>();

  _outFile = NULL;
  _inFile = NULL;
  _errFile = NULL;
  _background = false;
  _redirectIn = 0;
  _redirectOut = 0;
  _append = false;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
  // add the simple command to the vector
  _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
  // deallocate all the simple commands in the command vector
  for (auto simpleCommand : _simpleCommands) {
      delete simpleCommand;
  }

  // remove all references to the simple commands we've deallocated
  // (basically just sets the size to 0)
  _simpleCommands.clear();

  if ( _outFile && _errFile && (_errFile == _outFile)) {
    delete _outFile;
    _outFile = NULL;
    _errFile = NULL;
  }
  else {
    if ( _outFile ) {
      delete _outFile;
    }
    _outFile = NULL;
    if ( _errFile ) {
      delete _errFile;
    }
    _errFile = NULL;
  }

  if ( _inFile ) {
      delete _inFile;
  }
  _inFile = NULL;
  _background = false;
  _redirectIn = 0;
  _redirectOut = 0;
  _append = false;
}

void Command::print() {
  printf("\n\n");
  printf("              COMMAND TABLE                \n");
  printf("\n");
  printf("  #   Simple Commands\n");
  printf("  --- ----------------------------------------------------------\n");

  int i = 0;
  // iterate over the simple commands and print them nicely
  for ( auto & simpleCommand : _simpleCommands ) {
      printf("  %-3d ", i++ );
      simpleCommand->print();
  }

  printf( "\n\n" );
  printf( "  Output       Input        Error        Background\n" );
  printf( "  ------------ ------------ ------------ ------------\n" );
  printf( "  %-12s %-12s %-12s %-12s\n",
          _outFile?_outFile->c_str():"default",
          _inFile?_inFile->c_str():"default",
          _errFile?_errFile->c_str():"default",
          _background?"YES":"NO");
  printf( "\n\n" );
}

int Command::builtin(int i) {
  if(strcmp(_simpleCommands[0]->_arguments[0]->c_str(), "setenv") == 0) {
    int error = setenv(_simpleCommands[i]->_arguments[1]->c_str(), _simpleCommands[i]->_arguments[2]->c_str(), 1);
    if(error) {
      perror("setenv");
    }
    clear();
    Shell::prompt();
    return 1;
  }
  
  if(strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "unsetenv") == 0) {
    int error = unsetenv(_simpleCommands[i]->_arguments[1]->c_str());
    if(error) {
      perror("unsetenv");
    }
    clear();
    Shell::prompt();
    return 1;
  }
 
  if(strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "cd") == 0){
    char * notFound = (char *)malloc(100);
    if(_simpleCommands[i]->_arguments[1] != NULL) {
      if(chdir(_simpleCommands[i]->_arguments[1]->c_str()) < 0) {
        strcpy(notFound, (char *) "cd: can't cd to ");
        strcat(notFound, _simpleCommands[i]->_arguments[1]->c_str());
        perror(notFound);
      }
    }
    else {
      if(chdir(getenv("HOME")) < 0) {
        perror("cd");
      }
    }
    clear();
    Shell::prompt();
    return 1;
  }

  return 0;
}

void Command::execute() {
  // Don't do anything if there are no simple commands
  if ( _simpleCommands.size() == 0 ) {
      Shell::prompt();
      return;
  }

  // If they just enter exit, just end program
  if (strcmp(_simpleCommands[0]->_arguments[0]->c_str(), "exit" ) == 0) {
    printf("\nGood bye.\n\n");
    exit(1);
  }

  if (strcmp(_simpleCommands[0]->_arguments[0]->c_str(), "exit2" ) == 0) {
    _exit(1);
  }

  // Exit program if incorrect usage of redirects
  if (_redirectIn > 1 || _redirectOut > 1) {
    printf("Ambiguous output redirect.\n");
    clear();
    Shell::prompt();
    return;
  }


  // Print contents of Command data structure
  //print();

  /********* Set default file descriptors ********/
  int default_in = dup(0);
  if (default_in == -1) {
    perror("command_execute: input dup\n");
    clear();
    Shell::prompt();
    return;
  }
  int default_out = dup(1);
  if (default_out == -1) {
    perror("command_execute: output dup\n");
    clear();
    Shell::prompt();
    return;
  }
  int default_err = dup(2);
  if (default_err == -1) {
    perror("command_execute: error dup\n");
    clear();
    Shell::prompt();
    return;
  }

  /********* If provided file descriptors, set new ones ********/
  int fdin;
  int fdout;
  int fderr;

  // set initial in file
  if (_inFile){
    fdin = open(_inFile->c_str(), O_RDONLY);
    if (fdin < 0) {
      perror("command_execute: creat infile\n");
      clear();
      Shell::prompt();
      return;
    }
  }
  else {
    fdin = dup(default_in);
    if (fdin == -1) {
      perror("command_execute: dup\n");
      clear();
      Shell::prompt();
      return;
    }
  }
                
  // set initial err file
  if (_errFile) {
    if (_append) {
      fderr = open(_errFile->c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);
    }
    else {
      fderr = open(_errFile->c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    }
    if (fderr < 0) {
      perror("command_execute: creat errfile\n");
      clear();
      Shell::prompt();
      return;
    }
  }
  else {
    fderr = dup(default_err);
    if (fderr == -1) {
      perror("command_execute: dup\n");
      clear();
      Shell::prompt();
      return;
    }
  }

  if (dup2(fderr, 2) == -1) {
    perror ("command_execute: err output dup2\n");
    clear();
    Shell::prompt();
    return;
  }
  close(fderr);

  /****************************** Execute the commands ************************/
  int ret;

  for (unsigned i = 0; i < _simpleCommands.size(); i++) {
    
    if(builtin(i)) {
      return;
    }

    if (dup2(fdin, 0) == -1) {
       perror("command_execute: cmd output dup2\n");
       clear();
       Shell::prompt();
       return;
    }
    close(fdin);
    
    if(i == (_simpleCommands.size() - 1)) {
      if (_outFile) {
        if (_append) {
          fdout = open(_outFile->c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);
        }
        else {
          fdout = open(_outFile->c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        }
        if (fdout < 0) {
          perror("command_execute: creat outfile\n");
          clear();
          Shell::prompt();
          return;
        }
      }
      else {
        fdout = dup(default_out);
        if (fdout == -1) {
          perror("command_execute: dup\n");
          clear();
          Shell::prompt();
          return;
        }
      } 
    }
    else {
      int fd_pipe[2];
      if (pipe(fd_pipe) == -1) {
        perror("command_execute: pipe\n");
        clear();
        Shell::prompt();
        return;
      }
      fdout = fd_pipe[1];
      fdin = fd_pipe[0];
    }
    if (dup2(fdout, 1) == -1) {
      perror("command_execute: cmd output dup2\n");
      clear();
      Shell::prompt();
      return;
    }
    close(fdout);

    ret = fork();
    if (ret == -1) {
      perror("command_execute: fork\n");
      clear();
      Shell::prompt();
      return;
    }

    if (ret == 0) {
     
      if(strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv") == 0){
        char ** env = environ;
        while(*env){
          printf("%s\n", *env);
          env++;
        }

        exit(1);
      }

        std::vector<char*> strings;
        for (auto & string: _simpleCommands[i]->_arguments) {
          strings.push_back(const_cast<char*>(string->c_str()));
        }
        strings.push_back(NULL);
        execvp(strings.front(), strings.data());
        _exit(1);                                                  
    }
  }

  // Restore input, output and error to their initial state
  if (dup2(default_in, 0) == -1) {
    perror("command_execute: restore input dup2\n");
    clear();
    Shell::prompt();
    return;
  }
  if (dup2(default_out, 1) == -1) {
    perror("command_execute: restore output dup2\n");
    clear();
    Shell::prompt();
    return;
  }
  if (dup2(default_err, 2) == -1) {
    perror("command_execute: restore error dup2\n");
    clear();
    Shell::prompt();
    return;
  }

  // close file descriptors that are not needed
  close(default_in);
  close(default_out);
  close(default_err);                                           
  //close(fdin);
  //close(fdout);
  //close(fderr);
  // wait for last process in the pipe line
  int status;
  if(!_background) {
    waitpid(ret, &status, 0);
  }
  else {
    previous_background_pid = ret;
  }
 
  _simpleCommands[0]->return_code = (int)WEXITSTATUS(status);

  _simpleCommands[0]->background_pid = previous_background_pid;

  command_counter++;

  int arg_size = _simpleCommands[0]->_arguments.size();
  command_last_arg = strdup(_simpleCommands[0]->_arguments[arg_size - 1]->c_str()); 

  // Clear to prepare for next command
  clear();

  // Print new prompt
  Shell::prompt();
}
