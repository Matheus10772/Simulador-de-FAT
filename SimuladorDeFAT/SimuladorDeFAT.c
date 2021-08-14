#include "SimuladorDeFAT.h"

int createNewVirtualFATDisk()
{
	FATDisk = fopen(FATDiskName, "wb");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao criar novo disco");
		return FALIED_VALUE;
	}

	uint8_t bootValue = 0xbb;

	for (int i = 1; i < bootBlockSize; i++) {
		fwrite(&bootValue, sizeof(bootValue), 1, FATDisk); //Preenchendo o boot block com um valor qualquer
	}

	tableFAT[0].entry = bootBlock_FAT; //Defini o primeiro valor no bloco de memória da FAT, demarcando a divisão boot block/FAT
	for (int i = 1; i < 9; i++) {
		tableFAT[i].entry = FATValue; //preenchendo os primeiros valores
	}

	for (int i = 10; i < tableFATSize; i++) {
		tableFAT[i].entry = freeClusterValue; // 0x0000 preenchendo o restante da tabela FAT com valor vazio (disponível para uso)
	}
	fwrite(tableFAT, sizeof(FATEntry), tableFATSize, FATDisk); //Persistindo a tabela FAT no disco

	memset(freeDir, freeClusterValue, sizeof(dirEntry));

	dirEntry startRootDIR[DIRMaxCountEntry]; //Inicializando o diretório root
	memset(startRootDIR, freeClusterValue, DIRMaxCountEntry * dirEntrySize);
	/*for (int i = 0; i < DIRMaxCountEntry; i++) {
		sprintf(startRootDIR[i].filename, "%s", "NULL");
	}*/
	fwrite(startRootDIR, sizeof(dirEntry), DIRMaxCountEntry, FATDisk); //persistindo a inicialização do root no disco

	
	memset(freeCluster, freeClusterValue, clusterSize);

	/*preenchendo o data cluster com 0x0000**************************************************/
	dataCluster startDataCluster;
	memset(startDataCluster.data, freeClusterValue, clusterSize);//setando valor de memória
	for (int i = 0; i < dataClustersCountEntry; i++) {
		fwrite(startDataCluster.data, clusterSize, 1, FATDisk);//persistindo no disco
	}
	//****************************************************************************************

	fclose(FATDisk);
	load();
	createNewDirEntry("", "root");

	return SUCCESSFUL_VALUE;
}

int load()
{
	FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco para leitura! \n ");
		return FALIED_VALUE;
	}
	loadFat();
	return SUCCESSFUL_VALUE;
}

void persisOnDisk()
{
	fclose(FATDisk);
	FATDisk = fopen(FATDiskName, "r+b");
}

void clearBuffer()
{
	memset(bufferDir._dirEntry, 0, DIRMaxCountEntry * dirEntrySize);
	memset(bufferData.data, 0, clusterSize);
}

char** splitString(char string[], const char operator) {//divide uma string em várias sub-strings mediante ao delimitador fornecido
	char** strings = (char**)calloc(256, sizeof(char));
	char* buffer = (char*)calloc(256, sizeof(char));
	int j = 0;
	int k = 0;
	for (unsigned int i = 0; i < strlen(string); i++) {
		if (string[i] == operator || string[i] == '\n') {
			if (strlen(buffer) > 0) {
				strings[j] = buffer;
				buffer = (char*)calloc(256, sizeof(char));
				j++;
				k = 0;
			}
		}
		else {
			buffer[k] = string[i];
			k++;
		}
	}
	strings[j] = buffer;
	return strings;
}

int createNewDirEntry(char dir[], char name[])
{
	uint16_t adress = loadAvaliableDIrEntry(dir);
	tableFAT[adress].entry = endCluster;
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	if(memcmp(bufferDir._dirEntry, freeCluster, clusterSize) != 0)
	{
		 int indexForWrite = findFreeDir(&bufferDir); //olhar isso aqui "&"
		 if (indexForWrite == FALIED_VALUE)
			 return FALIED_VALUE;
		 sprintf(bufferDir._dirEntry[indexForWrite].filename, "%s", name);
		 bufferDir._dirEntry[indexForWrite].firstBlock = findFreeCluster();
		 bufferDir._dirEntry[indexForWrite].type = isFolder;
		 fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
		 fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	}
	else 
	{
		/*dirEntry startDIREntry[DIRMaxCountEntry];
		for (int i = 0; i < DIRMaxCountEntry; i++) {
			sprintf(startDIREntry[i].filename, "%s", "NULL");
		}*/
		/*fwrite(startDIREntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
		fseek(FATDisk, adress, SEEK_SET);*/
		//clearBuffer();
		//fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);

		sprintf(bufferDir._dirEntry[0].filename, "%s", name);
		bufferDir._dirEntry[0].firstBlock = findFreeCluster();
		bufferDir._dirEntry[0].type = isFolder;
		fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
		fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	}
	fclose(FATDisk);
	return SUCCESSFUL_VALUE;
}

uint16_t loadAvaliableDIrEntry(char directory[])
{
	if (strlen(directory) == 0)
		return StartRootDIR / totalCountCluster;
	else {
		char** directoryReady = splitString(directory, '/');
		int controle = 0;
		int index = 0;
		uint16_t currentAdress = StartRootDIR / totalCountCluster;
		fseek(FATDisk, StartRootDIR, 0);
		fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
		while (directoryReady[index] != 0) {
			for (int i = 0; i < DIRMaxCountEntry; i++) {
				if (memcmp(&(bufferDir._dirEntry[i]), freeDir, dirEntrySize) != 0) { //não está vazio{
					if (strcmp(bufferDir._dirEntry[i].filename, directoryReady[index]) == 0) { //São iguais
						currentAdress = bufferDir._dirEntry[i].firstBlock;
						clearBuffer();
						fseek(FATDisk, currentAdress * totalCountCluster, SEEK_SET);
						fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
						controle = 1;
						break;
					}
				}
			}
			if (!controle)
				return NULL; //diretorio não encontrado
			index++;
		}
		return currentAdress;
	}
}

 //Função de passagem de valores do Disco para a tabela Fat
int loadFat(){
	
	fseek(FATDisk,StartFAT,SEEK_SET);
	fread(tableFAT,sizeof(FATEntry),tableFATSize,FATDisk);
	fclose(FATDisk);
	//printFat();
	return SUCCESSFUL_VALUE;
}

uint16_t findFreeCluster()
{
	uint16_t adress;
	for (int i = 10; i < tableFATSize; i++) 
	{
		if (tableFAT[i].entry == freeClusterValue)
		{
			adress = i;
			break;
		}
	}

	return adress;
}

int findFreeDir(dataCluster* bufferDIR)
{
	for (int i = 0; i < DIRMaxCountEntry; i++) {
		if (memcmp(&(bufferDIR->_dirEntry[i]), freeDir,dirEntrySize) == 0 || strcmp(bufferDIR->_dirEntry[i].filename, "NULL") == 0)
			return i;
	}

	return FALIED_VALUE;
}

int loadRootDir()
{

	fseek(FATDisk, StartRootDIR, SEEK_SET);
	fread(bufferRootDir, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	fclose(FATDisk);
	return SUCCESSFUL_VALUE;
}

int writeDataOnDisk(dataCluster buffer[], int countBuffer, char directory[], char arqName[])
{
	
	uint16_t adress = loadAvaliableDIrEntry(directory);
	if (adress == NULL)
		return FALIED_VALUE;
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	int indexForWrite;
	for (indexForWrite = 0; indexForWrite < DIRMaxCountEntry; indexForWrite++) {
		if (strcmp(bufferDir._dirEntry[indexForWrite].filename, arqName) == 0)
			break;
	}
	bufferDir._dirEntry[indexForWrite].size = sizeof(dataCluster) * countBuffer;
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	uint16_t adressFAT = bufferDir._dirEntry[indexForWrite].firstBlock;
	tableFAT[adressFAT].entry = endCluster;
	for (int i = 0; i < countBuffer; i++) //laço de repetição para alocar os blocos de dados seguintes
	{
		tableFAT[adressFAT].entry = findFreeCluster();
		fseek(FATDisk, tableFAT[adressFAT].entry * totalCountCluster, SEEK_SET);
		fwrite(buffer[i].data, clusterSize, 1, FATDisk);
		adressFAT = tableFAT[adressFAT].entry;
		tableFAT[adressFAT].entry = endCluster;
	}
	fclose(FATDisk);
	return SUCCESSFUL_VALUE;
		
}
	

int createNewFile(char directory[], char arqName[])
{
	uint16_t adress = loadAvaliableDIrEntry(directory);
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();
	
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	int indexForWrite = findFreeDir(&bufferDir);
	if (indexForWrite == FALIED_VALUE)
		return FALIED_VALUE;
	else {
		tableFAT[adress].entry = endCluster;
		sprintf(bufferDir._dirEntry[indexForWrite].filename, "%s", arqName);
		//bufferDir._dirEntry[indexForWrite].size = sizeof(dataCluster) * countBuffer;
		bufferDir._dirEntry[indexForWrite].type = isFile;
		bufferDir._dirEntry[indexForWrite].firstBlock = findFreeCluster();
		fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
		fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
		//fseek(FATDisk, bufferDir._dirEntry[indexForWrite].firstBlock * totalCountCluster, SEEK_SET);
	}
	fclose(FATDisk);
	return SUCCESSFUL_VALUE;
}

int replaceDataOnDisk(dataCluster buffer[], int countBuffer, char directory[], char arqName[])
{
	int result = deleteEntryOnDisk(directory, arqName);
	if (result == FALIED_VALUE)
		return FALIED_VALUE;
	result = writeDataOnDisk(buffer, countBuffer, directory, arqName);
	if (result == FALIED_VALUE)
		return FALIED_VALUE;

	return SUCCESSFUL_VALUE;
}

int writeDirOnDisk(char directory[], char name[])
{
	int result = createNewDirEntry(directory, name);
	if (result == FALIED_VALUE)
		return FALIED_VALUE;

	return SUCCESSFUL_VALUE;
}

int deleteEntryOnDisk(char directory[], char arqName[])
{

	printf("\nqtd caracteres: %d\n", strlen(directory));
	printf("ajazxaduhj");
	uint16_t adress = loadAvaliableDIrEntry(directory);
	/*if (adress == NULL)
		return FALIED_VALUE;*/
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	int indiceDirEntry;
	for (indiceDirEntry = 0; indiceDirEntry < DIRMaxCountEntry; indiceDirEntry++)
	{
		printf("primeiro for");
		if (strcmp(bufferDir._dirEntry[indiceDirEntry].filename, arqName) == 0)
			break;
	}
	uint16_t FATAdress = bufferDir._dirEntry[indiceDirEntry].firstBlock;
	memset(&(bufferDir._dirEntry[indiceDirEntry]), freeDir, sizeof(dirEntry));
	/*sprintf(bufferDir._dirEntry[indiceDirEntry].filename, "%s", "NULL");
	bufferDir._dirEntry[indiceDirEntry].size = -1;
	bufferDir._dirEntry[indiceDirEntry].type = -1;*/
	uint16_t lastFATAdress;
	while (tableFAT[FATAdress].entry != endCluster)
	{
		lastFATAdress = FATAdress;
		FATAdress = tableFAT[FATAdress].entry;
		tableFAT[lastFATAdress].entry = freeClusterValue;
	}
	tableFAT[FATAdress].entry = freeClusterValue;
	//bufferDir._dirEntry[indiceDirEntry].firstBlock = -1;
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	return SUCCESSFUL_VALUE;
}

int deleteDirOnDisk(char directory[], char dirName[])
{

	char* FullDIR = (char*)calloc(256, sizeof(char));
	strcpy(FullDIR, directory);
	strcat(FullDIR, "/");
	strcat(FullDIR, dirName);

	uint16_t adress = loadAvaliableDIrEntry(FullDIR);
	int indexDirAndress;

	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	for (indexDirAndress = 0; indexDirAndress < DIRMaxCountEntry; indexDirAndress++)
	{
		if (memcmp(&(bufferDir._dirEntry[indexDirAndress]), freeDir, sizeof(dirEntry)) != 0 && strcmp(bufferDir._dirEntry[indexDirAndress].filename, "NULL") != 0)
			return FALIED_VALUE;
	}

	adress = loadAvaliableDIrEntry(directory);
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	for (indexDirAndress = 0; indexDirAndress < DIRMaxCountEntry; indexDirAndress++)
	{
		if (strcmp(bufferDir._dirEntry[indexDirAndress].filename, dirName) == 0)
			break;
	}

	/*sprintf(bufferDir._dirEntry[indexDirAndress].filename, "%s", "NULL");
	bufferDir._dirEntry[indexDirAndress].type = -1;*/
	uint16_t FATAdress = adress + indexDirAndress;
	memcpy(&(bufferDir._dirEntry[indexDirAndress]), freeDir, sizeof(dirEntry));
	if(memcmp(bufferDir._dirEntry, freeCluster, clusterSize) == 0)
		tableFAT[FATAdress].entry = freeClusterValue;
	//bufferDir._dirEntry[indexDirAndress].firstBlock = -1;
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	fclose(FATDisk);
	return SUCCESSFUL_VALUE;
}

dataCluster* readDataOnDisk(char directory[], char arqName[])
{
	uint16_t adress = loadAvaliableDIrEntry(directory);
	if (adress == NULL)
		return FALIED_VALUE;
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	int indexDirAndress;
	for (indexDirAndress = 0; indexDirAndress < DIRMaxCountEntry; indexDirAndress++)
	{
		if (strcmp(bufferDir._dirEntry[indexDirAndress].filename, arqName) == 0)
			break;
	}
	int TAMOfBuffer = 5;
	dataCluster* buffer = (dataCluster*)calloc(TAMOfBuffer,sizeof(dataCluster));
	uint16_t FATAdress = bufferDir._dirEntry[indexDirAndress].firstBlock;
	fseek(FATDisk, FATAdress * totalCountCluster, SEEK_SET);
	fread(bufferData.data, clusterSize, 1, FATDisk);
	int bufferIndex = 0;
	memcpy(buffer[bufferIndex].data, bufferData.data, clusterSize);
	bufferIndex++;
	while (tableFAT[FATAdress].entry != endCluster)
	{
		FATAdress = tableFAT[FATAdress].entry;
		fseek(FATDisk, FATAdress * totalCountCluster, SEEK_SET);
		fread(bufferData.data, clusterSize, 1, FATDisk);
		memcpy(buffer[bufferIndex].data, bufferData.data, clusterSize);
		bufferIndex++;
		if(bufferIndex == TAMOfBuffer)
		{
			TAMOfBuffer = TAMOfBuffer * 2;
			buffer = realloc(buffer, TAMOfBuffer);
		}
	}
	return buffer;
}

int showAllDirEntrys(char directory[])
{

	uint16_t adress = loadAvaliableDIrEntry(directory);
	if (adress == NULL)
		return FALIED_VALUE;
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	printf("\n");
	for (int i = 0; i < DIRMaxCountEntry; i++) 
	{
		if (strlen(bufferDir._dirEntry[i].filename) > 0 && strcmp(bufferDir._dirEntry[i].filename, "NULL") != 0) {
			printf("%s", bufferDir._dirEntry[i].filename);
			if (bufferDir._dirEntry[i].type == isFolder)
				printf("/");
			printf("\n");
		}
	}
	printf("\n");
	return SUCCESSFUL_VALUE;
}

//Função de impressão da Table Fat
void printFat(){
	for (int i = 0; i < tableFATSize; i++)
	{
		printf(" %x ", tableFAT[i].entry);
		if (i % 6 == 0)
			printf("\n");
	}
	
}

//int main()
//{
//	int result;
//	printf("\nEntrei\n");
//	createNewVirtualFATDisk();
//	//loadFat();
//	writeDirOnDisk("/root", "recursos");
//	//writeDirOnDisk("/root", "dirteste");
//	//writeDirOnDisk("/root/recursos", "profundidade");
//	//showAllDirEntrys("/root");
//	//deleteDirOnDisk("/root/recursos", "profundidade");
//	//showAllDirEntrys("/root/recursos");
//	dataCluster* teste = (dataCluster*)calloc(2, sizeof(dataCluster));
//	char* testeDeArquivo = "testandoEscritaDeArquivo";
//	memcpy(teste[0].data, testeDeArquivo, 24);
//	memcpy(teste[1].data, testeDeArquivo, 24);
//	createNewFile("/root/recursos", "teste.txt");
//	writeDataOnDisk(teste, 2, "/root/recursos", "teste.txt");
//	deleteEntryOnDisk("/root/recursos", "teste.txt");
//	showAllDirEntrys("/root/recursos");
//	printf("Feito");
//	return 0;
//
//}

int main() {
	currentDirectory = (char*)calloc(256, sizeof(char));
	sprintf(currentDirectory, "%s", "/root ");
	while (1)
	{
		char* input = readInput();
		char** inputs;
		inputs = splitString(input, ' ');
		int result;
		printf("\nO que está no 0: %s\n", inputs[0]);
		printf("\nO que está no 1: %s\n", inputs[1]);
		if (strcmp(inputs[0], "init") == 0)
		{
			result = createNewVirtualFATDisk();
			if (result == SUCCESSFUL_VALUE)
				printf("\nNova partição virtual FAT criada e inicializada com sucesso!\n");
			else
				printf("\nFalha ao criar uma nova partição virtual :c\n");
		}
		else if (strcmp(inputs[0], "ls") == 0)
		{
			result = showAllDirEntrys(currentDirectory);
			if (result == FALIED_VALUE)
				printf("\nO diretório informado não existe\n");
			persisOnDisk();
		}
		else if (strcmp(inputs[0], "mkdir") == 0)
		{
			char** readyDIR = splitString(inputs[1], '/');
			char* folderName = (char*)calloc(256, sizeof(char));
			int index = 0;
			while (readyDIR[index] != 0)
			{
				index++;
				printf("\nindex: %s\n", readyDIR[index]);
			}
			index--;
			sprintf(folderName, "%s", readyDIR[index]);
			printf("\nFolder name: %s\n", folderName);
			subString(inputs[1], folderName);
			printf("\nSubstring: %s\n", inputs[1]);
			result = createNewDirEntry(inputs[1], folderName);
			if (result == FALIED_VALUE)
				printf("\nNão foi possível criar o diretório. Ele pode não existe ou você pode estar sem espaço em disco");
			persisOnDisk();
			/*free(readyDIR);
			free(folderName);*/
		}
		else if (strcmp(inputs[0], "create") == 0)
		{
			char** readyDIR = splitString(inputs[1], '/');
			char* arqName = (char*)calloc(256, sizeof(char));
			int index = 0;
			while (readyDIR[index] != 0)
			{
				index++;
			}
			index--;
			sprintf(arqName, "%s", readyDIR[index]);
			subString(inputs[1], arqName);
			printf("\narqName: %s\n", arqName);
			printf("\ndir: %s\n", inputs[1]);
			result = createNewFile(inputs[1], arqName);
			if (result == FALIED_VALUE)
				printf("\nNão foi possível criar o arquivo. O diretório informado não existe ou não existe espaço em disco disponível");
			persisOnDisk();
			free(readyDIR);
			free(arqName);
		}
		else if (strcmp(inputs[0], "unlink") == 0)
		{
			char** readyDIR = splitString(inputs[1], '/');
			char* arqName = (char*)calloc(256, sizeof(char));
			int index = 0;
			while (readyDIR[index] != 0)
			{
				index++;
			}
			index--;
			sprintf(arqName, "%s", readyDIR[index]);
			subString(inputs[1], arqName);
			printf("\narqName: %s\n", arqName);
			printf("\ndir: %s\n", inputs[1]);
			result = deleteEntryOnDisk(inputs[1], arqName);
			if (result == FALIED_VALUE)
				printf("\nNão foi possível deletar o arquivo. O diretório informado não existe\n");
			persisOnDisk();
		}
		else if (strcmp(inputs[0], "write") == 0)
		{
			char** readyDIR = splitString(inputs[2], '/');
			strcat(inputs[1], "\0");
			char* arqName = (char*)calloc(256, sizeof(char));
			int index = 0;
			while (readyDIR[index] != 0)
			{
				index++;
			}
			index--;
			sprintf(arqName, "%s", readyDIR[index]);
			subString(inputs[2], arqName);
			int TAMOfBuffer = 10;
			dataCluster* buffer = (dataCluster*)calloc(TAMOfBuffer, sizeof(dataCluster));
			index = 0;
			int indexBuffer = 0;
			int indexData = 0;
			while (inputs[1][index] != '\0')
			{
				buffer[indexBuffer].data[indexData] = inputs[1][index];
				index++;
				indexData++;
				if (indexData >= 1024)
				{
					indexBuffer++;
					indexData = 0;
					if (indexBuffer >= TAMOfBuffer)
					{
						TAMOfBuffer = TAMOfBuffer * 2;
						buffer = realloc(buffer, TAMOfBuffer);
					}
				}
			}
			printf("\nAntes de chamar a função\n");
			result = writeDataOnDisk(buffer, indexBuffer, input[2], arqName);
			if (result == FALIED_VALUE)
				printf("\nNão foi possível completar a operação. O arquivo não existe ou não existe espaço em disco disponível\n");
			persisOnDisk();
		}
		else if (strcmp(inputs[0], "read") == 0)
		{
			char** readyDIR = splitString(inputs[1], '/');
			char* arqName = (char*)calloc(256, sizeof(char));
			int index = 0;
			while (readyDIR[index] != 0)
			{
				index++;
			}
			index--;
			sprintf(arqName, "%s", readyDIR[index]);
			subString(inputs[1], arqName);
			dataCluster* buffer = readDataOnDisk(inputs[1], arqName);
			index = 0;
			while (memcmp(buffer[index].data, freeCluster, clusterSize) != 0) {
				printf("%s", buffer[index].data);
				index++;
			}
			persisOnDisk();
		}
		else {
			printf("\nComando não encontrado\n");
		}
	}
	return 0;
}
