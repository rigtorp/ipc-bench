/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2016 Erik Rigtorp <erik@rigtorp.se> */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) &&                           \
    defined(_POSIX_MONOTONIC_CLOCK)
#define HAS_CLOCK_GETTIME_MONOTONIC
#endif

int main(int argc, char *argv[]) {
  int sv[2]; /* the pair of socket descriptors */
  int size;
  char *buf;
  int64_t count, i, delta;
#ifdef HAS_CLOCK_GETTIME_MONOTONIC
  struct timespec start, stop;
#else
  struct timeval start, stop;
#endif

  if (argc != 3) {
    printf("usage: unix_lat <message-size> <roundtrip-count>\n");
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
  printf("roundtrip count: %" PRId64 "\n", count);

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
    perror("socketpair");
    return 1;
  }

  if (!fork()) { /* child */
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

#ifdef HAS_CLOCK_GETTIME_MONOTONIC
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
      perror("clock_gettime");
      return 1;
    }
#else
    if (gettimeofday(&start, NULL) == -1) {
      perror("gettimeofday");
      return 1;
    }
#endif

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

#ifdef HAS_CLOCK_GETTIME_MONOTONIC
    if (clock_gettime(CLOCK_MONOTONIC, &stop) == -1) {
      perror("clock_gettime");
      return 1;
    }

    delta = ((stop.tv_sec - start.tv_sec) * 1000000000LL +
             (stop.tv_nsec - start.tv_nsec));

#else
    if (gettimeofday(&stop, NULL) == -1) {
      perror("gettimeofday");
      return 1;
    }

    delta = (stop.tv_sec - start.tv_sec) * 1000000000LL +
            (stop.tv_usec - start.tv_usec) * 1000LL;

#endif

    printf("average latency: %" PRId64 " ns\n", delta / (count * 2));
  }

  return 0;
}
