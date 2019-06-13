#include <cstdio>
#include <cstdlib>

#include "simpleCommand.hh"

SimpleCommand::SimpleCommand() {
  _arguments = std::vector<std::string *>();
}

char * SimpleCommand::relative_path;
int SimpleCommand::background_pid;
char * SimpleCommand::_arg;

std::string * SimpleCommand::EnvironmentExpansion(std::string * argument) {
  
  std::string arg = *argument;
  std::string full_string = "";

  // Check to see if argument contains environment variables
  std::size_t check = arg.find("$");
  if(check == std::string::npos) {
    return NULL;
  }

  // Find number of environment variables to expand.
  int num_envs = 0;
  bool inside_env = false;
  for (unsigned a = 0; a < arg.length(); a++) {
    if(arg[a] == '$' && !inside_env) {
      a++;
      if(arg[a] == '{') {
        inside_env = true;
      }
    }
    else {
      if(arg[a] == '}') {
        num_envs++;
        inside_env = false;
      }
    }
  }

  // Loop through enviornment variables, adding each onto the full string 
  for (int i = 0; i < num_envs; i++) {
    std::size_t found1 = arg.find("{");
    std::size_t found2 = arg.find("}");

    if((found2 - 1) == found1) {
      return NULL;
    }
  
    if (found1 != 1) {
      std::size_t found$ = arg.find("$");
      std::string front = arg.substr(0, found$);
      full_string += front;
      arg = arg.substr(found$);
      i--;
      continue;
    }

    std::size_t inside_size = (found2 - found1) - 1;
    std::string inside = arg.substr((found1 + 1), inside_size);

    if(inside.compare("$") == 0 || inside.compare("?") == 0 || inside.compare("!") == 0 ||
       inside.compare("_") == 0 || inside.compare("SHELL") == 0) {
      if(inside.compare("$") == 0) {
        int pid = getpid();
        std::string add = std::to_string(pid);
        full_string += add;
      }
      if(inside.compare("?") == 0) {
        int code = return_code;
        std::string add = std::to_string(code);
        full_string += add;
      }
      if(inside.compare("SHELL") == 0) {
        char * path = SimpleCommand::relative_path;
        char actualpath [PATH_MAX + 1];
        char * ptr = realpath(path, actualpath);
        std::string add(ptr);
        full_string += add; 
      }
      if(inside.compare("!") == 0) {
        int pid = background_pid;
        std::string add = std::to_string(pid);
        full_string += add;
      }
      if(inside.compare("_") == 0) {
        if(_arg != NULL) {
          std::string add(_arg);
          full_string += add;
        }
      }
    }
    else {
      char * env = getenv(inside.c_str());
      std::string add(env);
      full_string += add;
    }

    arg = arg.substr(found2 + 1);
  }

  if(arg.length() > 0) {
    full_string += arg;
  }

  std::string * ret = new std::string(const_cast<char*>(full_string.c_str()));
  return ret;
}

char * SimpleCommand::TildeExpansion(char * arg) {
  if(arg[0] == '~') {
    if(strlen(arg) == 1) {
      arg = strdup(getenv("HOME"));
      return arg;
    }
    else {
      if(arg[1] == '/') {
        char * dir = strdup(getenv("HOME"));
        arg++;
        arg = strcat(dir, arg);
        return arg;
      }

      char * nArg = (char *) malloc (strlen(arg) + 20);
      char * uName = (char *) malloc (50);
      char * user = uName;
      char * temp = arg;
 
      temp++;

      while(*temp != '/' && *temp) {
        *(uName++) = *(temp++);
      }
      *uName = '\0';

      if(*temp) {
        nArg = strcat(getpwnam(user)->pw_dir, temp);
        arg = strdup(nArg);
        return arg;
      }
      else {
        arg = strdup(getpwnam(user)->pw_dir);
        return arg;
      }
    }
  }
  return NULL;
}

SimpleCommand::~SimpleCommand() {
  // iterate over all the arguments and delete them
  for (auto & arg : _arguments) {
    delete arg;
  }
}

void SimpleCommand::insertArgument( std::string * argument ) {
  // Check for Environment Variable Expansion
  std::string * check = EnvironmentExpansion(argument);
  if(check) {
    argument = check;
  }

  // Check for Tilde Expansion
  char * check2 = TildeExpansion(const_cast<char*>(argument->c_str()));
  if(check2) {
    std::string * ret2 = new std::string(check2);
    argument = ret2;
  }
  
  // simply add the argument to the vector
  _arguments.push_back(argument);
}

// Print out the simple command
void SimpleCommand::print() {
  for (auto & arg : _arguments) {
    std::cout << "\"" << *arg << "\" \t";
  }
  // effectively the same as printf("\n\n");
  std::cout << std::endl;
}
