#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

#define STACKSIZE 32768
#define EXEC 2
#define READY 1
#define SUSP 0
#define ENDED -1

task_t mainTask;
task_t dispTask;

task_t *currentTask;
task_t *readyQueue;

struct sigaction action;
struct itimerval timer;

int taskCounter = 0;
int newTaskId = 0;
int menorPrio = 21;
int tempo_criacao = 0;

unsigned int tempo = 0;

unsigned int systime(){
  return tempo;
}

int task_id(){
  return currentTask->id;
}

void timer_handler(){

  if(currentTask->system_task == 0)
    currentTask->quantum--;

  if(currentTask->quantum == 0){
    task_yield();
  }

  tempo++;

}

void setup_timer(){

  action.sa_handler = timer_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  if (sigaction(SIGALRM, &action, 0) < 0){
    perror("Erro ao instalar o tratador de sinal: ");
    exit(-1);
  }

  // ajusta valores do temporizador
  timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
  timer.it_value.tv_sec  = 0 ;      // primeiro disparo, em segundos
  timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
  timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

  // arma o temporizador ITIMER_REAL
  if (setitimer (ITIMER_REAL, &timer, 0) < 0)
  {
    perror ("Erro em setitimer: ") ;
    exit (1) ;
  }

}

int task_getprio(task_t *task){
  
  if(! task)
    return currentTask->prioridade;

  return task->prioridade;

}

void task_setprio(task_t *task, int prio){

  if (prio > -20 && prio < 20)
    task->prioridade = prio;

}

void task_exit(int exit_code){

  currentTask->status = ENDED;

  

  #ifdef DEBUG
  printf("\nIniciando Task Exit!\n");
  printf("CurrentTask: %d \n", currentTask->id);
  printf("CurrentTask->status: %d \n", currentTask->status);
  #endif

  if (currentTask->id == dispTask.id && exit_code == 0){
    #ifdef DEBUG
    printf("Dispatcher encerrado! \n");
    #endif

    currentTask->tempo_criacao = systime() - currentTask->tempo_criacao;

    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", currentTask->id, currentTask->tempo_criacao, currentTask->tempo_processador, currentTask->ativacoes);

    exit(0);
  }
  else{
    #ifdef DEBUG
    printf("Trocando para Dispatcher!\nFim do Task Exit!\n");
    #endif
    currentTask->tempo_criacao = systime() - currentTask->tempo_criacao;

    // Task 17 exit: execution time 4955 ms, processor time 925 ms, 171 activations
    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", currentTask->id, currentTask->tempo_criacao, currentTask->tempo_processador, currentTask->ativacoes);

    task_switch(&dispTask);
  }
}

int task_switch(task_t *task){

  task_t *auxTask = currentTask;
  currentTask = task;

  swapcontext(&auxTask->context, &task->context);

  return 0;

}

task_t *scheduler(){

  task_t *auxTask = readyQueue;
  task_t *nextTask;

  do{

    #ifdef DEBUG
    printf("Tarefa: %d, Prioridade: %d, Envelhecimento: %d \n", auxTask->id, auxTask->prioridade, auxTask->envelhecimento); 
    #endif

    if(auxTask->prioridade < menorPrio){
      nextTask = auxTask;
      menorPrio = auxTask->prioridade;
    }
      
    auxTask = auxTask->next;
  
  } while(auxTask != readyQueue);

  menorPrio = 21;

  do{

    if(auxTask != nextTask){
      auxTask->envelhecimento++;
      auxTask->prioridade--;
    }

    #ifdef DEBUG
    printf("Tarefa: %d, Prioridade: %d, Envelhecimento: %d \n", auxTask->id, auxTask->prioridade, auxTask->envelhecimento); 
    #endif
      
    auxTask = auxTask->next;
  
  } while(auxTask != readyQueue);

  nextTask->prioridade = nextTask->prioridade + nextTask->envelhecimento;
  nextTask->envelhecimento = 0;

  return nextTask;

}

void task_yield(){

  currentTask->status = READY;

  dispTask.ativacoes++;

  task_switch(&dispTask);

}

void dispatcherBody(){

  setup_timer();

  #ifdef DEBUG
  printf("\nIniciando Dispatcher!\n");
  #endif

  if(queue_remove((queue_t **)&readyQueue, (queue_t *)&dispTask) == -1){
    perror("Erro ao remover dispatcher da fila de prontos: ");
    exit(-1);
  }

  taskCounter--;

  #ifdef DEBUG
  printf("TaskCounter: %d \n", taskCounter);
  #endif

  if (taskCounter == 0)
    task_exit(0);

  task_t *nextTask;

  while (taskCounter > 0){

    nextTask = scheduler();

    nextTask->quantum = 20;
    
    #ifdef DEBUG
    printf("Tarefa atual: %d \n", nextTask->id);
    #endif

    queue_remove((queue_t **)&readyQueue, (queue_t *)nextTask);

    taskCounter--;

    #ifdef DEBUG
    printf("TaskCounter: %d \n", taskCounter);
    #endif

    nextTask->ativacoes++;
    int tempo_processador = systime();
    task_switch(nextTask);
    nextTask->tempo_processador += systime() - tempo_processador;

    #ifdef DEBUG
    printf("\nTarefa: %d \n", nextTask->id);
    printf("\nStatus Atual: %d \n", nextTask->status);
    #endif

    switch(nextTask->status){
      case READY:
        queue_append((queue_t **)&readyQueue, (queue_t *)nextTask);
        taskCounter++;
        break;
      case SUSP:
        break;
      case ENDED:
        free(nextTask->context.uc_stack.ss_sp); 
        break;
    }
  }

  #ifdef DEBUG
  printf("Dispatcher encerrado! \n");
  #endif

  task_exit(0);

}

int task_init(task_t *task, void (*start_func)(void *), void *arg){

  char *stack;

  getcontext(&task->context);

  stack = malloc(STACKSIZE);

  if (stack){
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;
  } else {
    perror("Erro na criação da pilha: ");
    return -1;
  }

  makecontext(&task->context, (void (*)(void))start_func, 1, arg);

  task->id = ++newTaskId;
  task->status = READY;
  task->next = NULL;
  task->prev = NULL;

  task->prioridade = 0;
  task->envelhecimento = 0;

  task->tempo_criacao = 0;
  task->tempo_processador = 0;
  task->ativacoes = 0;

  if(queue_append((queue_t **)&readyQueue, (queue_t *)task) == -1){
    perror("Erro ao adicionar task na fila de prontos: ");
    return -1;
  }

  taskCounter++;

  if(task->id != dispTask.id)
    task->system_task = 0;
  else
    task->system_task = 1;
  
  task->quantum = 20;

  task->tempo_criacao = systime();

  return task->id;

}

void ppos_init(){

  setvbuf(stdout, 0, _IONBF, 0);

  mainTask.id = newTaskId;

  mainTask.next = NULL;
  mainTask.prev = NULL;
  mainTask.tempo_criacao = 0;
  mainTask.tempo_processador = 0;

  mainTask.status = EXEC;

  if (task_init(&dispTask, dispatcherBody, NULL) != 1){
    perror("Erro ao inicializar dispatcher: ");
    exit(-1);
  }

  currentTask = &mainTask;
  mainTask.ativacoes = 1;

  #ifdef DEBUG
  printf("CurrentTask: %d \n", currentTask->id);
  printf("Terminei ppos_init! \n");
  #endif

  setup_timer();
  
}