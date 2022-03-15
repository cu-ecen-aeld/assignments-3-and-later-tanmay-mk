#1/bin/bash

#
# file name	: finder.sh
#
# Author	: tanmay-mk
#
# Date		: 13 January 2022 
#

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
writefile=$1
writestr=$2

#exctract name of file
name_file="${writefile##*/}"
#extract name of directory
name_directory="${writefile%/*}"


#if directory does not exist, we have to create a new one
if ! [ -d "$name_directory" ]
then
	echo $name_directory Directory does not exist.
	echo Creating new directory.
	mkdir $name_directory
fi

#create a new file
touch "$writefile"

#if file is created correctly, write to the file
if [ -f "$writefile" ]
then
	#writing writestr to file created
	echo "$writestr" > "$writefile"
	exit 0 #successful operation
else
	#else print an error message
	echo Error creating file.
	exit 1 #failed operation
fi

## EOF
