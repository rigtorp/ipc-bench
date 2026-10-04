ipc-bench
=========

[![C/C++ CI](https://github.com/rigtorp/ipc-bench/workflows/C/C++%20CI/badge.svg)](https://github.com/rigtorp/ipc-bench/actions)
[![GitHub](https://img.shields.io/github/license/rigtorp/ipc-bench.svg)](https://github.com/rigtorp/ipc-bench/blob/master/LICENSE)

Some very crude IPC benchmarks.

ping-pong latency benchmarks:

* pipes
* unix domain sockets
* tcp sockets
* eventfd (Linux only; notifications)
* io_uring MSG_RING (Linux only; ring-to-ring messages)

throughput benchmarks:

* pipes
* unix domain sockets
* tcp sockets

This software is distributed under the MIT License.

Building
--------

Requires a C compiler, CMake 3.10 or newer, and a build tool such as Make
or Ninja. On Linux, install the optional liburing development package
(version 2.2 or newer) and pkg-config to also build `io_uring_lat`.

From the repository root:

```sh
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

Executables are created in the `build` directory. For example, from that
directory, measure pipe latency with 8-byte messages and 100,000 roundtrips:

```sh
./pipe_lat 8 100000
```

On Linux, you can also run:

```sh
./eventfd_lat 100000
./io_uring_lat 100000  # if built with liburing
```

Credits
-------

* *desbma* for adding cross platform support for clock_gettime
