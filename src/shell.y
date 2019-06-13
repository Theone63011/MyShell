/*
 * CS-252
 * shell.y: parser for shell
 */

%code requires 
{
#include <string>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD
%token NOTOKEN
%token NEWLINE
%token GREATGREATAMPERSAND
%token GREATGREAT
%token TWOGREAT
%token GREATAMPERSAND
%token GREAT
%token LESS
%token PIPE
%token AMPERSAND

%{
//#define yylex yylex
#include <cstdio>
#include "shell.hh"

void yyerror(const char * s);
int yylex();

void expandWildCardsIfNecessary(char * arg);
void expandWildCards(char * prefix, char * arg);
int cmpfunc(const void * file1, const void * file2);

%}

%%

goal:
  commands
  ;

commands:
  command
  | commands command
  ;

command:	
  pipe_list iomodifier_list background_opt NEWLINE {
    /* printf("   Yacc: Execute command\n"); */
    Shell::_currentCommand.execute();
  }
  | NEWLINE 
  | error NEWLINE { yyerrok; }
  ;

pipe_list:
  pipe_list PIPE command_and_args
  | command_and_args
  ;

command_and_args:
  command_word argument_list {
    Shell::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

iomodifier_list:
  iomodifier_list iomodifier
  | /* can be empty */
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  WORD {
    /* printf("   Yacc: insert argument \"%s\"\n", $1->c_str()); */
    if(strcmp(Command::_currentSimpleCommand->_arguments[0]->c_str(), "echo") == 0 
      && strchr($1->c_str(), '?')) {
        Command::_currentSimpleCommand->insertArgument( $1 );
    }
    else {
      expandWildCardsIfNecessary(const_cast<char*>($1->c_str()));
    }
  }
  ;

command_word:
  WORD {
    /* printf("   Yacc: insert command \"%s\"\n", $1->c_str()); */
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;


background_opt:
  AMPERSAND {
    Shell::_currentCommand._background = 1; 
  }
  | /* can be empty */
  ;


iomodifier:
  GREATGREATAMPERSAND WORD {
    /* printf("   Yacc: append output \"%s\"\n", $2->c_str());
    printf("   Yacc: append error \"%s\"\n", $2->c_str()); */
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._errFile = $2;
    Shell::_currentCommand._append = true;
    Shell::_currentCommand._redirectOut++;
  }
  |GREATGREAT WORD {
    /* printf("   Yacc: append output \"%s\"\n", $2->c_str()); */
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._redirectOut++;
    Shell::_currentCommand._append = true;
  }
  |TWOGREAT WORD {
    /* printf("   Yacc: insert error \"%s\"\n", $2->c_str()); */
    Shell::_currentCommand._errFile = $2;
  }
  |GREATAMPERSAND WORD {
    /* printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    printf("   Yacc: insert error \"%s\"\n", $2->c_str()); */
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._errFile = $2;
    Shell::_currentCommand._redirectOut++;
  }
  |GREAT WORD {
    /* printf("   Yacc: insert output \"%s\"\n", $2->c_str()); */
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._redirectOut++;
  }
  |LESS WORD {
    /* printf("   Yacc: insert input \"%s\"\n", $2->c_str()); */
    Shell::_currentCommand._inFile = $2;
    Shell::_currentCommand._redirectIn++;
  }
  ;

%%

int maxEntries = 20; 
int nEntries = 0;
char ** entries;

void expandWildCardsIfNecessary(char * arg) {

        maxEntries = 20; 
        nEntries = 0;
        entries = (char **) malloc (maxEntries * sizeof(char *));

        if (strchr(arg, '*') || strchr(arg, '?')) {
                expandWildCards(NULL, arg);
                qsort(entries, nEntries, sizeof(char *), cmpfunc);
                for (int i = 0; i < nEntries; i++) {
                  std::string * t = new std::string(entries[i]);
                  Command::_currentSimpleCommand->insertArgument(t);
                }
        }
        else {
                std::string * t = new std::string(arg);
                Command::_currentSimpleCommand->insertArgument(t);
        }
        return;
}


int cmpfunc (const void *file1, const void *file2) {
        const char *_file1 = *(const char **)file1;
        const char *_file2 = *(const char **)file2;
        return strcmp(_file1, _file2);
}

void expandWildCards(char * prefix, char * arg) {

        char * temp = arg;
        char * save = (char *) malloc (strlen(arg) + 10);
        char * dir = save;

        if (temp[0] == '/') *(save++) = *(temp++);

        while (*temp != '/' && *temp) *(save++) = *(temp++);
        *save = '\0';

        if (strchr(dir, '*') || strchr(dir, '?')) {
                if (!prefix && arg[0] == '/') {
                        prefix = strdup("/");
                        dir++;
                }

                char * reg = (char *) malloc (2*strlen(arg) + 10);
                char * a = dir;
                char * r = reg;

                *(r++) = '^';
                while (*a) {
                        if (*a == '*') { *(r++) = '.'; *(r++) = '*'; }
                        else if (*a == '?') { *(r++) = '.'; }
                        else if (*a == '.') { *(r++) = '\\'; *(r++) = '.'; }
                        else { *(r++) = *a; }
                        a++;
                }
                *(r++) = '$'; *r = '\0';

                regex_t re;

                int expbuf = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);

                char * toOpen = strdup((prefix)?prefix:".");
                DIR * dir = opendir(toOpen);
                if (dir == NULL) {
                        perror("opendir");
                        return;
                }

                struct dirent * ent;
                regmatch_t match;

                while ((ent = readdir(dir)) != NULL) {
                        if (!regexec(&re, ent->d_name, 1, &match, 0)) {
                                if (*temp) {
                                        if (ent->d_type == DT_DIR) {
                                                char * nPrefix = (char *) malloc (150);
                                                if (!strcmp(toOpen, ".")) nPrefix = strdup(ent->d_name);
                                                else if (!strcmp(toOpen, "/")) sprintf(nPrefix, "%s%s", toOpen, ent->d_name);
                                                else sprintf(nPrefix, "%s/%s", toOpen, ent->d_name);
                                                expandWildCards(nPrefix, (*temp == '/')?++temp:temp);
                                        }
                                } else {

                                        if (nEntries == maxEntries) { maxEntries *= 2; entries = (char **) realloc (entries, maxEntries * sizeof(char *)); }
                                        char * argument = (char *) malloc (100);
                                        argument[0] = '\0';
                                        if (prefix) sprintf(argument, "%s/%s", prefix, ent->d_name);

                                        if (ent->d_name[0] == '.') {
                                                if (arg[0] == '.') {
                                                        entries[nEntries++] = (argument[0] != '\0')?strdup(argument):strdup(ent->d_name);
                                                }
                                        } else {
                                                entries[nEntries++] = (argument[0] != '\0')?strdup(argument):strdup(ent->d_name);
                                        }
                                }
                        }
                }
                closedir(dir);
        } else {
                char * preToSend = (char *) malloc (100);
                if (prefix) sprintf(preToSend, "%s/%s", prefix, dir);
                else preToSend = strdup(dir);

                if (*temp) expandWildCards(preToSend, ++temp);
        }
}



/*
int maxEntries = 20;
int nEntries = 0;
char ** entries;

void expandWildCardsIfNecessary(char * arg) {
  maxEntries = 20;
  nEntries = 0;
  entries = (char **)malloc(maxEntries * sizeof(char *));

  if(strchr(arg, '*') || strchr(arg, '?')) {
    expandWildCards(NULL, arg);
    qsort(entries, nEntries, sizeof(char *), cmpfunc);
    for(int i = 0; i < nEntries; i++) {
      std::string * t = new std::string(entries[i]);
      Command::_currentSimpleCommand->insertArgument(t);
    }
  }
  else {
    std::string * t = new std::string(arg);
    Command::_currentSimpleCommand->insertArgument(t);
  }
  return;
}

int cmpfunc (const void *file1, const void *file2) {
        const char *_file1 = *(const char **)file1;
        const char *_file2 = *(const char **)file2;
        return strcmp(_file1, _file2);
}

void expandWildCards(char * prefix, char * arg) {

  char * temp = arg;
  char * save = (char *) malloc (strlen(arg) + 10);
  char * dir = save;

  if (temp[0] == '/') {
    *(save++) = *(temp++);
  }

  while (*temp != '/' && *temp) {
    *(save++) = *(temp++);
  }
  *save = '\0';

  if (strchr(dir, '*') || strchr(dir, '?')) {
    if (!prefix && arg[0] == '/') {
      prefix = strdup("/");
      dir++;
    }

    char * reg = (char *) malloc (2*strlen(arg) + 10);
    char * a = dir;
    char * r = reg;

    *(r++) = '^';
    while (*a) {
      if (*a == '*') { 
        *(r++) = '.'; 
        *(r++) = '*'; 
      }
      else if (*a == '?') { 
        *(r++) = '.'; 
      }
      else if (*a == '.') { 
        *(r++) = '\\'; 
        *(r++) = '.'; 
      }
      else { 
        *(r++) = *a; 
      }
      a++;
    }
    
    *(r++) = '$'; *r = '\0';

    regex_t re;

    int expbuf = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);

    char * toOpen = strdup((prefix)?prefix:".");
    DIR * dir = opendir(toOpen);
    if (dir == NULL) {
      perror("opendir");
      return;
    }

    struct dirent * ent;
    regmatch_t match;

    while ((ent = readdir(dir)) != NULL) {
      if (!regexec(&re, ent->d_name, 1, &match, 0)) {
        if (*temp) {
          if (ent->d_type == DT_DIR) {
            char * nPrefix = (char *) malloc (150);
              if (!strcmp(toOpen, ".")) {
                nPrefix = strdup(ent->d_name);
              }
              else if (!strcmp(toOpen, "/")) {
                sprintf(nPrefix, "%s%s", toOpen, ent->d_name);
              }
              else {
                sprintf(nPrefix, "%s/%s", toOpen, ent->d_name);
              }
              expandWildCards(nPrefix, (*temp == '/')?++temp:temp);
          }
        } 
        else {
          if (nEntries == maxEntries) { 
            maxEntries *= 2; 
            entries = (char **) realloc (entries, maxEntries * sizeof(char *)); 
          }
          char * argument = (char *) malloc (100);
          argument[0] = '\0';
          if (prefix) {
            sprintf(argument, "%s/%s", prefix, ent->d_name);
          }

          if (ent->d_name[0] == '.') {
            if (arg[0] == '.') {
              entries[nEntries++] = (argument[0] != '\0')?strdup(argument):strdup(ent->d_name);
            }
          } 
          else {
            entries[nEntries++] = (argument[0] != '\0')?strdup(argument):strdup(ent->d_name);
          }
        }
      }
    }
    closedir(dir);
  } 
  else {
    char * preToSend = (char *) malloc (100);
    if (prefix) {
      sprintf(preToSend, "%s/%s", prefix, dir);
    }
    else {
      preToSend = strdup(dir);
    }

    if (*temp) {
      expandWildCards(preToSend, ++temp);
    }
  }
}
*/


void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

#if 0
main()
{
  yyparse();
}
#endif
