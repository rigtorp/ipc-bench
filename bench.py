#!/usr/bin/env python3

import os
import re
import subprocess
import sys

from subprocess import PIPE

message_sizes = []

for i in range(0, 21):
    message_sizes.append(2 ** i)

if len(sys.argv) != 2:
    print("Usage: " + sys.argv[0] + " [ lat | thr ]")
    exit(1)

suite = sys.argv[1]

if (suite != "lat") and (suite != "thr"):
    print("Usage: " + sys.argv[0] + " [ lat | thr ]")
    exit(1)

if suite == "thr":
    message_count = 100_000
    pattern = re.compile(r'average throughput: (\d+) Mb/s')
else:
    message_count = 100_000
    pattern = re.compile(r'average latency: (\d+) ns')

scriptdir = os.path.dirname(os.path.realpath(__file__))

files = [f for f in os.listdir(scriptdir) if os.path.isfile(f)]

for f in files:
    if not os.access(f, os.X_OK):
        continue
    if not f.endswith(suite):
        continue
    scenario = f[:-len("_" + suite)]
    if scenario == "tcp_local" or scenario == "tcp_remote":
        continue
    for message_size in message_sizes:
        print(scenario, message_size, end =" ")
        sys.stdout.flush()
        completed = subprocess.run(["./" + f, str(message_size), str(message_count)], stdout=PIPE)
        m = pattern.search(completed.stdout.decode('utf-8'))
        print(m.group(1))