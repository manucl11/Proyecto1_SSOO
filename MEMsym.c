
/*	Título: Proyecto Sistemas Operativos Memoria
	Autor: Manuel Cabrera Liñán	  */

/* Instrucciones de preprocesado */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_FILAS 8
#define TAM_LINEA 16
#define TAM_TEXTO 100
#define TAM_RAM 4096

/* Declaración de estructuras */

typedef struct
{
	unsigned char ETQ;
	unsigned char Data[TAM_LINEA];
	
} T_CACHE_LINE;

/* Declaración de variables globales */

int numCaracteres = 0;
char texto[TAM_TEXTO];

/* Prototipado de funciones */

void LimpiarCACHE(T_CACHE_LINE tbl[NUM_FILAS]);
void VolcarCACHE(T_CACHE_LINE *tbl);
void ParsearDireccion(unsigned int addr, int *ETQ, int *palabra, int *linea, int *bloque);
void TratarFallo(T_CACHE_LINE *tbl, char *MRAM, int ETQ, int linea, int bloque);

/* Función principal */

int main(int argc, char** argv)
{
	/* Declaración de variables globales */
	
	int globaltime;
	int numfallos;
	int accesos;
	
	unsigned int* adrs;
	unsigned int label, line, word, block;

	unsigned char Simul_RAM[4096];
	
	/* Declaración de variables de fichero */
	
	FILE* fRam = NULL;
	FILE* fRamBin = NULL;
	FILE* fBin = NULL;
	
	T_CACHE_LINE tbl[NUM_FILAS];
	
	/* Comprobación de que se introducen los argumentos necesarios */
	
	if(argv[1] == NULL || argv[2] == NULL)
	{
		printf("Error: faltan parámetros para ejecutar el programa");
		return -1;
	}
	
	/* Apertura de ficheros */
	
	fRam = fopen(argv[1], "r");
	fRamBin = fopen(argv[2], "rb");
	fBin = fopen("CONTENTS_CACHE.bin", "wb");
	
	/* Inicialización de variables locales requeridas inicializadas a cero */
	
	globaltime = 0;
	numfallos = 0;
	accesos = 0;
	
	/* Carga de los elementos del archivo binario en la variable Simul_RAM */
	
	fread(Simul_RAM,sizeof(Simul_RAM),1,fRamBin);
	LimpiarCACHE(tbl);
	
	/* Primer fscanf fuera del while que va a leer el fichero para poder hacer el primer parseo de datos */
	
	fscanf(fRam, "%x", adrs);
	ParsearDireccion(*adrs, &label, &word, &line, &block);
	
	while(!feof(fRam))
	{
		
		globaltime++;
		
		/* Comprobación de que las etiquetas son iguales para saber si es un acierto o un fallo de caché */
		
		if(tbl[line].ETQ == label)
		{
			accesos++;
			
			printf("T: %d, Acierto de CACHE, ADDR %04X Label %X linea %02X palabra %02X DATO %02X", globaltime, *adrs, label, line, word, Simul_RAM[(block * TAM_LINEA)+word]);
			VolcarCACHE(tbl);
			fscanf(fRam, "%x", adrs);
			ParsearDireccion(*adrs, &label, &word, &line, &block);
		}	
		else
		{
			numfallos++;
			
			printf("T: %d, Fallo de CACHE %d, ADDR %04X Label %X linea %02X palabra %02X bloque %02X", globaltime, numfallos, *adrs, label, line, word, block);
			printf("\nCargando el bloque %02X en la linea %02X", block, line);
			
			globaltime += 9;
			
			TratarFallo(tbl, Simul_RAM, label, line, block);
		}
		
		printf("\n");
		
		/* Sleep que detiene la ejecución del programa durante 1 segundo */
		
		sleep(1);
	}
	
	/* Prints finales de accesos y textos leidos */
	
	printf("Accesos totales: %d; fallos: %d; Tiempo medio: %.2f ", accesos, numfallos, (float)globaltime / accesos);
	printf("\nTexto leido: ");
	
	for(int i = 0; i < TAM_TEXTO; i++)
	{
	    printf("%c", texto[i]);
	}
	
	/* Vuelque de los bytes de la memoria caché al fichero binario requerido */
	
	for(int i = 0; i < NUM_FILAS; i++)
	{
		for(int j = 0; j < TAM_LINEA; j++)
		{
			fputc(tbl[i].Data[j], fBin);
		}
		fputc('\n', fBin);
	}
	
	/* Cierre de ficheros */
	
	fclose(fRam);
	fclose(fRamBin);
	fclose(fBin);
	
	/* Fin de la función principal */
	
	return 0;
}

/* Funciones */

/* Función que inicializa los elementos ETQ a 0xFF y los bytes de la caché a 0x23 */

void LimpiarCACHE(T_CACHE_LINE tbl[NUM_FILAS])
{
	for(int i = 0; i < NUM_FILAS; i++)
	{
		tbl[i].ETQ = 0xFF;
		
		for(int j = 0; j < TAM_LINEA; j++)
		{
			tbl[i].Data[j] = 0x23;
		}
	}
}

/* Función que imprime por pantalla los datos de la caché */

void VolcarCACHE(T_CACHE_LINE *tbl)
{	
	printf("\n");

	for(int i = 0; i < NUM_FILAS; i++)
	{
		printf("ETQ:%02X	", tbl[i].ETQ);
		printf(" Data: ");
		
		for(int j = 0; j < TAM_LINEA; j++)
		{
			printf("%X ", tbl[i].Data[j]);
		}
		
		printf("\n");
	}
}

/* Función que desde la dirección de memoria dada, separa los bits para asignarle a cada campo sus correspondientes datos */

void ParsearDireccion(unsigned int addr, int *ETQ, int *palabra, int *linea, int *bloque)
{
	*ETQ = addr >> 7;
	*palabra = addr & 0b000000001111;
	*linea = (addr & 0b000001110000) >> 4;
	*bloque = addr >> 4;
}

/* Función que se encarga de rellenar la línea correspondiente de datos después de haber habido un fallo de caché */

void TratarFallo(T_CACHE_LINE *tbl, char *MRAM, int ETQ, int linea, int bloque)
{		
	tbl[linea].ETQ = ETQ;

	for(int i = 0; i < TAM_LINEA; i++)
	{
		tbl[linea].Data[i] = MRAM[(bloque * TAM_LINEA) + 15 -i];
		
		if(numCaracteres <= 99)
		{
			texto[numCaracteres] = MRAM[bloque * TAM_LINEA + 15 -i];
			numCaracteres++;
		}
	}
}