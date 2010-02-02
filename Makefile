
CFLAGS = -lrt

all: pipe_lat pipe_thr pipes pipes-busy pipes-poll queues pqueues spair unix_lat unix_thr tcp tcp_lat tcp_thr shm pshm thread-pipes

clean:
	rm -f *~ pipe_lat pipe_thr pipes pipes-busy pipes-poll queues pqueues spair unix_lat unix_thr tcp tcp_lat tcp_thr shm pshm thread-pipes
