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

int newTaskId = 1;

void ppos_init(){

  setvbuf(stdout, 0, _IONBF, 0);

  mainTask = (task_t *) malloc(sizeof(task_t));

  if (! mainTask){
    printf("Erro ao alocar memória para a mainTask\n");
    exit(1);
  }

  mainTask->id = 0;

  mainTask->next = mainTask;
  mainTask->prev = mainTask;

  taskCurrent = mainTask;

  taskCurrent->status = EXEC;

  getcontext(&(taskCurrent->context));

}

int task_init(task_t *task, void (*start_func)(void *), void *arg){

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

  return task->id;

}

void task_exit(int exit_code){

  if (exit_code == 9999){
    printf("Erro ao finalizar tarefa\n");
    exit(1);
  }
  taskCurrent->status = ENDED;

  task_switch(mainTask);
  
}

int task_switch(task_t *task){

  task_t *taskAux;

  taskAux = taskCurrent;

  taskCurrent = task;

  swapcontext(&(taskAux->context), &(taskCurrent->context));

  return 0;

}

int task_id(){
  
  return taskCurrent->id;

}
