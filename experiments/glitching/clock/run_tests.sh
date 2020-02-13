#!/usr/bin/env bash


# Broad scanning experiments
# while (a)
python loop_glitch.py single -o results_usenix/glitch_single_zero.pickle

# while (!a)
python loop_glitch.py single_one -o results_usenix/glitch_single_one.pickle

# while (a != XXXX)
python loop_glitch.py store -o results_usenix/glitch_single_fixed.pickle

python loop_glitch.py multi -o results_usenix/glitch_multi.pickle

python loop_glitch.py multi_one -o results_usenix/glitch_multi_one.pickle

python loop_glitch.py multi_fixed -o results_usenix/glitch_multi_fixed.pickle

python loop_glitch.py long -o results_usenix/glitch_long2.pickle
python loop_glitch.py long_one -o results_usenix/glitch_long_one2.pickle
python loop_glitch.py long_fixed -o results_usenix/glitch_long_fixed2.pickle

# Real Defenses
python loop_glitch.py long_real -o results_usenix/real_long.pickle
python loop_glitch.py scan10 -o results_usenix/real_10.pickle
python loop_glitch.py single_real -o results_usenix/real_single.pickle

# Real Defenses (no delay)
python loop_glitch.py long_real --nodelay -o results_usenix/real_long_nodelay.pickle
python loop_glitch.py scan10 --nodelay -o results_usenix/real_10_nodelay.pickle
python loop_glitch.py single_real --nodelay -o results_usenix/real_single_nodelay.pickle

python loop_glitch.py single_real --function=WHILENOT0 -o results_usenix/real_single_not0.pickle
python loop_glitch.py scan10 --function=WHILENOT0 -o results_usenix/real_10_not0.pickle
python loop_glitch.py long_real --function=WHILENOT0 -o results_usenix/real_long_not0.pickle
python loop_glitch.py single_real --function=WHILENOT0 --nodelay -o results_usenix/real_single_nodelay_not0.pickle
python loop_glitch.py scan10 --function=WHILENOT0 --nodelay -o results_usenix/real_10_nodelay_not0.pickle
python loop_glitch.py long_real --function=WHILENOT0 --nodelay -o results_usenix/real_long_nodelay_not0.pickle

python loop_glitch.py single_real --function=WHILE1 -o results_usenix/real_single_one.pickle
python loop_glitch.py long_real --function=WHILE1 -o results_usenix/real_long_one.pickle
python loop_glitch.py single_real --function=WHILE1 --nodelay -o results_usenix/real_single_nodelay_one.pickle
python loop_glitch.py scan10 --function=WHILE1 -o results_usenix/real_10_one.pickle
python loop_glitch.py scan10 --function=WHILE1 --nodelay -o results_usenix/real_10_nodelay_one.pickle
python loop_glitch.py long_real --function=WHILE1 --nodelay -o results_usenix/real_long_nodelay_one.pickle


python loop_glitch.py single_real --function=WHILEFIXED -o results_usenix/real_single_fixed.pickle
python loop_glitch.py long_real --function=WHILEFIXED -o results_usenix/real_long_fixed.pickle
python loop_glitch.py single_real --function=WHILEFIXED --nodelay -o results_usenix/real_single_nodelay_fixed.pickle
python loop_glitch.py scan10 --function=WHILEFIXED --nodelay -o results_usenix/real_10_nodelay_fixed.pickle
python loop_glitch.py scan10 --function=WHILEFIXED -o results_usenix/real_10_fixed.pickle
python loop_glitch.py long_real --function=WHILEFIXED --nodelay -o results_usenix/real_long_nodelay_fixed.pickle

# Optimization tests
#python loop_glitch.py store_optimal -o results_usenix/glitch_single_fixed_optimal.pickle

#python loop_glitch.py single_one_optimal -o results_usenix/glitch_single_one_optimal.pickle

#python loop_glitch.py single_optimal -o results_usenix/glitch_single_zero_optimal.pickle

