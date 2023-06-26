#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <sys/time.h>

#include "ppos.h"
#include "disk.h"
#include "ppos_disk.h"
#include "ppos-core-globals.h"

// ****************************************************************************
// VARIAVEIS

// Tarefa gerenciadora do disco
task_t taskDiskManager;

// O disco virtual
disk_t disk;

// estrutura que define um tratador de sinal
static struct sigaction actionDisk;

// estrutura de inicialização to timer
struct itimerval timer;

// ****************************************************************************
// STRUCT REQUISICOES


// requisicoes do disco
typedef struct request_t {
  
	int block;
	void *buffer;
	int option; // opcao de leitura ou escrita

	task_t *task; // faz a chamada do disco
	struct request_t *prev, *next; // anterior e proxima requisicao

} request_t;

// ****************************************************************************
// FUNCOES AUXILIARES

// Funcao tratadora do sinal para o disco
static void diskSignalHandler(int signum){
	// gerenciador do disco vai para a fila de tarefas prontas
    if (taskDiskManager.state == 'S') task_resume((task_t *)&taskDiskManager);
    disk.signal = 1;
} // end of diskSignalHandler

// Retorna requisicao
// Funcao para criar uma nova requisicao que sera colocada no gerenciador de disco
request_t* requestCreator(int block, void *buffer, int option){

	// aloca espaco
    request_t *request = (request_t *)malloc(sizeof(request_t));

    if (!request) exit(-1); // verifica se houve erro na alocacao

	// seta os valores iniciais da requisicao
    request->block = block;
    request->buffer = buffer;
	request->option = option;
	request->task = taskExec;
	request->prev = request->next = NULL;


    return request;
	
} // end of requestCreator

// Tarefa gerente de disco responsavel por tratar os pedidos de leitura/escrita das tarefas e os sinais gerados pelo disco (Tarefa de sistema)
// Implementacao sugerida pelo Mazieiro
static void diskDriverBody(void * args){
    
	request_t *request;

	// enquanto for true
    while (1){
    
        sem_down(&(disk.semaphore)); // requisita o semáforo
        
		// quando o disco eh acordado por um sinal
        if (disk.signal == 1){

            queue_remove((queue_t **)&disk.waitQueue, (queue_t *)request->task); // tira a tarefa que sera atendida da lista de espera

            request->task->state = 'R'; // seta o estado da tarefa para pronta (ready)
            queue_append((queue_t **)&readyQueue, (queue_t *)request->task); // anexa a tarefa  na fila de prontas
            
			// remove da fila de requisicoes
            queue_remove((queue_t **)&(disk.reqQueue), (queue_t *)request);
            

			// volta o valor para zero permitindo saber quando o disco receber outro sinal
            disk.signal = 0; 

			free(request);
        }

        // Quando o disco esta livre e com pedidos de E/S na fila
        if (disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE && &disk.reqQueue){
            
            request = disk.reqQueue; // recebe o pedido a ser atendido
            disk_cmd(request->option, request->block, request->buffer); // solicita a operacao

        }

		// Caso nao haja tarefas na fila de requisicoes e de espera, o disco fica suspenso
        if (disk.reqQueue != NULL && disk.waitQueue != NULL){
            queue_remove((queue_t **)&readyQueue, (queue_t *)&taskDiskManager);
            taskDiskManager.state = 'S'; 
        }
        else {
            taskDiskManager.state = 'R'; // caso contrario o disco fica pronto para receber requisicoes
		}

        sem_up(&(disk.semaphore)); // libera o semáforo



        // suspende a tarefa corrente (retorna ao dispatcher)
        task_yield();
    }
} // end of diskDriverBody

// ****************************************************************************
// FUNCOES DO ppos_disk.h

// inicializa do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init(int *numBlocks, int *blockSize)
{
	// Registra a ação para o sinal de timer SIGALRM
    // SIGALRM -> Timer signal from alarm
    // Define o handler da ação como a função de tratamento 'tratador'
	actionDisk.sa_handler = diskSignalHandler;
	// Define um conjunto de masks vazias para dizer que nenhum dos sinais está ignorada
    sigemptyset(&actionDisk.sa_mask);
    actionDisk.sa_flags = 0;
	// Se o kernel receber o SIGALRM ativa a ação (ponteiro pra função de tratamento)
    if (sigaction(SIGUSR1, &actionDisk, 0) < 0)
	{
		// Caso contrário exibe erro
        perror("Erro em sigaction: ");
        exit(1);
    }

    // inicializa o disco
    if (disk_cmd(DISK_CMD_INIT, 0, 0) < 0) return -1; // em caso de erro 

	// seta o tamanho do bloco
    disk.sizeBlocks = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);

    // seta o tamanho do disco
    disk.qtdBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    
    if (disk.qtdBlocks < 0 || disk.sizeBlocks < 0) return -1; // em caso de erro

    *numBlocks = disk.qtdBlocks;
    *blockSize = disk.sizeBlocks;

	disk.signal = 0;
    disk.reqQueue = disk.waitQueue = NULL;

    sem_create((&disk.semaphore), 1); // cria um semáforo com valor inicial 1
    sem_down(&disk.semaphore); // requisita o semáforo

    // criacao da tarefa para gerenciar o disco
    task_create(&taskDiskManager, diskDriverBody, NULL);
    task_resume(&taskDiskManager);
    
	taskDiskManager.state = 'S'; // coloca como estado suspensa

    sem_up(&disk.semaphore); // libera o semáforo

    return 0;

} // end of disk_mgr_init

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer){
    
	if (block < 0 || !buffer) return -1; // em caso de erro

    request_t *request = requestCreator(block, buffer, DISK_CMD_READ);
    
	sem_down(&disk.semaphore); // requisita o semáforo
    
    queue_append((queue_t **)&disk.reqQueue, (queue_t *)request); // a requisicao vai para a fila de pedidos do disco


	// gerenciador do disco vai para a fila de tarefas prontas
    if (taskDiskManager.state == 'S') task_resume((task_t *)&taskDiskManager);
    
    sem_up(&disk.semaphore);  // libera semáforo

    
    task_yield();

    return 0;

} // end of disk_block_read

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer){
    
	if (block < 0 || !buffer) return -1; // em caso de erro

    request_t *request = requestCreator(block, buffer, DISK_CMD_WRITE);

    sem_down(&disk.semaphore); // requisita o semáforo
    
    queue_append((queue_t **)&disk.reqQueue, (queue_t *)request); // a requisicao vai para a fila de pedidos do disco
    
	// gerenciador do disco vai para a fila de tarefas prontas
	if (taskDiskManager.state == 'S') task_resume((task_t *)&taskDiskManager);
    

    sem_up(&disk.semaphore);  // libera semáforo

    task_yield(); // volta para o dispatcher

    return 0;
} // end of disk_block_write
