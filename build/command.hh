#ifndef command_hh
#define command_hh

#include "simpleCommand.hh"

// Command Data Structure

struct Command {
  std::vector<SimpleCommand *> _simpleCommands;
  std::string * _outFile;
  std::string * _inFile;
  std::string * _errFile;
  bool _background;
  int _redirectIn;
  int _redirectOut;
  bool _append;
  static int previous_background_pid;
  static char * command_last_arg;
  static int command_counter;

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  void clear();
  void print();
  int builtin(int i);
  void execute();

  static SimpleCommand *_currentSimpleCommand;
};

#endif
