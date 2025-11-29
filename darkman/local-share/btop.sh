#!/usr/bin/env bash

FROM="dark"
TO="light"
if [[ "$1" = "dark" ]]; then
    TO="dark"
    FROM="light"
fi

sed -i "s/${FROM}.theme/${TO}.theme/" "${HOME}/.config/btop/btop.conf"
