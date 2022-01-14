#!/bin/bash

#checking for number of arguments entered
if [ $# -lt 2 ] 
then
	echo Error. Arguements are missing.
	exit 1
fi

if [ $# -gt 2 ] 
then
	echo Error. More arguments provided than required.
	exit 1
fi

#assigining values to variables
filesdir=$1
searchstr=$2

echo filesdir: $filesdir, searchstr: $searchstr

#checking if the files directory is valid or not
if [ -d "$filesdir" ]
then
	#assign values to X as number of files
	X="$(find "$filesdir" -type f | wc -l)"
	#assign value to Y as number of matches
	Y="$(grep -rnw "$filesdir" -e "$searchstr" | wc -l)"
else

	echo Invalid directory
	exit 1
fi

#printing the result
echo The number of files are $X and the number of matching lines are $Y



