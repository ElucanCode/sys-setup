#!/usr/bin/env bash

# check if x server is running
# if ! timeout 1s xset q &>/dev/null; then
#     echo "No X-Server running" >&2
#     exit 1
# fi

case "$1" in
    light) xsetroot -name "fsignal:1" ;;
    dark)  xsetroot -name "fsignal:2" ;;
esac
# dwm-cmd "theme" "$1"
