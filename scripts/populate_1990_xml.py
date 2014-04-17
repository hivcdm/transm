import os
from os import path

cepac_in_files = [('SA Transmission cepac_inputs_10.09.13_NOART.in', 0, 0),
                  ('2002_PerfTesting.in', 2002, 2),
                  ('2004_PerfTesting.in', 2004, 2),
                  ('2010_PerfTesting.in', 2010, 2),
                  ('2013_PerfTesting.in', 2013, 2),
                  ('', -1, -1),
                  ('', -1, -1),
                  ('', -1, -1),
                  ('', -1, -1),
                  ('', -1, -1),
                  ('', -1, -1),
                  ('', -1, -1),
                  ('', -1, -1)]

def extract_months_of_1990(post_calib_file):
    delimiter = '\t'
    run_name_column_text = 'run name'
    weight_passed_runs_column_text = 'Month Of 1990'
    
    read_header = False
    run_name_column_index = 0
    weight_passed_runs_column_index = 0
    months_of_1990 = {}
    for row in post_calib_file:
        if not read_header:
            header = row.split(delimiter)
            run_name_column_index = header.index(run_name_column_text)
            weight_passed_runs_column_index = header.index(weight_passed_runs_column_text)
            read_header = True
            continue
        row_values = row.split(delimiter)
        run_name = row_values[run_name_column_index]
        month_of_1990 = int(row_values[weight_passed_runs_column_index])
        yield run_name, month_of_1990

def write_1990(in_file, month_of_1990, out_file):
    writing_rollout_files = False
    for line in in_file:
        if '<monthOf1990>' in line and '</monthOf1990>' in line:
            out_file.write(line[:line.index('<')] + '<monthOf1990>' + str(month_of_1990) + '</monthOf1990>\n')
        elif '<rolloutTreatmentFiles>' in line:
            writing_rollout_files = True
            written = False
        elif writing_rollout_files:
            if '</rolloutTreatmentFiles>' in line:
                writing_rollout_files = False
            else:
                if written:
                    continue
                written = True
                number = 0
                for cepac_file, year, pop in cepac_in_files:
                    out_file.write('<rolloutFile>\n')
                    if year == -1:
                        out_file.write('<time>-1</time>\n')
                        out_file.write('<fileName/>\n')
                        out_file.write('<fileNumber>' + str(number) + '</fileNumber>\n')
                        out_file.write('<popToApply>' + str(pop) + '</popToApply>\n')
                    else:
                        time = month_of_1990 + (year - 1990) * 12 if year else 0
                        out_file.write('<time>' + str(time) + '</time>\n')
                        out_file.write('<fileName>' + cepac_file + '</fileName>\n')
                        out_file.write('<fileNumber>' + str(number) + '</fileNumber>\n')
                        out_file.write('<popToApply>' + str(pop) + '</popToApply>\n')
                    out_file.write('</rolloutFile>\n')
                    number += 1
        else:
            out_file.write(line)

def populate_1990_xml(top_level_directory):
    post_calib_filename = 'post calib.out'
    print('Using directory ' + top_level_directory + '.')

    xml_directory = path.join(top_level_directory, 'top fitting runs')
    if not path.exists(xml_directory):
        print('Error: excpected a directory ' + xml_directory + ' containing one or more XML files to be transformed.')
        return
    
    output_directory = path.join(top_level_directory, 'top fitting runs fixed 1990')
    if not path.exists(output_directory):
        os.mkdir(output_directory)

    count = 0
    post_calib_path = path.join(top_level_directory, post_calib_filename)
    if not path.exists(post_calib_path):
        print('Error: expected a file called "post calib.out" in the main directory.')
        return
    
    for run, month in extract_months_of_1990(open(post_calib_path)):
        xml_file = path.join(xml_directory, run + '.xml')
        if path.isfile(xml_file):
            write_1990(open(xml_file), month, open(path.join(output_directory, run + '.xml'), 'w'))
            count += 1

    print('Updated monthOf1990 for ' + str(count) + ' runs.')
    print('New xml files are in directory ' + output_directory + '.')
    
if __name__ == '__main__':
    populate_1990_xml(os.getcwd())
