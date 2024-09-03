#include <stdio.h>
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

int main() {
  // Declara uma variável do tipo semaphore_t na pilha
  semaphore_t s;

  // Inicializa o semáforo
  sem_init(&s, 1);

  // Declara uma variável do tipo task_t na pilha
  task_t task;

  // Inicializa a tarefa (você pode precisar de mais inicialização aqui)
  task.id = 1;
  task.prev = NULL;
  task.next = NULL;

  // Insere a tarefa na fila do semáforo
  if (queue_append((queue_t **) &s.queue, (queue_t *) &task) == -1) {
    printf("Erro ao inserir na fila\n");
    return -1;
  }

  printf("queue size: %d\n", queue_size((queue_t *) s.queue));

  return 0;
}
