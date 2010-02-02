
CFLAGS = -lrt

all: pipe_lat pipe_thr unix_lat unix_thr tcp_lat tcp_thr shm 

clean:
	rm -f *~ pipe_lat pipe_thr unix_lat unix_thr tcp_lat tcp_thr shm 
