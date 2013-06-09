/* Measure latency of IPC using shm */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(void)
{
  int pfds[2];
  
  char c;
  int shmid;
  key_t key;
  struct timespec *shm;



  struct timespec start, stop;

  int64_t delta;

  int64_t max = 0;
  int64_t min = INT64_MAX;
  int64_t sum = 0;
  int64_t count = 0;

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
    if ((shm = shmat(shmid, NULL, 0)) == (struct timespec*) -1) {
      perror("shmat");
      exit(1);
    }

    while (1) {
      gettimeofday(shm, NULL);

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
    if ((shm = shmat(shmid, NULL, 0)) == (struct timespec *) -1) {
      perror("shmat");
      exit(1);
    }

    while (1) {
      while ((shm->tv_sec == start.tv_sec) && (shm->tv_nsec == start.tv_nsec)) {}

      gettimeofday(&stop, NULL);
      start = *shm;
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
