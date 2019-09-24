#!/usr/bin/env python
import os
import pickle
import pprint
import sys
from os import listdir
from os.path import isfile, join

from mpl_toolkits import mplot3d
import numpy as np
import matplotlib.pyplot as plt

path = sys.argv[1]
onlyfiles = [f for f in listdir(path) if isfile(join(sys.argv[1], f))]
onlyfiles = [f for f in onlyfiles if f.endswith("pickle")]

ORIGINAL_VALUES = [0xE7D25763, 1]
GUARD_VALUE = 3552161478

SUCCESS = "Yes!"
SUCCESS_PARTIAL = "Yes1!"

if not os.path.exists("figures"):
    try:
        os.makedirs("figures")
    except:
        print("Could not create directory ('figures').")
        sys.exit(0)

for f in onlyfiles:
    results = pickle.load(open(os.path.join(path, f), "rb"))

    if len(results) == 0:
        continue

    print(len(results['successes']), results['failures'])

    values = {}
    xs = []
    ys = []
    zs = []
    for params in results['successes']:
        ext_offset, width, offset, successes, partial_successes, sample_size, \
        success_result = params

        xs.append(ext_offset)
        ys.append(width)
        zs.append(offset)

        value_str = success_result.split("\n")

        value_partial = None
        value = None
        if SUCCESS_PARTIAL in value_str:
            value_partial = 0
            idx = 0
            for ch in value_str[value_str.index(SUCCESS_PARTIAL) - 1]:
                value_partial |= ord(ch) << idx
                idx += 8

        if SUCCESS in value_str:
            value = 0
            idx = 0
            for ch in value_str[value_str.index(SUCCESS) - 1]:
                value |= ord(ch) << idx
                idx += 8

        if value is not None:
            if value not in values:
                values[value] = 0
            values[value] += 1

    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    ax.scatter(xs, ys, zs)
    plt.show()

    print(f)
    pprint.pprint(values)
