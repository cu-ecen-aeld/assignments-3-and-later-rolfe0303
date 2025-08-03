#!/bin/bash

#Path to file argument
writefile=$1
#String to write argument
writestr=$2

if [ $# -lt 2 ]
then
    echo "Error: Missing argument"
    echo "Argument list: "
    echo "1: writefile : path to file in which to write"
    echo "2: writestr : string to write to file"
    exit 1
fi

filedirectory=$(dirname $writefile)
# Check if directory exists
if [ ! -d $filedirectory ]
then
    #Create the directory
    mkdir -p $filedirectory
    #Check for errors
    if [ $? -gt 0 ]
    then
        #Error creating the directory
        echo "Directory does not exist and cannot be created"
        exit 1
    fi
fi

#Check if file exists
if [ ! -e $writefile ]
then
    #File does not exist, create it
    touch $writefile
    if [ $? -gt 0 ]
    then
        # File couldn't be created
        echo "File does not exist and cannot be created"
        exit 1
    fi
fi

#Check if file is writtable
if [ ! -w $writefile ]
then
    # File is not writtable
    echo "The file exists, but can't be written"
    exit 1
fi

# All checks are good, write to file
echo "$writestr" > $writefile
