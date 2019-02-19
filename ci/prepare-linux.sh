#!/bin/bash

sudo setcap cap_net_raw+ep src/fping

if [[ ! $PATH =~ fping/src ]]; then
    PATH=/home/dws/checkouts/fping/src:$PATH
fi
