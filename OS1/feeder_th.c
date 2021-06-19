#include "declarations.h"

sem_t mutex,writeblock;             //Global variables so that the threads 
int count = 0, rcount = 0;          // 
entry* shared_memory;               // 
int* array;                         //can synchronize better
int times_writen = 0; 

/* The thread type that reads from shared memory one int at the time */
/***********************************************************************************************************************************/
void *reader(void *arg) 
{
  int array_size;
  array_size = *((int*)arg);
  ch_entry* ch_array = (ch_entry*)malloc(sizeof(ch_entry)*array_size);
  for(int y = 0; y < array_size; y++) 
  {
    ch_array[y].time = -1;
    ch_array[y].num = -1;
  }

  while(1)
  {
    if(ch_array[count].num != -1 && count <= array_size-1) continue;
    
    sem_wait(&mutex);
    rcount = rcount + 1;
    if(rcount==1) sem_wait(&writeblock);
    sem_post(&mutex);
    /* Does the reading */
    /*------------------------------------------------------------------------------------------------------------*/
    ch_array[count].num = shared_memory[0].num;
    ch_array[count].time = shared_memory[0].time + SLEEP_TIME;
    printf("Array[%d] is now read by thread:%d\n", count, (int)pthread_self());
    usleep(SLEEP_TIME);
    /*------------------------------------------------------------------------------------------------------------*/
    sem_wait(&mutex);
    rcount = rcount - 1;
    if(rcount==0) sem_post(&writeblock);
    sem_post(&mutex);
    if(count >= array_size-1) break; //Break after the int array is filled
  }
  /* This part takes care of the printing to a file */
  /*------------------------------------------------------------------------------------------------------------*/
  char* my_output = (char*)malloc(sizeof(char)*7);
  sprintf(my_output, "pr%d", (int)pthread_self());
  char* file_to_create = (char*)malloc(sizeof(char)*32);
  sprintf(file_to_create, "Output_dir/%s", my_output);
  FILE * fp;
  fp = fopen(file_to_create, "a+");
  free(my_output); free(file_to_create);
  fprintf(fp, "\n--Thread ID = %d--\n", (int)pthread_self());
  double av_delay_time = 0;
  for(int y = 0; y < array_size; y++) av_delay_time =+ ch_array[y].time;
  av_delay_time = av_delay_time/array_size;
  fprintf(fp, "\nAverage wait time == %f microseconds\n", av_delay_time);
  for(int y = 0; y < array_size; y++) fprintf(fp, "%c (%d) %c --%d\n", '|', y+1, '|',ch_array[y].num);
  /*------------------------------------------------------------------------------------------------------------*/
}
/***********************************************************************************************************************************/

/* The tread type that updates the shared memory one int at the time */
/***********************************************************************************************************************************/
void *writer(void *arg) 
{
  int array_size;
  array_size = *((int*) arg);
  sem_wait(&writeblock);

  if(count == array_size - 1) pthread_exit(0);
  count++;
  shared_memory[0].num = array[count];
  shared_memory[0].time = times_writen*SLEEP_TIME;
  printf("\nArray[%d] is now on shared memory\n\n", times_writen);

  sem_post(&writeblock);
}
/***********************************************************************************************************************************/

int main (int argc, char *argv[])
{
  printf("\n\nUsing THREADS\n\n");
  if(argc != 3)
  {
    printf("The correct form to run the program is: ./feeder number_of_processes array_size\n");
    exit(1);
  }
  int total_processes = atoi(argv[1]);
  int array_size = atoi(argv[2]);
  mkdir("Output_dir", 0777); //Directory that contains the output files

  /* Dealing with the shared memory now. */
  
  int segment_id; 
  struct shmid_ds shmbuffer; 
  int segment_size; 
  const int shared_segment_size = sizeof(entry);

  /* Allocate a shared memory segment.  */ 
  segment_id = shmget(IPC_PRIVATE, shared_segment_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); 
  /* Attach the shared memory segment.  */ 
  shared_memory = (entry*)shmat(segment_id, NULL, 0);
  printf("-----------------------------------------------------------------\n"); 
  printf ("Shared memory attached at address %p\n", shared_memory); 
  /* Determine the segment's size. */ 
  shmctl (segment_id, IPC_STAT, &shmbuffer); 
  segment_size  = shmbuffer.shm_segsz; 
  printf ("Segment size: %d bytes\n", segment_size);
  printf("-----------------------------------------------------------------\n");

  /* Done dealing with the shared memory. */

  time_t rawtime;
  struct tm* timeinfo;
  time (&rawtime);
  timeinfo = localtime(&rawtime);
  printf ("Current local time and date: %s", asctime(timeinfo));
  /* The full int array */
  array = (int*)malloc(sizeof(int)*array_size);
  for(int i = 0; i < array_size; i++) array[i] = rand();
  for(int i = 0; i < array_size; i++) printf("%c (%d) %c --%d\n", '|', i+1, '|', array[i]);
  shared_memory[0].num = array[0];
  shared_memory[0].time = 0;
  
  int i,b; 
  pthread_t rtid[total_processes],wtid[array_size];
  sem_init(&mutex,0,1);
  sem_init(&writeblock,0,1);
  int* array_sizeptr = &array_size;
  /* I create n reader threads */
  for(i = 0; i < total_processes; i++) pthread_create(rtid + i, NULL , reader ,(void *)array_sizeptr);
  /* I create n writer threads */
  for(i = 0; i < array_size; i++)
  {
    usleep(SLEEP_TIME);
    times_writen++;
    pthread_create(wtid + i, NULL , writer ,(void *)array_sizeptr);
  }
  /* I wait all threads to terminate */
  for(i=0;i<total_processes;i++) pthread_join(rtid[i],NULL);
  for(i=0;i<array_size;i++) pthread_join(wtid[i],NULL);
  return 0;
}
