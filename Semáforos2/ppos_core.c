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
task_t *suspendedQueue;
task_t *sleepQueue;
task_t *semaphoreQueue;

struct sigaction action;
struct itimerval timer;

int newTaskId = 0;
int menorPrio = 21;
int tempo_criacao = 0;
int taskCounter = 0;

int lock = 0;

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

  if(queue_size((queue_t *)suspendedQueue) > 0){
    task_t *auxTask = suspendedQueue;
      
    do{

      if(currentTask->id == auxTask->child_id){
        task_t *next = auxTask->next;
        task_awake(auxTask, &suspendedQueue);
        auxTask->child_id = -1;
        auxTask->child_return = exit_code;
        auxTask = next;
      } else
        auxTask = auxTask->next;

      if(! suspendedQueue)
      break;

    } while(auxTask != suspendedQueue);

  }

  taskCounter--;

  if (currentTask->id == dispTask.id && exit_code == 0){

    currentTask->tempo_criacao = systime() - currentTask->tempo_criacao;

    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", currentTask->id, currentTask->tempo_criacao, currentTask->tempo_processador, currentTask->ativacoes);

    exit(0);
  }
  else{
    
    currentTask->tempo_criacao = systime() - currentTask->tempo_criacao;

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

  if(auxTask == NULL)
    return NULL;

  task_t *nextTask;

  do{

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

  
  

  if(queue_remove((queue_t **)&readyQueue, (queue_t *)&dispTask) == -1){
    perror("Erro ao remover dispatcher da fila de prontos: ");
    exit(-1);
  }

  printf("Tarefa %d removida da fila de prontas\n", dispTask.id);
  printf("Queue_size: %d\n", queue_size((queue_t *)readyQueue));

  taskCounter--;

  if (queue_size((queue_t *)readyQueue) == 0)
    task_exit(0);

  task_t *nextTask;

  while (taskCounter > 0){

    if(queue_size((queue_t *)sleepQueue) > 0){
      task_t *auxTask = sleepQueue;
    
      do{

        if(tempo >= auxTask->sleep){
          task_t *next = auxTask->next;
          task_awake(auxTask, &sleepQueue);
          auxTask = next;
        } else
          auxTask = auxTask->next;

        if(! sleepQueue)
          break;

      } while(auxTask != sleepQueue);

    }
    
    nextTask = scheduler();

    if(nextTask == NULL)
      continue;
    
    nextTask->quantum = 20;

    if(queue_remove((queue_t **)&readyQueue, (queue_t *)nextTask) == -1){
      perror("Erro ao remover task da fila de prontos: ");
      exit(-1);
    }

    printf("Tarefa %d removida da fila de prontas\n", nextTask->id);
    printf("Queue_size: %d\n", queue_size((queue_t *)readyQueue));

    nextTask->ativacoes++;
    int tempo_processador = systime();
    task_switch(nextTask);
    nextTask->tempo_processador += systime() - tempo_processador;

    switch(nextTask->status){
      case READY:
        queue_append((queue_t **)&readyQueue, (queue_t *)nextTask);
        printf("Tarefa %d adicionada na fila de prontas\n", nextTask->id);
        printf("Queue_size: %d\n", queue_size((queue_t *)readyQueue));
        break;
      case SUSP:
        break;
      case ENDED:
        free(nextTask->context.uc_stack.ss_sp); 
        break;
    }

  }

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
  task->child_id = -1;
  task->child_return = 0;

  task->prioridade = 0;
  task->envelhecimento = 0;

  task->tempo_criacao = 0;
  task->tempo_processador = 0;
  task->ativacoes = 0;

  if(queue_append((queue_t **)&readyQueue, (queue_t *)task) == -1){
    perror("Erro ao adicionar task na fila de prontos: ");
    return -1;
  }

  printf("Tarefa %d adicioanda na fila de prontas\n", task->id);
  printf("Queue_size: %d\n", queue_size((queue_t *)readyQueue));

  if(task->id != dispTask.id)
    task->system_task = 0;
  else
    task->system_task = 1;
  
  task->quantum = 20;

  task->tempo_criacao = systime();

  taskCounter++;

  return task->id;

}

void ppos_init(){

  setvbuf(stdout, 0, _IONBF, 0);

  mainTask.id = newTaskId;
  mainTask.child_id = -1;

  mainTask.next = NULL;
  mainTask.prev = NULL;
  mainTask.tempo_criacao = 0;
  mainTask.tempo_processador = 0;
  mainTask.ativacoes = 0;
  mainTask.prioridade = 0;
  mainTask.envelhecimento = 0;
  mainTask.system_task = 0;
  mainTask.quantum = 20;
  mainTask.child_return = 0;

  mainTask.status = READY;

  currentTask = &mainTask;

  queue_append((queue_t **)&readyQueue, (queue_t *)&mainTask);

  printf("Tarefa %d adicionada na fila de prontas\n", mainTask.id);
  printf("Queue_size: %d\n", queue_size((queue_t *)readyQueue));

  taskCounter++;

  if (task_init(&dispTask, dispatcherBody, NULL) != 1){
    perror("Erro ao inicializar dispatcher: ");
    exit(-1);
  }

  setup_timer();
  
}


void task_suspend(task_t **queue){

  queue_remove((queue_t **)&readyQueue, (queue_t *)currentTask);

  printf("Tarefa %d removida da fila de prontas\n", currentTask->id);
  printf("Queue_size: %d\n", queue_size((queue_t *)readyQueue));
  
  queue_remove((queue_t **)&suspendedQueue, (queue_t *)currentTask);

  printf("Tarefa %d removida da fila de suspensas\n", currentTask->id);
  printf("Queue_size: %d\n", queue_size((queue_t *)suspendedQueue));

  currentTask->status = SUSP;

  if(queue){
    
    if(queue_append((queue_t **)queue, (queue_t *)currentTask) == -1){
      perror("Erro ao adicionar task na fila de suspensas: ");
      exit(-1);
    } 

    printf("Tarefa %d adicionada na fila\n", currentTask->id);
    printf("Queue_size: %d\n", queue_size((queue_t *)*queue));
  }

  task_switch(&dispTask);
}

void task_awake(task_t *task, task_t **queue){
  
    if(queue_remove((queue_t **)queue, (queue_t *)task) == -1){
      printf("\nErro: Queue_remove na task_awake! task->Id: %d\n\n", task->id);
      exit(-1);
    }

    printf("Tarefa %d removida da fila\n", task->id);
    printf("Queue_size: %d\n", queue_size((queue_t *)queue));
  
    task->status = READY;
  
    queue_append((queue_t **)&readyQueue, (queue_t *)task);

    printf("Tarefa %d adicionada da fila de prontas\n", task->id);
    printf("Queue_size: %d\n", queue_size((queue_t *)readyQueue));

}


int task_wait(task_t *task){

  if(!task || task->status == ENDED)
    return -1;

  currentTask->child_id = task->id;

  task_suspend(&suspendedQueue);

  int retorno = currentTask->child_return;

  currentTask->child_return = 0;

  return retorno;

}

void task_sleep(int t){

  currentTask->sleep = tempo + t;

  task_suspend(&sleepQueue);

}

void enter_cs(int *lock){
  
  //atomic OR
  //busy waiting
  while(__sync_fetch_and_or(lock, 1));
   
}

void leave_cs(int *lock){
  
  //atomic AND
  (*lock) = 0;

}

int sem_init(semaphore_t *s, int value){

  s->contador = value;
  s->queue = NULL;

  return 0;

}

int sem_down(semaphore_t *s){

  if(! s){
    perror("Erro: sem_up com semáforo nulo!\n");
    exit(-1);
  }

  enter_cs(&lock);
  s->contador--;
  leave_cs(&lock);

  if(s->contador < 0)
    task_suspend(&s->queue);

  return 0;
}

int sem_up(semaphore_t *s){

  if(! s){
    perror("Erro: sem_up com semáforo nulo!\n");
    exit(-1);
  }

  enter_cs(&lock);
  s->contador++;
  leave_cs(&lock);

  if(s->contador <= 0)
    task_awake(s->queue, &s->queue);

  return 0;
}

int sem_destroy(semaphore_t *s){

  if(! s){
    perror("Erro: sem_destroy com semáforo nulo!\n");
    exit(-1);
  }

  while(queue_size((queue_t *)s->queue) > 0){
    task_awake(s->queue, &s->queue);
  }

  return 0;
}

