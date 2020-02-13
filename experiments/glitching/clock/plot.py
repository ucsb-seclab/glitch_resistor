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

results_table = []

for f in onlyfiles:
    results = pickle.load(open(os.path.join(path, f), "rb"))

    cycle_plots = {}

    if len(results) == 0:
        continue

    # Patch up incorrect logging
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
    if len(results['parameters']['repeats']) > 0:
        total_samples *= len(results['parameters']['repeats'])
    print(total_samples)

    # print(total_samples)
    partials = 0
    if 'partial_successes' in results:
        print("partial", results['partial_successes'])
        partials = results['partial_successes']
    print("Success rate:", (total_samples - results['failures'] - partials) /
          total_samples)
    print("Time:", results['parameters']['time_elapsed'])
    print(len(results['successes']), results['failures'], total_samples)

    values = {}
    xs = []
    ys = []
    zs = []

    detection_xs = []
    detection_ys = []
    detection_zs = []

    successes_total = 0
    for params in results['successes']:
        repeat = 0
        if len(params) == 8:
            ext_offset, width, offset, successes, partial_successes, sample_size, \
            success_result, repeat = params
        else:
            ext_offset, width, offset, successes, partial_successes, sample_size, \
            success_result = params

        value_str = success_result.split("\n")

        if SUCCESS in success_result:
            successes_total += successes
            if "long" in f:
                xs.append(repeat)
            else:
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

            if ext_offset not in cycle_plots:
                cycle_plots[ext_offset] = {'widths': [],
                                           'offsets': [],
                                           'success_rates': []}
            cycle_plots[ext_offset]['widths'].append(width)
            cycle_plots[ext_offset]['offsets'].append(offset)
            cycle_plots[ext_offset]['success_rates'].append(
                1.0 * successes / sample_size)

        if value is not None:
            if value not in values:
                values[value] = 0
            values[value] += 1

    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    print("Success rate:", successes_total / total_samples, results['failures'])

    if "detections" in results and len(results['detections']) > 0:

        print(
            "Detection rate:", len(results['detections']) /
            (len(results['detections']) + successes_total))
        for params in results['detections']:
            ext_offset, width, offset, successes, partial_successes, sample_size, \
            success_result, repeat = params

            if "long" in f:
                detection_xs.append(repeat)
            else:
                detection_xs.append(ext_offset)
            detection_ys.append(width)
            detection_zs.append(offset)

        results_table.append([f, total_samples, successes_total, len(results[
                                                                         'detections'])])
    else:
        results_table.append([f, total_samples, successes_total, 0])

    for idx in range(len(xs)):
        l = a3d.Line3D((xs[idx], xs[idx]), (ys[idx], ys[idx]),
                       (min(results['parameters']['offsets']) - 2, zs[idx]),
                       color='gray', ls='--', linewidth=.5)
        l.set_alpha(0.5)
        ax.add_line(l)
    for idx in range(len(detection_xs)):
        l = a3d.Line3D((detection_xs[idx], detection_xs[idx]),
                       (detection_ys[idx], detection_ys[idx]),
                       (min(results['parameters']['offsets']) - 2, detection_zs[
                           idx]),
                       color='red', ls='--', linewidth=.5)
        l.set_alpha(0.5)
        ax.add_line(l)

    ax.scatter(xs, ys, zs)
    ax.scatter(detection_xs, detection_ys, detection_zs, marker='x',
               color='red')

    if "long" in f:
        print(min(results['parameters']['repeats']))
        ax.set_xticks(xs + detection_xs)
        ax.set_xlabel('Repeat Count', fontsize=20)
        ax.set_xlim([min(results['parameters']['repeats']),
                     max(results['parameters']['repeats'])])
    else:
        ax.set_xticks(xs + detection_xs)
        ax.set_xlabel('Target Offset', fontsize=20)
        ax.set_xlim([min(results['parameters']['ext_offsets']),
                     max(results['parameters']['ext_offsets'])])
        # print(f)
        pprint.pprint(values)
    ax.set_ylim([min(results['parameters']['widths']),
                 max(results['parameters']['widths'])])
    ax.set_zlim([min(results['parameters']['offsets']),
                 max(results['parameters']['offsets'])])
    ax.set_ylabel('Width', fontsize=20)
    ax.set_zlabel('Offset', fontsize=20)

    # plt.show()
    filename = os.path.join(path, '%s.pdf' % os.path.splitext(f)[0])
    plt.savefig(filename, transparent=True)
    plt.close()

    pprint.pprint(results['parameters'])

    print ("--\n")

    if len(cycle_plots.keys()) == 0:
        continue

    fig, axs = plt.subplots(1, max(cycle_plots.keys()) + 1, sharex='col',
                            sharey='row',
                            gridspec_kw={'hspace': 0, 'wspace': 0},
                            figsize=(10 * (max(cycle_plots.keys()) + 1), 10))
    print(cycle_plots)
    print(axs)
    for ext_offset in cycle_plots:
        rgba_colors = np.zeros(
            (len(cycle_plots[ext_offset]['success_rates']), 4))
        # for red the first column needs to be one
        rgba_colors[:, 0] = 1.0
        # the fourth column needs to be your alphas
        rgba_colors[:, 3] = cycle_plots[ext_offset]['success_rates']
        if len(cycle_plots.keys()) == 1:
            cur_axs = axs
        else:
            cur_axs = axs[ext_offset]
        cur_axs.scatter(cycle_plots[ext_offset]['widths'],
                        cycle_plots[ext_offset]['offsets'],
                        color=rgba_colors)
    # Save to file
    filename = os.path.join(path, '%s_cycles.pdf' % (
        os.path.splitext(f)[0]))
    fig.savefig(filename, transparent=True)
    # plt.show()
    plt.close()

for r in results_table:
    print("\t".join([str(x) for x in r]))
