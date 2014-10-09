#!/bin/bash

# only do this on Mac OS X
if [ `uname -s` != "Darwin" ]; then
    exit 0;
fi

if [[ ! `ifconfig lo0` =~ 127\.0\.0\.2 ]]; then
    sudo ifconfig lo0 127.0.0.2/8 alias
    sudo ifconfig lo0 127.0.0.3/8 alias
    sudo ifconfig lo0 127.0.0.4/8 alias
    sudo ifconfig lo0 127.0.0.5/8 alias
fi 
