#!/usr/bin/env bash

# Optimization tests
python loop_glitch.py store_optimal -o results_usenix/glitch_single_fixed_optimal.pickle

python loop_glitch.py single_one_optimal -o results_usenix/glitch_single_one_optimal.pickle

python loop_glitch.py single_optimal -o results_usenix/glitch_single_zero_optimal.pickle


# Broad scanning experiments
python loop_glitch.py single -o results_usenix/glitch_single_zero.pickle

python loop_glitch.py single_one -o results_usenix/glitch_single_one.pickle

python loop_glitch.py store -o results_usenix/glitch_single_fixed.pickle

python loop_glitch.py multi -o results_usenix/glitch_multi.pickle

python loop_glitch.py multi_fixed -o results_usenix/glitch_multi_fixed.pickle

python loop_glitch.py long -o results_usenix/glitch_long.pickle


# Real Defenses
python loop_glitch.py single_real -o results_usenix/real_single.pickle

python loop_glitch.py scan10 -o results_usenix/real_10.pickle

python loop_glitch.py long_real -o results_usenix/real_long.pickle


# Real Defenses (no delay)
python loop_glitch.py single_real --nodelay -o results_usenix/real_single.pickle

python loop_glitch.py scan10 --nodelay -o results_usenix/real_10.pickle

python loop_glitch.py long_real --nodelay -o results_usenix/real_long.pickle

