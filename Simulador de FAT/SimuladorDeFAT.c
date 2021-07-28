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
		tableFAT[i].entry = freeCluster; // 0x0000 preenchendo o restante da tabela FAT com valor vazio (disponível para uso)
	}

	fwrite(tableFAT, sizeof(FATEntry), tableFATSize, FATDisk); //Persistindo a tabela FAT no disco

	dirEntry startRootDIR[rootDIRCountEntry];

	fwrite(startRootDIR, sizeof(dirEntry), rootDIRCountEntry, FATDisk);

	fclose(FATDisk);
	return SUCCESSFUL_VALUE;
}

int main()
{
	
	createNewVirtualFATDisk();
	return 0;

}
