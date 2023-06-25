#include <stdlib.h>
#include "ppos_disk.h"
#include "ppos_data.h"
#include "disk.h"
#include "ppos-core-globals.h"
#include "ppos-core-aux.c"


// gerenciadora da inicializacao de disco
int disk_mgr_init (int *numBlocks, int *blockSize){
	
	//task que sera chamada a cada sinal
	// mover para uma funcao propria
	sigDisk.sa_handler = diskSignalAction; 
	sigemptyset (&sigDisk.sa_mask);
	sigDisk.sa_flags = 0 ;

	if (sigaction (SIGUSR1, &sigDisk, 0) < 0)
	{
		fprintf (stderr, "Erro no disk_mgr_init: sigaction retornou erro\n");
		return (-1) ;
	}
	
	// inicializa o disco (sincrona)
	if (disk_cmd(DISK_CMD_INIT, 0, 0) < 0){
		fprintf(stderr, "ERRO: disco inicializado incorretamente\n");
		return -1;
	}

	// seta o tamanho do disco
	*numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
	// consulta tamanho do disco
	if(*numBlocks < 0){
		fprintf(stderr, "ERRO: tamanho do disco incorreto\n");
		return -1;
	}

	// seta o tamanho de cada bloco
	*blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
	// consulta tamanho de cada bloco
	if(*blockSize < 0){
		fprintf(stderr, "ERRO: tamanho do blocos do disco incorreto\n");
		return -1;
	}

	// aloca espaco do disco e do semaforo
	disk = malloc(sizeof(disk_t*));
	disk->semaphore = malloc(sizeof(semaphore_t*));

	// cria o semaforo do disco com valor inicial 1
	sem_create((semaphore_t*)disk->semaphore, 1);

	disk->next = disk->prev = NULL;
	disk->handler = 0; // tratador

	// cria a tarefa gerenciadora do disco e a coloca na fila de prontas
	task_create(&taskDiskManager, diskDriverBody, NULL);
	queue_remove ((queue_t **) &readyQueue, (queue_t *) &taskDiskManager);

	return 0;
} // end of disk_mgr_init

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer){

	// requisita o semaforo
	sem_down((semaphore_t*)disk->semaphore);
	
	//aloca espaco
	disk_t *request = malloc(sizeof(disk_t*));
	// seta valores iniciais
	request->next = request->prev = NULL;
	request->block = block;
	request->buffer = buffer;
	request->option = 1; // opcao de leitura
	request->task = taskExec;

	queue_remove((queue_t**)&readyQueue, (queue_t*) taskExec);
	queue_append((queue_t**)&sleepQueue, (queue_t*) request);

	queue_append((queue_t**)&readyQueue, (queue_t*)&taskDiskManager);

	// libera o semaforo
	sem_up((semaphore_t*)disk->semaphore);

	task_yield();

	free(request);

	return 0;
} // end of disk_block_read

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer){
	
	// requisita o semaforo
	sem_down((semaphore_t*)disk->semaphore);
	
	//aloca espaco
	disk_t *request = malloc(sizeof(disk_t*));
	// seta valores iniciais
	request->next = request->prev = NULL;
	request->block = block;
	request->buffer = buffer;
	request->option = 2; // opcao de escrita
	request->task = taskExec;

	queue_remove((queue_t**)&readyQueue, (queue_t*)taskExec);
	queue_append((queue_t**)&sleepQueue, (queue_t*)request);

	queue_append((queue_t**)&readyQueue, (queue_t*)&taskDiskManager);

	// libera o semaforo
	sem_up((semaphore_t*)disk->semaphore);

	task_yield();

	free(request);

	return 0;
} // end of disk_block_write
