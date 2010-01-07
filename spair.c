/* Measure latency of IPC using unix domain sockets */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <stdint.h>

int main(void)
{
  int sv[2]; /* the pair of socket descriptors */

  struct timespec start, stop;

  int64_t delta;

  int64_t max = 0;
  int64_t min = INT64_MAX;
  int64_t sum = 0;
  int64_t count = 0;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
    perror("socketpair");
    exit(1);
  }

  if (!fork()) {  /* child */
    sleep(1);

    while (1) {
      clock_gettime(CLOCK_MONOTONIC, &start);

      if (write(sv[1], &start, sizeof(struct timespec)) == -1) {
        perror("write");
        exit(1);
      }
      
      usleep(10000);
    }
  } else { /* parent */
    while (1) {
      if (read(sv[0], &start, sizeof(struct timespec)) == -1) {
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
  
  return 0;
}
