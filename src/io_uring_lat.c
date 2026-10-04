/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Erik Rigtorp <erik@rigtorp.se> */

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <inttypes.h>
#include <liburing.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SEND_COMPLETION UINT64_MAX

static int ring_error(const char *operation, int result) {
  fprintf(stderr, "%s: %s\n", operation, strerror(-result));
  return -1;
}

static int send_message(struct io_uring *ring, int peer_fd, uint64_t sequence) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
  if (sqe == NULL) {
    fprintf(stderr, "io_uring submission queue full\n");
    return -1;
  }
  io_uring_prep_msg_ring(sqe, peer_fd, 1, sequence, 0);
  sqe->user_data = SEND_COMPLETION;
  int result = io_uring_submit(ring);
  if (result < 0)
    return ring_error("io_uring_submit", result);
  if (result != 1) {
    fprintf(stderr, "io_uring_submit did not submit the message\n");
    return -1;
  }
  return 0;
}

/* Local send completions and peer messages may arrive in either order. */
static int receive_completions(struct io_uring *ring, uint64_t sequence,
                               int need_send, int need_message) {
  while (need_send || need_message) {
    struct io_uring_cqe *cqe;
    struct __kernel_timespec timeout = {5, 0};
    int result;
    do {
      result = io_uring_wait_cqe_timeout(ring, &cqe, &timeout);
    } while (result == -EINTR);
    if (result < 0)
      return ring_error("io_uring_wait_cqe_timeout", result);

    uint64_t data = cqe->user_data;
    int res = cqe->res;
    io_uring_cqe_seen(ring, cqe);
    if (data == SEND_COMPLETION) {
      if (res < 0)
        return ring_error("MSG_RING", res);
      if (!need_send || res != 0) {
        fprintf(stderr, "unexpected MSG_RING send completion\n");
        return -1;
      }
      need_send = 0;
    } else {
      if (!need_message || data != sequence || res != 1) {
        fprintf(stderr, "unexpected MSG_RING peer message\n");
        return -1;
      }
      need_message = 0;
    }
  }
  return 0;
}

static int roundtrip(struct io_uring *ring, int peer_fd, uint64_t sequence) {
  if (send_message(ring, peer_fd, sequence) == -1)
    return -1;
  return receive_completions(ring, sequence, 1, 1);
}

int main(int argc, char *argv[]) {
  char *end;
  int64_t count;
  struct io_uring parent_ring, child_ring;
  struct io_uring_probe *probe;
  struct timespec start, stop;
  int status, result;
  int exit_status = EXIT_FAILURE;
  pid_t child, waited;

  if (argc != 2) {
    fprintf(stderr, "usage: io_uring_lat <roundtrip-count>\n");
    return EXIT_FAILURE;
  }
  errno = 0;
  count = strtoimax(argv[1], &end, 10);
  if (errno || end == argv[1] || *end || count <= 0 ||
      count > INT64_MAX / 2) {
    fprintf(stderr, "invalid roundtrip count\n");
    return EXIT_FAILURE;
  }

  result = io_uring_queue_init(8, &parent_ring, 0);
  if (result < 0) {
    ring_error("io_uring_queue_init", result);
    return EXIT_FAILURE;
  }
  probe = io_uring_get_probe_ring(&parent_ring);
  if (probe == NULL ||
      !io_uring_opcode_supported(probe, IORING_OP_MSG_RING)) {
    fprintf(stderr, "IORING_OP_MSG_RING is not supported\n");
    io_uring_free_probe(probe);
    io_uring_queue_exit(&parent_ring);
    return EXIT_FAILURE;
  }
  io_uring_free_probe(probe);
  result = io_uring_queue_init(8, &child_ring, 0);
  if (result < 0) {
    ring_error("io_uring_queue_init", result);
    io_uring_queue_exit(&parent_ring);
    return EXIT_FAILURE;
  }

  /* Create both rings before fork so each process inherits its peer's fd.
     Only the parent accesses parent_ring's queues; only the child accesses
     child_ring's queues. No SINGLE_ISSUER or task-bound setup flags are used. */
  child = fork();
  if (child == -1) {
    perror("fork");
    io_uring_queue_exit(&child_ring);
    io_uring_queue_exit(&parent_ring);
    return EXIT_FAILURE;
  }
  if (child == 0) {
    int child_status = EXIT_FAILURE;
    if (receive_completions(&child_ring, 0, 0, 1) == -1)
      goto child_cleanup;
    /* Sequence zero is the untimed warm-up. */
    for (int64_t i = 0; i <= count; ++i) {
      if (send_message(&child_ring, parent_ring.ring_fd, (uint64_t)i) == -1 ||
          receive_completions(&child_ring, (uint64_t)i + 1, 1,
                              i < count) == -1)
        goto child_cleanup;
    }
    child_status = EXIT_SUCCESS;
child_cleanup:
    io_uring_queue_exit(&child_ring);
    io_uring_queue_exit(&parent_ring);
    _exit(child_status);
  }

  if (roundtrip(&parent_ring, child_ring.ring_fd, 0) == -1)
    goto cleanup;
  if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
    perror("clock_gettime");
    goto cleanup;
  }
  for (int64_t i = 1; i <= count; ++i) {
    if (roundtrip(&parent_ring, child_ring.ring_fd, (uint64_t)i) == -1)
      goto cleanup;
  }
  if (clock_gettime(CLOCK_MONOTONIC, &stop) == -1) {
    perror("clock_gettime");
    goto cleanup;
  }
  exit_status = EXIT_SUCCESS;

cleanup:
  if (exit_status != EXIT_SUCCESS)
    kill(child, SIGTERM);
  do {
    waited = waitpid(child, &status, 0);
  } while (waited == -1 && errno == EINTR);
  io_uring_queue_exit(&child_ring);
  io_uring_queue_exit(&parent_ring);
  if (waited == -1) {
    perror("waitpid");
    return EXIT_FAILURE;
  }
  if (exit_status != EXIT_SUCCESS || !WIFEXITED(status) ||
      WEXITSTATUS(status) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  int64_t delta = (int64_t)(stop.tv_sec - start.tv_sec) * 1000000000LL +
                  (stop.tv_nsec - start.tv_nsec);
  printf("roundtrip count: %" PRId64 "\n", count);
  printf("average roundtrip latency: %" PRId64 " ns\n", delta / count);
  printf("average latency: %" PRId64 " ns\n", delta / (count * 2));
  return EXIT_SUCCESS;
}
