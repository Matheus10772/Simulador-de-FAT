#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
//#include "readline-5.0-1-lib/include/readline/readline.h"
//#include "readline-5.0-1-lib/include/readline/history.h"
#include <readline/readline.h>
#include <readline/history.h>

char* currentdirectory;

char* readInput();
void removeCharacter(char string[], const char operator);
void subString(char string[], char target[]);