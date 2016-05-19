#!/bin/bash

for dir in ./*; do
	# if not a directory, skip
	[ -d "${dir}" ] || continue
	
	echo "Now executing the script on $dir..."
	(python3 "$1" $dir "$2" "$(basename $dir)-prep.xls")
done
