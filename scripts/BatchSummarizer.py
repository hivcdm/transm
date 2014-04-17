import csv
import os

class TabularData(object):
    
    POST_CALIB_MONTHS_OF_1990 = ['A', 'B', 'E']
    
    def __init__(self, columns=None):
        self.columns = columns

    def parse_row(self, row):
        if not self.columns:
            return row[:]
        row_data = []
        for column in self.columns:
            row_data.append(float(row[TabularData.column_to_index(column)]))
        return row_data

    def parse_csv(self, file, num_header_rows=1):
        reader = csv.reader(file, delimiter='\t')
        i = 0
        for row in reader:
            i += 1
            if i <= num_header_rows:
                continue
            yield row

    def column_to_index(column):
        if type(column) != str:
            return column
        value = [ord(i) - ord('a') for i in column.lower()]
        total = 0
        for i in range(len(column)):
            total += 26 ** (len(column) - 1 - i) * (value[i] + 1)
        return total - 1

class MonthlyOutcomes(TabularData):
    
    INFECTIONS_INCIDENT_BY_HVL = ['BL', 'BM', 'BN', 'BO','BP', 'BQ', 'BR', 'BS', 'BT']
    POPULATION_SIZES_BY_GENDER_AGE = ['AD', 'AE', 'AF', 'AG', 'AH', 'AI', 'AJ', 'AK', 'AL', 'AN', 'AO', 'AP', 'AQ', 'AR', 'AS', 'AT', 'AU', 'AV']
    PREVALENT_CASES_BY_GENDER_AGE = ['V', 'W', 'X', 'Y', 'Z', 'AA', 'AB', 'AC', 'AD', 'AF', 'AG', 'AH', 'AI', 'AJ', 'AK', 'AL', 'AM', 'AN']
    
    def __init__(self, columns, start_year=None, end_year=None):
        TabularData.__init__(self, columns)
        self.start_year = start_year
        self.end_year = end_year

    def monthly_summary(self, file, month_of_1990):
        reader = csv.reader(file, delimiter='\t')
        header = True
        for row in reader:
            if header:
                if row[0] == '1':
                    header = False
                else:
                    continue
            try:
                month = int(row[0])
                if not self.start_year or month >= month_of_1990 + (self.start_year - 1990) * 12:
                    if not self.end_year or month <= month_of_1990 + (self.end_year - 1990) * 12 + 11:
                        yield month, self.parse_row(row)
            except:
                break

    def yearly_summary(self, file, month_of_1990):
        last_year = None
        year_total = None
        for month, row in self.monthly_summary(file, month_of_1990):
            year = self.start_year + (month - month_of_1990) // 12
            if year == last_year or not last_year:
                if not year_total:
                    year_total = row[:]
                else:
                    year_total = [i + j for i, j in zip(year_total, row)]
            else:
                yield year, year_total
                year_total = None
            last_year = year

    def overall_summary(self, file, month_of_1990):
        total = None
        for month, row in self.parse_monthly(file, month_of_1990):
            total = [i + j for i, j in zip(total, row)] if total else row[:]
        return total

class BatchSummarizer(object):
    def __init__(self, directory, start_year=None, end_year=None):
        self.directory = directory
        self.start_year = start_year
        self.end_year = end_year
        self.read_post_calib(os.path.join(directory, 'post calib_sigma1.out'))
        
    def find_all_matches(self, suffix, recursive=False, directory=None):
        if not directory:
            directory = self.directory
        all_files = os.listdir(directory)
        matches = [os.path.join(directory, i) for i in all_files if i.endswith(suffix) and os.path.isfile(os.path.join(directory, i))]
        if recursive:
            for subdirectory in [os.path.join(directory, i) for i in all_files if os.path.isdir(os.path.join(directory, i))]:
                for match in self.find_all_matches(suffix, True, subdirectory):
                    matches.append(match)
        return matches

    def get_root(self, filename):
        return '_'.join(os.path.basename(filename).split('_')[:3]).split('-')[0]

    def read_post_calib(self, post_calib_filename):
        self.months_of_1990 = {}
        self.run_weights = {}
        weights = []
        self.top_90 = []
        try:
            with open(post_calib_filename) as post_calib:
                for row in TabularData(TabularData.POST_CALIB_MONTHS_OF_1990).parse_csv(post_calib):
                    if row[4] != 'None':
                        self.months_of_1990[row[0]] = int(row[1])
                        self.run_weights[row[0]] = float(row[4])
                        weights.append((row[0], float(row[4])))
        except FileNotFoundError as e:
            raise Exception('post calib file not found: ' + post_calib_filename) from e
        cumulative_weight = 0
        for run_weight in reversed(sorted(weights, key=lambda x: x[1])):
            cumulative_weight += run_weight[1]
            self.top_90.append(run_weight[0])
            print(cumulative_weight)
            if cumulative_weight > 0.9:
                break

    def summarize_incident_infections_by_hvl(self):
        print('Summarizing number of incident infections stratified by HVL of infector...', end='')
        incident_by_hvl_parser = MonthlyOutcomes(MonthlyOutcomes.INFECTIONS_INCIDENT_BY_HVL, 2000, 2010)
        for filename in find_all_matches(directory, 'Infections.xls', True):
            with open(filename) as csv_file:
                root = self.get_root(filename)
                month_of_1990 = self.months_of_1990[root]
                for year, year_total in incident_by_hvl_parser.yearly_summary(csv_file, month_of_1990):
                    print(year, year_total)
        print('done.')
        print('Results written to {}.'.format(os.path.join(self.directory, 'infections.out')))

    def summarize_prevalence_by_gender_age(self):
        print('Summarizing life expectancies...', end='')

        print(self.top_90)
        
        out_filename = os.path.join(self.directory, 'prevalence by age summary.out')

        age_ranges = ['0-203', '204-239', '240-299', '300-359', '360-419', '420-479', '480-539', '540-599', '600-1211']
        
        sa_age_range_sizes_by_run = {}
        prevalent_cases_by_run = {}
        total_prevalence = [0 for i in range(self.end_year-self.start_year+1)]
        male_prevalence = [0 for i in range(self.end_year-self.start_year+1)]
        female_prevalence = [0 for i in range(self.end_year-self.start_year+1)]

        male_age_prevalence = { j : [0 for i in range(self.end_year-self.start_year+1)] for j in age_ranges }
        female_age_prevalence = { j : [0 for i in range(self.end_year-self.start_year+1)] for j in age_ranges }

        print('reading population trace...', end='')
        population_parser = MonthlyOutcomes(MonthlyOutcomes.POPULATION_SIZES_BY_GENDER_AGE, self.start_year, self.end_year)
        fc = 0
        for filename in self.find_all_matches('Population.xls', True):
            #print(filename)
            root = self.get_root(filename)
            #print(root)
            if root not in self.top_90:
                continue
            month_of_1990 = self.months_of_1990[root]
            name = os.path.basename(filename)[:-len('-Population.xls')]
            with open(filename) as csv_file:
                try:
                    sa_age_range_sizes_by_run[name] = []
                    i = 0
                    for month, monthly_pop_sizes in population_parser.monthly_summary(csv_file, month_of_1990):
                        if i % 12 == 0:
                            sa_age_range_sizes_by_run[name].append((month, monthly_pop_sizes))
                        i += 1
                except:
                    print('bad pop file:',filename)
                    del sa_age_range_sizes_by_run[name]

        #print('reading infections trace...', end='')
        infections_parser = MonthlyOutcomes(MonthlyOutcomes.PREVALENT_CASES_BY_GENDER_AGE, self.start_year, self.end_year)
        fc = 0
        for filename in self.find_all_matches('Infections.xls', True):
            #print(filename)
            name = os.path.basename(filename)[:-len('-Infections.xls')]
            root = self.get_root(filename)
            if root not in self.top_90:
                continue
            month_of_1990 = self.months_of_1990[root]
            with open(filename) as csv_file:
                try:
                    prevalent_cases_by_run[name] = []
                    i = 0
                    for month, monthly_infections in infections_parser.monthly_summary(csv_file, month_of_1990):
                        if i % 12 == 0:
                            prevalent_cases_by_run[name].append((month, monthly_infections))
                        i += 1
                except:
                    print('bad inf file:', filename)
                    del prevalent_cases_by_run[name]

        headers = ['summary'] + [str(i) for i in range(self.start_year, self.end_year)]

        print('writing yearly prevalence...', end='')
        with open(out_filename, 'w') as out_file:
            out_file.write('\t'.join(headers) + '\n')

            sum_weight = 0
            for run in prevalent_cases_by_run:
                root = self.get_root(run)
                if root not in self.top_90:
                    continue
                sum_weight += self.run_weights[root]

            print('total weight:', sum_weight)
                
            for run in prevalent_cases_by_run:
                root = self.get_root(filename)
                if root not in self.top_90:
                    continue
                weight = self.run_weights[root]
                try:
                    pop_sizes = sa_age_range_sizes_by_run[run]
                    prevalent_cases = prevalent_cases_by_run[run]
                    year = self.start_year
                    for (pop_month, year_pop_size), (inf_month, year_prevalent_cases) in zip(pop_sizes, prevalent_cases):
                        #print(run,weight,year,year_pop_size[1],year_prevalent_cases[1])
                        if pop_month != inf_month:
                            print('script error for', run, 'months should match', pop_month, inf_month)
                            return
                        
                        male_pop_size = sum(self.split_list(year_pop_size)[0])
                        male_prevalent_cases = sum(self.split_list(year_prevalent_cases)[0])
                        male_prevalence[year - self.start_year] += weight / sum_weight * male_prevalent_cases / male_pop_size if male_pop_size > 0 else 0
                        female_pop_size = sum(self.split_list(year_pop_size)[1])
                        female_prevalent_cases = sum(self.split_list(year_prevalent_cases)[1])
                        female_prevalence[year - self.start_year] += weight / sum_weight * female_prevalent_cases / female_pop_size if female_pop_size > 0 else 0
                        total_pop_size = male_pop_size+female_pop_size
                        total_prevalent_cases = male_prevalent_cases+ female_prevalent_cases
                        total_prevalence[year - self.start_year] += weight / sum_weight * total_prevalent_cases / total_pop_size if total_pop_size > 0 else 0
                        for i in range(len(age_ranges)):
                            age_range_pop_size = self.split_list(year_pop_size)[0][i]
                            age_range_prevalent_cases = self.split_list(year_prevalent_cases)[0][i]
                            male_age_prevalence[age_ranges[i]][year - self.start_year] += weight / sum_weight * age_range_prevalent_cases / age_range_pop_size if age_range_pop_size > 0 else 0
                            age_range_pop_size = self.split_list(year_pop_size)[1][i]
                            age_range_prevalent_cases = self.split_list(year_prevalent_cases)[1][i]
                            female_age_prevalence[age_ranges[i]][year - self.start_year] += weight / sum_weight * age_range_prevalent_cases / age_range_pop_size if age_range_pop_size > 0 else 0
                        year += 1
                        if year > self.end_year:
                            break
                except Exception as e:
                    print('problem writing run:', run, e)
            out_file.write('\t'.join(['weighted total prevalence'] + [str(i) for i in total_prevalence]) + '\n')
            
            out_file.write('\t'.join(['weighted male prevalence'] + [str(i) for i in male_prevalence]) + '\n')
            for age_range in age_ranges:
                age_range_prevalence = male_age_prevalence[age_range]
                out_file.write('\t'.join(['weighted male prevalence {}'.format(age_range)] + [str(i) for i in age_range_prevalence]) + '\n')
                
            out_file.write('\t'.join(['weighted female prevalence'] + [str(i) for i in female_prevalence]) + '\n')
            for age_range in age_ranges:
                age_range_prevalence = female_age_prevalence[age_range]
                out_file.write('\t'.join(['weighted female prevalence {}'.format(age_range)] + [str(i) for i in age_range_prevalence]) + '\n')
            
        print('done.')
        print('Results written to {}.'.format(out_filename))

    def split_list(self, l):
        return l[:len(l)//2], l[len(l)//2:]

    def summarize_life_expectancy(self):
        print('Summarizing life expectancies...', end='')
        life_expectancy_parser = TabularData()
        out_filename = os.path.join(directory, 'le summary.out')
        with open(out_filename, 'w') as out_file:
            out_file.write('\t'.join(['run', 'time1', 'time2', 'time3', 'time4', 'time5']) + '\n')
            for filename in find_all_matches(directory, 'LE.xls', True):
                with open(filename) as csv_file:
                    row_index = 0
                    life_expectancies = []
                    for row in life_expectancy_parser.parse_csv(csv_file, 0):
                        row_index += 1
                        if row_index % 103 == 3:
                            life_expectancies.append(row[9])
                    out_file.write('\t'.join([os.path.basename(filename)] + list(map(str, life_expectancies))) + '\n')
        print('done.')
        print('Results written to {}.'.format(out_filename))

def is_thomass_computer():
    import socket
    return socket.gethostname() == 'HSPH-FV2YPN1'

def main(directory):
    if is_thomass_computer():
        directory = r'C:\Users\taf656\Downloads\SequenceTests (1)\SequenceTests'
        
    start_year = 2013
    end_year = 2063
    
    BatchSummarizer(directory, start_year, end_year).summarize_prevalence_by_gender_age()

if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1:
        main(sys.argv[1])
    else:
        main(os.getcwd())
