#include "SimuladorDeFAT.h"

int createNewVirtualFATDisk()
{
	FILE* FATDisk = fopen(FATDiskName, "wb");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao criar um novo disco virtual");
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
	if (createNewDirEntry("/", "padrao") == FALIED_VALUE)
		return FALIED_VALUE;

	return SUCCESSFUL_VALUE;
}

int load()
{
	clearBuffer();
	if (loadFat() == FALIED_VALUE)
		return FALIED_VALUE;
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
	if (adress == FALIED_VALUE)
		return FALIED_VALUE;
	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco");
		return FALIED_VALUE;
	}
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	if (memcmp(bufferDir._dirEntry, freeCluster, clusterSize) != 0)
	{
		int indexForWrite = findFreeDir(&bufferDir);
		if (indexForWrite == FALIED_VALUE)
			return FALIED_VALUE;
		sprintf(bufferDir._dirEntry[indexForWrite].filename, "%s", name);
		bufferDir._dirEntry[indexForWrite].firstBlock = findFreeCluster();
		tableFAT[bufferDir._dirEntry[indexForWrite].firstBlock].entry = endCluster;
		if (bufferDir._dirEntry[indexForWrite].firstBlock == FALIED_VALUE)
			return FALIED_VALUE;
		bufferDir._dirEntry[indexForWrite].type = isFolder;
		fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
		fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	}
	else
	{
		tableFAT[adress].entry = endCluster;
		sprintf(bufferDir._dirEntry[0].filename, "%s", name);
		bufferDir._dirEntry[0].firstBlock = findFreeCluster();
		if (bufferDir._dirEntry[0].firstBlock == FALIED_VALUE)
			return FALIED_VALUE;
		tableFAT[bufferDir._dirEntry[0].firstBlock].entry = endCluster;
		bufferDir._dirEntry[0].type = isFolder;
		fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
		fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	}
	fclose(FATDisk);
	return SUCCESSFUL_VALUE;
}

uint16_t loadAvaliableDIrEntry(char directory[])
{
	if (strcmp(directory, "/") == 0)
		return StartRootDIR / totalCountCluster;
	else {
		FILE* FATDisk = fopen(FATDiskName, "r+b");
		if (FATDisk == NULL) {
			fprintf(stderr, "Falha ao abrir o disco");
			return FALIED_VALUE;
		}
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
				return FALIED_VALUE; //diretorio não encontrado
			index++;
		}
		fclose(FATDisk);
		return currentAdress;
	}
}

//Função de passagem de valores do Disco para a tabela Fat
int loadFat() {

	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco");
		return FALIED_VALUE;
	}
	fseek(FATDisk, StartFAT, SEEK_SET);
	fread(tableFAT, sizeof(FATEntry), tableFATSize, FATDisk);
	fclose(FATDisk);
	return SUCCESSFUL_VALUE;
}

uint16_t findFreeCluster()
{
	uint16_t adress = FALIED_VALUE;
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
		if (memcmp(&(bufferDIR->_dirEntry[i]), freeDir, dirEntrySize) == 0 || strcmp(bufferDIR->_dirEntry[i].filename, "NULL") == 0)
			return i;
	}

	return FALIED_VALUE;
}

int loadRootDir()
{

	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco");
		return FALIED_VALUE;
	}
	fseek(FATDisk, StartRootDIR, SEEK_SET);
	fread(bufferRootDir, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	fclose(FATDisk);
	return SUCCESSFUL_VALUE;
}

int writeDataOnDisk(dataCluster buffer[], int countBuffer, char directory[], char arqName[])
{
	uint16_t adress = loadAvaliableDIrEntry(directory);
	if (adress == FALIED_VALUE)
		return FALIED_VALUE;
	FILE* FATDisk;
	FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco");
		return FALIED_VALUE;
	}
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	int indexForWrite;
	for (indexForWrite = 0; indexForWrite < DIRMaxCountEntry; indexForWrite++) {
		if (strcmp(bufferDir._dirEntry[indexForWrite].filename, arqName) == 0) //{
			break;
	}
	if (bufferDir._dirEntry[indexForWrite].firstBlock != endCluster)
	{
		fclose(FATDisk);
		deleteEntryOnDisk(directory, arqName);
		createNewFile(directory, arqName);
		FATDisk = fopen(FATDiskName, "r+b");
		if (FATDisk == NULL) {
			fprintf(stderr, "Falha ao abrir o disco");
			return FALIED_VALUE;
		}
	}
	bufferDir._dirEntry[indexForWrite].size = sizeof(dataCluster) * countBuffer;
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	bufferDir._dirEntry[indexForWrite].firstBlock = findFreeCluster();
	uint16_t adressFAT = bufferDir._dirEntry[indexForWrite].firstBlock;
	fseek(FATDisk, adressFAT * totalCountCluster, SEEK_SET);
	fwrite(buffer[0].data, clusterSize, 1, FATDisk);
	tableFAT[adressFAT].entry = endCluster;
	for (int i = 1; i < countBuffer; i++) //laço de repetição para alocar os blocos de dados seguintes
	{
		tableFAT[adressFAT].entry = findFreeCluster();
		if (tableFAT[adressFAT].entry == FALIED_VALUE)
			return FALIED_VALUE;
		fseek(FATDisk, tableFAT[adressFAT].entry * totalCountCluster, SEEK_SET);
		fwrite(buffer[i].data, clusterSize, 1, FATDisk);
		adressFAT = tableFAT[adressFAT].entry;
		tableFAT[adressFAT].entry = endCluster;
	}
	fclose(FATDisk);
	return SUCCESSFUL_VALUE;

}

int appendDataOnDisk(dataCluster buffer[], int countBuffer, char directory[], char arqName[])
{
	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco");
		return FALIED_VALUE;
	}
	uint16_t adress = loadAvaliableDIrEntry(directory);
	if (adress == FALIED_VALUE)
		return FALIED_VALUE;
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	int indexForWrite;
	for (indexForWrite = 0; indexForWrite < DIRMaxCountEntry; indexForWrite++) {
		if (strcmp(bufferDir._dirEntry[indexForWrite].filename, arqName) == 0)
			break;
	}
	uint16_t FATAdress = bufferDir._dirEntry[indexForWrite].firstBlock;
	while (tableFAT[FATAdress].entry != endCluster)
	{
		FATAdress = tableFAT[FATAdress].entry;
	}
	fseek(FATDisk, FATAdress * totalCountCluster, SEEK_SET);
	fread(bufferData.data, clusterSize, 1, FATDisk);
	int lengthOfData = strlen(bufferData.data);
	int lengthOfbuffer = strlen(buffer[0].data);
	int indexOfBuffer = 0;

	if (lengthOfData + lengthOfbuffer <= clusterSize)
	{
		strcat(bufferData.data, buffer[indexOfBuffer].data);
		fseek(FATDisk, FATAdress * totalCountCluster, SEEK_SET);
		fwrite(bufferData.data, clusterSize, 1, FATDisk);
	}
	else
	{
		tableFAT[FATAdress].entry = findFreeCluster();
		if (tableFAT[FATAdress].entry == FALIED_VALUE)
			return FALIED_VALUE;
		fseek(FATDisk, tableFAT[FATAdress].entry * totalCountCluster, SEEK_SET);
		fwrite(buffer[indexOfBuffer].data, clusterSize, 1, FATDisk);
		FATAdress = tableFAT[FATAdress].entry;
	}
	for (indexOfBuffer = 1; indexOfBuffer < countBuffer; indexOfBuffer++)
	{
		tableFAT[FATAdress].entry = endCluster;
		tableFAT[FATAdress].entry = findFreeCluster();
		if (tableFAT[FATAdress].entry == FALIED_VALUE)
			return FALIED_VALUE;
		fseek(FATDisk, tableFAT[FATAdress].entry * totalCountCluster, SEEK_SET);
		fwrite(buffer[indexOfBuffer].data, clusterSize, 1, FATDisk);
		FATAdress = tableFAT[FATAdress].entry;
	}
	tableFAT[FATAdress].entry = endCluster;
	fclose(FATDisk);

	return SUCCESSFUL_VALUE;
}


int createNewFile(char directory[], char arqName[])
{
	uint16_t adress = loadAvaliableDIrEntry(directory);
	if (adress == FALIED_VALUE)
		return FALIED_VALUE;
	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco");
		return FALIED_VALUE;
	}
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();

	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	int indexForWrite = findFreeDir(&bufferDir);
	if (indexForWrite == FALIED_VALUE)
		return FALIED_VALUE;
	else {
		tableFAT[adress].entry = endCluster;
		sprintf(bufferDir._dirEntry[indexForWrite].filename, "%s", arqName);
		bufferDir._dirEntry[indexForWrite].type = isFile;
		bufferDir._dirEntry[indexForWrite].firstBlock = endCluster;
		/*if (bufferDir._dirEntry[indexForWrite].firstBlock == FALIED_VALUE)
			return FALIED_VALUE;*/
			//tableFAT[bufferDir._dirEntry[indexForWrite].firstBlock].entry = endCluster;
		fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
		fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
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
	printf("\ndirectory: %s\n", directory);
	printf("\narqnmae: %s\n", arqName);

	uint16_t adress = loadAvaliableDIrEntry(directory);
	if (adress == FALIED_VALUE) {
		return FALIED_VALUE;
	}
	printf("\nEndereço inicio do cluster: %x\n", adress);
	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco");
		return FALIED_VALUE;
	}
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	clearBuffer();
	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	int indiceDirEntry;
	for (indiceDirEntry = 0; indiceDirEntry < DIRMaxCountEntry; indiceDirEntry++)
	{
		if (strcmp(bufferDir._dirEntry[indiceDirEntry].filename, arqName) == 0)
			break;
	}
	printf("\nEndereço dir: %d\n", indiceDirEntry);
	uint16_t FATAdress = bufferDir._dirEntry[indiceDirEntry].firstBlock;
	memcpy(&(bufferDir._dirEntry[indiceDirEntry]), freeDir, sizeof(dirEntry));
	uint16_t lastFATAdress;
	while (tableFAT[FATAdress].entry != endCluster)
	{
		lastFATAdress = FATAdress;
		FATAdress = tableFAT[FATAdress].entry;
		printf("\nEndereço de apagamento: %x\n", FATAdress);
		tableFAT[lastFATAdress].entry = freeClusterValue;
	}
	tableFAT[FATAdress].entry = freeClusterValue;
	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
	fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
	fclose(FATDisk);
	if (memcmp(bufferDir._dirEntry, freeCluster, clusterSize) == 0) {
		printf("\nEndereço de liberação da FAT: %x\n", adress);
		tableFAT[adress].entry = freeClusterValue;
	}
	return SUCCESSFUL_VALUE;
}

//int deleteDirOnDisk(char directory[], char dirName[])
//{
//
//	char* FullDIR = (char*)calloc(256, sizeof(char));
//	strcpy(FullDIR, directory);
//	strcat(FullDIR, "/");
//	strcat(FullDIR, dirName);
//
//	uint16_t adress = loadAvaliableDIrEntry(FullDIR);
//	if (adress == FALIED_VALUE)
//		return FALIED_VALUE;
//	FILE* FATDisk = fopen(FATDiskName, "r+b");
//	if (FATDisk == NULL) {
//		fprintf(stderr, "Falha ao abrir o disco");
//		return FALIED_VALUE;
//	}
//	int indexDirAndress;
//
//	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
//	clearBuffer();
//	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
//	for (indexDirAndress = 0; indexDirAndress < DIRMaxCountEntry; indexDirAndress++)
//	{
//		if (memcmp(&(bufferDir._dirEntry[indexDirAndress]), freeDir, sizeof(dirEntry)) != 0 && strcmp(bufferDir._dirEntry[indexDirAndress].filename, "NULL") != 0)
//			return FALIED_VALUE;
//	}
//
//	adress = loadAvaliableDIrEntry(directory);
//	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
//	clearBuffer();
//	fread(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
//	for (indexDirAndress = 0; indexDirAndress < DIRMaxCountEntry; indexDirAndress++)
//	{
//		if (strcmp(bufferDir._dirEntry[indexDirAndress].filename, dirName) == 0)
//			break;
//	}
//
//	/*sprintf(bufferDir._dirEntry[indexDirAndress].filename, "%s", "NULL");
//	bufferDir._dirEntry[indexDirAndress].type = -1;*/
//	uint16_t FATAdress = adress + indexDirAndress;
//	memcpy(&(bufferDir._dirEntry[indexDirAndress]), freeDir, sizeof(dirEntry));
//	if (memcmp(bufferDir._dirEntry, freeCluster, clusterSize) == 0)
//		tableFAT[FATAdress].entry = freeClusterValue;
//	//bufferDir._dirEntry[indexDirAndress].firstBlock = -1;
//	fseek(FATDisk, adress * totalCountCluster, SEEK_SET);
//	fwrite(bufferDir._dirEntry, sizeof(dirEntry), DIRMaxCountEntry, FATDisk);
//	fclose(FATDisk);
//	return SUCCESSFUL_VALUE;
//}

dataCluster* readDataOnDisk(char directory[], char arqName[])
{
	uint16_t adress = loadAvaliableDIrEntry(directory);
	if (adress == FALIED_VALUE)
		return FALIED_VALUE;
	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco");
		return FALIED_VALUE;
	}
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
	dataCluster* buffer = (dataCluster*)calloc(TAMOfBuffer, sizeof(dataCluster));
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
		if (bufferIndex == TAMOfBuffer)
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
	if (adress == FALIED_VALUE)
		return FALIED_VALUE;
	FILE* FATDisk = fopen(FATDiskName, "r+b");
	if (FATDisk == NULL) {
		fprintf(stderr, "Falha ao abrir o disco");
		return FALIED_VALUE;
	}
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
void printFat() {
	for (int i = 0; i < tableFATSize; i++)
	{
		printf(" %x ", tableFAT[i].entry);
		if (i % 6 == 0)
			printf("\n");
	}

}

//int main() {
//	createNewVirtualFATDisk();
//	createNewFile("/root", "file");
//	dataCluster* teste = (dataCluster*)calloc(1, sizeof(dataCluster));
//		char* testeDeArquivo = "testandoEscritaDeArquivo";
//		memcpy(teste[0].data, testeDeArquivo, 24);
//		writeDataOnDisk(teste, 1, "/root", "file");
//		appendDataOnDisk(teste, 1, "/root", "file");
//}

int main() {
	currentDirectory = (char*)calloc(256, sizeof(char));
	sprintf(currentDirectory, "%s", "/root/");
	while (1)
	{
		char* input = readInput();
		char** inputs;
		inputs = splitString(input, ' ');
		int result;
		if (strcmp(inputs[0], "init") == 0)
		{
			result = createNewVirtualFATDisk();
			if (result == SUCCESSFUL_VALUE)
				printf("\nNova partição virtual FAT criada e inicializada com sucesso!\n");
			else
				printf("\nFalha ao criar uma nova partição virtual :c\n");
		}
		else if (strcmp(inputs[0], "load") == 0)
		{
			result = load();
			if (result == FALIED_VALUE)
				printf("\nNão foi possível carregar o disco. Verifique se existe um disco virtual já criado.\n");
		}
		else if (strcmp(inputs[0], "ls") == 0)
		{
			result = showAllDirEntrys(inputs[1]);
			if (result == FALIED_VALUE)
				printf("\nO diretório informado não existe\n");

		}
		else if (strcmp(inputs[0], "mkdir") == 0)
		{
			char** readyDIR;
			char* folderName;
			folderName = (char*)calloc(256, sizeof(char));
			int i = 0;
			if (strcmp(&(inputs[1][strlen(inputs[1]) - 1]), "/") == 0)
			{
				inputs[1][strlen(inputs[1]) - 1] = 0;
			}
			readyDIR = splitString(inputs[1], '/');
			while (readyDIR[i] != 0)
			{
				i++;
			}
			--i;
			sprintf(folderName, "%s", readyDIR[i]);
			if (i > 0) {
				subString(inputs[1], folderName);
			}
			else {
				sprintf(inputs[1], "%s", "/");
			}
			result = createNewDirEntry(inputs[1], folderName);
			if (result == FALIED_VALUE)
				printf("\nNão foi possível criar o diretório. O diretório de criação pode não existir ou você pode estar sem espaço em disco");
			free(readyDIR);
			free(folderName);
		}
		else if (strcmp(inputs[0], "create") == 0)
		{
			char** readyDIR;
			char* arqName;
			arqName = (char*)calloc(256, sizeof(char));
			int index = 0;
			if (strcmp(&(inputs[1][strlen(inputs[1]) - 1]), "/") == 0)
			{
				inputs[1][strlen(inputs[1]) - 1] = 0;
			}
			readyDIR = splitString(inputs[1], '/');
			while (readyDIR[index] != 0)
			{
				index++;
			}
			index--;
			sprintf(arqName, "%s", readyDIR[index]);
			if (index > 0) {
				subString(inputs[1], arqName);
			}
			else {
				sprintf(inputs[1], "%s", "/");
			}
			result = createNewFile(inputs[1], arqName);
			if (result == FALIED_VALUE)
				printf("\nNão foi possível criar o arquivo. O diretório de criação pode não existir ou você pode estar sem espaço em disco");
			free(readyDIR);
			free(arqName);
		}
		else if (strcmp(inputs[0], "unlink") == 0)
		{
			char** readyDIR;
			char* arqName;
			arqName = (char*)calloc(256, sizeof(char));
			int index = 0;
			if (strcmp(&(inputs[1][strlen(inputs[1]) - 1]), "/") == 0)
			{
				inputs[1][strlen(inputs[1]) - 1] = 0;
			}
			readyDIR = splitString(inputs[1], '/');
			while (readyDIR[index] != 0)
			{
				index++;
			}
			index--;
			sprintf(arqName, "%s", readyDIR[index]);
			if (index > 0) {
				subString(inputs[1], arqName);
			}
			else {
				sprintf(inputs[1], "%s", "/");
			}
			result = deleteEntryOnDisk(inputs[1], arqName);
			if (result == FALIED_VALUE)
				printf("\nNão foi possível excluir o arquivo. O diretório de criação pode não existir.");
			free(readyDIR);
			free(arqName);

		}
		else if (strcmp(inputs[0], "write") == 0)
		{
			strcat(inputs[1], "\0");
			char** readyDIR;
			char* arqName;
			int index = 0;
			arqName = (char*)calloc(256, sizeof(char));

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
			if (strcmp(&(inputs[2][strlen(inputs[2]) - 1]), "/") == 0)
			{
				inputs[2][strlen(inputs[2]) - 1] = 0;
			}
			readyDIR = splitString(inputs[2], '/');
			while (readyDIR[index] != '\0')
			{
				index++;
			}
			index--;
			printf("\nindex: %d\n", index);
			sprintf(arqName, "%s", readyDIR[index - 1]);
			if (index > 0) {
				subString(inputs[2], arqName);
			}
			else {
				sprintf(inputs[2], "%s", "/");
			}

			result = writeDataOnDisk(buffer, indexBuffer, inputs[2], arqName);
			if (result == FALIED_VALUE)
				printf("\nNão foi possível completar a operação. O arquivo não existe ou não existe espaço em disco disponível\n");


			free(readyDIR);
			free(arqName);
		}
		else if (strcmp(inputs[0], "append") == 0)
		{
			strcat(inputs[1], "\0");
			char** readyDIR;
			char* arqName;
			int index = 0;
			arqName = (char*)calloc(256, sizeof(char));

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

			if (strcmp(&(inputs[2][strlen(inputs[2]) - 1]), "/") == 0)
			{
				inputs[2][strlen(inputs[2]) - 1] = 0;
			}
			readyDIR = splitString(inputs[2], '/');
			while (readyDIR[index] != 0)
			{
				index++;
			}
			index--;
			sprintf(arqName, "%s", readyDIR[index - 1]);
			if (index > 0) {
				subString(inputs[2], arqName);
			}
			else {
				sprintf(inputs[2], "%s", "/");
			}

			result = appendDataOnDisk(buffer, indexBuffer, inputs[2], arqName);
			if (result == FALIED_VALUE)
				printf("\nNão foi possível completar a operação. O arquivo não existe ou não existe espaço em disco disponível\n");


			free(readyDIR);
			free(arqName);
		}
		else if (strcmp(inputs[0], "read") == 0) {
			char** readyDIR;
			char* arqName;
			dataCluster* buffer;
			arqName = (char*)calloc(256, sizeof(char));
			int index = 0;
			if (strcmp(&(inputs[1][strlen(inputs[1]) - 1]), "/") == 0)
			{
				inputs[1][strlen(inputs[1]) - 1] = 0;
			}
			readyDIR = splitString(inputs[1], '/');
			while (readyDIR[index] != 0)
			{
				index++;
			}
			index--;
			sprintf(arqName, "%s", readyDIR[index]);
			if (index > 0) {
				subString(inputs[1], arqName);
			}
			else {
				sprintf(inputs[1], "%s", "/");
			}
			buffer = readDataOnDisk(inputs[1], arqName);
			if (buffer == FALIED_VALUE)
				printf("\nNão foi possível excluir o arquivo. O diretório de criação pode não existir.");
			else {
				int index = 0;
				while (buffer[index].data != 0)
				{
					printf("%s", buffer[index].data);
				}
			}
			free(readyDIR);
			free(arqName);
		}
		else {
			printf("\nComando não encontrado\n");
		}
		free(input);
		free(inputs);
	}
	return 0;
}
