cmake_minimum_required(VERSION 2.8)
project(ipc-bench C)

add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(pipe_lat src/pipe_lat.c)
add_executable(pipe_thr src/pipe_thr.c)
add_executable(tcp_lat src/tcp_lat.c)
add_executable(tcp_local_lat src/tcp_local_lat.c)
add_executable(tcp_remote_lat src/tcp_remote_lat.c)
add_executable(tcp_thr src/tcp_thr.c)
add_executable(udp_lat src/udp_lat.c)
add_executable(unix_lat src/unix_lat.c)
add_executable(unix_thr src/unix_thr.c)

