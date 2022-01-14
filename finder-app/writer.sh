#1/bin/bash

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

name_file="${writefile##*/}"
name_directory="${writefile%/*}"


#if directory does not exist, we have to create a new one
if ! [ -d "$name_directory" ]
then
	echo $name_directory Directory does not exist.
	echo Creating new directory.
	mkdir $name_directory
fi

touch "$writefile"

if [ -f "$writefile" ]
then
	#writing writestr to file created
	echo "$writestr" > "$writefile"
fi
