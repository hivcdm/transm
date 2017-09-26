# analyse_batches.py end_year target_dir post_calib_file output_file
#
# end_year:   Last year of analysis
# directory:  Directory with batch files
# post_calib: Post Calibration file with batch information
# out:        Output file

import collections # for OrderedDict
import openpyxl # for creating xlsx
import os
import zipfile
import sys
import numpy

supported_versions = ['3.6', '3.7']

# Original Outputs
tabular_outputs = ['Infections', 'Averted', 'Cost-Undiscounted', 'Cost-Discounted',
                   'LMs-Undiscounted', 'LMs-Discounted']
cepac_outputs = ['Cascade']
infection_outputs = ['Prevalence', 'Male-Prevalence', 'Female-Prevalence', 'Incidence', 'Male-Incidence', 'Female-Incidence']

# Extra Health State Outputs
health_state_outputs = ['Pop', 'SA', 'CEPAC-HIV+']

# CEPAC
ident_state_outputs = ['IdentHIV+', 'UnidentHIV+', 'Dead']
ART_state_outputs = ['Off-ART', 'On-ART']
off_ART_state_outputs = ['LTFU', 'Waiting-or-Not-Eligible']
on_ART_state_outputs = ['Suppressed', 'Partially-Suppressed', 'Failed-ART']
# CDM
eligible_state_outputs = ['Eligible-for-Access', 'Accessing-Treatment',
                          'Eligible-for-ART', 'Receiving-ART']

# TabularFile reads a text file composed of lines of data.
# Each line of data is composed of an equal number of elements separated by
# a separator character, usually <tab>.
# For accessing data, each line after the header is keyed based on the
# provided index_column parameter. If the first column is months which range
# from 0 to 1200, we can use this to select rows corresponding their month.
class TabularFile:
    # filename is the file to read
    # num_header_rows is the number of lines to skip at the beginning
    # index_column is the column containing keys that can be used to find rows
    # column_separator is the token separating elements in a row
    def __init__(self, file, num_header_rows=1, index_column=0, column_separator='\t'):
        self.rows = {}
        for i, row in enumerate(file):
            if i < num_header_rows: continue
            if isinstance(row, bytes):
                row = row.decode('utf8')
            split = row.rstrip().split(column_separator)
            row_index = split.pop(index_column)
            if row_index in self.rows:
                raise Exception('duplicate key for row {}'.format(i))
            if row_index == '':
                return
            self.rows[row_index] = split

    def has_row(self, row):
        return row in self.rows

    # return a string corresponding to the given row and column_index
    def get_cell(self, row, column_index):
        return self.rows[row][column_index]

    # return a float corresponding to the given row and column_index
    def get_float(self, row, column_index):
        return float(self.get_cell(row, column_index))

    # return an int corresponding to the given row and column_index
    def get_int(self, row, column_index):
        return int(self.get_cell(row, column_index))

# CepacOutFile reads a .out file created in CEPAC and provides access to monthly
# data in the form of a multidimensional array (rows are separated by <newline>
# and columns are separated by <tab>).
class CepacOutFile:
    def __init__(self, _file, target_month):
        self.month_data = []
        in_target_month = False
        for i, row in enumerate(_file):
            if isinstance(row, bytes):
                row = row.decode('utf8')
            if not in_target_month:
                if row.rstrip() != 'COHORT SUMMARY FOR MONTH {}'.format(target_month):
                    continue
                in_target_month = True
            elif row.rstrip() == 'COHORT SUMMARY FOR MONTH {}'.format(target_month + 1):
                break
            else:
                split = row.rstrip().split('\t')
                self.month_data.append(split)

    # returns the population size during the given month
    def get_pop_size(self):
        return float(self.month_data[1][5])

    # returns the number of peole who died during the given  month
    def get_number_deaths_nonAIDS(self):
        return float(self.month_data[28][18])

    # returns the number of peole who died during the given  month
    def get_number_deaths_chrAIDS(self):
        return float(self.month_data[28][17])

    # returns the number of peole who died during the given  month
    def get_number_deaths_total(self):
        return sum(map(float, self.month_data[28][2:25]))

    # returns the number with HIV during the given month
    def get_number_with_hiv(self):
        return float(self.month_data[1][3]) + int(self.month_data[1][4])

    # returns the number with HIV that were HVL tested during the given month
    def get_number_with_hiv_hvl_tested(self):
        return sum(map(float, self.month_data[20][2:9]))

    # returns the number with HIV that were HIV tested during the given month
    def get_number_with_hiv_identified(self):
        return float(self.month_data[1][4])

    # returns the number with HIV that were not HIV tested during the given month
    def get_number_with_hiv_unidentified(self):
        return float(self.month_data[1][3])

    # returns the number with HIV that were on treatment during the given month
    def get_number_with_hiv_on_ART(self):
        return float(self.month_data[13][9]) + \
            float(self.month_data[14][9]) + \
            float(self.month_data[15][9]) + \
            float(self.month_data[16][9]) + \
            float(self.month_data[17][9]) + \
            float(self.month_data[18][9])


    # returns the number with HIV that were off treatement during the given month
    def get_number_with_hiv_off_ART(self):
        return float(self.month_data[7][9]) + \
            float(self.month_data[8][9]) + \
            float(self.month_data[9][9]) + \
            float(self.month_data[10][9]) + \
            float(self.month_data[11][9]) + \
            float(self.month_data[12][9])

    # returns the number with HIV that were virally supressed during the given month
    def get_number_with_hiv_suppressed(self):
        return float(self.month_data[14][12])

    # returns the number with HIV that are partially suppressed
    def get_number_with_hiv_partially_suppressed(self):
        return float(self.month_data[15][12])

    # returns the number with HIV that failed ART
    def get_number_failed_ART(self):
        return float(self.month_data[16][12])

    # returns the number with HIV that were LTFU during the given month
    def get_number_with_hiv_ltfu(self):
        return sum(map(float, self.month_data[41][2:12]))

# A Page is a page of ordered data in a larger Workbook.
class Page:
    # name is the name of the page
    # parent is an openpyxl.Workbook which will have an openpyxl.Worksheet
    #     corresponding to this page added to it
    # if transposed is true, new data sets will be added across columns instead of
    #     across rows
    def __init__(self, name, parent, transposed=False):
        self.name = name
        self.parent = parent
        self.transposed = transposed

        # we don't want to keep preexisting data
        if name in parent:
            parent.remove_sheet(parent[name])

        self.ws = parent.create_sheet(title=name)

        # keep track of where the last row/column was added so we don't overwrite
        #    old data
        self.current_row = 1
        self.current_column = 1

        # it's more efficient to create once and apply to multiple cells
        self.bold_font = openpyxl.styles.Font(bold=True)

    # starting at corner, move down or right (depending on if transposed or not)
    # adding each element of data to this Page
    def set_data(self, data, corner, cell_format=None):
        row, column = corner
        for element in data:
            self.ws.cell(row=row, column=column).value = element
            if cell_format != None:
                self.ws.cell(row=row, column=column).number_format = cell_format
            if self.transposed:
                row += 1
            else:
                column += 1

    # Add a list of data (can be a list or OrderedDict) with the given label
    # at the current position and then move the position over/down one cell
    # for the next call.
    def add_data(self, label, data, cell_format=None):
        full = [label]
        if isinstance(data, collections.OrderedDict):
            full += [data[i] for i in data]
        else:
            full += data
        self.set_data(full, (self.current_row, self.current_column), cell_format)

        if self.transposed:
            self.current_column += 1
        else:
            self.current_row += 1

    # This should be called before add_data.
    # This sets the labels for columns/rows based on the given list.
    # If a list of lists is given, multiple header rows will be added.
    def set_headers(self, headers):
        if isinstance(headers[0], list):
            for header_row in headers:
                self.add_data(header_row[0], header_row[1:])
        else:
            self.add_data(headers[0], headers[1:])
    ######
    # The rest of the methods are just simple wrappers around Worksheet methods
    ######

    # Make all cells in row bold
    def bold_row(self, row):
        for i in range(1, self.ws.max_row + 1):
            self.ws.cell(row=row, column=i).font = self.bold_font

    # Make all cells in column (should be a letter) bold
    def bold_column(self, column):
        for i in range(1, self.ws.max_row + 1):
            self.ws.cell(row=i, column=column).font = self.bold_font

    # Make cell at given coordinate the given color
    # if bold is true, also make it bold
    def color_cell(self, cell, color, bold=False):
        font = openpyxl.styles.Font(color=color, bold=bold)
        self.ws.cell(cell).font = font

    def freeze(self, cell):
        self.ws.freeze_panes = cell

    def column_width(self, column, width):
        self.ws.column_dimensions[column].width = width

    def row_height(self, row, height):
        self.ws.row_dimensions[row].height = height

    def column_number_format(self, column, number_format):
        for i in range(1, self.ws.max_row + 1):
            self.ws.cell(row=i, column=column).number_format = number_format

    def merge(self, range_string):
        self.ws.merge_cells(range_string)

class Run:
    def __init__(self, xml_filename, full_output, full_outputs):
        self.xml_filename = xml_filename
        self.name = ''
        self.path = ''
        self.is_sane = True
        self.state_msg = str()
        self.full_output = full_output
        self.full_health_state_outputs = full_outputs

        if xml_filename != '':
            self.name = os.path.splitext(os.path.basename(xml_filename))[0]
            self.path = xml_filename

        base_outputs = tabular_outputs + cepac_outputs + infection_outputs
        extended_outputs = []
        if self.full_output:
            extended_outputs.extend(
                health_state_outputs + ident_state_outputs + ART_state_outputs + \
                on_ART_state_outputs + off_ART_state_outputs + \
                eligible_state_outputs)
        self.all_outputs = base_outputs + extended_outputs

        self.stat_names = self.all_outputs
        self.statistics = collections.OrderedDict([(i, collections.OrderedDict())
                                                   for i in self.stat_names])
        self.series = collections.OrderedDict([(i, collections.OrderedDict())
                                               for i in self.stat_names])
        self.median = collections.OrderedDict([(i, collections.OrderedDict())
                                               for i in self.stat_names])
        self.lower_quartile = collections.OrderedDict([(i, collections.OrderedDict())
                                                       for i in self.stat_names])
        self.upper_quartile = collections.OrderedDict([(i, collections.OrderedDict())
                                                       for i in self.stat_names])
        self.differences = collections.OrderedDict([(i, collections.OrderedDict())
                                                    for i in self.stat_names])
        self.percentage_diff = collections.OrderedDict([(i, collections.OrderedDict())
                                                        for i in self.stat_names])
        self.max_year = 0

    def get_max_year(self):
        return self.max_year

    def set_max_year(self, max_year):
        self.max_year = max_year

    # Don't use an XML parser here to reduce execution time.
    # (We're going to do this a lot!)
    def read_xml(self):
        version = None
        duration = 0
        with open(self.xml_filename) as f:
            for line in f:
                if version == None:
                    version = line.strip().split('=')[1][1:-2]
                stripped = line.strip() # take off newline and leading tab
                tag = 'duration'
                open_tag = '<{}>'.format(tag)
                close_tag = '</{}>'.format(tag)
                if stripped.startswith(open_tag) and stripped.endswith(close_tag):
                    duration = int(stripped[len(open_tag):-len(close_tag)])
        #print ("The duration we found is {}".format(duration), file =sys.stderr)
        return version, duration

    def sanity_checks(self, duration, end_year, month_of_1990, zip_file=None):
        infections_filename = '{}-Infections.xls'.format(self.name)
        if zip_file:
            infections = zip_file.open('results/' + infections_filename, 'r')
        directory = os.path.dirname(self.xml_filename)
        results_dir = os.path.join(directory, 'results')
        infections_filename = os.path.join(results_dir, '{}-Infections.xls'.
                                           format(self.name))
        # Sanity checks start here
        if not os.path.isdir(results_dir):
            self.is_sane = False
            self.state_msg = "MISSING RESULTS DIR"
        elif not os.path.isfile(infections_filename):
            self.is_sane = False
            self.state_msg = "MISSING INFECTIONS FILE"
        elif month_of_1990 + (end_year - 1990) * 12 + 11 > duration:
            self.is_sane = False
            self.state_msg = "END MONTH IN YEAR RANGE ({}) > THAN XML DURATION ({})"\
                .format(month_of_1990 + (end_year - 1990) * 12 + 11, duration)
        else:
            try:
                infections = open(infections_filename, 'rt')
            except OSError:
                self.is_sane = False
                self.state_msg = "CANNOT OPEN INFECTIONS FILE"

        if self.is_sane == True:
            run_has_begun = False
            expected_month = 0
            for i, line in enumerate(infections):
                if isinstance(line, bytes):
                    line = line.decode('utf8')
                month = line.split('\t')[0]
                if run_has_begun == True and expected_month == 0:
                    expected_month = 1
                else:
                    if month == 'init':
                        run_has_begun = True
                if run_has_begun == True and expected_month > 0 and self.is_sane == True:
                    if expected_month <= duration:
                        try:
                            if int(float(month)) == expected_month:
                                expected_month += 1
                            else:
                                self.is_sane = False
                                self.state_msg = "ERROR READING INFECTIONS FILE"
                                return
                        except ValueError:
                            self.is_sane = False
                            self.state_msg = "ERROR INFECTIONS FILE DURATION"
                            return
                    elif expected_month == duration + 1:
                        try:
                            int(float(month))
                            self.is_sane = False
                            self.state_msg = "ERROR INFECTIONS FILE DURATION"
                            return
                        except ValueError:
                            expected_month += 1
        if self.is_sane == True and run_has_begun == False:
            self.is_sane = False
            self.state_msg = "ERROR NO init IN INFECTIONS FILE"
        elif self.is_sane == True and expected_month < duration:
            self.is_sane = False
            self.state_msg = "ERROR INFECTIONS FILE DURATION"
        return

    def read_results(self, start_year, end_year, month_of_1990, zip_file):
        self.start_year = start_year
        self.start_month = month_of_1990 + (self.start_year - 1990) * 12
        self.end_year = end_year
        self.end_month = month_of_1990 + (self.end_year - 1990) * 12 + 11
        self.month_of_1990 = month_of_1990

        num_header_rows = {'CE' : 4,
                           'Infections' : 3,
                           'Population' : 2,
                           'ShiftedOutcomes' : 3,
                           'ARTRollout' : 3,
                           'cepac' : 0}
        self.files = {}
        self.cepac_data = {}

        for file_type in num_header_rows:
            filename = '{}-{}.xls'.format(self.name, file_type)
            if file_type == 'cepac':
                filename = '{}.out'.format(self.name)
            full_path = os.path.join('results', filename)
            if zip_file == None:
                full_path = os.path.join(os.path.dirname(self.xml_filename), full_path)
                try:
                    _file = open(full_path, 'r')
                except OSError:
                    self.is_sane = False
                    self.state_msg = "CANNOT OPEN FILE " + full_path
                    break
            else:
                try:
                    _file = zip_file.open('results/' + filename, 'rt')
                except OSError:
                    self.is_sane = False
                    self.state_msg = "CANNOT OPEN ZIP FILE " + filename
                    break

            if file_type == 'cepac':
                 for year in range(1990, self.end_year + 1):
                     first_month = self.month_of_1990 + (year - 1990) * 12
                     month_data = CepacOutFile(_file, first_month)
                     self.cepac_data.update({ year : month_data })
            else:
                self.files[file_type] = TabularFile(_file, num_header_rows[file_type])

        if self.is_sane == True:
            self.extract_infections()
            self.extract_cascade()
            self.extract_costs_and_lms()
            self.extract_prevalence_and_incidence()
            if full_output:
                self.extract_health_states()
                self.extract_sa_proportion()
                self.extract_eligibility_stats_AR()

    def extract_infections(self):
        infections_stats = [
            ('hvl-0', 62),
            ('hvl-1', 63),
            ('hvl-2', 64),
            ('hvl-3', 65),
            ('hvl-4', 66),
            ('hvl-5', 67),
            ('hvl-6', 68),
            ('hvl-primary', 69),
            ('hvl-late-stage', 70)
        ]

        for stat, column in infections_stats:
            self.statistics['Infections'][stat] = 0

        total = 0
        for month in map(str, range(self.start_month, self.end_month + 1)):
            for stat, column in infections_stats:
                num_infections = self.files['Infections'].get_float(month, column)
                self.statistics['Infections'][stat] += num_infections
                total += num_infections

        self.statistics['Infections']['total'] = total

    def extract_cascade(self):
        cepac_file = self.cepac_data[self.end_year]

        hiv_pos = cepac_file.get_number_with_hiv()
        identified = cepac_file.get_number_with_hiv_identified()
        treated =  cepac_file.get_number_with_hiv_on_ART()
        suppressed = cepac_file.get_number_with_hiv_suppressed()
        self.statistics['Cascade']['sa-pop-size'] = cepac_file.get_pop_size()
        self.statistics['Cascade']['num-positive'] = hiv_pos
        self.statistics['Cascade']['num-hiv-hiv-tested'] = identified
        self.statistics['Cascade']['num-hiv-treated'] = treated
        self.statistics['Cascade']['num-hiv-supressed'] = suppressed
        self.statistics['Cascade']['num-hiv-hvl-tested'] = cepac_file.get_number_with_hiv_hvl_tested()
        self.statistics['Cascade']['num-hiv-ltfu'] = cepac_file.get_number_with_hiv_ltfu()

        percent_tested = identified / hiv_pos
        self.statistics['Cascade']['percent-tested'] = percent_tested
        percent_treated = treated / identified
        self.statistics['Cascade']['percent-treated'] = percent_treated
        percent_suppressed = suppressed / treated
        self.statistics['Cascade']['percent-suppressed'] = percent_suppressed
        self.statistics['Cascade']['total-percent-suppressed'] = \
                            percent_tested * percent_treated * percent_suppressed

    # Calculated for health states from the 'cepac.out' file
    def extract_health_states(self):
        for i, year in enumerate(self.cepac_data):
            num_with_hiv =  self.cepac_data[year].get_number_with_hiv()
            self.statistics['CEPAC-HIV+'][year] = num_with_hiv

            if num_with_hiv == 0:
                # don't divide by zero
                continue

            # Dead
            deaths = self.cepac_data[year].get_number_deaths_total()
            deaths_nonAIDS = self.cepac_data[year].get_number_deaths_nonAIDS()
            self.statistics['Dead'][year] = (deaths - deaths_nonAIDS) / num_with_hiv

            # Untested | Tested
            identified = self.cepac_data[year].get_number_with_hiv_identified()
            unidentified =  self.cepac_data[year].get_number_with_hiv_unidentified()
            self.statistics['IdentHIV+'][year] = identified / num_with_hiv
            self.statistics['UnidentHIV+'][year] = unidentified / num_with_hiv

            # On-ART | Off-ART | LTFU
            num_on_ART = self.cepac_data[year].get_number_with_hiv_on_ART()
            num_off_ART = self.cepac_data[year].get_number_with_hiv_off_ART()
            num_ltfu = self.cepac_data[year].get_number_with_hiv_ltfu()
            self.statistics['On-ART'][year] = num_on_ART / num_with_hiv
            self.statistics['Off-ART'][year] = num_off_ART / num_with_hiv
            self.statistics['LTFU'][year] = num_ltfu / num_with_hiv

            # On_ART | Off_ART
            num_on_ART = self.cepac_data[year].get_number_with_hiv_on_ART()
            num_off_ART = self.cepac_data[year].get_number_with_hiv_off_ART()
            self.statistics['On-ART'][year] = num_on_ART / num_with_hiv
            self.statistics['Off-ART'][year] = num_off_ART / num_with_hiv

            # On-ART Subsets: Suppressed | Partially-Suppressed | Failed-ART
            suppressed = self.cepac_data[year].get_number_with_hiv_suppressed()
            partially_suppressed = self.cepac_data[year].\
                                   get_number_with_hiv_partially_suppressed()
            failed_ART = self.cepac_data[year].get_number_failed_ART()
            self.statistics['Suppressed'][year] = suppressed / num_with_hiv
            self.statistics['Partially-Suppressed'][year] = partially_suppressed / num_with_hiv
            self.statistics['Failed-ART'][year] = failed_ART / num_with_hiv

            # Off_ART Subsets : LTFU | Waiting-or-Not-Eligible
            num_ltfu = self.cepac_data[year].get_number_with_hiv_ltfu()
            num_waiting_or_ne = identified - num_on_ART
            self.statistics['LTFU'][year] = num_ltfu / num_with_hiv
            self.statistics['Waiting-or-Not-Eligible'][year] = num_waiting_or_ne / num_with_hiv

    # Calculated for each year from the 'ARTRollout' file
    def extract_eligibility_stats_AR(self):
        year = 1990
        while self.files['ShiftedOutcomes'].has_row(str(year)):
            year += 1
        self.max_year = year - 1

        # Number with HIV from Infection file
        currently_infected_column = 2

        # ART Data from ARTRollout file
        male_eligible_column = 42
        female_eligible_column = 43
        male_accessing_column = 76
        female_accessing_column = 77
        male_waiting_column = 110
        female_waiting_column = 111
        male_receiving_column = 144
        female_receiving_column = 145

        for year in range(1990, min(self.end_year, self.max_year) + 1):
            first_month = self.month_of_1990 + (year - 1990) * 12
            first_month += 1 # add one month to sync with cepac output

            num_with_hiv =  self.cepac_data[year].get_number_with_hiv()
            if num_with_hiv == 0:
                continue

            male_eligible = self.files['ARTRollout'].get_int(
                str(first_month), male_eligible_column)
            female_eligible = self.files['ARTRollout'].get_int(
                str(first_month), female_eligible_column)
            eligible_for_access = male_eligible + female_eligible

            male_accessing = self.files['ARTRollout'].get_int(
                str(first_month), male_accessing_column)
            female_accessing = self.files['ARTRollout'].get_int(
                str(first_month), female_accessing_column)
            accessing_treatment = male_accessing + female_accessing

            male_waiting = self.files['ARTRollout'].get_int(
                str(first_month), male_waiting_column)
            female_waiting = self.files['ARTRollout'].get_int(
                str(first_month), female_waiting_column)
            eligible_for_art = male_waiting + female_waiting

            male_receiving = self.files['ARTRollout'].get_int(
                str(first_month), male_receiving_column)
            female_receiving = self.files['ARTRollout'].get_int(
                str(first_month), female_receiving_column)
            receiving_art = male_receiving + female_receiving

            self.statistics['Eligible-for-Access'][year] = eligible_for_access / num_with_hiv
            self.statistics['Accessing-Treatment'][year] = accessing_treatment / num_with_hiv
            self.statistics['Eligible-for-ART'][year] = eligible_for_art / num_with_hiv
            self.statistics['Receiving-ART'][year] = receiving_art / num_with_hiv

    # Calculated for each year by summing values in the 'Infections' file
    def extract_sa_proportion(self):
        year = 1990
        while self.files['ShiftedOutcomes'].has_row(str(year)):
            year += 1
        self.max_year = year - 1

        prev_column = 2
        pop_column = 3
        sa_column = 5

        for year in range(1990, min(self.end_year, self.max_year) + 1):
            first_month = self.month_of_1990 + (year - 1990) * 12

            yearly_pop = self.files['Infections'].get_int(str(first_month), pop_column)
            yearly_sa = self.files['Infections'].get_int(str(first_month), sa_column)
            yearly_prev = self.files['Infections'].get_int(str(first_month), prev_column)

            self.statistics['Pop'][year] = yearly_pop
            self.statistics['SA'][year] = yearly_sa

    def extract_prevalence_and_incidence(self):
        year = 1990
        while self.files['ShiftedOutcomes'].has_row(str(year)):
            year += 1
        self.max_year = year - 1

        # From Infections file
        sa_column = 5
        currently_infected_column = 2
        newly_infected_column = 0
        prevalent_male_column = 17
        prevalent_female_column = 18
        incident_male_column = 80
        incident_female_column = 81

        # From Population file
        male_sa_column = 9
        female_sa_column = 10

        for year in range(1990, min(self.end_year, self.max_year) + 1):
            first_month = self.month_of_1990 + (year - 1990) * 12
            monthly_sa = 0
            monthly_sa_male = 0
            monthly_sa_female = 0
            monthly_prevalent = 0
            monthly_prevalent_male = 0
            monthly_prevalent_female = 0
            monthly_incident = 0
            monthly_incident_male = 0
            monthly_incident_female = 0
            yearly_prevalence = 0
            yearly_prevalence_male = 0
            yearly_prevalence_female = 0
            yearly_incidence = 0
            yearly_incidence_male = 0
            yearly_incidence_female = 0

            for month_in_year in range(first_month, first_month + 12):
                monthly_sa = self.files['Infections'].get_int(str(month_in_year), \
                                                                     sa_column)
                monthly_sa_male = self.files['Population'].get_int(str(month_in_year), \
                                                                     male_sa_column)
                monthly_sa_female = self.files['Population'].get_int(str(month_in_year), \
                                                                     female_sa_column)
                monthly_prevalent = self.files['Infections'].get_int(str(month_in_year), \
                                                                     currently_infected_column)
                monthly_prevalent_male = self.files['Infections'].get_int(str(month_in_year), \
                                                                     prevalent_male_column)
                monthly_prevalent_female = self.files['Infections'].get_int(str(month_in_year), \
                                                                     prevalent_female_column)
                monthly_incident = self.files['Infections'].get_int(str(month_in_year), \
                                                                    newly_infected_column)
                monthly_incident_male = self.files['Infections'].get_int(str(month_in_year), \
                                                                    incident_male_column)
                monthly_incident_female = self.files['Infections'].get_int(str(month_in_year), \
                                                                    incident_female_column)

                yearly_incidence += monthly_incident / (monthly_sa - monthly_prevalent)
                yearly_incidence_male += monthly_incident_male / (monthly_sa_male - monthly_prevalent_male)
                yearly_incidence_female += monthly_incident_female / (monthly_sa_female - monthly_prevalent_female)
                if month_in_year == int(first_month) and monthly_sa > 0:
                    yearly_prevalence = monthly_prevalent / monthly_sa
                    yearly_prevalence_male = monthly_prevalent_male / monthly_sa_male
                    yearly_prevalence_female = monthly_prevalent_female / monthly_sa_female

            self.statistics['Prevalence'][year] = yearly_prevalence
            self.statistics['Male-Prevalence'][year] = yearly_prevalence_male
            self.statistics['Female-Prevalence'][year] = yearly_prevalence_female

            self.statistics['Incidence'][year] = yearly_incidence
            self.statistics['Male-Incidence'][year] = yearly_incidence_male
            self.statistics['Female-Incidence'][year] = yearly_incidence_female

    def extract_costs_and_lms(self):
        cost_stats = [
            ('total', 16),
            ('condoms', 19),
            ('circumcision', 18),
            ('hiv-testing', 29),
            ('hvl-testing', 26),
            ('treatment', 30)
        ]
        lm_stats = [
            ('negative', 1),
            ('acute-observed', 2),
            ('acute-unobserved', 3),
            ('chronic-observed', 4),
            ('chronic-unobserved', 5),
            ('late-stage-observed', 6),
            ('late-stage-unobserved', 7)
        ]

        discounted_offset = 62

        self.statistics['LMs-Undiscounted']['total'] = 0
        self.statistics['LMs-Discounted']['total'] = 0

        for stat, column in cost_stats:
            self.statistics['Cost-Undiscounted'][stat] = 0
            self.statistics['Cost-Discounted'][stat] = 0

        for stat, column in lm_stats:
            self.statistics['LMs-Undiscounted'][stat] = 0
            self.statistics['LMs-Discounted'][stat] = 0

        for month in map(str, range(self.start_month, self.end_month + 1)):
            for stat, column in cost_stats:
                self.statistics['Cost-Undiscounted'][stat] += float(self.files['CE'].get_cell(month, column))
                self.statistics['Cost-Discounted'][stat] += float(self.files['CE'].get_cell(month, column + discounted_offset))

            for stat, column in lm_stats:
                self.statistics['LMs-Undiscounted'][stat] += float(self.files['CE'].get_cell(month, column))
                self.statistics['LMs-Discounted'][stat] += float(self.files['CE'].get_cell(month, column + discounted_offset))

                self.statistics['LMs-Undiscounted']['total'] += float(self.files['CE'].get_cell(month, column))
                self.statistics['LMs-Discounted']['total'] += float(self.files['CE'].get_cell(month, column + discounted_offset))

class RunSet:
    def __init__(self, directory, full_output, full_outputs):
        self.directory = directory
        self.name = os.path.basename(directory)
        self.is_sane = True
        self.state_msg = str()
        self.full_output = full_output
        self.full_health_state_outputs = full_outputs

    def load_runs(self, year_range_min, year_range_max, get_month_of_1990):
        subdirs = [os.path.join(self.directory, i) for i in os.listdir(self.directory)
                   if os.path.isdir(os.path.join(self.directory, i))]
        batchdirs = [i for i in subdirs if os.path.basename(i).startswith('batch')]
        if len(batchdirs) == 0:
            batchdirs = [self.directory]
        for batchdir in batchdirs:
            for file in os.listdir(batchdir):
                if os.path.splitext(file)[1] == '.xml':
                    run = Run(os.path.join(batchdir, file), self.full_output,
                              self.full_health_state_outputs)
                    zip_filename = os.path.join(batchdir, 'results.zip')
                    zip_file = None
                    if os.path.isfile(zip_filename):
                        try:
                            zip_file = zipfile.ZipFile(zip_filename)
                        except zipfile.BadZipfile:
                            run.is_sane = False
                            run.state_msg = "ERROR OPENING ZIP FILE"
                            continue
                    try:
                        version, duration = run.read_xml()
                    except Exception:
                        run.is_sane = False
                        run.state_msg = "ERROR READING XML"
                        continue
                    if version not in supported_versions:
                        self.is_sane = False
                        self.state_msg = "WRONG XML VERSION"
                        continue
                    else:
                        month_of_1990 = get_month_of_1990(run.name)
                        # to save time -- create a file called '.skip_sanity_check'
                        if not (os.path.isfile(".skip_sanity_check")) :
                                run.sanity_checks(duration, year_range_max, month_of_1990, zip_file)

                        if run.is_sane == True:
                            run.read_results(year_range_min, year_range_max, month_of_1990, zip_file)
                            print('\t{} Sanity checks passed, read result'.format(run.name))

                        else:
                            print("\t{} Sanity checks failed: {}".format(run.name, run.state_msg),
                                  file=sys.stderr)

                    yield run

class Summary:
    def __init__(self, directory, post_calib_filename, full_output, \
                 weight_cutoff, year_range, year_comparison, status_quo):

        self.directory = directory
        self.full_output = full_output
        self.read_calibration_data(post_calib_filename, weight_cutoff)
        self.year_range_min, self.year_range_max = year_range
        self.comparison_base_year = year_comparison

        if self.comparison_base_year < self.year_range_min or \
           self.comparison_base_year > self.year_range_max:
            print("Incorrect year comparison ({}) vs year analysis ({}-{}) range. " \
                  "Please change the parameters and re-run the script." \
                  .format(self.comparison_base_year, self.year_range_min, self.year_range_max))
            exit()

        # must construct headers before running status quo stats
        self.construct_headers()

        self.status_quo = status_quo
        self.status_quo_found = 0
        self.status_quo_is_sane = True
        self.excluded_run_set = []
        self.processed_run_set = []
        self.skipped_run_set = dict()
        self.status_quo_stats = Run('', self.full_output, self.full_health_state_outputs)

    def construct_headers(self):
        self.headers = [
            ('Infections', [
                [
                    'Years: {} to {}'.format(self.year_range_min, self.year_range_max),
                    'Infections by HVL (of Infector)'
                ],
                [
                    '',
                    'HVL 0-20',
                    'HVL 21-500',
                    'HVL 501-3000',
                    'HVL 3001-10000',
                    'HVL 10000-30000',
                    'HVL 30001-100000',
                    'HVL 100000+',
                    'HVL Primary',
                    'HVL Late Stage',
                    'Total',
                ]
            ]),
            ('Averted', [
                [
                    'Years: {} to {}'.format(self.year_range_min, self.year_range_max),
                    'Infections by HVL (of Infector)'
                ],
                [
                    '',
                    'HVL 0-20',
                    'HVL 21-500',
                    'HVL 501-3000',
                    'HVL 3001-10000',
                    'HVL 10000-30000',
                    'HVL 30001-100000',
                    'HVL 100000+',
                    'HVL Primary',
                    'HVL Late Stage',
                    'Total',
                    'Primary %',
                    'Total %'
                ]
            ]),
            ('Cascade', [
                'Year {}'.format(self.year_range_max),
                'SA Pop Size', 'Number With HIV',
                'Number With HIV Tested', 'Number with HIV Treated',
                'Number With HIV Suppressed',
                'Number With HIV HVL Tested' ,'Number With HIV LTFU',
                '% Tested', '% Treated', '% Suppressed', 'Total % Suppressed'
            ]),
            ('Cost-Undiscounted', [
                'Years: {} to {}'.format(self.year_range_min, self.year_range_max),
                'Total',
                'Condoms',
                'Circumcision',
                'HIV Testing',
                'HVL Testing',
                'Treatment'
            ]),
            ('Cost-Discounted', [
                'Years: {} to {}'.format(self.year_range_min, self.year_range_max),
                'Total',
                'Condoms',
                'Circumcision',
                'HIV Testing',
                'HVL Testing',
                'Treatment'
            ]),
            ('LMs-Undiscounted', [
                'Years: {} to {}'.format(self.year_range_min, self.year_range_max),
                'Total',
                'Negative',
                'Acute (Observed)',
                'Acute (Unobserved)',
                'Chronic (Observed',
                'Chronic (Unobserved)',
                'Late-Stage (Observed)',
                'Late-Stage (Unobserved)'
            ]),
            ('LMs-Discounted', [
                'Years: {} to {}'.format(self.year_range_min, self.year_range_max),
                'Total',
                'Negative',
                'Acute (Observed)',
                'Acute (Unobserved)',
                'Chronic (Observed',
                'Chronic (Unobserved)',
                'Late-Stage (Observed)',
                'Late-Stage (Unobserved)'
            ])
        ]

        self.full_health_state_outputs = []
        if self.full_output:
            self.full_health_state_outputs.extend(
                health_state_outputs + ident_state_outputs + ART_state_outputs + \
                on_ART_state_outputs + off_ART_state_outputs + \
                eligible_state_outputs)
        self.yearly_outputs = infection_outputs + self.full_health_state_outputs

        for tab in self.yearly_outputs:
            self.headers.extend([(tab, [''])])

    def read_calibration_data(self, post_calib_filename, weight_cutoff):
        print('Reading weights and months of 1990 from {}... '.format(post_calib_filename), end='')
        self.months_of_1990 = {}
        self.weights = {}

        with open(post_calib_filename) as post_calib:
            file = TabularFile(post_calib)

        all_weights = [(r, float(file.rows[r][3])) for r in file.rows]
        ordered_by_weight = reversed(sorted(all_weights, key=lambda t: t[1]))
        self.cumulative_weight = 0
        self.num_runs = 0

        for run, weight in ordered_by_weight:
            self.weights[run] = weight
            self.months_of_1990[run] = int(file.rows[run][0])

            self.cumulative_weight += weight
            self.num_runs += 1

            if weight_cutoff != None and self.cumulative_weight > weight_cutoff:
                break

        print('done.')
        print('Found {} runs comprising {} cumulative weight.'.format(len(self.weights), self.cumulative_weight))
        print()

    def match_name(self, name, options, exact_only):
        '''
        WARNING: This function is dangerously dependent on the formatting of the name of the runs.
        Right now it works, but if for some reason we forget the underscore in the name
        (ex. Cal_Batch23_4224) we will run into the prefix issue again. Not good at all. G
        '''
        if 'standard' in name: name = 'Cal_Batch1_7602'
        if exact_only:
            if name not in options:
                raise Exception('exact run name not found in post calib file: {}'.format(name))
            else:
                #print ("Exact match found for {}".format(name), file=sys.stderr)
                return options[name]
        possible = [i for i in options if name.startswith(i)]
        for i in possible:
            suffix = name[len(i):]
            #print ("SUFFIX IS: {}".format(suffix), file=sys.stderr)
            if len(suffix) == 0:
                #print("ZERO-SUFFIX match is {}, name is {}.".format(i, name), file=sys.stderr)
                return options[i]
            if suffix[0] not in '0123456789':
                #print("SUFFIX match is {}, name is {}.".format(i, name), file=sys.stderr)
                return options[i]
        raise Exception('matching run name not found in post calib file: {}'.format(name))

    def get_weight(self, run_name, exact_only):
        return self.match_name(run_name, self.weights, exact_only)

    def get_month_of_1990(self, run_name, exact_only):
        return self.match_name(run_name, self.months_of_1990, exact_only)

    def find_run_sets(self, directory, allow_non_batch=False):
        set_directories = []
        #print('looking in {}'.format(directory))

        for i in os.listdir(directory):
            full_path = os.path.join(directory, i)
            if allow_non_batch:
                if os.path.isfile(full_path): continue
                if os.path.splitext(i)[1] == '.xml':
                    return [directory]
            else:
                if not os.path.isdir(full_path): continue
                if i == 'batch0':
                    #print('found directory containing batch0 {}'.format(directory))
                    return [directory]

        for i in os.listdir(directory):
            full_path = os.path.join(directory, i)
            if not os.path.isdir(full_path): continue
            set_directories.extend(self.find_run_sets(full_path, allow_non_batch))

        return set_directories

    def wquantile (self, values, weights, quantile):
        values = numpy.array(values)
        weights = numpy.array(weights)
        assert quantile <= 1 and quantile >=0, 'quantiles should be in [0, 1]'
        sorter = numpy.argsort(values)
        values_sorted = values[sorter]
        weights_sorted = weights[sorter]
        cumulative_weights = numpy.cumsum(weights_sorted)
        normalized_quantile = quantile * numpy.sum(weights)
        quantile_value = None
        quantile_position = None
        i= 0
        for value, cum_weight, position in zip (values_sorted, cumulative_weights, sorter):
            #print("iter {}, value is {}, cum_weight is {}, position is {}.".format(i, value, cum_weight, position), file = sys.stderr)
            i += 1
            if cum_weight >= normalized_quantile:
                quantile_value = value
                quantile_position = position
                #print("We found the median: iter {}, value is {}, cum_weight is {}, position is {}.".format(i, value, cum_weight, position), file = sys.stderr)
                break
        return quantile_value, quantile_position

    def calculate_stats(self, run_set):
        '''
        WARNING: The weights in this function are calculated for the standard full batch (typically 90%
        of the calibration run), regardless of the actual input: this will output incorrect values if we
        use just a subset of it. In that case, the function can print a warning in the end about all runs
        not being present, but it is not clear to the user that the results are incorrect. G
        '''
        stats = Run('', self.full_output, self.full_health_state_outputs)
        stats.max_year = None
        total_weight = 0
        weights = []
        processed_runs = []
        skipped_runs = dict()

        num_runs = 0

        for run in run_set.load_runs(self.year_range_min, self.year_range_max, lambda r: self.get_month_of_1990(r, False)):
            if run.is_sane == False:
                skipped_runs[run.name] = (run.path, run.state_msg)
            else:
                weight = self.get_weight(run.name, False)
                total_weight += weight
                weight = weight / self.cumulative_weight
                weights.append(weight)

                num_runs += 1
                processed_runs.append(run.name)

                if stats.max_year == None:
                    stats.max_year = min(run.max_year, self.year_range_max)
                else:
                    stats.max_year = min(run.max_year, stats.max_year)

                for stat_category in run.statistics:
                    for stat in run.statistics[stat_category]:
                        if stat not in stats.statistics[stat_category]:
                            stats.statistics[stat_category][stat] = 0
                            stats.series[stat_category][stat] = []

                        run_value = run.statistics[stat_category][stat]

                        if run_value == None:
                            stats.statistics[stat_category][stat] = None
                            stats.series[stat_category][stat] = None
                        else:
                            stats.statistics[stat_category][stat] += weight * run_value
                            stats.series[stat_category][stat].append(run_value)

        # If we have nothing we better just return
        if len(processed_runs) == 0:
            return stats, processed_runs, skipped_runs

        # censor years not present in every run in the set
        if stats.max_year != None:
            for tab in self.yearly_outputs:
                while stats.max_year + 1 in stats.statistics[tab]:
                    stats.statistics[tab].popitem()

                while stats.max_year + 1 in stats.series[tab]:
                    stats.series[tab].popitem()

        for stat_category in stats.series:
            for stat in stats.series[stat_category]:
                if stats.series[stat_category][stat] == None:
                    continue
                else:
                    stats.median[stat_category][stat], median_position = self.wquantile(stats.series[stat_category][stat], weights, 0.5)
                    stats.lower_quartile[stat_category][stat], q1_position = self.wquantile(stats.series[stat_category][stat], weights, 0.25)
                    stats.upper_quartile[stat_category][stat], q3_position = self.wquantile(stats.series[stat_category][stat], weights, 0.75)

        for stat_category in stats.series:
            if stat_category in self.yearly_outputs:
                year_range = "{} to {}".format(self.year_range_min, self.year_range_max)
                stats.differences[stat_category][year_range] = numpy.asarray(stats.series[stat_category][stats.max_year]) - numpy.asarray(stats.series[stat_category][self.comparison_base_year])
                stats.percentage_diff[stat_category][year_range] = stats.differences[stat_category][year_range] / stats.series[stat_category][self.comparison_base_year]
                stats.statistics[stat_category][year_range] = numpy.average(stats.differences[stat_category][year_range], weights=weights)
                stats.median[stat_category][year_range] = self.wquantile(stats.differences[stat_category][year_range], weights, 0.5)[0]
                stats.lower_quartile[stat_category][year_range] =  self.wquantile(stats.differences[stat_category][year_range], weights, 0.25)[0]
                stats.upper_quartile[stat_category][year_range] =  self.wquantile(stats.differences[stat_category][year_range], weights, 0.75)[0]

                stats.statistics[stat_category]["%"] = stats.statistics[stat_category][year_range] / stats.statistics[stat_category][self.comparison_base_year]
                stats.median[stat_category]["%"] = self.wquantile(stats.percentage_diff[stat_category][year_range], weights, 0.5)[0]

                stats.lower_quartile[stat_category]["%"] = self.wquantile(stats.percentage_diff[stat_category][year_range], weights, 0.25)[0]
                stats.upper_quartile[stat_category]["%"] = self.wquantile(stats.percentage_diff[stat_category][year_range], weights, 0.75)[0]

        if run_set.name == self.status_quo:
            self.status_quo_stats = stats
            self.status_quo_is_sane = run_set.is_sane
        if self.status_quo_found == 1 and self.status_quo_is_sane == True and len(processed_runs) == self.num_runs:
            for stat in stats.statistics['Infections']:
                try:
                    stats.differences['Averted'][stat] = numpy.asarray(self.status_quo_stats.series['Infections'][stat]) - numpy.asarray(stats.series['Infections'][stat])
                    stats.statistics['Averted'][stat] = numpy.average(stats.differences['Averted'][stat], weights=weights)
                    stats.percentage_diff['Averted'][stat] = stats.differences['Averted'][stat] / self.status_quo_stats.series['Infections'][stat]
                    stats.median['Averted'][stat], median_position = self.wquantile(stats.differences['Averted'][stat], weights, 0.5)
                    stats.lower_quartile['Averted'][stat], q1_position = self.wquantile(stats.differences['Averted'][stat], weights, 0.25)
                    stats.upper_quartile['Averted'][stat], q3_position = self.wquantile(stats.differences['Averted'][stat], weights, 0.75)
                    if stat == 'hvl-primary':
                        stats.statistics['Averted']['primary-percent'] = stats.statistics['Averted']['hvl-primary'] / self.status_quo_stats.statistics['Infections']['hvl-primary']
                        stats.median['Averted']['primary-percent'], median_position = self.wquantile(stats.percentage_diff['Averted'][stat], weights, 0.5)
                        stats.lower_quartile['Averted']['primary-percent'], q1_position = self.wquantile(stats.percentage_diff['Averted'][stat], weights, 0.25)
                        stats.upper_quartile['Averted']['primary-percent'], q3_position = self.wquantile(stats.percentage_diff['Averted'][stat], weights, 0.75)

                    elif stat == 'total':
                        stats.statistics['Averted']['total-percent'] = stats.statistics['Averted']['total'] / self.status_quo_stats.statistics['Infections']['total']
                        stats.median['Averted']['total-percent'], median_position = self.wquantile(stats.percentage_diff['Averted'][stat], weights, 0.5)
                        stats.lower_quartile['Averted']['total-percent'], q1_position = self.wquantile(stats.percentage_diff['Averted'][stat], weights, 0.25)
                        stats.upper_quartile['Averted']['total-percent'], q3_position = self.wquantile(stats.percentage_diff['Averted'][stat], weights, 0.75)
                except ValueError as e:
                    print ("Couldn't calculate Averted Infections for runset {}. The skipped runs are {}, the processed ones {}, the length of the Infections array is {}.".format(run_set.name, len(skipped_runs), len(processed_runs), len(stats.series['Infections'][stat])), file = sys.stderr)
                    print (e)
        return stats, processed_runs, skipped_runs

    def apply_formatting(self, wb):
        for page, header in self.headers:
            self.pages[page].bold_row(1)
            self.pages[page].bold_column(1)
            self.pages[page].color_cell('A1', 'FF0000', True)
            self.pages[page].freeze('B2')
            if page not in self.yearly_outputs:
                self.pages[page].column_width('A', 25)

            if page in ['Infections', 'Averted']:
                self.pages[page].row_height(1, 60)
                self.pages[page].row_height(2, 60)
                self.pages[page].merge('A1:A2')
                self.pages[page].bold_row(2)
                self.pages[page].freeze('B3')
            elif page in ['Cascade']:
                self.pages[page].row_height(1, 60)
                for column in ['C', 'D', 'E', 'F', 'G', 'H']:
                    self.pages[page].column_width(column, 25)
                for column in range(9,15):
                    self.pages[page].column_number_format(column, '0.00%')
            elif page in ['LMs-Undiscounted','LMs-Discounted']:
                self.pages[page].row_height(1, 45)
            elif page in self.full_health_state_outputs and \
                 page not in health_state_outputs:
                for column in range(2,100):
                    self.pages[page].column_number_format(column, '0.00%')

    # Output values for the excel tabs that have per-year values
    def write_year_trend_tab_headers(self, max_year):
        if max_year > 1989:
            for year in range(1990, max_year + 1):
                row = 2 + year - 1990
                for tab in self.yearly_outputs:
                    self.pages[tab].ws.cell(column=1,row=row).value = year

            last_row = 2 + max_year + 1 -1990
            cell_value =  "{} to {}".format(self.comparison_base_year, max_year)
            for page in self.yearly_outputs:
                self.pages[page].ws.cell(column=1,row=last_row).value = cell_value

    def summarise(self, out_filename, excludes):
        out_filename = os.path.normpath(out_filename)
        print('Reading results files and writing stats to {}.'.format(out_filename))
        print()

        wb = openpyxl.Workbook()
        wb.remove_sheet(wb['Sheet']) # workbooks contain

        self.pages = {}

        for page_name, header in self.headers:
            transposed = page_name in self.yearly_outputs
            self.pages[page_name] = Page(page_name, wb, transposed)
            self.pages[page_name].set_headers(header)

        max_year = 1989
        logname = out_filename.split('.')[0] + '_error_log.txt'
        logname = os.path.join(self.directory, logname)

        # We touch the file to avoid confusion with other logs
        with open(logname, 'w') as logfile:
            print ("THE FOLLOWING RUNS WERE SKIPPED (NAME REASON):\n", file = logfile)

        found_directories = self.find_run_sets(self.directory)
        index_SQ = 0

        if self.status_quo != "":
            i = 0
            for d in found_directories:
                if os.path.basename(d).endswith(self.status_quo):
                    self.status_quo_found += 1
                    index_SQ = i
                else:
                    i += 1
                    continue
            if self.status_quo_found == 0:
                print ("Status quo runset {} not found. Check the parameters and run the script again.".format(self.status_quo), file = sys.stderr)
                exit()
            elif self.status_quo_found > 1:
                print ("More than one match found for status quo runset {}.\nCheck the parameters and run the script again.".format(self.status_quo), file = sys.stderr)
                exit()
            else:
                # If we have status quo input we need to process it first
                found_directories[0], found_directories[index_SQ] = found_directories[index_SQ], found_directories[0]

        for run_set_directory in found_directories:
            run_set = RunSet(run_set_directory, self.full_output, self.full_health_state_outputs)

            if run_set.name in excludes and run_set.name != self.status_quo:
                print('{} is an excluded set, skipping...'.format(run_set.name), file = sys.stderr)
                print()
                self.excluded_run_set.append(run_set.name)
                continue

            print('Averaging run set {}'.format(run_set.name))
            stats, processed_runs, skipped_runs = self.calculate_stats(run_set)

            for page_name, header in self.headers:
                if page_name == 'Averted' and self.status_quo_found != 1:
                    continue

                number_format = '#,0.00'
                if 'Cost' in page_name:
                    number_format = '$#,0.00'
                elif page_name in infection_outputs:
                    number_format = '0.000000'
                elif page_name in self.full_health_state_outputs and \
                     page_name not in health_state_outputs:
                    number_format = '#%,0.00'

                self.pages[page_name].add_data('AVG ' + run_set.name,
                                               stats.statistics[page_name],
                                               number_format)
                self.pages[page_name].add_data('Q1 ' + run_set.name,
                                               stats.lower_quartile[page_name],
                                               number_format)
                self.pages[page_name].add_data('MED ' + run_set.name,
                                               stats.median[page_name],
                                               number_format)
                self.pages[page_name].add_data('Q3 ' + run_set.name,
                                               stats.upper_quartile[page_name],
                                               number_format)

                if page_name == 'Averted':
                    self.pages[page_name].add_data('Q1 ' + run_set.name, \
                                                   stats.lower_quartile[page_name], number_format)
                    self.pages[page_name].add_data('MED ' + run_set.name, \
                                                   stats.median[page_name], number_format)
                    self.pages[page_name].add_data('Q3 ' + run_set.name, \
                                                   stats.upper_quartile[page_name], number_format)

            # If this is true at this point it means we have missing runs
            missing_runs_names = set()
            if self.num_runs != (len(processed_runs) + len(skipped_runs)):
                run_set.is_sane = False
                processed_runs_names = set()
                skipped_runs_names = set()
                all_run_names = set(self.weights.keys())
                for name in all_run_names:
                    for namepath in processed_runs:
                        if namepath.startswith(name) and namepath[len(name)] not in '0123456789':
                            processed_runs_names.add(name)
                    for namepath in skipped_runs:
                        if namepath.startswith(name) and namepath[len(name)] not in '0123456789':
                            skipped_runs_names.add(name)
                missing_runs_names = all_run_names - set(processed_runs_names) - set(skipped_runs_names)
                print("Missing: {}".format(missing_runs_names))
                print("Skipped: {}".format(skipped_runs_names))
                print("Processed: {}".format(processed_runs_names))
                run_set.state_msg = "HAS {} MISSING RUNS".format(len(missing_runs_names))
                self.skipped_run_set[run_set.name] = run_set.state_msg
                with open(logname, 'a') as logfile:
                    for run_name in missing_runs_names:
                        print(run_set.name + run_name + '\t\t' + "MISSING", file = logfile)
            if len(skipped_runs) > 0:
                if run_set.is_sane == True:
                    run_set.is_sane = False
                    run_set.state_msg = "HAS SKIPPED RUNS"
                self.skipped_run_set[run_set.name] = run_set.state_msg
                with open(logname, 'a') as logfile:
                    for run_name, (path, error) in skipped_runs.items():
                        print(path + '\t\t' + error, file = logfile)
            if run_set.is_sane == False:
                print("{}: {} processed runs, {} skipped runs, {} missing runs.\n"\
                      .format(run_set.name, len(processed_runs), len(skipped_runs),
                              len(missing_runs_names)), file = sys.stderr)
            else:
                try:
                    if stats.get_max_year() > max_year:
                        max_year = stats.get_max_year()
                except TypeError:
                    print ("ERROR: Couldn't read max year!", file = sys.stderr)
                    run_set.is_sane = False
                    run_set.state_msg = "ERROR NO MAX YEAR"
                    self.skipped_run_set[run_set.name] = run_set.state_msg
                    continue
                try:
                    wb.save(out_filename)
                except PermissionError as e:
                    print ("ERROR: Couldn't save excel file!", file = sys.stderr)
                    run_set.is_sane = False
                    run_set.state_msg = "ERROR SAVING EXCEL OUTFILE"
                    self.skipped_run_set[run_set.name] = run_set.state_msg
                    continue
                # If we are here we should be good
                self.processed_run_set.append(run_set.name)
                print ("Summary for {}: All runs completed successfully.\n"\
                       .format(run_set.name), file = sys.stderr)
        print()

        self.write_year_trend_tab_headers(max_year)
        self.apply_formatting(wb)

        try:
            wb.save(out_filename)
        except PermissionError as e:
            print ("FATAL ERROR: CANNOT SAVE EXCEL OUTFILE", file = sys.stderr)
            return

        print ('Done.\n')
        print ("\nRUN SETS PROCESSED SUCCESSFULLY:\n")
        for run_set in self.processed_run_set:
            print (run_set)

        print ("\nRUN SETS EXCLUDED:\n")
        for run_set in self.excluded_run_set:
            print (run_set)

        print ("\nRUN SETS SKIPPED (REASON):\n")
        for run_set, error in self.skipped_run_set.items():
            print (run_set + '\t(' + error + ')')

        print ("\nFINAL SUMMARY:\n{} processed run sets \n{} excluded run sets \n"\
               "{} run sets skipped.\n".format(len(self.processed_run_set),
                                               len(self.excluded_run_set),
                                               len(self.skipped_run_set)))
        print('All stats have been written to {}.\n'.format(out_filename))
        if len(self.skipped_run_set) > 0:
            print ("For details on skipped run sets check {}.\n".format(logname))

def run(end_year, directory, post_calib, out, full_output):
    weight = float(0.9) # Weight cutoff for the calibration file
    year_range = (2014, end_year) # Years of interest for our analysis
    # base year for computing the differences to rank in  quartiles
    year_comparison = 2014
    # run sets to be excluded from our analysis (useful on very large folders)
    excludes = ["85PerSudDecBef2016_50K", "3PerIncBef2016_50K"]
    status_quo = "" #"BaseCase_50K"

    Summary(directory, post_calib, full_output, weight, year_range, year_comparison, \
            status_quo).summarise(out, excludes)


if __name__ == '__main__':
    if len(sys.argv) < 5:
        print ("usage: analyse_batches.py end_year target_dir post_calib_file "\
               "output_file <full_output>", file = sys.stderr)
        exit(1)

    end_year = int(sys.argv[1])
    directory = sys.argv[2]
    post_calib = sys.argv[3]
    out = sys.argv[4]

    full_output = False
    if (len(sys.argv) == 6):
        full_output = bool(sys.argv[5])

    run(end_year, directory, post_calib, out, full_output)
