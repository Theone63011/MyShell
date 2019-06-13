#ifndef shell_hh
#define shell_hh

#include "command.hh"

struct Shell {

  static char * last_arg;

  static void prompt();

  static Command _currentCommand;
};

#endif
