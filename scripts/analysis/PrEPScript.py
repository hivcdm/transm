import collections # for OrderedDict
import openpyxl # for creating xlsx
import os
import zipfile
import sys

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
		for element in data:
			self.ws.cell(row=row, column=column).value = element
			if cell_format != None and column != 1:
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
		
		if xml_filename != '':
			self.name = os.path.splitext(os.path.basename(xml_filename))[0]

		self.statistics = ['Stratified Outcomes']
		self.statistics = collections.OrderedDict([(i, collections.OrderedDict()) for i in self.statistics])

		self.max_year = 0

	def get_max_year(self):
		return self.max_year

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
		return version, duration

	def is_complete(self, duration, zip_file=None):
		infections_filename = '{}-Infections.xls'.format(self.name)
		if zip_file:
			infections = zip_file.open('results/' + infections_filename, 'r')
		else:
			directory = os.path.dirname(self.xml_filename)
			results_dir = os.path.join(directory, 'results')
			if not os.path.isdir(results_dir): return False
			infections_filename = os.path.join(results_dir, '{}-Infections.xls'.format(self.name))
			if not os.path.isfile(infections_filename): return False
			infections = open(infections_filename, 'r')
	
		expected_month = 0
		for i, line in enumerate(infections):
			if i < 3: continue
			if isinstance(line, bytes):
				line = line.decode('utf8')
			month = line.split('\t')[0]
			if month == 'init' and expected_month != 0:
				return False
			if expected_month > 0 and expected_month <= duration:
				if int(month) != expected_month:
					return False
			if expected_month > duration:
				return month.split()[0] == 'Statistics'
			expected_month += 1
			
		return False

	def read_results(self, start_year, end_year, month_of_1990, zip_file):
		self.start_year = start_year
		self.start_month = month_of_1990 + (self.start_year - 1990) * 12
		self.end_year = end_year
		self.end_month = month_of_1990 + (self.end_year - 1990) * 12 + 11
		self.month_of_1990 = month_of_1990
		
		num_header_rows = {'CE' : 4,
						   'Infections' : 3,
						   'ShiftedOutcomes' : 3,
						   'ARTRollout' : 3,
						   'Population' : 2,
						   'cepac' : 0}
		self.files = {}

		for file_type in num_header_rows:
			filename = '{}-{}.xls'.format(self.name, file_type)
			
			if file_type == 'cepac':
				filename = '{}.out'.format(self.name)
			
			full_path = os.path.join('results', filename)
			
			if zip_file == None:
				full_path = os.path.join(os.path.dirname(self.xml_filename), full_path)
				file = open(full_path, 'r')
			else:
				file = zip_file.open('results/' + filename, 'r')
			
			if file_type == 'cepac':
				self.files[file_type] = CepacOutFile(file, self.end_month - 11)
			else:
				self.files[file_type] = TabularFile(file, num_header_rows[file_type])

		self.extract_stratified_outcomes()
		
	def extract_stratified_outcomes(self):
		stat_types = [
			'Population',
			'Using PrEP',
			'Receiving ART',
			'Infected',
			'Incidence Rate',
			'Prevalence'
		]
		
		stratifications = [
			'Males 0-16',
			'Males 17-19',
			'Males 20-24',
			'Males 25-29',
			'Males 30-34',
			'Males 35-39',
			'Males 40-44',
			'Males 45-49',
			'Males 50+',
			'Females 0-16',
			'Females 17-19',
			'Females 20-24',
			'Females 25-29',
			'Females 30-34',
			'Females 35-39',
			'Females 40-44',
			'Females 45-49',
			'Females 50+'
		]

		for year in map(str, range(self.start_year, self.end_year + 1)):
			self.statistics['Stratified Outcomes'][year] = collections.OrderedDict()
			for stat in [i + ' ' + j for i in stat_types for j in ['Total', 'Total Males', 'Total Females'] + stratifications]:
				self.statistics['Stratified Outcomes'][year][stat] = 0
	   
		for year in map(str, range(self.start_year, self.end_year + 1)):
			month = str((int(year) - 1990) * 12 + self.month_of_1990)
			
			sa_total = 0
			sa_males = 0
			sa_females = 0
			infected_total = 0
			infected_males = 0
			infected_females = 0
						
			stratified_sa = collections.OrderedDict()
			stratified_prevalent = collections.OrderedDict()
			stratified_incident = collections.OrderedDict()
			
			
			for i, stratification in enumerate(stratifications):
				pop_column = 28 + i

				if i > 8:
					pop_column = 38 + (i - 9)

				prep_column = 181 + i

				if i > 8:
					prep_column = 191 + (i - 9)

				treatment_column = 146 + i

				if i > 8:
					treatment_column = 156 + (i - 9)

				infected_column = 20 + i

				if i > 8:
					infected_column = 30 + (i - 9)

				incident_column = 88 + i

				if i > 8:
					incident_column = 97 + (i - 9)
					
				num_people = self.files['Population'].get_float(month, pop_column)
				num_using_prep = self.files['ARTRollout'].get_float(month, prep_column)
				num_on_treatment = self.files['ARTRollout'].get_float(month, treatment_column)
				num_infected = self.files['Infections'].get_float(month, infected_column)

				incidence_rate = 0
				
				female_monthly_sa = collections.deque()
				female_monthly_prevalent = collections.deque()
				female_monthly_incident = collections.deque()
				male_monthly_sa = collections.deque()
				male_monthly_prevalent = collections.deque()
				male_monthly_incident = collections.deque()
				
				monthly_sa_queue = collections.deque()
				monthly_prevalent_queue = collections.deque()
				monthly_incident_queue = collections.deque()
				
				for incidence_month in range(int(month), int(month) + 12):
					#print("Reading month {}. First month is {}, year is {}.".format(incidence_month, month, year), file=sys.stderr)
					monthly_sa = self.files['Population'].get_float(str(incidence_month), pop_column)
					monthly_prevalent = self.files['Infections'].get_float(str(incidence_month), infected_column)
					monthly_incident = self.files['Infections'].get_float(str(incidence_month), incident_column)
					monthly_incidence_rate = monthly_incident / (monthly_sa - monthly_prevalent) if monthly_sa > 0 else 0
					incidence_rate += monthly_incidence_rate
					
					monthly_sa_queue.append(monthly_sa)
					monthly_prevalent_queue.append(monthly_prevalent)
					monthly_incident_queue.append(monthly_incident)
					
				stratified_sa[stratification] = monthly_sa_queue
				stratified_prevalent[stratification] = monthly_prevalent_queue
				stratified_incident[stratification] = monthly_incident_queue
				
				prevalence_rate = num_infected / num_people if num_people > 0 else 0
			
				self.statistics['Stratified Outcomes'][year]['Population ' + stratification] = num_people
				self.statistics['Stratified Outcomes'][year]['Using PrEP ' + stratification] = num_using_prep
				self.statistics['Stratified Outcomes'][year]['Receiving ART ' + stratification] = num_on_treatment
				self.statistics['Stratified Outcomes'][year]['Infected ' + stratification] = num_infected
				self.statistics['Stratified Outcomes'][year]['Incidence Rate ' + stratification] = incidence_rate
				self.statistics['Stratified Outcomes'][year]['Prevalence ' + stratification] = prevalence_rate

				if stratification.startswith('Female'):
					sa_females += num_people
					infected_females += num_infected
					self.statistics['Stratified Outcomes'][year]['Population Total Females'] += num_people
					self.statistics['Stratified Outcomes'][year]['Using PrEP Total Females'] += num_using_prep
					self.statistics['Stratified Outcomes'][year]['Receiving ART Total Females'] += num_on_treatment
					self.statistics['Stratified Outcomes'][year]['Infected Total Females'] += num_infected
				else:
					sa_males += num_people
					infected_males += num_infected
					self.statistics['Stratified Outcomes'][year]['Population Total Males'] += num_people
					self.statistics['Stratified Outcomes'][year]['Using PrEP Total Males'] += num_using_prep
					self.statistics['Stratified Outcomes'][year]['Receiving ART Total Males'] += num_on_treatment
					self.statistics['Stratified Outcomes'][year]['Infected Total Males'] += num_infected

				self.statistics['Stratified Outcomes'][year]['Population Total'] += num_people
				self.statistics['Stratified Outcomes'][year]['Using PrEP Total'] += num_using_prep
				self.statistics['Stratified Outcomes'][year]['Receiving ART Total'] += num_on_treatment
				self.statistics['Stratified Outcomes'][year]['Infected Total'] += num_infected

				sa_total += num_people
				infected_total += num_infected

			incidence_rate_males = 0
			incidence_rate_females = 0
			incidence_rate_total = 0
			
			for month in range(0,12):
				male_monthly_sa = 0
				male_monthly_prevalent = 0
				male_monthly_incident = 0
				female_monthly_sa = 0
				female_monthly_prevalent = 0
				female_monthly_incident = 0
				total_monthly_sa = 0
				total_monthly_prevalent = 0
				total_monthly_incident = 0
				
				for stratification in stratifications:
					monthly_sa += stratified_sa[stratification].popleft()
					monthly_prevalent += stratified_prevalent[stratification].popleft()
					monthly_incident += stratified_incident[stratification].popleft()
					total_monthly_sa += monthly_sa
					total_monthly_prevalent += monthly_prevalent
					total_monthly_incident += monthly_incident
					
					if stratification.startswith('Female'):
						female_monthly_sa += monthly_sa
						female_monthly_prevalent += monthly_prevalent
						female_monthly_incident += monthly_incident
					else:
						male_monthly_sa += monthly_sa
						male_monthly_prevalent += monthly_prevalent
						male_monthly_incident += monthly_incident
				
				incidence_rate_males += male_monthly_incident / (male_monthly_sa - male_monthly_prevalent) if male_monthly_sa > 0 else 0
				incidence_rate_females += female_monthly_incident / (female_monthly_sa - female_monthly_prevalent) if female_monthly_sa > 0 else 0
				incidence_rate_total += total_monthly_incident / (total_monthly_sa - total_monthly_prevalent) if total_monthly_sa > 0 else 0
# 				print("Year is {}, month is {}.".format(year, month), file=sys.stderr)
# 				print("Total incidence is {}, male incidence is {}, female incidence is {}.".format(incidence_rate_total, incidence_rate_males, incidence_rate_females), file=sys.stderr)
# 				print("SA male is {}, SA female is {}, SA total is {}.".format(male_monthly_sa, female_monthly_sa, total_monthly_sa), file=sys.stderr)
# 				print("Incident male is {}, Incident female is {}, Incident total is {}.".format(male_monthly_incident, female_monthly_incident, total_monthly_incident), file=sys.stderr)
# 				print("Prevalent male is {}, Prevalent female is {}, Prevalent total is {}.".format(male_monthly_prevalent, female_monthly_prevalent, total_monthly_prevalent), file=sys.stderr)
			
			self.statistics['Stratified Outcomes'][year]['Incidence Rate Total Males'] = incidence_rate_males
			self.statistics['Stratified Outcomes'][year]['Prevalence Total Males'] = infected_males / sa_males

			self.statistics['Stratified Outcomes'][year]['Incidence Rate Total Females'] = incidence_rate_females
			self.statistics['Stratified Outcomes'][year]['Prevalence Total Females'] = infected_females / sa_females
			
			self.statistics['Stratified Outcomes'][year]['Incidence Rate Total'] = incidence_rate_total
			self.statistics['Stratified Outcomes'][year]['Prevalence Total'] = infected_total / sa_total


class RunSet:
	def __init__(self, directory):
		self.directory = directory
		self.name = os.path.basename(directory)

	def load_runs(self, year_range, get_month_of_1990):
		subdirs = [os.path.join(self.directory, i) for i in os.listdir(self.directory) if os.path.isdir(os.path.join(self.directory, i))]
		batchdirs = [i for i in subdirs if os.path.basename(i).startswith('batch')]
		if len(batchdirs) == 0:
			batchdirs = [self.directory]
		for batchdir in batchdirs:
			for file in os.listdir(batchdir):
				if os.path.splitext(file)[1] == '.xml':
					run = Run(os.path.join(batchdir, file))
					version, duration = run.read_xml()
					if version not in supported_versions:
						raise Exception('invalid version for XML {}. should be one of: {}'.format(os.path.join(batchdir, file), ', '.join(supported_versions)))
					zip_filename = os.path.join(batchdir, 'results.zip')
					zip_file = None
					if os.path.isfile(zip_filename):
						zip_file = zipfile.ZipFile(zip_filename)
					if not run.is_complete(duration, zip_file):
						raise Exception('incomplete results for XML {}'.format(os.path.join(batchdir, file)))
					month_of_1990 = get_month_of_1990(run.name)
					run.read_results(year_range[0], year_range[1], month_of_1990, zip_file)
					yield run

class Summary:
	def __init__(self, directory, post_calib_filename, weight_cutoff, year_range):
		self.directory = directory
		self.read_calibration_data(post_calib_filename, weight_cutoff)
		self.year_range = year_range

		self.headers = [
			('Stratified Outcomes', [
				[
					''] +
					['Population'] + [''] * 20 +
					['Using PrEP'] + [''] * 20 +
					['Receiving ART'] + [''] * 20 +
					['Infected'] + [''] * 20 +
					['Incidence Rate'] + [''] * 20 +
					['Prevalence']
				,
				['Year'] +
				[
					'Total',
					'Total Males',
					'Total Females',
					'Males 0-16',
					'Males 17-19',
					'Males 20-24',
					'Males 25-29',
					'Males 30-34',
					'Males 35-39',
					'Males 40-44',
					'Males 45-49',
					'Males 50+',
					'Females 0-16',
					'Females 17-19',
					'Females 20-24',
					'Females 25-29',
					'Females 30-34',
					'Females 35-39',
					'Females 40-44',
					'Females 45-49',
					'Females 50+'
				] * 6
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
		if 'standard' in name: name = 'Cal_Batch1_7602'
		if exact_only:
			if name not in options:
				raise Exception('exact run name not found in post calib file: {}'.format(name))
			return options[name]
		possible = [i for i in options if name.startswith(i)]
		for i in possible:
			suffix = name[len(i):]
			if len(suffix) == 0:
				return options[i]
			if suffix[0] not in '0123456789':
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

	def calculate_average(self, run_set):
		average = Run()
		average.max_year = None
		total_weight = 0
		num_runs = 0
		
		for run in run_set.load_runs(self.year_range, lambda r: self.get_month_of_1990(r, False)):
			print('\t{}'.format(run.name))
			
			weight = self.get_weight(run.name, False)
			total_weight += weight
			num_runs += 1
			
			if average.max_year == None:
				average.max_year = run.max_year
			else:
				average.max_year = min(run.max_year, average.max_year)

			for stat_category in run.statistics:
				for year in map(str, range(self.year_range[0], self.year_range[1] + 1)):
					if year not in average.statistics[stat_category]:
						average.statistics[stat_category][year] = collections.OrderedDict()
					for stat in run.statistics[stat_category][year]:
						if stat not in average.statistics[stat_category][year]:
							average.statistics[stat_category][year][stat] = 0
							
						run_value = run.statistics[stat_category][year][stat]
						
						if run_value == None:
							average.statistics[stat_category][year][stat] = None
						else:
							average.statistics[stat_category][year][stat] += weight / self.cumulative_weight * run_value

		if self.num_runs != num_runs:
			print('Number of runs processed is {}, expected {}. Are all runs present and completed in your batches?'.format(num_runs, self.num_runs))
		elif abs(self.cumulative_weight - total_weight) > 1e-6:
			print('Total weight of all runs in batch is {}, expected {}. Are all runs present and completed in your batches?'.format(total_weight, self.cumulative_weight))

		return average

	def apply_formatting(self, runs):
		for page, header in self.headers:
			self.pages[page].bold_row(1)
			self.pages[page].bold_column(1)
			self.pages[page].color_cell('A1', 'FF0000', True)
			self.pages[page].freeze('B2')
			if page not in ['Prevalence', 'Incidence']:
				self.pages[page].column_width('A', 25)

	def summarise(self, out_filename, excludes):
		out_filename = os.path.normpath(out_filename)
		print('Reading results files and writing averages to {}.'.format(out_filename))
		print()

		wb = openpyxl.Workbook()
		wb.remove_sheet(wb['Sheet']) # workbooks contain

		self.pages = {}

		for page_name, header in self.headers:
			transposed = page_name in ['Prevalence', 'Incidence']
			self.pages[page_name] = Page(page_name, wb, transposed)
			self.pages[page_name].set_headers(header)

		max_year = 1989

		for run_set_directory in self.find_run_sets(self.directory):
			run_set = RunSet(run_set_directory)
			
			if run_set.name in excludes:
				print('{} is an excluded set, skipping...'.format(run_set.name))
				print()
				continue
			
##			try:
			print('Averaging run set {}'.format(run_set.name))
			average = self.calculate_average(run_set)
##			except Exception as e:
##				print('Skipping {}: {}'.format(run_set.name, e))
##				print()
##				continue
			
			for page_name, header in self.headers:
				number_format = '0.000000'
				for year in average.statistics[page_name]:
					self.pages[page_name].add_data(year, average.statistics[page_name][year], number_format)
				
			if average.get_max_year() > max_year:
				max_year = average.get_max_year()
				
			try:
				wb.save(out_filename)
			except PermissionError as e:
				print('There was a problem saving to {}. Is it open in Excel?'.format(out_filename))

			print()

		print()

		self.apply_formatting(wb)

		try:
			wb.save(out_filename)
		except PermissionError as e:
			print('There was a problem saving to {}. Is it open in Excel?'.format(out_filename))
			return
			
		print('Done.')

def run():
	if len(sys.argv) != 4:
		print ("usage: analyse_batches.py target_dir post_calib_file output_file", file = sys.stderr)
		return
	directory = sys.argv[1]
	post_calib = sys.argv[2]
	out = sys.argv[3]
	weight = 0.9
	year_range = (2014, 2059)
	excludes = ["85PerSudDecBef2016_50K", "3PerIncBef2016_50K"]
	Summary(directory, post_calib, weight, year_range).summarise(out, excludes)
	
if __name__ == '__main__':
	run()
