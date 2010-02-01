
CFLAGS = -lrt

all: pipes pipes-busy pipes-poll queues pqueues spair unix_lat tcp tcp_lat shm pshm thread-pipes

clean:
	rm -f *~ pipes pipes-busy pipes-poll queues pqueues spair unix_lat tcp tcp_lat shm pshm thread-pipes
