
CFLAGS = -lrt

all: pipe_lat pipe_thr unix_lat unix_thr tcp_lat tcp_thr shm 

run:
	./pipe_lat 100 10000
	./unix_lat 100 10000
	./tcp_lat 100 10000
	./pipe_thr 100 10000
	./unix_thr 100 10000
	./tcp_thr 100 10000

clean:
	rm -f *~ pipe_lat pipe_thr unix_lat unix_thr tcp_lat tcp_thr shm 
