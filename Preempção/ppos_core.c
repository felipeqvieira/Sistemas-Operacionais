#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
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

int taskCounter = 0;
int newTaskId = 0;

int menorPrio = 21;

int timer_handler(){

  if(currentTask->system_task == 0){
    currentTask->quantum--;
    if(currentTask->quantum == 0){
      currentTask->quantum = 20;
      task_yield();
      return 1;
    }
  }

  return 0;

}

void setup_timer(){

  struct sigaction action;
  struct itimerval timer;

  action.sa_handler = timer_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  if (sigaction(SIGALRM, &action, 0) < 0){
    perror("Erro ao instalar o tratador de sinal: ");
    exit(-1);
  }

  // ajusta valores do temporizador
  timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
  timer.it_value.tv_sec  = 1 ;      // primeiro disparo, em segundos
  timer.it_interval.tv_usec = 0 ;   // disparos subsequentes, em micro-segundos
  timer.it_interval.tv_sec  = 0.001 ;   // disparos subsequentes, em segundos

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
    exit(0);
  }
  else{
    #ifdef DEBUG
    printf("Trocando para Dispatcher!\nFim do Task Exit!\n");
    #endif
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
    
    #ifdef DEBUG
    printf("Tarefa atual: %d \n", nextTask->id);
    #endif

    queue_remove((queue_t **)&readyQueue, (queue_t *)nextTask);

    taskCounter--;

    #ifdef DEBUG
    printf("TaskCounter: %d \n", taskCounter);
    #endif

    task_switch(nextTask);

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

  return task->id;

}

void ppos_init(){

  setvbuf(stdout, 0, _IONBF, 0);

  mainTask.id = newTaskId;

  mainTask.next = NULL;
  mainTask.prev = NULL;

  mainTask.status = EXEC;

  if (task_init(&dispTask, dispatcherBody, NULL) != 1){
    perror("Erro ao inicializar dispatcher: ");
    exit(-1);
  }

  currentTask = &mainTask;

  #ifdef DEBUG
  printf("CurrentTask: %d \n", currentTask->id);
  printf("Terminei ppos_init! \n");
  #endif

  setup_timer();
  
}