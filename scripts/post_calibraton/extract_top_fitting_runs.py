import os
from os import path
import shutil
from glob import glob
import heapq

def top_fitting_run_names(post_calib_file, max_cumulative_weight):
    delimiter = '\t'
    run_name_column_text = 'run name'
    weight_passed_runs_column_text = 'Weight Passed Runs'
    
    read_header = False
    run_name_column_index = 0
    weight_passed_runs_column_index = 0
    run_weights = []
    for row in post_calib_file:
        if not read_header:
            header = row.split(delimiter)
            run_name_column_index = header.index(run_name_column_text)
            weight_passed_runs_column_index = header.index(weight_passed_runs_column_text)
            read_header = True
            continue
        row_values = row.split(delimiter)
        run_name = row_values[run_name_column_index]
        weight_string = row_values[weight_passed_runs_column_index]
        if weight_string == 'None' or not weight_string: continue
        heapq.heappush(run_weights, (-1 * float(weight_string), run_name))

    print('Found ' + str(len(run_weights)) + ' total passed runs in ' + post_calib_file.name + '.')

    cumulative_weight = 0    
    while run_weights:
        weight, run = heapq.heappop(run_weights)
        cumulative_weight += -1 * weight
        if cumulative_weight < max_cumulative_weight:
            yield run
        else:
            break

def extract_top_fitting_runs(top_level_directory, max_cumulative_weight):
    post_calib_filename = 'post_calib.out'
    print('Reading from directory ' + top_level_directory + '.')
    
    passed_runs_directory = path.join(top_level_directory, 'passed_runs')
    passed_runs_results_directory = path.join(passed_runs_directory, 'results')
    
    output_directory = path.join(top_level_directory, 'top_fitting_runs')
    if not path.exists(output_directory):
        os.mkdir(output_directory)
    output_results_directory = path.join(output_directory, 'results')
    if not path.exists(output_results_directory):
        os.mkdir(output_results_directory)

    count = 0
    post_calib_path = path.join(top_level_directory, post_calib_filename)
    for run in top_fitting_run_names(open(post_calib_path), max_cumulative_weight):
        shutil.copy(path.join(passed_runs_directory, run + '.xml'), output_directory)
        matching_results = glob(path.join(passed_runs_results_directory, run + '*'))
        for result_file in matching_results:
            shutil.copy(result_file, output_results_directory)
        count += 1

    print('Copied ' + str(count) + ' runs and their associated results comprising the top ' + str(int(max_cumulative_weight * 100)) + '% of passed weights to directory ' + output_directory + '.')
    
if __name__ == '__main__':
    extract_top_fitting_runs(os.getcwd(), 0.9)
