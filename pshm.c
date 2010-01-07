/* Measure latency of IPC using shm and POSIX threads locking primitives */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

struct shared {
  pthread_mutex_t count_mutex;
  pthread_cond_t count_threshold_cv;
  struct timespec ts;
};

int main(void)
{
  int pfds[2];

  pthread_mutexattr_t psharedm;
  pthread_condattr_t psharedc;

  
  char c;
  int shmid;
  key_t key;
  struct shared *shm;

  struct timespec start, stop;

  int64_t delta;

  int64_t max = 0;
  int64_t min = INT64_MAX;
  int64_t sum = 0;
  int64_t count = 0;

  (void) pthread_mutexattr_init(&psharedm);
  (void) pthread_mutexattr_setpshared(&psharedm,
                                      PTHREAD_PROCESS_SHARED);
  (void) pthread_condattr_init(&psharedc);
  (void) pthread_condattr_setpshared(&psharedc,
                                     PTHREAD_PROCESS_SHARED);



  /*
   * We'll name our shared memory segment
   * "5678".
   */
  key = 5678;
  
  if (!fork()) {

    /*
     * Create the segment.
     */
    if ((shmid = shmget(key, 100, IPC_CREAT | 0666)) < 0) {
      perror("shmget");
      exit(1);
    }

    /*
     * Now we attach the segment to our data space.
     */
    if ((shm = shmat(shmid, NULL, 0)) == (struct shared*) -1) {
      perror("shmat");
      exit(1);
    }

    pthread_mutex_init(&shm->count_mutex,  &psharedm);
    pthread_cond_init (&shm->count_threshold_cv, &psharedc);


    while (1) {
      pthread_mutex_lock(&shm->count_mutex);
      clock_gettime(CLOCK_MONOTONIC, &shm->ts);
      pthread_cond_signal(&shm->count_threshold_cv);
      pthread_mutex_unlock(&shm->count_mutex);

      usleep(10000);
    }
  } else {
    sleep(1);

    /*
     * Locate the segment.
     */
    if ((shmid = shmget(key, 100, 0666)) < 0) {
      perror("shmget");
      exit(1);
    }

    /*
     * Now we attach the segment to our data space.
     */
    if ((shm = shmat(shmid, NULL, 0)) == (struct shared *) -1) {
      perror("shmat");
      exit(1);
    }


    while (1) {
      pthread_mutex_lock(&shm->count_mutex);
      
      if ((shm->ts.tv_sec == start.tv_sec) && (shm->ts.tv_nsec == start.tv_nsec)) {
        pthread_cond_wait(&shm->count_threshold_cv, &shm->count_mutex);
      } 
        
      pthread_mutex_unlock(&shm->count_mutex);
      

      clock_gettime(CLOCK_MONOTONIC, &stop);
      start = shm->ts;
      delta = ((stop.tv_sec - start.tv_sec) * (int64_t) 1000000000 +
               stop.tv_nsec - start.tv_nsec);
      
      if (delta > max)
        max = delta;
      else if (delta < min)
        min = delta;
      
      sum += delta;
      count++;
      
      if (!(count % 100)) {
        printf("%lli %lli %lli\n", max, min, sum / count);
      }
    }
  }
  
  return 0;
}
