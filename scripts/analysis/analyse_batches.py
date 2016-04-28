#!/bin/bash
''''which python3 >/dev/null 2>&1 && exec python3 "$0" "$@" # '''
''''which python  >/dev/null 2>&1 && exec python  "$0" "$@" # '''
''''exec echo "Error: I can't find python anywhere! :("	 # '''

import collections # for OrderedDict
import openpyxl # for creating xlsx
import os
import zipfile
import sys
import numpy
#from numpy.lib.function_base import average
#from numpy.lib.function_base import median
#from test.test_Decimal import directory

supported_versions = ['3.6', '3.7']

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
	def __init__(self, file, target_month):
		self.month_data = []
		in_target_month = False
		for i, row in enumerate(file):
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

	# returns the number with HIV during the given month
	def get_number_with_hiv(self):
		return float(self.month_data[1][3]) + int(self.month_data[1][4])

	# returns the number with HIV that were HIV tested during the given month
	def get_number_with_hiv_hiv_tested(self):
		return float(self.month_data[1][4])

	# returns the number with HIV that were on treatment during the given month
	def get_number_with_hiv_treated(self):
		return sum(map(float, [self.month_data[14][12], self.month_data[15][12], self.month_data[16][12]]))

	# returns the number with HIV that were virally supressed during the given month
	def get_number_with_hiv_supressed(self):
		return float(self.month_data[14][12])

	# returns the number with HIV that were HVL tested during the given month
	def get_number_with_hiv_hvl_tested(self):
		return sum(map(float, self.month_data[20][2:9]))

	# returns the number with HIV that were LTFU during the given month
	def get_number_with_hiv_ltfu(self):
		return sum(map(float, self.month_data[41][2:12]))

# A Page is a page of ordered data in a larger Workbook.
class Page:
	# name is the name of the page
	# parent is an openpyxl.Workbook which will have an openpyxl.Worksheet corresponding to this page added to it
	# if transposed is true, new data sets will be added across columns instead of across rows
	def __init__(self, name, parent, transposed=False):
		self.name = name
		self.parent = parent
		self.transposed = transposed

		# we don't want to keep preexisting data
		if name in parent:
			parent.remove_sheet(parent[name])
			
		self.ws = parent.create_sheet(title=name)

		# keep track of where the last row/column was added so we don't overwrite old data
		self.current_row = 1
		self.current_column = 1

		# it's more efficient to create once and apply to multiple cells
		self.bold_font = openpyxl.styles.Font(bold=True)

	# starting at corner, move down or right (depending on if transposed or not)
	# adding each element of data to this Page
	def set_data(self, data, corner, cell_format=None):
		row, column = corner
		#print ("Data is of type {}".format(type(data)), file = sys.stderr)
		for element in data:
			#print ("Element is of type {}, its value is {}".format(type(element), element), file = sys.stderr)
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
	def __init__(self, xml_filename=''):
		self.xml_filename = xml_filename
		self.name = ''
		self.path = ''
		self.is_sane = True
		self.state_msg = str()
		
		if xml_filename != '':
			self.name = os.path.splitext(os.path.basename(xml_filename))[0]
			self.path = xml_filename

		self.stat_names = ['Infections', 'Averted', 'Cascade', 'Prevalence',
						   'Incidence', 'Cost-Undiscounted', 'Cost-Discounted',
						   'LMs-Undiscounted', 'LMs-Discounted']
		self.statistics = collections.OrderedDict([(i, collections.OrderedDict()) for i in self.stat_names])
		self.series = collections.OrderedDict([(i, collections.OrderedDict()) for i in self.stat_names])
		self.median = collections.OrderedDict([(i, collections.OrderedDict()) for i in self.stat_names])
		self.lower_quartile = collections.OrderedDict([(i, collections.OrderedDict()) for i in self.stat_names])
		self.upper_quartile = collections.OrderedDict([(i, collections.OrderedDict()) for i in self.stat_names])
		self.differences = collections.OrderedDict([(i, collections.OrderedDict()) for i in self.stat_names])
		self.max_year = 0

	def get_max_year(self):
		return self.max_year

	def set_max_year(max_year):
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

	def sanity_checks(self, duration,  start_year, end_year, month_of_1990, zip_file=None):
		infections_filename = '{}-Infections.xls'.format(self.name)
		if zip_file:
			infections = zip_file.open('results/' + infections_filename, 'r')
		directory = os.path.dirname(self.xml_filename)
		results_dir = os.path.join(directory, 'results')
		infections_filename = os.path.join(results_dir, '{}-Infections.xls'.format(self.name))
		# Sanity checks start here
		if not os.path.isdir(results_dir): 
			self.is_sane = False
			self.state_msg = "MISSING RESULTS DIR"
			#print ("Missing results directory for {}.".format(self.name), file = sys.stderr)
		elif not os.path.isfile(infections_filename):
			self.is_sane = False
			self.state_msg = "MISSING INFECTIONS FILE"
			#print ("Missing infections file for {}.".format(self.name), file = sys.stderr)
		elif month_of_1990 + (end_year - 1990) * 12 + 11 > duration:
			self.is_sane = False
			self.state_msg = "END MONTH IN YEAR RANGE ({}) > THAN XML DURATION ({})".format(month_of_1990 + (end_year - 1990) * 12 + 11, duration)
		else:
			try:
				infections = open(infections_filename, 'r')
			except Exception as e:
				self.is_sane = False
				self.state_msg = "CANNOT OPEN INFECTIONS FILE"
				#print ("Cannot open file {}: {}.".format(infections_filename, e), file = sys.stderr)
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
								#print ("Error reading file 1 {}. Please check it for consistency.".format(infections_filename))
								#print ("Expected month: {} Duration: {} Month: {}".format(expected_month, duration, month), file = sys.stderr)
						except ValueError as v:
							self.is_sane = False
							self.state_msg = "ERROR INFECTIONS FILE DURATION"
							return
							#print ("Error reading file 2 {}. Please check it for consistency (is the number of months {}?). {}".format(infections_filename, duration, v))
					elif expected_month == duration + 1:
						try:
							int(float(month))
							self.is_sane = False
							self.state_msg = "ERROR INFECTIONS FILE DURATION"
							return
							#print ("Error reading file 3 {}. Please check it for consistency (is the number of months {}?). {}".format(infections_filename, duration, v))
						except ValueError as v:
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
						   'ShiftedOutcomes' : 3,
						   'cepac' : 0}
		self.files = {}

		for file_type in num_header_rows:
			filename = '{}-{}.xls'.format(self.name, file_type)
			if file_type == 'cepac':
				filename = '{}.out'.format(self.name)
			full_path = os.path.join('results', filename)
			if zip_file == None:
				full_path = os.path.join(os.path.dirname(self.xml_filename), full_path)
				try:
					file = open(full_path, 'r')
				except Exception as e:
					self.is_sane = False
					self.state_msg = "CANNOT OPEN FILE " + full_path
					break
			else:
				try:
					file = zip_file.open('results/' + filename, 'r')
				except Exception as e:
					self.is_sane = False
					self.state_msg = "CANNOT OPEN ZIP FILE " + filename
					break
			if file_type == 'cepac':
				self.files[file_type] = CepacOutFile(file, self.end_month - 11)
			else:
				self.files[file_type] = TabularFile(file, num_header_rows[file_type])
		
		if self.is_sane == True:
			self.extract_infections()
			self.extract_cascade()
			self.extract_prevalence_and_incidence()
			self.extract_costs_and_lms()
			#self.initialize_status_quo()

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
		cepac_file = self.files['cepac']
		month = self.end_month
		self.statistics['Cascade']['pop-size'] = cepac_file.get_pop_size()
		self.statistics['Cascade']['num-positive'] = cepac_file.get_number_with_hiv()
		self.statistics['Cascade']['num-hiv-hiv-tested'] = cepac_file.get_number_with_hiv_hiv_tested()
		self.statistics['Cascade']['num-hiv-treated'] = cepac_file.get_number_with_hiv_treated()
		self.statistics['Cascade']['num-hiv-supressed'] = cepac_file.get_number_with_hiv_supressed()
		self.statistics['Cascade']['num-hiv-hvl-tested'] = cepac_file.get_number_with_hiv_hvl_tested()
		self.statistics['Cascade']['num-hiv-ltfu'] = cepac_file.get_number_with_hiv_ltfu()
		#self.statistics['Cascade']['blank1'] = None
		percent_tested = cepac_file.get_number_with_hiv_hiv_tested() / cepac_file.get_number_with_hiv()
		self.statistics['Cascade']['percent-tested'] = percent_tested
		percent_treated = cepac_file.get_number_with_hiv_treated() / cepac_file.get_number_with_hiv_hiv_tested()
		self.statistics['Cascade']['percent-treated'] = percent_treated
		percent_supressed = cepac_file.get_number_with_hiv_supressed() / cepac_file.get_number_with_hiv_treated()
		self.statistics['Cascade']['percent-supressed'] = percent_supressed
		self.statistics['Cascade']['total-percent-supressed'] = percent_tested * percent_treated * percent_tested
		
	def extract_prevalence_and_incidence(self):
	# This first check is to make sure we can read far enough in the files
#		at_end = False
		year = 1990
		while self.files['ShiftedOutcomes'].has_row(str(year)):
			year += 1
		self.max_year = year - 1
		sa_column = 5
		pop_column = 3
		currently_infected_column = 2
		newly_infected_column = 0

		for year in range(1990, min(self.end_year, self.max_year) + 1):
#		for year in map(str, range(1990, self.end_year + 1)):
			first_month = self.month_of_1990 + (year - 1990) * 12
#			print("Year is {}, first month is {}, start month is {}.".format(year, first_month, self.start_month), file=sys.stderr)
			monthly_sa = 0
			monthly_currently_infected = 0
			monthly_newly_infected = 0
			yearly_incidence = 0
			yearly_prevalence = 0
			
			yearly_newly_infected = 0
			yearly_sa = 0
			yearly_currently_infected = 0
# 			test_inc = self.files['ShiftedOutcomes'].get_float(str(year), 4)
# 			test_prev = self.files['ShiftedOutcomes'].get_float(str(year), 3)
#			print("First Month is: {}, year is {}.".format(first_month, year, file=sys.stderr))
#			print("STATS: month of 1990 is {}, last year is {}, last month needed is {}.".format(self.month_of_1990, min(self.end_year, self.max_year), (min(self.end_year, self.max_year) - 1990) * 12 + self.month_of_1990 + 11), file=sys.stderr)

			for month_in_year in range(first_month, first_month + 12):
				monthly_sa = self.files['Infections'].get_int(str(month_in_year), sa_column)
				monthly_currently_infected = self.files['Infections'].get_int(str(month_in_year), currently_infected_column)
				monthly_newly_infected = self.files['Infections'].get_int(str(month_in_year), newly_infected_column)
				yearly_incidence += monthly_newly_infected / (monthly_sa - monthly_currently_infected)
				if month_in_year == int(first_month) and monthly_sa > 0:
					yearly_prevalence = monthly_currently_infected / monthly_sa
					yearly_sa = monthly_sa
					yearly_currently_infected = monthly_currently_infected
					
				yearly_newly_infected += monthly_newly_infected
#				print("Looking at month {}. SA pop is {}.".format(month_in_year, self.files['Infections'].get_int(str(month_in_year), sa_column)), file=sys.stderr)
#				print("Looking at month {}. Total pop is {}.".format(month_in_year, self.files['Infections'].get_int(str(month_in_year), pop_column)), file=sys.stderr)
#				print("Looking at month {}. Currently infected is {}.".format(month_in_year, self.files['Infections'].get_int(str(month_in_year), currently_infected_column)), file=sys.stderr)
#				print("Looking at month {}. Newly Infected is {}.".format(month_in_year, self.files['Infections'].get_int(str(month_in_year), newly_infected_column)), file=sys.stderr)
#				print("End of year {}. SA pop is {}, Pop is {}, currently infected is {}, newly infected is {}.".format(year, yearly_sa, yearly_pop, yearly_currently_infected, yearly_newly_infected), file=sys.stderr)
#				print("Computing year {}: prevalence is {}, incidence is {}.".format(year, yearly_currently_infected / yearly_pop, yearly_newly_infected / (yearly_sa - yearly_currently_infected + yearly_newly_infected)), file=sys.stderr)
			self.statistics['Prevalence'][year] = yearly_prevalence
			self.statistics['Incidence'][year] = yearly_incidence
			print("Incidence for {} is:\n{} in the monthly calculations;\n{} in the 1st month calculations.\n\n".format(year, yearly_incidence, yearly_newly_infected / (yearly_sa - yearly_currently_infected)), file=sys.stderr)
#			print("Prevalence for {} is {} in the monthly calculations, {} in the 1st month calculations.".format(year, yearly_prevalence, yearly_currently_infected / yearly_sa), file=sys.stderr)
		return

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
	def __init__(self, directory):
		self.directory = directory
		self.name = os.path.basename(directory)
		self.is_sane = True
		self.state_msg = str()

	def load_runs(self, year_range_min, year_range_max, get_month_of_1990):
		subdirs = [os.path.join(self.directory, i) for i in os.listdir(self.directory) if os.path.isdir(os.path.join(self.directory, i))]
		batchdirs = [i for i in subdirs if os.path.basename(i).startswith('batch')]
		if len(batchdirs) == 0:
			batchdirs = [self.directory]
		for batchdir in batchdirs:
			for file in os.listdir(batchdir):
				if os.path.splitext(file)[1] == '.xml':
					run = Run(os.path.join(batchdir, file))
					zip_filename = os.path.join(batchdir, 'results.zip')
					zip_file = None
					if os.path.isfile(zip_filename):
						try:
							zip_file = zipfile.ZipFile(zip_filename)
						except Exception as e:
							run.is_sane = False
							run.state_msg = "ERROR OPENING ZIP FILE"
							continue
					try:
						version, duration = run.read_xml()
					except Exception as e:
						run.is_sane = False
						run.state_msg = "ERROR READING XML"
						continue
					if version not in supported_versions:
						self.is_sane = False
						self.state_msg = "WRONG XML VERSION"
						#print('Invalid version for XML {}. should be one of: {}'.format(os.path.join(batchdir, file), ', '.join(supported_versions)), file = sys.stderr)
						continue
					else: 
						month_of_1990 = get_month_of_1990(run.name)
						run.sanity_checks(duration, year_range_min, year_range_max, month_of_1990, zip_file)
						print('\t{} (sanity checks passed: {})'.format(run.name, run.is_sane))
						if run.is_sane == True:
							run.read_results(year_range_min, year_range_max, month_of_1990, zip_file)
						else:
							print("\t({})".format(run.state_msg), file = sys.stderr)
							
					yield run

class Summary:
	def __init__(self, directory, post_calib_filename, weight_cutoff, year_range, year_comparison, status_quo):
		self.directory = directory
		self.read_calibration_data(post_calib_filename, weight_cutoff)
		self.year_range_min, self.year_range_max = year_range
		self.comparison_base_year = year_comparison
		self.status_quo = status_quo
		self.status_quo_found = 0
		self.status_quo_is_sane = True
		self.status_quo_stats = Run()
		self.excluded_run_set = []
		self.processed_run_set = []
		self.skipped_run_set = dict()

		if self.comparison_base_year < self.year_range_min or self.comparison_base_year > self.year_range_max:
			print("Incorrect year comparison ({}) vs year analysis ({}-{}) range. Please change the parameters and re-run the script.".format(self.comparison_base_year, self.year_range_min, self.year_range_max)) 
			exit()
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
				'Pop Size', 'Number With HIV',
				'Number With HIV Tested', 'Number with HIV Treated',
				'Number With HIV Supressed',
				'Number With HIV HVL Tested' ,'Number With HIV LTFU',
				'% Tested', '% Treated', '% Suppressed',
				'Total % Suppressed'
			]),
			('Prevalence', [
				''
			]),
			('Incidence', [
				''
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
		stats = Run()
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
				#print ("Run Weight: {} \t Cumulative Weight: {} \t Total Weight: {}".format(self.get_weight(run.name, False), self.cumulative_weight, total_weight), file=sys.stderr)
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
						#print ("Category: {} \t Name: {} \t Run Value: {}".format(stat_category, stat, run_value), file=sys.stderr)
						
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
			while stats.max_year + 1 in stats.statistics['Incidence']:
				stats.statistics['Incidence'].popitem()
			while stats.max_year + 1 in stats.statistics['Prevalence']:
				stats.statistics['Prevalence'].popitem()
			while stats.max_year + 1 in stats.series['Incidence']:
				stats.series['Incidence'].popitem()
			while stats.max_year + 1 in stats.series['Prevalence']:
				stats.series['Prevalence'].popitem()

		for stat_category in stats.series:
			for stat in stats.series[stat_category]:
				if stats.series[stat_category][stat] == None:
					continue
				else:
					stats.median[stat_category][stat], median_position = self.wquantile(stats.series[stat_category][stat], weights, 0.5)
					stats.lower_quartile[stat_category][stat], q1_position = self.wquantile(stats.series[stat_category][stat], weights, 0.25)
					stats.upper_quartile[stat_category][stat], q3_position = self.wquantile(stats.series[stat_category][stat], weights, 0.75)
					#stats.differences[stat_category][stat] = numpy.asarray(stats.series[stat_category][stat]) - numpy.asarray(stats.series[stat_category][self.comparison_base_year])
					#stats.median[stat_category][stat] = weighted.quantile(stats.differences[stat_category][stat], weights, 0.5)
					#stats.lower_quartile[stat_category][stat] = weighted.quantile(stats.differences[stat_category][stat], weights, 0.25)
					#stats.upper_quartile[stat_category][stat] = weighted.quantile(stats.differences[stat_category][stat], weights, 0.75)
		for stat_category in stats.series:
			if stat_category in ['Incidence', 'Prevalence']:
				year_range = "{} to {}".format(self.year_range_min, self.year_range_max)
				array_indices = numpy.array(range(len(weights)))
# 				print("Stats max year is {}".format(stats.max_year), file=sys.stderr)
# 				print("stat category is {}, year is {}, stats max year is {}, length of array 1 is {}, length of array 2 is {}.".format(stat_category, year_range, stats.max_year, len(stats.series[stat_category]), len(stats.series[stat_category])), file=sys.stderr)
				stats.differences[stat_category][year_range] = numpy.asarray(stats.series[stat_category][stats.max_year]) - numpy.asarray(stats.series[stat_category][self.comparison_base_year])
				stats.statistics[stat_category][year_range] = numpy.average(stats.differences[stat_category][year_range], weights=weights)
				stats.median[stat_category][year_range], median_position = self.wquantile(stats.differences[stat_category][year_range], weights, 0.5)
				stats.lower_quartile[stat_category][year_range], q1_position =  self.wquantile(stats.differences[stat_category][year_range], weights, 0.25)
				stats.upper_quartile[stat_category][year_range], q3_position =  self.wquantile(stats.differences[stat_category][year_range], weights, 0.75)
				#median_value, median_position = self.wquantile(stats.differences[stat_category][year_range], weights, 0.5)
				#print ("The values are: {}".format(stats.differences[stat_category]), file = sys.stderr)
				#print ("The old median is {}. The new one is {}, its position is {}.".format(stats.median[stat_category][year_range], median_value, median_position), file = sys.stderr)
				stats.statistics[stat_category]["%"] = stats.statistics[stat_category][year_range] / stats.statistics[stat_category][self.comparison_base_year]
				stats.median[stat_category]["%"] = stats.median[stat_category][year_range] / stats.series[stat_category][self.comparison_base_year][median_position]
				#print ("average is of type {} and the value is {}.".format(type(stats.statistics[stat_category]["%"]), stats.statistics[stat_category]["%"]), file = sys.stderr)
				#print ("The divisor is of type {}, value {}".format(type (stats.series[stat_category][self.comparison_base_year]), stats.series[stat_category][self.comparison_base_year]), file = sys.stderr)
				stats.lower_quartile[stat_category]["%"] = stats.lower_quartile[stat_category][year_range] / stats.series[stat_category][self.comparison_base_year][q1_position]
				stats.upper_quartile[stat_category]["%"] = stats.upper_quartile[stat_category][year_range] / stats.series[stat_category][self.comparison_base_year][q3_position]

		if run_set.name == self.status_quo:
			self.status_quo_stats = stats 
			self.status_quo_is_sane = run_set.is_sane
		if self.status_quo_found == 1 and self.status_quo_is_sane == True and len(processed_runs) == self.num_runs:
			for stat in stats.statistics['Infections']:
				try:
					stats.differences['Averted'][stat] = numpy.asarray(self.status_quo_stats.series['Infections'][stat]) - numpy.asarray(stats.series['Infections'][stat])
					stats.statistics['Averted'][stat] = numpy.average(stats.differences['Averted'][stat], weights=weights)
					stats.median['Averted'][stat], median_position = self.wquantile(stats.differences['Averted'][stat], weights, 0.5)
					stats.lower_quartile['Averted'][stat], q1_position = self.wquantile(stats.differences['Averted'][stat], weights, 0.25)
					stats.upper_quartile['Averted'][stat], q3_position = self.wquantile(stats.differences['Averted'][stat], weights, 0.75)
					if stat == 'hvl-primary':
						primary_positions = (median_position, q1_position, q3_position)
					elif stat == 'total':
						total_positions = (median_position, q1_position, q3_position)
				except ValueError as e:
					print ("Couldn't calculate Averted Infections for runset {}. The skipped runs are {}, the processed ones {}, the length of the Infections array is {}.".format(run_set.name, len(skipped_runs), len(processed_runs), len(stats.series['Infections'][stat])), file = sys.stderr)
					print (e)

			# Now we calculate the averted %... Terribly clunky, but so it goes
			median_position, q1_position, q3_position = primary_positions
			stats.statistics['Averted']['primary-percent'] = stats.statistics['Averted']['hvl-primary'] / self.status_quo_stats.statistics['Infections']['hvl-primary']
			stats.median['Averted']['primary-percent'] = stats.median['Averted']['hvl-primary'] / self.status_quo_stats.series['Infections']['hvl-primary'][median_position]
			stats.lower_quartile['Averted']['primary-percent'] = stats.lower_quartile['Averted']['hvl-primary'] / self.status_quo_stats.series['Infections']['hvl-primary'][q1_position]
			stats.upper_quartile['Averted']['primary-percent'] = stats.upper_quartile['Averted']['hvl-primary'] / self.status_quo_stats.series['Infections']['hvl-primary'][q3_position]

			median_position, q1_position, q3_position = total_positions
			stats.statistics['Averted']['total-percent'] = stats.statistics['Averted']['total'] / self.status_quo_stats.statistics['Infections']['total']
			stats.median['Averted']['total-percent'] = stats.median['Averted']['total'] / self.status_quo_stats.series['Infections']['total'][median_position]
			stats.lower_quartile['Averted']['total-percent'] = stats.lower_quartile['Averted']['total'] / self.status_quo_stats.series['Infections']['total'][q1_position]
			stats.upper_quartile['Averted']['total-percent'] = stats.upper_quartile['Averted']['total'] / self.status_quo_stats.series['Infections']['total'][q3_position]
		'''
		if self.num_runs != num_runs:
			print('Number of runs processed is {}, expected {}. Are all runs present and completed in your batches?'.format(num_runs, self.num_runs))
		if self.cumulative_weight != total_weight:
			print('Total weight of all runs in {} is {}, expected {}. Are all runs present and completed in your batches?'.format(run_set.name, total_weight, self.cumulative_weight))
		'''
		return stats, processed_runs, skipped_runs

	def apply_formatting(self, runs):
		for page, header in self.headers:
			self.pages[page].bold_row(1)
			self.pages[page].bold_column(1)
			self.pages[page].color_cell('A1', 'FF0000', True)
			self.pages[page].freeze('B2')
			if page not in ['Prevalence', 'Incidence']:
				self.pages[page].column_width('A', 25)
			if page in ['Infections', 'Averted']:
				self.pages[page].row_height(1, 60)
				self.pages[page].row_height(2, 60)
				self.pages[page].merge('A1:A2')
				self.pages[page].bold_row(2)
				self.pages[page].freeze('B3')

		self.pages['Cascade'].row_height(1, 60)
		for column in ['C', 'D', 'E', 'F', 'G', 'H']:
			self.pages['Cascade'].column_width(column, 25)
		for column in range(9,15):
			self.pages['Cascade'].column_number_format(column, '0.00%')

		self.pages['LMs-Undiscounted'].row_height(1, 45)
		self.pages['LMs-Discounted'].row_height(1, 45)
		'''
		self.pages['Missing Runs'].row_height(1, 45)
		self.pages['Missing Runs'].row_height(1, 45)
		self.pages['Missing Runs'].bold_row(1)
		self.pages['Missing Runs'].bold_column(1)
		self.pages['Missing Runs'].column_width('A', 25)
		'''
	def write_incidence_and_prevalence_years(self, max_year):
		if max_year > 1989:
			for year in range(1990, max_year + 1):
				row = 2 + year - 1990
				self.pages['Prevalence'].ws.cell(column=1,row=row).value = year
				self.pages['Incidence'].ws.cell(column=1,row=row).value = year
			last_row = 2 + max_year + 1 -1990
			self.pages['Prevalence'].ws.cell(column=1,row=last_row).value = "{} to {}".format(self.comparison_base_year, max_year)
			self.pages['Incidence'].ws.cell(column=1,row=last_row).value = "{} to {}".format(self.comparison_base_year, max_year)
			self.pages['Prevalence'].ws.cell(column=1,row=last_row + 1).value = "%"
			self.pages['Incidence'].ws.cell(column=1,row=last_row + 1).value = "%"

	def summarise(self, out_filename, excludes):
		out_filename = os.path.normpath(out_filename)
		print('Reading results files and writing stats to {}.'.format(out_filename))
		print()

		wb = openpyxl.Workbook()
		wb.remove_sheet(wb['Sheet']) # workbooks contain

		self.pages = {}

		for page_name, header in self.headers:
			transposed = page_name in ['Prevalence', 'Incidence']
			self.pages[page_name] = Page(page_name, wb, transposed)
			self.pages[page_name].set_headers(header)
		# Added to have a list of the missing runs
		'''
		self.pages['Missing Runs'] = Page('Missing Runs', wb, transposed=False)
		self.pages['Missing Runs'].set_headers(['Run Name', 'Error description'])
		'''

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
			for dir in found_directories:
				if os.path.basename(dir).endswith(self.status_quo):
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
			run_set = RunSet(run_set_directory)
			
			if run_set.name in excludes and run_set.name != self.status_quo:
				print('{} is an excluded set, skipping...'.format(run_set.name), file = sys.stderr)
				print()
				self.excluded_run_set.append(run_set.name)
				continue

##			try:
			print('Averaging run set {}'.format(run_set.name))
			stats, processed_runs, skipped_runs = self.calculate_stats(run_set)
##			except Exception as e:
##				print('Skipping {}: {}'.format(run_set.name, e))
##				print()
##				continue
			for page_name, header in self.headers:
				number_format = '#,0.00'
				if page_name == 'Averted' and self.status_quo_found != 1:
					continue
				if 'Cost' in page_name:
					number_format = '$#,0.00'
				if page_name in ['Incidence', 'Prevalence']:
					number_format = '0.000000'
				self.pages[page_name].add_data('AVG ' + run_set.name, stats.statistics[page_name], number_format)
				self.pages[page_name].add_data('Q1 ' + run_set.name, stats.lower_quartile[page_name], number_format)
				self.pages[page_name].add_data('MED ' + run_set.name, stats.median[page_name], number_format)
				self.pages[page_name].add_data('Q3 ' + run_set.name, stats.upper_quartile[page_name], number_format)
				'''
				if page_name == 'Averted':
					self.pages[page_name].add_data('Q1 ' + run_set.name, stats.lower_quartile[page_name], number_format)
					self.pages[page_name].add_data('MED ' + run_set.name, stats.median[page_name], number_format)
					self.pages[page_name].add_data('Q3 ' + run_set.name, stats.upper_quartile[page_name], number_format)
				'''
			# If this is true at this point it means we have missing runs
			missing_runs_names = set()
			if self.num_runs != (len(processed_runs) + len(skipped_runs)):
			#	self.pages['Missing Runs'].add_data("{} ({})".format(run_set.name, self.num_runs - len(processed_runs)), list(set(self.weights.keys()) - processed_runs))
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
				print("Summary for {}: {} processed runs, {} skipped runs, {} missing runs.\n".format(run_set.name, len(processed_runs), len(skipped_runs), len(missing_runs_names), logname), file = sys.stderr)
			else:
				try:
					if stats.get_max_year() > max_year:
						max_year = stats.get_max_year()
				except TypeError as e:
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
				print ("Summary for {}: All runs completed successfully.\n".format(run_set.name), file = sys.stderr)

		print()
		'''
		# These checks are too aggressive, we can just skip calculating the averted infections and keep going
				if self.status_quo_found == 1:
					if self.status_quo in self.skipped_run_set:
						print("FATAL ERROR: status quo scenario is a skipped run set! Please check error log.") 
						exit()
					elif self.status_quo in self.excluded_run_set:
						print("FATAL ERROR: status quo scenario is an excluded run set! Please remove it from exclude list.") 
						exit()
		'''
		self.write_incidence_and_prevalence_years(max_year)
		self.apply_formatting(wb)
		
		try:
			wb.save(out_filename)
		except PermissionError as e:
			print ("FATAL ERROR: CANNOT SAVE EXCEL OUTFILE", file = sys.stderr)
			return
		'''
		if len (skipped_run_set) > 0:
			with open(logname, 'a') as logfile:
				print ("\nTHE FOLLOWING RUNSETS WERE SKIPPED (NAME  REASON):\n\n", file = logfile)
				for run_set_name, error in skipped_run_set.items():
					print(run_set_name + '\t\t' + error, file = logfile)
		'''
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

		print ("\nFINAL SUMMARY:\n{} processed run sets \n{} excluded run sets \n{} run sets skipped.\n".format(len(self.processed_run_set), len(self.excluded_run_set), len(self.skipped_run_set)))
		print('All stats have been written to {}.\n'.format(out_filename)) 
		if len(self.skipped_run_set) > 0:
			print ("For details on skipped run sets check {}.\n".format(logname))

def run():
	#directory = r'Z:\CEPAC - All Users\Transmission Model\Runs01_2016\PrEP_ReRuns_3.76'
	#post_calib = r'Z:\CEPAC - All Users\Transmission Model\Runs08_2015\90_90_90_Runs\30PercLRFemale_All\post calib.out'
	#out = r'Z:\CEPAC - All Users\Transmission Model\Runs01_2016\PrEP_ReRuns_3.76\analyse_batches_2030_PrEP__FINAL.xlsx'
	if len(sys.argv) != 4:
		print ("usage: analyse_batches.py target_dir post_calib_file output_file", file = sys.stderr)
		return
	directory = sys.argv[1]
	post_calib = sys.argv[2]
	out = sys.argv[3]
	'''
	The following parameters can be manually changed by the user
	'''
	weight = float(0.9) # Weight cutoff for the calibration file
	year_range = (2014, 2059) # Years of interest for our analysis
	year_comparison = 2014 # base year for computing the differences to rank in  quartiles 
	excludes = ["85PerSudDecBef2016_50K", "3PerIncBef2016_50K"]  # run sets to be excluded from our analysis (useful when the script is executed on very large folders
	status_quo = ""#"BaseCase_50K"

	Summary(directory, post_calib, weight, year_range, year_comparison, status_quo).summarise(out, excludes)
	
if __name__ == '__main__':
	run()
