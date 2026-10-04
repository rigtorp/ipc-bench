/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2016 Erik Rigtorp <erik@rigtorp.se> */

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

int main(int argc, char *argv[]) {
  int size;
  char *buf;
  int64_t count, i;

  ssize_t len;
  ssize_t sofar;

  int yes = 1;
  int ret;
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  struct addrinfo hints;
  struct addrinfo *res;
  int sockfd, new_fd;

  if (argc != 5) {
    printf("usage: tcp_local_lat <bind-to> <port> <message-size> "
           "<roundtrip-count>\n");
    return 1;
  }

  size = atoi(argv[3]);
  count = atol(argv[4]);

  buf = malloc(size);
  if (buf == NULL) {
    perror("malloc");
    return 1;
  }

  printf("message size: %i octets\n", size);
  printf("roundtrip count: %" PRId64 "\n", count);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // fill in my IP for me
  if ((ret = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    return 1;
  }

  if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) ==
      -1) {
    perror("socket");
    return 1;
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
  }

  if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
    perror("bind");
    return 1;
  }

  if (listen(sockfd, 1) == -1) {
    perror("listen");
    return 1;
  }

  addr_size = sizeof their_addr;

  if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) ==
      -1) {
    perror("accept");
    return 1;
  }

  for (i = 0; i < count; i++) {

    for (sofar = 0; sofar < size;) {
      len = read(new_fd, buf, size - sofar);
      if (len == -1) {
        perror("read");
        return 1;
      }
      sofar += len;
    }

    if (write(new_fd, buf, size) != size) {
      perror("write");
      return 1;
    }
  }

  return 0;
}
