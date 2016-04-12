import os
import sys
import fnmatch
import xml.etree.ElementTree as ET
import time

'''
We are trying to find all the copies of some potentially compromised XML files so that we
can check their month of 1990 against its correct value: if they do not match we let the
user know where they are by printing their full path.
'''

batches_list = {"Cal_Batch25_774_" : 677, 
			"Cal_Batch17_827_" : 635, 
			"Cal_Batch13_401_" : 668, 
			"Cal_Batch20_979_" : 650, 
			"Cal_Batch16_975_" : 683}

def find_all_batches(batches, root):
	results = []
	for path, dirs, files in os.walk(root):
		for file in files:
			for name, value in batches.items():
				if fnmatch.fnmatch(file, name + '*.xml'):
					results.append((name, os.path.join(path, file)))
					#print (name, os.path.join(path, file))
	return results

def check_batches (batches, files):
	faulty_batches = []
	for match_name, match_path in files:
		with open (match_path, 'r') as f:
			try:
				tree = ET.parse(f)
			except ET.ParseError as process_xml_except:
				print ("Error parsing the file {}.".format(match_path))
				print (process_xml_except)
			else:
				root = tree.getroot()
				if root.findall('monthOf1990'):
					if root.findall('.//monthOf1990')[0].text != str(batches[match_name]):
						print ("FOUND FAULTY BATCH FILE: " + match_path)
						print ("The value is {} instead of {}.".format (root.find("monthOf1990").text, str(batches[match_name])))
						faulty_batches.append(match_path)
	return faulty_batches

if (len(sys.argv) == 1):
	print ("Using current directory as base dir for the search.")
	root_dir = os.getcwd()
elif os.path.isdir(sys.argv[1]):
	root_dir = sys.argv[1]
	print ("Using {} as base dir for the search.".format(sys.argv[1]))
else:
	print ("Argument must be the target directory. Use without arguments for current dir.", file = sys.stderr)
	sys.exit(0)
		
file_matches = find_all_batches(batches_list, root_dir)
print ("Found " + str(len(file_matches)) + " potential matches.")

final_matches = check_batches (batches_list, file_matches)
if len(file_matches) > 0:
	print ("Found {:d} faulty batches ({:.2f}%) out of {:d}.".format(len(final_matches), float(len(final_matches)/len(file_matches))*100, len(file_matches)))

# Let's save everything to a file, just in case
if (len(sys.argv) == 1):
	dir_name = os.path.basename(os.getcwd())
else:
	dir_name = root_dir
timestr = time.strftime("%Y%m%d-%H%M%S")
out_file_name = dir_name + '_faulty_' + timestr 
with open(out_file_name, 'w') as out_file:
	for item in final_matches:
		out_file.write("{}\n".format(item))
	print ("Results saved into file {} in the current directory.".format(out_file_name))

	