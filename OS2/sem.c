#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h> 
#include "sem.h"

typedef sem_t Semaphore;
void perror_exit (char *s)
{
  perror (s);  exit (-1);
}

Semaphore *make_semaphore(int value)
{
  Semaphore *sem = check_malloc(sizeof(Semaphore));
  int n = sem_init(sem, 0, value);
  if (n != 0) perror_exit("sem_init failed");
  return sem;
}

void semaphore_wait(Semaphore *sem)
{
  int n = sem_wait(sem);
  if (n != 0) perror_exit("sem_wait failed");
}

void semaphore_signal(Semaphore *sem)
{
  int n = sem_post(sem);
  if (n != 0) perror_exit("sem_post failed");
}
