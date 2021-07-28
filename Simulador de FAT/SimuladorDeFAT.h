#include <stdio.h>
#include <string.h>

#ifdef __unix__         
	/*#include <unistd.h>
	#include <stdlib.h>
	#include "nfd.h"*/
	#include<stdint.h>
#elif defined(_WIN32) || defined(WIN32) 
	#define OS_Windows
	#include <windows.h>
	#include <cstdint>
#endif

#define clusterSize 1024 //bytes
#define tableFATSize 4096 //entradas
#define bootBlockSize 1024 //bytes
#define rootDIRCountEntry 32 //entradas
#define dataClustersCountEntre 4086 //entradas



char* FATDiskName = "fat.part";
const int FALIED_VALUE = -1;
const int SUCCESSFUL_VALUE = 1;

/*Endereços de memória fixos------------------------------------------------------------------*/
const uint16_t StartBootBlock = 0x0000; //endereço de memória em que começa o boot block
const uint16_t bootBlock = 0x0400; //endereço de memória em que termina o boot block
const uint16_t StartFAT = 0x0400; //endereço de memória em que começa a tabela FAT
const uint16_t FAT = 0x2400; //endereço de memória em que termina a tabela FAT
const uint16_t StartRootDIR = 0x2400; //endereço de memória que começa os cluster de dados
/*--------------------------------------------------------------------------------------------*/

/*Constantes utilizadas para comparação na busca pelos diretórios-----------------------------*/
const uint16_t freeCluster = 0x0000; //indica que o cluster esta vazio e/ou pode ser reescrito
// nextCluster - > vai de 0x0001 ~ 0xfffc
const uint16_t bootBlock_FAT = 0xfffd; //valor que marca a divisão entre o fim/início de bootBlock/FAT
const uint16_t FATValue = 0xfffe; //valor para preencher a FAT na criação do disco
const uint16_t endCluster = 0xffff; //valor na tabela FAT que indica que foi o fim do arquivo
const uint8_t isFile = 0x0;
const uint8_t isFolder = 0x1;
/*--------------------------------------------------------------------------------------------*/

typedef struct {
	char filename[18]; //nome do arquivo ou sub-diretório
	uint8_t type;//byte usado para indicar se é arquivo ou sub-diretório
	uint8_t reserved[7]; //7 bytes reserva
	uint16_t firstBlock; //contém o endereço de memória que leva para o primeiro bloco de memória que armazena determinado arquivo
	int size; //Tamanho total do arquivo
}dirEntry;

typedef struct {
	uint16_t entry;
}FATEntry;

FATEntry tableFAT[4096]; //8192 bytes reservados na memória principal para carregar a tabela FAT

typedef union {
	dirEntry _dirEntry[clusterSize / sizeof(dirEntry)];
	uint8_t data[clusterSize];
}dataCluster;

dirEntry buffer[32]; //1024 bytes reservados na memória principal para servir como buffer de I/O caso seja necessário

int createNewVirtualFATDisk();
