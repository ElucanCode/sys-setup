#!/usr/bin/env bash
#
# default case $1 = light
FROM="dark"
TO="light"
if [[ "$1" = "dark" ]]; then
    TO="dark"
    FROM="light"
fi

sed -i "s/${FROM}.qbtheme/${TO}.qbtheme/" "${HOME}/.config/qBittorrent/qBittorrent.conf"
