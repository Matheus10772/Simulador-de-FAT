#include "SimuladorDeFAT.h"

int createNewVirtualFATDisk()
{
	FILE* FATDisk = fopen(FATDiskName, "wb");
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

	dirEntry startRootDIR[DIRMaxCountEntry]; //Inicializando o diretório root
	memset(startRootDIR, freeClusterValue, DIRMaxCountEntry * dirEntrySize);
	/*for (int i = 0; i < DIRMaxCountEntry; i++) {
		sprintf(startRootDIR[i].filename, "%s", "NULL");
	}*/
	fwrite(startRootDIR, sizeof(dirEntry), DIRMaxCountEntry, FATDisk); //persistindo a inicialização do root no disco

	

	/*preenchendo o data cluster com 0x0000**************************************************/
	dataCluster startDataCluster;
	memset(startDataCluster.data, freeClusterValue, clusterSize);//setando valor de memória
	for (int i = 0; i < dataClustersCountEntry; i++) {
		fwrite(startDataCluster.data, clusterSize, 1, FATDisk);//persistindo no disco
	}
	//****************************************************************************************

	fclose(FATDisk);
	return SUCCESSFUL_VALUE;
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
			strings[j] = buffer;
			buffer = (char*)calloc(256, sizeof(char));
			j++;
			k = 0;
		}
		else {
			buffer[k] = string[i];
			k++;
		}
	}
	return strings;
}

int createNewDirEntry(char dir[], FILE* FATDisk, char name[])
{
	char** directoryReady = splitString(dir, '/');
	char* dirForFind = (char*)calloc(1024, sizeof(char)); //alterar depois
	sprintf(dirForFind, "%s", "");
	int index = 0;
	while (1) {
		strcat(dirForFind, "/");
		if (directoryReady[index + 2] == 0) //acabou
			break;
		else
			strcat(dirForFind, directoryReady[index]);
		index++;
	}
	uint16_t adress = loadAvaliableDIrEntry(dirForFind, FATDisk);
	fseek(FATDisk, adress, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	if(memcmp(bufferDir._dirEntry, freeCluster, clusterSize) != 0)
	{
		 int indexForWrite = findFreeDir(&bufferDir); //olhar isso aqui "&"
		 if (indexForWrite == FALIED_VALUE)
			 return FALIED_VALUE;
		 sprintf(bufferDir._dirEntry[indexForWrite].filename, "%s", name);
		 bufferDir._dirEntry[indexForWrite].firstBlock = findFreeCluster(FATDisk);
		 bufferDir._dirEntry[indexForWrite].type = isFolder;
		 fseek(FATDisk, adress, SEEK_SET);
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
		bufferDir._dirEntry[0].firstBlock = findFreeCluster(FATDisk);
		bufferDir._dirEntry[0].type = isFolder;
		fseek(FATDisk, adress, SEEK_SET);
		fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	}

	return SUCCESSFUL_VALUE;
}

uint16_t loadAvaliableDIrEntry(char directory[], FILE* FATDisk)
{
	if (strcmp(directory, "/") == 0)
		return StartRootDIR;
	else {
		char** directoryReady = splitString(directory, '/');
		int controle = 0;
		int index = 0;
		uint16_t currentAdress = StartRootDIR;
		fseek(FATDisk, StartRootDIR, 0);
		fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
		while (directoryReady[index] != 0) {
			for (int i = 0; i < DIRMaxCountEntry; i++) {
				if (memcmp(&(bufferDir._dirEntry[i]), freeDir, dirEntrySize) != 0) { //não está vazio{
					if (strcmp(bufferDir._dirEntry[i].filename, directoryReady[index]) == 0) { //São iguais
						currentAdress = bufferDir._dirEntry[i].firstBlock;
						clearBuffer();
						fseek(FATDisk, currentAdress, SEEK_SET);
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
	FILE * FATDisk = fopen(FATDiskName, "r+b");
	if(FATDisk == NULL){
		fprintf(stderr, "Falha ao abrir o disco para leitura! \n ");
		return FALIED_VALUE; 
	}
	fseek(FATDisk,StartFAT,SEEK_SET);
	fread(tableFAT,sizeof(FATEntry),tableFATSize,FATDisk);
	fclose(FATDisk);
	printFat();
	return SUCCESSFUL_VALUE;
}

uint16_t findFreeCluster(FILE* FATDisk)
{
	fseek(FATDisk, StartDataCluster, SEEK_SET);
	uint16_t currentAdress = StartDataCluster;
	for (int i = 0; i < dataClustersCountEntry; i++) {
		fread(bufferData.data, clusterSize, 1, FATDisk);
		if (memcmp(bufferData.data, freeCluster, clusterSize) == 0) {
			return currentAdress;
		}
		currentAdress += clusterSize;
	}
	return NULL;
}

int findFreeDir(dataCluster* bufferDIR)
{
	for (int i = 0; i < DIRMaxCountEntry; i++) {
		if (memcmp(&(bufferDIR->_dirEntry[i]), freeDir,dirEntrySize) == 0)
			return i;
	}

	return FALIED_VALUE;
}

int loadRootDir()
{
	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco para leitura! \n ");
		return FALIED_VALUE;
	}

	fseek(FATDisk, StartRootDIR, SEEK_SET);
	fread(bufferRootDir, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	fclose(FATDisk);
	return SUCCESSFUL_VALUE;
}

int writeDataOnDisk(dataCluster buffer[], int countBuffer, char directory[], char arqName[])
{
	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco para leitura! \n ");
		return FALIED_VALUE;
	}

	uint16_t adress = loadAvaliableDIrEntry(directory, FATDisk);
	fseek(FATDisk, adress, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	int indexForWrite = findFreeDir(&bufferDir);
	if (indexForWrite == FALIED_VALUE)
		return FALIED_VALUE;
	else {
		sprintf(bufferDir._dirEntry[indexForWrite].filename, "%s", arqName);
		bufferDir._dirEntry[indexForWrite].size = sizeof(dataCluster) * countBuffer;
		bufferDir._dirEntry[indexForWrite].type = isFile;
		fseek(FATDisk, bufferDir._dirEntry[indexForWrite].firstBlock, SEEK_SET);
		fwrite(buffer[0].data, clusterSize, 1, FATDisk);
		//laço de repetição para alocar os blocos de dados seguintes
	}
	
}

int writeDirOnDisk(char directory[])
{
	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco para leitura! \n ");
		return FALIED_VALUE;
	}
	
	char** directoryReady = splitString(directory, '/');
	int index = 0;
	char* name;
	while (directoryReady[index] != 0)
	{
		name = directoryReady[index];
		index++;
	}

	int result = createNewDirEntry(directory, FATDisk, name);
	if (result == FALIED_VALUE)
		return FALIED_VALUE;
	return SUCCESSFUL_VALUE;
}

//Função de impressão da Table Fat
void printFat(){
	for (int i = 0; i < tableFATSize; i++)
	{
		printf(" %x \n", tableFAT[i].entry);
	}
	
}

int main()
{
	int result;
	createNewVirtualFATDisk();
	result = Transfer_Fat();

	return 0;

}
