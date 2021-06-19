#include <stdio.h>
#include <stdlib.h> 
#include <sys/shm.h> 
#include <sys/stat.h>
#include <sys/wait.h>  
#include <unistd.h> 
#include <semaphore.h>
#include <time.h>
#include <math.h>
#include "sem.h"
#include "struct.h"

#define TIMES_ENABLED 2
#define RATE 0.0000001
 
int main (int argc, char *argv[]) 
{
  if(argc < 4)
  {
    printf("This program needs 3 arguments, you gave %d\n" , argc-1);
    exit(0);
  }
  int total_processes = atoi(argv[1]), max_entries = atoi(argv[2]), rd_percentage = atoi(argv[3]);
  
  /* Dealing with the shared memory now. */
  int segment_id; 
  entry* shared_memory; 
  struct shmid_ds shmbuffer; 
  int segment_size; 
  const int shared_segment_size = sizeof(entry) * max_entries;
 
  /* Allocate a shared memory segment.  */ 
  segment_id = shmget (IPC_PRIVATE, shared_segment_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); 
  /* Attach the shared memory segment.  */ 
  shared_memory = (entry*) shmat (segment_id, 0, 0);
  printf("-----------------------------------------------------------------\n"); 
  printf ("Shared memory attached at address %p\n", shared_memory); 
  /* Determine the segment's size. */ 
  shmctl (segment_id, IPC_STAT, &shmbuffer); 
  segment_size  = shmbuffer.shm_segsz; 
  printf ("Segment size: %d bytes\n", segment_size);
  printf("-----------------------------------------------------------------\n");
  int num_of_entries;
  for(num_of_entries = 0; num_of_entries < max_entries + 3; num_of_entries++)
  {
    if(num_of_entries < max_entries)
    {
      entry *tentry = (entry*)malloc(sizeof(entry));
      tentry->id = num_of_entries+1;
      tentry->data = (num_of_entries*2) + 1;
      tentry->wrt = 0;
      tentry->rd = 0;
      shared_memory[num_of_entries] = *tentry;
    }
    else if(num_of_entries >= max_entries && num_of_entries < (max_entries + 2))
    {
      entry *tentry = (entry*)malloc(sizeof(entry));
      tentry->data = 0;
      shared_memory[num_of_entries] = *tentry;
    }
    else if(num_of_entries == max_entries + 2)
    {
      entry *tentry = (entry*)malloc(sizeof(entry));
      tentry->data = 1;
      shared_memory[num_of_entries] = *tentry;
    }

  }
/* Done dealing with the shared memory. */

/* An 1D array that stores the data when a process is a reader */  
  int *array_of_data_read;
  array_of_data_read = (int*) malloc(max_entries * sizeof(int));
  if(array_of_data_read == NULL) 
  {
    printf("ERROR: Memory for the reading array was not allocated!\n");
    exit(0);
  }
/*--------------------------------------------------------------------*/ 

/* An 1D array that stores the average time that a process have waited */
  double *average_time_waited;
  average_time_waited = (double*)malloc(TIMES_ENABLED*sizeof(double));
  if(average_time_waited == NULL) 
  {
    printf("ERROR: Memory for the reading array was not allocated!\n");
    exit(0);
  }
/*--------------------------------------------------------------------*/ 

  srand(time(0) + getppid());
  int i ,j;// rc = 0;
  pid_t pid, wpid;
  Semaphore  *mutex = make_semaphore(1);
  Semaphore  *wrt = make_semaphore(1);

  for(i = 0; i < total_processes; i++) // Number of peers
  {  
    shared_memory[max_entries + 2].data = (rand() % 100) < rd_percentage ? 1 : 0; // This is the way in which a peer becomes a writer or a reader.
    pid = fork();
    if(pid == 0) // Child process
    {
      clock_t begin = clock();
      int ttwrt = 0 , ttrd = 0;
      if(shared_memory[max_entries + 2].data == 0) // Writer.
      {
        for(j = 0; j < TIMES_ENABLED; j++) // The total times the process is enabled.
        {
            semaphore_wait(wrt);
            srand(getppid()*time(NULL)+j);
            unsigned int t = ((rand() * getpid()) / (RAND_MAX / max_entries)); // Random position in the shared memory.
            while(shared_memory[t].wrt == 1) sleep(1);
            // Do the writing.
            shared_memory[t].data = rand()+(t*(j+1)); 
            ttwrt++; // Add 1 to the total times that THIS SPESIFIC PROCESS have writen something.
            shared_memory[max_entries+1].data += ttwrt; // Pass that value to shared memory
            if(shared_memory[t].wrt == 1) shared_memory[t].wrt--;
            /* Expotential sleep() time is calculated with the following method */
            /* An int that stores the number created by a math formula */
            int time = ( -log((float)rand()/(float)(RAND_MAX / 1)) ) / RATE;
            /*---------------------------------------------------------*/

            /* This is how I will make my process sleep for nanoseconds */ 
            timespec *tim = (timespec*)malloc(sizeof(timespec));
            if(tim == NULL) 
            {
                printf("ERROR: Memory for the Timespec was not allocated!\n");
                exit(0);
            }
            tim->tv_sec = 0;
            tim->tv_nsec = (time * 10);
            nanosleep(tim, NULL);
            /*---------------------------------------------------------*/
            clock_t end = clock();
            double time_child = (1)*((double)(end - begin))/CLOCKS_PER_SEC;
            average_time_waited[j] = time_child; // The time that this porcess have waited.
            semaphore_signal(wrt);
         }
      }
      else // Reader
      {
            for(j = 0; j < TIMES_ENABLED; j++) // The total times a process is enabled.
            {
              semaphore_wait(mutex);
              srand(getppid()*time(NULL)+j);
              unsigned int t = ((rand() * getpid()) / (RAND_MAX / max_entries)); // Random position in the shared memory.
              shared_memory[t].rd++;
              if(shared_memory[t].rd == 1) semaphore_wait(wrt);
              semaphore_signal(mutex);
              // Do the reading
              array_of_data_read[t] = shared_memory[t].data;
              ttrd++;
              shared_memory[max_entries].data += ttrd;
              semaphore_wait(mutex);
              shared_memory[t].rd--;
              if(shared_memory[t].rd == 0) semaphore_signal(wrt);
              /* Expotential sleep() time is calculated with the following method */
              /* An int that stores the number created by a math formula */
              int time = ( -log((float)rand()/(float)(RAND_MAX / 1)) ) / RATE;
              /*---------------------------------------------------------*/

              /* This is how I will make my process sleep for nanoseconds */ 
              timespec *tim = (timespec*)malloc(sizeof(timespec));
              if(tim == NULL) 
              {
                  printf("ERROR: Memory for the Timespec was not allocated!\n");
                 exit(0);
              }
              tim->tv_sec = 0;
              tim->tv_nsec = (time * 10);
              nanosleep(tim, NULL);
              /*---------------------------------------------------------*/
              clock_t end = clock();
              double time_child = (1)*((double)(end - begin))/CLOCKS_PER_SEC;
              average_time_waited[j] = time_child;
              semaphore_signal(mutex);
            }
        }
      /* Printing the stats of each process */
      int temp;
      double ttw = 0;
      for(temp = 0; temp < TIMES_ENABLED; temp++)
      {
        ttw += average_time_waited[temp];
      }
      printf("\nProcess: %d --> Read %d times and writen %d times and the average wait time for process was: %lf seconds\n" ,getpid() ,ttrd , ttwrt , ttw/TIMES_ENABLED);
      exit(0);
      /*---------------------------------------------------------------------------------------------------------------------*/
    }
  } 

  while((wpid = wait(NULL)) > 0); // Parent waits untill all child processes are exited.

  printf("\n-----------------------------------------------------------------\n");
  printf("The Shared memory after %d reads and %d writes is the following.\n\n" , shared_memory[max_entries].data, shared_memory[max_entries + 1].data);
  for(i=0; i<max_entries; i++)
  {
    printf("Entry ID = %d ---- Entry DATA = %d\n" , shared_memory[i].id , shared_memory[i].data);
  }
  shmdt (shared_memory);
  shmctl (segment_id, IPC_RMID, 0);
  free(array_of_data_read);
  free(average_time_waited);
  return 0; 
} 