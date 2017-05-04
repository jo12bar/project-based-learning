#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

gcc -Wall -o "${DIR}/lsh" "${DIR}/src/main.c"
