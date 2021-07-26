ipc-bench
=========

[![C/C++ CI](https://github.com/rigtorp/ipcbench/workflows/C/C++%20CI/badge.svg)](https://github.com/rigtorp/ipcbench/actions)
[![GitHub](https://img.shields.io/github/license/rigtorp/ipc-bench.svg)](https://github.com/rigtorp/ipc-bench/blob/master/LICENSE)
[![Travis (.org)](https://img.shields.io/travis/rigtorp/ipc-bench.svg)](https://travis-ci.org/rigtorp/ipc-bench)

Some very crude IPC benchmarks.

ping-pong latency benchmarks:

* pipes
* unix domain sockets
* tcp sockets

throughput benchmarks:

* pipes
* unix domain sockets
* tcp sockets

This software is distributed under the MIT License.

Credits
-------

* *desbma* for adding cross platform support for clock_gettime
