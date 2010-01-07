
CFLAGS = -lrt

all: pipes pipes-busy queues pqueues spair tcp shm pshm

clean:
	rm -f *~ pipes pipes-busy queues pqueues spair tcp shm pshm
