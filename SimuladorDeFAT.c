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
	for (int i = 0; i < DIRMaxCountEntry; i++) {
		sprintf(startRootDIR[i].filename, "%s", "NULL");
	}
	fwrite(startRootDIR, sizeof(dirEntry), DIRMaxCountEntry, FATDisk); //persistindo a inicialização do root no disco
	
	dirEntry startDataClusterDIR[dataClustersCountEntry];
	for (int i = 0; i < dataClustersCountEntry; i++) {
		sprintf(startDataClusterDIR[i].filename, "%s", "NULL");
	}
	fwrite(startDataClusterDIR, sizeof(dirEntry), dataClustersCountEntry, FATDisk);

	/*preenchendo o data cluster com 0x0000**************************************************/
	dataCluster startDataCluster;
	startDataCluster.data[dataClustersCountEntry - 128];//criando vetor
	memset(startDataCluster.data, freeClusterValue, dataClustersCountEntry * clusterSize);//setando valor de memória
	fwrite(startDataCluster.data, sizeof(uint8_t), dataClustersCountEntry * clusterSize, FATDisk);//persistindo no disco
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
	for (int i = 0; i < strlen(string); i++) {
		if (string[i] == operator || string[i] == '\n') {
			strings[j] = buffer;
			buffer = (char*)malloc(sizeof(char) * 256);
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

uint16_t writeOnAvaliableDirEntry(char name[], uint16_t cluster, uint8_t type,FILE* FATDisk)
{
	fseek(FATDisk, cluster, 0);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	uint16_t AdressEntry = cluster;
	int controle = 0;
	for (int i = 0; i < DIRMaxCountEntry; i++) {
		if (strcmp(bufferDir._dirEntry[i].filename, "NULL") == 0) {
			fseek(FATDisk, AdressEntry, 0);
			sprintf(bufferDir._dirEntry[i].filename, "%s", name);
			bufferDir._dirEntry[i].type = type;
			controle = 1;
			//Reescreve bufferDir._dirEntry[i] no disco
			break;
		}
		AdressEntry += dirEntrySize * i;
	}
	if (controle)
		return AdressEntry;
	else
		return NULL;
}

uint16_t findAvaliableDirEntry(char directory[], FILE* FATDisk, uint8_t type = isFolder)
{
	char** directoryReady = splitString(directory, '/');
	int i = 0;
	fseek(FATDisk, StartRootDIR, 0);
	dirEntry currentDir; uint16_t currentAdressDir = StartRootDIR; uint16_t startClusterAdress = StartRootDIR; fread(&currentDir, dirEntrySize, 1, FATDisk);
	fseek(FATDisk, StartRootDIR, 0);
	fread(bufferDir._dirEntry, dirEntrySize * DIRMaxCountEntry, 1, FATDisk);
	while (directoryReady[i] != 0) //indica que chegou ao fim do caminho literal
	{
		int controle = 0;
		//startClusterAdress = currentAdressDir;
		for (int k = 0; k < DIRMaxCountEntry; k++) {
			currentAdressDir = k * dirEntrySize;
			if (strcmp(bufferDir._dirEntry[k].filename, directoryReady[i]) == 0)
			{
				currentDir = bufferDir._dirEntry[k];
				fseek(FATDisk, bufferDir._dirEntry[k].firstBlock, 0);
				clearBuffer();
				fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
				controle = 1;
				break;
			}
		}
		if (controle == 0 && directoryReady[i + 1] == 0) //grava uma nova entrada de diretório caso seja necessário
		{
			currentAdressDir = writeOnAvaliableDirEntry(directoryReady[i], startClusterAdress, type, FATDisk);
			return currentAdressDir;
			//dirEntry* newDirEntry = (dirEntry*)calloc(DIRMaxCountEntry, sizeof(dirEntry));
			//fwrite(newDirEntry, DIRMaxCountEntry * dirEntrySize, 1, FATDisk); //grvando 32 novas entradas de diretório em um cluster
		}
		else if(controle == 0 && directoryReady[i + 1] != 0)
		{
			return NULL; //o diretório solicitado não foi encontrado
		}
		currentAdressDir += dirEntrySize;
		startClusterAdress += currentAdressDir;
	}

	return currentAdressDir - dirEntrySize;
}

 //Função de passagem de valores do Disco para a tabela Fat
int loadFat(){
	FILE * FATDisk = fopen(FATDiskName, "r+b");
	if(FATDisk == NULL){
		fprintf(stderr, "Falha ao abrir o disco para leitura! \n ");
		return FALIED_VALUE; 
	}
	fseek(FATDisk,StartFAT,0);
	fread(tableFAT,sizeof(FATEntry),tableFATSize,FATDisk);
	fclose(FATDisk);
	printFat();
	return SUCCESSFUL_VALUE;
}

int loadRootDir()
{
	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco para leitura! \n ");
		return FALIED_VALUE;
	}

	fseek(FATDisk, StartRootDIR, 0);
	fread(bufferRootDir, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	fclose(FATDisk);
	return SUCCESSFUL_VALUE;
}

int writeOnDisk(dataCluster* buffer, uint8_t type, char directory[])
{
	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco para leitura! \n ");
		return FALIED_VALUE;
	}

	uint16_t startAdress = findAvaliableDirEntry(directory, FATDisk, type);



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
