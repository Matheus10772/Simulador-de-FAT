//#include "shell.h"
//
//char* readInput()
//{
//	char* line;
//	line = readline("GDV_Shell");
//	add_history(line);
//}
//
////int main() {
////	char* input = readInput();
////	int result;
////	char* currentDirectory = "/";
////	if (strcmp(input, "init") == 0) 
////	{
////		result = createNewVirtualFATDisk();
////		if (result == SUCCESSFUL_VALUE)
////			printf("\nNova parti��o virtual FAT criada e inicializada com sucesso!\n");
////		else
////			printf("\nFalha ao criar uma nova parti��o virtual :c\n");
////	}
////	else if (strcmp(input, "ls") == 0) 
////	{
////		printf("\nDiret�rio: %s\n", currentDirectory);
////		showAllDirEntrys(currentDirectory)
////	}
////	return 0;
////}