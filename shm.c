/*
    Measure latency of IPC using shm

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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int main(void) {
  int shmid;
  key_t key;
  struct timeval *shm;

  struct timeval start, stop;

  int64_t delta;

  int64_t max = 0;
  int64_t min = INT64_MAX;
  int64_t sum = 0;
  int64_t count = 0;

  /*
   * We'll name our shared memory segment
   * "5678".
   */
  key = 5678;

  if (!fork()) {

    /*
     * Create the segment.
     */
    if ((shmid = shmget(key, 100, IPC_CREAT | 0666)) < 0) {
      perror("shmget");
      return 1;
    }

    /*
     * Now we attach the segment to our data space.
     */
    if ((shm = shmat(shmid, NULL, 0)) == (struct timeval *)-1) {
      perror("shmat");
      return 1;
    }

    while (1) {
      gettimeofday(shm, NULL);

      usleep(10000);
    }
  } else {
    sleep(1);

    /*
     * Locate the segment.
     */
    if ((shmid = shmget(key, 100, 0666)) < 0) {
      perror("shmget");
      return 1;
    }

    /*
     * Now we attach the segment to our data space.
     */
    if ((shm = shmat(shmid, NULL, 0)) == (struct timeval *)-1) {
      perror("shmat");
      return 1;
    }

    while (1) {
      while ((shm->tv_sec == start.tv_sec) && (shm->tv_usec == start.tv_usec)) {
      }

      gettimeofday(&stop, NULL);
      start = *shm;
      delta = ((stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec -
               start.tv_usec);

      if (delta > max)
        max = delta;
      else if (delta < min)
        min = delta;

      sum += delta;
      count++;

      if (!(count % 100)) {
        printf("%li %li %li\n", max, min, sum / count);
      }
    }
  }

  return 0;
}
