#!/bin/bash
# Script to find files which match given string

# Files directory
filesdir=$1
# String to search for
searchstr=$2

if [ $# -lt 2 ]
then
    echo "Error: Missing argument"
    echo "Argument list: "
    echo "1: filesdir : path to directory in which to perform the search"
    echo "2: searchstr : string to search for"
    exit 1
fi

if [ ! -d "$filesdir" ]
then
    echo "Error: filesdir does not exist or is not a directory"
    exit 1
fi

number_files=$( find $filesdir -type f | wc -l )
number_lines=$( grep "$searchstr" $filesdir/* | wc -l )

echo "The number of files are ${number_files} and the number of matching lines are $number_lines"
