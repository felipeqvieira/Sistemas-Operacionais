// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Teste do task dispatcher e escalonador FCFS

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t Pang, Peng, Ping, Pong, Pung ;

// corpo das threads
void Body (void * arg)
{

   int i ;

   printf ("%s: inicio\n", (char *) arg) ;
   for (i=0; i<5; i++)
   {
      printf ("%s: %d\n", (char *) arg, i) ;
      task_yield ();
   }
   printf ("%s: fim\n", (char *) arg) ;
   task_exit (0) ;
}

int main (int argc, char *argv[])
{
   printf ("main: inicio\n");

   ppos_init () ;
   
   if (task_init (&Pang, Body, "    Pang") != 2)
   {
      printf ("Erro Pang\n");
      return 1;
   }

   if (task_init (&Peng, Body, "        Peng") != 3)
   {
      printf ("Erro Peng\n");
      return 1;
   }

   if (task_init (&Ping, Body, "            Ping") != 4)
   {
      printf ("Erro Ping\n");
      return 1;
   }

   if (task_init (&Pong, Body, "                Pong") != 5)
   {
      printf ("Erro Pong\n");
      return 1;
   }

   if(task_init (&Pung, Body, "                    Pung") != 6)
   {
      printf ("Erro Pung\n");
      return 1;
   }
   
   printf ("main: fim\n");

   task_exit (0);
}