/* Measure latency of IPC using unix domain sockets */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <stdint.h>


int main(int argc, char *argv[])
{
  int sv[2]; /* the pair of socket descriptors */
  int size;
  char *buf;
  int64_t count, i, delta;
  struct timespec start, stop;

  if (argc != 3) {
    printf ("usage: unix_lat <message-size> <roundtrip-count>\n");
    return 1;
  }

  size = atoi(argv[1]);
  count = atol(argv[2]);

  buf = malloc(size);
  if (buf == NULL) {
    perror("malloc");
    return 1;
  }

  printf("message size: %i octets\n", size);
  printf("roundtrip count: %lli\n", count);

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
    perror("socketpair");
    exit(1);
  }

  if (!fork()) {  /* child */
    for (i = 0; i < count; i++) {
      
      if (read(sv[1], buf, size) != size) {
        perror("read");
        return 1;
      }
      
      if (write(sv[1], buf, size) != size) {
        perror("write");
        return 1;
      }
    }
  } else { /* parent */
  
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (i = 0; i < count; i++) {

      if (write(sv[0], buf, size) != size) {
        perror("write");
        return 1;
      }

      if (read(sv[0], buf, size) != size) {
        perror("read");
        return 1;
      }
      
    }

    clock_gettime(CLOCK_MONOTONIC, &stop);

    delta = ((stop.tv_sec - start.tv_sec) * (int64_t) 1000000000 +
	     stop.tv_nsec - start.tv_nsec);
    
    printf("average latency: %lli ns\n", delta / (count * 2));

  }
  
  return 0;
}
