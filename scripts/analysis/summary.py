#!/usr/bin/env python
'''Run this script on a directory of batch files to generate output plots'''
'''usage: summary.py [-h] [-f FILES [FILES ...]] [--debug] directory'''

import sys
import os
import argparse
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pylab as plt

data_dir = ''
output_dir = '.'

def list_files():
    print("Searching for plottable files...")
    files = set()
    for top, dirs, fs in os.walk(data_dir):
        for f in fs:
            if f.split('.')[1] == 'out':
                try: 
                    d = np.genfromtxt(top+'/'+f, delimiter='\t')
                except ValueError:
                    continue;
                files.add(f)
    if len(files) > 0:
        print("Plottable files:")
        for h in files:
            print(h)
    else:
        print("No plottable files found")

def summarize(f_in):
    data = []
    for top, dirs, fs in os.walk(data_dir):
        for f in fs:
            if f == f_in:
                try:
                    d = np.genfromtxt(top+'/'+f, delimiter='\t')
                except ValueError:
                    print(f + " is not a plottable output")
                    print("Run 'python summary.py <dir> -l|--list' to see plottable files")
                    return;
                data.extend([d[601:]])
    if data:
        print("Plotting data in: " + f_in)
        for d in data:
            plt.plot(d)
            plt.savefig(str(output_dir + '/' + f_in + '.png'))
        plt.close()
    else:
        print(f_in + " does not exists in " + data_dir)
    del data[:]
    
# Main Entry
if __name__ == '__main__':
    #Parse arguments from the command line
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', action='store', help='Directory containing output data')
    parser.add_argument('-f', '--files', action='store', nargs='+', required=False,\
                        help='A list of outputs to plot')
    parser.add_argument('-l', '--list', action='store_true', default=False,\
                        help='Lists plottable files')
    parser.add_argument('-o', '--output', action='store', help='Directory to output summary')
    parser.add_argument('--debug', action='store_true', default=False)
    args = parser.parse_args()

    if args.debug:
        print(args)

    data_dir = args.directory

    if args.list:
        list_files()
        exit(0)

    files = []
    if args.files:
        files = args.files
    else:
        # default outputs
        files.extend(['batchstats-SAprevalence.out'])
        files.extend(['batchstats-incidence.out'])

    if args.output:
        if not os.path.isdir(args.output):
            os.mkdir(args.output)
        output_dir = args.output

    for f in files:
        summarize(f)

    exit(0)
