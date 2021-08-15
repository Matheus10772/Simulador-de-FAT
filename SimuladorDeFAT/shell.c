#include "shell.h"



char* readInput()
{
	char* line;
	char* message = (char*)calloc(256, sizeof(char));
	sprintf(message, "%s", "GDV_Shell> ");
	/*strcat(message, currentDirectory);
	strcat(message, " ");*/
	line = readline(message);
	add_history(line);
	return line;
}

void removeCharacter(char string[], const char operator)
{
	int i, j;
	for (i = 0, j = 0; string[i] != '\0'; i++)
		if (string[i] != operator) string[j++] = string[i];
	string[j] = '\0';
	return j;
}

void subString(char string[], char target[])
{
	int i = 0, j = 0, index = 0;

	while (string[index] != '\0')
	{
		if (string[index] == target[i]) {
			i = index;
			while (string[index] != '\0')
			{
				if (string[index] == target[j])
					j++;
				else
				{
					index = i;
					j = 0;
					i = 0;
					break;
				}
				index++;
			}
		}
		index++;
	}
	j += i;
	for (i; i < j; i++) {
		string[i] = 0;
	}
}

