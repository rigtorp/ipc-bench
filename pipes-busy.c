/* Measure latency of IPC using busy reading pipes */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>

int main(void)
{
  int pfds[2];
  
  struct timespec start, stop;

  int64_t delta;

  int64_t max = 0;
  int64_t min = INT64_MAX;
  int64_t sum = 0;
  int64_t count = 0;

  if (pipe(pfds) == -1) {
    perror("pipe");
    exit(1);
  }
      
  if (!fork()) {
    sleep(1);

    while (1) {
      clock_gettime(CLOCK_MONOTONIC, &start);

      if (write(pfds[1], &start, sizeof(struct timespec)) == -1) {
        perror("write");
        exit(1);
      }

      usleep(10000);
    }
  } else {
    if (fcntl(pfds[0], F_SETFL, O_NONBLOCK) == -1) {
      perror("fcntl");
      exit(1);
    }

    while (1) {
      if (read(pfds[0], &start, sizeof(struct timespec)) == -1) {
        //perror("read");
        //exit(1);
        continue;
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
  
  return 0;
}
