/* Measure latency of IPC using POSIX message queues */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <mqueue.h>


int main(void)
{
  mqd_t mq;
  struct mq_attr mq_attrs;        /* buffer for stat info */
  
  struct timespec start, stop;

  int64_t delta;

  int64_t max = 0;
  int64_t min = INT64_MAX;
  int64_t sum = 0;
  int64_t count = 0;

  mq_attrs.mq_msgsize = sizeof(struct timespec);
  mq_attrs.mq_maxmsg = 10;  

  if (!fork()) {    
    if ((mq = mq_open("/q1", O_CREAT | O_WRONLY, 0660, &mq_attrs)) == -1) {
      perror("mq_open");
      exit(1);
    }
    
    sleep(1);

    while (1) {
      clock_gettime(CLOCK_MONOTONIC, &start);

      if (mq_send(mq, &start, sizeof(struct timespec), 0) == -1) {
        perror("mq_send");
        exit(1);
      }

      usleep(10000);
    }
  } 
  else {
    sleep(1);
    
    if ((mq = mq_open("/q1", O_RDONLY, 0660, &mq_attrs)) == -1) { /* connect to the queue */
      perror("mq_open2");
      exit(1);
    }

    
    while (1) {
      if (mq_receive(mq, &start, sizeof(struct timespec), 0) == -1) {
        perror("mq_receive");
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
