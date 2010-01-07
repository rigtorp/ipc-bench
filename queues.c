/* Measure latency of IPC using SYS V queues */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/msg.h>


struct msgbuf {
  long mtype;
  struct timespec ts;
};


int main(void)
{
  struct msgbuf buf;
  int msqid;
  
  struct timespec start, stop;

  int64_t delta;

  int64_t max = 0;
  int64_t min = INT64_MAX;
  int64_t sum = 0;
  int64_t count = 0;

  key_t key;

  if ((key = ftok("queues.c", 'B')) == -1) {
    perror("ftok");
    exit(1);
  }

  
  if (!fork()) {
    if ((msqid = msgget(key, 0644 | IPC_CREAT)) == -1) {
      perror("msgget");
      exit(1);
    }
  
    buf.mtype = 1;

    sleep(1);

    while (1) {
      clock_gettime(CLOCK_MONOTONIC, &buf.ts);

      if (msgsnd(msqid, &buf, sizeof(struct timespec), 0) == -1) {
        perror("msgsnd");
        exit(1);
      }

      usleep(10000);
    }
  } 
  else {
    if ((msqid = msgget(key, 0644)) == -1) { /* connect to the queue */
      perror("msgget");
      exit(1);
    }
    
    while (1) {  
      if (msgrcv(msqid, &buf, sizeof(struct timespec), 0, 0) == -1) {
        perror("msgrcv");
        exit(1);
      }

      clock_gettime(CLOCK_MONOTONIC, &stop);
      delta = ((stop.tv_sec - buf.ts.tv_sec) * (int64_t) 1000000000 +
               stop.tv_nsec - buf.ts.tv_nsec);
      
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
