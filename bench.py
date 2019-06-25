#!/usr/bin/env python3

import os

import re

import subprocess
from subprocess import PIPE

import sys

MESSAGE_SIZES = []

for i in range(0, 21):
    MESSAGE_SIZES.append(2 ** i)

if len(sys.argv) != 2:
    print("Usage: " + sys.argv[0] + " [ lat | thr ]")
    exit(1)

def main():
    suite = sys.argv[1]

    if (suite != "lat") and (suite != "thr"):
        print("Usage: " + sys.argv[0] + " [ lat | thr ]")
        exit(1)

    if suite == "thr":
        message_count = 1_000_000
        pattern = re.compile(r'average throughput: (\d+) Mb/s')
    else:
        message_count = 100_000
        pattern = re.compile(r'average latency: (\d+) ns')

    scriptdir = os.path.dirname(os.path.realpath(__file__))

    files = [f for f in os.listdir(scriptdir) if os.path.isfile(f)]

    for filename in files:
        if not os.access(filename, os.X_OK):
            continue
        if not filename.endswith(suite):
            continue
        scenario = filename[:-len("_" + suite)]
        if scenario == "tcp_local" or scenario == "tcp_remote":
            continue
        for message_size in MESSAGE_SIZES:
            print(scenario, message_size, end=" ")
            sys.stdout.flush()
            args = ["./" + filename, str(message_size), str(message_count)]
            completed = subprocess.run(args, stdout=PIPE)
            results = pattern.search(completed.stdout.decode('utf-8'))
            print(results.group(1))

if __name__ == "__main__":
    main()
