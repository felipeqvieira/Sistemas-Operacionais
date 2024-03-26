#include <stdio.h>
#include <stdlib.h>

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

  //printf("\nIniciando Task Exit!\n");
  //printf("CurrentTask: %d \n", currentTask->id);
  //printf("CurrentTask->status: %d \n", currentTask->status);

  if (currentTask->id == dispTask.id){
    //printf("Dispatcher encerrado! \n");
    exit(0);
  }
  else{
    //printf("Trocando para Dispatcher!\nFim do Task Exit!\n");
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

    //printa tarefa id, prioridade e envelhecimento em um printf
    //printf("Tarefa: %d, Prioridade: %d, Envelhecimento: %d \n", auxTask->id, auxTask->prioridade, auxTask->envelhecimento); 

    if(auxTask->prioridade < menorPrio){
      nextTask = auxTask;
      menorPrio = auxTask->prioridade;
    }
      
    auxTask = auxTask->next;
  
  } while(auxTask != readyQueue);

  menorPrio = 21;

  printf("\n");

  do{

    if(auxTask != nextTask){
      auxTask->envelhecimento++;
      auxTask->prioridade--;
    }

    //printf("Tarefa: %d, Prioridade: %d, Envelhecimento: %d \n", auxTask->id, auxTask->prioridade, auxTask->envelhecimento); 
      
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

  //printf("\nIniciando Dispatcher!\n");

  if(queue_remove((queue_t **)&readyQueue, (queue_t *)&dispTask) == -1){
    perror("Erro ao remover dispatcher da fila de prontos: ");
    exit(-1);
  }

  taskCounter--;

  //printf("TaskCounter: %d \n", taskCounter);

  if (taskCounter == 0)
    task_exit(0);

  task_t *nextTask;

  while (taskCounter > 0){
    
    nextTask = scheduler();
    
    //printf("Tarefa atual: %d \n", nextTask->id);

    queue_remove((queue_t **)&readyQueue, (queue_t *)nextTask);

    taskCounter--;

    //printf("TaskCounter: %d \n", taskCounter);

    task_switch(nextTask);

    //printf("\nTarefa: %d \n", nextTask->id);
    //printf("\nStatus Atual: %d \n", nextTask->status);

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

  //printf("Dispatcher encerrado! \n");

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

  //printf("CurrentTask: %d \n", currentTask->id);
  //printf("Terminei ppos_init! \n");
  
}