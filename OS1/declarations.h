#include <stdio.h>
#include <stdlib.h> 
#include <sys/shm.h> 
#include <sys/stat.h>
#include<sys/wait.h> 
#include <unistd.h> 
#include <semaphore.h>
#include <pthread.h>

#define SLEEP_TIME 62500

typedef struct Entry
{
  int num;
  int time;
}entry;

typedef struct Entry_ch
{
  int num;
  int time;
}ch_entry;
