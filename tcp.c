/* Measure latency of IPC using TCP sockets */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <netdb.h>

int main(void)
{
  int yes = 1;
  int s;
  struct timespec start, stop;

  int64_t delta;

  int64_t max = 0;
  int64_t min = INT64_MAX;
  int64_t sum = 0;
  int64_t count = 0;

  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  struct addrinfo hints;
  struct addrinfo *res;
  int sockfd, new_fd;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
  getaddrinfo("127.0.0.1", "3491", &hints, &res);

  if (!fork()) {  /* child */

    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
      perror("socket");
      exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    } 

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
      perror("bind");
      exit(1);
    }
    
    if (listen(sockfd, 1) == -1) {
      perror("listen");
      exit(1);
    } 
    
    addr_size = sizeof their_addr;
    
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1) {
      perror("accept");
      exit(1);
    } 
    
    sleep(1);

    while (1) {
      clock_gettime(CLOCK_MONOTONIC, &start);

      if (write(new_fd, &start, sizeof(struct timespec)) == -1) {
        perror("write");
        exit(1);
      }
      
      usleep(10000);
    }
  } else { /* parent */

    sleep(1);
    
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
      perror("socket");
      exit(1);
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
      perror("connect");
      exit(1);
    }

    while (1) {
      if (read(sockfd, &start, sizeof(struct timespec)) == -1) {
        perror("read");
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
