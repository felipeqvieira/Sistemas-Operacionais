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

task_t *taskCurrent;
task_t *mainTask;

queue_t *readyQueue;

task_t *dispatcherTask;

int newTaskId = 1;
int userTasks = 0;

void task_exit(int exit_code){
  
  if(exit_code == 0){
    taskCurrent->status = ENDED;
    task_switch(mainTask);
  } else
  {
    taskCurrent->status = ENDED;
    task_switch(dispatcherTask);
  }
  
}

int task_switch(task_t *task){

  task_t *taskAux = taskCurrent;

  taskCurrent = task;

  printf("task_switch: trocando contexto %d -> %d\n", taskAux->id, taskCurrent->id);

  swapcontext(&(taskAux->context), &(taskCurrent->context));

  return 0;

}

task_t *scheduler(){

  //retorna a primeira tarefa da fila
  return (task_t *) readyQueue;

}

void task_yield(){

  taskCurrent->status = READY;

  queue_append((queue_t **) &readyQueue, (queue_t *) taskCurrent);

  userTasks++;

  task_switch(dispatcherTask);

}

void dispatcherBody(){

  printf("Dispatcher!\n");

  queue_remove((queue_t **) &readyQueue, (queue_t *) dispatcherTask);

  userTasks--;

  task_t *nextTask;

  printf("userTasks: %d\n", userTasks);

  while (userTasks > 0){

    if (queue_size(readyQueue) > 0){

      nextTask = scheduler();

      task_switch(nextTask);

      switch (taskCurrent->status){
        case READY:
          queue_remove((queue_t **) &readyQueue, (queue_t *) taskCurrent);
          userTasks--;
          break;
        case SUSP:
          break;
        case ENDED:
          userTasks--;
          break;
      }

    }

  }

  task_exit(0);

}


int task_init(task_t *task, void (*start_func)(void *), void *arg){

  if (task == dispatcherTask)
    printf("task_init: dispatcherTask já inicializada\n");
  else 
    task = (task_t *) malloc(sizeof(task_t));

  if(!task){
    printf("Erro ao alocar memória para a nova tarefa\n");
    return -1;
  }

  task->id = newTaskId++;

  task->next = task;
  task->prev = task;

  task->status = READY;

  getcontext(&(task->context));

  char *stack;

  stack = malloc(STACKSIZE);

  if (stack){
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;
  } else {
    printf("Erro ao alocar memória para a pilha da nova tarefa\n");
    return -1;
  }

  makecontext(&(task->context), (void *) start_func, 1, arg);

  printf("task_init: criou tarefa %d\n", task->id);

  taskCurrent = task;

  return task->id;

}



void ppos_init(){

  setvbuf(stdout, 0, _IONBF, 0);

  mainTask = (task_t *) malloc(sizeof(task_t));

  if (! mainTask){
    printf("Erro ao alocar memória para a mainTask\n");
    exit(1);
  }

  mainTask->id = 0;

  mainTask->next = NULL;
  mainTask->prev = NULL;

  mainTask->status = EXEC;

  taskCurrent = mainTask; 

  getcontext(&(mainTask->context));

  dispatcherTask = (task_t *) malloc(sizeof(task_t));

  if (! dispatcherTask){
    printf("Erro ao alocar memória para a dispatcherTask\n");
    exit(1);
  }

  task_init(dispatcherTask, dispatcherBody, NULL);

  task_yield();

}