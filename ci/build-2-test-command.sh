#!/bin/sh

set -ex

curl -L http://cpanmin.us | perl - -L $HOME/perl5 App::cpanminus
$HOME/perl5/bin/cpanm --sudo Test::Command
