#!/bin/bash

if ! /usr/sbin/getcap src/fping | grep -q "cap_net_admin,cap_net_raw=ep"; then
    sudo setcap cap_net_raw,cap_net_admin+ep src/fping
fi

if [ -d "$PWD/src" ]; then
    if [[ ":$PATH:" != *":$PWD/src:"* ]]; then
        PATH="$PWD/src:$PATH"
    fi
fi
