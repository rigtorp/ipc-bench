/*
    Measure latency of IPC using udp sockets


    Copyright (c) 2016 Erik Rigtorp <erik@rigtorp.se>

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
*/

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) &&                           \
    defined(_POSIX_MONOTONIC_CLOCK)
#define HAS_CLOCK_GETTIME_MONOTONIC
#endif

int main(int argc, char *argv[]) {
  int size;
  char *buf;
  int64_t count, i, delta;
#ifdef HAS_CLOCK_GETTIME_MONOTONIC
  struct timespec start, stop;
#else
  struct timeval start, stop;
#endif

  ssize_t len;
  size_t sofar;

  int yes = 1;
  int ret;
  struct addrinfo hints;
  struct addrinfo *resChild;
  struct addrinfo *resParent;
  int sockfd;

  if (argc != 3) {
    printf("usage: udp_lat <message-size> <roundtrip-count>\n");
    return 1;
  }

  size = atoi(argv[1]);
  count = atol(argv[2]);

  buf = malloc(size);
  if (buf == NULL) {
    perror("malloc");
    return 1;
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE; // fill in my IP for me
  if ((ret = getaddrinfo("127.0.0.1", "3491", &hints, &resParent)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    return 1;
  }
  if ((ret = getaddrinfo("127.0.0.1", "3492", &hints, &resChild)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    return 1;
  }

  printf("message size: %i octets\n", size);
  printf("roundtrip count: %" PRId64 "\n", count);

  if (!fork()) { /* child */

    if ((sockfd = socket(resChild->ai_family, resChild->ai_socktype,
                         resChild->ai_protocol)) == -1) {
      perror("socket");
      return 1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      return 1;
    }

    if (bind(sockfd, resChild->ai_addr, resChild->ai_addrlen) == -1) {
      perror("bind");
      return 1;
    }

    for (i = 0; i < count; i++) {

      for (sofar = 0; sofar < size;) {
        len = recvfrom(sockfd, buf, size - sofar, 0, resParent->ai_addr,
                       &resParent->ai_addrlen);
        if (len == -1) {
          perror("recvfrom");
          return 1;
        }
        sofar += len;
      }

      if (sendto(sockfd, buf, size, 0, resParent->ai_addr,
                 resParent->ai_addrlen) != size) {
        perror("sendto");
        return 1;
      }
    }
  } else { /* parent */

    sleep(1);

    if ((sockfd = socket(resParent->ai_family, resParent->ai_socktype,
                         resParent->ai_protocol)) == -1) {
      perror("socket");
      return 1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      return 1;
    }

    if (bind(sockfd, resParent->ai_addr, resParent->ai_addrlen) == -1) {
      perror("bind");
      return 1;
    }

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

      if (sendto(sockfd, buf, size, 0, resChild->ai_addr,
                 resChild->ai_addrlen) != size) {
        perror("sendto");
        return 1;
      }

      for (sofar = 0; sofar < size;) {
        len = recvfrom(sockfd, buf, size - sofar, 0, resChild->ai_addr,
                       &resChild->ai_addrlen);
        if (len == -1) {
          perror("read");
          return 1;
        }
        sofar += len;
      }
    }

#ifdef HAS_CLOCK_GETTIME_MONOTONIC
    if (clock_gettime(CLOCK_MONOTONIC, &stop) == -1) {
      perror("clock_gettime");
      return 1;
    }

    delta = ((stop.tv_sec - start.tv_sec) * 1000000000 +
             (stop.tv_nsec - start.tv_nsec));

#else
    if (gettimeofday(&stop, NULL) == -1) {
      perror("gettimeofday");
      return 1;
    }

    delta = (stop.tv_sec - start.tv_sec) * 1000000000 +
            (stop.tv_usec - start.tv_usec) * 1000;

#endif

    printf("average latency: %" PRId64 " ns\n", delta / (count * 2));
  }

  return 0;
}
