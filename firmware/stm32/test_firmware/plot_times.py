import os
import pickle

import numpy

output_path = "output"
output_filename = os.path.join(output_path, "boot_times.pickle")
boot_times = pickle.load(open(output_filename, "rb"))

base_case = []
for f in boot_times['total']:
    if f == "none.bin":
        base_case = boot_times['total'][f]

base_avg = numpy.average(base_case)
for f in boot_times['total']:
    average = numpy.average(boot_times['total'][f])
    prct = (average - base_avg) / base_avg
    average_adj = numpy.average(boot_times['total'][f]) - numpy.average(boot_times['flash'][f])
    prct_adj = (average_adj - base_avg) / base_avg
    print("%s\t%d\t%0.04f\t%d\t%0.04f" % (f, average, prct, numpy.average(boot_times['flash'][f]), prct_adj))
