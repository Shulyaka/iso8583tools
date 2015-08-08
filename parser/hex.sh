#!/bin/bash

sed -e 's/.*:  //' -e 's/  .*//' | xxd -r -ps | ./testparse
