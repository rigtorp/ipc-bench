/* Measure latency of thread IPC using pipes */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

void *receiver(void *fd_) {
  struct timespec start, stop;

  int fd = (int)fd_;

  int64_t delta;

  int64_t max = 0;
  int64_t min = INT64_MAX;
  int64_t sum = 0;
  int64_t count = 0;

    while (1) {
      if (read(fd, &start, sizeof(struct timespec)) == -1) {
        perror("read");
        exit(1);
      }
      
      clock_gettime(CLOCK_MONOTONIC, &stop);
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


int main(void)
{
  int pfds[2];  
  pthread_t thread;
  struct timespec start;

  if (pipe(pfds) == -1) {
    perror("pipe");
    exit(1);
  }
  
  pthread_create(&thread, NULL, receiver, (void *)pfds[0]);

  while (1) {
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    if (write(pfds[1], &start, sizeof(struct timespec)) == -1) {
      perror("write");
      exit(1);
    }
    
    usleep(10000);
  }
  
  return 0;
}
