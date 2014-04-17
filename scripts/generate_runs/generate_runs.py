import os
import sys
from openpyxl.reader.excel import load_workbook

header = ['Assort', 'PropHRMale', 'PropHRFemale', 'epsilon', 'HRMult', 'CSWMult', 'RegActs', 'ChanceCSW', 'AqRateStdyLR', 'AqRateRegLR', 'AqRateCasLR', 'AqRateCSWLR', 'AqRateStdyHR', 'AqRateRegHR', 'AqRateCasHR', 'AqRateCSWHR']

def read_parameters(parameters_filename):
    print('Reading parameters...', end='')
    workbook = load_workbook(filename = parameters_filename)
    sheet_ranges = workbook.get_sheet_by_name(name='Sheet1')
    rows = [[cell.value for cell in row] for row in sheet_ranges.rows[1:]]
    print('done.')

    return rows

def read_template(template_filename):
    print('Reading template...', end='')
    with open(template_filename, 'r') as template_file:
        template = template_file.read()
    print('done.')
    return template

def filter_rows_by_weight(rows, cutoff):
    sum_weight = 0
    parameter_sets = {}

    print('Filtering runs by weight...', end='')
    for row in sorted(rows, key=lambda r: float(r[1]), reverse=True):
        sum_weight += float(row[1])
        parameter_sets[row[0]] = [float(i) for i in row[2:]]
        if sum_weight > cutoff:
            break
    print('done.')

    return parameter_sets

def try_make_directory(directory):
    try:
        os.mkdir(directory)
    except:
        print()
        print('Error: output folder already exists - {}'.format(directory))
        return False
    return True

def write_xml_files(parameter_sets, template, output_directory):
    for run in parameter_sets:
        xml_filename = os.path.join(output_directory, run + '.xml')
        parameters = {k : v for k, v in zip(header, parameter_sets[run])}
        with open(xml_filename, 'w') as xml_file:
            xml_file.write(template.format(**parameters))

def write_xml_batches(parameter_sets, template, base_output_directory, batch_size):
    keys = parameter_sets.keys()
    for i in range(0, len(parameter_sets), batch_size):
        subset_keys = list(keys)[i:i+batch_size]
        batch_parameter_sets = {j : parameter_sets[j] for j in subset_keys}
        batch_directory = os.join(base_output_directory, 'batch' + str(i // batch_size))
        if not try_make_directory(batch_directory):
            return
        write_xml_files(batch_parameter_sets, template, batch_directory)

def generate_runs(parameters_filename, template_filename, weight_cutoff, run_set_name, batch_size=0):
    rows = read_parameters(parameters_filename)
    template = read_template(template_filename)
    parameter_sets = filter_rows_by_weight(rows, weight_cutoff)
        
    generate_message = 'Generating XML files for the top {:g}% of parameter sets by weight ({} files)...'
    print(generate_message.format(weight_cutoff * 100, len(parameter_sets)), end='')

    base_output_directory = os.path.join(os.path.dirname(parameters_filename), run_set_name)
    if not try_make_directory(base_output_directory):
        return

    if batch_size == 0:
        write_xml_files(parameter_sets, template, base_output_directory)
    else:
        write_xml_batches(parameter_sets, template, base_output_directory, batch_size)

    print('done.')
    print('XMLs can be found in the folder {}.'.format(base_output_directory))

def run(args):
    if len(args) < 4 or len(args) > 6:
        print('Usage: generate_runs.py [batch=0] [weight_cutoff=0.9] parameters_file template_file run_set_name')
        return
    args.pop(0)
    batch_size = 0
    weight_cutoff = 0.9
    while len(args) > 3:
        arg = args.pop(0)
        if arg.startswith('batch='):
            batch_size = int(arg[6:])
        elif arg.startswith('weight_cutoff='):
            weight_cutoff = float(arg[15:])
        else:
            print('unknown argument {} ignored'.format(arg))
    generate_runs(args[0], args[1], weight_cutoff, args[2], batch_size)

if __name__ == '__main__':
    run(sys.argv)
