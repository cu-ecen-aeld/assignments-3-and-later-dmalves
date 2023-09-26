#!/bin/bash

if [ "$#" -ne 2 ]; then
	echo "Wrong number of parameters."
	exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d "$filesdir" ]; then
	echo "$filesdir is not a directory"
	exit 1
fi

X=`find $filesdir -type f | wc -l`
Y=`find $filesdir -type f | xargs cat | grep $searchstr | wc -l`

echo "The number of files are $X and the number of matching lines are $Y"
