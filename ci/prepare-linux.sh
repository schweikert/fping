#!/bin/bash

sudo setcap cap_net_raw,cap_net_admin+ep src/fping

if [ -d "$PWD/src" ]; then
    if [[ ":$PATH:" != *":$PWD/src:"* ]]; then
        PATH="$PWD/src:$PATH"
    fi
fi
