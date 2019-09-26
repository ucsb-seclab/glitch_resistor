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
import mpl_toolkits.mplot3d.art3d as a3d

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

    # Patch up icorrect logging
    if results['parameters']['widths'][-1] == 48:
        results['parameters']['widths'] = np.append(results['parameters'][
                                                        'widths'], 49)
    if results['parameters']['offsets'][-1] == 48:
        results['parameters']['offsets'] = np.append(results['parameters'][
                                                         'offsets'], 49)
    print(f)
    # pprint.pprint(results['parameters'])
    total_samples = len(results['parameters']['ext_offsets'])
    total_samples *= len(results['parameters']['widths'])
    total_samples *= len(results['parameters']['offsets'])
    # total_samples *= results['parameters']['repeat']

    # print(total_samples)
    partials = 0
    if 'partial_successes' in results:
        print("partial", results['partial_successes'])
        partials = results['partial_successes']
    print("Success rate:", (total_samples - results['failures'] - partials) /
          total_samples)
    print("Time:", results['parameters']['time_elapsed'])
    print(len(results['successes']), results['failures'])

    values = {}
    xs = []
    ys = []
    zs = []
    for params in results['successes']:
        ext_offset, width, offset, successes, partial_successes, sample_size, \
        success_result = params

        value_str = success_result.split("\n")

        if SUCCESS in success_result:
            xs.append(ext_offset)
            ys.append(width)
            zs.append(offset)

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

    for idx in range(len(xs)):
        l = a3d.Line3D((xs[idx], xs[idx]), (ys[idx], ys[idx]), (min(zs) - 1,
                                                                zs[idx]),
                       color='gray', ls='--', linewidth=.5)
        l.set_alpha(0.5)
        ax.add_line(l)

    ax.scatter(xs, ys, zs)
    ax.set_xticks(xs)
    ax.set_xlabel('Target Offset', fontsize=20)
    ax.set_xlim([min(xs), max(xs)])
    ax.set_ylim([min(ys), max(ys)])
    ax.set_zlim([min(zs), max(zs)])
    ax.set_ylabel('Width', fontsize=20)
    ax.set_zlabel('Offset', fontsize=20)


    # plt.show()
    filename = os.path.join(path, '%s.pdf' % os.path.splitext(f)[0])
    plt.savefig(filename)

    # print(f)
    pprint.pprint(values)

    print ("--\n")
