#!/bin/sh

if [ ! -d ci ]; then
    echo "you must run this in the root fping directory" >&2
    exit 1
fi


sudo /opt/fping/sbin/fping 127.0.0.1
