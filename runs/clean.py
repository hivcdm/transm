import shutil
import os
import os.path
import sys

if __name__ == '__main__':
    directory = os.path.dirname(sys.argv[0])
    subdirectories = [directory + os.sep + i for i in os.listdir(directory) if os.path.isdir(i)]
    print(subdirectories)
    subdirs_with_results = [i + os.sep + 'results' for i in subdirectories if os.path.isdir(i + os.sep + 'results')]
    if(len(subdirs_with_results) > 0):
        a = None
        while(a not in ['yes', 'no']):
            a = input('Deleting ' + ','.join(subdirs_with_results) + ' is this okay?(yes/no): ')
        if a == 'yes':
            for i in subdirs_with_results:
                shutil.rmtree(i)
    else:
        print('No subdirectories containing a results directory were found.')
