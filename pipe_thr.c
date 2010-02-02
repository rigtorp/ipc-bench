/* Measure throughput of IPC using pipes */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <stdint.h>


int main(int argc, char *argv[])
{
  int fds[2];

  int size;
  char *buf;
  int64_t count, i, delta;
  struct timespec start, stop;

  if (argc != 3) {
    printf ("usage: pipe_thr <message-size> <message-count>\n");
    exit(1);
  }

  size = atoi(argv[1]);
  count = atol(argv[2]);

  buf = malloc(size);
  if (buf == NULL) {
    perror("malloc");
    exit(1);
  }

  printf("message size: %i octets\n", size);
  printf("message count: %lli\n", count);

  if (pipe(fds) == -1) {
    perror("pipe");
    exit(1);
  }

  if (!fork()) {  
    /* child */

    for (i = 0; i < count; i++) {      
      if (read(fds[0], buf, size) != size) {
        perror("read");
        exit(1);
      }
    }
  } else { 
    /* parent */
  
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < count; i++) {
      if (write(fds[1], buf, size) != size) {
        perror("write");
        exit(1);
      }
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);

    delta = ((stop.tv_sec - start.tv_sec) * (int64_t) 1e9 +
	     stop.tv_nsec - start.tv_nsec);

    printf("average throughput: %lli msg/s\n", (count * (int64_t) 1e9) / delta);
    printf("average throughput: %lli Mb/s\n", (((count * (int64_t) 1e9) / delta) * size * 8) / (int64_t) 1e6);
  }
  
  return 0;
}
