#!/bin/bash

if [ $# -eq 1 ]
then
	cat $1 | $0
	exit $?
fi

sed -e 's/.*:  //' -e 's/  .*//' | xxd -r -ps | ./testparse
