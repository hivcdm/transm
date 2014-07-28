generate_runs is a Python script that will eventually integrate all of the post-processing functionality required to create XMLs according to a defined collection of parameter sets and a given template. It is used by specifying a template file and a name for the generated runs (will be added as a suffix to the run names). In the created folder, you will find 564 new XMLs based on the template with all of the calibration parameters replaced. 

There are two optional parameters, batch_size and weight_cutoff. If not set to 0, batch_size determines the number of XMLs to place into each subfolder (similar to the make batches script). By default, the XMLs are not batched into subfolders. The weight_cutoff parameter determines the inclusive range of cumulative weights to generate XMLs for. "0.9" means generate the top 90% of runs. "0.9" is the default value.

To run it, call it like this:

python generate_runs.py 35_example.xml example

To specify the optional parameters, call it like this:

python generate_runs.py batch_size=5 weight_cutoff=0.99 35_example.xml example

There are a few other parameters that cannot be changed through command line arguments that are found in the script at the top. These are:

parameters_filename - The file to load calibration parameter sets from. This should be in XLSX format for now.
post_calib_filename - The file to search for the month of 1990 from. This should be in XLSX format for now.
earliest_end_year - If not set to 0, duration for all XMLs will be increased so that the model continues at least until the start of the specified year relative to the calibrated month of 1990. A value of 2100 with a month of 1990 of 725 would yield a duration of 2045.

Both of the required files are included with the script.

One final note: This script requires a python library called openpyxl to read the Excel spreadsheets. I might have installed this on your computers, I can't remember. If it's not installed, you can install it using pip like this:

pip install openpyxl

If you also don't have pip, follow the directions here to get it (once you've installed this, installing new libraries for python will me much easier!): http://pip.readthedocs.org/en/latest/installing.html