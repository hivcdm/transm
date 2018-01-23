#!python
# make_runs.py
#
# Used in conjuction with the generate_runs script to create
# batches for multiple runsets in a directory.
#
# EXAMPLE: python make_runs.py ../Scenarios .

import os
import sys
import shutil

from generate_runs import *

if __name__ == '__main__':

    if len(sys.argv) < 2:
        exit()

    path = sys.argv[1]
    out_dir = sys.argv[2]
    os.mkdir(out_dir)
 
    if os.path.isdir(path):
        for top, dirs, fs in os.walk(path):
            for f in fs:
                split = os.path.splitext(f)
                if split[1] == ".xml":
                    out=split[0]
                    generate_runs(top+'/'+f, weight_cutoff=0.9, batch_size=1, run_set_name=out)
                    shutil.move(out, out_dir)
    else:
        print(sys.argv[1], "is not a directory")
        exit(1)
                        
    exit(0)

