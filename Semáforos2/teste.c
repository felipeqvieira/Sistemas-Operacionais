// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Teste de semáforos (light)

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t      a1, a2, b1, b2;
semaphore_t s1, s2 ;

// corpo da thread A
void TaskA (void * arg)
{
   int i ;
   for (i=0; i<10; i++)
   {
      sem_down (&s1) ;
      printf ("%s zig (%d)\n", (char *) arg, i) ;
      task_sleep (1000) ;
      sem_up (&s2) ;
   }
   task_exit (0) ;
}

// corpo da thread B
void TaskB (void * arg)
{
   int i ;
   for (i=0; i<10; i++)
   {
      sem_down (&s2) ;
      printf ("%s zag (%d)\n", (char *) arg, i) ;
      task_sleep (1000) ;
      sem_up (&s1) ;
   }
   task_exit (0) ;
}

int main (int argc, char *argv[])
{
   printf ("main: inicio\n") ;

   ppos_init () ;

   // inicia semaforos
   sem_init (&s1, 1) ;
   sem_init (&s2, 0) ;

   // inicia tarefas
   if(task_init (&a1, TaskA, "A1") < 0){
     printf("Erro ao inicializar a tarefa A1\n");
     return -1;
   }

   if(task_init (&a2, TaskA, "\tA2") < 0){
     printf("Erro ao inicializar a tarefa A2\n");
     return -1;
   }

   if(task_init (&b1, TaskB, "\t\t\tB1") < 0){
     printf("Erro ao inicializar a tarefa B1\n");
     return -1;
   }

   if(task_init (&b2, TaskB, "\t\t\t\tB2") < 0){
     printf("Erro ao inicializar a tarefa B2\n");
     return -1;
   }

   // aguarda a1 encerrar
   if (task_wait (&a1) == -1){
     printf("Erro ao aguardar a tarefa A1\n");
     return -1;
   }

   // destroi semaforos
   sem_destroy (&s1) ;
   sem_destroy (&s2) ;

   printf("Semaforos destruídos!\n");

   // aguarda a2, b1 e b2 encerrarem
   if(task_wait (&a2) == -1){
     printf("Erro ao aguardar a tarefa A2\n");
     return -1;
   }

   if(task_wait (&b1) == -1){
     printf("Erro ao aguardar a tarefa B1\n");
     return -1;
   }
   
   if(task_wait (&b2) == -1){
     printf("Erro ao aguardar a tarefa B2\n");
     return -1;
   }

   printf ("main: fim\n") ;
   task_exit (0) ;
}