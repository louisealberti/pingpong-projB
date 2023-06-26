#include "ppos.h"
#include "ppos-core-globals.h"


// ****************************************************************************
// Coloque aqui as suas modificações, p.ex. includes, defines variáveis, 
// estruturas e funções

#include <signal.h>
#include <sys/time.h>

// estrutura que define um tratador de sinal 
struct sigaction action ;

// estrutura de inicialização to timer
struct itimerval timer ;

// define a prioridade estática de uma tarefa (ou a tarefa atual)
// taskExec -> Ponteiro para a TCB da tarefa em execucao (ppos-core-globals.h)
void task_setprio (task_t *task, int prio) {
    // Verifica se a prioridade da tarefa é valida
    if(prio > 20 || prio < (-20)){
        printf("\ntask_setprio - Error: Prioridade inválida");
        return;
    }

    // Pelo enunciado  caso a tarefa seja nula, ajusta a prioridade da tarefa atual.
    if(task == NULL){
        taskExec->prio_static = prio;
        taskExec->prio_dynamic = prio;
    }
    else{
        task->prio_static = prio;
        task->prio_dynamic = prio;
    }
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
// taskExec -> Ponteiro para a TCB da tarefa em execucao (ppos-core-globals.h)
int task_getprio (task_t *task) {
    // Pelo enunciado se a tarefa for nula, retorna a prioridade estatica da tarefa atual
    if(task == NULL){
        return taskExec->prio_static;
    }
    else{
        return task->prio_static;
    }
}

// retorna a proxima tarefa a ser executada conforme a politica de escalonamento
// readyQueue -> fila de prontas
// nextTask -> task escolhida pelo escalonador
task_t * scheduler() {
    // Verifica se a fila de prontas está vazia
    if(readyQueue == NULL) return NULL;
    // Verifica se a fila tem só um elemento
    if(readyQueue->next == readyQueue) return readyQueue;

    // Implementa escalonamente por prioridade com envelhecimento
    task_t *nextTask = readyQueue;
    task_t *aux = readyQueue;

    // Percorre a fila de prontas procurando a tarefa com maior prioridade dinâmica
    do{
        // Aplica o envelhecimento
        if(aux->prio_dynamic > -20){
            aux->prio_dynamic--;
        }
        // Se a tarefa atual tem igual ou mais prioridade, escolhe ela para ser a nextTask
        if(aux->prio_dynamic <= nextTask->prio_dynamic){
            nextTask = aux;
        }
        // Envelhece a próxima tarefa e verifica se não tem prioridade maior
        aux = aux->next;
    }while(aux != readyQueue);

    // Reinicia a prioridade da tarefa escolhida para o valor da prioridade estática
    nextTask->prio_dynamic = nextTask->prio_static;
    return nextTask;
}


// Simula o ISR (Interrupt Service Routine)
void tratador (int signum){
    systemTime++;  // Incrementa o valor da variável global a cada tick do relógio
    // Se a tarefa não for de usuário retorna
    if (taskExec->task_type != 1) return;
    // Se tarefa for de usuário e quantum for maior que 0 decrementa
    if (taskExec->quantum > 0){
        taskExec->quantum--;
    }else{
        // Se tarefa tem quantum igual a 0 faz ela voltar pro final da fila de prontas
        // passando o controle para o dispatcher
        task_yield();
    }
}

// ****************************************************************************

void before_ppos_init () {
    // put your customization here
    // Inicia temporizador

#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif
}

void after_ppos_init () {
    // put your customization here

    // Registra a ação para o sinal de timer SIGALRM
    // SIGALRM -> Timer signal from alarm
    // Define o handler da ação como a função de tratamento 'tratador'
    action.sa_handler = tratador ;
    // Define um conjunto de masks vazias para dizer que nenhum dos sinais está ignorada
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    // Se o kernel receber o SIGALRM ativa a ação (ponteiro pra função de tratamento)
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        // Caso contrário exibe erro
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    // Ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0;      // primeiro disparo, em segundos
    // 1ms == 1000us
    timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0;   // disparos subsequentes, em segundos

    // Arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }

    // Zera a variavel de tempo global do sistema
    systemTime = 0;

    // Define dispatcher como tarefa de sistema
    taskDisp->task_type = 0;

#ifdef DEBUG
    printf("\ninit - AFTER");
#endif
}

void before_task_create (task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif
}

void after_task_create (task_t *task ) {
    // put your customization here
    // Inicializa as prioridades
    task->prio_static = 0;
    task->prio_dynamic = 0;
    // Indica tipo de tarefa (default: tarefa de usuário - 1)
    task->task_type = 1;
    // Inicia o valor do quantum
    task->quantum = 20;
    // Inicializa variaveis de tempo e de ativações
    task->task_init = 0;
    task->activations = 0;
#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif
}

void before_task_exit () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_exit () {
    // put your customization here

    // Contabiliza o último tempo utilizado de processador
    freeTask->processor_time += systime() - freeTask->task_init;
    
    // Imprime a contabilização das tasks
    printf("Task %d exit: execution time %d ms, processor time  %d ms, %d activations\n", freeTask->id, systime(), freeTask->processor_time, freeTask->activations);
    
    // Se só resta a main como tarefa de usuário, imprime a contabilização do dispatcher
    if(countTasks == 1){
        printf("Task %d exit: execution time %d ms, processor time  %d ms, %d activations\n", taskDisp->id, systime(), taskDisp->processor_time, taskDisp->activations);
    }
#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif
}

void before_task_switch ( task_t *task ) {
    // put your customization here

    // Contabiliza o tempo de processador utilizado pela tarefa anterior
    taskExec->processor_time += systime() - taskExec->task_init;

    // Registra tempo de início da tarefa que será executada
    task->task_init = systime();
    // Reinicia o quantum da tarefa que será executada
    task->quantum = 20;
    // Incrementa o número de ativações
    task->activations++;

#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
}

void after_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
}

void before_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif
}
void after_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}

void before_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif
}

void after_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif
}

void before_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif
}

void after_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif
}

void before_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif
}

int before_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}


int before_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

