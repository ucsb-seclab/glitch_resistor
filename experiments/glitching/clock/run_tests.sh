#!/usr/bin/env bash


#python loop_glitch.py single -o results_new/glitch_single_zero.pickle
#
#python loop_glitch.py single_one -o results_new/glitch_single_one.pickle
#
#python loop_glitch.py store -o results_new/glitch_store.pickle
#
#python loop_glitch.py multi -o results_new/glitch_multi.pickle
#
#python loop_glitch.py multi_fixed -o results_new/glitch_multi_fixed.pickle
#
#
#python loop_glitch.py long -o results_new/glitch_long.pickle


# Real Defenses
#python loop_glitch.py single_real -o results_new/real_single_no_delay.pickle

python loop_glitch.py scan10 -o results_new/real_10.pickle

python loop_glitch.py long_real -o results_new/real_long.pickle


# Optimization tests
#python loop_glitch.py store_optimal -o results_new/glitch_store_optimal.pickle
#
#python loop_glitch.py single_optimal -o results_new/glitch_single_optimal.pickle