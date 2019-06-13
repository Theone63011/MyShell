#ifndef simplcommand_hh
#define simplecommand_hh

#include <string>
#include <algorithm>
#include <iterator>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "y.tab.hh"
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
//#include "shell.hh"

struct SimpleCommand {

  // Simple command is simply a vector of strings
  std::vector<std::string *> _arguments;
  int return_code;
  static int background_pid;
  static char * relative_path;
  static char * _arg;

  SimpleCommand();
  ~SimpleCommand();
  void insertArgument( std::string * argument );
  std::string * EnvironmentExpansion(std::string * arg);
  char * TildeExpansion(char * arg);
  void print();
};

#endif
