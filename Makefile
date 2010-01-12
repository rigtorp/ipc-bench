
CFLAGS = -lrt

all: pipes pipes-busy pipes-poll queues pqueues spair tcp shm pshm thread-pipes

clean:
	rm -f *~ pipes pipes-busy pipes-poll queues pqueues spair tcp shm pshm thread-pipes
