#!/bin/sh

set -ex

curl -L http://cpanmin.us | perl - --sudo App::cpanminus
cpanm --sudo Test::Command

