#!/usr/bin/env bash

# https://neovim.discourse.group/t/how-to-automatically-change-between-light-and-dark-themes-on-linux-and-gnome3/2111/3
IFS=$'\n'
for s in $(nvr --serverlist); do
    test ! -S "$s" && continue
    nvr --nostart --servername "$s" -c "lua Scheme:set('${1}')"
done
