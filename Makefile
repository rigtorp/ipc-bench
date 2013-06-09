
CFLAGS = -g

all: pipe_lat pipe_thr \
	unix_lat unix_thr \
	tcp_lat tcp_thr \
	tcp_local_lat tcp_remote_lat \
	shm

run:
	./pipe_lat 100 10000
	./unix_lat 100 10000
	./tcp_lat 100 10000
	./pipe_thr 100 10000
	./unix_thr 100 10000
	./tcp_thr 100 10000

clean:
	rm -f *~ core
	rm -f pipe_lat pipe_thr 
	rm -f unix_lat unix_thr 
	rm -f tcp_lat tcp_thr 
	rm -f tcp_local_lat tcp_remote_lat
	rm -f shm 
