#!/usr/bin/env bash

# check if x server is running
# if ! timeout 1s xset q &>/dev/null; then
#     echo "No X-Server running" >&2
#     exit 1
# fi

case "$1" in
    dark)  redshift -P -O 4500 ;;
    light) redshift -P -O 6500 ;;
esac
