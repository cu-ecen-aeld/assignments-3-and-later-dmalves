#!/bin/bash

if [ "$#" -ne 2 ]; then
        echo "Wrong number of parameters."
        exit 1
fi


writefile=$1
writestr=$2

base=`dirname $writefile`

mkdir -p $base

echo $writestr > $writefile
