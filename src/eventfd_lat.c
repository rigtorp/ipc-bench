/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Erik Rigtorp <erik@rigtorp.se> */

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int notify(int fd) {
  uint64_t value = 1;
  ssize_t result;
  do {
    result = write(fd, &value, sizeof(value));
  } while (result == -1 && errno == EINTR);
  if (result != sizeof(value)) {
    perror("eventfd write");
    return -1;
  }
  return 0;
}

static int receive(int fd) {
  uint64_t value;
  ssize_t result;
  do {
    result = read(fd, &value, sizeof(value));
  } while (result == -1 && errno == EINTR);
  if (result != sizeof(value)) {
    perror("eventfd read");
    return -1;
  }
  if (value != 1) {
    fprintf(stderr, "unexpected eventfd counter: %" PRIu64 "\n", value);
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  char *end;
  int64_t count;
  int ping, pong, status;
  int result = EXIT_FAILURE;
  pid_t child, waited;
  struct timespec start, stop;

  if (argc != 2) {
    fprintf(stderr, "usage: eventfd_lat <roundtrip-count>\n");
    return EXIT_FAILURE;
  }
  errno = 0;
  count = strtoimax(argv[1], &end, 10);
  if (errno || end == argv[1] || *end || count <= 0 ||
      count > INT64_MAX / 2) {
    fprintf(stderr, "invalid roundtrip count\n");
    return EXIT_FAILURE;
  }

  ping = eventfd(0, EFD_CLOEXEC);
  if (ping == -1) {
    perror("eventfd");
    return EXIT_FAILURE;
  }
  pong = eventfd(0, EFD_CLOEXEC);
  if (pong == -1) {
    perror("eventfd");
    close(ping);
    return EXIT_FAILURE;
  }
  child = fork();
  if (child == -1) {
    perror("fork");
    close(ping);
    close(pong);
    return EXIT_FAILURE;
  }
  if (child == 0) {
    /* Signal readiness before the parent starts its timer. */
    if (notify(pong) == -1)
      _exit(EXIT_FAILURE);
    for (int64_t i = 0; i < count; ++i) {
      if (receive(ping) == -1 || notify(pong) == -1)
        _exit(EXIT_FAILURE);
    }
    close(ping);
    close(pong);
    _exit(EXIT_SUCCESS);
  }

  if (receive(pong) == -1)
    goto cleanup;
  if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
    perror("clock_gettime");
    goto cleanup;
  }
  for (int64_t i = 0; i < count; ++i) {
    if (notify(ping) == -1 || receive(pong) == -1)
      goto cleanup;
  }
  if (clock_gettime(CLOCK_MONOTONIC, &stop) == -1) {
    perror("clock_gettime");
    goto cleanup;
  }
  result = EXIT_SUCCESS;

cleanup:
  if (result != EXIT_SUCCESS)
    kill(child, SIGTERM);
  do {
    waited = waitpid(child, &status, 0);
  } while (waited == -1 && errno == EINTR);
  close(ping);
  close(pong);
  if (waited == -1) {
    perror("waitpid");
    return EXIT_FAILURE;
  }
  if (result != EXIT_SUCCESS || !WIFEXITED(status) ||
      WEXITSTATUS(status) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  int64_t delta = (int64_t)(stop.tv_sec - start.tv_sec) * 1000000000LL +
                  (stop.tv_nsec - start.tv_nsec);
  printf("roundtrip count: %" PRId64 "\n", count);
  printf("average roundtrip latency: %" PRId64 " ns\n", delta / count);
  /* Match the other latency benchmarks: elapsed time / (2 * count). */
  printf("average latency: %" PRId64 " ns\n", delta / (count * 2));
  return EXIT_SUCCESS;
}
