/* 
    Measure latency of IPC using unix domain sockets


    Copyright (c) 2010 Erik Rigtorp <erik@rigtorp.com>

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


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <stdint.h>


int main(int argc, char *argv[])
{
  int ofds[2];
  int ifds[2];

  int size;
  char *buf;
  int64_t count, i, delta;
  struct timeval start, stop;

  if (argc != 3) {
    printf ("usage: pipe_lat <message-size> <roundtrip-count>\n");
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

  if (pipe(ofds) == -1) {
    perror("pipe");
    return 1;
  }

  if (pipe(ifds) == -1) {
    perror("pipe");
    return 1;
  }

  if (!fork()) {  /* child */
    for (i = 0; i < count; i++) {
      
      if (read(ifds[0], buf, size) != size) {
        perror("read");
        return 1;
      }
      
      if (write(ofds[1], buf, size) != size) {
        perror("write");
        return 1;
      }
    }
  } else { /* parent */
  
    gettimeofday(&start, NULL);

    for (i = 0; i < count; i++) {

      if (write(ifds[1], buf, size) != size) {
        perror("write");
        return 1;
      }

      if (read(ofds[0], buf, size) != size) {
        perror("read");
        return 1;
      }
      
    }

    gettimeofday(&stop, NULL);

    delta = ((stop.tv_sec - start.tv_sec) * (int64_t) 1000000 +
	     stop.tv_usec - start.tv_usec);
    
    printf("average latency: %lli us\n", delta / (count * 2));

  }
  
  return 0;
}
