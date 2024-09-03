// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  
  struct task_t *prev, *next ;		// ponteiros para usar em filas
  int id ;				// identificador da tarefa
  ucontext_t context ;			// contexto armazenado da tarefa
  short status ;			// pronta, rodando, suspensa, ...
  short prioridade;
  short envelhecimento;
  short quantum;
  int system_task;
  unsigned int tempo_criacao;
  unsigned int tempo_processador;
  int ativacoes;
  int sleep;
  int child_id;
  int child_return;

} task_t ;

// estrutura que define um semáforo
typedef struct
{
  int contador;
  task_t *queue;
  
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  
  unsigned int maximum_size;
  unsigned int byte_size;
  unsigned int qtd_msg;
  int start;
  int end;
  short int status;
  void *buffer;

  semaphore_t sem_buffer;
  semaphore_t sem_productor;
  semaphore_t sem_consumer;
  
  // preencher quando necessário
} mqueue_t ;

#endif