#!/usr/bin/env bash


python loop_glitch.py single -o glitch_single_zero.pickle

python loop_glitch.py single_one -o glitch_single_one.pickle

python loop_glitch.py store -o glitch_store.pickle

python loop_glitch.py multi -o glitch_multi.pickle

python loop_glitch.py multi_fixed -o glitch_multi_fixed.pickle


python loop_glitch.py long -o glitch_long.pickle

python loop_glitch.py store_optimal -o glitch_store_optimal.pickle

python loop_glitch.py single_optimal -o glitch_single_optimal.pickle


# Real Defenses
python loop_glitch.py optimal_real -o real_optimal.pickle
python loop_glitch.py long_real -o real_long.pickle